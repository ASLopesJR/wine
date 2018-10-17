/*
 * File source filter unit tests
 *
 * Copyright 2016 Sebastian Lackner
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

#define COBJMACROS
#include "dshow.h"
#include "wine/test.h"

#include "util.h"

static const WCHAR source_id[] = {'O','u','t','p','u','t',0};

static IBaseFilter *create_file_source(void)
{
    IBaseFilter *filter = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_AsyncReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    return filter;
}

static void load_file(IBaseFilter *filter, const WCHAR *filename)
{
    IFileSourceFilter *filesource;
    HRESULT hr;

    hr = IBaseFilter_QueryInterface(filter, &IID_IFileSourceFilter, (void **)&filesource);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IFileSourceFilter_Load(filesource, filename, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IFileSourceFilter_Release(filesource);
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#x, expected %#x.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static void test_interfaces(void)
{
    IBaseFilter *filter = create_file_source();

    check_interface(filter, &IID_IBaseFilter, TRUE);
    check_interface(filter, &IID_IFileSourceFilter, TRUE);

    check_interface(filter, &IID_IAMFilterMiscFlags, FALSE);
    check_interface(filter, &IID_IBasicAudio, FALSE);
    check_interface(filter, &IID_IBasicVideo, FALSE);
    check_interface(filter, &IID_IKsPropertySet, FALSE);
    check_interface(filter, &IID_IMediaPosition, FALSE);
    check_interface(filter, &IID_IMediaSeeking, FALSE);
    check_interface(filter, &IID_IPersistPropertyBag, FALSE);
    check_interface(filter, &IID_IPin, FALSE);
    check_interface(filter, &IID_IQualityControl, FALSE);
    check_interface(filter, &IID_IQualProp, FALSE);
    check_interface(filter, &IID_IReferenceClock, FALSE);
    check_interface(filter, &IID_IVideoWindow, FALSE);

    IBaseFilter_Release(filter);
}

static void test_file_source_filter(void)
{
    static const WCHAR prefix[] = {'w','i','n',0};
    static const struct
    {
        const char *label;
        const char *data;
        DWORD size;
        const GUID *subtype;
    }
    tests[] =
    {
        {
            "AVI",
            "\x52\x49\x46\x46xxxx\x41\x56\x49\x20",
            12,
            &MEDIASUBTYPE_Avi,
        },
        {
            "MPEG1 System",
            "\x00\x00\x01\xBA\x21\x00\x01\x00\x01\x80\x00\x01\x00\x00\x01\xBB",
            16,
            &MEDIASUBTYPE_MPEG1System,
        },
        {
            "MPEG1 Video",
            "\x00\x00\x01\xB3",
            4,
            &MEDIASUBTYPE_MPEG1Video,
        },
        {
            "MPEG1 Audio",
            "\xFF\xE0",
            2,
            &MEDIASUBTYPE_MPEG1Audio,
        },
        {
            "MPEG2 Program",
            "\x00\x00\x01\xBA\x40",
            5,
            &MEDIASUBTYPE_MPEG2_PROGRAM,
        },
        {
            "WAVE",
            "\x52\x49\x46\x46xxxx\x57\x41\x56\x45",
            12,
            &MEDIASUBTYPE_WAVE,
        },
        {
            "unknown format",
            "Hello World",
            11,
            &MEDIASUBTYPE_NULL,
        },
    };
    WCHAR path[MAX_PATH], temp[MAX_PATH], *filename;
    AM_MEDIA_TYPE mt, file_mt, *pmt;
    IFileSourceFilter *filesource;
    IEnumMediaTypes *enum_mt;
    IBaseFilter *filter;
    OLECHAR *olepath;
    DWORD written;
    HANDLE file;
    HRESULT hr;
    ULONG ref;
    IPin *pin;
    BOOL ret;
    int i;

    GetTempPathW(MAX_PATH, temp);
    GetTempFileNameW(temp, prefix, 0, path);

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        trace("Running test for %s.\n", tests[i].label);

        file = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
        ok(file != INVALID_HANDLE_VALUE, "Failed to create file, error %u.\n", GetLastError());
        ret = WriteFile(file, tests[i].data, tests[i].size, &written, NULL);
        ok(ret, "Failed to write file, error %u.\n", GetLastError());
        CloseHandle(file);

        filter = create_file_source();
        IBaseFilter_QueryInterface(filter, &IID_IFileSourceFilter, (void **)&filesource);

        olepath = (void *)0xdeadbeef;
        hr = IFileSourceFilter_GetCurFile(filesource, &olepath, NULL);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        ok(!olepath, "Got path %s.\n", wine_dbgstr_w(olepath));

        hr = IFileSourceFilter_Load(filesource, NULL, NULL);
        ok(hr == E_POINTER, "Got hr %#x.\n", hr);

        hr = IFileSourceFilter_Load(filesource, path, NULL);
        ok(hr == S_OK, "Got hr %#x.\n", hr);

        hr = IFileSourceFilter_GetCurFile(filesource, NULL, &mt);
        ok(hr == E_POINTER, "Got hr %#x.\n", hr);

        olepath = NULL;
        hr = IFileSourceFilter_GetCurFile(filesource, &olepath, NULL);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        CoTaskMemFree(olepath);

        olepath = NULL;
        memset(&file_mt, 0x11, sizeof(file_mt));
        hr = IFileSourceFilter_GetCurFile(filesource, &olepath, &file_mt);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        ok(!lstrcmpW(olepath, path), "Expected path %s, got %s.\n",
                wine_dbgstr_w(path), wine_dbgstr_w(olepath));
        ok(IsEqualGUID(&file_mt.majortype, &MEDIATYPE_Stream), "Got major type %s.\n",
                wine_dbgstr_guid(&file_mt.majortype));
        /* winegstreamer hijacks format type detection. */
        if (!IsEqualGUID(tests[i].subtype, &MEDIASUBTYPE_NULL))
            ok(IsEqualGUID(&file_mt.subtype, tests[i].subtype), "Expected subtype %s, got %s.\n",
                wine_dbgstr_guid(tests[i].subtype), wine_dbgstr_guid(&file_mt.subtype));
        ok(file_mt.bFixedSizeSamples == TRUE, "Got fixed size %d.\n", file_mt.bFixedSizeSamples);
        ok(file_mt.bTemporalCompression == FALSE, "Got temporal compression %d.\n",
                file_mt.bTemporalCompression);
