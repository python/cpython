#ifndef MS_WIN32
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

#define PARAMFLAG_FIN 0x1
#define PARAMFLAG_FOUT 0x2
#define PARAMFLAG_FLCID 0x4
#endif

typedef struct tagPyCArgObject PyCArgObject;
typedef struct tagCDataObject CDataObject;
typedef PyObject *(* GETFUNC)(void *, Py_ssize_t size);
typedef PyObject *(* SETFUNC)(void *, PyObject *value, Py_ssize_t size);
typedef PyCArgObject *(* PARAMFUNC)(CDataObject *obj);

/* A default buffer in CDataObject, which can be used for small C types.  If
this buffer is too small, PyMem_Malloc will be called to create a larger one,
and this one is not used.

Making CDataObject a variable size object would be a better solution, but more
difficult in the presence of CFuncPtrObject.  Maybe later.
*/
union value {
		char c[16];
		short s;
		int i;
		long l;
		float f;
		double d;
#ifdef HAVE_LONG_LONG
		PY_LONG_LONG ll;
#endif
};

/*
  Hm. Are there CDataObject's which do not need the b_objects member?  In
  this case we probably should introduce b_flags to mark it as present...  If
  b_objects is not present/unused b_length is unneeded as well.
*/

struct tagCDataObject {
	PyObject_HEAD
	char *b_ptr;		/* pointer to memory block */
	int  b_needsfree;	/* need _we_ free the memory? */
	CDataObject *b_base;	/* pointer to base object or NULL */
	Py_ssize_t b_size;	/* size of memory block in bytes */
	Py_ssize_t b_length;	/* number of references we need */
	Py_ssize_t b_index;	/* index of this object into base's
				   b_object list */
	PyObject *b_objects;	/* dictionary of references we need to keep, or Py_None */
	union value b_value;
};

typedef struct {
	ffi_closure *pcl; /* the C callable */
	ffi_cif cif;
	PyObject *converters;
	PyObject *callable;
	SETFUNC setfunc;
	ffi_type *restype;
	ffi_type *atypes[1];
} ffi_info;

typedef struct {
	/* First part identical to tagCDataObject */
	PyObject_HEAD
	char *b_ptr;		/* pointer to memory block */
	int  b_needsfree;	/* need _we_ free the memory? */
	CDataObject *b_base;	/* pointer to base object or NULL */
	Py_ssize_t b_size;	/* size of memory block in bytes */
	Py_ssize_t b_length;	/* number of references we need */
	Py_ssize_t b_index;	/* index of this object into base's
				   b_object list */
	PyObject *b_objects;	/* list of references we need to keep */
	union value b_value;
	/* end of tagCDataObject, additional fields follow */

	ffi_info *thunk;
	PyObject *callable;

	/* These two fields will override the ones in the type's stgdict if
	   they are set */
	PyObject *converters;
	PyObject *argtypes;
	PyObject *restype;
	PyObject *checker;
	PyObject *errcheck;
#ifdef MS_WIN32
	int index;
	GUID *iid;
#endif
	PyObject *paramflags;
} CFuncPtrObject;

extern PyTypeObject StgDict_Type;
#define StgDict_CheckExact(v)	    ((v)->ob_type == &StgDict_Type)
#define StgDict_Check(v)	    PyObject_TypeCheck(v, &StgDict_Type)

extern int StructUnionType_update_stgdict(PyObject *fields, PyObject *type, int isStruct);
extern int PyType_stginfo(PyTypeObject *self, Py_ssize_t *psize, Py_ssize_t *palign, Py_ssize_t *plength);
extern int PyObject_stginfo(PyObject *self, Py_ssize_t *psize, Py_ssize_t *palign, Py_ssize_t *plength);



extern PyTypeObject CData_Type;
#define CDataObject_CheckExact(v)	((v)->ob_type == &CData_Type)
#define CDataObject_Check(v)		PyObject_TypeCheck(v, &CData_Type)

extern PyTypeObject SimpleType_Type;
#define SimpleTypeObject_CheckExact(v)	((v)->ob_type == &SimpleType_Type)
#define SimpleTypeObject_Check(v)	PyObject_TypeCheck(v, &SimpleType_Type)

