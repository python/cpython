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

static PyObject *MSIError;

static PyObject*
uuidcreate(PyObject* obj, PyObject*args)
{
    UUID result;
    char *cresult;
    PyObject *oresult;

    /* May return ok, local only, and no address.
       For local only, the documentation says we still get a uuid.
       For RPC_S_UUID_NO_ADDRESS, it's not clear whether we can
       use the result. */
    if (UuidCreate(&result) == RPC_S_UUID_NO_ADDRESS) {
        PyErr_SetString(PyExc_NotImplementedError, "processing 'no address' result");
        return NULL;
    }

    if (UuidToString(&result, &cresult) == RPC_S_OUT_OF_MEMORY) {
        PyErr_SetString(PyExc_MemoryError, "out of memory in uuidgen");
        return NULL;
    }

    oresult = PyString_FromString(cresult);
    RpcStringFree(&cresult);
    return oresult;

}

/* FCI callback functions */

static FNFCIALLOC(cb_alloc)
{
    return malloc(cb);
}

static FNFCIFREE(cb_free)
{
    free(memory);
}

static FNFCIOPEN(cb_open)
{
    int result = _open(pszFile, oflag, pmode);
    if (result == -1)
        *err = errno;
    return result;
}

static FNFCIREAD(cb_read)
{
    UINT result = (UINT)_read(hf, memory, cb);
    if (result != cb)
        *err = errno;
    return result;
}

static FNFCIWRITE(cb_write)
{
    UINT result = (UINT)_write(hf, memory, cb);
    if (result != cb)
        *err = errno;
    return result;
}

static FNFCICLOSE(cb_close)
{
    int result = _close(hf);
    if (result != 0)
        *err = errno;
    return result;
}

static FNFCISEEK(cb_seek)
{
    long result = (long)_lseek(hf, dist, seektype);
    if (result == -1)
        *err = errno;
    return result;
}

static FNFCIDELETE(cb_delete)
{
    int result = remove(pszFile);
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
        PyObject *result = PyObject_CallMethod(pv, "status", "iii", typeStatus, cb1, cb2);
        if (result == NULL)
            return -1;
        Py_DECREF(result);
    }
    return 0;
}

static FNFCIGETNEXTCABINET(cb_getnextcabinet)
{
    if (pv) {
        PyObject *result = PyObject_CallMethod(pv, "getnextcabinet", "i", pccab->iCab);
        if (result == NULL)
            return -1;
        if (!PyString_Check(result)) {
            PyErr_Format(PyExc_TypeError,
                "Incorrect return type %s from getnextcabinet",
                result->ob_type->tp_name);
            Py_DECREF(result);
            return FALSE;
        }
        strncpy(pccab->szCab, PyString_AsString(result), sizeof(pccab->szCab));
        return TRUE;
    }
    return FALSE;
}

static FNFCIGETOPENINFO(cb_getopeninfo)
{
    BY_HANDLE_FILE_INFORMATION bhfi;
    FILETIME filetime;
    HANDLE handle;

    /* Need Win32 handle to get time stamps */
    handle = CreateFile(pszName, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE)
        return -1;

    if (GetFileInformationByHandle(handle, &bhfi) == FALSE)
    {
        CloseHandle(handle);
        return -1;
    }

    FileTimeToLocalFileTime(&bhfi.ftLastWriteTime, &filetime);
    FileTimeToDosDateTime(&filetime, pdate, ptime);

    *pattribs = (int)(bhfi.dwFileAttributes &
        (_A_RDONLY | _A_SYSTEM | _A_HIDDEN | _A_ARCH));

    CloseHandle(handle);

    return _open(pszName, _O_RDONLY | _O_BINARY);
}

static PyObject* fcicreate(PyObject* obj, PyObject* args)
{
    char *cabname, *p;
    PyObject *files;
    CCAB ccab;
    HFCI hfci;
    ERF erf;
    Py_ssize_t i;


    if (!PyArg_ParseTuple(args, "sO:FCICreate", &cabname, &files))
        return NULL;

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

    for (i = 0, p = cabname; *p; p = CharNext(p))
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
        if (!PyArg_ParseTuple(item, "ss", &filename, &cabname))
            goto err;
        if (!FCIAddFile(hfci, filename, cabname, FALSE,
            cb_getnextcabinet, cb_status, cb_getopeninfo,
            tcompTYPE_MSZIP))
            goto err;
    }

    if (!FCIFlushCabinet(hfci, FALSE, cb_getnextcabinet, cb_status))
        goto err;

    if (!FCIDestroy(hfci))
        goto err;

    Py_INCREF(Py_None);
    return Py_None;
