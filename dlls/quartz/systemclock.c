/*
 * Implementation of IReferenceClock
 *
 * Copyright 2004 Raphael Junqueira
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "quartz_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static int cookie_counter;

struct advise_sink
{
    struct list entry;
    HANDLE handle;
    REFERENCE_TIME due_time, period;
    int cookie;
};

static DWORD WINAPI advise_thread(void *param)
{
    REFERENCE_TIME current_time, next_time;
    struct reference_clock *clock = param;
    struct advise_sink *sink, *cursor;

    TRACE("Starting advise thread for clock %p.\n", clock);

    for (;;)
    {
        current_time = clock->ops->clock_get_time(clock);
        next_time = LLONG_MAX;

        EnterCriticalSection(&clock->cs);

        LIST_FOR_EACH_ENTRY_SAFE(sink, cursor, &clock->sinks, struct advise_sink, entry)
        {
            if (sink->due_time <= current_time)
            {
                if (sink->period)
                {
                    DWORD periods = ((current_time - sink->due_time) / sink->period) + 1;
                    ReleaseSemaphore(sink->handle, periods, NULL);
                    sink->due_time += periods * sink->period;
                }
                else
                {
                    SetEvent(sink->handle);
                    list_remove(&sink->entry);
                    heap_free(sink);
                    continue;
                }
            }

            next_time = min(next_time, sink->due_time);
        }

        LeaveCriticalSection(&clock->cs);

        if (!clock->ops->clock_wait_time(clock, next_time))
            return 0;
    }
}

static void notify_thread(struct reference_clock *clock)
{
    if (!InterlockedCompareExchange(&clock->thread_created, TRUE, FALSE))
    {
        clock->notify_event = CreateEventW(NULL, FALSE, FALSE, NULL);
        clock->stop_event = CreateEventW(NULL, TRUE, FALSE, NULL);
        clock->thread = CreateThread(NULL, 0, advise_thread, clock, 0, NULL);
    }
    SetEvent(clock->notify_event);
}

HRESULT reference_clock_get_time(struct reference_clock *clock, REFERENCE_TIME *time)
{
    REFERENCE_TIME ret;
    HRESULT hr;

    TRACE("clock %p, time %p.\n", clock, time);

    if (!time)
        return E_POINTER;

    ret = clock->ops->clock_get_time(clock);

    EnterCriticalSection(&clock->cs);

    hr = (ret == clock->last_time) ? S_FALSE : S_OK;
    *time = clock->last_time = ret;

    LeaveCriticalSection(&clock->cs);

    TRACE("Returning %s.\n", debugstr_time(ret));

    return hr;
}

HRESULT reference_clock_advise(struct reference_clock *clock,
        REFERENCE_TIME time, HEVENT event, DWORD_PTR *cookie)
{
    struct advise_sink *sink;

    TRACE("clock %p, time %s, event %#lx, cookie %p.\n",
            clock, debugstr_time(time), event, cookie);

    if (!event)
        return E_INVALIDARG;

    if (time <= 0)
        return E_INVALIDARG;

    if (!cookie)
        return E_POINTER;

    if (!(sink = heap_alloc_zero(sizeof(*sink))))
        return E_OUTOFMEMORY;

    sink->handle = (HANDLE)event;
    sink->due_time = time;
    sink->period = 0;
    sink->cookie = InterlockedIncrement(&cookie_counter);

    EnterCriticalSection(&clock->cs);
    list_add_tail(&clock->sinks, &sink->entry);
    LeaveCriticalSection(&clock->cs);

    notify_thread(clock);

    *cookie = sink->cookie;
    return S_OK;
}

HRESULT reference_clock_advise_periodic(struct reference_clock *clock,
        REFERENCE_TIME start, REFERENCE_TIME period, HSEMAPHORE semaphore, DWORD_PTR *cookie)
{
    struct advise_sink *sink;

    TRACE("clock %p, start %s, period %s, semaphore %#lx, cookie %p.\n",
            clock, debugstr_time(start), debugstr_time(period), semaphore, cookie);

    if (!semaphore)
        return E_INVALIDARG;

    if (start <= 0 || period <= 0)
        return E_INVALIDARG;

    if (!cookie)
        return E_POINTER;

    if (!(sink = heap_alloc_zero(sizeof(*sink))))
        return E_OUTOFMEMORY;

    sink->handle = (HANDLE)semaphore;
    sink->due_time = start;
    sink->period = period;
    sink->cookie = InterlockedIncrement(&cookie_counter);

    EnterCriticalSection(&clock->cs);
    list_add_tail(&clock->sinks, &sink->entry);
    LeaveCriticalSection(&clock->cs);

    notify_thread(clock);

    *cookie = sink->cookie;
    return S_OK;
}

HRESULT reference_clock_unadvise(struct reference_clock *clock, DWORD_PTR cookie)
{
    struct advise_sink *sink;

    TRACE("clock %p, cookie %#lx.\n", clock, cookie);

    EnterCriticalSection(&clock->cs);

    LIST_FOR_EACH_ENTRY(sink, &clock->sinks, struct advise_sink, entry)
    {
        if (sink->cookie == cookie)
        {
            list_remove(&sink->entry);
            heap_free(sink);
            LeaveCriticalSection(&clock->cs);
            return S_OK;
        }
    }

    LeaveCriticalSection(&clock->cs);

    return S_FALSE;
}

void reference_clock_init(struct reference_clock *clock, const struct reference_clock_ops *ops)
{
    memset(clock, 0, sizeof(*clock));
    clock->ops = ops;
    list_init(&clock->sinks);
    InitializeCriticalSection(&clock->cs);
    clock->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": reference_clock.cs");
}

void reference_clock_cleanup(struct reference_clock *clock)
{
    if (clock->thread)
    {
        SetEvent(clock->stop_event);
        WaitForSingleObject(clock->thread, INFINITE);
        CloseHandle(clock->thread);
        CloseHandle(clock->notify_event);
        CloseHandle(clock->stop_event);
    }
    clock->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&clock->cs);
}

struct system_clock
{
    struct reference_clock clock;

    IReferenceClock IReferenceClock_iface;
    IUnknown IUnknown_inner;
    IUnknown *outer_unk;
    LONG refcount;
};

static inline struct system_clock *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct system_clock, IUnknown_inner);
}

static HRESULT WINAPI system_clock_inner_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct system_clock *clock = impl_from_IUnknown(iface);
    TRACE("clock %p, iid %s, out %p.\n", clock, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown))
        *out = iface;
    else if (IsEqualGUID(iid, &IID_IReferenceClock))
        *out = &clock->IReferenceClock_iface;
    else
    {
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI system_clock_inner_AddRef(IUnknown *iface)
{
    struct system_clock *clock = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&clock->refcount);

    TRACE("%p increasing refcount to %u.\n", clock, refcount);

    return refcount;
}

static ULONG WINAPI system_clock_inner_Release(IUnknown *iface)
{
    struct system_clock *clock = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&clock->refcount);

    TRACE("%p decreasing refcount to %u.\n", clock, refcount);

    if (!refcount)
    {
        reference_clock_cleanup(&clock->clock);
        heap_free(clock);
    }
    return refcount;
}

static const IUnknownVtbl system_clock_inner_vtbl =
{
    system_clock_inner_QueryInterface,
    system_clock_inner_AddRef,
    system_clock_inner_Release,
};

static inline struct system_clock *impl_from_IReferenceClock(IReferenceClock *iface)
{
    return CONTAINING_RECORD(iface, struct system_clock, IReferenceClock_iface);
}

static HRESULT WINAPI system_clock_QueryInterface(IReferenceClock *iface, REFIID iid, void **out)
{
    struct system_clock *clock = impl_from_IReferenceClock(iface);
    return IUnknown_QueryInterface(clock->outer_unk, iid, out);
}

static ULONG WINAPI system_clock_AddRef(IReferenceClock *iface)
{
    struct system_clock *clock = impl_from_IReferenceClock(iface);
    return IUnknown_AddRef(clock->outer_unk);
}

static ULONG WINAPI system_clock_Release(IReferenceClock *iface)
{
    struct system_clock *clock = impl_from_IReferenceClock(iface);
    return IUnknown_Release(clock->outer_unk);
}

static HRESULT WINAPI system_clock_GetTime(IReferenceClock *iface, REFERENCE_TIME *time)
{
    struct system_clock *clock = impl_from_IReferenceClock(iface);

    TRACE("clock %p, time %p.\n", clock, time);

    return reference_clock_get_time(&clock->clock, time);
}

static HRESULT WINAPI system_clock_AdviseTime(IReferenceClock *iface,
        REFERENCE_TIME base, REFERENCE_TIME offset, HEVENT event, DWORD_PTR *cookie)
{
    struct system_clock *clock = impl_from_IReferenceClock(iface);

    TRACE("clock %p, base %s, offset %s, event %#lx, cookie %p.\n",
            clock, debugstr_time(base), debugstr_time(offset), event, cookie);

    return reference_clock_advise(&clock->clock, base + offset, event, cookie);
}

static HRESULT WINAPI system_clock_AdvisePeriodic(IReferenceClock* iface,
        REFERENCE_TIME start, REFERENCE_TIME period, HSEMAPHORE semaphore, DWORD_PTR *cookie)
{
    struct system_clock *clock = impl_from_IReferenceClock(iface);

    TRACE("clock %p, start %s, period %s, semaphore %#lx, cookie %p.\n",
            clock, debugstr_time(start), debugstr_time(period), semaphore, cookie);

    return reference_clock_advise_periodic(&clock->clock, start, period, semaphore, cookie);
}

static HRESULT WINAPI system_clock_Unadvise(IReferenceClock *iface, DWORD_PTR cookie)
{
    struct system_clock *clock = impl_from_IReferenceClock(iface);

    TRACE("clock %p, cookie %#lx.\n", clock, cookie);

    return reference_clock_unadvise(&clock->clock, cookie);
}

static const IReferenceClockVtbl system_clock_vtbl =
{
    system_clock_QueryInterface,
    system_clock_AddRef,
    system_clock_Release,
    system_clock_GetTime,
    system_clock_AdviseTime,
    system_clock_AdvisePeriodic,
    system_clock_Unadvise,
};

static REFERENCE_TIME system_clock_get_time(struct reference_clock *clock)
{
    return GetTickCount64() * 10000;
}

static BOOL system_clock_wait_time(struct reference_clock *clock, REFERENCE_TIME time)
{
    HANDLE handles[2] = {clock->stop_event, clock->notify_event};
    DWORD timeout;

    if (time == LLONG_MAX)
        timeout = INFINITE;
    else
        timeout = max(time / 10000 - GetTickCount64(), 0);

    return WaitForMultipleObjects(2, handles, FALSE, timeout) != 0;
}

static const struct reference_clock_ops system_clock_ops =
{
    system_clock_get_time,
    system_clock_wait_time,
};

HRESULT system_clock_create(IUnknown *outer, void **out)
{
    struct system_clock *object;
  
    TRACE("outer %p, out %p.\n", outer, out);

    if (!(object = heap_alloc_zero(sizeof(*object))))
    {
        *out = NULL;
        return E_OUTOFMEMORY;
    }

    object->IReferenceClock_iface.lpVtbl = &system_clock_vtbl;
    object->IUnknown_inner.lpVtbl = &system_clock_inner_vtbl;
    object->outer_unk = outer ? outer : &object->IUnknown_inner;
    object->refcount = 1;
    reference_clock_init(&object->clock, &system_clock_ops);

    TRACE("Created system clock %p.\n", object);
    *out = &object->IUnknown_inner;

    return S_OK;
}