extern PyTypeObject CField_Type;
extern struct fielddesc *getentry(const char *fmt);


extern PyObject *
CField_FromDesc(PyObject *desc, Py_ssize_t index,
		Py_ssize_t *pfield_size, int bitsize, int *pbitofs,
		Py_ssize_t *psize, Py_ssize_t *poffset, Py_ssize_t *palign,
		int pack, int is_big_endian);

extern PyObject *CData_AtAddress(PyObject *type, void *buf);
extern PyObject *CData_FromBytes(PyObject *type, char *data, Py_ssize_t length);

extern PyTypeObject ArrayType_Type;
extern PyTypeObject Array_Type;
extern PyTypeObject PointerType_Type;
extern PyTypeObject Pointer_Type;
extern PyTypeObject CFuncPtr_Type;
extern PyTypeObject CFuncPtrType_Type;
extern PyTypeObject StructType_Type;

#define ArrayTypeObject_Check(v)	PyObject_TypeCheck(v, &ArrayType_Type)
#define ArrayObject_Check(v)		PyObject_TypeCheck(v, &Array_Type)
#define PointerObject_Check(v)		PyObject_TypeCheck(v, &Pointer_Type)
#define PointerTypeObject_Check(v)	PyObject_TypeCheck(v, &PointerType_Type)
#define CFuncPtrObject_Check(v)		PyObject_TypeCheck(v, &CFuncPtr_Type)
#define CFuncPtrTypeObject_Check(v)	PyObject_TypeCheck(v, &CFuncPtrType_Type)
#define StructTypeObject_Check(v)	PyObject_TypeCheck(v, &StructType_Type)

extern PyObject *
CreateArrayType(PyObject *itemtype, Py_ssize_t length);

extern void init_callbacks_in_module(PyObject *m);

extern PyMethodDef module_methods[];

extern ffi_info *AllocFunctionCallback(PyObject *callable,
				       PyObject *converters,
				       PyObject *restype,
				       int stdcall);
/* a table entry describing a predefined ctypes type */
struct fielddesc {
	char code;
	SETFUNC setfunc;
	GETFUNC getfunc;
	ffi_type *pffi_type; /* always statically allocated */
	SETFUNC setfunc_swapped;
	GETFUNC getfunc_swapped;
};

typedef struct {
	PyObject_HEAD
	Py_ssize_t offset;
	Py_ssize_t size;
	Py_ssize_t index;		/* Index into CDataObject's
					   object array */
	PyObject *proto;		/* a type or NULL */
	GETFUNC getfunc;		/* getter function if proto is NULL */
	SETFUNC setfunc;		/* setter function if proto is NULL */
	int anonymous;
} CFieldObject;

/* A subclass of PyDictObject, used as the instance dictionary of ctypes
   metatypes */
typedef struct {
	PyDictObject dict;	/* first part identical to PyDictObject */
/* The size and align fields are unneeded, they are in ffi_type as well.  As
   an experiment shows, it's trivial to get rid of them, the only thing to
   remember is that in ArrayType_new the ffi_type fields must be filled in -
   so far it was unneeded because libffi doesn't support arrays at all
   (because they are passed as pointers to function calls anyway).  But it's
   too much risk to change that now, and there are other fields which doen't
   belong into this structure anyway.  Maybe in ctypes 2.0... (ctypes 2000?)
*/
	Py_ssize_t size;	/* number of bytes */
	Py_ssize_t align;	/* alignment requirements */
	Py_ssize_t length;	/* number of fields */
	ffi_type ffi_type_pointer;
	PyObject *proto;	/* Only for Pointer/ArrayObject */
	SETFUNC setfunc;	/* Only for simple objects */
	GETFUNC getfunc;	/* Only for simple objects */
	PARAMFUNC paramfunc;

	/* Following fields only used by CFuncPtrType_Type instances */
	PyObject *argtypes;	/* tuple of CDataObjects */
	PyObject *converters;	/* tuple([t.from_param for t in argtypes]) */
	PyObject *restype;	/* CDataObject or NULL */
	PyObject *checker;
	int flags;		/* calling convention and such */
} StgDictObject;

