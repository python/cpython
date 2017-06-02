//
// Helper library for location Visual Studio installations
// using the COM-based query API.
//
// Copyright (c) Microsoft Corporation
// Licensed to PSF under a contributor agreement
//

// Version history
//  2017-05: Initial contribution (Steve Dower)

#include <Windows.h>
#include <Strsafe.h>
#include "external\include\Setup.Configuration.h"
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "Microsoft.VisualStudio.Setup.Configuration.Native.lib")

#include <Python.h>

static PyObject *error_from_hr(HRESULT hr)
{
    if (FAILED(hr))
        PyErr_Format(PyExc_OSError, "Error %08x", hr);
    assert(PyErr_Occurred());
    return nullptr;
}

static PyObject *get_install_name(ISetupInstance2 *inst)
{
    HRESULT hr;
    BSTR name;
    PyObject *str = nullptr;
    if (FAILED(hr = inst->GetDisplayName(LOCALE_USER_DEFAULT, &name)))
        goto error;
    str = PyUnicode_FromWideChar(name, SysStringLen(name));
    SysFreeString(name);
    return str;
error:

    return error_from_hr(hr);
}

static PyObject *get_install_version(ISetupInstance *inst)
{
    HRESULT hr;
    BSTR ver;
    PyObject *str = nullptr;
    if (FAILED(hr = inst->GetInstallationVersion(&ver)))
        goto error;
    str = PyUnicode_FromWideChar(ver, SysStringLen(ver));
    SysFreeString(ver);
    return str;
error:

    return error_from_hr(hr);
}

static PyObject *get_install_path(ISetupInstance *inst)
{
    HRESULT hr;
    BSTR path;
    PyObject *str = nullptr;
    if (FAILED(hr = inst->GetInstallationPath(&path)))
        goto error;
    str = PyUnicode_FromWideChar(path, SysStringLen(path));
    SysFreeString(path);
    return str;
error:

    return error_from_hr(hr);
}

static PyObject *get_installed_packages(ISetupInstance2 *inst)
{
    HRESULT hr;
    PyObject *res = nullptr;
    LPSAFEARRAY sa_packages = nullptr;
    LONG ub = 0;
    IUnknown **packages = nullptr;
    PyObject *str = nullptr;

    if (FAILED(hr = inst->GetPackages(&sa_packages)) ||
        FAILED(hr = SafeArrayAccessData(sa_packages, (void**)&packages)) ||
        FAILED(SafeArrayGetUBound(sa_packages, 1, &ub)) ||
        !(res = PyList_New(0)))
        goto error;

    for (LONG i = 0; i < ub; ++i) {
        ISetupPackageReference *package = nullptr;
        BSTR id = nullptr;
        PyObject *str = nullptr;

        if (FAILED(hr = packages[i]->QueryInterface(&package)) ||
            FAILED(hr = package->GetId(&id)))
            goto iter_error;

        str = PyUnicode_FromWideChar(id, SysStringLen(id));
        SysFreeString(id);

        if (!str || PyList_Append(res, str) < 0)
            goto iter_error;

        Py_CLEAR(str);
        package->Release();
        continue;

    iter_error:
        if (package) package->Release();
        Py_XDECREF(str);

        goto error;
    }

    SafeArrayUnaccessData(sa_packages);
    SafeArrayDestroy(sa_packages);

    return res;
error:
    if (sa_packages && packages) SafeArrayUnaccessData(sa_packages);
    if (sa_packages) SafeArrayDestroy(sa_packages);
    Py_XDECREF(res);

    return error_from_hr(hr);
}