err:
    PyErr_Format(PyExc_ValueError, "FCI error %d", erf.erfOper); /* XXX better error type */
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
}

static PyObject*
msiobj_close(msiobj* msidb, PyObject *args)
{
    MsiCloseHandle(msidb->h);
    msidb->h = 0;
    Py_INCREF(Py_None);
    return Py_None;
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
        default:
            PyErr_Format(MSIError, "unknown error %x", status);
            return NULL;
        }
    }

    code = MsiRecordGetInteger(err, 1); /* XXX code */
    if (MsiFormatRecord(0, err, res, &size) == ERROR_MORE_DATA) {
        res = malloc(size+1);
        MsiFormatRecord(0, err, res, &size);
        res[size]='\0';
    }
    MsiCloseHandle(err);
    PyErr_SetString(MSIError, res);
    if (res != buf)
        free(res);
    return NULL;
}

/*************************** Record objects **********************/

static PyObject*
record_getfieldcount(msiobj* record, PyObject* args)
{
    return PyInt_FromLong(MsiRecordGetFieldCount(record->h));
}

static PyObject*
record_getinteger(msiobj* record, PyObject* args)
{
    unsigned int field;
    int status;

    if (!PyArg_ParseTuple(args, "I:GetInteger", &field))
        return NULL;
    status = MsiRecordGetInteger(record->h, field);
    if (status == MSI_NULL_INTEGER){
        PyErr_SetString(MSIError, "could not convert record field to integer");
        return NULL;
    }
    return PyInt_FromLong((long) status);
}

static PyObject*
record_getstring(msiobj* record, PyObject* args)
{
    unsigned int field;
    unsigned int status;
    char buf[2000];
    char *res = buf;
    DWORD size = sizeof(buf);
    PyObject* string;

    if (!PyArg_ParseTuple(args, "I:GetString", &field))
        return NULL;
    status = MsiRecordGetString(record->h, field, res, &size);
    if (status == ERROR_MORE_DATA) {
        res = (char*) malloc(size + 1);
        if (res == NULL)
            return PyErr_NoMemory();
        status = MsiRecordGetString(record->h, field, res, &size);
    }
    if (status != ERROR_SUCCESS)
        return msierror((int) status);
    string = PyString_FromString(res);
    if (buf != res)
        free(res);
    return string;
}

