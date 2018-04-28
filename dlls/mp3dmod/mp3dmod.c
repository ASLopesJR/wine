/*
 * MP3 decoder DMO
 *
 * Copyright 2018 Zebediah Figura
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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#define COBJMACROS
#include "objbase.h"
#include "rpcproxy.h"
#include "wmcodecdsp.h"
#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(mp3dmod);

static HINSTANCE mp3dmod_instance;

struct mp3_decoder {
    IMediaObject IMediaObject_iface;
    LONG ref;
};

static inline struct mp3_decoder *impl_from_IMediaObject(IMediaObject *iface)
{
    return CONTAINING_RECORD(iface, struct mp3_decoder, IMediaObject_iface);
}

static HRESULT WINAPI MediaObject_QueryInterface(IMediaObject *iface, REFIID riid, void **ppv)
{
    struct mp3_decoder *This = impl_from_IMediaObject(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IMediaObject))
    {
        *ppv = &This->IMediaObject_iface;
    }
    else
    {
        WARN("no interface for %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IMediaObject_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI MediaObject_AddRef(IMediaObject *iface)
{
    struct mp3_decoder *This = impl_from_IMediaObject(iface);
    ULONG refcount = InterlockedIncrement(&This->ref);

    TRACE("(%p) AddRef from %d\n", This, refcount - 1);

    return refcount;
}

static ULONG WINAPI MediaObject_Release(IMediaObject *iface)
{
    struct mp3_decoder *This = impl_from_IMediaObject(iface);
    ULONG refcount = InterlockedDecrement(&This->ref);

    TRACE("(%p) Release from %d\n", This, refcount + 1);

    if (!refcount)
    {
        heap_free(This);
    }
    return refcount;
}

static HRESULT WINAPI MediaObject_GetStreamCount(IMediaObject *iface, DWORD *input, DWORD *output)
{
    FIXME("(%p)->(%p %p) stub!\n", iface, input, output);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_GetInputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("(%p)->(%d %p) stub!\n", iface, index, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_GetOutputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("(%p)->(%d %p) stub!\n", iface, index, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_GetInputType(IMediaObject *iface, DWORD index, DWORD type_index, DMO_MEDIA_TYPE *type)
{
    FIXME("(%p)->(%d %d %p) stub!\n", iface, index, type_index, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_GetOutputType(IMediaObject *iface, DWORD index, DWORD type_index, DMO_MEDIA_TYPE *type)
{
    FIXME("(%p)->(%d %d %p) stub!\n", iface, index, type_index, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_SetInputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *type, DWORD flags)
{
    FIXME("(%p)->(%d %p %x) stub!\n", iface, index, type, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_SetOutputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *type, DWORD flags)
{
    FIXME("(%p)->(%d %p %x) stub!\n", iface, index, type, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_GetInputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *type)
{
    FIXME("(%p)->(%d %p) stub!\n", iface, index, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_GetOutputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *type)
{
    FIXME("(%p)->(%d %p) stub!\n", iface, index, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_GetInputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *max_lookahead, DWORD *alignment)
{
    FIXME("(%p)->(%d %p %p %p) stub!\n", iface, index, size, max_lookahead, alignment);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_GetOutputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *alignment)
{
    FIXME("(%p)->(%d %p %p) stub!\n", iface, index, size, alignment);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_GetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME *latency)
{
    FIXME("(%p)->(%d %p) stub!\n", iface, index, latency);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_SetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME latency)
{
    FIXME("(%p)->(%d %s) stub!\n", iface, index, wine_dbgstr_longlong(latency));

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_Flush(IMediaObject *iface)
{
    FIXME("(%p)->() stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_Discontinuity(IMediaObject *iface, DWORD index)
{
    FIXME("(%p)->(%d) stub!\n", iface, index);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_AllocateStreamingResources(IMediaObject *iface)
{
    FIXME("(%p)->() stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_FreeStreamingResources(IMediaObject *iface)
{
    FIXME("(%p)->() stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_GetInputStatus(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("(%p)->(%d %p) stub!\n", iface, index, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_ProcessInput(IMediaObject *iface, DWORD index,
    IMediaBuffer *buffer, DWORD flags, REFERENCE_TIME timestamp, REFERENCE_TIME timelength)
{
    FIXME("(%p)->(%d %p %x %s %s) stub!\n", iface, index, buffer, flags,
          wine_dbgstr_longlong(timestamp), wine_dbgstr_longlong(timelength));

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_ProcessOutput(IMediaObject *iface, DWORD flags, DWORD count, DMO_OUTPUT_DATA_BUFFER *buffers, DWORD *status)
{
    FIXME("(%p)->(%x %d %p %p) stub!\n", iface, flags, count, buffers, status);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaObject_Lock(IMediaObject *iface, LONG lock)
{
    FIXME("(%p)->(%d) stub!\n", iface, lock);

    return E_NOTIMPL;
}

static const IMediaObjectVtbl IMediaObject_vtbl = {
    MediaObject_QueryInterface,
    MediaObject_AddRef,
    MediaObject_Release,
    MediaObject_GetStreamCount,
    MediaObject_GetInputStreamInfo,
    MediaObject_GetOutputStreamInfo,
    MediaObject_GetInputType,
    MediaObject_GetOutputType,
    MediaObject_SetInputType,
    MediaObject_SetOutputType,
    MediaObject_GetInputCurrentType,
    MediaObject_GetOutputCurrentType,
    MediaObject_GetInputSizeInfo,
    MediaObject_GetOutputSizeInfo,
    MediaObject_GetInputMaxLatency,
    MediaObject_SetInputMaxLatency,
    MediaObject_Flush,
    MediaObject_Discontinuity,
    MediaObject_AllocateStreamingResources,
    MediaObject_FreeStreamingResources,
    MediaObject_GetInputStatus,
    MediaObject_ProcessInput,
    MediaObject_ProcessOutput,
    MediaObject_Lock,
};

static HRESULT create_mp3_decoder(REFIID riid, void **obj)
{
    struct mp3_decoder *This;

    if (!(This = heap_alloc(sizeof(*This))))
        return E_OUTOFMEMORY;

    This->IMediaObject_iface.lpVtbl = &IMediaObject_vtbl;
    This->ref = 0;

    return IMediaObject_QueryInterface(&This->IMediaObject_iface, riid, obj);
}

/******************************************************************
 *      IClassFactory implementation
 */
