/* Helper library for MSI creation with Python.
 * Copyright (C) 2005 Martin v. Löwis
 * Licensed to PSF under a contributor agreement.
 */

#include <Python.h>
#include <fci.h>
#include <fcntl.h>
#include <windows.h>
#include <msi.h>
#include <msiquery.h>
#include <msidefs.h>
#include <rpc.h>

/*[clinic input]
module _msi
class _msi.Record "msiobj *" "&record_Type"
class _msi.SummaryInformation "msiobj *" "&summary_Type"
class _msi.View "msiobj *" "&msiview_Type"
class _msi.Database "msiobj *" "&msidb_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=89a3605762cf4bdc]*/

static PyObject *MSIError;

/*[clinic input]
_msi.UuidCreate

Return the string representation of a new unique identifier.
[clinic start generated code]*/

static PyObject *
_msi_UuidCreate_impl(PyObject *module)
/*[clinic end generated code: output=534ecf36f10af98e input=168024ab4b3e832b]*/
{
    UUID result;
    wchar_t *cresult;
    PyObject *oresult;

    /* May return ok, local only, and no address.
       For local only, the documentation says we still get a uuid.
       For RPC_S_UUID_NO_ADDRESS, it's not clear whether we can
       use the result. */
    if (UuidCreate(&result) == RPC_S_UUID_NO_ADDRESS) {
        PyErr_SetString(PyExc_NotImplementedError, "processing 'no address' result");
        return NULL;
    }

    if (UuidToStringW(&result, &cresult) == RPC_S_OUT_OF_MEMORY) {
        PyErr_SetString(PyExc_MemoryError, "out of memory in uuidgen");
        return NULL;
    }

    oresult = PyUnicode_FromWideChar(cresult, wcslen(cresult));
    RpcStringFreeW(&cresult);
    return oresult;

}

/* Helper for converting file names from UTF-8 to wchat_t*.  */
static wchar_t *
utf8_to_wchar(const char *s, int *err)
{
    PyObject *obj = PyUnicode_FromString(s);
    if (obj == NULL) {
        if (PyErr_ExceptionMatches(PyExc_MemoryError)) {
            *err = ENOMEM;
        }
        else {
            *err = EINVAL;
        }
        PyErr_Clear();
        return NULL;
    }
    wchar_t *ws = PyUnicode_AsWideCharString(obj, NULL);
    if (ws == NULL) {
        *err = ENOMEM;
        PyErr_Clear();
    }
    Py_DECREF(obj);
    return ws;
}

/* FCI callback functions */

static FNFCIALLOC(cb_alloc)
{
    return PyMem_RawMalloc(cb);
}

static FNFCIFREE(cb_free)
{
    PyMem_RawFree(memory);
}

static FNFCIOPEN(cb_open)
{
    wchar_t *ws = utf8_to_wchar(pszFile, err);
    if (ws == NULL) {
        return -1;
    }
    int result = _wopen(ws, oflag | O_NOINHERIT, pmode);
    PyMem_Free(ws);
    if (result == -1)
        *err = errno;
    return result;
}

static FNFCIREAD(cb_read)
{
    UINT result = (UINT)_read((int)hf, memory, cb);
    if (result != cb)
        *err = errno;
    return result;
}

static FNFCIWRITE(cb_write)
{
    UINT result = (UINT)_write((int)hf, memory, cb);
    if (result != cb)
        *err = errno;
    return result;
}

static FNFCICLOSE(cb_close)
{
    int result = _close((int)hf);
    if (result != 0)
        *err = errno;
    return result;
}

static FNFCISEEK(cb_seek)
{
    long result = (long)_lseek((int)hf, dist, seektype);
    if (result == -1)
        *err = errno;
    return result;
}

static FNFCIDELETE(cb_delete)
{
    wchar_t *ws = utf8_to_wchar(pszFile, err);
    if (ws == NULL) {
        return -1;
    }
    int result = _wremove(ws);
    PyMem_Free(ws);
    if (result != 0)
        *err = errno;
    return result;
}

static FNFCIFILEPLACED(cb_fileplaced)
{
    return 0;
}

static FNFCIGETTEMPFILE(cb_gettempfile)
{
    char *name = _tempnam("", "tmp");
    if ((name != NULL) && ((int)strlen(name) < cbTempName)) {
        strcpy(pszTempName, name);
        free(name);
        return TRUE;
    }

    if (name) free(name);
    return FALSE;
}

static FNFCISTATUS(cb_status)
{
    if (pv) {
        _Py_IDENTIFIER(status);

        PyObject *result = _PyObject_CallMethodId(pv, &PyId_status, "iii", typeStatus, cb1, cb2);
        if (result == NULL)
            return -1;
        Py_DECREF(result);
    }
    return 0;
}

static FNFCIGETNEXTCABINET(cb_getnextcabinet)
{
    if (pv) {
        _Py_IDENTIFIER(getnextcabinet);

        PyObject *result = _PyObject_CallMethodId(pv, &PyId_getnextcabinet, "i", pccab->iCab);
        if (result == NULL)
            return -1;
        if (!PyBytes_Check(result)) {
            PyErr_Format(PyExc_TypeError,
                "Incorrect return type %s from getnextcabinet",
                Py_TYPE(result)->tp_name);
            Py_DECREF(result);
            return FALSE;
        }
        strncpy(pccab->szCab, PyBytes_AsString(result), sizeof(pccab->szCab));
        return TRUE;
    }
    return FALSE;
}