todo_wine {
        ok(file_mt.lSampleSize == 1, "Got sample size %u.\n", file_mt.lSampleSize);
        ok(IsEqualGUID(&file_mt.formattype, &GUID_NULL), "Got format type %s.\n",
                wine_dbgstr_guid(&file_mt.formattype));
}
        ok(!file_mt.pUnk, "Got pUnk %p.\n", file_mt.pUnk);
        ok(!file_mt.cbFormat, "Got format size %#x.\n", file_mt.cbFormat);
        ok(!file_mt.pbFormat, "Got format %p.\n", file_mt.pbFormat);
        CoTaskMemFree(olepath);

        hr = IBaseFilter_FindPin(filter, source_name, &pin);
        ok(hr == S_OK, "Got hr %#x.\n", hr);

        hr = IPin_EnumMediaTypes(pin, &enum_mt);
        ok(hr == S_OK, "Got hr %#x.\n", hr);

        hr = IEnumMediaTypes_Next(enum_mt, 1, &pmt, NULL);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        ok(!memcmp(pmt, &file_mt, sizeof(*pmt)), "Media types did not match.\n");
        CoTaskMemFree(pmt);

        hr = IEnumMediaTypes_Next(enum_mt, 1, &pmt, NULL);
todo_wine
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        if (hr == S_OK)
        {
            mt = file_mt;
            mt.subtype = GUID_NULL;
            ok(!memcmp(pmt, &mt, sizeof(*pmt)), "Media types did not match.\n");
            CoTaskMemFree(pmt);
        }

        hr = IEnumMediaTypes_Next(enum_mt, 1, &pmt, NULL);
        ok(hr == S_FALSE, "Got hr %#x.\n", hr);

        IEnumMediaTypes_Release(enum_mt);

        mt = file_mt;
        hr = IPin_QueryAccept(pin, &mt);
        ok(hr == S_OK, "Got hr %#x.\n", hr);

        mt.bFixedSizeSamples = FALSE;
        mt.bTemporalCompression = TRUE;
        mt.lSampleSize = 123;
        mt.formattype = FORMAT_VideoInfo;
        hr = IPin_QueryAccept(pin, &mt);