typedef struct {
    IClassFactory IClassFactory_iface;
    HRESULT (*fnCreateInstance)(REFIID riid, void **obj);
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **obj)
{
    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(&IID_IUnknown, riid) ||
        IsEqualGUID(&IID_IClassFactory, riid))
    {
        IClassFactory_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;
    WARN("no interface for %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **obj)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);

    TRACE("(%p, %s, %p)\n", outer, debugstr_guid(riid), obj);

    if (outer) {
        *obj = NULL;
        return CLASS_E_NOAGGREGATION;
    }

    return This->fnCreateInstance(riid, obj);
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL lock)
{
    FIXME("(%d) stub\n", lock);
    return S_OK;
}

static const IClassFactoryVtbl classfactory_vtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactoryImpl mp3_decoder_cf = { { &classfactory_vtbl }, create_mp3_decoder };

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    TRACE("%p, %d, %p\n", instance, reason, reserved);
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(instance);
        mp3dmod_instance = instance;
        break;
    }
    return TRUE;
}

/*************************************************************************
 *              DllGetClassObject (DSDMO.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **obj)
{
    TRACE("%s, %s, %p\n", debugstr_guid(clsid), debugstr_guid(riid), obj);

    if (IsEqualGUID(clsid, &CLSID_CMP3DecMediaObject))
        return IClassFactory_QueryInterface(&mp3_decoder_cf.IClassFactory_iface, riid, obj);

    FIXME("class %s not available\n", debugstr_guid(clsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/******************************************************************
 *              DllCanUnloadNow (DSDMO.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

/***********************************************************************
 *              DllRegisterServer (DSDMO.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( mp3dmod_instance );
}

/***********************************************************************
 *              DllUnregisterServer (DSDMO.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( mp3dmod_instance );
}