static FNFCIGETOPENINFO(cb_getopeninfo)
{
    BY_HANDLE_FILE_INFORMATION bhfi;
    FILETIME filetime;
    HANDLE handle;

    wchar_t *ws = utf8_to_wchar(pszName, err);
    if (ws == NULL) {
        return -1;
    }

    /* Need Win32 handle to get time stamps */
    handle = CreateFileW(ws, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        PyMem_Free(ws);
        return -1;
    }

    if (GetFileInformationByHandle(handle, &bhfi) == FALSE) {
        CloseHandle(handle);
        PyMem_Free(ws);
        return -1;
    }

    FileTimeToLocalFileTime(&bhfi.ftLastWriteTime, &filetime);
    FileTimeToDosDateTime(&filetime, pdate, ptime);

    *pattribs = (int)(bhfi.dwFileAttributes &
        (_A_RDONLY | _A_SYSTEM | _A_HIDDEN | _A_ARCH));

    CloseHandle(handle);

    int result = _wopen(ws, _O_RDONLY | _O_BINARY | O_NOINHERIT);
    PyMem_Free(ws);
    return result;
}

/*[clinic input]
_msi.FCICreate
    cabname: str
        the name of the CAB file
    files: object
        a list of tuples, each containing the name of the file on disk,
        and the name of the file inside the CAB file
    /

Create a new CAB file.
[clinic start generated code]*/

static PyObject *
_msi_FCICreate_impl(PyObject *module, const char *cabname, PyObject *files)
/*[clinic end generated code: output=55dc05728361b799 input=1d2d75fdc8b44b71]*/
{
    const char *p;
    CCAB ccab;
    HFCI hfci;
    ERF erf;
    Py_ssize_t i;

    if (!PyList_Check(files)) {
        PyErr_SetString(PyExc_TypeError, "FCICreate expects a list");
        return NULL;
    }

    ccab.cb = INT_MAX; /* no need to split CAB into multiple media */
    ccab.cbFolderThresh = 1000000; /* flush directory after this many bytes */
    ccab.cbReserveCFData = 0;
    ccab.cbReserveCFFolder = 0;
    ccab.cbReserveCFHeader = 0;

    ccab.iCab = 1;
    ccab.iDisk = 1;

    ccab.setID = 0;
    ccab.szDisk[0] = '\0';

    for (i = 0, p = cabname; *p; p++)
        if (*p == '\\' || *p == '/')
            i = p - cabname + 1;

    if (i >= sizeof(ccab.szCabPath) ||
        strlen(cabname+i) >= sizeof(ccab.szCab)) {
        PyErr_SetString(PyExc_ValueError, "path name too long");
        return 0;
    }

    if (i > 0) {
        memcpy(ccab.szCabPath, cabname, i);
        ccab.szCabPath[i] = '\0';
        strcpy(ccab.szCab, cabname+i);
    } else {
        strcpy(ccab.szCabPath, ".\\");
        strcpy(ccab.szCab, cabname);
    }

    hfci = FCICreate(&erf, cb_fileplaced, cb_alloc, cb_free,
        cb_open, cb_read, cb_write, cb_close, cb_seek, cb_delete,
        cb_gettempfile, &ccab, NULL);

    if (hfci == NULL) {
        PyErr_Format(PyExc_ValueError, "FCI error %d", erf.erfOper);
        return NULL;
    }

    for (i=0; i < PyList_GET_SIZE(files); i++) {
        PyObject *item = PyList_GET_ITEM(files, i);
        char *filename, *cabname;

        if (!PyArg_ParseTuple(item, "ss", &filename, &cabname)) {
            PyErr_SetString(PyExc_TypeError, "FCICreate expects a list of tuples containing two strings");
            FCIDestroy(hfci);
            return NULL;
        }

        if (!FCIAddFile(hfci, filename, cabname, FALSE,
            cb_getnextcabinet, cb_status, cb_getopeninfo,
            tcompTYPE_MSZIP))
            goto err;
    }

    if (!FCIFlushCabinet(hfci, FALSE, cb_getnextcabinet, cb_status))
        goto err;

    if (!FCIDestroy(hfci))
        goto err;

    Py_RETURN_NONE;
err:
    if(erf.fError)
        PyErr_Format(PyExc_ValueError, "FCI error %d", erf.erfOper); /* XXX better error type */
    else
        PyErr_SetString(PyExc_ValueError, "FCI general error");

    FCIDestroy(hfci);
    return NULL;
}

typedef struct msiobj{
    PyObject_HEAD
    MSIHANDLE h;
}msiobj;

static void
msiobj_dealloc(msiobj* msidb)
{
    MsiCloseHandle(msidb->h);
    msidb->h = 0;
    PyObject_Del(msidb);
}