todo_wine_if(IsEqualGUID(tests[i].subtype, &GUID_NULL))
        ok(hr == S_OK, "Got hr %#x.\n", hr);

        mt.majortype = MEDIATYPE_Video;
        hr = IPin_QueryAccept(pin, &mt);
        ok(hr == S_FALSE, "Got hr %#x.\n", hr);
        mt.majortype = MEDIATYPE_Stream;

        if (!IsEqualGUID(tests[i].subtype, &GUID_NULL))
        {
            mt.subtype = GUID_NULL;
            hr = IPin_QueryAccept(pin, &mt);
            ok(hr == S_FALSE, "Got hr %#x.\n", hr);
        }

        IPin_Release(pin);
        IFileSourceFilter_Release(filesource);
        ref = IBaseFilter_Release(filter);
        ok(!ref, "Got outstanding refcount %d.\n", ref);

        ret = DeleteFileW(path);
        ok(ret, "Failed to delete file, error %u\n", GetLastError());
    }

    /* test prescribed format */
    filter = create_file_source();
    hr = IBaseFilter_QueryInterface(filter, &IID_IFileSourceFilter, (void **)&filesource);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    mt.majortype = MEDIATYPE_Video;
    mt.subtype = MEDIASUBTYPE_RGB8;
    mt.bFixedSizeSamples = FALSE;
    mt.bTemporalCompression = TRUE;
    mt.lSampleSize = 123;
    mt.formattype = FORMAT_None;
    mt.pUnk = NULL;
    mt.cbFormat = 0;
    mt.pbFormat = NULL;
    filename = load_resource(avifile);
    hr = IFileSourceFilter_Load(filesource, filename, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IFileSourceFilter_GetCurFile(filesource, &olepath, &file_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!memcmp(&file_mt, &mt, sizeof(mt)), "Media types did not match.\n");
    CoTaskMemFree(olepath);

    hr = IBaseFilter_FindPin(filter, source_name, &pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_EnumMediaTypes(pin, &enum_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum_mt, 1, &pmt, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!memcmp(pmt, &file_mt, sizeof(*pmt)), "Media types did not match.\n");
    CoTaskMemFree(pmt);

    hr = IEnumMediaTypes_Next(enum_mt, 1, &pmt, NULL);
todo_wine
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    if (hr == S_OK)
    {
        ok(IsEqualGUID(&pmt->majortype, &MEDIATYPE_Stream), "Got major type %s.\n",
                wine_dbgstr_guid(&pmt->majortype));
        ok(IsEqualGUID(&pmt->subtype, &GUID_NULL), "Got subtype %s.\n",
                wine_dbgstr_guid(&pmt->subtype));
        ok(pmt->bFixedSizeSamples == TRUE, "Got fixed size %d.\n", pmt->bFixedSizeSamples);
        ok(!pmt->bTemporalCompression, "Got temporal compression %d.\n", pmt->bTemporalCompression);
        ok(pmt->lSampleSize == 1, "Got sample size %u.\n", pmt->lSampleSize);
        ok(IsEqualGUID(&pmt->formattype, &GUID_NULL), "Got format type %s.\n",
                wine_dbgstr_guid(&pmt->formattype));
        ok(!pmt->pUnk, "Got pUnk %p.\n", pmt->pUnk);
        ok(!pmt->cbFormat, "Got format size %#x.\n", pmt->cbFormat);
        ok(!pmt->pbFormat, "Got format %p.\n", pmt->pbFormat);

        hr = IPin_QueryAccept(pin, pmt);
        ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    }

    hr = IEnumMediaTypes_Next(enum_mt, 1, &pmt, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    IEnumMediaTypes_Release(enum_mt);

    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    mt.bFixedSizeSamples = TRUE;
    mt.bTemporalCompression = FALSE;
    mt.lSampleSize = 456;
    mt.formattype = FORMAT_VideoInfo;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    mt.majortype = MEDIATYPE_Stream;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    mt.majortype = MEDIATYPE_Video;

    mt.subtype = MEDIASUBTYPE_NULL;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    IPin_Release(pin);
    IFileSourceFilter_Release(filesource);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete file, error %u.\n", GetLastError());
}

static void test_enum_pins(void)
{
    const WCHAR *filename = load_resource(avifile);
    IBaseFilter *filter = create_file_source();
    IEnumPins *enum1, *enum2;
    IPin *pins[2];
    ULONG count;
    HRESULT hr;
    ULONG ref;
    BOOL ret;

    hr = IBaseFilter_EnumPins(filter, &enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumPins_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    load_file(filter, filename);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
todo_wine
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IPin_Release(pins[0]);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, &count);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);
    IPin_Release(pins[0]);

    hr = IEnumPins_Next(enum1, 1, pins, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(!count, "Got count %u.\n", count);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 2, pins, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 2, pins, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);
    IPin_Release(pins[0]);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Skip(enum1, 2);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumPins_Skip(enum1, 1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum2, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IPin_Release(pins[0]);

    IEnumPins_Release(enum2);
    IEnumPins_Release(enum1);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete file, error %u.\n", GetLastError());
}

static void test_find_pin(void)
{
    const WCHAR *filename = load_resource(avifile);
    IBaseFilter *filter = create_file_source();
    IEnumPins *enumpins;
    IPin *pin, *pin2;
    HRESULT hr;
    ULONG ref;
    BOOL ret;

    hr = IBaseFilter_FindPin(filter, source_id, &pin);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#x.\n", hr);

    load_file(filter, filename);

    hr = IBaseFilter_FindPin(filter, source_id, &pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_EnumPins(filter, &enumpins);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enumpins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(pin == pin2, "Expected pin %p, got %p.\n", pin, pin2);

    IPin_Release(pin2);
    IPin_Release(pin);
    IEnumPins_Release(enumpins);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete file, error %u.\n", GetLastError());
}

static void test_pin_info(void)
{
    const WCHAR *filename = load_resource(avifile);
    IBaseFilter *filter = create_file_source();
    PIN_DIRECTION dir;
    PIN_INFO info;
    HRESULT hr;
    WCHAR *id;
    ULONG ref;
    IPin *pin;
    BOOL ret;

    load_file(filter, filename);

    hr = IBaseFilter_FindPin(filter, source_name, &pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_OUTPUT, "Got direction %d.\n", info.dir);
    ok(!lstrcmpW(info.achName, source_name), "Got name %s.\n", wine_dbgstr_w(info.achName));
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(dir == PINDIR_OUTPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!lstrcmpW(id, source_name), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    IPin_Release(pin);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete file, error %u.\n", GetLastError());
}

START_TEST(filesource)
{
    CoInitialize(NULL);

    test_interfaces();
    test_enum_pins();
    test_find_pin();
    test_pin_info();
    test_file_source_filter();

    CoUninitialize();
}
