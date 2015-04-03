
/* UNIX group file access module */

#include "Python.h"
#include "posixmodule.h"

#include <grp.h>

#include "clinic/grpmodule.c.h"
/*[clinic input]
module grp
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=cade63f2ed1bd9f8]*/

static PyStructSequence_Field struct_group_type_fields[] = {
   {"gr_name", "group name"},
   {"gr_passwd", "password"},
   {"gr_gid", "group id"},
   {"gr_mem", "group members"},
   {0}
};

PyDoc_STRVAR(struct_group__doc__,
"grp.struct_group: Results from getgr*() routines.\n\n\
This object may be accessed either as a tuple of\n\
  (gr_name,gr_passwd,gr_gid,gr_mem)\n\
or via the object attributes as named in the above tuple.\n");

static PyStructSequence_Desc struct_group_type_desc = {
   "grp.struct_group",
   struct_group__doc__,
   struct_group_type_fields,
   4,
};


static int initialized;
static PyTypeObject StructGrpType;

static PyObject *
mkgrent(struct group *p)
{
    int setIndex = 0;
    PyObject *v = PyStructSequence_New(&StructGrpType), *w;
    char **member;

    if (v == NULL)
        return NULL;

    if ((w = PyList_New(0)) == NULL) {
        Py_DECREF(v);
        return NULL;
    }
    for (member = p->gr_mem; *member != NULL; member++) {
        PyObject *x = PyUnicode_DecodeFSDefault(*member);
        if (x == NULL || PyList_Append(w, x) != 0) {
            Py_XDECREF(x);
            Py_DECREF(w);
            Py_DECREF(v);
            return NULL;
        }
        Py_DECREF(x);
    }

#define SET(i,val) PyStructSequence_SET_ITEM(v, i, val)
    SET(setIndex++, PyUnicode_DecodeFSDefault(p->gr_name));
    if (p->gr_passwd)
            SET(setIndex++, PyUnicode_DecodeFSDefault(p->gr_passwd));
    else {
            SET(setIndex++, Py_None);
            Py_INCREF(Py_None);
    }
    SET(setIndex++, _PyLong_FromGid(p->gr_gid));
    SET(setIndex++, w);
#undef SET

    if (PyErr_Occurred()) {
        Py_DECREF(v);
        return NULL;
    }

    return v;
}

/*[clinic input]
grp.getgrgid

    id: object

Return the group database entry for the given numeric group ID.

If id is not valid, raise KeyError.
[clinic start generated code]*/

static PyObject *
grp_getgrgid_impl(PyModuleDef *module, PyObject *id)
/*[clinic end generated code: output=8a11f5fdeb8c78a0 input=15fa0e2ccf5cda25]*/
{
    PyObject *py_int_id;
    gid_t gid;
    struct group *p;

    py_int_id = PyNumber_Long(id);
    if (!py_int_id)
            return NULL;
    if (!_Py_Gid_Converter(py_int_id, &gid)) {
        Py_DECREF(py_int_id);
        return NULL;
    }
    Py_DECREF(py_int_id);

    if ((p = getgrgid(gid)) == NULL) {
        PyObject *gid_obj = _PyLong_FromGid(gid);
        if (gid_obj == NULL)
            return NULL;
        PyErr_Format(PyExc_KeyError, "getgrgid(): gid not found: %S", gid_obj);
        Py_DECREF(gid_obj);
        return NULL;
    }
    return mkgrent(p);
}

/*[clinic input]
grp.getgrnam

    name: unicode

Return the group database entry for the given group name.

If name is not valid, raise KeyError.
[clinic start generated code]*/

static PyObject *
grp_getgrnam_impl(PyModuleDef *module, PyObject *name)
/*[clinic end generated code: output=cd47511f4854da8e input=08ded29affa3c863]*/
{
    char *name_chars;
    struct group *p;
    PyObject *bytes, *retval = NULL;

    if ((bytes = PyUnicode_EncodeFSDefault(name)) == NULL)
        return NULL;
    if (PyBytes_AsStringAndSize(bytes, &name_chars, NULL) == -1)
        goto out;

    if ((p = getgrnam(name_chars)) == NULL) {
        PyErr_Format(PyExc_KeyError, "getgrnam(): name not found: %s", name_chars);
        goto out;
    }
    retval = mkgrent(p);
out:
    Py_DECREF(bytes);
    return retval;
}

/*[clinic input]
grp.getgrall

Return a list of all available group entries, in arbitrary order.

An entry whose name starts with '+' or '-' represents an instruction
to use YP/NIS and may not be accessible via getgrnam or getgrgid.
[clinic start generated code]*/

static PyObject *
grp_getgrall_impl(PyModuleDef *module)
/*[clinic end generated code: output=add9037a20c202de input=d7df76c825c367df]*/
{
    PyObject *d;
    struct group *p;

    if ((d = PyList_New(0)) == NULL)
        return NULL;
    setgrent();
    while ((p = getgrent()) != NULL) {
        PyObject *v = mkgrent(p);
        if (v == NULL || PyList_Append(d, v) != 0) {
            Py_XDECREF(v);
            Py_DECREF(d);
            endgrent();
            return NULL;
        }
        Py_DECREF(v);
    }
    endgrent();
    return d;
}

static PyMethodDef grp_methods[] = {
    GRP_GETGRGID_METHODDEF
    GRP_GETGRNAM_METHODDEF
    GRP_GETGRALL_METHODDEF
    {NULL, NULL}
};

PyDoc_STRVAR(grp__doc__,
"Access to the Unix group database.\n\
\n\
Group entries are reported as 4-tuples containing the following fields\n\
from the group database, in order:\n\
\n\
  gr_name   - name of the group\n\
  gr_passwd - group password (encrypted); often empty\n\
  gr_gid    - numeric ID of the group\n\
  gr_mem    - list of members\n\
\n\
The gid is an integer, name and password are strings.  (Note that most\n\
users are not explicitly listed as members of the groups they are in\n\
according to the password database.  Check both databases to get\n\
complete membership information.)");



static struct PyModuleDef grpmodule = {
        PyModuleDef_HEAD_INIT,
        "grp",
        grp__doc__,
        -1,
        grp_methods,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC
PyInit_grp(void)
{
    PyObject *m, *d;
    m = PyModule_Create(&grpmodule);
    if (m == NULL)
        return NULL;
    d = PyModule_GetDict(m);
    if (!initialized) {
        if (PyStructSequence_InitType2(&StructGrpType,
                                       &struct_group_type_desc) < 0)
            return NULL;
    }
    if (PyDict_SetItemString(d, "struct_group",
                             (PyObject *)&StructGrpType) < 0)
        return NULL;
    initialized = 1;
    return m;
}