static PyObject*
msierror(int status)
{
    int code;
    char buf[2000];
    char *res = buf;
    DWORD size = sizeof(buf);
    MSIHANDLE err = MsiGetLastErrorRecord();

    if (err == 0) {
        switch(status) {
        case ERROR_ACCESS_DENIED:
            PyErr_SetString(MSIError, "access denied");
            return NULL;
        case ERROR_FUNCTION_FAILED:
            PyErr_SetString(MSIError, "function failed");
            return NULL;
        case ERROR_INVALID_DATA:
            PyErr_SetString(MSIError, "invalid data");
            return NULL;
        case ERROR_INVALID_HANDLE:
            PyErr_SetString(MSIError, "invalid handle");
            return NULL;
        case ERROR_INVALID_STATE:
            PyErr_SetString(MSIError, "invalid state");
            return NULL;
        case ERROR_INVALID_PARAMETER:
            PyErr_SetString(MSIError, "invalid parameter");
            return NULL;
        case ERROR_OPEN_FAILED:
            PyErr_SetString(MSIError, "open failed");
            return NULL;
        case ERROR_CREATE_FAILED:
            PyErr_SetString(MSIError, "create failed");
            return NULL;
        default:
            PyErr_Format(MSIError, "unknown error %x", status);
            return NULL;
        }
    }

    code = MsiRecordGetInteger(err, 1); /* XXX code */
    if (MsiFormatRecord(0, err, res, &size) == ERROR_MORE_DATA) {
        res = malloc(size+1);
        if (res == NULL) {
            MsiCloseHandle(err);
            return PyErr_NoMemory();
        }
        MsiFormatRecord(0, err, res, &size);
        res[size]='\0';
    }
    MsiCloseHandle(err);
    PyErr_SetString(MSIError, res);
    if (res != buf)
        free(res);
    return NULL;
}

#include "clinic/_msi.c.h"

/*[clinic input]
_msi.Database.Close

Close the database object.
[clinic start generated code]*/

static PyObject *
_msi_Database_Close_impl(msiobj *self)
/*[clinic end generated code: output=ddf2d7712ea804f1 input=104330ce4a486187]*/
{
    int status;
    if ((status = MsiCloseHandle(self->h)) != ERROR_SUCCESS) {
        return msierror(status);
    }
    self->h = 0;
    Py_RETURN_NONE;
}

/*************************** Record objects **********************/

/*[clinic input]
_msi.Record.GetFieldCount

Return the number of fields of the record.
[clinic start generated code]*/

static PyObject *
_msi_Record_GetFieldCount_impl(msiobj *self)
/*[clinic end generated code: output=112795079c904398 input=5fb9d4071b28897b]*/
{
    return PyLong_FromLong(MsiRecordGetFieldCount(self->h));
}

/*[clinic input]
_msi.Record.GetInteger
    field: unsigned_int(bitwise=True)
    /

Return the value of field as an integer where possible.
[clinic start generated code]*/

static PyObject *
_msi_Record_GetInteger_impl(msiobj *self, unsigned int field)
/*[clinic end generated code: output=7174ebb6e8ed1c79 input=d19209947e2bfe61]*/
{
    int status;

    status = MsiRecordGetInteger(self->h, field);
    if (status == MSI_NULL_INTEGER){
        PyErr_SetString(MSIError, "could not convert record field to integer");
        return NULL;
    }
    return PyLong_FromLong((long) status);
}

/*[clinic input]
_msi.Record.GetString
    field: unsigned_int(bitwise=True)
    /

Return the value of field as a string where possible.
[clinic start generated code]*/

static PyObject *
_msi_Record_GetString_impl(msiobj *self, unsigned int field)
/*[clinic end generated code: output=f670d1b484cfa47c input=ffa11f21450b77d8]*/
{
    unsigned int status;
    WCHAR buf[2000];
    WCHAR *res = buf;
    DWORD size = sizeof(buf);
    PyObject* string;

    status = MsiRecordGetStringW(self->h, field, res, &size);
    if (status == ERROR_MORE_DATA) {
        res = (WCHAR*) malloc((size + 1)*sizeof(WCHAR));
        if (res == NULL)
            return PyErr_NoMemory();
        status = MsiRecordGetStringW(self->h, field, res, &size);
    }
    if (status != ERROR_SUCCESS)
        return msierror((int) status);
    string = PyUnicode_FromWideChar(res, size);
    if (buf != res)
        free(res);
    return string;
}

/*[clinic input]
_msi.Record.ClearData

Set all fields of the record to 0.
[clinic start generated code]*/

static PyObject *
_msi_Record_ClearData_impl(msiobj *self)
/*[clinic end generated code: output=1891467214b977f4 input=2a911c95aaded102]*/
{
    int status = MsiRecordClearData(self->h);
    if (status != ERROR_SUCCESS)
        return msierror(status);

    Py_RETURN_NONE;
}

/*[clinic input]
_msi.Record.SetString
    field: int
    value: Py_UNICODE
    /

Set field to a string value.
[clinic start generated code]*/

static PyObject *
_msi_Record_SetString_impl(msiobj *self, int field, const Py_UNICODE *value)
/*[clinic end generated code: output=2e37505b0f11f985 input=fb8ec70a2a6148e0]*/
{
    int status;

    if ((status = MsiRecordSetStringW(self->h, field, value)) != ERROR_SUCCESS)
        return msierror(status);

    Py_RETURN_NONE;
}

/*[clinic input]
_msi.Record.SetStream
    field: int
    value: Py_UNICODE
    /

Set field to the contents of the file named value.
[clinic start generated code]*/