/****************************************************************
 StgDictObject fields

 setfunc and getfunc is only set for simple data types, it is copied from the
 corresponding fielddesc entry.  These are functions to set and get the value
 in a memory block.
 They should probably by used by other types as well.

 proto is only used for Pointer and Array types - it points to the item type
 object.

 Probably all the magic ctypes methods (like from_param) should have C
 callable wrappers in the StgDictObject.  For simple data type, for example,
 the fielddesc table could have entries for C codec from_param functions or
 other methods as well, if a subtype overrides this method in Python at
 construction time, or assigns to it later, tp_setattro should update the
 StgDictObject function to a generic one.

 Currently, CFuncPtr types have 'converters' and 'checker' entries in their
 type dict.  They are only used to cache attributes from other entries, whihc
 is wrong.

 One use case is the .value attribute that all simple types have.  But some
 complex structures, like VARIANT, represent a single value also, and should
 have this attribute.

 Another use case is a _check_retval_ function, which is called when a ctypes
 type is used as return type of a function to validate and compute the return
 value.

 Common ctypes protocol:

  - setfunc: store a python value in a memory block
  - getfunc: convert data from a memory block into a python value

  - checkfunc: validate and convert a return value from a function call
  - toparamfunc: convert a python value into a function argument

*****************************************************************/

/* May return NULL, but does not set an exception! */
extern StgDictObject *PyType_stgdict(PyObject *obj);

/* May return NULL, but does not set an exception! */
extern StgDictObject *PyObject_stgdict(PyObject *self);

extern int StgDict_clone(StgDictObject *src, StgDictObject *dst);

typedef int(* PPROC)(void);

PyObject *_CallProc(PPROC pProc,
		    PyObject *arguments,
#ifdef MS_WIN32
		    IUnknown *pIUnk,
		    GUID *iid,
#endif
		    int flags,
		    PyObject *argtypes,
		    PyObject *restype,
		    PyObject *checker);
 

#define FUNCFLAG_STDCALL 0x0
#define FUNCFLAG_CDECL   0x1
#define FUNCFLAG_HRESULT 0x2
#define FUNCFLAG_PYTHONAPI 0x4

#define DICTFLAG_FINAL 0x1000

struct tagPyCArgObject {
	PyObject_HEAD
	ffi_type *pffi_type;
	char tag;
	union {
		char c;
		char b;
		short h;
		int i;
		long l;
#ifdef HAVE_LONG_LONG
		PY_LONG_LONG q;
#endif
		double d;
		float f;
		void *p;
	} value;
	PyObject *obj;
	Py_ssize_t size; /* for the 'V' tag */
};

extern PyTypeObject PyCArg_Type;
extern PyCArgObject *new_CArgObject(void);
#define PyCArg_CheckExact(v)	    ((v)->ob_type == &PyCArg_Type)
extern PyCArgObject *new_CArgObject(void);

extern PyObject *
CData_get(PyObject *type, GETFUNC getfunc, PyObject *src,
	  Py_ssize_t index, Py_ssize_t size, char *ptr);

extern int
CData_set(PyObject *dst, PyObject *type, SETFUNC setfunc, PyObject *value,
	  Py_ssize_t index, Py_ssize_t size, char *ptr);

extern void Extend_Error_Info(PyObject *exc_class, char *fmt, ...);

struct basespec {
	CDataObject *base;
	Py_ssize_t index;
	char *adr;
};

extern char basespec_string[];

extern ffi_type *GetType(PyObject *obj);

/* exception classes */
extern PyObject *PyExc_ArgError;

extern char *conversion_mode_encoding;
extern char *conversion_mode_errors;

#if defined(HAVE_WCHAR_H)
#  define CTYPES_UNICODE
#endif


extern void FreeClosure(void *);
extern void *MallocClosure(void);

extern void _AddTraceback(char *, char *, int);

extern PyObject *CData_FromBaseObj(PyObject *type, PyObject *base, Py_ssize_t index, char *adr);

/* XXX better name needed! */
extern int IsSimpleSubType(PyObject *obj);


#ifdef MS_WIN32
extern PyObject *ComError;
#endif

/*
 Local Variables:
 compile-command: "python setup.py -q build install --home ~"
 End:
*/
