/* This file must be kept in alphabetical order.  See test_commontypes.py */

#define EQ(key, value)    key "\0" value   /* string concatenation */
#ifdef _WIN64
#  define W32_64(X,Y)  Y
# else
#  define W32_64(X,Y)  X
# endif


static const char *common_simple_types[] = {

#ifdef MS_WIN32   /* Windows types */
    EQ("ATOM", "WORD"),
    EQ("BOOL", "int"),
    EQ("BOOLEAN", "BYTE"),
    EQ("BYTE", "unsigned char"),
    EQ("CCHAR", "char"),
    EQ("CHAR", "char"),
    EQ("COLORREF", "DWORD"),
    EQ("DWORD", "unsigned long"),
    EQ("DWORD32", "unsigned int"),
    EQ("DWORD64", "unsigned long long"),
    EQ("DWORDLONG", "ULONGLONG"),
    EQ("DWORD_PTR", "ULONG_PTR"),
#endif

    EQ("FILE", "struct _IO_FILE"),

#ifdef MS_WIN32   /* more Windows types */
    EQ("FLOAT", "float"),
    EQ("HACCEL", "HANDLE"),
    EQ("HALF_PTR", W32_64("short","int")),
    EQ("HANDLE", "PVOID"),
    EQ("HBITMAP", "HANDLE"),
    EQ("HBRUSH", "HANDLE"),
    EQ("HCOLORSPACE", "HANDLE"),
    EQ("HCONV", "HANDLE"),
    EQ("HCONVLIST", "HANDLE"),
    EQ("HCURSOR", "HICON"),
    EQ("HDC", "HANDLE"),
    EQ("HDDEDATA", "HANDLE"),
    EQ("HDESK", "HANDLE"),
    EQ("HDROP", "HANDLE"),
    EQ("HDWP", "HANDLE"),
    EQ("HENHMETAFILE", "HANDLE"),
    EQ("HFILE", "int"),
    EQ("HFONT", "HANDLE"),
    EQ("HGDIOBJ", "HANDLE"),
    EQ("HGLOBAL", "HANDLE"),
    EQ("HHOOK", "HANDLE"),
    EQ("HICON", "HANDLE"),
    EQ("HINSTANCE", "HANDLE"),
    EQ("HKEY", "HANDLE"),
    EQ("HKL", "HANDLE"),
    EQ("HLOCAL", "HANDLE"),
    EQ("HMENU", "HANDLE"),
    EQ("HMETAFILE", "HANDLE"),
    EQ("HMODULE", "HINSTANCE"),
    EQ("HMONITOR", "HANDLE"),
    EQ("HPALETTE", "HANDLE"),
    EQ("HPEN", "HANDLE"),
    EQ("HRESULT", "LONG"),
    EQ("HRGN", "HANDLE"),
    EQ("HRSRC", "HANDLE"),
    EQ("HSZ", "HANDLE"),
    EQ("HWND", "HANDLE"),
    EQ("INT", "int"),
    EQ("INT16", "short"),
    EQ("INT32", "int"),
    EQ("INT64", "long long"),
    EQ("INT8", "signed char"),
    EQ("INT_PTR", W32_64("int","long long")),
    EQ("LANGID", "WORD"),
    EQ("LCID", "DWORD"),
    EQ("LCTYPE", "DWORD"),
    EQ("LGRPID", "DWORD"),
    EQ("LONG", "long"),
    EQ("LONG32", "int"),
    EQ("LONG64", "long long"),
    EQ("LONGLONG", "long long"),
    EQ("LONG_PTR", W32_64("long","long long")),
    EQ("LPARAM", "LONG_PTR"),
    EQ("LPBOOL", "BOOL *"),
    EQ("LPBYTE", "BYTE *"),
    EQ("LPCOLORREF", "DWORD *"),
    EQ("LPCSTR", "const char *"),
    EQ("LPCVOID", "const void *"),
    EQ("LPCWSTR", "const WCHAR *"),
    EQ("LPDWORD", "DWORD *"),
    EQ("LPHANDLE", "HANDLE *"),
    EQ("LPINT", "int *"),
    EQ("LPLONG", "long *"),
    EQ("LPSTR", "CHAR *"),
    EQ("LPVOID", "void *"),
    EQ("LPWORD", "WORD *"),
    EQ("LPWSTR", "WCHAR *"),
    EQ("LRESULT", "LONG_PTR"),
    EQ("PBOOL", "BOOL *"),
    EQ("PBOOLEAN", "BOOLEAN *"),
    EQ("PBYTE", "BYTE *"),
    EQ("PCHAR", "CHAR *"),
    EQ("PCSTR", "const CHAR *"),
    EQ("PCWSTR", "const WCHAR *"),
    EQ("PDWORD", "DWORD *"),
    EQ("PDWORD32", "DWORD32 *"),
    EQ("PDWORD64", "DWORD64 *"),
    EQ("PDWORDLONG", "DWORDLONG *"),
    EQ("PDWORD_PTR", "DWORD_PTR *"),
    EQ("PFLOAT", "FLOAT *"),
    EQ("PHALF_PTR", "HALF_PTR *"),
    EQ("PHANDLE", "HANDLE *"),
    EQ("PHKEY", "HKEY *"),
    EQ("PINT", "int *"),
    EQ("PINT16", "INT16 *"),
    EQ("PINT32", "INT32 *"),
    EQ("PINT64", "INT64 *"),
    EQ("PINT8", "INT8 *"),
    EQ("PINT_PTR", "INT_PTR *"),
    EQ("PLCID", "PDWORD"),
    EQ("PLONG", "LONG *"),
    EQ("PLONG32", "LONG32 *"),
    EQ("PLONG64", "LONG64 *"),
    EQ("PLONGLONG", "LONGLONG *"),
    EQ("PLONG_PTR", "LONG_PTR *"),
    EQ("PSHORT", "SHORT *"),
    EQ("PSIZE_T", "SIZE_T *"),
    EQ("PSSIZE_T", "SSIZE_T *"),
    EQ("PSTR", "CHAR *"),
    EQ("PUCHAR", "UCHAR *"),
    EQ("PUHALF_PTR", "UHALF_PTR *"),
    EQ("PUINT", "UINT *"),
    EQ("PUINT16", "UINT16 *"),
    EQ("PUINT32", "UINT32 *"),
    EQ("PUINT64", "UINT64 *"),
    EQ("PUINT8", "UINT8 *"),
    EQ("PUINT_PTR", "UINT_PTR *"),
    EQ("PULONG", "ULONG *"),
    EQ("PULONG32", "ULONG32 *"),
    EQ("PULONG64", "ULONG64 *"),
    EQ("PULONGLONG", "ULONGLONG *"),
    EQ("PULONG_PTR", "ULONG_PTR *"),
    EQ("PUSHORT", "USHORT *"),
    EQ("PVOID", "void *"),
    EQ("PWCHAR", "WCHAR *"),
    EQ("PWORD", "WORD *"),
    EQ("PWSTR", "WCHAR *"),
    EQ("QWORD", "unsigned long long"),
    EQ("SC_HANDLE", "HANDLE"),
    EQ("SC_LOCK", "LPVOID"),
    EQ("SERVICE_STATUS_HANDLE", "HANDLE"),
    EQ("SHORT", "short"),
    EQ("SIZE_T", "ULONG_PTR"),
    EQ("SSIZE_T", "LONG_PTR"),
    EQ("UCHAR", "unsigned char"),
    EQ("UHALF_PTR", W32_64("unsigned short","unsigned int")),
    EQ("UINT", "unsigned int"),
    EQ("UINT16", "unsigned short"),
    EQ("UINT32", "unsigned int"),
    EQ("UINT64", "unsigned long long"),
    EQ("UINT8", "unsigned char"),
    EQ("UINT_PTR", W32_64("unsigned int","unsigned long long")),
    EQ("ULONG", "unsigned long"),
    EQ("ULONG32", "unsigned int"),
    EQ("ULONG64", "unsigned long long"),
    EQ("ULONGLONG", "unsigned long long"),
    EQ("ULONG_PTR", W32_64("unsigned long","unsigned long long")),
    EQ("USHORT", "unsigned short"),
    EQ("USN", "LONGLONG"),
    EQ("VOID", "void"),
    EQ("WCHAR", "wchar_t"),
    EQ("WINSTA", "HANDLE"),
    EQ("WORD", "unsigned short"),
    EQ("WPARAM", "UINT_PTR"),
    EQ("_Dcomplex", "_cffi_double_complex_t"),
    EQ("_Fcomplex", "_cffi_float_complex_t"),
#endif

    EQ("bool", "_Bool"),
};


#undef EQ
#undef W32_64

#define num_common_simple_types    \
    (sizeof(common_simple_types) / sizeof(common_simple_types[0]))


static const char *get_common_type(const char *search, size_t search_len)
{
    const char *entry;
    int index = search_sorted(common_simple_types, sizeof(const char *),
                              num_common_simple_types, search, search_len);
    if (index < 0)
        return NULL;

    entry = common_simple_types[index];
    return entry + strlen(entry) + 1;
}

static PyObject *b__get_common_types(PyObject *self, PyObject *arg)
{
    int err;
    size_t i;
    for (i = 0; i < num_common_simple_types; i++) {
        const char *s = common_simple_types[i];
        PyObject *o = PyText_FromString(s + strlen(s) + 1);
        if (o == NULL)
            return NULL;
        err = PyDict_SetItemString(arg, s, o);
        Py_DECREF(o);
        if (err < 0)
            return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}