static PyObject *
_msi_Record_SetStream_impl(msiobj *self, int field, const Py_UNICODE *value)
/*[clinic end generated code: output=442facac16913b48 input=a07aa19b865e8292]*/
{
    int status;

    if ((status = MsiRecordSetStreamW(self->h, field, value)) != ERROR_SUCCESS)
        return msierror(status);

    Py_RETURN_NONE;
}

/*[clinic input]
_msi.Record.SetInteger
    field: int
    value: int
    /

Set field to an integer value.
[clinic start generated code]*/

static PyObject *
_msi_Record_SetInteger_impl(msiobj *self, int field, int value)
/*[clinic end generated code: output=669e8647775d0ce7 input=c571aa775e7e451b]*/
{
    int status;

    if ((status = MsiRecordSetInteger(self->h, field, value)) != ERROR_SUCCESS)
        return msierror(status);

    Py_RETURN_NONE;
}



static PyMethodDef record_methods[] = {
    _MSI_RECORD_GETFIELDCOUNT_METHODDEF
    _MSI_RECORD_GETINTEGER_METHODDEF
    _MSI_RECORD_GETSTRING_METHODDEF
    _MSI_RECORD_SETSTRING_METHODDEF
    _MSI_RECORD_SETSTREAM_METHODDEF
    _MSI_RECORD_SETINTEGER_METHODDEF
    _MSI_RECORD_CLEARDATA_METHODDEF
    { NULL, NULL }
};

static PyTypeObject record_Type = {
        PyVarObject_HEAD_INIT(NULL, 0)
        "_msi.Record",          /*tp_name*/
        sizeof(msiobj), /*tp_basicsize*/
        0,                      /*tp_itemsize*/
        /* methods */
        (destructor)msiobj_dealloc, /*tp_dealloc*/
        0,                      /*tp_vectorcall_offset*/
        0,                      /*tp_getattr*/
        0,                      /*tp_setattr*/
        0,                      /*tp_as_async*/
        0,                      /*tp_repr*/
        0,                      /*tp_as_number*/
        0,                      /*tp_as_sequence*/
        0,                      /*tp_as_mapping*/
        0,                      /*tp_hash*/
        0,                      /*tp_call*/
        0,                      /*tp_str*/
        PyObject_GenericGetAttr,/*tp_getattro*/
        PyObject_GenericSetAttr,/*tp_setattro*/
        0,                      /*tp_as_buffer*/
        Py_TPFLAGS_DEFAULT,     /*tp_flags*/
        0,                      /*tp_doc*/
        0,                      /*tp_traverse*/
        0,                      /*tp_clear*/
        0,                      /*tp_richcompare*/
        0,                      /*tp_weaklistoffset*/
        0,                      /*tp_iter*/
        0,                      /*tp_iternext*/
        record_methods,           /*tp_methods*/
        0,                      /*tp_members*/
        0,                      /*tp_getset*/
        0,                      /*tp_base*/
        0,                      /*tp_dict*/
        0,                      /*tp_descr_get*/
        0,                      /*tp_descr_set*/
        0,                      /*tp_dictoffset*/
        0,                      /*tp_init*/
        0,                      /*tp_alloc*/
        0,                      /*tp_new*/
        0,                      /*tp_free*/
        0,                      /*tp_is_gc*/
};

static PyObject*
record_new(MSIHANDLE h)
{
    msiobj *result = PyObject_New(struct msiobj, &record_Type);

    if (!result) {
        MsiCloseHandle(h);
        return NULL;
    }

    result->h = h;
    return (PyObject*)result;
}

/*************************** SummaryInformation objects **************/

/*[clinic input]
_msi.SummaryInformation.GetProperty
    field: int
        the name of the property, one of the PID_* constants
    /

Return a property of the summary.
[clinic start generated code]*/

static PyObject *
_msi_SummaryInformation_GetProperty_impl(msiobj *self, int field)
/*[clinic end generated code: output=f8946a33ee14f6ef input=f8dfe2c890d6cb8b]*/
{
    int status;
    PyObject *result;
    UINT type;
    INT ival;
    FILETIME fval;
    char sbuf[1000];
    char *sval = sbuf;
    DWORD ssize = sizeof(sbuf);

    status = MsiSummaryInfoGetProperty(self->h, field, &type, &ival,
        &fval, sval, &ssize);
    if (status == ERROR_MORE_DATA) {
        ssize++;
        sval = malloc(ssize);
        if (sval == NULL) {
            return PyErr_NoMemory();
        }
        status = MsiSummaryInfoGetProperty(self->h, field, &type, &ival,
            &fval, sval, &ssize);
    }
    if (status != ERROR_SUCCESS) {
        return msierror(status);
    }

    switch(type) {
        case VT_I2:
        case VT_I4:
            result = PyLong_FromLong(ival);
            break;
        case VT_FILETIME:
            PyErr_SetString(PyExc_NotImplementedError, "FILETIME result");
            result = NULL;
            break;
        case VT_LPSTR:
            result = PyBytes_FromStringAndSize(sval, ssize);
            break;
        case VT_EMPTY:
            Py_INCREF(Py_None);
            result = Py_None;
            break;
        default:
            PyErr_Format(PyExc_NotImplementedError, "result of type %d", type);
            result = NULL;
            break;
    }
    if (sval != sbuf)
        free(sval);
    return result;
}

/*[clinic input]
_msi.SummaryInformation.GetPropertyCount

Return the number of summary properties.
[clinic start generated code]*/

