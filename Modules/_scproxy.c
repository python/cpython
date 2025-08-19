/*
 * Helper method for urllib to fetch the proxy configuration settings
 * using the SystemConfiguration framework.
 */

// Need limited C API version 3.13 for Py_mod_gil
#include "pyconfig.h"   // Py_GIL_DISABLED
#ifndef Py_GIL_DISABLED
#  define Py_LIMITED_API 0x030d0000
#endif

#include <Python.h>
#include <SystemConfiguration/SystemConfiguration.h>

static int32_t
cfnum_to_int32(CFNumberRef num)
{
    int32_t result;

    CFNumberGetValue(num, kCFNumberSInt32Type, &result);
    return result;
}

static PyObject*
cfstring_to_pystring(CFStringRef ref)
{
    const char* s;

    s = CFStringGetCStringPtr(ref, kCFStringEncodingUTF8);
    if (s) {
        return PyUnicode_DecodeUTF8(s, strlen(s), NULL);

    } else {
        CFIndex len = CFStringGetLength(ref);
        Boolean ok;
        PyObject* result;
        char* buf;

        buf = PyMem_Malloc(len*4);
        if (buf == NULL) {
            PyErr_NoMemory();
            return NULL;
        }

        ok = CFStringGetCString(ref,
                        buf, len * 4,
                        kCFStringEncodingUTF8);
        if (!ok) {
            PyMem_Free(buf);
            return NULL;
        } else {
            result = PyUnicode_DecodeUTF8(buf, strlen(buf), NULL);
            PyMem_Free(buf);
        }
        return result;
    }
}


static PyObject*
get_proxy_settings(PyObject* Py_UNUSED(mod), PyObject *Py_UNUSED(ignored))
{
    CFDictionaryRef proxyDict = NULL;
    CFNumberRef aNum = NULL;
    CFArrayRef anArray = NULL;
    PyObject* result = NULL;
    PyObject* v;
    int r;

    Py_BEGIN_ALLOW_THREADS
    proxyDict = SCDynamicStoreCopyProxies(NULL);
    Py_END_ALLOW_THREADS

    if (!proxyDict) {
        Py_RETURN_NONE;
    }

    result = PyDict_New();
    if (result == NULL) goto error;

    aNum = CFDictionaryGetValue(proxyDict,
        kSCPropNetProxiesExcludeSimpleHostnames);
    if (aNum == NULL) {
        v = PyBool_FromLong(0);
    } else {
        v = PyBool_FromLong(cfnum_to_int32(aNum));
    }

    if (v == NULL) goto error;

    r = PyDict_SetItemString(result, "exclude_simple", v);
    Py_CLEAR(v);
    if (r == -1) goto error;

    anArray = CFDictionaryGetValue(proxyDict,
                    kSCPropNetProxiesExceptionsList);
    if (anArray != NULL) {
        CFIndex len = CFArrayGetCount(anArray);
        CFIndex i;
        v = PyTuple_New(len);
        if (v == NULL) goto error;

        r = PyDict_SetItemString(result, "exceptions", v);
        Py_DECREF(v);
        if (r == -1) goto error;

        for (i = 0; i < len; i++) {
            CFStringRef aString = NULL;

            aString = CFArrayGetValueAtIndex(anArray, i);
            if (aString == NULL) {
                PyTuple_SetItem(v, i, Py_NewRef(Py_None));
            } else {
                PyObject* t = cfstring_to_pystring(aString);
                if (!t) {
                    PyTuple_SetItem(v, i, Py_NewRef(Py_None));
                } else {
                    PyTuple_SetItem(v, i, t);
                }
            }
        }
    }

    CFRelease(proxyDict);
    return result;

error:
    if (proxyDict)  CFRelease(proxyDict);
    Py_XDECREF(result);
    return NULL;
}

static int
set_proxy(PyObject* proxies, const char* proto, CFDictionaryRef proxyDict,
                CFStringRef enabledKey,
                CFStringRef hostKey, CFStringRef portKey)
{
    CFNumberRef aNum;

    aNum = CFDictionaryGetValue(proxyDict, enabledKey);
    if (aNum && cfnum_to_int32(aNum)) {
        CFStringRef hostString;

        hostString = CFDictionaryGetValue(proxyDict, hostKey);
        aNum = CFDictionaryGetValue(proxyDict, portKey);

        if (hostString) {
            int r;
            PyObject* h = cfstring_to_pystring(hostString);
            PyObject* v;
            if (h) {
                if (aNum) {
                    int32_t port = cfnum_to_int32(aNum);
                    v = PyUnicode_FromFormat("http://%U:%ld", h, (long)port);
                } else {
                    v = PyUnicode_FromFormat("http://%U", h);
                }
                Py_DECREF(h);
                if (!v) return -1;
                r = PyDict_SetItemString(proxies, proto, v);
                Py_DECREF(v);
                return r;
            }
        }

    }
    return 0;
}



static PyObject*
get_proxies(PyObject* Py_UNUSED(mod), PyObject *Py_UNUSED(ignored))
{
    PyObject* result = NULL;
    int r;
    CFDictionaryRef proxyDict = NULL;

    Py_BEGIN_ALLOW_THREADS
    proxyDict = SCDynamicStoreCopyProxies(NULL);
    Py_END_ALLOW_THREADS

    if (proxyDict == NULL) {
        return PyDict_New();
    }

    result = PyDict_New();
    if (result == NULL) goto error;

    r = set_proxy(result, "http", proxyDict,
        kSCPropNetProxiesHTTPEnable,
        kSCPropNetProxiesHTTPProxy,
        kSCPropNetProxiesHTTPPort);
    if (r == -1) goto error;
    r = set_proxy(result, "https", proxyDict,
        kSCPropNetProxiesHTTPSEnable,
        kSCPropNetProxiesHTTPSProxy,
        kSCPropNetProxiesHTTPSPort);
    if (r == -1) goto error;
    r = set_proxy(result, "ftp", proxyDict,
        kSCPropNetProxiesFTPEnable,
        kSCPropNetProxiesFTPProxy,
        kSCPropNetProxiesFTPPort);
    if (r == -1) goto error;
    r = set_proxy(result, "gopher", proxyDict,
        kSCPropNetProxiesGopherEnable,
        kSCPropNetProxiesGopherProxy,
        kSCPropNetProxiesGopherPort);
    if (r == -1) goto error;
    r = set_proxy(result, "socks", proxyDict,
        kSCPropNetProxiesSOCKSEnable,
        kSCPropNetProxiesSOCKSProxy,
        kSCPropNetProxiesSOCKSPort);
    if (r == -1) goto error;

    CFRelease(proxyDict);
    return result;
error:
    if (proxyDict)  CFRelease(proxyDict);
    Py_XDECREF(result);
    return NULL;
}

static PyMethodDef mod_methods[] = {
    {
        "_get_proxy_settings",
        get_proxy_settings,
        METH_NOARGS,
        NULL,
    },
    {
        "_get_proxies",
        get_proxies,
        METH_NOARGS,
        NULL,
    },
    { 0, 0, 0, 0 }
};

static PyModuleDef_Slot _scproxy_slots[] = {
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef _scproxy_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_scproxy",
    .m_size = 0,
    .m_methods = mod_methods,
    .m_slots = _scproxy_slots,
};

PyMODINIT_FUNC
PyInit__scproxy(void)
{
    return PyModuleDef_Init(&_scproxy_module);
}
