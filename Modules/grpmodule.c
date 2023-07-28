
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


typedef struct {
  PyTypeObject *StructGrpType;
} grpmodulestate;

static inline grpmodulestate*
get_grp_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (grpmodulestate *)state;
}

static struct PyModuleDef grpmodule;

#define DEFAULT_BUFFER_SIZE 1024

static PyObject *
mkgrent(PyObject *module, struct group *p)
{
    int setIndex = 0;
    PyObject *v, *w;
    char **member;

    v = PyStructSequence_New(get_grp_state(module)->StructGrpType);
    if (v == NULL)
        return NULL;

    if ((w = PyList_New(0)) == NULL) {
        Py_DECREF(v);
        return NULL;
    }
    for (member = p->gr_mem; ; member++) {
        char *group_member;
        // member can be misaligned
        memcpy(&group_member, member, sizeof(group_member));
        if (group_member == NULL) {
            break;
        }
        PyObject *x = PyUnicode_DecodeFSDefault(group_member);
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
grp_getgrgid_impl(PyObject *module, PyObject *id)
/*[clinic end generated code: output=30797c289504a1ba input=15fa0e2ccf5cda25]*/
{
    PyObject *retval = NULL;
    int nomem = 0;
    char *buf = NULL, *buf2 = NULL;
    gid_t gid;
    struct group *p;

    if (!_Py_Gid_Converter(id, &gid)) {
        return NULL;
    }
#ifdef HAVE_GETGRGID_R
    int status;
    Py_ssize_t bufsize;
    /* Note: 'grp' will be used via pointer 'p' on getgrgid_r success. */
    struct group grp;

    Py_BEGIN_ALLOW_THREADS
    bufsize = sysconf(_SC_GETGR_R_SIZE_MAX);
    if (bufsize == -1) {
        bufsize = DEFAULT_BUFFER_SIZE;
    }

    while (1) {
        buf2 = PyMem_RawRealloc(buf, bufsize);
        if (buf2 == NULL) {
            p = NULL;
            nomem = 1;
            break;
        }
        buf = buf2;
        status = getgrgid_r(gid, &grp, buf, bufsize, &p);
        if (status != 0) {
            p = NULL;
        }
        if (p != NULL || status != ERANGE) {
            break;
        }
        if (bufsize > (PY_SSIZE_T_MAX >> 1)) {
            nomem = 1;
            break;
        }
        bufsize <<= 1;
    }

    Py_END_ALLOW_THREADS
#else
    p = getgrgid(gid);
#endif
    if (p == NULL) {
        PyMem_RawFree(buf);
        if (nomem == 1) {
            return PyErr_NoMemory();
        }
        PyObject *gid_obj = _PyLong_FromGid(gid);
        if (gid_obj == NULL)
            return NULL;
        PyErr_Format(PyExc_KeyError, "getgrgid(): gid not found: %S", gid_obj);
        Py_DECREF(gid_obj);
        return NULL;
    }
    retval = mkgrent(module, p);
#ifdef HAVE_GETGRGID_R
    PyMem_RawFree(buf);
#endif
    return retval;
}

/*[clinic input]
grp.getgrnam

    name: unicode

Return the group database entry for the given group name.

If name is not valid, raise KeyError.
[clinic start generated code]*/

static PyObject *
grp_getgrnam_impl(PyObject *module, PyObject *name)
/*[clinic end generated code: output=67905086f403c21c input=08ded29affa3c863]*/
{
    char *buf = NULL, *buf2 = NULL, *name_chars;
    int nomem = 0;
    struct group *p;
    PyObject *bytes, *retval = NULL;

    if ((bytes = PyUnicode_EncodeFSDefault(name)) == NULL)
        return NULL;
    /* check for embedded null bytes */
    if (PyBytes_AsStringAndSize(bytes, &name_chars, NULL) == -1)
        goto out;
#ifdef HAVE_GETGRNAM_R
    int status;
    Py_ssize_t bufsize;
    /* Note: 'grp' will be used via pointer 'p' on getgrnam_r success. */
    struct group grp;

    Py_BEGIN_ALLOW_THREADS
    bufsize = sysconf(_SC_GETGR_R_SIZE_MAX);
    if (bufsize == -1) {
        bufsize = DEFAULT_BUFFER_SIZE;
    }

    while(1) {
        buf2 = PyMem_RawRealloc(buf, bufsize);
        if (buf2 == NULL) {
            p = NULL;
            nomem = 1;
            break;
        }
        buf = buf2;
        status = getgrnam_r(name_chars, &grp, buf, bufsize, &p);
        if (status != 0) {
            p = NULL;
        }
        if (p != NULL || status != ERANGE) {
            break;
        }
        if (bufsize > (PY_SSIZE_T_MAX >> 1)) {
            nomem = 1;
            break;
        }
        bufsize <<= 1;
    }

    Py_END_ALLOW_THREADS
#else
    p = getgrnam(name_chars);
#endif
    if (p == NULL) {
        if (nomem == 1) {
            PyErr_NoMemory();
        }
        else {
            PyErr_Format(PyExc_KeyError, "getgrnam(): name not found: %R", name);
        }
        goto out;
    }
    retval = mkgrent(module, p);
out:
    PyMem_RawFree(buf);
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
grp_getgrall_impl(PyObject *module)
/*[clinic end generated code: output=585dad35e2e763d7 input=d7df76c825c367df]*/
{
    PyObject *d;
    struct group *p;

    if ((d = PyList_New(0)) == NULL)
        return NULL;
    setgrent();
    while ((p = getgrent()) != NULL) {
        PyObject *v = mkgrent(module, p);
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

static int
grpmodule_exec(PyObject *module)
{
    grpmodulestate *state = get_grp_state(module);

    state->StructGrpType = PyStructSequence_NewType(&struct_group_type_desc);
    if (state->StructGrpType == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->StructGrpType) < 0) {
        return -1;
    }
    return 0;
}

static PyModuleDef_Slot grpmodule_slots[] = {
    {Py_mod_exec, grpmodule_exec},
    {0, NULL}
};

static int grpmodule_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(get_grp_state(m)->StructGrpType);
    return 0;
}

static int grpmodule_clear(PyObject *m) {
    Py_CLEAR(get_grp_state(m)->StructGrpType);
    return 0;
}

static void grpmodule_free(void *m) {
    grpmodule_clear((PyObject *)m);
}

static struct PyModuleDef grpmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "grp",
    .m_doc = grp__doc__,
    .m_size = sizeof(grpmodulestate),
    .m_methods = grp_methods,
    .m_slots = grpmodule_slots,
    .m_traverse = grpmodule_traverse,
    .m_clear = grpmodule_clear,
    .m_free = grpmodule_free,
};

PyMODINIT_FUNC
PyInit_grp(void)
{
   return PyModuleDef_Init(&grpmodule);
}