static PyObject *
_msi_SummaryInformation_GetPropertyCount_impl(msiobj *self)
/*[clinic end generated code: output=68e94b2aeee92b3d input=2e71e985586d82dc]*/
{
    int status;
    UINT result;

    status = MsiSummaryInfoGetPropertyCount(self->h, &result);
    if (status != ERROR_SUCCESS)
        return msierror(status);

    return PyLong_FromLong(result);
}

/*[clinic input]
_msi.SummaryInformation.SetProperty
    field: int
        the name of the property, one of the PID_* constants
    value as data: object
        the new value of the property (integer or string)
    /

Set a property.
[clinic start generated code]*/

static PyObject *
_msi_SummaryInformation_SetProperty_impl(msiobj *self, int field,
                                         PyObject *data)
/*[clinic end generated code: output=3d4692c8984bb675 input=f2a7811b905abbed]*/
{
    int status;

    if (PyUnicode_Check(data)) {
#if USE_UNICODE_WCHAR_CACHE
        const WCHAR *value = _PyUnicode_AsUnicode(data);
#else /* USE_UNICODE_WCHAR_CACHE */
        WCHAR *value = PyUnicode_AsWideCharString(data, NULL);
#endif /* USE_UNICODE_WCHAR_CACHE */
        if (value == NULL) {
            return NULL;
        }
        status = MsiSummaryInfoSetPropertyW(self->h, field, VT_LPSTR,
            0, NULL, value);
#if !USE_UNICODE_WCHAR_CACHE
        PyMem_Free(value);
#endif /* USE_UNICODE_WCHAR_CACHE */
    } else if (PyLong_CheckExact(data)) {
        long value = PyLong_AsLong(data);
        if (value == -1 && PyErr_Occurred()) {
            return NULL;
        }
        status = MsiSummaryInfoSetProperty(self->h, field, VT_I4,
            value, NULL, NULL);
    } else {
        PyErr_SetString(PyExc_TypeError, "unsupported type");
        return NULL;
    }

    if (status != ERROR_SUCCESS)
        return msierror(status);

    Py_RETURN_NONE;
}


/*[clinic input]
_msi.SummaryInformation.Persist

Write the modified properties to the summary information stream.
[clinic start generated code]*/

static PyObject *
_msi_SummaryInformation_Persist_impl(msiobj *self)
/*[clinic end generated code: output=c564bd17f5e122c9 input=e3dda9d530095ef7]*/
{
    int status;

    status = MsiSummaryInfoPersist(self->h);
    if (status != ERROR_SUCCESS)
        return msierror(status);
    Py_RETURN_NONE;
}

static PyMethodDef summary_methods[] = {
    _MSI_SUMMARYINFORMATION_GETPROPERTY_METHODDEF
    _MSI_SUMMARYINFORMATION_GETPROPERTYCOUNT_METHODDEF
    _MSI_SUMMARYINFORMATION_SETPROPERTY_METHODDEF
    _MSI_SUMMARYINFORMATION_PERSIST_METHODDEF
    { NULL, NULL }
};

static PyTypeObject summary_Type = {
        PyVarObject_HEAD_INIT(NULL, 0)
        "_msi.SummaryInformation",              /*tp_name*/
        sizeof(msiobj), /*tp_basicsize*/
        0,                      /*tp_itemsize*/
        /* methods */
        (destructor)msiobj_dealloc, /*tp_dealloc*/
        0,                      /*tp_vectorcall_offset*/
        0,                      /*tp_getattr*/
        0,                      /*tp_setattr*/
        0,                      /*tp_as_async*/
        0,                      /*tp_repr*/
        0,                      /*tp_as_number*/
        0,                      /*tp_as_sequence*/
        0,                      /*tp_as_mapping*/
        0,                      /*tp_hash*/
        0,                      /*tp_call*/
        0,                      /*tp_str*/
        PyObject_GenericGetAttr,/*tp_getattro*/
        PyObject_GenericSetAttr,/*tp_setattro*/
        0,                      /*tp_as_buffer*/
        Py_TPFLAGS_DEFAULT,     /*tp_flags*/
        0,                      /*tp_doc*/
        0,                      /*tp_traverse*/
        0,                      /*tp_clear*/
        0,                      /*tp_richcompare*/
        0,                      /*tp_weaklistoffset*/
        0,                      /*tp_iter*/
        0,                      /*tp_iternext*/
        summary_methods,        /*tp_methods*/
        0,                      /*tp_members*/
        0,                      /*tp_getset*/
        0,                      /*tp_base*/
        0,                      /*tp_dict*/
        0,                      /*tp_descr_get*/
        0,                      /*tp_descr_set*/
        0,                      /*tp_dictoffset*/
        0,                      /*tp_init*/
        0,                      /*tp_alloc*/
        0,                      /*tp_new*/
        0,                      /*tp_free*/
        0,                      /*tp_is_gc*/
};

/*************************** View objects **************/

/*[clinic input]
_msi.View.Execute
    params as oparams: object
        a record describing actual values of the parameter tokens
        in the query or None
    /

Execute the SQL query of the view.
[clinic start generated code]*/

