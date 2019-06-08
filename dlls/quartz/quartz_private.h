/*              DirectShow private interfaces (QUARTZ.DLL)
 *
 * Copyright 2002 Lionel Ulmer
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

#ifndef __QUARTZ_PRIVATE_INCLUDED__
#define __QUARTZ_PRIVATE_INCLUDED__

#include <limits.h>
#include <stdarg.h>
#include <wchar.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "wingdi.h"
#include "winuser.h"
#include "dshow.h"
#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/strmbase.h"
#include "wine/list.h"

static inline const char *debugstr_fourcc(DWORD fourcc)
{
    if (!fourcc) return "''";
    return wine_dbg_sprintf("'%c%c%c%c'", (char)(fourcc), (char)(fourcc >> 8),
            (char)(fourcc >> 16), (char)(fourcc >> 24));
}

static inline const char *debugstr_time(REFERENCE_TIME time)
{
    unsigned int i = 0, j = 0;
    char buffer[22], rev[22];

    while (time || i <= 8)
    {
        buffer[i++] = '0' + (time % 10);
        time /= 10;
        if (i == 7) buffer[i++] = '.';
    }

    while (i--) rev[j++] = buffer[i];
    rev[j] = 0;

    return wine_dbg_sprintf("%s", rev);
}

/* see IAsyncReader::Request on MSDN for the explanation of this */
#define MEDIATIME_FROM_BYTES(x) ((LONGLONG)(x) * 10000000)
#define SEC_FROM_MEDIATIME(time) ((time) / 10000000)
#define BYTES_FROM_MEDIATIME(time) SEC_FROM_MEDIATIME(time)
#define MSEC_FROM_MEDIATIME(time) ((time) / 10000)

HRESULT FilterGraph_create(IUnknown *pUnkOuter, LPVOID *ppObj) DECLSPEC_HIDDEN;
HRESULT FilterGraphNoThread_create(IUnknown *pUnkOuter, LPVOID *ppObj) DECLSPEC_HIDDEN;
HRESULT FilterMapper2_create(IUnknown *pUnkOuter, LPVOID *ppObj) DECLSPEC_HIDDEN;
HRESULT FilterMapper_create(IUnknown *pUnkOuter, LPVOID *ppObj) DECLSPEC_HIDDEN;
HRESULT AsyncReader_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT StdMemAllocator_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT AVISplitter_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT MPEGSplitter_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT AVIDec_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT dsound_render_create(IUnknown *outer, void **out) DECLSPEC_HIDDEN;
HRESULT VideoRenderer_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT VideoRendererDefault_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT system_clock_create(IUnknown *outer, void **out) DECLSPEC_HIDDEN;
HRESULT ACMWrapper_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT WAVEParser_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT VMR7Impl_create(IUnknown *pUnkOuter, LPVOID *ppv) DECLSPEC_HIDDEN;
HRESULT VMR9Impl_create(IUnknown *pUnkOuter, LPVOID *ppv) DECLSPEC_HIDDEN;

HRESULT EnumMonikerImpl_Create(IMoniker ** ppMoniker, ULONG nMonikerCount, IEnumMoniker ** ppEnum) DECLSPEC_HIDDEN;

HRESULT IEnumRegFiltersImpl_Construct(REGFILTER * pInRegFilters, const ULONG size, IEnumRegFilters ** ppEnum) DECLSPEC_HIDDEN;

extern const char * qzdebugstr_guid(const GUID * id) DECLSPEC_HIDDEN;
extern void video_unregister_windowclass(void) DECLSPEC_HIDDEN;

BOOL CompareMediaTypes(const AM_MEDIA_TYPE * pmt1, const AM_MEDIA_TYPE * pmt2, BOOL bWildcards);
void dump_AM_MEDIA_TYPE(const AM_MEDIA_TYPE * pmt) DECLSPEC_HIDDEN;

BOOL get_media_type(const WCHAR *filename, GUID *majortype, GUID *subtype, GUID *source_clsid) DECLSPEC_HIDDEN;

struct reference_clock;

struct reference_clock_ops
{
    /* Get the current time. */
    REFERENCE_TIME (*clock_get_time)(struct reference_clock *clock);
    /* Wait until the "time" or one of the clock's events is signaled. If "time"
     * is LLONG_MAX, wait forever. Return FALSE if stop_event was signaled,
     * TRUE otherwise. */
    BOOL (*clock_wait_time)(struct reference_clock *clock, REFERENCE_TIME time);
};

struct reference_clock
{
    const struct reference_clock_ops *ops;

    BOOL thread_created;
    HANDLE thread, notify_event, stop_event;
    REFERENCE_TIME last_time;
    CRITICAL_SECTION cs;

    struct list sinks;
};

void reference_clock_init(struct reference_clock *clock, const struct reference_clock_ops *ops) DECLSPEC_HIDDEN;
void reference_clock_cleanup(struct reference_clock *clock) DECLSPEC_HIDDEN;

HRESULT reference_clock_get_time(struct reference_clock *clock, REFERENCE_TIME *time) DECLSPEC_HIDDEN;
HRESULT reference_clock_advise(struct reference_clock *clock, REFERENCE_TIME time, HEVENT event, DWORD_PTR *cookie) DECLSPEC_HIDDEN;
HRESULT reference_clock_advise_periodic(struct reference_clock *clock, REFERENCE_TIME time, REFERENCE_TIME offset,
        HEVENT event, DWORD_PTR *cookie) DECLSPEC_HIDDEN;
HRESULT reference_clock_unadvise(struct reference_clock *clock, DWORD_PTR cookie) DECLSPEC_HIDDEN;

#endif /* __QUARTZ_PRIVATE_INCLUDED__ */
