/*
 * Helper method for urllib to fetch the proxy configuration settings
 * using the SystemConfiguration framework.
 */
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
		return PyString_FromString(s);

	} else {
		CFIndex len = CFStringGetLength(ref);
		Boolean ok;
		PyObject* result;
		result = PyString_FromStringAndSize(NULL, len*4);

		ok = CFStringGetCString(ref, 
				PyString_AS_STRING(result),
				PyString_GET_SIZE(result),
				kCFStringEncodingUTF8);
		if (!ok) {
			Py_DECREF(result);
			return NULL;
		} else {
			_PyString_Resize(&result, 
				strlen(PyString_AS_STRING(result)));
		}
		return result;
	}
}


static PyObject*
get_proxy_settings(PyObject* mod __attribute__((__unused__)))
{
	CFDictionaryRef proxyDict = NULL;
	CFNumberRef aNum = NULL;
	CFArrayRef anArray = NULL;
	PyObject* result = NULL;
	PyObject* v;
	int r;

	proxyDict = SCDynamicStoreCopyProxies(NULL);
	if (!proxyDict) {
		Py_INCREF(Py_None);
		return Py_None;
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
	Py_DECREF(v); v = NULL;
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
				PyTuple_SetItem(v, i, Py_None);
				Py_INCREF(Py_None);
			} else {
				PyObject* t = cfstring_to_pystring(aString);
				if (!t) {
					PyTuple_SetItem(v, i, Py_None);
					Py_INCREF(Py_None);
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
set_proxy(PyObject* proxies, char* proto, CFDictionaryRef proxyDict,
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
					v = PyString_FromFormat("http://%s:%ld",
						PyString_AS_STRING(h),
						(long)port);
				} else {
					v = PyString_FromFormat("http://%s",
						PyString_AS_STRING(h));
				}
				Py_DECREF(h);
				if (!v) return -1;
				r = PyDict_SetItemString(proxies, proto,
					v);
				Py_DECREF(v);
				return r;
			}
		}

	}
	return 0;
}



static PyObject*
get_proxies(PyObject* mod __attribute__((__unused__)))
{
	PyObject* result = NULL;
	int r;
	CFDictionaryRef proxyDict = NULL;

	proxyDict = SCDynamicStoreCopyProxies(NULL);
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
		(PyCFunction)get_proxy_settings,
		METH_NOARGS,
		NULL,
	},
	{
		"_get_proxies",
		(PyCFunction)get_proxies,
		METH_NOARGS,
		NULL,
	},
	{ 0, 0, 0, 0 }
};

void init_scproxy(void)
{
	(void)Py_InitModule4("_scproxy", mod_methods, NULL, NULL, PYTHON_API_VERSION);
}