static PyObject *
_msi_View_Execute(msiobj *self, PyObject *oparams)
/*[clinic end generated code: output=f0f65fd2900bcb4e input=cb163a15d453348e]*/
{
    int status;
    MSIHANDLE params = 0;

    if (oparams != Py_None) {
        if (!Py_IS_TYPE(oparams, &record_Type)) {
            PyErr_SetString(PyExc_TypeError, "Execute argument must be a record");
            return NULL;
        }
        params = ((msiobj*)oparams)->h;
    }

    status = MsiViewExecute(self->h, params);
    if (status != ERROR_SUCCESS)
        return msierror(status);

    Py_RETURN_NONE;
}

/*[clinic input]
_msi.View.Fetch

Return a result record of the query.
[clinic start generated code]*/

static PyObject *
_msi_View_Fetch_impl(msiobj *self)
/*[clinic end generated code: output=ba154a3794537d4e input=7f3e3d06c449001c]*/
{
    int status;
    MSIHANDLE result;

    status = MsiViewFetch(self->h, &result);
    if (status == ERROR_NO_MORE_ITEMS) {
        Py_RETURN_NONE;
    } else if (status != ERROR_SUCCESS) {
        return msierror(status);
    }

    return record_new(result);
}

/*[clinic input]
_msi.View.GetColumnInfo
    kind: int
        MSICOLINFO_NAMES or MSICOLINFO_TYPES
    /

Return a record describing the columns of the view.
[clinic start generated code]*/

static PyObject *
_msi_View_GetColumnInfo_impl(msiobj *self, int kind)
/*[clinic end generated code: output=e7c1697db9403660 input=afedb892bf564a3b]*/
{
    int status;
    MSIHANDLE result;

    if ((status = MsiViewGetColumnInfo(self->h, kind, &result)) != ERROR_SUCCESS)
        return msierror(status);

    return record_new(result);
}

/*[clinic input]
_msi.View.Modify
    kind: int
        one of the MSIMODIFY_* constants
    data: object
        a record describing the new data
    /

Modify the view.
[clinic start generated code]*/

static PyObject *
_msi_View_Modify_impl(msiobj *self, int kind, PyObject *data)
/*[clinic end generated code: output=69aaf3ce8ddac0ba input=2828de22de0d47b4]*/
{
    int status;

    if (!Py_IS_TYPE(data, &record_Type)) {
        PyErr_SetString(PyExc_TypeError, "Modify expects a record object");
        return NULL;
    }

    if ((status = MsiViewModify(self->h, kind, ((msiobj*)data)->h)) != ERROR_SUCCESS)
        return msierror(status);

    Py_RETURN_NONE;
}

/*[clinic input]
_msi.View.Close

Close the view.
[clinic start generated code]*/

static PyObject *
_msi_View_Close_impl(msiobj *self)
/*[clinic end generated code: output=488f7b8645ca104a input=de6927d1308c401c]*/
{
    int status;

    if ((status = MsiViewClose(self->h)) != ERROR_SUCCESS)
        return msierror(status);

    Py_RETURN_NONE;
}

static PyMethodDef view_methods[] = {
    _MSI_VIEW_EXECUTE_METHODDEF
    _MSI_VIEW_GETCOLUMNINFO_METHODDEF
    _MSI_VIEW_FETCH_METHODDEF
    _MSI_VIEW_MODIFY_METHODDEF
    _MSI_VIEW_CLOSE_METHODDEF
    { NULL, NULL }
};

static PyTypeObject msiview_Type = {
        PyVarObject_HEAD_INIT(NULL, 0)
        "_msi.View",            /*tp_name*/
        sizeof(msiobj), /*tp_basicsize*/
        0,                      /*tp_itemsize*/
        /* methods */
        (destructor)msiobj_dealloc, /*tp_dealloc*/
        0,                      /*tp_vectorcall_offset*/
        0,                      /*tp_getattr*/
        0,                      /*tp_setattr*/
        0,                      /*tp_as_async*/
        0,                      /*tp_repr*/
        0,                      /*tp_as_number*/
        0,                      /*tp_as_sequence*/
        0,                      /*tp_as_mapping*/
        0,                      /*tp_hash*/
        0,                      /*tp_call*/
        0,                      /*tp_str*/
        PyObject_GenericGetAttr,/*tp_getattro*/
        PyObject_GenericSetAttr,/*tp_setattro*/
        0,                      /*tp_as_buffer*/
        Py_TPFLAGS_DEFAULT,     /*tp_flags*/
        0,                      /*tp_doc*/
        0,                      /*tp_traverse*/
        0,                      /*tp_clear*/
        0,                      /*tp_richcompare*/
        0,                      /*tp_weaklistoffset*/
        0,                      /*tp_iter*/
        0,                      /*tp_iternext*/
        view_methods,           /*tp_methods*/
        0,                      /*tp_members*/
        0,                      /*tp_getset*/
        0,                      /*tp_base*/
        0,                      /*tp_dict*/
        0,                      /*tp_descr_get*/
        0,                      /*tp_descr_set*/
        0,                      /*tp_dictoffset*/
        0,                      /*tp_init*/
        0,                      /*tp_alloc*/
        0,                      /*tp_new*/
        0,                      /*tp_free*/
        0,                      /*tp_is_gc*/
};

/*************************** Database objects **************/

/*[clinic input]
_msi.Database.OpenView
    sql: Py_UNICODE
        the SQL statement to execute
    /

Return a view object.
[clinic start generated code]*/