static PyObject*
record_cleardata(msiobj* record, PyObject *args)
{
    int status = MsiRecordClearData(record->h);
    if (status != ERROR_SUCCESS)
        return msierror(status);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
record_setstring(msiobj* record, PyObject *args)
{
    int status;
    int field;
    char *data;

    if (!PyArg_ParseTuple(args, "is:SetString", &field, &data))
        return NULL;

    if ((status = MsiRecordSetString(record->h, field, data)) != ERROR_SUCCESS)
        return msierror(status);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
record_setstream(msiobj* record, PyObject *args)
{
    int status;
    int field;
    char *data;

    if (!PyArg_ParseTuple(args, "is:SetStream", &field, &data))
        return NULL;

    if ((status = MsiRecordSetStream(record->h, field, data)) != ERROR_SUCCESS)
        return msierror(status);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
record_setinteger(msiobj* record, PyObject *args)
{
    int status;
    int field;
    int data;

    if (!PyArg_ParseTuple(args, "ii:SetInteger", &field, &data))
        return NULL;

    if ((status = MsiRecordSetInteger(record->h, field, data)) != ERROR_SUCCESS)
        return msierror(status);

    Py_INCREF(Py_None);
    return Py_None;
}



static PyMethodDef record_methods[] = {
    { "GetFieldCount", (PyCFunction)record_getfieldcount, METH_NOARGS,
        PyDoc_STR("GetFieldCount() -> int\nWraps MsiRecordGetFieldCount")},
    { "GetInteger", (PyCFunction)record_getinteger, METH_VARARGS,
    PyDoc_STR("GetInteger(field) -> int\nWraps MsiRecordGetInteger")},
    { "GetString", (PyCFunction)record_getstring, METH_VARARGS,
    PyDoc_STR("GetString(field) -> string\nWraps MsiRecordGetString")},
    { "SetString", (PyCFunction)record_setstring, METH_VARARGS,
        PyDoc_STR("SetString(field,str) -> None\nWraps MsiRecordSetString")},
    { "SetStream", (PyCFunction)record_setstream, METH_VARARGS,
        PyDoc_STR("SetStream(field,filename) -> None\nWraps MsiRecordSetInteger")},
    { "SetInteger", (PyCFunction)record_setinteger, METH_VARARGS,
        PyDoc_STR("SetInteger(field,int) -> None\nWraps MsiRecordSetInteger")},
    { "ClearData", (PyCFunction)record_cleardata, METH_NOARGS,
        PyDoc_STR("ClearData() -> int\nWraps MsiRecordGClearData")},
    { NULL, NULL }
};

static PyTypeObject record_Type = {
        PyVarObject_HEAD_INIT(NULL, 0)
        "_msi.Record",          /*tp_name*/
        sizeof(msiobj), /*tp_basicsize*/
        0,                      /*tp_itemsize*/
        /* methods */
        (destructor)msiobj_dealloc, /*tp_dealloc*/
        0,                      /*tp_print*/
        0,                      /*tp_getattr*/
        0,                      /*tp_setattr*/
        0,                      /*tp_compare*/
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
    msiobj *result = PyObject_NEW(struct msiobj, &record_Type);

    if (!result) {
        MsiCloseHandle(h);
        return NULL;
    }

    result->h = h;
    return (PyObject*)result;
}

/*************************** SummaryInformation objects **************/

static PyObject*
summary_getproperty(msiobj* si, PyObject *args)
{
    int status;
    int field;
    PyObject *result;
    UINT type;
    INT ival;
    FILETIME fval;
    char sbuf[1000];
    char *sval = sbuf;
    DWORD ssize = sizeof(sval);

    if (!PyArg_ParseTuple(args, "i:GetProperty", &field))
        return NULL;

    status = MsiSummaryInfoGetProperty(si->h, field, &type, &ival,
        &fval, sval, &ssize);
    if (status == ERROR_MORE_DATA) {
        sval = malloc(ssize);
        status = MsiSummaryInfoGetProperty(si->h, field, &type, &ival,
            &fval, sval, &ssize);
    }

    switch(type) {
        case VT_I2: case VT_I4:
            return PyInt_FromLong(ival);
        case VT_FILETIME:
            PyErr_SetString(PyExc_NotImplementedError, "FILETIME result");
            return NULL;
        case VT_LPSTR:
            result = PyString_FromStringAndSize(sval, ssize);
            if (sval != sbuf)
                free(sval);
            return result;
    }
    PyErr_Format(PyExc_NotImplementedError, "result of type %d", type);
    return NULL;
}

static PyObject*
summary_getpropertycount(msiobj* si, PyObject *args)
{
    int status;
    UINT result;

    status = MsiSummaryInfoGetPropertyCount(si->h, &result);
    if (status != ERROR_SUCCESS)
        return msierror(status);

    return PyInt_FromLong(result);
}

static PyObject*
summary_setproperty(msiobj* si, PyObject *args)
{
    int status;
    int field;
    PyObject* data;

    if (!PyArg_ParseTuple(args, "iO:SetProperty", &field, &data))
        return NULL;

    if (PyString_Check(data)) {
        status = MsiSummaryInfoSetProperty(si->h, field, VT_LPSTR,
            0, NULL, PyString_AsString(data));
    } else if (PyInt_Check(data)) {
        status = MsiSummaryInfoSetProperty(si->h, field, VT_I4,
            PyInt_AsLong(data), NULL, NULL);
    } else {
        PyErr_SetString(PyExc_TypeError, "unsupported type");
        return NULL;
    }

    if (status != ERROR_SUCCESS)
        return msierror(status);

    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject*
summary_persist(msiobj* si, PyObject *args)
{
    int status;

    status = MsiSummaryInfoPersist(si->h);
    if (status != ERROR_SUCCESS)
        return msierror(status);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef summary_methods[] = {
    { "GetProperty", (PyCFunction)summary_getproperty, METH_VARARGS,
        PyDoc_STR("GetProperty(propid) -> value\nWraps MsiSummaryInfoGetProperty")},
    { "GetPropertyCount", (PyCFunction)summary_getpropertycount, METH_NOARGS,
        PyDoc_STR("GetProperty() -> int\nWraps MsiSummaryInfoGetPropertyCount")},
    { "SetProperty", (PyCFunction)summary_setproperty, METH_VARARGS,
        PyDoc_STR("SetProperty(value) -> None\nWraps MsiSummaryInfoProperty")},
    { "Persist", (PyCFunction)summary_persist, METH_NOARGS,
        PyDoc_STR("Persist() -> None\nWraps MsiSummaryInfoPersist")},
    { NULL, NULL }
};

static PyTypeObject summary_Type = {
        PyVarObject_HEAD_INIT(NULL, 0)
        "_msi.SummaryInformation",              /*tp_name*/
        sizeof(msiobj), /*tp_basicsize*/
        0,                      /*tp_itemsize*/
        /* methods */
        (destructor)msiobj_dealloc, /*tp_dealloc*/
        0,                      /*tp_print*/
        0,                      /*tp_getattr*/
        0,                      /*tp_setattr*/
        0,                      /*tp_compare*/
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

static PyObject*
view_execute(msiobj *view, PyObject*args)
{
    int status;
    MSIHANDLE params = 0;
    PyObject *oparams = Py_None;

    if (!PyArg_ParseTuple(args, "O:Execute", &oparams))
        return NULL;

    if (oparams != Py_None) {
        if (oparams->ob_type != &record_Type) {
            PyErr_SetString(PyExc_TypeError, "Execute argument must be a record");
            return NULL;
        }
        params = ((msiobj*)oparams)->h;
    }

    status = MsiViewExecute(view->h, params);
    if (status != ERROR_SUCCESS)
        return msierror(status);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
view_fetch(msiobj *view, PyObject*args)
{
    int status;
    MSIHANDLE result;

    if ((status = MsiViewFetch(view->h, &result)) != ERROR_SUCCESS)
        return msierror(status);

    return record_new(result);
}

static PyObject*
view_getcolumninfo(msiobj *view, PyObject *args)
{
    int status;
    int kind;
    MSIHANDLE result;

    if (!PyArg_ParseTuple(args, "i:GetColumnInfo", &kind))
        return NULL;

    if ((status = MsiViewGetColumnInfo(view->h, kind, &result)) != ERROR_SUCCESS)
        return msierror(status);

    return record_new(result);
}

static PyObject*
view_modify(msiobj *view, PyObject *args)
{
    int kind;
    PyObject *data;
    int status;

    if (!PyArg_ParseTuple(args, "iO:Modify", &kind, &data))
        return NULL;

    if (data->ob_type != &record_Type) {
        PyErr_SetString(PyExc_TypeError, "Modify expects a record object");
        return NULL;
    }

    if ((status = MsiViewModify(view->h, kind, ((msiobj*)data)->h)) != ERROR_SUCCESS)
        return msierror(status);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
view_close(msiobj *view, PyObject*args)
{
    int status;

    if ((status = MsiViewClose(view->h)) != ERROR_SUCCESS)
        return msierror(status);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef view_methods[] = {
    { "Execute", (PyCFunction)view_execute, METH_VARARGS,
        PyDoc_STR("Execute(params=None) -> None\nWraps MsiViewExecute")},
    { "GetColumnInfo", (PyCFunction)view_getcolumninfo, METH_VARARGS,
        PyDoc_STR("GetColumnInfo() -> result\nWraps MsiGetColumnInfo")},
    { "Fetch", (PyCFunction)view_fetch, METH_NOARGS,
        PyDoc_STR("Fetch() -> result\nWraps MsiViewFetch")},
    { "Modify", (PyCFunction)view_modify, METH_VARARGS,
        PyDoc_STR("Modify(mode,record) -> None\nWraps MsiViewModify")},
    { "Close", (PyCFunction)view_close, METH_NOARGS,
        PyDoc_STR("Close() -> result\nWraps MsiViewClose")},
    { NULL, NULL }
};

static PyTypeObject msiview_Type = {
        PyVarObject_HEAD_INIT(NULL, 0)
        "_msi.View",            /*tp_name*/
        sizeof(msiobj), /*tp_basicsize*/
        0,                      /*tp_itemsize*/
        /* methods */
        (destructor)msiobj_dealloc, /*tp_dealloc*/
        0,                      /*tp_print*/
        0,                      /*tp_getattr*/
        0,                      /*tp_setattr*/
        0,                      /*tp_compare*/
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

static PyObject*
msidb_openview(msiobj *msidb, PyObject *args)
{
    int status;
    char *sql;
    MSIHANDLE hView;
    msiobj *result;

    if (!PyArg_ParseTuple(args, "s:OpenView", &sql))
        return NULL;

    if ((status = MsiDatabaseOpenView(msidb->h, sql, &hView)) != ERROR_SUCCESS)
        return msierror(status);

    result = PyObject_NEW(struct msiobj, &msiview_Type);
    if (!result) {
        MsiCloseHandle(hView);
        return NULL;
    }

    result->h = hView;
    return (PyObject*)result;
}

static PyObject*
msidb_commit(msiobj *msidb, PyObject *args)
{
    int status;

    if ((status = MsiDatabaseCommit(msidb->h)) != ERROR_SUCCESS)
        return msierror(status);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
msidb_getsummaryinformation(msiobj *db, PyObject *args)
{
    int status;
    int count;
    MSIHANDLE result;
    msiobj *oresult;

    if (!PyArg_ParseTuple(args, "i:GetSummaryInformation", &count))
        return NULL;

    status = MsiGetSummaryInformation(db->h, NULL, count, &result);
    if (status != ERROR_SUCCESS)
        return msierror(status);

    oresult = PyObject_NEW(struct msiobj, &summary_Type);
    if (!result) {
        MsiCloseHandle(result);
        return NULL;
    }

    oresult->h = result;
    return (PyObject*)oresult;
}

static PyMethodDef db_methods[] = {
    { "OpenView", (PyCFunction)msidb_openview, METH_VARARGS,
        PyDoc_STR("OpenView(sql) -> viewobj\nWraps MsiDatabaseOpenView")},
    { "Commit", (PyCFunction)msidb_commit, METH_NOARGS,
        PyDoc_STR("Commit() -> None\nWraps MsiDatabaseCommit")},
    { "GetSummaryInformation", (PyCFunction)msidb_getsummaryinformation, METH_VARARGS,
        PyDoc_STR("GetSummaryInformation(updateCount) -> viewobj\nWraps MsiGetSummaryInformation")},
    { NULL, NULL }
};

static PyTypeObject msidb_Type = {
        PyVarObject_HEAD_INIT(NULL, 0)
        "_msi.Database",                /*tp_name*/
        sizeof(msiobj), /*tp_basicsize*/
        0,                      /*tp_itemsize*/
        /* methods */
        (destructor)msiobj_dealloc, /*tp_dealloc*/
        0,                      /*tp_print*/
        0,                      /*tp_getattr*/
        0,                      /*tp_setattr*/
        0,                      /*tp_compare*/
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

static PyObject* msiopendb(PyObject *obj, PyObject *args)
{
    int status;
    char *path;
    int persist;
    MSIHANDLE h;
    msiobj *result;

    if (!PyArg_ParseTuple(args, "si:MSIOpenDatabase", &path, &persist))
        return NULL;

        status = MsiOpenDatabase(path, (LPCSTR)persist, &h);
    if (status != ERROR_SUCCESS)
        return msierror(status);

    result = PyObject_NEW(struct msiobj, &msidb_Type);
    if (!result) {
        MsiCloseHandle(h);
        return NULL;
    }
    result->h = h;
    return (PyObject*)result;
}

static PyObject*
createrecord(PyObject *o, PyObject *args)
{
    int count;
    MSIHANDLE h;

    if (!PyArg_ParseTuple(args, "i:CreateRecord", &count))
        return NULL;

    h = MsiCreateRecord(count);
    if (h == 0)
        return msierror(0);

    return record_new(h);
}


static PyMethodDef msi_methods[] = {
        {"UuidCreate", (PyCFunction)uuidcreate, METH_NOARGS,
                PyDoc_STR("UuidCreate() -> string")},
        {"FCICreate",   (PyCFunction)fcicreate, METH_VARARGS,
                PyDoc_STR("fcicreate(cabname,files) -> None")},
        {"OpenDatabase", (PyCFunction)msiopendb, METH_VARARGS,
        PyDoc_STR("OpenDatabase(name, flags) -> dbobj\nWraps MsiOpenDatabase")},
        {"CreateRecord", (PyCFunction)createrecord, METH_VARARGS,
        PyDoc_STR("OpenDatabase(name, flags) -> dbobj\nWraps MsiCreateRecord")},
        {NULL,          NULL}           /* sentinel */
};

static char msi_doc[] = "Documentation";

PyMODINIT_FUNC
init_msi(void)
{
    PyObject *m;

    m = Py_InitModule3("_msi", msi_methods, msi_doc);
    if (m == NULL)
        return;

    PyModule_AddIntConstant(m, "MSIDBOPEN_CREATEDIRECT", (int)MSIDBOPEN_CREATEDIRECT);
    PyModule_AddIntConstant(m, "MSIDBOPEN_CREATE", (int)MSIDBOPEN_CREATE);
    PyModule_AddIntConstant(m, "MSIDBOPEN_DIRECT", (int)MSIDBOPEN_DIRECT);
    PyModule_AddIntConstant(m, "MSIDBOPEN_READONLY", (int)MSIDBOPEN_READONLY);
    PyModule_AddIntConstant(m, "MSIDBOPEN_TRANSACT", (int)MSIDBOPEN_TRANSACT);
    PyModule_AddIntConstant(m, "MSIDBOPEN_PATCHFILE", (int)MSIDBOPEN_PATCHFILE);

    PyModule_AddIntConstant(m, "MSICOLINFO_NAMES", MSICOLINFO_NAMES);
    PyModule_AddIntConstant(m, "MSICOLINFO_TYPES", MSICOLINFO_TYPES);

    PyModule_AddIntConstant(m, "MSIMODIFY_SEEK", MSIMODIFY_SEEK);
    PyModule_AddIntConstant(m, "MSIMODIFY_REFRESH", MSIMODIFY_REFRESH);
    PyModule_AddIntConstant(m, "MSIMODIFY_INSERT", MSIMODIFY_INSERT);
    PyModule_AddIntConstant(m, "MSIMODIFY_UPDATE", MSIMODIFY_UPDATE);
    PyModule_AddIntConstant(m, "MSIMODIFY_ASSIGN", MSIMODIFY_ASSIGN);
    PyModule_AddIntConstant(m, "MSIMODIFY_REPLACE", MSIMODIFY_REPLACE);
    PyModule_AddIntConstant(m, "MSIMODIFY_MERGE", MSIMODIFY_MERGE);
    PyModule_AddIntConstant(m, "MSIMODIFY_DELETE", MSIMODIFY_DELETE);
    PyModule_AddIntConstant(m, "MSIMODIFY_INSERT_TEMPORARY", MSIMODIFY_INSERT_TEMPORARY);
    PyModule_AddIntConstant(m, "MSIMODIFY_VALIDATE", MSIMODIFY_VALIDATE);
    PyModule_AddIntConstant(m, "MSIMODIFY_VALIDATE_NEW", MSIMODIFY_VALIDATE_NEW);
    PyModule_AddIntConstant(m, "MSIMODIFY_VALIDATE_FIELD", MSIMODIFY_VALIDATE_FIELD);
    PyModule_AddIntConstant(m, "MSIMODIFY_VALIDATE_DELETE", MSIMODIFY_VALIDATE_DELETE);

    PyModule_AddIntConstant(m, "PID_CODEPAGE", PID_CODEPAGE);
    PyModule_AddIntConstant(m, "PID_TITLE", PID_TITLE);
    PyModule_AddIntConstant(m, "PID_SUBJECT", PID_SUBJECT);
    PyModule_AddIntConstant(m, "PID_AUTHOR", PID_AUTHOR);
    PyModule_AddIntConstant(m, "PID_KEYWORDS", PID_KEYWORDS);
    PyModule_AddIntConstant(m, "PID_COMMENTS", PID_COMMENTS);
    PyModule_AddIntConstant(m, "PID_TEMPLATE", PID_TEMPLATE);
    PyModule_AddIntConstant(m, "PID_LASTAUTHOR", PID_LASTAUTHOR);
    PyModule_AddIntConstant(m, "PID_REVNUMBER", PID_REVNUMBER);
    PyModule_AddIntConstant(m, "PID_LASTPRINTED", PID_LASTPRINTED);
    PyModule_AddIntConstant(m, "PID_CREATE_DTM", PID_CREATE_DTM);
    PyModule_AddIntConstant(m, "PID_LASTSAVE_DTM", PID_LASTSAVE_DTM);
    PyModule_AddIntConstant(m, "PID_PAGECOUNT", PID_PAGECOUNT);
    PyModule_AddIntConstant(m, "PID_WORDCOUNT", PID_WORDCOUNT);
    PyModule_AddIntConstant(m, "PID_CHARCOUNT", PID_CHARCOUNT);
    PyModule_AddIntConstant(m, "PID_APPNAME", PID_APPNAME);
    PyModule_AddIntConstant(m, "PID_SECURITY", PID_SECURITY);

    MSIError = PyErr_NewException ("_msi.MSIError", NULL, NULL);
    if (!MSIError)
        return;
    PyModule_AddObject(m, "MSIError", MSIError);
}
