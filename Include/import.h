#ifndef Py_IMPORT_H
#define Py_IMPORT_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Module definition and import interface */

DL_IMPORT(long) PyImport_GetMagicNumber Py_PROTO((void));
DL_IMPORT(PyObject *) PyImport_ExecCodeModule Py_PROTO((char *name, PyObject *co));
DL_IMPORT(PyObject *) PyImport_ExecCodeModuleEx Py_PROTO((
	char *name, PyObject *co, char *pathname));
DL_IMPORT(PyObject *) PyImport_GetModuleDict Py_PROTO((void));
DL_IMPORT(PyObject *) PyImport_AddModule Py_PROTO((char *name));
DL_IMPORT(PyObject *) PyImport_ImportModule Py_PROTO((char *name));
DL_IMPORT(PyObject *) PyImport_ImportModuleEx Py_PROTO((
	char *name, PyObject *globals, PyObject *locals, PyObject *fromlist));
DL_IMPORT(PyObject *) PyImport_Import Py_PROTO((PyObject *name));
DL_IMPORT(PyObject *) PyImport_ReloadModule Py_PROTO((PyObject *m));
DL_IMPORT(void) PyImport_Cleanup Py_PROTO((void));
DL_IMPORT(int) PyImport_ImportFrozenModule Py_PROTO((char *));

extern DL_IMPORT(PyObject *)_PyImport_FindExtension Py_PROTO((char *, char *));
extern DL_IMPORT(PyObject *)_PyImport_FixupExtension Py_PROTO((char *, char *));

struct _inittab {
	char *name;
	void (*initfunc)();
};

extern DL_IMPORT(struct _inittab *) PyImport_Inittab;

extern DL_IMPORT(int) PyImport_AppendInittab Py_PROTO((char *name, void (*initfunc)()));
extern DL_IMPORT(int) PyImport_ExtendInittab Py_PROTO((struct _inittab *newtab));

struct _frozen {
	char *name;
	unsigned char *code;
	int size;
};

/* Embedding apps may change this pointer to point to their favorite
   collection of frozen modules: */

extern DL_IMPORT(struct _frozen *) PyImport_FrozenModules;

#ifdef __cplusplus
}
#endif
#endif /* !Py_IMPORT_H */
