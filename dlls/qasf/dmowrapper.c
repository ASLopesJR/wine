/*
 * DMO wrapper filter
 *
 * Copyright (C) 2019 Zebediah Figura
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

#include "qasf_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(qasf);

struct dmo_wrapper
{
    struct strmbase_filter filter;
    IDMOWrapperFilter IDMOWrapperFilter_iface;

    IUnknown *dmo;

    DWORD sink_count, source_count;
    struct strmbase_sink *sinks;
    struct strmbase_source *sources;
};

static inline struct dmo_wrapper *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_wrapper, filter);
}

static inline struct strmbase_sink *impl_sink_from_strmbase_pin(struct strmbase_pin *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_sink, pin);
}

static HRESULT sink_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct strmbase_sink *sink = impl_sink_from_strmbase_pin(iface);

    if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &sink->IMemInputPin_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT sink_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    struct dmo_wrapper *filter = impl_from_strmbase_filter(iface->filter);
    IMediaObject *dmo;
    HRESULT hr;

    IUnknown_QueryInterface(filter->dmo, &IID_IMediaObject, (void **)&dmo);

    hr = IMediaObject_SetInputType(dmo, impl_sink_from_strmbase_pin(iface) - filter->sinks,
            (const DMO_MEDIA_TYPE *)mt, DMO_SET_TYPEF_TEST_ONLY);

    IMediaObject_Release(dmo);

    return hr;
}

static HRESULT sink_get_media_type(struct strmbase_pin *iface, unsigned int index, AM_MEDIA_TYPE *mt)
{
    struct dmo_wrapper *filter = impl_from_strmbase_filter(iface->filter);
    IMediaObject *dmo;
    HRESULT hr;

    IUnknown_QueryInterface(filter->dmo, &IID_IMediaObject, (void **)&dmo);

    hr = IMediaObject_GetInputType(dmo, impl_sink_from_strmbase_pin(iface) - filter->sinks,
            index, (DMO_MEDIA_TYPE *)mt);

    IMediaObject_Release(dmo);

    return hr == S_OK ? S_OK : VFW_S_NO_MORE_ITEMS;
}

static const struct strmbase_sink_ops sink_ops =
{
    .base.pin_query_interface = sink_query_interface,
    .base.pin_query_accept = sink_query_accept,
    .base.pin_get_media_type = sink_get_media_type,
};

static inline struct strmbase_source *impl_source_from_strmbase_pin(struct strmbase_pin *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_source, pin);
}

static HRESULT source_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    struct dmo_wrapper *filter = impl_from_strmbase_filter(iface->filter);
    IMediaObject *dmo;
    HRESULT hr;

    IUnknown_QueryInterface(filter->dmo, &IID_IMediaObject, (void **)&dmo);

    hr = IMediaObject_SetOutputType(dmo, impl_source_from_strmbase_pin(iface) - filter->sources,
            (const DMO_MEDIA_TYPE *)mt, DMO_SET_TYPEF_TEST_ONLY);

    IMediaObject_Release(dmo);

    return hr;
}

static HRESULT source_get_media_type(struct strmbase_pin *iface, unsigned int index, AM_MEDIA_TYPE *mt)
{
    struct dmo_wrapper *filter = impl_from_strmbase_filter(iface->filter);
    IMediaObject *dmo;
    HRESULT hr;

    IUnknown_QueryInterface(filter->dmo, &IID_IMediaObject, (void **)&dmo);

    hr = IMediaObject_GetOutputType(dmo, impl_source_from_strmbase_pin(iface) - filter->sources,
            index, (DMO_MEDIA_TYPE *)mt);

    IMediaObject_Release(dmo);

    return hr == S_OK ? S_OK : VFW_S_NO_MORE_ITEMS;
}

static const struct strmbase_source_ops source_ops =
{
    .base.pin_query_accept = source_query_accept,
    .base.pin_get_media_type = source_get_media_type,
};

static inline struct dmo_wrapper *impl_from_IDMOWrapperFilter(IDMOWrapperFilter *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_wrapper, IDMOWrapperFilter_iface);
}

static HRESULT WINAPI dmo_wrapper_filter_QueryInterface(IDMOWrapperFilter *iface, REFIID iid, void **out)
{
    struct dmo_wrapper *filter = impl_from_IDMOWrapperFilter(iface);
    return IUnknown_QueryInterface(filter->filter.outer_unk, iid, out);
}

static ULONG WINAPI dmo_wrapper_filter_AddRef(IDMOWrapperFilter *iface)
{
    struct dmo_wrapper *filter = impl_from_IDMOWrapperFilter(iface);
    return IUnknown_AddRef(filter->filter.outer_unk);
}

static ULONG WINAPI dmo_wrapper_filter_Release(IDMOWrapperFilter *iface)
{
    struct dmo_wrapper *filter = impl_from_IDMOWrapperFilter(iface);
    return IUnknown_Release(filter->filter.outer_unk);
}

static HRESULT WINAPI dmo_wrapper_filter_Init(IDMOWrapperFilter *iface, REFCLSID clsid, REFCLSID category)
{
    struct dmo_wrapper *filter = impl_from_IDMOWrapperFilter(iface);
    DWORD input_count, output_count;
    struct strmbase_source *sources;
    struct strmbase_sink *sinks;
    IMediaObject *dmo;
    IUnknown *unk;
    WCHAR id[14];
    HRESULT hr;
    DWORD i;

    TRACE("filter %p, clsid %s, category %s.\n", filter, debugstr_guid(clsid), debugstr_guid(category));

    if (FAILED(hr = CoCreateInstance(clsid, &filter->filter.IUnknown_inner,
            CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk)))
        return hr;

    if (FAILED(hr = IUnknown_QueryInterface(unk, &IID_IMediaObject, (void **)&dmo)))
    {
        IUnknown_Release(unk);
        return hr;
    }

    if (FAILED(IMediaObject_GetStreamCount(dmo, &input_count, &output_count)))
        input_count = output_count = 0;

    if (!(sinks = heap_alloc_zero(sizeof(*sinks) * input_count)))
    {
        IMediaObject_Release(dmo);
        IUnknown_Release(unk);
        return hr;
    }

    if (!(sources = heap_alloc_zero(sizeof(*sources) * output_count)))
    {
        heap_free(sinks);
        IMediaObject_Release(dmo);
        IUnknown_Release(unk);
        return hr;
    }

    for (i = 0; i < input_count; ++i)
    {
        swprintf(id, ARRAY_SIZE(id), L"in%u", i);
        strmbase_sink_init(&sinks[i], &filter->filter, id, &sink_ops, NULL);
    }

    for (i = 0; i < output_count; ++i)
    {
        swprintf(id, ARRAY_SIZE(id), L"out%u", i);
        strmbase_source_init(&sources[i], &filter->filter, id, &source_ops);
    }

    EnterCriticalSection(&filter->filter.csFilter);

    filter->dmo = unk;
    filter->sink_count = input_count;
    filter->source_count = output_count;
    filter->sinks = sinks;
    filter->sources = sources;

    LeaveCriticalSection(&filter->filter.csFilter);

    IMediaObject_Release(dmo);

    return S_OK;
}

static const IDMOWrapperFilterVtbl dmo_wrapper_filter_vtbl =
{
    dmo_wrapper_filter_QueryInterface,
    dmo_wrapper_filter_AddRef,
    dmo_wrapper_filter_Release,
    dmo_wrapper_filter_Init,
};

static IPin *dmo_wrapper_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct dmo_wrapper *filter = impl_from_strmbase_filter(iface);

    if (index < filter->sink_count)
        return &filter->sinks[index].pin.IPin_iface;
    else if (index < filter->sink_count + filter->source_count)
        return &filter->sources[index - filter->sink_count].pin.IPin_iface;
    return NULL;
}

static void dmo_wrapper_destroy(struct strmbase_filter *iface)
{
    struct dmo_wrapper *filter = impl_from_strmbase_filter(iface);
    DWORD i;

    if (filter->dmo)
        IUnknown_Release(filter->dmo);
    for (i = 0; i < filter->sink_count; ++i)
        strmbase_sink_cleanup(&filter->sinks[i]);
    for (i = 0; i < filter->source_count; ++i)
        strmbase_source_cleanup(&filter->sources[i]);
    strmbase_filter_cleanup(&filter->filter);
    heap_free(filter);
}

static HRESULT dmo_wrapper_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    struct dmo_wrapper *filter = impl_from_strmbase_filter(iface);

    if (IsEqualGUID(iid, &IID_IDMOWrapperFilter))
    {
        *out = &filter->IDMOWrapperFilter_iface;
        IUnknown_AddRef((IUnknown *)*out);
        return S_OK;
    }

    if (filter->dmo && !IsEqualGUID(iid, &IID_IUnknown))
        return IUnknown_QueryInterface(filter->dmo, iid, out);
    return E_NOINTERFACE;
}

static struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = dmo_wrapper_get_pin,
    .filter_destroy = dmo_wrapper_destroy,
    .filter_query_interface = dmo_wrapper_query_interface,
};

HRESULT dmo_wrapper_create(IUnknown *outer, REFIID iid, void **out)
{
    struct dmo_wrapper *object;
    HRESULT hr;

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDMOWrapperFilter_iface.lpVtbl = &dmo_wrapper_filter_vtbl;

    /* Always pass NULL as the outer object; see test_aggregation(). */
    strmbase_filter_init(&object->filter, NULL, &CLSID_DMOWrapperFilter, &filter_ops);

    hr = IUnknown_QueryInterface(&object->filter.IUnknown_inner, iid, out);
    IUnknown_Release(&object->filter.IUnknown_inner);
    return hr;
}