static PyObject *find_all_instances()
{
    ISetupConfiguration *sc = nullptr;
    ISetupConfiguration2 *sc2 = nullptr;
    IEnumSetupInstances *enm = nullptr;
    ISetupInstance *inst = nullptr;
    ISetupInstance2 *inst2 = nullptr;
    PyObject *res = nullptr;
    ULONG fetched;
    HRESULT hr;

    if (!(res = PyList_New(0)))
        goto error;

    if (FAILED(hr = CoCreateInstance(
        __uuidof(SetupConfiguration),
        NULL,
        CLSCTX_INPROC_SERVER,
        __uuidof(ISetupConfiguration),
        (LPVOID*)&sc
    )) && hr != REGDB_E_CLASSNOTREG)
        goto error;

    // If the class is not registered, there are no VS instances installed
    if (hr == REGDB_E_CLASSNOTREG)
        return res;

    if (FAILED(hr = sc->QueryInterface(&sc2)) ||
        FAILED(hr = sc2->EnumAllInstances(&enm)))
        goto error;

    while (SUCCEEDED(enm->Next(1, &inst, &fetched)) && fetched) {
        PyObject *name = nullptr;
        PyObject *version = nullptr;
        PyObject *path = nullptr;
        PyObject *packages = nullptr;

        if (FAILED(hr = inst->QueryInterface(&inst2)) ||
            !(name = get_install_name(inst2)) ||
            !(version = get_install_version(inst)) ||
            !(path = get_install_path(inst)) ||
            !(packages = get_installed_packages(inst2)) ||
            PyList_Append(res, PyTuple_Pack(4, name, version, path, packages)) < 0)
            goto iter_error;

        continue;
    iter_error:
        if (inst2) inst2->Release();
        Py_XDECREF(packages);
        Py_XDECREF(path);
        Py_XDECREF(version);
        Py_XDECREF(name);
        goto error;
    }

    enm->Release();
    sc2->Release();
    sc->Release();
    return res;

error:
    if (enm) enm->Release();
    if (sc2) sc2->Release();
    if (sc) sc->Release();
    Py_XDECREF(res);

    return error_from_hr(hr);
}

PyDoc_STRVAR(findvs_findall_doc, "findall()\
\
Finds all installed versions of Visual Studio.");

static PyObject *findvs_findall(PyObject *self, PyObject *args, PyObject *kwargs)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (hr == RPC_E_CHANGED_MODE)
        hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
        return error_from_hr(hr);
    PyObject *res = find_all_instances();
    CoUninitialize();
    return res;
}

PyDoc_STRVAR(findvs_getversion_doc, "getversion(path)\
\
Reads the product version from the specified file.");

static PyObject *findvs_getversion(PyObject *self, PyObject *args, PyObject *kwargs)
{
    LPVOID verblock;
    DWORD verblock_size;

    PyObject *res = NULL;

    PyObject *path_obj;
    const wchar_t *path;
    Py_ssize_t path_len;

    static const char* keywords[] = { "path", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&:getversion",
            (char**)keywords, PyUnicode_FSDecoder, &path_obj))
        return NULL;

    path = PyUnicode_AsWideCharString(path_obj, &path_len);
    Py_DECREF(path_obj);
    if (!path) {
        return NULL;
    }

    if ((verblock_size = GetFileVersionInfoSizeW(path, NULL)) &&
        (verblock = malloc(verblock_size))) {
        WORD *langinfo;
        wchar_t *verstr;
        UINT langinfo_size, ver_size;

        if (GetFileVersionInfoW(path, 0, verblock_size, verblock) &&
            VerQueryValueW(verblock, L"\\VarFileInfo\\Translation",
                           (LPVOID*)&langinfo, &langinfo_size)) {
            wchar_t rsrc_name[256];
            StringCchPrintfW(rsrc_name, 256,
                             L"\\StringFileInfo\\%04x%04x\\ProductVersion",
                             langinfo[0], langinfo[1]);
            if (VerQueryValueW(verblock, rsrc_name, (LPVOID*)&verstr, &ver_size)) {
                while (ver_size > 0 && !verstr[ver_size]) {
                    ver_size -= 1;
                }
                res = PyUnicode_FromWideChar(verstr, ver_size - 1);
            }
        }
        free(verblock);
    }

    PyMem_Free((void*)path);
    if (!res) {
        Py_RETURN_NONE;
    }
    return res;
}


// List of functions to add to findvs in exec_findvs().
static PyMethodDef findvs_functions[] = {
    { "findall", (PyCFunction)findvs_findall, METH_VARARGS | METH_KEYWORDS, findvs_findall_doc },
    { "getversion", (PyCFunction)findvs_getversion, METH_VARARGS | METH_KEYWORDS, findvs_getversion_doc },
    { NULL, NULL, 0, NULL }
};

// Initialize findvs. May be called multiple times, so avoid
// using static state.
static int exec_findvs(PyObject *module)
{
    PyModule_AddFunctions(module, findvs_functions);

    return 0; // success
}

PyDoc_STRVAR(findvs_doc, "The _findvs helper module");

static PyModuleDef_Slot findvs_slots[] = {
    { Py_mod_exec, exec_findvs },
    { 0, NULL }
};

static PyModuleDef findvs_def = {
    PyModuleDef_HEAD_INIT,
    "_findvs",
    findvs_doc,
    0,              // m_size
    NULL,           // m_methods
    findvs_slots,
    NULL,           // m_traverse
    NULL,           // m_clear
    NULL,           // m_free
};

extern "C" {
    PyMODINIT_FUNC PyInit__findvs(void)
    {
        return PyModuleDef_Init(&findvs_def);
    }
}