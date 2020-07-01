/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_msi_UuidCreate__doc__,
"UuidCreate($module, /)\n"
"--\n"
"\n"
"Return the string representation of a new unique identifier.");

#define _MSI_UUIDCREATE_METHODDEF    \
    {"UuidCreate", (PyCFunction)_msi_UuidCreate, METH_NOARGS, _msi_UuidCreate__doc__},

static PyObject *
_msi_UuidCreate_impl(PyObject *module);

static PyObject *
_msi_UuidCreate(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _msi_UuidCreate_impl(module);
}

PyDoc_STRVAR(_msi_FCICreate__doc__,
"FCICreate($module, cabname, files, /)\n"
"--\n"
"\n"
"Create a new CAB file.\n"
"\n"
"  cabname\n"
"    the name of the CAB file\n"
"  files\n"
"    a list of tuples, each containing the name of the file on disk,\n"
"    and the name of the file inside the CAB file");

#define _MSI_FCICREATE_METHODDEF    \
    {"FCICreate", (PyCFunction)(void(*)(void))_msi_FCICreate, METH_FASTCALL, _msi_FCICreate__doc__},

static PyObject *
_msi_FCICreate_impl(PyObject *module, const char *cabname, PyObject *files);

static PyObject *
_msi_FCICreate(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const char *cabname;
    PyObject *files;

    if (!_PyArg_CheckPositional("FCICreate", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("FCICreate", "argument 1", "str", args[0]);
        goto exit;
    }
    Py_ssize_t cabname_length;
    cabname = PyUnicode_AsUTF8AndSize(args[0], &cabname_length);
    if (cabname == NULL) {
        goto exit;
    }
    if (strlen(cabname) != (size_t)cabname_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    files = args[1];
    return_value = _msi_FCICreate_impl(module, cabname, files);

exit:
    return return_value;
}

PyDoc_STRVAR(_msi_Database_Close__doc__,
"Close($self, /)\n"
"--\n"
"\n"
"Close the database object.");

#define _MSI_DATABASE_CLOSE_METHODDEF    \
    {"Close", (PyCFunction)_msi_Database_Close, METH_NOARGS, _msi_Database_Close__doc__},

static PyObject *
_msi_Database_Close_impl(msiobj *self);

static PyObject *
_msi_Database_Close(msiobj *self, PyObject *Py_UNUSED(ignored))
{
    return _msi_Database_Close_impl(self);
}

PyDoc_STRVAR(_msi_Record_GetFieldCount__doc__,
"GetFieldCount($self, /)\n"
"--\n"
"\n"
"Return the number of fields of the record.");

#define _MSI_RECORD_GETFIELDCOUNT_METHODDEF    \
    {"GetFieldCount", (PyCFunction)_msi_Record_GetFieldCount, METH_NOARGS, _msi_Record_GetFieldCount__doc__},

static PyObject *
_msi_Record_GetFieldCount_impl(msiobj *self);

static PyObject *
_msi_Record_GetFieldCount(msiobj *self, PyObject *Py_UNUSED(ignored))
{
    return _msi_Record_GetFieldCount_impl(self);
}

PyDoc_STRVAR(_msi_Record_GetInteger__doc__,
"GetInteger($self, field, /)\n"
"--\n"
"\n"
"Return the value of field as an integer where possible.");

#define _MSI_RECORD_GETINTEGER_METHODDEF    \
    {"GetInteger", (PyCFunction)_msi_Record_GetInteger, METH_O, _msi_Record_GetInteger__doc__},

static PyObject *
_msi_Record_GetInteger_impl(msiobj *self, unsigned int field);

static PyObject *
_msi_Record_GetInteger(msiobj *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    unsigned int field;

    field = (unsigned int)PyLong_AsUnsignedLongMask(arg);
    if (field == (unsigned int)-1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _msi_Record_GetInteger_impl(self, field);

exit:
    return return_value;
}

PyDoc_STRVAR(_msi_Record_GetString__doc__,
"GetString($self, field, /)\n"
"--\n"
"\n"
"Return the value of field as a string where possible.");

#define _MSI_RECORD_GETSTRING_METHODDEF    \
    {"GetString", (PyCFunction)_msi_Record_GetString, METH_O, _msi_Record_GetString__doc__},

static PyObject *
_msi_Record_GetString_impl(msiobj *self, unsigned int field);

static PyObject *
_msi_Record_GetString(msiobj *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    unsigned int field;

    field = (unsigned int)PyLong_AsUnsignedLongMask(arg);
    if (field == (unsigned int)-1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _msi_Record_GetString_impl(self, field);

exit:
    return return_value;
}

PyDoc_STRVAR(_msi_Record_ClearData__doc__,
"ClearData($self, /)\n"
"--\n"
"\n"
"Set all fields of the record to 0.");

#define _MSI_RECORD_CLEARDATA_METHODDEF    \
    {"ClearData", (PyCFunction)_msi_Record_ClearData, METH_NOARGS, _msi_Record_ClearData__doc__},

static PyObject *
_msi_Record_ClearData_impl(msiobj *self);

static PyObject *
_msi_Record_ClearData(msiobj *self, PyObject *Py_UNUSED(ignored))
{
    return _msi_Record_ClearData_impl(self);
}

PyDoc_STRVAR(_msi_Record_SetString__doc__,
"SetString($self, field, value, /)\n"
"--\n"
"\n"
"Set field to a string value.");

#define _MSI_RECORD_SETSTRING_METHODDEF    \
    {"SetString", (PyCFunction)(void(*)(void))_msi_Record_SetString, METH_FASTCALL, _msi_Record_SetString__doc__},

static PyObject *
_msi_Record_SetString_impl(msiobj *self, int field, const Py_UNICODE *value);

static PyObject *
_msi_Record_SetString(msiobj *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int field;
    const Py_UNICODE *value;

    if (!_PyArg_CheckPositional("SetString", nargs, 2, 2)) {
        goto exit;
    }
    field = _PyLong_AsInt(args[0]);
    if (field == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("SetString", "argument 2", "str", args[1]);
        goto exit;
    }
    #if USE_UNICODE_WCHAR_CACHE
    _Py_COMP_DIAG_PUSH
    _Py_COMP_DIAG_IGNORE_DEPR_DECLS
    value = _PyUnicode_AsUnicode(args[1]);
    _Py_COMP_DIAG_POP
    #else /* USE_UNICODE_WCHAR_CACHE */
    value = PyUnicode_AsWideCharString(args[1], NULL);
    #endif /* USE_UNICODE_WCHAR_CACHE */
    if (value == NULL) {
        goto exit;
    }
    return_value = _msi_Record_SetString_impl(self, field, value);

exit:
    /* Cleanup for value */
    #if !USE_UNICODE_WCHAR_CACHE
    PyMem_Free((void *)value);
    #endif /* USE_UNICODE_WCHAR_CACHE */

    return return_value;
}

PyDoc_STRVAR(_msi_Record_SetStream__doc__,
"SetStream($self, field, value, /)\n"
"--\n"
"\n"
"Set field to the contents of the file named value.");

#define _MSI_RECORD_SETSTREAM_METHODDEF    \
    {"SetStream", (PyCFunction)(void(*)(void))_msi_Record_SetStream, METH_FASTCALL, _msi_Record_SetStream__doc__},

static PyObject *
_msi_Record_SetStream_impl(msiobj *self, int field, const Py_UNICODE *value);

static PyObject *
_msi_Record_SetStream(msiobj *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int field;
    const Py_UNICODE *value;

    if (!_PyArg_CheckPositional("SetStream", nargs, 2, 2)) {
        goto exit;
    }
    field = _PyLong_AsInt(args[0]);
    if (field == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("SetStream", "argument 2", "str", args[1]);
        goto exit;
    }
    #if USE_UNICODE_WCHAR_CACHE
    _Py_COMP_DIAG_PUSH
    _Py_COMP_DIAG_IGNORE_DEPR_DECLS
    value = _PyUnicode_AsUnicode(args[1]);
    _Py_COMP_DIAG_POP
    #else /* USE_UNICODE_WCHAR_CACHE */
    value = PyUnicode_AsWideCharString(args[1], NULL);
    #endif /* USE_UNICODE_WCHAR_CACHE */
    if (value == NULL) {
        goto exit;
    }
    return_value = _msi_Record_SetStream_impl(self, field, value);

exit:
    /* Cleanup for value */
    #if !USE_UNICODE_WCHAR_CACHE
    PyMem_Free((void *)value);
    #endif /* USE_UNICODE_WCHAR_CACHE */

    return return_value;
}

PyDoc_STRVAR(_msi_Record_SetInteger__doc__,
"SetInteger($self, field, value, /)\n"
"--\n"
"\n"
"Set field to an integer value.");

#define _MSI_RECORD_SETINTEGER_METHODDEF    \
    {"SetInteger", (PyCFunction)(void(*)(void))_msi_Record_SetInteger, METH_FASTCALL, _msi_Record_SetInteger__doc__},

static PyObject *
_msi_Record_SetInteger_impl(msiobj *self, int field, int value);

static PyObject *
_msi_Record_SetInteger(msiobj *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int field;
    int value;

    if (!_PyArg_CheckPositional("SetInteger", nargs, 2, 2)) {
        goto exit;
    }
    field = _PyLong_AsInt(args[0]);
    if (field == -1 && PyErr_Occurred()) {
        goto exit;
    }
    value = _PyLong_AsInt(args[1]);
    if (value == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _msi_Record_SetInteger_impl(self, field, value);

exit:
    return return_value;
}

PyDoc_STRVAR(_msi_SummaryInformation_GetProperty__doc__,
"GetProperty($self, field, /)\n"
"--\n"
"\n"
"Return a property of the summary.\n"
"\n"
"  field\n"
"    the name of the property, one of the PID_* constants");

#define _MSI_SUMMARYINFORMATION_GETPROPERTY_METHODDEF    \
    {"GetProperty", (PyCFunction)_msi_SummaryInformation_GetProperty, METH_O, _msi_SummaryInformation_GetProperty__doc__},

static PyObject *
_msi_SummaryInformation_GetProperty_impl(msiobj *self, int field);

static PyObject *
_msi_SummaryInformation_GetProperty(msiobj *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int field;

    field = _PyLong_AsInt(arg);
    if (field == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _msi_SummaryInformation_GetProperty_impl(self, field);

exit:
    return return_value;
}

PyDoc_STRVAR(_msi_SummaryInformation_GetPropertyCount__doc__,
"GetPropertyCount($self, /)\n"
"--\n"
"\n"
"Return the number of summary properties.");

#define _MSI_SUMMARYINFORMATION_GETPROPERTYCOUNT_METHODDEF    \
    {"GetPropertyCount", (PyCFunction)_msi_SummaryInformation_GetPropertyCount, METH_NOARGS, _msi_SummaryInformation_GetPropertyCount__doc__},

static PyObject *
_msi_SummaryInformation_GetPropertyCount_impl(msiobj *self);

static PyObject *
_msi_SummaryInformation_GetPropertyCount(msiobj *self, PyObject *Py_UNUSED(ignored))
{
    return _msi_SummaryInformation_GetPropertyCount_impl(self);
}

PyDoc_STRVAR(_msi_SummaryInformation_SetProperty__doc__,
"SetProperty($self, field, value, /)\n"
"--\n"
"\n"
"Set a property.\n"
"\n"
"  field\n"
"    the name of the property, one of the PID_* constants\n"
"  value\n"
"    the new value of the property (integer or string)");

#define _MSI_SUMMARYINFORMATION_SETPROPERTY_METHODDEF    \
    {"SetProperty", (PyCFunction)(void(*)(void))_msi_SummaryInformation_SetProperty, METH_FASTCALL, _msi_SummaryInformation_SetProperty__doc__},

static PyObject *
_msi_SummaryInformation_SetProperty_impl(msiobj *self, int field,
                                         PyObject *data);

static PyObject *
_msi_SummaryInformation_SetProperty(msiobj *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int field;
    PyObject *data;

    if (!_PyArg_CheckPositional("SetProperty", nargs, 2, 2)) {
        goto exit;
    }
    field = _PyLong_AsInt(args[0]);
    if (field == -1 && PyErr_Occurred()) {
        goto exit;
    }
    data = args[1];
    return_value = _msi_SummaryInformation_SetProperty_impl(self, field, data);

exit:
    return return_value;
}

PyDoc_STRVAR(_msi_SummaryInformation_Persist__doc__,
"Persist($self, /)\n"
"--\n"
"\n"
"Write the modified properties to the summary information stream.");

#define _MSI_SUMMARYINFORMATION_PERSIST_METHODDEF    \
    {"Persist", (PyCFunction)_msi_SummaryInformation_Persist, METH_NOARGS, _msi_SummaryInformation_Persist__doc__},

static PyObject *
_msi_SummaryInformation_Persist_impl(msiobj *self);

static PyObject *
_msi_SummaryInformation_Persist(msiobj *self, PyObject *Py_UNUSED(ignored))
{
    return _msi_SummaryInformation_Persist_impl(self);
}

PyDoc_STRVAR(_msi_View_Execute__doc__,
"Execute($self, params, /)\n"
"--\n"
"\n"
"Execute the SQL query of the view.\n"
"\n"
"  params\n"
"    a record describing actual values of the parameter tokens\n"
"    in the query or None");

#define _MSI_VIEW_EXECUTE_METHODDEF    \
    {"Execute", (PyCFunction)_msi_View_Execute, METH_O, _msi_View_Execute__doc__},

PyDoc_STRVAR(_msi_View_Fetch__doc__,
"Fetch($self, /)\n"
"--\n"
"\n"
"Return a result record of the query.");

#define _MSI_VIEW_FETCH_METHODDEF    \
    {"Fetch", (PyCFunction)_msi_View_Fetch, METH_NOARGS, _msi_View_Fetch__doc__},

static PyObject *
_msi_View_Fetch_impl(msiobj *self);

static PyObject *
_msi_View_Fetch(msiobj *self, PyObject *Py_UNUSED(ignored))
{
    return _msi_View_Fetch_impl(self);
}

PyDoc_STRVAR(_msi_View_GetColumnInfo__doc__,
"GetColumnInfo($self, kind, /)\n"
"--\n"
"\n"
"Return a record describing the columns of the view.\n"
"\n"
"  kind\n"
"    MSICOLINFO_NAMES or MSICOLINFO_TYPES");

#define _MSI_VIEW_GETCOLUMNINFO_METHODDEF    \
    {"GetColumnInfo", (PyCFunction)_msi_View_GetColumnInfo, METH_O, _msi_View_GetColumnInfo__doc__},

static PyObject *
_msi_View_GetColumnInfo_impl(msiobj *self, int kind);

static PyObject *
_msi_View_GetColumnInfo(msiobj *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int kind;

    kind = _PyLong_AsInt(arg);
    if (kind == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _msi_View_GetColumnInfo_impl(self, kind);

exit:
    return return_value;
}

PyDoc_STRVAR(_msi_View_Modify__doc__,
"Modify($self, kind, data, /)\n"
"--\n"
"\n"
"Modify the view.\n"
"\n"
"  kind\n"
"    one of the MSIMODIFY_* constants\n"
"  data\n"
"    a record describing the new data");

#define _MSI_VIEW_MODIFY_METHODDEF    \
    {"Modify", (PyCFunction)(void(*)(void))_msi_View_Modify, METH_FASTCALL, _msi_View_Modify__doc__},

static PyObject *
_msi_View_Modify_impl(msiobj *self, int kind, PyObject *data);

static PyObject *
_msi_View_Modify(msiobj *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int kind;
    PyObject *data;

    if (!_PyArg_CheckPositional("Modify", nargs, 2, 2)) {
        goto exit;
    }
    kind = _PyLong_AsInt(args[0]);
    if (kind == -1 && PyErr_Occurred()) {
        goto exit;
    }
    data = args[1];
    return_value = _msi_View_Modify_impl(self, kind, data);

exit:
    return return_value;
}

PyDoc_STRVAR(_msi_View_Close__doc__,
"Close($self, /)\n"
"--\n"
"\n"
"Close the view.");

#define _MSI_VIEW_CLOSE_METHODDEF    \
    {"Close", (PyCFunction)_msi_View_Close, METH_NOARGS, _msi_View_Close__doc__},

static PyObject *
_msi_View_Close_impl(msiobj *self);

static PyObject *
_msi_View_Close(msiobj *self, PyObject *Py_UNUSED(ignored))
{
    return _msi_View_Close_impl(self);
}

PyDoc_STRVAR(_msi_Database_OpenView__doc__,
"OpenView($self, sql, /)\n"
"--\n"
"\n"
"Return a view object.\n"
"\n"
"  sql\n"
"    the SQL statement to execute");

#define _MSI_DATABASE_OPENVIEW_METHODDEF    \
    {"OpenView", (PyCFunction)_msi_Database_OpenView, METH_O, _msi_Database_OpenView__doc__},

static PyObject *
_msi_Database_OpenView_impl(msiobj *self, const Py_UNICODE *sql);

static PyObject *
_msi_Database_OpenView(msiobj *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const Py_UNICODE *sql;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("OpenView", "argument", "str", arg);
        goto exit;
    }
    #if USE_UNICODE_WCHAR_CACHE
    _Py_COMP_DIAG_PUSH
    _Py_COMP_DIAG_IGNORE_DEPR_DECLS
    sql = _PyUnicode_AsUnicode(arg);
    _Py_COMP_DIAG_POP
    #else /* USE_UNICODE_WCHAR_CACHE */
    sql = PyUnicode_AsWideCharString(arg, NULL);
    #endif /* USE_UNICODE_WCHAR_CACHE */
    if (sql == NULL) {
        goto exit;
    }
    return_value = _msi_Database_OpenView_impl(self, sql);

exit:
    /* Cleanup for sql */
    #if !USE_UNICODE_WCHAR_CACHE
    PyMem_Free((void *)sql);
    #endif /* USE_UNICODE_WCHAR_CACHE */

    return return_value;
}

PyDoc_STRVAR(_msi_Database_Commit__doc__,
"Commit($self, /)\n"
"--\n"
"\n"
"Commit the changes pending in the current transaction.");

#define _MSI_DATABASE_COMMIT_METHODDEF    \
    {"Commit", (PyCFunction)_msi_Database_Commit, METH_NOARGS, _msi_Database_Commit__doc__},

static PyObject *
_msi_Database_Commit_impl(msiobj *self);

static PyObject *
_msi_Database_Commit(msiobj *self, PyObject *Py_UNUSED(ignored))
{
    return _msi_Database_Commit_impl(self);
}

PyDoc_STRVAR(_msi_Database_GetSummaryInformation__doc__,
"GetSummaryInformation($self, count, /)\n"
"--\n"
"\n"
"Return a new summary information object.\n"
"\n"
"  count\n"
"    the maximum number of updated values");

#define _MSI_DATABASE_GETSUMMARYINFORMATION_METHODDEF    \
    {"GetSummaryInformation", (PyCFunction)_msi_Database_GetSummaryInformation, METH_O, _msi_Database_GetSummaryInformation__doc__},

static PyObject *
_msi_Database_GetSummaryInformation_impl(msiobj *self, int count);

static PyObject *
_msi_Database_GetSummaryInformation(msiobj *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int count;

    count = _PyLong_AsInt(arg);
    if (count == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _msi_Database_GetSummaryInformation_impl(self, count);

exit:
    return return_value;
}

PyDoc_STRVAR(_msi_OpenDatabase__doc__,
"OpenDatabase($module, path, persist, /)\n"
"--\n"
"\n"
"Return a new database object.\n"
"\n"
"  path\n"
"    the file name of the MSI file\n"
"  persist\n"
"    the persistence mode");

#define _MSI_OPENDATABASE_METHODDEF    \
    {"OpenDatabase", (PyCFunction)(void(*)(void))_msi_OpenDatabase, METH_FASTCALL, _msi_OpenDatabase__doc__},

static PyObject *
_msi_OpenDatabase_impl(PyObject *module, const Py_UNICODE *path, int persist);

static PyObject *
_msi_OpenDatabase(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const Py_UNICODE *path;
    int persist;

    if (!_PyArg_CheckPositional("OpenDatabase", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("OpenDatabase", "argument 1", "str", args[0]);
        goto exit;
    }
    #if USE_UNICODE_WCHAR_CACHE
    _Py_COMP_DIAG_PUSH
    _Py_COMP_DIAG_IGNORE_DEPR_DECLS
    path = _PyUnicode_AsUnicode(args[0]);
    _Py_COMP_DIAG_POP
    #else /* USE_UNICODE_WCHAR_CACHE */
    path = PyUnicode_AsWideCharString(args[0], NULL);
    #endif /* USE_UNICODE_WCHAR_CACHE */
    if (path == NULL) {
        goto exit;
    }
    persist = _PyLong_AsInt(args[1]);
    if (persist == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _msi_OpenDatabase_impl(module, path, persist);

exit:
    /* Cleanup for path */
    #if !USE_UNICODE_WCHAR_CACHE
    PyMem_Free((void *)path);
    #endif /* USE_UNICODE_WCHAR_CACHE */

    return return_value;
}

PyDoc_STRVAR(_msi_CreateRecord__doc__,
"CreateRecord($module, count, /)\n"
"--\n"
"\n"
"Return a new record object.\n"
"\n"
"  count\n"
"    the number of fields of the record");

#define _MSI_CREATERECORD_METHODDEF    \
    {"CreateRecord", (PyCFunction)_msi_CreateRecord, METH_O, _msi_CreateRecord__doc__},

static PyObject *
_msi_CreateRecord_impl(PyObject *module, int count);

static PyObject *
_msi_CreateRecord(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int count;

    count = _PyLong_AsInt(arg);
    if (count == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _msi_CreateRecord_impl(module, count);

exit:
    return return_value;
}
/*[clinic end generated code: output=39807788326ad0e9 input=a9049054013a1b77]*/
