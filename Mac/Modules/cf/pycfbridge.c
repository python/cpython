/*
** Convert objects from Python to CoreFoundation and vice-versa.
*/

#ifdef WITHOUT_FRAMEWORKS
#include <CFBase.h>
#include <CFArray.h>
#include <CFData.h>
#include <CFDictionary.h>
#include <CFString.h>
#include <CFURL.h>
#else
#include <CoreServices/CoreServices.h>
#endif

#include "Python.h"
#include "macglue.h"
#include "pycfbridge.h"


/* ---------------------------------------- */
/* CoreFoundation objects to Python objects */
/* ---------------------------------------- */

PyObject *
PyCF_CF2Python(CFTypeRef src) {
	CFTypeID typeid;
	
	if( src == NULL ) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	typeid = CFGetTypeID(src);
	if (typeid == CFArrayGetTypeID())
		return PyCF_CF2Python_sequence((CFArrayRef)src);
	if (typeid == CFDictionaryGetTypeID())
		return PyCF_CF2Python_mapping((CFDictionaryRef)src);
	return PyCF_CF2Python_simple(src);
}

PyObject *
PyCF_CF2Python_sequence(CFArrayRef src) {
	int size = CFArrayGetCount(src);
	PyObject *rv;
	CFTypeRef item_cf;
	PyObject *item_py = NULL;
	int i;
	
	if ( (rv=PyList_New(size)) == NULL )
		return NULL;
	for(i=0; i<size; i++) {
		item_cf = CFArrayGetValueAtIndex(src, i);
		if (item_cf == NULL ) goto err;
		item_py = PyCF_CF2Python(item_cf);
		if (item_py == NULL ) goto err;
		if (PyList_SetItem(rv, i, item_py) < 0) goto err;
		item_py = NULL;
	}
	return rv;
err:
	Py_XDECREF(item_py);
	Py_DECREF(rv);
	return NULL;
}

PyObject *
PyCF_CF2Python_mapping(CFTypeRef src) {
	int size = CFDictionaryGetCount(src);
	PyObject *rv;
	CFTypeRef *allkeys, *allvalues;
	CFTypeRef key_cf, value_cf;
	PyObject *key_py = NULL, *value_py = NULL;
	int i;
	
	allkeys = malloc(size*sizeof(CFTypeRef *));
	if (allkeys == NULL) return PyErr_NoMemory();
	allvalues = malloc(size*sizeof(CFTypeRef *));
	if (allvalues == NULL) return PyErr_NoMemory();
	if ( (rv=PyDict_New()) == NULL )
		return NULL;
	CFDictionaryGetKeysAndValues(src, allkeys, allvalues);
	for(i=0; i<size; i++) {
		key_cf = allkeys[i];
		value_cf = allvalues[i];
		key_py = PyCF_CF2Python(key_cf);
		if (key_py == NULL ) goto err;
		value_py = PyCF_CF2Python(value_py);
		if (value_py == NULL ) goto err;
		if (PyDict_SetItem(rv, key_py, value_py) < 0) goto err;
		key_py = NULL;
		value_py = NULL;
	}
	return rv;
err:
	Py_XDECREF(key_py);
	Py_XDECREF(value_py);
	Py_DECREF(rv);
	free(allkeys);
	free(allvalues);
	return NULL;
}

PyObject *
PyCF_CF2Python_simple(CFTypeRef src) {
	CFTypeID typeid;
	
	typeid = CFGetTypeID(src);
	if (typeid == CFStringGetTypeID())
		return PyCF_CF2Python_string((CFStringRef)src);
	PyMac_Error(resNotFound);
	return NULL;
}

/* Unsure - Return unicode or 8 bit strings? */
PyObject *
PyCF_CF2Python_string(CFStringRef src) {
	int size = CFStringGetLength(src)+1;
	Py_UNICODE *data = malloc(size*sizeof(Py_UNICODE));
	CFRange range;
	PyObject *rv;

	range.location = 0;
	range.length = size;
	if( data == NULL ) return PyErr_NoMemory();
	CFStringGetCharacters(src, range, data);
	rv = (PyObject *)PyUnicode_FromUnicode(data, size);
	free(data);
	return rv;
}

/* ---------------------------------------- */
/* Python objects to CoreFoundation objects */
/* ---------------------------------------- */

int
PyCF_Python2CF(PyObject *src, CFTypeRef *dst) {

	if (PyString_Check(src) || PyUnicode_Check(src))
		return PyCF_Python2CF_simple(src, dst);
	if (PySequence_Check(src))
		return PyCF_Python2CF_sequence(src, (CFArrayRef *)dst);
	if (PyMapping_Check(src))
		return PyCF_Python2CF_mapping(src, (CFDictionaryRef *)dst);
	return PyCF_Python2CF_simple(src, dst);
}

