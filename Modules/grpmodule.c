
/* UNIX group file access module */

#include "Python.h"

#include <sys/types.h>
#include <grp.h>


static PyObject *
mkgrent(struct group *p)
{
    PyObject *v, *w;
    char **member;
    if ((w = PyList_New(0)) == NULL) {
        return NULL;
    }
    for (member = p->gr_mem; *member != NULL; member++) {
        PyObject *x = PyString_FromString(*member);
        if (x == NULL || PyList_Append(w, x) != 0) {
            Py_XDECREF(x);
            Py_DECREF(w);
            return NULL;
        }
        Py_DECREF(x);
    }
    v = Py_BuildValue("(sslO)",
                      p->gr_name,
                      p->gr_passwd,
#if defined(NeXT) && defined(_POSIX_SOURCE) && defined(__LITTLE_ENDIAN__)
/* Correct a bug present on Intel machines in NextStep 3.2 and 3.3;
   for later versions you may have to remove this */
                      (long)p->gr_short_pad, /* ugh-NeXT broke the padding */
#else
                      (long)p->gr_gid,
#endif
                      w);
    Py_DECREF(w);
    return v;
}

static PyObject *
grp_getgrgid(PyObject *self, PyObject *args)
{
    int gid;
    struct group *p;
    if (!PyArg_ParseTuple(args, "i:getgrgid", &gid))
        return NULL;
    if ((p = getgrgid(gid)) == NULL) {
        PyErr_SetString(PyExc_KeyError, "getgrgid(): gid not found");
        return NULL;
    }
    return mkgrent(p);
}

static PyObject *
grp_getgrnam(PyObject *self, PyObject *args)
{
    char *name;
    struct group *p;
    if (!PyArg_ParseTuple(args, "s:getgrnam", &name))
        return NULL;
    if ((p = getgrnam(name)) == NULL) {
        PyErr_SetString(PyExc_KeyError, "getgrnam(): name not found");
        return NULL;
    }
    return mkgrent(p);
}

static PyObject *
grp_getgrall(PyObject *self, PyObject *args)
{
    PyObject *d;
    struct group *p;

    if (!PyArg_ParseTuple(args, ":getgrall"))
        return NULL;
    if ((d = PyList_New(0)) == NULL)
        return NULL;
    setgrent();
    while ((p = getgrent()) != NULL) {
        PyObject *v = mkgrent(p);
        if (v == NULL || PyList_Append(d, v) != 0) {
            Py_XDECREF(v);
            Py_DECREF(d);
            return NULL;
        }
        Py_DECREF(v);
    }
    endgrent();
    return d;
}

static PyMethodDef grp_methods[] = {
    {"getgrgid",	grp_getgrgid,	METH_VARARGS,
     "getgrgid(id) -> tuple\n\
Return the group database entry for the given numeric group ID.  If\n\
id is not valid, raise KeyError."},
    {"getgrnam",	grp_getgrnam,	METH_VARARGS,
     "getgrnam(name) -> tuple\n\
Return the group database entry for the given group name.  If\n\
name is not valid, raise KeyError."},
    {"getgrall",	grp_getgrall,	METH_VARARGS,
     "getgrall() -> list of tuples\n\
Return a list of all available group entries, in arbitrary order."},
    {NULL,		NULL}		/* sentinel */
};

static char grp__doc__[] =
"Access to the Unix group database.\n\
\n\
Group entries are reported as 4-tuples containing the following fields\n\
from the group database, in order:\n\
\n\
  name   - name of the group\n\
  passwd - group password (encrypted); often empty\n\
  gid    - numeric ID of the group\n\
  mem    - list of members\n\
\n\
The gid is an integer, name and password are strings.  (Note that most\n\
users are not explicitly listed as members of the groups they are in\n\
according to the password database.  Check both databases to get\n\
complete membership information.)";


DL_EXPORT(void)
initgrp(void)
{
    Py_InitModule3("grp", grp_methods, grp__doc__);
}
