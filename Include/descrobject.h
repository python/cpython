/* XXX getter, setter, getsetlist and wrapperbase need 'Py'-prefixed names */

typedef PyObject *(*getter)(PyObject *, void *);
typedef int (*setter)(PyObject *, PyObject *, void *);

struct getsetlist {
	char *name;
	getter get;
	setter set;
	void *closure;
};

typedef PyObject *(*wrapperfunc)(PyObject *self, PyObject *args,
				 void *wrapped);

struct wrapperbase {
	char *name;
	wrapperfunc wrapper;
	char *doc;
};

extern DL_IMPORT(PyObject *) PyDescr_NewMethod(PyTypeObject *, PyMethodDef *);
extern DL_IMPORT(PyObject *) PyDescr_NewMember(PyTypeObject *,
					       struct PyMemberDef *);
extern DL_IMPORT(PyObject *) PyDescr_NewGetSet(PyTypeObject *,
					       struct getsetlist *);
extern DL_IMPORT(PyObject *) PyDescr_NewWrapper(PyTypeObject *,
						struct wrapperbase *, void *);
extern DL_IMPORT(int) PyDescr_IsData(PyObject *);

extern DL_IMPORT(PyObject *) PyDictProxy_New(PyObject *);
extern DL_IMPORT(PyObject *) PyWrapper_New(PyObject *, PyObject *);


extern DL_IMPORT(PyTypeObject) PyProperty_Type;