int
PyCF_Python2CF_sequence(PyObject *src, CFArrayRef *dst) {
	CFMutableArrayRef rv = NULL;
	CFTypeRef item_cf = NULL;
	PyObject *item_py = NULL;
	int size, i;
	
	size = PySequence_Size(src);
	rv = CFArrayCreateMutable((CFAllocatorRef)NULL, size, &kCFTypeArrayCallBacks);
	if (rv == NULL) {
		PyMac_Error(resNotFound); 
		goto err;
	}

	for( i=0; i<size; i++) {
		item_py = PySequence_GetItem(src, i);
		if (item_py == NULL) goto err;
		if ( !PyCF_Python2CF(item_py, &item_cf)) goto err;
		Py_DECREF(item_py);
		CFArraySetValueAtIndex(rv, i, item_cf);
		CFRelease(item_cf);
		item_cf = NULL;
	}
	*dst = rv;
	return 1;
err:
	Py_XDECREF(item_py);
	if (rv) CFRelease(rv);
	if (item_cf) CFRelease(item_cf);
	return 0;		
}

int
PyCF_Python2CF_mapping(PyObject *src, CFDictionaryRef *dst) {
	CFMutableDictionaryRef rv = NULL;
	PyObject *aslist = NULL;
	CFTypeRef key_cf = NULL, value_cf = NULL;
	PyObject *item_py = NULL, *key_py = NULL, *value_py = NULL;
	int size, i;
	
	size = PyMapping_Size(src);
	rv = CFDictionaryCreateMutable((CFAllocatorRef)NULL, size,
					&kCFTypeDictionaryKeyCallBacks,
	                                &kCFTypeDictionaryValueCallBacks);
	if (rv == NULL) {
		PyMac_Error(resNotFound); 
		goto err;
	}
	if ( (aslist = PyMapping_Items(src)) == NULL ) goto err;

	for( i=0; i<size; i++) {
		item_py = PySequence_GetItem(aslist, i);
		if (item_py == NULL) goto err;
		if (!PyArg_ParseTuple(item_py, "OO", key_py, value_py)) goto err;
		if ( !PyCF_Python2CF(key_py, &key_cf) ) goto err;
		Py_DECREF(key_py);
		if ( !PyCF_Python2CF(value_py, &value_cf) ) goto err;
		Py_DECREF(value_py);
		CFDictionaryAddValue(rv, key_cf, value_cf);
		CFRelease(key_cf);
		key_cf = NULL;
		CFRelease(value_cf);
		value_cf = NULL;
	}
	*dst = rv;
	return 1;
err:
	Py_XDECREF(item_py);
	Py_XDECREF(key_py);
	Py_XDECREF(value_py);
	Py_XDECREF(aslist);
	if (rv) CFRelease(rv);
	if (key_cf) CFRelease(key_cf);
	if (value_cf) CFRelease(value_cf);
	return 0;		
}

int
PyCF_Python2CF_simple(PyObject *src, CFTypeRef *dst) {
	
	if (PyObject_HasAttrString(src, "CFType")) {
		*dst = PyObject_CallMethod(src, "CFType", "");
		return (*dst != NULL);
	}
	if (PyString_Check(src) || PyUnicode_Check(src)) 
		return PyCF_Python2CF_string(src, (CFStringRef *)dst);
	PyErr_Format(PyExc_TypeError,
		  "Cannot convert %.500s objects to CF",
				     src->ob_type->tp_name);
	return 0;
}

int
PyCF_Python2CF_string(PyObject *src, CFStringRef *dst) {
	char *chars;
	CFIndex size;
	UniChar *unichars;
	
	if (PyString_Check(src)) {
		if ((chars = PyString_AsString(src)) == NULL ) goto err;
		*dst = CFStringCreateWithCString((CFAllocatorRef)NULL, chars, 0);
		return 1;
	}
	if (PyUnicode_Check(src)) {
		/* We use the CF types here, if Python was configured differently that will give an error */
		size = PyUnicode_GetSize(src);
		if ((unichars = PyUnicode_AsUnicode(src)) == NULL ) goto err;
		*dst = CFStringCreateWithCharacters((CFAllocatorRef)NULL, unichars, size);
		return 1;
	}
err:
	PyErr_Format(PyExc_TypeError,
		  "Cannot convert %.500s objects to CF",
				     src->ob_type->tp_name);
	return 0;
}