static PyObject *
_msi_Database_OpenView_impl(msiobj *self, const Py_UNICODE *sql)
/*[clinic end generated code: output=e712e6a11229abfd input=50f1771f37e500df]*/
{
    int status;
    MSIHANDLE hView;
    msiobj *result;

    if ((status = MsiDatabaseOpenViewW(self->h, sql, &hView)) != ERROR_SUCCESS)
        return msierror(status);

    result = PyObject_New(struct msiobj, &msiview_Type);
    if (!result) {
        MsiCloseHandle(hView);
        return NULL;
    }

    result->h = hView;
    return (PyObject*)result;
}

/*[clinic input]
_msi.Database.Commit

Commit the changes pending in the current transaction.
[clinic start generated code]*/

static PyObject *
_msi_Database_Commit_impl(msiobj *self)
/*[clinic end generated code: output=f33021feb8b0cdd8 input=375bb120d402266d]*/
{
    int status;

    if ((status = MsiDatabaseCommit(self->h)) != ERROR_SUCCESS)
        return msierror(status);

    Py_RETURN_NONE;
}

/*[clinic input]
_msi.Database.GetSummaryInformation
    count: int
        the maximum number of updated values
    /

Return a new summary information object.
[clinic start generated code]*/

static PyObject *
_msi_Database_GetSummaryInformation_impl(msiobj *self, int count)
/*[clinic end generated code: output=781e51a4ea4da847 input=18a899ead6521735]*/
{
    int status;
    MSIHANDLE result;
    msiobj *oresult;

    status = MsiGetSummaryInformation(self->h, NULL, count, &result);
    if (status != ERROR_SUCCESS)
        return msierror(status);

    oresult = PyObject_New(struct msiobj, &summary_Type);
    if (!oresult) {
        MsiCloseHandle(result);
        return NULL;
    }

    oresult->h = result;
    return (PyObject*)oresult;
}

static PyMethodDef db_methods[] = {
    _MSI_DATABASE_OPENVIEW_METHODDEF
    _MSI_DATABASE_COMMIT_METHODDEF
    _MSI_DATABASE_GETSUMMARYINFORMATION_METHODDEF
    _MSI_DATABASE_CLOSE_METHODDEF
    { NULL, NULL }
};

static PyTypeObject msidb_Type = {
        PyVarObject_HEAD_INIT(NULL, 0)
        "_msi.Database",                /*tp_name*/
        sizeof(msiobj), /*tp_basicsize*/
        0,                      /*tp_itemsize*/
        /* methods */
        (destructor)msiobj_dealloc, /*tp_dealloc*/
        0,                      /*tp_vectorcall_offset*/
        0,                      /*tp_getattr*/
        0,                      /*tp_setattr*/
        0,                      /*tp_as_async*/
        0,                      /*tp_repr*/
        0,                      /*tp_as_number*/
        0,                      /*tp_as_sequence*/
        0,                      /*tp_as_mapping*/
        0,                      /*tp_hash*/
        0,                      /*tp_call*/
        0,                      /*tp_str*/
        PyObject_GenericGetAttr,/*tp_getattro*/
        PyObject_GenericSetAttr,/*tp_setattro*/
        0,                      /*tp_as_buffer*/
        Py_TPFLAGS_DEFAULT,     /*tp_flags*/
        0,                      /*tp_doc*/
        0,                      /*tp_traverse*/
        0,                      /*tp_clear*/
        0,                      /*tp_richcompare*/
        0,                      /*tp_weaklistoffset*/
        0,                      /*tp_iter*/
        0,                      /*tp_iternext*/
        db_methods,             /*tp_methods*/
        0,                      /*tp_members*/
        0,                      /*tp_getset*/
        0,                      /*tp_base*/
        0,                      /*tp_dict*/
        0,                      /*tp_descr_get*/
        0,                      /*tp_descr_set*/
        0,                      /*tp_dictoffset*/
        0,                      /*tp_init*/
        0,                      /*tp_alloc*/
        0,                      /*tp_new*/
        0,                      /*tp_free*/
        0,                      /*tp_is_gc*/
};

#define Py_NOT_PERSIST(x, flag)                        \
    (x != (SIZE_T)(flag) &&                      \
    x != ((SIZE_T)(flag) | MSIDBOPEN_PATCHFILE))

#define Py_INVALID_PERSIST(x)                \
    (Py_NOT_PERSIST(x, MSIDBOPEN_READONLY) &&  \
    Py_NOT_PERSIST(x, MSIDBOPEN_TRANSACT) &&   \
    Py_NOT_PERSIST(x, MSIDBOPEN_DIRECT) &&     \
    Py_NOT_PERSIST(x, MSIDBOPEN_CREATE) &&     \
    Py_NOT_PERSIST(x, MSIDBOPEN_CREATEDIRECT))

/*[clinic input]
_msi.OpenDatabase
    path: Py_UNICODE
        the file name of the MSI file
    persist: int
        the persistence mode
    /

Return a new database object.
[clinic start generated code]*/

