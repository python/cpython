/* Descriptors */

typedef PyObject *(*getter)(PyObject *, void *);
typedef int (*setter)(PyObject *, PyObject *, void *);

typedef struct PyGetSetDef {
	char *name;
	getter get;
	setter set;
	char *doc;
	void *closure;
} PyGetSetDef;

typedef PyObject *(*wrapperfunc)(PyObject *self, PyObject *args,
				 void *wrapped);

typedef PyObject *(*wrapperfunc_kwds)(PyObject *self, PyObject *args,
				      void *wrapped, PyObject *kwds);

struct wrapperbase {
	char *name;
	int offset;
	void *function;
	wrapperfunc wrapper;
	char *doc;
	int flags;
	PyObject *name_strobj;
};

/* Flags for above struct */
#define PyWrapperFlag_KEYWORDS 1 /* wrapper function takes keyword args */

/* Various kinds of descriptor objects */

#define PyDescr_COMMON \
	PyObject_HEAD \
	PyTypeObject *d_type; \
	PyObject *d_name

typedef struct {
	PyDescr_COMMON;
} PyDescrObject;

typedef struct {
	PyDescr_COMMON;
	PyMethodDef *d_method;
} PyMethodDescrObject;

typedef struct {
	PyDescr_COMMON;
	struct PyMemberDef *d_member;
} PyMemberDescrObject;

typedef struct {
	PyDescr_COMMON;
	PyGetSetDef *d_getset;
} PyGetSetDescrObject;

typedef struct {
	PyDescr_COMMON;
	struct wrapperbase *d_base;
	void *d_wrapped; /* This can be any function pointer */
} PyWrapperDescrObject;

extern DL_IMPORT(PyTypeObject) PyWrapperDescr_Type;

extern DL_IMPORT(PyObject *) PyDescr_NewMethod(PyTypeObject *, PyMethodDef *);
extern DL_IMPORT(PyObject *) PyDescr_NewMember(PyTypeObject *,
					       struct PyMemberDef *);
extern DL_IMPORT(PyObject *) PyDescr_NewGetSet(PyTypeObject *,
					       struct PyGetSetDef *);
extern DL_IMPORT(PyObject *) PyDescr_NewWrapper(PyTypeObject *,
						struct wrapperbase *, void *);
extern DL_IMPORT(int) PyDescr_IsData(PyObject *);

extern DL_IMPORT(PyObject *) PyDictProxy_New(PyObject *);
extern DL_IMPORT(PyObject *) PyWrapper_New(PyObject *, PyObject *);


extern DL_IMPORT(PyTypeObject) PyProperty_Type;
