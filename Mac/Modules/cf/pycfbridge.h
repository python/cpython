extern PyObject *PyCF_CF2Python(CFTypeRef src);
extern PyObject *PyCF_CF2Python_sequence(CFArrayRef src);
extern PyObject *PyCF_CF2Python_mapping(CFTypeRef src);
extern PyObject *PyCF_CF2Python_simple(CFTypeRef src);
extern PyObject *PyCF_CF2Python_string(CFStringRef src);

extern int PyCF_Python2CF(PyObject *src, CFTypeRef *dst);
extern int PyCF_Python2CF_sequence(PyObject *src, CFArrayRef *dst);
extern int PyCF_Python2CF_mapping(PyObject *src, CFDictionaryRef *dst);
extern int PyCF_Python2CF_simple(PyObject *src, CFTypeRef *dst);
extern int PyCF_Python2CF_string(PyObject *src, CFStringRef *dst);