static PyObject *
_msi_OpenDatabase_impl(PyObject *module, const Py_UNICODE *path, int persist)
/*[clinic end generated code: output=d34b7202b745de05 input=1300f3b97659559b]*/
{
    int status;
    MSIHANDLE h;
    msiobj *result;

    /* We need to validate that persist is a valid MSIDBOPEN_* value. Otherwise,
       MsiOpenDatabase may treat the value as a pointer, leading to unexpected
       behavior. */
    if (Py_INVALID_PERSIST(persist))
        return msierror(ERROR_INVALID_PARAMETER);
    status = MsiOpenDatabaseW(path, (LPCWSTR)(SIZE_T)persist, &h);
    if (status != ERROR_SUCCESS)
        return msierror(status);

    result = PyObject_New(struct msiobj, &msidb_Type);
    if (!result) {
        MsiCloseHandle(h);
        return NULL;
    }
    result->h = h;
    return (PyObject*)result;
}

/*[clinic input]
_msi.CreateRecord
    count: int
        the number of fields of the record
    /

Return a new record object.
[clinic start generated code]*/

static PyObject *
_msi_CreateRecord_impl(PyObject *module, int count)
/*[clinic end generated code: output=0ba0a00beea3e99e input=53f17d5b5d9b077d]*/
{
    MSIHANDLE h;

    h = MsiCreateRecord(count);
    if (h == 0)
        return msierror(0);

    return record_new(h);
}


static PyMethodDef msi_methods[] = {
        _MSI_UUIDCREATE_METHODDEF
        _MSI_FCICREATE_METHODDEF
        _MSI_OPENDATABASE_METHODDEF
        _MSI_CREATERECORD_METHODDEF
        {NULL,          NULL}           /* sentinel */
};

static char msi_doc[] = "Documentation";


static struct PyModuleDef _msimodule = {
        PyModuleDef_HEAD_INIT,
        "_msi",
        msi_doc,
        -1,
        msi_methods,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC
PyInit__msi(void)
{
    PyObject *m;

    m = PyModule_Create(&_msimodule);
    if (m == NULL)
        return NULL;

    PyModule_AddIntConstant(m, "MSIDBOPEN_CREATEDIRECT", (long)(SIZE_T)MSIDBOPEN_CREATEDIRECT);
    PyModule_AddIntConstant(m, "MSIDBOPEN_CREATE", (long)(SIZE_T)MSIDBOPEN_CREATE);
    PyModule_AddIntConstant(m, "MSIDBOPEN_DIRECT", (long)(SIZE_T)MSIDBOPEN_DIRECT);
    PyModule_AddIntConstant(m, "MSIDBOPEN_READONLY", (long)(SIZE_T)MSIDBOPEN_READONLY);
    PyModule_AddIntConstant(m, "MSIDBOPEN_TRANSACT", (long)(SIZE_T)MSIDBOPEN_TRANSACT);
    PyModule_AddIntConstant(m, "MSIDBOPEN_PATCHFILE", (long)(SIZE_T)MSIDBOPEN_PATCHFILE);

    PyModule_AddIntMacro(m, MSICOLINFO_NAMES);
    PyModule_AddIntMacro(m, MSICOLINFO_TYPES);

    PyModule_AddIntMacro(m, MSIMODIFY_SEEK);
    PyModule_AddIntMacro(m, MSIMODIFY_REFRESH);
    PyModule_AddIntMacro(m, MSIMODIFY_INSERT);
    PyModule_AddIntMacro(m, MSIMODIFY_UPDATE);
    PyModule_AddIntMacro(m, MSIMODIFY_ASSIGN);
    PyModule_AddIntMacro(m, MSIMODIFY_REPLACE);
    PyModule_AddIntMacro(m, MSIMODIFY_MERGE);
    PyModule_AddIntMacro(m, MSIMODIFY_DELETE);
    PyModule_AddIntMacro(m, MSIMODIFY_INSERT_TEMPORARY);
    PyModule_AddIntMacro(m, MSIMODIFY_VALIDATE);
    PyModule_AddIntMacro(m, MSIMODIFY_VALIDATE_NEW);
    PyModule_AddIntMacro(m, MSIMODIFY_VALIDATE_FIELD);
    PyModule_AddIntMacro(m, MSIMODIFY_VALIDATE_DELETE);

    PyModule_AddIntMacro(m, PID_CODEPAGE);
    PyModule_AddIntMacro(m, PID_TITLE);
    PyModule_AddIntMacro(m, PID_SUBJECT);
    PyModule_AddIntMacro(m, PID_AUTHOR);
    PyModule_AddIntMacro(m, PID_KEYWORDS);
    PyModule_AddIntMacro(m, PID_COMMENTS);
    PyModule_AddIntMacro(m, PID_TEMPLATE);
    PyModule_AddIntMacro(m, PID_LASTAUTHOR);
    PyModule_AddIntMacro(m, PID_REVNUMBER);
    PyModule_AddIntMacro(m, PID_LASTPRINTED);
    PyModule_AddIntMacro(m, PID_CREATE_DTM);
    PyModule_AddIntMacro(m, PID_LASTSAVE_DTM);
    PyModule_AddIntMacro(m, PID_PAGECOUNT);
    PyModule_AddIntMacro(m, PID_WORDCOUNT);
    PyModule_AddIntMacro(m, PID_CHARCOUNT);
    PyModule_AddIntMacro(m, PID_APPNAME);
    PyModule_AddIntMacro(m, PID_SECURITY);

    MSIError = PyErr_NewException ("_msi.MSIError", NULL, NULL);
    Py_XINCREF(m);
    if (PyModule_Add(m, "MSIError", MSIError) < 0) {
        Py_DECREF(m);
        return NULL;
    }
    return m;
}
