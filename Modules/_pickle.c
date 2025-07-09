/* pickle accelerator C extensor: _pickle module.
 *
 * It is built as a built-in module (Py_BUILD_CORE_BUILTIN define) on Windows
 * and as an extension module (Py_BUILD_CORE_MODULE define) on other
 * platforms. */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_bytesobject.h"   // _PyBytesWriter
#include "pycore_ceval.h"         // _Py_EnterRecursiveCall()
#include "pycore_critical_section.h" // Py_BEGIN_CRITICAL_SECTION()
#include "pycore_long.h"          // _PyLong_AsByteArray()
#include "pycore_moduleobject.h"  // _PyModule_GetState()
#include "pycore_object.h"        // _PyNone_Type
#include "pycore_pyerrors.h"      // _PyErr_FormatNote
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_runtime.h"       // _Py_ID()
#include "pycore_setobject.h"     // _PySet_NextEntry()
#include "pycore_sysmodule.h"     // _PySys_GetSizeOf()
#include "pycore_unicodeobject.h" // _PyUnicode_EqualToASCIIString()

#include <stdlib.h>               // strtol()


PyDoc_STRVAR(pickle_module_doc,
"Optimized C implementation for the Python pickle module.");

/*[clinic input]
module _pickle
class _pickle.Pickler "PicklerObject *" ""
class _pickle.PicklerMemoProxy "PicklerMemoProxyObject *" ""
class _pickle.Unpickler "UnpicklerObject *" ""
class _pickle.UnpicklerMemoProxy "UnpicklerMemoProxyObject *" ""
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=b6d7191ab6466cda]*/

/* Bump HIGHEST_PROTOCOL when new opcodes are added to the pickle protocol.
   Bump DEFAULT_PROTOCOL only when the oldest still supported version of Python
   already includes it. */
enum {
    HIGHEST_PROTOCOL = 5,
    DEFAULT_PROTOCOL = 5
};

#ifdef MS_WINDOWS
// These are already typedefs from windows.h, pulled in via pycore_runtime.h.
#define FLOAT FLOAT_
#define INT INT_
#define LONG LONG_

/* This can already be defined on Windows to set the character set
   the Windows header files treat as default */
#ifdef UNICODE
#undef UNICODE
#endif
#endif

/* Pickle opcodes. These must be kept updated with pickle.py.
   Extensive docs are in pickletools.py. */
enum opcode {
    MARK            = '(',
    STOP            = '.',
    POP             = '0',
    POP_MARK        = '1',
    DUP             = '2',
    FLOAT           = 'F',
    INT             = 'I',
    BININT          = 'J',
    BININT1         = 'K',
    LONG            = 'L',
    BININT2         = 'M',
    NONE            = 'N',
    PERSID          = 'P',
    BINPERSID       = 'Q',
    REDUCE          = 'R',
    STRING          = 'S',
    BINSTRING       = 'T',
    SHORT_BINSTRING = 'U',
    UNICODE         = 'V',
    BINUNICODE      = 'X',
    APPEND          = 'a',
    BUILD           = 'b',
    GLOBAL          = 'c',
    DICT            = 'd',
    EMPTY_DICT      = '}',
    APPENDS         = 'e',
    GET             = 'g',
    BINGET          = 'h',
    INST            = 'i',
    LONG_BINGET     = 'j',
    LIST            = 'l',
    EMPTY_LIST      = ']',
    OBJ             = 'o',
    PUT             = 'p',
    BINPUT          = 'q',
    LONG_BINPUT     = 'r',
    SETITEM         = 's',
    TUPLE           = 't',
    EMPTY_TUPLE     = ')',
    SETITEMS        = 'u',
    BINFLOAT        = 'G',

    /* Protocol 2. */
    PROTO       = '\x80',
    NEWOBJ      = '\x81',
    EXT1        = '\x82',
    EXT2        = '\x83',
    EXT4        = '\x84',
    TUPLE1      = '\x85',
    TUPLE2      = '\x86',
    TUPLE3      = '\x87',
    NEWTRUE     = '\x88',
    NEWFALSE    = '\x89',
    LONG1       = '\x8a',
    LONG4       = '\x8b',

    /* Protocol 3 (Python 3.x) */
    BINBYTES       = 'B',
    SHORT_BINBYTES = 'C',

    /* Protocol 4 */
    SHORT_BINUNICODE = '\x8c',
    BINUNICODE8      = '\x8d',
    BINBYTES8        = '\x8e',
    EMPTY_SET        = '\x8f',
    ADDITEMS         = '\x90',
    FROZENSET        = '\x91',
    NEWOBJ_EX        = '\x92',
    STACK_GLOBAL     = '\x93',
    MEMOIZE          = '\x94',
    FRAME            = '\x95',

    /* Protocol 5 */
    BYTEARRAY8       = '\x96',
    NEXT_BUFFER      = '\x97',
    READONLY_BUFFER  = '\x98'
};

enum {
   /* Keep in synch with pickle.Pickler._BATCHSIZE.  This is how many elements
      batch_list/dict() pumps out before doing APPENDS/SETITEMS.  Nothing will
      break if this gets out of synch with pickle.py, but it's unclear that would
      help anything either. */
    BATCHSIZE = 1000,

    /* Nesting limit until Pickler, when running in "fast mode", starts
       checking for self-referential data-structures. */
    FAST_NESTING_LIMIT = 50,

    /* Initial size of the write buffer of Pickler. */
    WRITE_BUF_SIZE = 4096,

    /* Prefetch size when unpickling (disabled on unpeekable streams) */
    PREFETCH = 8192 * 16,

    FRAME_SIZE_MIN = 4,
    FRAME_SIZE_TARGET = 64 * 1024,
    FRAME_HEADER_SIZE = 9
};

/*************************************************************************/

/* State of the pickle module, per PEP 3121. */
typedef struct {
    /* Exception classes for pickle. */
    PyObject *PickleError;
    PyObject *PicklingError;
    PyObject *UnpicklingError;

    /* copyreg.dispatch_table, {type_object: pickling_function} */
    PyObject *dispatch_table;

    /* For the extension opcodes EXT1, EXT2 and EXT4. */

    /* copyreg._extension_registry, {(module_name, function_name): code} */
    PyObject *extension_registry;
    /* copyreg._extension_cache, {code: object} */
    PyObject *extension_cache;
    /* copyreg._inverted_registry, {code: (module_name, function_name)} */
    PyObject *inverted_registry;

    /* Import mappings for compatibility with Python 2.x */

    /* _compat_pickle.NAME_MAPPING,
       {(oldmodule, oldname): (newmodule, newname)} */
    PyObject *name_mapping_2to3;
    /* _compat_pickle.IMPORT_MAPPING, {oldmodule: newmodule} */
    PyObject *import_mapping_2to3;
    /* Same, but with REVERSE_NAME_MAPPING / REVERSE_IMPORT_MAPPING */
    PyObject *name_mapping_3to2;
    PyObject *import_mapping_3to2;

    /* codecs.encode, used for saving bytes in older protocols */
    PyObject *codecs_encode;
    /* builtins.getattr, used for saving nested names with protocol < 4 */
    PyObject *getattr;
    /* functools.partial, used for implementing __newobj_ex__ with protocols
       2 and 3 */
    PyObject *partial;

    /* Types */
    PyTypeObject *Pickler_Type;
    PyTypeObject *Unpickler_Type;
    PyTypeObject *Pdata_Type;
    PyTypeObject *PicklerMemoProxyType;
    PyTypeObject *UnpicklerMemoProxyType;
} PickleState;

/* Forward declaration of the _pickle module definition. */
static struct PyModuleDef _picklemodule;

/* Given a module object, get its per-module state. */
static inline PickleState *
_Pickle_GetState(PyObject *module)
{
    void *state = _PyModule_GetState(module);
    assert(state != NULL);
    return (PickleState *)state;
}

static inline PickleState *
_Pickle_GetStateByClass(PyTypeObject *cls)
{
    void *state = _PyType_GetModuleState(cls);
    assert(state != NULL);
    return (PickleState *)state;
}

static inline PickleState *
_Pickle_FindStateByType(PyTypeObject *tp)
{
    PyObject *module = PyType_GetModuleByDef(tp, &_picklemodule);
    assert(module != NULL);
    return _Pickle_GetState(module);
}

/* Clear the given pickle module state. */
static void
_Pickle_ClearState(PickleState *st)
{
    Py_CLEAR(st->PickleError);
    Py_CLEAR(st->PicklingError);
    Py_CLEAR(st->UnpicklingError);
    Py_CLEAR(st->dispatch_table);
    Py_CLEAR(st->extension_registry);
    Py_CLEAR(st->extension_cache);
    Py_CLEAR(st->inverted_registry);
    Py_CLEAR(st->name_mapping_2to3);
    Py_CLEAR(st->import_mapping_2to3);
    Py_CLEAR(st->name_mapping_3to2);
    Py_CLEAR(st->import_mapping_3to2);
    Py_CLEAR(st->codecs_encode);
    Py_CLEAR(st->getattr);
    Py_CLEAR(st->partial);
    Py_CLEAR(st->Pickler_Type);
    Py_CLEAR(st->Unpickler_Type);
    Py_CLEAR(st->Pdata_Type);
    Py_CLEAR(st->PicklerMemoProxyType);
    Py_CLEAR(st->UnpicklerMemoProxyType);
}

/* Initialize the given pickle module state. */
static int
_Pickle_InitState(PickleState *st)
{
    PyObject *copyreg = NULL;
    PyObject *compat_pickle = NULL;

    st->getattr = _PyEval_GetBuiltin(&_Py_ID(getattr));
    if (st->getattr == NULL)
        goto error;

    copyreg = PyImport_ImportModule("copyreg");
    if (!copyreg)
        goto error;
    st->dispatch_table = PyObject_GetAttrString(copyreg, "dispatch_table");
    if (!st->dispatch_table)
        goto error;
    if (!PyDict_CheckExact(st->dispatch_table)) {
        PyErr_Format(PyExc_RuntimeError,
                     "copyreg.dispatch_table should be a dict, not %.200s",
                     Py_TYPE(st->dispatch_table)->tp_name);
        goto error;
    }
    st->extension_registry = \
        PyObject_GetAttrString(copyreg, "_extension_registry");
    if (!st->extension_registry)
        goto error;
    if (!PyDict_CheckExact(st->extension_registry)) {
        PyErr_Format(PyExc_RuntimeError,
                     "copyreg._extension_registry should be a dict, "
                     "not %.200s", Py_TYPE(st->extension_registry)->tp_name);
        goto error;
    }
    st->inverted_registry = \
        PyObject_GetAttrString(copyreg, "_inverted_registry");
    if (!st->inverted_registry)
        goto error;
    if (!PyDict_CheckExact(st->inverted_registry)) {
        PyErr_Format(PyExc_RuntimeError,
                     "copyreg._inverted_registry should be a dict, "
                     "not %.200s", Py_TYPE(st->inverted_registry)->tp_name);
        goto error;
    }
    st->extension_cache = PyObject_GetAttrString(copyreg, "_extension_cache");
    if (!st->extension_cache)
        goto error;
    if (!PyDict_CheckExact(st->extension_cache)) {
        PyErr_Format(PyExc_RuntimeError,
                     "copyreg._extension_cache should be a dict, "
                     "not %.200s", Py_TYPE(st->extension_cache)->tp_name);
        goto error;
    }
    Py_CLEAR(copyreg);

    /* Load the 2.x -> 3.x stdlib module mapping tables */
    compat_pickle = PyImport_ImportModule("_compat_pickle");
    if (!compat_pickle)
        goto error;
    st->name_mapping_2to3 = \
        PyObject_GetAttrString(compat_pickle, "NAME_MAPPING");
    if (!st->name_mapping_2to3)
        goto error;
    if (!PyDict_CheckExact(st->name_mapping_2to3)) {
        PyErr_Format(PyExc_RuntimeError,
                     "_compat_pickle.NAME_MAPPING should be a dict, not %.200s",
                     Py_TYPE(st->name_mapping_2to3)->tp_name);
        goto error;
    }
    st->import_mapping_2to3 = \
        PyObject_GetAttrString(compat_pickle, "IMPORT_MAPPING");
    if (!st->import_mapping_2to3)
        goto error;
    if (!PyDict_CheckExact(st->import_mapping_2to3)) {
        PyErr_Format(PyExc_RuntimeError,
                     "_compat_pickle.IMPORT_MAPPING should be a dict, "
                     "not %.200s", Py_TYPE(st->import_mapping_2to3)->tp_name);
        goto error;
    }
    /* ... and the 3.x -> 2.x mapping tables */
    st->name_mapping_3to2 = \
        PyObject_GetAttrString(compat_pickle, "REVERSE_NAME_MAPPING");
    if (!st->name_mapping_3to2)
        goto error;
    if (!PyDict_CheckExact(st->name_mapping_3to2)) {
        PyErr_Format(PyExc_RuntimeError,
                     "_compat_pickle.REVERSE_NAME_MAPPING should be a dict, "
                     "not %.200s", Py_TYPE(st->name_mapping_3to2)->tp_name);
        goto error;
    }
    st->import_mapping_3to2 = \
        PyObject_GetAttrString(compat_pickle, "REVERSE_IMPORT_MAPPING");
    if (!st->import_mapping_3to2)
        goto error;
    if (!PyDict_CheckExact(st->import_mapping_3to2)) {
        PyErr_Format(PyExc_RuntimeError,
                     "_compat_pickle.REVERSE_IMPORT_MAPPING should be a dict, "
                     "not %.200s", Py_TYPE(st->import_mapping_3to2)->tp_name);
        goto error;
    }
    Py_CLEAR(compat_pickle);

    st->codecs_encode = PyImport_ImportModuleAttrString("codecs", "encode");
    if (st->codecs_encode == NULL) {
        goto error;
    }
    if (!PyCallable_Check(st->codecs_encode)) {
        PyErr_Format(PyExc_RuntimeError,
                     "codecs.encode should be a callable, not %.200s",
                     Py_TYPE(st->codecs_encode)->tp_name);
        goto error;
    }

    st->partial = PyImport_ImportModuleAttrString("functools", "partial");
    if (!st->partial)
        goto error;

    return 0;

  error:
    Py_CLEAR(copyreg);
    Py_CLEAR(compat_pickle);
    _Pickle_ClearState(st);
    return -1;
}

/* Helper for calling a function with a single argument quickly.

   This function steals the reference of the given argument. */
static PyObject *
_Pickle_FastCall(PyObject *func, PyObject *obj)
{
    PyObject *result;

    result = PyObject_CallOneArg(func, obj);
    Py_DECREF(obj);
    return result;
}

/*************************************************************************/

/* Internal data type used as the unpickling stack. */
typedef struct {
    PyObject_VAR_HEAD
    PyObject **data;
    int mark_set;          /* is MARK set? */
    Py_ssize_t fence;      /* position of top MARK or 0 */
    Py_ssize_t allocated;  /* number of slots in data allocated */
} Pdata;

#define Pdata_CAST(op)  ((Pdata *)(op))

static int
Pdata_traverse(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static void
Pdata_dealloc(PyObject *op)
{
    Pdata *self = Pdata_CAST(op);
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    Py_ssize_t i = Py_SIZE(self);
    while (--i >= 0) {
        Py_DECREF(self->data[i]);
    }
    PyMem_Free(self->data);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyType_Slot pdata_slots[] = {
    {Py_tp_dealloc, Pdata_dealloc},
    {Py_tp_traverse, Pdata_traverse},
    {0, NULL},
};

static PyType_Spec pdata_spec = {
    .name = "_pickle.Pdata",
    .basicsize = sizeof(Pdata),
    .itemsize = sizeof(PyObject *),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = pdata_slots,
};

static PyObject *
Pdata_New(PickleState *state)
{
    Pdata *self;

    if (!(self = PyObject_GC_New(Pdata, state->Pdata_Type)))
        return NULL;
    Py_SET_SIZE(self, 0);
    self->mark_set = 0;
    self->fence = 0;
    self->allocated = 8;
    self->data = PyMem_Malloc(self->allocated * sizeof(PyObject *));
    if (self->data) {
        PyObject_GC_Track(self);
        return (PyObject *)self;
    }
    Py_DECREF(self);
    return PyErr_NoMemory();
}


/* Retain only the initial clearto items.  If clearto >= the current
 * number of items, this is a (non-erroneous) NOP.
 */
static int
Pdata_clear(Pdata *self, Py_ssize_t clearto)
{
    Py_ssize_t i = Py_SIZE(self);

    assert(clearto >= self->fence);
    if (clearto >= i)
        return 0;

    while (--i >= clearto) {
        Py_CLEAR(self->data[i]);
    }
    Py_SET_SIZE(self, clearto);
    return 0;
}

static int
Pdata_grow(Pdata *self)
{
    PyObject **data = self->data;
    size_t allocated = (size_t)self->allocated;
    size_t new_allocated;

    new_allocated = (allocated >> 3) + 6;
    /* check for integer overflow */
    if (new_allocated > (size_t)PY_SSIZE_T_MAX - allocated)
        goto nomemory;
    new_allocated += allocated;
    PyMem_RESIZE(data, PyObject *, new_allocated);
    if (data == NULL)
        goto nomemory;

    self->data = data;
    self->allocated = (Py_ssize_t)new_allocated;
    return 0;

  nomemory:
    PyErr_NoMemory();
    return -1;
}

static int
Pdata_stack_underflow(PickleState *st, Pdata *self)
{
    PyErr_SetString(st->UnpicklingError,
                    self->mark_set ?
                    "unexpected MARK found" :
                    "unpickling stack underflow");
    return -1;
}

/* D is a Pdata*.  Pop the topmost element and store it into V, which
 * must be an lvalue holding PyObject*.  On stack underflow, UnpicklingError
 * is raised and V is set to NULL.
 */
static PyObject *
Pdata_pop(PickleState *state, Pdata *self)
{
    if (Py_SIZE(self) <= self->fence) {
        Pdata_stack_underflow(state, self);
        return NULL;
    }
    Py_SET_SIZE(self, Py_SIZE(self) - 1);
    return self->data[Py_SIZE(self)];
}
#define PDATA_POP(S, D, V) do { (V) = Pdata_pop(S, (D)); } while (0)

static int
Pdata_push(Pdata *self, PyObject *obj)
{
    if (Py_SIZE(self) == self->allocated && Pdata_grow(self) < 0) {
        return -1;
    }
    self->data[Py_SIZE(self)] = obj;
    Py_SET_SIZE(self, Py_SIZE(self) + 1);
    return 0;
}

/* Push an object on stack, transferring its ownership to the stack. */
#define PDATA_PUSH(D, O, ER) do {                               \
        if (Pdata_push((D), (O)) < 0) return (ER); } while(0)

/* Push an object on stack, adding a new reference to the object. */
#define PDATA_APPEND(D, O, ER) do {                             \
        Py_INCREF((O));                                         \
        if (Pdata_push((D), (O)) < 0) return (ER); } while(0)

static PyObject *
Pdata_poptuple(PickleState *state, Pdata *self, Py_ssize_t start)
{
    PyObject *tuple;
    Py_ssize_t len, i, j;

    if (start < self->fence) {
        Pdata_stack_underflow(state, self);
        return NULL;
    }
    len = Py_SIZE(self) - start;
    tuple = PyTuple_New(len);
    if (tuple == NULL)
        return NULL;
    for (i = start, j = 0; j < len; i++, j++)
        PyTuple_SET_ITEM(tuple, j, self->data[i]);

    Py_SET_SIZE(self, start);
    return tuple;
}

static PyObject *
Pdata_poplist(Pdata *self, Py_ssize_t start)
{
    PyObject *list;
    Py_ssize_t len, i, j;

    len = Py_SIZE(self) - start;
    list = PyList_New(len);
    if (list == NULL)
        return NULL;
    for (i = start, j = 0; j < len; i++, j++)
        PyList_SET_ITEM(list, j, self->data[i]);

    Py_SET_SIZE(self, start);
    return list;
}

typedef struct {
    PyObject *me_key;
    Py_ssize_t me_value;
} PyMemoEntry;

typedef struct {
    size_t mt_mask;
    size_t mt_used;
    size_t mt_allocated;
    PyMemoEntry *mt_table;
} PyMemoTable;

typedef struct PicklerObject {
    PyObject_HEAD
    PyMemoTable *memo;          /* Memo table, keep track of the seen
                                   objects to support self-referential objects
                                   pickling. */
    PyObject *persistent_id;    /* persistent_id() method, can be NULL */
    PyObject *persistent_id_attr; /* instance attribute, can be NULL */
    PyObject *dispatch_table;   /* private dispatch_table, can be NULL */
    PyObject *reducer_override; /* hook for invoking user-defined callbacks
                                   instead of save_global when pickling
                                   functions and classes*/

    PyObject *write;            /* write() method of the output stream. */
    PyObject *output_buffer;    /* Write into a local bytearray buffer before
                                   flushing to the stream. */
    Py_ssize_t output_len;      /* Length of output_buffer. */
    Py_ssize_t max_output_len;  /* Allocation size of output_buffer. */
    int proto;                  /* Pickle protocol number, >= 0 */
    int bin;                    /* Boolean, true if proto > 0 */
    int framing;                /* True when framing is enabled, proto >= 4 */
    Py_ssize_t frame_start;     /* Position in output_buffer where the
                                   current frame begins. -1 if there
                                   is no frame currently open. */

    Py_ssize_t buf_size;        /* Size of the current buffered pickle data */
    int fast;                   /* Enable fast mode if set to a true value.
                                   The fast mode disable the usage of memo,
                                   therefore speeding the pickling process by
                                   not generating superfluous PUT opcodes. It
                                   should not be used if with self-referential
                                   objects. */
    int fast_nesting;
    int fix_imports;            /* Indicate whether Pickler should fix
                                   the name of globals for Python 2.x. */
    PyObject *fast_memo;
    PyObject *buffer_callback;  /* Callback for out-of-band buffers, or NULL */
} PicklerObject;

typedef struct UnpicklerObject {
    PyObject_HEAD
    Pdata *stack;               /* Pickle data stack, store unpickled objects. */

    /* The unpickler memo is just an array of PyObject *s. Using a dict
       is unnecessary, since the keys are contiguous ints. */
    PyObject **memo;
    size_t memo_size;       /* Capacity of the memo array */
    size_t memo_len;        /* Number of objects in the memo */

    PyObject *persistent_load;  /* persistent_load() method, can be NULL. */
    PyObject *persistent_load_attr;  /* instance attribute, can be NULL. */

    Py_buffer buffer;
    char *input_buffer;
    char *input_line;
    Py_ssize_t input_len;
    Py_ssize_t next_read_idx;
    Py_ssize_t prefetched_idx;  /* index of first prefetched byte */

    PyObject *read;             /* read() method of the input stream. */
    PyObject *readinto;         /* readinto() method of the input stream. */
    PyObject *readline;         /* readline() method of the input stream. */
    PyObject *peek;             /* peek() method of the input stream, or NULL */
    PyObject *buffers;          /* iterable of out-of-band buffers, or NULL */

    char *encoding;             /* Name of the encoding to be used for
                                   decoding strings pickled using Python
                                   2.x. The default value is "ASCII" */
    char *errors;               /* Name of errors handling scheme to used when
                                   decoding strings. The default value is
                                   "strict". */
    Py_ssize_t *marks;          /* Mark stack, used for unpickling container
                                   objects. */
    Py_ssize_t num_marks;       /* Number of marks in the mark stack. */
    Py_ssize_t marks_size;      /* Current allocated size of the mark stack. */
    int proto;                  /* Protocol of the pickle loaded. */
    int fix_imports;            /* Indicate whether Unpickler should fix
                                   the name of globals pickled by Python 2.x. */
} UnpicklerObject;

typedef struct {
    PyObject_HEAD
    PicklerObject *pickler; /* Pickler whose memo table we're proxying. */
}  PicklerMemoProxyObject;

typedef struct {
    PyObject_HEAD
    UnpicklerObject *unpickler;
} UnpicklerMemoProxyObject;

#define PicklerObject_CAST(op)              ((PicklerObject *)(op))
#define UnpicklerObject_CAST(op)            ((UnpicklerObject *)(op))
#define PicklerMemoProxyObject_CAST(op)     ((PicklerMemoProxyObject *)(op))
#define UnpicklerMemoProxyObject_CAST(op)   ((UnpicklerMemoProxyObject *)(op))

/* Forward declarations */
static int save(PickleState *state, PicklerObject *, PyObject *, int);
static int save_reduce(PickleState *, PicklerObject *, PyObject *, PyObject *);

#include "clinic/_pickle.c.h"

/*************************************************************************
 A custom hashtable mapping void* to Python ints. This is used by the pickler
 for memoization. Using a custom hashtable rather than PyDict allows us to skip
 a bunch of unnecessary object creation. This makes a huge performance
 difference. */

#define MT_MINSIZE 8
#define PERTURB_SHIFT 5


static PyMemoTable *
PyMemoTable_New(void)
{
    PyMemoTable *memo = PyMem_Malloc(sizeof(PyMemoTable));
    if (memo == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    memo->mt_used = 0;
    memo->mt_allocated = MT_MINSIZE;
    memo->mt_mask = MT_MINSIZE - 1;
    memo->mt_table = PyMem_Malloc(MT_MINSIZE * sizeof(PyMemoEntry));
    if (memo->mt_table == NULL) {
        PyMem_Free(memo);
        PyErr_NoMemory();
        return NULL;
    }
    memset(memo->mt_table, 0, MT_MINSIZE * sizeof(PyMemoEntry));

    return memo;
}

static PyMemoTable *
PyMemoTable_Copy(PyMemoTable *self)
{
    PyMemoTable *new = PyMemoTable_New();
    if (new == NULL)
        return NULL;

    new->mt_used = self->mt_used;
    new->mt_allocated = self->mt_allocated;
    new->mt_mask = self->mt_mask;
    /* The table we get from _New() is probably smaller than we wanted.
       Free it and allocate one that's the right size. */
    PyMem_Free(new->mt_table);
    new->mt_table = PyMem_NEW(PyMemoEntry, self->mt_allocated);
    if (new->mt_table == NULL) {
        PyMem_Free(new);
        PyErr_NoMemory();
        return NULL;
    }
    for (size_t i = 0; i < self->mt_allocated; i++) {
        Py_XINCREF(self->mt_table[i].me_key);
    }
    memcpy(new->mt_table, self->mt_table,
           sizeof(PyMemoEntry) * self->mt_allocated);

    return new;
}

static Py_ssize_t
PyMemoTable_Size(PyMemoTable *self)
{
    return self->mt_used;
}

static int
PyMemoTable_Clear(PyMemoTable *self)
{
    Py_ssize_t i = self->mt_allocated;

    while (--i >= 0) {
        Py_XDECREF(self->mt_table[i].me_key);
    }
    self->mt_used = 0;
    memset(self->mt_table, 0, self->mt_allocated * sizeof(PyMemoEntry));
    return 0;
}

static void
PyMemoTable_Del(PyMemoTable *self)
{
    if (self == NULL)
        return;
    PyMemoTable_Clear(self);

    PyMem_Free(self->mt_table);
    PyMem_Free(self);
}

/* Since entries cannot be deleted from this hashtable, _PyMemoTable_Lookup()
   can be considerably simpler than dictobject.c's lookdict(). */
static PyMemoEntry *
_PyMemoTable_Lookup(PyMemoTable *self, PyObject *key)
{
    size_t i;
    size_t perturb;
    size_t mask = self->mt_mask;
    PyMemoEntry *table = self->mt_table;
    PyMemoEntry *entry;
    Py_hash_t hash = (Py_hash_t)key >> 3;

    i = hash & mask;
    entry = &table[i];
    if (entry->me_key == NULL || entry->me_key == key)
        return entry;

    for (perturb = hash; ; perturb >>= PERTURB_SHIFT) {
        i = (i << 2) + i + perturb + 1;
        entry = &table[i & mask];
        if (entry->me_key == NULL || entry->me_key == key)
            return entry;
    }
    Py_UNREACHABLE();
}

/* Returns -1 on failure, 0 on success. */
static int
_PyMemoTable_ResizeTable(PyMemoTable *self, size_t min_size)
{
    PyMemoEntry *oldtable = NULL;
    PyMemoEntry *oldentry, *newentry;
    size_t new_size = MT_MINSIZE;
    size_t to_process;

    assert(min_size > 0);

    if (min_size > PY_SSIZE_T_MAX) {
        PyErr_NoMemory();
        return -1;
    }

    /* Find the smallest valid table size >= min_size. */
    while (new_size < min_size) {
        new_size <<= 1;
    }
    /* new_size needs to be a power of two. */
    assert((new_size & (new_size - 1)) == 0);

    /* Allocate new table. */
    oldtable = self->mt_table;
    self->mt_table = PyMem_NEW(PyMemoEntry, new_size);
    if (self->mt_table == NULL) {
        self->mt_table = oldtable;
        PyErr_NoMemory();
        return -1;
    }
    self->mt_allocated = new_size;
    self->mt_mask = new_size - 1;
    memset(self->mt_table, 0, sizeof(PyMemoEntry) * new_size);

    /* Copy entries from the old table. */
    to_process = self->mt_used;
    for (oldentry = oldtable; to_process > 0; oldentry++) {
        if (oldentry->me_key != NULL) {
            to_process--;
            /* newentry is a pointer to a chunk of the new
               mt_table, so we're setting the key:value pair
               in-place. */
            newentry = _PyMemoTable_Lookup(self, oldentry->me_key);
            newentry->me_key = oldentry->me_key;
            newentry->me_value = oldentry->me_value;
        }
    }

    /* Deallocate the old table. */
    PyMem_Free(oldtable);
    return 0;
}

/* Returns NULL on failure, a pointer to the value otherwise. */
static Py_ssize_t *
PyMemoTable_Get(PyMemoTable *self, PyObject *key)
{
    PyMemoEntry *entry = _PyMemoTable_Lookup(self, key);
    if (entry->me_key == NULL)
        return NULL;
    return &entry->me_value;
}

/* Returns -1 on failure, 0 on success. */
static int
PyMemoTable_Set(PyMemoTable *self, PyObject *key, Py_ssize_t value)
{
    PyMemoEntry *entry;

    assert(key != NULL);

    entry = _PyMemoTable_Lookup(self, key);
    if (entry->me_key != NULL) {
        entry->me_value = value;
        return 0;
    }
    entry->me_key = Py_NewRef(key);
    entry->me_value = value;
    self->mt_used++;

    /* If we added a key, we can safely resize. Otherwise just return!
     * If used >= 2/3 size, adjust size. Normally, this quaduples the size.
     *
     * Quadrupling the size improves average table sparseness
     * (reducing collisions) at the cost of some memory. It also halves
     * the number of expensive resize operations in a growing memo table.
     *
     * Very large memo tables (over 50K items) use doubling instead.
     * This may help applications with severe memory constraints.
     */
    if (SIZE_MAX / 3 >= self->mt_used && self->mt_used * 3 < self->mt_allocated * 2) {
        return 0;
    }
    // self->mt_used is always < PY_SSIZE_T_MAX, so this can't overflow.
    size_t desired_size = (self->mt_used > 50000 ? 2 : 4) * self->mt_used;
    return _PyMemoTable_ResizeTable(self, desired_size);
}

#undef MT_MINSIZE
#undef PERTURB_SHIFT

/*************************************************************************/


static int
_Pickler_ClearBuffer(PicklerObject *self)
{
    Py_XSETREF(self->output_buffer,
              PyBytes_FromStringAndSize(NULL, self->max_output_len));
    if (self->output_buffer == NULL)
        return -1;
    self->output_len = 0;
    self->frame_start = -1;
    return 0;
}

static void
_write_size64(char *out, size_t value)
{
    size_t i;

    static_assert(sizeof(size_t) <= 8, "size_t is larger than 64-bit");

    for (i = 0; i < sizeof(size_t); i++) {
        out[i] = (unsigned char)((value >> (8 * i)) & 0xff);
    }
    for (i = sizeof(size_t); i < 8; i++) {
        out[i] = 0;
    }
}

static int
_Pickler_CommitFrame(PicklerObject *self)
{
    size_t frame_len;
    char *qdata;

    if (!self->framing || self->frame_start == -1)
        return 0;
    frame_len = self->output_len - self->frame_start - FRAME_HEADER_SIZE;
    qdata = PyBytes_AS_STRING(self->output_buffer) + self->frame_start;
    if (frame_len >= FRAME_SIZE_MIN) {
        qdata[0] = FRAME;
        _write_size64(qdata + 1, frame_len);
    }
    else {
        memmove(qdata, qdata + FRAME_HEADER_SIZE, frame_len);
        self->output_len -= FRAME_HEADER_SIZE;
    }
    self->frame_start = -1;
    return 0;
}

static PyObject *
_Pickler_GetString(PicklerObject *self)
{
    PyObject *output_buffer = self->output_buffer;

    assert(self->output_buffer != NULL);

    if (_Pickler_CommitFrame(self))
        return NULL;

    self->output_buffer = NULL;
    /* Resize down to exact size */
    if (_PyBytes_Resize(&output_buffer, self->output_len) < 0)
        return NULL;
    return output_buffer;
}

static int
_Pickler_FlushToFile(PicklerObject *self)
{
    PyObject *output, *result;

    assert(self->write != NULL);

    /* This will commit the frame first */
    output = _Pickler_GetString(self);
    if (output == NULL)
        return -1;

    result = _Pickle_FastCall(self->write, output);
    Py_XDECREF(result);
    return (result == NULL) ? -1 : 0;
}

static int
_Pickler_OpcodeBoundary(PicklerObject *self)
{
    Py_ssize_t frame_len;

    if (!self->framing || self->frame_start == -1) {
        return 0;
    }
    frame_len = self->output_len - self->frame_start - FRAME_HEADER_SIZE;
    if (frame_len >= FRAME_SIZE_TARGET) {
        if(_Pickler_CommitFrame(self)) {
            return -1;
        }
        /* Flush the content of the committed frame to the underlying
         * file and reuse the pickler buffer for the next frame so as
         * to limit memory usage when dumping large complex objects to
         * a file.
         *
         * self->write is NULL when called via dumps.
         */
        if (self->write != NULL) {
            if (_Pickler_FlushToFile(self) < 0) {
                return -1;
            }
            if (_Pickler_ClearBuffer(self) < 0) {
                return -1;
            }
        }
    }
    return 0;
}

static Py_ssize_t
_Pickler_Write(PicklerObject *self, const char *s, Py_ssize_t data_len)
{
    Py_ssize_t i, n, required;
    char *buffer;
    int need_new_frame;

    assert(s != NULL);
    need_new_frame = (self->framing && self->frame_start == -1);

    if (need_new_frame)
        n = data_len + FRAME_HEADER_SIZE;
    else
        n = data_len;

    required = self->output_len + n;
    if (required > self->max_output_len) {
        /* Make place in buffer for the pickle chunk */
        if (self->output_len >= PY_SSIZE_T_MAX / 2 - n) {
            PyErr_NoMemory();
            return -1;
        }
        self->max_output_len = (self->output_len + n) / 2 * 3;
        if (_PyBytes_Resize(&self->output_buffer, self->max_output_len) < 0)
            return -1;
    }
    buffer = PyBytes_AS_STRING(self->output_buffer);
    if (need_new_frame) {
        /* Setup new frame */
        Py_ssize_t frame_start = self->output_len;
        self->frame_start = frame_start;
        for (i = 0; i < FRAME_HEADER_SIZE; i++) {
            /* Write an invalid value, for debugging */
            buffer[frame_start + i] = 0xFE;
        }
        self->output_len += FRAME_HEADER_SIZE;
    }
    if (data_len < 8) {
        /* This is faster than memcpy when the string is short. */
        for (i = 0; i < data_len; i++) {
            buffer[self->output_len + i] = s[i];
        }
    }
    else {
        memcpy(buffer + self->output_len, s, data_len);
    }
    self->output_len += data_len;
    return data_len;
}

static PicklerObject *
_Pickler_New(PickleState *st)
{
    PyMemoTable *memo = PyMemoTable_New();
    if (memo == NULL) {
        return NULL;
    }

    const Py_ssize_t max_output_len = WRITE_BUF_SIZE;
    PyObject *output_buffer = PyBytes_FromStringAndSize(NULL, max_output_len);
    if (output_buffer == NULL) {
        goto error;
    }

    PicklerObject *self = PyObject_GC_New(PicklerObject, st->Pickler_Type);
    if (self == NULL) {
        goto error;
    }

    self->memo = memo;
    self->persistent_id = NULL;
    self->persistent_id_attr = NULL;
    self->dispatch_table = NULL;
    self->reducer_override = NULL;
    self->write = NULL;
    self->output_buffer = output_buffer;
    self->output_len = 0;
    self->max_output_len = max_output_len;
    self->proto = 0;
    self->bin = 0;
    self->framing = 0;
    self->frame_start = -1;
    self->buf_size = 0;
    self->fast = 0;
    self->fast_nesting = 0;
    self->fix_imports = 0;
    self->fast_memo = NULL;
    self->buffer_callback = NULL;

    PyObject_GC_Track(self);
    return self;

error:
    PyMem_Free(memo);
    Py_XDECREF(output_buffer);
    return NULL;
}

static int
_Pickler_SetProtocol(PicklerObject *self, PyObject *protocol, int fix_imports)
{
    long proto;

    if (protocol == Py_None) {
        proto = DEFAULT_PROTOCOL;
    }
    else {
        proto = PyLong_AsLong(protocol);
        if (proto < 0) {
            if (proto == -1 && PyErr_Occurred())
                return -1;
            proto = HIGHEST_PROTOCOL;
        }
        else if (proto > HIGHEST_PROTOCOL) {
            PyErr_Format(PyExc_ValueError, "pickle protocol must be <= %d",
                         HIGHEST_PROTOCOL);
            return -1;
        }
    }
    self->proto = (int)proto;
    self->bin = proto > 0;
    self->fix_imports = fix_imports && proto < 3;
    return 0;
}

/* Returns -1 (with an exception set) on failure, 0 on success. This may
   be called once on a freshly created Pickler. */
static int
_Pickler_SetOutputStream(PicklerObject *self, PyObject *file)
{
    assert(file != NULL);
    if (PyObject_GetOptionalAttr(file, &_Py_ID(write), &self->write) < 0) {
        return -1;
    }
    if (self->write == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "file must have a 'write' attribute");
        return -1;
    }

    return 0;
}

static int
_Pickler_SetBufferCallback(PicklerObject *self, PyObject *buffer_callback)
{
    if (buffer_callback == Py_None) {
        buffer_callback = NULL;
    }
    if (buffer_callback != NULL && self->proto < 5) {
        PyErr_SetString(PyExc_ValueError,
                        "buffer_callback needs protocol >= 5");
        return -1;
    }

    self->buffer_callback = Py_XNewRef(buffer_callback);
    return 0;
}

/* Returns the size of the input on success, -1 on failure. This takes its
   own reference to `input`. */
static Py_ssize_t
_Unpickler_SetStringInput(UnpicklerObject *self, PyObject *input)
{
    if (self->buffer.buf != NULL)
        PyBuffer_Release(&self->buffer);
    if (PyObject_GetBuffer(input, &self->buffer, PyBUF_CONTIG_RO) < 0)
        return -1;
    self->input_buffer = self->buffer.buf;
    self->input_len = self->buffer.len;
    self->next_read_idx = 0;
    self->prefetched_idx = self->input_len;
    return self->input_len;
}

static int
bad_readline(PickleState *st)
{
    PyErr_SetString(st->UnpicklingError, "pickle data was truncated");
    return -1;
}

/* Skip any consumed data that was only prefetched using peek() */
static int
_Unpickler_SkipConsumed(UnpicklerObject *self)
{
    Py_ssize_t consumed;
    PyObject *r;

    consumed = self->next_read_idx - self->prefetched_idx;
    if (consumed <= 0)
        return 0;

    assert(self->peek);  /* otherwise we did something wrong */
    /* This makes a useless copy... */
    r = PyObject_CallFunction(self->read, "n", consumed);
    if (r == NULL)
        return -1;
    Py_DECREF(r);

    self->prefetched_idx = self->next_read_idx;
    return 0;
}

static const Py_ssize_t READ_WHOLE_LINE = -1;

/* If reading from a file, we need to only pull the bytes we need, since there
   may be multiple pickle objects arranged contiguously in the same input
   buffer.

   If `n` is READ_WHOLE_LINE, read a whole line. Otherwise, read up to `n`
   bytes from the input stream/buffer.

   Update the unpickler's input buffer with the newly-read data. Returns -1 on
   failure; on success, returns the number of bytes read from the file.

   On success, self->input_len will be 0; this is intentional so that when
   unpickling from a file, the "we've run out of data" code paths will trigger,
   causing the Unpickler to go back to the file for more data. Use the returned
   size to tell you how much data you can process. */
static Py_ssize_t
_Unpickler_ReadFromFile(UnpicklerObject *self, Py_ssize_t n)
{
    PyObject *data;
    Py_ssize_t read_size;

    assert(self->read != NULL);

    if (_Unpickler_SkipConsumed(self) < 0)
        return -1;

    if (n == READ_WHOLE_LINE) {
        data = PyObject_CallNoArgs(self->readline);
    }
    else {
        PyObject *len;
        /* Prefetch some data without advancing the file pointer, if possible */
        if (self->peek && n < PREFETCH) {
            len = PyLong_FromSsize_t(PREFETCH);
            if (len == NULL)
                return -1;
            data = _Pickle_FastCall(self->peek, len);
            if (data == NULL) {
                if (!PyErr_ExceptionMatches(PyExc_NotImplementedError))
                    return -1;
                /* peek() is probably not supported by the given file object */
                PyErr_Clear();
                Py_CLEAR(self->peek);
            }
            else {
                read_size = _Unpickler_SetStringInput(self, data);
                Py_DECREF(data);
                if (read_size < 0) {
                    return -1;
                }

                self->prefetched_idx = 0;
                if (n <= read_size)
                    return n;
            }
        }
        len = PyLong_FromSsize_t(n);
        if (len == NULL)
            return -1;
        data = _Pickle_FastCall(self->read, len);
    }
    if (data == NULL)
        return -1;

    read_size = _Unpickler_SetStringInput(self, data);
    Py_DECREF(data);
    return read_size;
}

/* Don't call it directly: use _Unpickler_Read() */
static Py_ssize_t
_Unpickler_ReadImpl(UnpicklerObject *self, PickleState *st, char **s, Py_ssize_t n)
{
    Py_ssize_t num_read;

    *s = NULL;
    if (self->next_read_idx > PY_SSIZE_T_MAX - n) {
        PyErr_SetString(st->UnpicklingError,
                        "read would overflow (invalid bytecode)");
        return -1;
    }

    /* This case is handled by the _Unpickler_Read() macro for efficiency */
    assert(self->next_read_idx + n > self->input_len);

    if (!self->read)
        return bad_readline(st);

    /* Extend the buffer to satisfy desired size */
    num_read = _Unpickler_ReadFromFile(self, n);
    if (num_read < 0)
        return -1;
    if (num_read < n)
        return bad_readline(st);
    *s = self->input_buffer;
    self->next_read_idx = n;
    return n;
}

/* Read `n` bytes from the unpickler's data source, storing the result in `buf`.
 *
 * This should only be used for non-small data reads where potentially
 * avoiding a copy is beneficial.  This method does not try to prefetch
 * more data into the input buffer.
 *
 * _Unpickler_Read() is recommended in most cases.
 */
static Py_ssize_t
_Unpickler_ReadInto(PickleState *state, UnpicklerObject *self, char *buf,
                    Py_ssize_t n)
{
    assert(n != READ_WHOLE_LINE);

    /* Read from available buffer data, if any */
    Py_ssize_t in_buffer = self->input_len - self->next_read_idx;
    if (in_buffer > 0) {
        Py_ssize_t to_read = Py_MIN(in_buffer, n);
        memcpy(buf, self->input_buffer + self->next_read_idx, to_read);
        self->next_read_idx += to_read;
        buf += to_read;
        n -= to_read;
        if (n == 0) {
            /* Entire read was satisfied from buffer */
            return n;
        }
    }

    /* Read from file */
    if (!self->read) {
        /* We're unpickling memory, this means the input is truncated */
        return bad_readline(state);
    }
    if (_Unpickler_SkipConsumed(self) < 0) {
        return -1;
    }

    if (!self->readinto) {
        /* readinto() not supported on file-like object, fall back to read()
         * and copy into destination buffer (bpo-39681) */
        PyObject* len = PyLong_FromSsize_t(n);
        if (len == NULL) {
            return -1;
        }
        PyObject* data = _Pickle_FastCall(self->read, len);
        if (data == NULL) {
            return -1;
        }
        if (!PyBytes_Check(data)) {
            PyErr_Format(PyExc_ValueError,
                         "read() returned non-bytes object (%R)",
                         Py_TYPE(data));
            Py_DECREF(data);
            return -1;
        }
        Py_ssize_t read_size = PyBytes_GET_SIZE(data);
        if (read_size < n) {
            Py_DECREF(data);
            return bad_readline(state);
        }
        memcpy(buf, PyBytes_AS_STRING(data), n);
        Py_DECREF(data);
        return n;
    }

    /* Call readinto() into user buffer */
    PyObject *buf_obj = PyMemoryView_FromMemory(buf, n, PyBUF_WRITE);
    if (buf_obj == NULL) {
        return -1;
    }
    PyObject *read_size_obj = _Pickle_FastCall(self->readinto, buf_obj);
    if (read_size_obj == NULL) {
        return -1;
    }
    Py_ssize_t read_size = PyLong_AsSsize_t(read_size_obj);
    Py_DECREF(read_size_obj);

    if (read_size < 0) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_ValueError,
                            "readinto() returned negative size");
        }
        return -1;
    }
    if (read_size < n) {
        return bad_readline(state);
    }
    return n;
}

/* Read `n` bytes from the unpickler's data source, storing the result in `*s`.

   This should be used for all data reads, rather than accessing the unpickler's
   input buffer directly. This method deals correctly with reading from input
   streams, which the input buffer doesn't deal with.

   Note that when reading from a file-like object, self->next_read_idx won't
   be updated (it should remain at 0 for the entire unpickling process). You
   should use this function's return value to know how many bytes you can
   consume.

   Returns -1 (with an exception set) on failure. On success, return the
   number of chars read. */
#define _Unpickler_Read(self, state, s, n) \
    (((n) <= (self)->input_len - (self)->next_read_idx)      \
     ? (*(s) = (self)->input_buffer + (self)->next_read_idx, \
        (self)->next_read_idx += (n),                        \
        (n))                                                 \
     : _Unpickler_ReadImpl(self, state, (s), (n)))

static Py_ssize_t
_Unpickler_CopyLine(UnpicklerObject *self, char *line, Py_ssize_t len,
                    char **result)
{
    char *input_line = PyMem_Realloc(self->input_line, len + 1);
    if (input_line == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    memcpy(input_line, line, len);
    input_line[len] = '\0';
    self->input_line = input_line;
    *result = self->input_line;
    return len;
}

/* Read a line from the input stream/buffer. If we run off the end of the input
   before hitting \n, raise an error.

   Returns the number of chars read, or -1 on failure. */
static Py_ssize_t
_Unpickler_Readline(PickleState *state, UnpicklerObject *self, char **result)
{
    Py_ssize_t i, num_read;

    for (i = self->next_read_idx; i < self->input_len; i++) {
        if (self->input_buffer[i] == '\n') {
            char *line_start = self->input_buffer + self->next_read_idx;
            num_read = i - self->next_read_idx + 1;
            self->next_read_idx = i + 1;
            return _Unpickler_CopyLine(self, line_start, num_read, result);
        }
    }
    if (!self->read)
        return bad_readline(state);

    num_read = _Unpickler_ReadFromFile(self, READ_WHOLE_LINE);
    if (num_read < 0)
        return -1;
    if (num_read == 0 || self->input_buffer[num_read - 1] != '\n')
        return bad_readline(state);
    self->next_read_idx = num_read;
    return _Unpickler_CopyLine(self, self->input_buffer, num_read, result);
}

/* Returns -1 (with an exception set) on failure, 0 on success. The memo array
   will be modified in place. */
static int
_Unpickler_ResizeMemoList(UnpicklerObject *self, size_t new_size)
{
    size_t i;

    assert(new_size > self->memo_size);

    PyObject **memo_new = self->memo;
    PyMem_RESIZE(memo_new, PyObject *, new_size);
    if (memo_new == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    self->memo = memo_new;
    for (i = self->memo_size; i < new_size; i++)
        self->memo[i] = NULL;
    self->memo_size = new_size;
    return 0;
}

/* Returns NULL if idx is out of bounds. */
static PyObject *
_Unpickler_MemoGet(UnpicklerObject *self, size_t idx)
{
    if (idx >= self->memo_size)
        return NULL;

    return self->memo[idx];
}

/* Returns -1 (with an exception set) on failure, 0 on success.
   This takes its own reference to `value`. */
static int
_Unpickler_MemoPut(UnpicklerObject *self, size_t idx, PyObject *value)
{
    PyObject *old_item;

    if (idx >= self->memo_size) {
        if (_Unpickler_ResizeMemoList(self, idx * 2) < 0)
            return -1;
        assert(idx < self->memo_size);
    }
    old_item = self->memo[idx];
    self->memo[idx] = Py_NewRef(value);
    if (old_item != NULL) {
        Py_DECREF(old_item);
    }
    else {
        self->memo_len++;
    }
    return 0;
}

static PyObject **
_Unpickler_NewMemo(Py_ssize_t new_size)
{
    PyObject **memo = PyMem_NEW(PyObject *, new_size);
    if (memo == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    memset(memo, 0, new_size * sizeof(PyObject *));
    return memo;
}

/* Free the unpickler's memo, taking care to decref any items left in it. */
static void
_Unpickler_MemoCleanup(UnpicklerObject *self)
{
    Py_ssize_t i;
    PyObject **memo = self->memo;

    if (self->memo == NULL)
        return;
    self->memo = NULL;
    i = self->memo_size;
    while (--i >= 0) {
        Py_XDECREF(memo[i]);
    }
    PyMem_Free(memo);
}

static UnpicklerObject *
_Unpickler_New(PyObject *module)
{
    const int MEMO_SIZE = 32;
    PyObject **memo = _Unpickler_NewMemo(MEMO_SIZE);
    if (memo == NULL) {
        return NULL;
    }

    PickleState *st = _Pickle_GetState(module);
    PyObject *stack = Pdata_New(st);
    if (stack == NULL) {
        goto error;
    }

    UnpicklerObject *self = PyObject_GC_New(UnpicklerObject,
                                            st->Unpickler_Type);
    if (self == NULL) {
        goto error;
    }

    self->stack = (Pdata *)stack;
    self->memo = memo;
    self->memo_size = MEMO_SIZE;
    self->memo_len = 0;
    self->persistent_load = NULL;
    self->persistent_load_attr = NULL;
    memset(&self->buffer, 0, sizeof(Py_buffer));
    self->input_buffer = NULL;
    self->input_line = NULL;
    self->input_len = 0;
    self->next_read_idx = 0;
    self->prefetched_idx = 0;
    self->read = NULL;
    self->readinto = NULL;
    self->readline = NULL;
    self->peek = NULL;
    self->buffers = NULL;
    self->encoding = NULL;
    self->errors = NULL;
    self->marks = NULL;
    self->num_marks = 0;
    self->marks_size = 0;
    self->proto = 0;
    self->fix_imports = 0;

    PyObject_GC_Track(self);
    return self;

error:
    PyMem_Free(memo);
    Py_XDECREF(stack);
    return NULL;
}

/* Returns -1 (with an exception set) on failure, 0 on success. This may
   be called once on a freshly created Unpickler. */
static int
_Unpickler_SetInputStream(UnpicklerObject *self, PyObject *file)
{
    /* Optional file methods */
    if (PyObject_GetOptionalAttr(file, &_Py_ID(peek), &self->peek) < 0) {
        goto error;
    }
    if (PyObject_GetOptionalAttr(file, &_Py_ID(readinto), &self->readinto) < 0) {
        goto error;
    }
    if (PyObject_GetOptionalAttr(file, &_Py_ID(read), &self->read) < 0) {
        goto error;
    }
    if (PyObject_GetOptionalAttr(file, &_Py_ID(readline), &self->readline) < 0) {
        goto error;
    }
    if (!self->readline || !self->read) {
        PyErr_SetString(PyExc_TypeError,
                        "file must have 'read' and 'readline' attributes");
        goto error;
    }
    return 0;

error:
    Py_CLEAR(self->read);
    Py_CLEAR(self->readinto);
    Py_CLEAR(self->readline);
    Py_CLEAR(self->peek);
    return -1;
}

/* Returns -1 (with an exception set) on failure, 0 on success. This may
   be called once on a freshly created Unpickler. */
static int
_Unpickler_SetInputEncoding(UnpicklerObject *self,
                            const char *encoding,
                            const char *errors)
{
    if (encoding == NULL)
        encoding = "ASCII";
    if (errors == NULL)
        errors = "strict";

    self->encoding = _PyMem_Strdup(encoding);
    self->errors = _PyMem_Strdup(errors);
    if (self->encoding == NULL || self->errors == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    return 0;
}

/* Returns -1 (with an exception set) on failure, 0 on success. This may
   be called once on a freshly created Unpickler. */
static int
_Unpickler_SetBuffers(UnpicklerObject *self, PyObject *buffers)
{
    if (buffers == NULL || buffers == Py_None) {
        self->buffers = NULL;
    }
    else {
        self->buffers = PyObject_GetIter(buffers);
        if (self->buffers == NULL) {
            return -1;
        }
    }
    return 0;
}

/* Generate a GET opcode for an object stored in the memo. */
static int
memo_get(PickleState *st, PicklerObject *self, PyObject *key)
{
    Py_ssize_t *value;
    char pdata[30];
    Py_ssize_t len;

    value = PyMemoTable_Get(self->memo, key);
    if (value == NULL)  {
        PyErr_SetObject(PyExc_KeyError, key);
        return -1;
    }

    if (!self->bin) {
        pdata[0] = GET;
        PyOS_snprintf(pdata + 1, sizeof(pdata) - 1,
                      "%zd\n", *value);
        len = strlen(pdata);
    }
    else {
        if (*value < 256) {
            pdata[0] = BINGET;
            pdata[1] = (unsigned char)(*value & 0xff);
            len = 2;
        }
        else if ((size_t)*value <= 0xffffffffUL) {
            pdata[0] = LONG_BINGET;
            pdata[1] = (unsigned char)(*value & 0xff);
            pdata[2] = (unsigned char)((*value >> 8) & 0xff);
            pdata[3] = (unsigned char)((*value >> 16) & 0xff);
            pdata[4] = (unsigned char)((*value >> 24) & 0xff);
            len = 5;
        }
        else { /* unlikely */
            PyErr_SetString(st->PicklingError,
                            "memo id too large for LONG_BINGET");
            return -1;
        }
    }

    if (_Pickler_Write(self, pdata, len) < 0)
        return -1;

    return 0;
}

/* Store an object in the memo, assign it a new unique ID based on the number
   of objects currently stored in the memo and generate a PUT opcode. */
static int
memo_put(PickleState *st, PicklerObject *self, PyObject *obj)
{
    char pdata[30];
    Py_ssize_t len;
    Py_ssize_t idx;

    const char memoize_op = MEMOIZE;

    if (self->fast)
        return 0;

    idx = PyMemoTable_Size(self->memo);
    if (PyMemoTable_Set(self->memo, obj, idx) < 0)
        return -1;

    if (self->proto >= 4) {
        if (_Pickler_Write(self, &memoize_op, 1) < 0)
            return -1;
        return 0;
    }
    else if (!self->bin) {
        pdata[0] = PUT;
        PyOS_snprintf(pdata + 1, sizeof(pdata) - 1,
                      "%zd\n", idx);
        len = strlen(pdata);
    }
    else {
        if (idx < 256) {
            pdata[0] = BINPUT;
            pdata[1] = (unsigned char)idx;
            len = 2;
        }
        else if ((size_t)idx <= 0xffffffffUL) {
            pdata[0] = LONG_BINPUT;
            pdata[1] = (unsigned char)(idx & 0xff);
            pdata[2] = (unsigned char)((idx >> 8) & 0xff);
            pdata[3] = (unsigned char)((idx >> 16) & 0xff);
            pdata[4] = (unsigned char)((idx >> 24) & 0xff);
            len = 5;
        }
        else { /* unlikely */
            PyErr_SetString(st->PicklingError,
                            "memo id too large for LONG_BINPUT");
            return -1;
        }
    }
    if (_Pickler_Write(self, pdata, len) < 0)
        return -1;

    return 0;
}

static PyObject *
get_dotted_path(PyObject *name)
{
    return PyUnicode_Split(name, _Py_LATIN1_CHR('.'), -1);
}

static int
check_dotted_path(PickleState *st, PyObject *obj, PyObject *dotted_path)
{
    Py_ssize_t i, n;
    n = PyList_GET_SIZE(dotted_path);
    assert(n >= 1);
    for (i = 0; i < n; i++) {
        PyObject *subpath = PyList_GET_ITEM(dotted_path, i);
        if (_PyUnicode_EqualToASCIIString(subpath, "<locals>")) {
            PyErr_Format(st->PicklingError,
                         "Can't pickle local object %R", obj);
            return -1;
        }
    }
    return 0;
}

static PyObject *
getattribute(PyObject *obj, PyObject *names, int raises)
{
    Py_ssize_t i, n;

    assert(PyList_CheckExact(names));
    Py_INCREF(obj);
    n = PyList_GET_SIZE(names);
    for (i = 0; i < n; i++) {
        PyObject *name = PyList_GET_ITEM(names, i);
        PyObject *parent = obj;
        if (raises) {
            obj = PyObject_GetAttr(parent, name);
        }
        else {
            (void)PyObject_GetOptionalAttr(parent, name, &obj);
        }
        Py_DECREF(parent);
        if (obj == NULL) {
            return NULL;
        }
    }
    return obj;
}

static int
_checkmodule(PyObject *module_name, PyObject *module,
             PyObject *global, PyObject *dotted_path)
{
    if (module == Py_None) {
        return -1;
    }
    if (PyUnicode_Check(module_name) &&
            _PyUnicode_EqualToASCIIString(module_name, "__main__")) {
        return -1;
    }
    if (PyUnicode_Check(module_name) &&
            _PyUnicode_EqualToASCIIString(module_name, "__mp_main__")) {
        return -1;
    }

    PyObject *candidate = getattribute(module, dotted_path, 0);
    if (candidate == NULL) {
        return -1;
    }
    if (candidate != global) {
        Py_DECREF(candidate);
        return -1;
    }
    Py_DECREF(candidate);
    return 0;
}

static PyObject *
whichmodule(PickleState *st, PyObject *global, PyObject *global_name, PyObject *dotted_path)
{
    PyObject *module_name;
    PyObject *module = NULL;
    Py_ssize_t i;
    PyObject *modules;

    if (check_dotted_path(st, global, dotted_path) < 0) {
        return NULL;
    }
    if (PyObject_GetOptionalAttr(global, &_Py_ID(__module__), &module_name) < 0) {
        return NULL;
    }
    if (module_name == NULL || module_name == Py_None) {
        /* In some rare cases (e.g., bound methods of extension types),
           __module__ can be None. If it is so, then search sys.modules for
           the module of global. */
        Py_CLEAR(module_name);
        modules = PySys_GetAttr(&_Py_ID(modules));
        if (modules == NULL) {
            return NULL;
        }
        if (PyDict_CheckExact(modules)) {
            i = 0;
            while (PyDict_Next(modules, &i, &module_name, &module)) {
                Py_INCREF(module_name);
                Py_INCREF(module);
                if (_checkmodule(module_name, module, global, dotted_path) == 0) {
                    Py_DECREF(module);
                    Py_DECREF(modules);
                    return module_name;
                }
                Py_DECREF(module);
                Py_DECREF(module_name);
                if (PyErr_Occurred()) {
                    Py_DECREF(modules);
                    return NULL;
                }
            }
        }
        else {
            PyObject *iterator = PyObject_GetIter(modules);
            if (iterator == NULL) {
                Py_DECREF(modules);
                return NULL;
            }
            while ((module_name = PyIter_Next(iterator))) {
                module = PyObject_GetItem(modules, module_name);
                if (module == NULL) {
                    Py_DECREF(module_name);
                    Py_DECREF(iterator);
                    Py_DECREF(modules);
                    return NULL;
                }
                if (_checkmodule(module_name, module, global, dotted_path) == 0) {
                    Py_DECREF(module);
                    Py_DECREF(iterator);
                    Py_DECREF(modules);
                    return module_name;
                }
                Py_DECREF(module);
                Py_DECREF(module_name);
                if (PyErr_Occurred()) {
                    Py_DECREF(iterator);
                    Py_DECREF(modules);
                    return NULL;
                }
            }
            Py_DECREF(iterator);
        }
        Py_DECREF(modules);
        if (PyErr_Occurred()) {
            return NULL;
        }

        /* If no module is found, use __main__. */
        module_name = Py_NewRef(&_Py_ID(__main__));
    }

    /* XXX: Change to use the import C API directly with level=0 to disallow
       relative imports.

       XXX: PyImport_ImportModuleLevel could be used. However, this bypasses
       builtins.__import__. Therefore, _pickle, unlike pickle.py, will ignore
       custom import functions (IMHO, this would be a nice security
       feature). The import C API would need to be extended to support the
       extra parameters of __import__ to fix that. */
    module = PyImport_Import(module_name);
    if (module == NULL) {
        if (PyErr_ExceptionMatches(PyExc_ImportError) ||
            PyErr_ExceptionMatches(PyExc_ValueError))
        {
            PyObject *exc = PyErr_GetRaisedException();
            PyErr_Format(st->PicklingError,
                         "Can't pickle %R: %S", global, exc);
            _PyErr_ChainExceptions1(exc);
        }
        Py_DECREF(module_name);
        return NULL;
    }
    PyObject *actual = getattribute(module, dotted_path, 1);
    Py_DECREF(module);
    if (actual == NULL) {
        assert(PyErr_Occurred());
        if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PyObject *exc = PyErr_GetRaisedException();
            PyErr_Format(st->PicklingError,
                         "Can't pickle %R: it's not found as %S.%S",
                         global, module_name, global_name);
            _PyErr_ChainExceptions1(exc);
        }
        Py_DECREF(module_name);
        return NULL;
    }
    if (actual != global) {
        Py_DECREF(actual);
        PyErr_Format(st->PicklingError,
                     "Can't pickle %R: it's not the same object as %S.%S",
                     global, module_name, global_name);
        Py_DECREF(module_name);
        return NULL;
    }
    Py_DECREF(actual);
    return module_name;
}

/* fast_save_enter() and fast_save_leave() are guards against recursive
   objects when Pickler is used with the "fast mode" (i.e., with object
   memoization disabled). If the nesting of a list or dict object exceed
   FAST_NESTING_LIMIT, these guards will start keeping an internal
   reference to the seen list or dict objects and check whether these objects
   are recursive. These are not strictly necessary, since save() has a
   hard-coded recursion limit, but they give a nicer error message than the
   typical RuntimeError. */
static int
fast_save_enter(PicklerObject *self, PyObject *obj)
{
    /* if fast_nesting < 0, we're doing an error exit. */
    if (++self->fast_nesting >= FAST_NESTING_LIMIT) {
        PyObject *key = NULL;
        if (self->fast_memo == NULL) {
            self->fast_memo = PyDict_New();
            if (self->fast_memo == NULL) {
                self->fast_nesting = -1;
                return 0;
            }
        }
        key = PyLong_FromVoidPtr(obj);
        if (key == NULL) {
            self->fast_nesting = -1;
            return 0;
        }
        int r = PyDict_Contains(self->fast_memo, key);
        if (r > 0) {
            PyErr_Format(PyExc_ValueError,
                         "fast mode: can't pickle cyclic objects "
                         "including object type %.200s at %p",
                         Py_TYPE(obj)->tp_name, obj);
        }
        else if (r == 0) {
            r = PyDict_SetItem(self->fast_memo, key, Py_None);
        }
        Py_DECREF(key);
        if (r != 0) {
            self->fast_nesting = -1;
            return 0;
        }
    }
    return 1;
}

static int
fast_save_leave(PicklerObject *self, PyObject *obj)
{
    if (self->fast_nesting-- >= FAST_NESTING_LIMIT) {
        PyObject *key = PyLong_FromVoidPtr(obj);
        if (key == NULL)
            return 0;
        if (PyDict_DelItem(self->fast_memo, key) < 0) {
            Py_DECREF(key);
            return 0;
        }
        Py_DECREF(key);
    }
    return 1;
}

static int
save_none(PicklerObject *self, PyObject *obj)
{
    const char none_op = NONE;
    if (_Pickler_Write(self, &none_op, 1) < 0)
        return -1;

    return 0;
}

static int
save_bool(PicklerObject *self, PyObject *obj)
{
    if (self->proto >= 2) {
        const char bool_op = (obj == Py_True) ? NEWTRUE : NEWFALSE;
        if (_Pickler_Write(self, &bool_op, 1) < 0)
            return -1;
    }
    else {
        /* These aren't opcodes -- they're ways to pickle bools before protocol 2
         * so that unpicklers written before bools were introduced unpickle them
         * as ints, but unpicklers after can recognize that bools were intended.
         * Note that protocol 2 added direct ways to pickle bools.
         */
        const char *bool_str = (obj == Py_True) ? "I01\n" : "I00\n";
        if (_Pickler_Write(self, bool_str, strlen(bool_str)) < 0)
            return -1;
    }
    return 0;
}

static int
save_long(PicklerObject *self, PyObject *obj)
{
    PyObject *repr = NULL;
    Py_ssize_t size;
    long val;
    int overflow;
    int status = 0;

    val= PyLong_AsLongAndOverflow(obj, &overflow);
    if (!overflow && (sizeof(long) <= 4 ||
            (val <= 0x7fffffffL && val >= (-0x7fffffffL - 1))))
    {
        /* result fits in a signed 4-byte integer.

           Note: we can't use -0x80000000L in the above condition because some
           compilers (e.g., MSVC) will promote 0x80000000L to an unsigned type
           before applying the unary minus when sizeof(long) <= 4. The
           resulting value stays unsigned which is commonly not what we want,
           so MSVC happily warns us about it.  However, that result would have
           been fine because we guard for sizeof(long) <= 4 which turns the
           condition true in that particular case. */
        char pdata[32];
        Py_ssize_t len = 0;

        if (self->bin) {
            pdata[1] = (unsigned char)(val & 0xff);
            pdata[2] = (unsigned char)((val >> 8) & 0xff);
            pdata[3] = (unsigned char)((val >> 16) & 0xff);
            pdata[4] = (unsigned char)((val >> 24) & 0xff);

            if ((pdata[4] != 0) || (pdata[3] != 0)) {
                pdata[0] = BININT;
                len = 5;
            }
            else if (pdata[2] != 0) {
                pdata[0] = BININT2;
                len = 3;
            }
            else {
                pdata[0] = BININT1;
                len = 2;
            }
        }
        else {
            sprintf(pdata, "%c%ld\n", INT,  val);
            len = strlen(pdata);
        }
        if (_Pickler_Write(self, pdata, len) < 0)
            return -1;

        return 0;
    }
    assert(!PyErr_Occurred());

    if (self->proto >= 2) {
        /* Linear-time pickling. */
        int64_t nbits;
        size_t nbytes;
        unsigned char *pdata;
        char header[5];
        int i;

        int sign;
        assert(PyLong_Check(obj));
        (void)PyLong_GetSign(obj, &sign);
        if (sign == 0) {
            header[0] = LONG1;
            header[1] = 0;      /* It's 0 -- an empty bytestring. */
            if (_Pickler_Write(self, header, 2) < 0)
                goto error;
            return 0;
        }
        nbits = _PyLong_NumBits(obj);
        assert(nbits >= 0);
        assert(!PyErr_Occurred());
        /* How many bytes do we need?  There are nbits >> 3 full
         * bytes of data, and nbits & 7 leftover bits.  If there
         * are any leftover bits, then we clearly need another
         * byte.  What's not so obvious is that we *probably*
         * need another byte even if there aren't any leftovers:
         * the most-significant bit of the most-significant byte
         * acts like a sign bit, and it's usually got a sense
         * opposite of the one we need.  The exception is ints
         * of the form -(2**(8*j-1)) for j > 0.  Such an int is
         * its own 256's-complement, so has the right sign bit
         * even without the extra byte.  That's a pain to check
         * for in advance, though, so we always grab an extra
         * byte at the start, and cut it back later if possible.
         */
        nbytes = (size_t)((nbits >> 3) + 1);
        if (nbytes > 0x7fffffffL) {
            PyErr_SetString(PyExc_OverflowError,
                            "int too large to pickle");
            goto error;
        }
        repr = PyBytes_FromStringAndSize(NULL, (Py_ssize_t)nbytes);
        if (repr == NULL)
            goto error;
        pdata = (unsigned char *)PyBytes_AS_STRING(repr);
        i = _PyLong_AsByteArray((PyLongObject *)obj,
                                pdata, nbytes,
                                1 /* little endian */ , 1 /* signed */ ,
                                1 /* with exceptions */);
        if (i < 0)
            goto error;
        /* If the int is negative, this may be a byte more than
         * needed.  This is so iff the MSB is all redundant sign
         * bits.
         */
        if (sign < 0 &&
            nbytes > 1 &&
            pdata[nbytes - 1] == 0xff &&
            (pdata[nbytes - 2] & 0x80) != 0) {
            nbytes--;
        }

        if (nbytes < 256) {
            header[0] = LONG1;
            header[1] = (unsigned char)nbytes;
            size = 2;
        }
        else {
            header[0] = LONG4;
            size = (Py_ssize_t) nbytes;
            for (i = 1; i < 5; i++) {
                header[i] = (unsigned char)(size & 0xff);
                size >>= 8;
            }
            size = 5;
        }
        if (_Pickler_Write(self, header, size) < 0 ||
            _Pickler_Write(self, (char *)pdata, (int)nbytes) < 0)
            goto error;
    }
    else {
        const char long_op = LONG;
        const char *string;

        /* proto < 2: write the repr and newline.  This is quadratic-time (in
           the number of digits), in both directions.  We add a trailing 'L'
           to the repr, for compatibility with Python 2.x. */

        repr = PyObject_Repr(obj);
        if (repr == NULL)
            goto error;

        string = PyUnicode_AsUTF8AndSize(repr, &size);
        if (string == NULL)
            goto error;

        if (_Pickler_Write(self, &long_op, 1) < 0 ||
            _Pickler_Write(self, string, size) < 0 ||
            _Pickler_Write(self, "L\n", 2) < 0)
            goto error;
    }

    if (0) {
  error:
      status = -1;
    }
    Py_XDECREF(repr);

    return status;
}

static int
save_float(PicklerObject *self, PyObject *obj)
{
    double x = PyFloat_AS_DOUBLE((PyFloatObject *)obj);

    if (self->bin) {
        char pdata[9];
        pdata[0] = BINFLOAT;
        if (PyFloat_Pack8(x, &pdata[1], 0) < 0)
            return -1;
        if (_Pickler_Write(self, pdata, 9) < 0)
            return -1;
   }
    else {
        int result = -1;
        char *buf = NULL;
        char op = FLOAT;

        if (_Pickler_Write(self, &op, 1) < 0)
            goto done;

        buf = PyOS_double_to_string(x, 'r', 0, Py_DTSF_ADD_DOT_0, NULL);
        if (!buf) {
            PyErr_NoMemory();
            goto done;
        }

        if (_Pickler_Write(self, buf, strlen(buf)) < 0)
            goto done;

        if (_Pickler_Write(self, "\n", 1) < 0)
            goto done;

        result = 0;
done:
        PyMem_Free(buf);
        return result;
    }

    return 0;
}

/* Perform direct write of the header and payload of the binary object.

   The large contiguous data is written directly into the underlying file
   object, bypassing the output_buffer of the Pickler.  We intentionally
   do not insert a protocol 4 frame opcode to make it possible to optimize
   file.read calls in the loader.
 */
static int
_Pickler_write_bytes(PicklerObject *self,
                     const char *header, Py_ssize_t header_size,
                     const char *data, Py_ssize_t data_size,
                     PyObject *payload)
{
    int bypass_buffer = (data_size >= FRAME_SIZE_TARGET);
    int framing = self->framing;

    if (bypass_buffer) {
        assert(self->output_buffer != NULL);
        /* Commit the previous frame. */
        if (_Pickler_CommitFrame(self)) {
            return -1;
        }
        /* Disable framing temporarily */
        self->framing = 0;
    }

    if (_Pickler_Write(self, header, header_size) < 0) {
        return -1;
    }

    if (bypass_buffer && self->write != NULL) {
        /* Bypass the in-memory buffer to directly stream large data
           into the underlying file object. */
        PyObject *result, *mem = NULL;
        /* Dump the output buffer to the file. */
        if (_Pickler_FlushToFile(self) < 0) {
            return -1;
        }

        /* Stream write the payload into the file without going through the
           output buffer. */
        if (payload == NULL) {
            /* TODO: It would be better to use a memoryview with a linked
               original string if this is possible. */
            payload = mem = PyBytes_FromStringAndSize(data, data_size);
            if (payload == NULL) {
                return -1;
            }
        }
        result = PyObject_CallOneArg(self->write, payload);
        Py_XDECREF(mem);
        if (result == NULL) {
            return -1;
        }
        Py_DECREF(result);

        /* Reinitialize the buffer for subsequent calls to _Pickler_Write. */
        if (_Pickler_ClearBuffer(self) < 0) {
            return -1;
        }
    }
    else {
        if (_Pickler_Write(self, data, data_size) < 0) {
            return -1;
        }
    }

    /* Re-enable framing for subsequent calls to _Pickler_Write. */
    self->framing = framing;

    return 0;
}

static int
_save_bytes_data(PickleState *st, PicklerObject *self, PyObject *obj,
                 const char *data, Py_ssize_t size)
{
    assert(self->proto >= 3);

    char header[9];
    Py_ssize_t len;

    if (size < 0)
        return -1;

    if (size <= 0xff) {
        header[0] = SHORT_BINBYTES;
        header[1] = (unsigned char)size;
        len = 2;
    }
    else if ((size_t)size <= 0xffffffffUL) {
        header[0] = BINBYTES;
        header[1] = (unsigned char)(size & 0xff);
        header[2] = (unsigned char)((size >> 8) & 0xff);
        header[3] = (unsigned char)((size >> 16) & 0xff);
        header[4] = (unsigned char)((size >> 24) & 0xff);
        len = 5;
    }
    else if (self->proto >= 4) {
        header[0] = BINBYTES8;
        _write_size64(header + 1, size);
        len = 9;
    }
    else {
        PyErr_SetString(PyExc_OverflowError,
                        "serializing a bytes object larger than 4 GiB "
                        "requires pickle protocol 4 or higher");
        return -1;
    }

    if (_Pickler_write_bytes(self, header, len, data, size, obj) < 0) {
        return -1;
    }

    if (memo_put(st, self, obj) < 0) {
        return -1;
    }

    return 0;
}

static int
save_bytes(PickleState *st, PicklerObject *self, PyObject *obj)
{
    if (self->proto < 3) {
        /* Older pickle protocols do not have an opcode for pickling bytes
           objects. Therefore, we need to fake the copy protocol (i.e.,
           the __reduce__ method) to permit bytes object unpickling.

           Here we use a hack to be compatible with Python 2. Since in Python
           2 'bytes' is just an alias for 'str' (which has different
           parameters than the actual bytes object), we use codecs.encode
           to create the appropriate 'str' object when unpickled using
           Python 2 *and* the appropriate 'bytes' object when unpickled
           using Python 3. Again this is a hack and we don't need to do this
           with newer protocols. */
        PyObject *reduce_value;
        int status;

        if (PyBytes_GET_SIZE(obj) == 0) {
            reduce_value = Py_BuildValue("(O())", (PyObject*)&PyBytes_Type);
        }
        else {
            PyObject *unicode_str =
                PyUnicode_DecodeLatin1(PyBytes_AS_STRING(obj),
                                       PyBytes_GET_SIZE(obj),
                                       "strict");

            if (unicode_str == NULL)
                return -1;
            reduce_value = Py_BuildValue("(O(OO))",
                                         st->codecs_encode, unicode_str,
                                         &_Py_ID(latin1));
            Py_DECREF(unicode_str);
        }

        if (reduce_value == NULL)
            return -1;

        /* save_reduce() will memoize the object automatically. */
        status = save_reduce(st, self, reduce_value, obj);
        Py_DECREF(reduce_value);
        return status;
    }
    else {
        return _save_bytes_data(st, self, obj, PyBytes_AS_STRING(obj),
                                PyBytes_GET_SIZE(obj));
    }
}

static int
_save_bytearray_data(PickleState *state, PicklerObject *self, PyObject *obj,
                     const char *data, Py_ssize_t size)
{
    assert(self->proto >= 5);

    char header[9];
    Py_ssize_t len;

    if (size < 0)
        return -1;

    header[0] = BYTEARRAY8;
    _write_size64(header + 1, size);
    len = 9;

    if (_Pickler_write_bytes(self, header, len, data, size, obj) < 0) {
        return -1;
    }

    if (memo_put(state, self, obj) < 0) {
        return -1;
    }

    return 0;
}

static int
save_bytearray(PickleState *state, PicklerObject *self, PyObject *obj)
{
    if (self->proto < 5) {
        /* Older pickle protocols do not have an opcode for pickling
         * bytearrays. */
        PyObject *reduce_value = NULL;
        int status;

        if (PyByteArray_GET_SIZE(obj) == 0) {
            reduce_value = Py_BuildValue("(O())",
                                         (PyObject *) &PyByteArray_Type);
        }
        else {
            PyObject *bytes_obj = PyBytes_FromObject(obj);
            if (bytes_obj != NULL) {
                reduce_value = Py_BuildValue("(O(O))",
                                             (PyObject *) &PyByteArray_Type,
                                             bytes_obj);
                Py_DECREF(bytes_obj);
            }
        }
        if (reduce_value == NULL)
            return -1;

        /* save_reduce() will memoize the object automatically. */
        status = save_reduce(state, self, reduce_value, obj);
        Py_DECREF(reduce_value);
        return status;
    }
    else {
        return _save_bytearray_data(state, self, obj,
                                    PyByteArray_AS_STRING(obj),
                                    PyByteArray_GET_SIZE(obj));
    }
}

static int
save_picklebuffer(PickleState *st, PicklerObject *self, PyObject *obj)
{
    if (self->proto < 5) {
        PyErr_SetString(st->PicklingError,
                        "PickleBuffer can only be pickled with protocol >= 5");
        return -1;
    }
    const Py_buffer* view = PyPickleBuffer_GetBuffer(obj);
    if (view == NULL) {
        return -1;
    }
    if (view->suboffsets != NULL || !PyBuffer_IsContiguous(view, 'A')) {
        PyErr_SetString(st->PicklingError,
                        "PickleBuffer can not be pickled when "
                        "pointing to a non-contiguous buffer");
        return -1;
    }
    int in_band = 1;
    if (self->buffer_callback != NULL) {
        PyObject *ret = PyObject_CallOneArg(self->buffer_callback, obj);
        if (ret == NULL) {
            return -1;
        }
        in_band = PyObject_IsTrue(ret);
        Py_DECREF(ret);
        if (in_band == -1) {
            return -1;
        }
    }
    if (in_band) {
        /* Write data in-band */
        if (view->readonly) {
            return _save_bytes_data(st, self, obj, (const char *)view->buf,
                                    view->len);
        }
        else {
            return _save_bytearray_data(st, self, obj, (const char *)view->buf,
                                        view->len);
        }
    }
    else {
        /* Write data out-of-band */
        const char next_buffer_op = NEXT_BUFFER;
        if (_Pickler_Write(self, &next_buffer_op, 1) < 0) {
            return -1;
        }
        if (view->readonly) {
            const char readonly_buffer_op = READONLY_BUFFER;
            if (_Pickler_Write(self, &readonly_buffer_op, 1) < 0) {
                return -1;
            }
        }
    }
    return 0;
}

/* A copy of PyUnicode_AsRawUnicodeEscapeString() that also translates
   backslash and newline characters to \uXXXX escapes. */
static PyObject *
raw_unicode_escape(PyObject *obj)
{
    char *p;
    Py_ssize_t i, size;
    const void *data;
    int kind;
    _PyBytesWriter writer;

    _PyBytesWriter_Init(&writer);

    size = PyUnicode_GET_LENGTH(obj);
    data = PyUnicode_DATA(obj);
    kind = PyUnicode_KIND(obj);

    p = _PyBytesWriter_Alloc(&writer, size);
    if (p == NULL)
        goto error;
    writer.overallocate = 1;

    for (i=0; i < size; i++) {
        Py_UCS4 ch = PyUnicode_READ(kind, data, i);
        /* Map 32-bit characters to '\Uxxxxxxxx' */
        if (ch >= 0x10000) {
            /* -1: subtract 1 preallocated byte */
            p = _PyBytesWriter_Prepare(&writer, p, 10-1);
            if (p == NULL)
                goto error;

            *p++ = '\\';
            *p++ = 'U';
            *p++ = Py_hexdigits[(ch >> 28) & 0xf];
            *p++ = Py_hexdigits[(ch >> 24) & 0xf];
            *p++ = Py_hexdigits[(ch >> 20) & 0xf];
            *p++ = Py_hexdigits[(ch >> 16) & 0xf];
            *p++ = Py_hexdigits[(ch >> 12) & 0xf];
            *p++ = Py_hexdigits[(ch >> 8) & 0xf];
            *p++ = Py_hexdigits[(ch >> 4) & 0xf];
            *p++ = Py_hexdigits[ch & 15];
        }
        /* Map 16-bit characters, '\\' and '\n' to '\uxxxx' */
        else if (ch >= 256 ||
                 ch == '\\' || ch == 0 || ch == '\n' || ch == '\r' ||
                 ch == 0x1a)
        {
            /* -1: subtract 1 preallocated byte */
            p = _PyBytesWriter_Prepare(&writer, p, 6-1);
            if (p == NULL)
                goto error;

            *p++ = '\\';
            *p++ = 'u';
            *p++ = Py_hexdigits[(ch >> 12) & 0xf];
            *p++ = Py_hexdigits[(ch >> 8) & 0xf];
            *p++ = Py_hexdigits[(ch >> 4) & 0xf];
            *p++ = Py_hexdigits[ch & 15];
        }
        /* Copy everything else as-is */
        else
            *p++ = (char) ch;
    }

    return _PyBytesWriter_Finish(&writer, p);

error:
    _PyBytesWriter_Dealloc(&writer);
    return NULL;
}

static int
write_unicode_binary(PicklerObject *self, PyObject *obj)
{
    char header[9];
    Py_ssize_t len;
    PyObject *encoded = NULL;
    Py_ssize_t size;
    const char *data;

    data = PyUnicode_AsUTF8AndSize(obj, &size);
    if (data == NULL) {
        /* Issue #8383: for strings with lone surrogates, fallback on the
           "surrogatepass" error handler. */
        PyErr_Clear();
        encoded = PyUnicode_AsEncodedString(obj, "utf-8", "surrogatepass");
        if (encoded == NULL)
            return -1;

        data = PyBytes_AS_STRING(encoded);
        size = PyBytes_GET_SIZE(encoded);
    }

    assert(size >= 0);
    if (size <= 0xff && self->proto >= 4) {
        header[0] = SHORT_BINUNICODE;
        header[1] = (unsigned char)(size & 0xff);
        len = 2;
    }
    else if ((size_t)size <= 0xffffffffUL) {
        header[0] = BINUNICODE;
        header[1] = (unsigned char)(size & 0xff);
        header[2] = (unsigned char)((size >> 8) & 0xff);
        header[3] = (unsigned char)((size >> 16) & 0xff);
        header[4] = (unsigned char)((size >> 24) & 0xff);
        len = 5;
    }
    else if (self->proto >= 4) {
        header[0] = BINUNICODE8;
        _write_size64(header + 1, size);
        len = 9;
    }
    else {
        PyErr_SetString(PyExc_OverflowError,
                        "serializing a string larger than 4 GiB "
                        "requires pickle protocol 4 or higher");
        Py_XDECREF(encoded);
        return -1;
    }

    if (_Pickler_write_bytes(self, header, len, data, size, encoded) < 0) {
        Py_XDECREF(encoded);
        return -1;
    }
    Py_XDECREF(encoded);
    return 0;
}

static int
save_unicode(PickleState *state, PicklerObject *self, PyObject *obj)
{
    if (self->bin) {
        if (write_unicode_binary(self, obj) < 0)
            return -1;
    }
    else {
        PyObject *encoded;
        Py_ssize_t size;
        const char unicode_op = UNICODE;

        encoded = raw_unicode_escape(obj);
        if (encoded == NULL)
            return -1;

        if (_Pickler_Write(self, &unicode_op, 1) < 0) {
            Py_DECREF(encoded);
            return -1;
        }

        size = PyBytes_GET_SIZE(encoded);
        if (_Pickler_Write(self, PyBytes_AS_STRING(encoded), size) < 0) {
            Py_DECREF(encoded);
            return -1;
        }
        Py_DECREF(encoded);

        if (_Pickler_Write(self, "\n", 1) < 0)
            return -1;
    }
    if (memo_put(state, self, obj) < 0)
        return -1;

    return 0;
}

/* A helper for save_tuple.  Push the len elements in tuple t on the stack. */
static int
store_tuple_elements(PickleState *state, PicklerObject *self, PyObject *t,
                     Py_ssize_t len)
{
    Py_ssize_t i;

    assert(PyTuple_Size(t) == len);

    for (i = 0; i < len; i++) {
        PyObject *element = PyTuple_GET_ITEM(t, i);

        if (element == NULL)
            return -1;
        if (save(state, self, element, 0) < 0) {
            _PyErr_FormatNote("when serializing %T item %zd", t, i);
            return -1;
        }
    }

    return 0;
}

/* Tuples are ubiquitous in the pickle protocols, so many techniques are
 * used across protocols to minimize the space needed to pickle them.
 * Tuples are also the only builtin immutable type that can be recursive
 * (a tuple can be reached from itself), and that requires some subtle
 * magic so that it works in all cases.  IOW, this is a long routine.
 */
static int
save_tuple(PickleState *state, PicklerObject *self, PyObject *obj)
{
    Py_ssize_t len, i;

    const char mark_op = MARK;
    const char tuple_op = TUPLE;
    const char pop_op = POP;
    const char pop_mark_op = POP_MARK;
    const char len2opcode[] = {EMPTY_TUPLE, TUPLE1, TUPLE2, TUPLE3};

    if ((len = PyTuple_Size(obj)) < 0)
        return -1;

    if (len == 0) {
        char pdata[2];

        if (self->proto) {
            pdata[0] = EMPTY_TUPLE;
            len = 1;
        }
        else {
            pdata[0] = MARK;
            pdata[1] = TUPLE;
            len = 2;
        }
        if (_Pickler_Write(self, pdata, len) < 0)
            return -1;
        return 0;
    }

    /* The tuple isn't in the memo now.  If it shows up there after
     * saving the tuple elements, the tuple must be recursive, in
     * which case we'll pop everything we put on the stack, and fetch
     * its value from the memo.
     */
    if (len <= 3 && self->proto >= 2) {
        /* Use TUPLE{1,2,3} opcodes. */
        if (store_tuple_elements(state, self, obj, len) < 0)
            return -1;

        if (PyMemoTable_Get(self->memo, obj)) {
            /* pop the len elements */
            for (i = 0; i < len; i++)
                if (_Pickler_Write(self, &pop_op, 1) < 0)
                    return -1;
            /* fetch from memo */
            if (memo_get(state, self, obj) < 0)
                return -1;

            return 0;
        }
        else { /* Not recursive. */
            if (_Pickler_Write(self, len2opcode + len, 1) < 0)
                return -1;
        }
        goto memoize;
    }

    /* proto < 2 and len > 0, or proto >= 2 and len > 3.
     * Generate MARK e1 e2 ... TUPLE
     */
    if (_Pickler_Write(self, &mark_op, 1) < 0)
        return -1;

    if (store_tuple_elements(state, self, obj, len) < 0)
        return -1;

    if (PyMemoTable_Get(self->memo, obj)) {
        /* pop the stack stuff we pushed */
        if (self->bin) {
            if (_Pickler_Write(self, &pop_mark_op, 1) < 0)
                return -1;
        }
        else {
            /* Note that we pop one more than len, to remove
             * the MARK too.
             */
            for (i = 0; i <= len; i++)
                if (_Pickler_Write(self, &pop_op, 1) < 0)
                    return -1;
        }
        /* fetch from memo */
        if (memo_get(state, self, obj) < 0)
            return -1;

        return 0;
    }
    else { /* Not recursive. */
        if (_Pickler_Write(self, &tuple_op, 1) < 0)
            return -1;
    }

  memoize:
    if (memo_put(state, self, obj) < 0)
        return -1;

    return 0;
}

/* iter is an iterator giving items, and we batch up chunks of
 *     MARK item item ... item APPENDS
 * opcode sequences.  Calling code should have arranged to first create an
 * empty list, or list-like object, for the APPENDS to operate on.
 * Returns 0 on success, <0 on error.
 */
static int
batch_list(PickleState *state, PicklerObject *self, PyObject *iter, PyObject *origobj)
{
    PyObject *obj = NULL;
    PyObject *firstitem = NULL;
    int i, n;
    Py_ssize_t total = 0;

    const char mark_op = MARK;
    const char append_op = APPEND;
    const char appends_op = APPENDS;

    assert(iter != NULL);

    /* XXX: I think this function could be made faster by avoiding the
       iterator interface and fetching objects directly from list using
       PyList_GET_ITEM.
    */

    if (self->proto == 0) {
        /* APPENDS isn't available; do one at a time. */
        for (;; total++) {
            obj = PyIter_Next(iter);
            if (obj == NULL) {
                if (PyErr_Occurred())
                    return -1;
                break;
            }
            i = save(state, self, obj, 0);
            Py_DECREF(obj);
            if (i < 0) {
                _PyErr_FormatNote("when serializing %T item %zd", origobj, total);
                return -1;
            }
            if (_Pickler_Write(self, &append_op, 1) < 0)
                return -1;
        }
        return 0;
    }

    /* proto > 0:  write in batches of BATCHSIZE. */
    do {
        /* Get first item */
        firstitem = PyIter_Next(iter);
        if (firstitem == NULL) {
            if (PyErr_Occurred())
                goto error;

            /* nothing more to add */
            break;
        }

        /* Try to get a second item */
        obj = PyIter_Next(iter);
        if (obj == NULL) {
            if (PyErr_Occurred())
                goto error;

            /* Only one item to write */
            if (save(state, self, firstitem, 0) < 0) {
                _PyErr_FormatNote("when serializing %T item %zd", origobj, total);
                goto error;
            }
            if (_Pickler_Write(self, &append_op, 1) < 0)
                goto error;
            Py_CLEAR(firstitem);
            break;
        }

        /* More than one item to write */

        /* Pump out MARK, items, APPENDS. */
        if (_Pickler_Write(self, &mark_op, 1) < 0)
            goto error;

        if (save(state, self, firstitem, 0) < 0) {
            _PyErr_FormatNote("when serializing %T item %zd", origobj, total);
            goto error;
        }
        Py_CLEAR(firstitem);
        total++;
        n = 1;

        /* Fetch and save up to BATCHSIZE items */
        while (obj) {
            if (save(state, self, obj, 0) < 0) {
                _PyErr_FormatNote("when serializing %T item %zd", origobj, total);
                goto error;
            }
            Py_CLEAR(obj);
            total++;
            n += 1;

            if (n == BATCHSIZE)
                break;

            obj = PyIter_Next(iter);
            if (obj == NULL) {
                if (PyErr_Occurred())
                    goto error;
                break;
            }
        }

        if (_Pickler_Write(self, &appends_op, 1) < 0)
            goto error;

    } while (n == BATCHSIZE);
    return 0;

  error:
    Py_XDECREF(firstitem);
    Py_XDECREF(obj);
    return -1;
}

/* This is a variant of batch_list() above, specialized for lists (with no
 * support for list subclasses). Like batch_list(), we batch up chunks of
 *     MARK item item ... item APPENDS
 * opcode sequences.  Calling code should have arranged to first create an
 * empty list, or list-like object, for the APPENDS to operate on.
 * Returns 0 on success, -1 on error.
 *
 * This version is considerably faster than batch_list(), if less general.
 *
 * Note that this only works for protocols > 0.
 */
static int
batch_list_exact(PickleState *state, PicklerObject *self, PyObject *obj)
{
    PyObject *item = NULL;
    Py_ssize_t this_batch, total;

    const char append_op = APPEND;
    const char appends_op = APPENDS;
    const char mark_op = MARK;

    assert(obj != NULL);
    assert(self->proto > 0);
    assert(PyList_CheckExact(obj));

    if (PyList_GET_SIZE(obj) == 1) {
        item = PyList_GET_ITEM(obj, 0);
        Py_INCREF(item);
        int err = save(state, self, item, 0);
        Py_DECREF(item);
        if (err < 0) {
            _PyErr_FormatNote("when serializing %T item 0", obj);
            return -1;
        }
        if (_Pickler_Write(self, &append_op, 1) < 0)
            return -1;
        return 0;
    }

    /* Write in batches of BATCHSIZE. */
    total = 0;
    do {
        this_batch = 0;
        if (_Pickler_Write(self, &mark_op, 1) < 0)
            return -1;
        while (total < PyList_GET_SIZE(obj)) {
            item = PyList_GET_ITEM(obj, total);
            Py_INCREF(item);
            int err = save(state, self, item, 0);
            Py_DECREF(item);
            if (err < 0) {
                _PyErr_FormatNote("when serializing %T item %zd", obj, total);
                return -1;
            }
            total++;
            if (++this_batch == BATCHSIZE)
                break;
        }
        if (_Pickler_Write(self, &appends_op, 1) < 0)
            return -1;

    } while (total < PyList_GET_SIZE(obj));

    return 0;
}

static int
save_list(PickleState *state, PicklerObject *self, PyObject *obj)
{
    char header[3];
    Py_ssize_t len;
    int status = 0;

    if (self->fast && !fast_save_enter(self, obj))
        goto error;

    /* Create an empty list. */
    if (self->bin) {
        header[0] = EMPTY_LIST;
        len = 1;
    }
    else {
        header[0] = MARK;
        header[1] = LIST;
        len = 2;
    }

    if (_Pickler_Write(self, header, len) < 0)
        goto error;

    /* Get list length, and bow out early if empty. */
    if ((len = PyList_Size(obj)) < 0)
        goto error;

    if (memo_put(state, self, obj) < 0)
        goto error;

    if (len != 0) {
        /* Materialize the list elements. */
        if (PyList_CheckExact(obj) && self->proto > 0) {
            if (_Py_EnterRecursiveCall(" while pickling an object"))
                goto error;
            status = batch_list_exact(state, self, obj);
            _Py_LeaveRecursiveCall();
        } else {
            PyObject *iter = PyObject_GetIter(obj);
            if (iter == NULL)
                goto error;

            if (_Py_EnterRecursiveCall(" while pickling an object")) {
                Py_DECREF(iter);
                goto error;
            }
            status = batch_list(state, self, iter, obj);
            _Py_LeaveRecursiveCall();
            Py_DECREF(iter);
        }
    }
    if (0) {
  error:
        status = -1;
    }

    if (self->fast && !fast_save_leave(self, obj))
        status = -1;

    return status;
}

/* iter is an iterator giving (key, value) pairs, and we batch up chunks of
 *     MARK key value ... key value SETITEMS
 * opcode sequences.  Calling code should have arranged to first create an
 * empty dict, or dict-like object, for the SETITEMS to operate on.
 * Returns 0 on success, <0 on error.
 *
 * This is very much like batch_list().  The difference between saving
 * elements directly, and picking apart two-tuples, is so long-winded at
 * the C level, though, that attempts to combine these routines were too
 * ugly to bear.
 */
static int
batch_dict(PickleState *state, PicklerObject *self, PyObject *iter, PyObject *origobj)
{
    PyObject *obj = NULL;
    PyObject *firstitem = NULL;
    int i, n;

    const char mark_op = MARK;
    const char setitem_op = SETITEM;
    const char setitems_op = SETITEMS;

    assert(iter != NULL);

    if (self->proto == 0) {
        /* SETITEMS isn't available; do one at a time. */
        for (;;) {
            obj = PyIter_Next(iter);
            if (obj == NULL) {
                if (PyErr_Occurred())
                    return -1;
                break;
            }
            if (!PyTuple_Check(obj) || PyTuple_Size(obj) != 2) {
                PyErr_SetString(PyExc_TypeError, "dict items "
                                "iterator must return 2-tuples");
                Py_DECREF(obj);
                return -1;
            }
            i = save(state, self, PyTuple_GET_ITEM(obj, 0), 0);
            if (i >= 0) {
                i = save(state, self, PyTuple_GET_ITEM(obj, 1), 0);
                if (i < 0) {
                    _PyErr_FormatNote("when serializing %T item %R",
                                      origobj, PyTuple_GET_ITEM(obj, 0));
                }
            }
            Py_DECREF(obj);
            if (i < 0)
                return -1;
            if (_Pickler_Write(self, &setitem_op, 1) < 0)
                return -1;
        }
        return 0;
    }

    /* proto > 0:  write in batches of BATCHSIZE. */
    do {
        /* Get first item */
        firstitem = PyIter_Next(iter);
        if (firstitem == NULL) {
            if (PyErr_Occurred())
                goto error;

            /* nothing more to add */
            break;
        }
        if (!PyTuple_Check(firstitem) || PyTuple_Size(firstitem) != 2) {
            PyErr_SetString(PyExc_TypeError, "dict items "
                                "iterator must return 2-tuples");
            goto error;
        }

        /* Try to get a second item */
        obj = PyIter_Next(iter);
        if (obj == NULL) {
            if (PyErr_Occurred())
                goto error;

            /* Only one item to write */
            if (save(state, self, PyTuple_GET_ITEM(firstitem, 0), 0) < 0)
                goto error;
            if (save(state, self, PyTuple_GET_ITEM(firstitem, 1), 0) < 0) {
                _PyErr_FormatNote("when serializing %T item %R",
                                  origobj, PyTuple_GET_ITEM(firstitem, 0));
                goto error;
            }
            if (_Pickler_Write(self, &setitem_op, 1) < 0)
                goto error;
            Py_CLEAR(firstitem);
            break;
        }

        /* More than one item to write */

        /* Pump out MARK, items, SETITEMS. */
        if (_Pickler_Write(self, &mark_op, 1) < 0)
            goto error;

        if (save(state, self, PyTuple_GET_ITEM(firstitem, 0), 0) < 0)
            goto error;
        if (save(state, self, PyTuple_GET_ITEM(firstitem, 1), 0) < 0) {
            _PyErr_FormatNote("when serializing %T item %R",
                              origobj, PyTuple_GET_ITEM(firstitem, 0));
            goto error;
        }
        Py_CLEAR(firstitem);
        n = 1;

        /* Fetch and save up to BATCHSIZE items */
        while (obj) {
            if (!PyTuple_Check(obj) || PyTuple_Size(obj) != 2) {
                PyErr_SetString(PyExc_TypeError, "dict items "
                    "iterator must return 2-tuples");
                goto error;
            }
            if (save(state, self, PyTuple_GET_ITEM(obj, 0), 0) < 0)
                goto error;
            if (save(state, self, PyTuple_GET_ITEM(obj, 1), 0) < 0) {
                _PyErr_FormatNote("when serializing %T item %R",
                                  origobj, PyTuple_GET_ITEM(obj, 0));
                goto error;
            }
            Py_CLEAR(obj);
            n += 1;

            if (n == BATCHSIZE)
                break;

            obj = PyIter_Next(iter);
            if (obj == NULL) {
                if (PyErr_Occurred())
                    goto error;
                break;
            }
        }

        if (_Pickler_Write(self, &setitems_op, 1) < 0)
            goto error;

    } while (n == BATCHSIZE);
    return 0;

  error:
    Py_XDECREF(firstitem);
    Py_XDECREF(obj);
    return -1;
}

/* This is a variant of batch_dict() above that specializes for dicts, with no
 * support for dict subclasses. Like batch_dict(), we batch up chunks of
 *     MARK key value ... key value SETITEMS
 * opcode sequences.  Calling code should have arranged to first create an
 * empty dict, or dict-like object, for the SETITEMS to operate on.
 * Returns 0 on success, -1 on error.
 *
 * Note that this currently doesn't work for protocol 0.
 */
static int
batch_dict_exact(PickleState *state, PicklerObject *self, PyObject *obj)
{
    PyObject *key = NULL, *value = NULL;
    int i;
    Py_ssize_t dict_size, ppos = 0;

    const char mark_op = MARK;
    const char setitem_op = SETITEM;
    const char setitems_op = SETITEMS;

    assert(obj != NULL && PyDict_CheckExact(obj));
    assert(self->proto > 0);

    dict_size = PyDict_GET_SIZE(obj);

    /* Special-case len(d) == 1 to save space. */
    if (dict_size == 1) {
        PyDict_Next(obj, &ppos, &key, &value);
        Py_INCREF(key);
        Py_INCREF(value);
        if (save(state, self, key, 0) < 0) {
            goto error;
        }
        if (save(state, self, value, 0) < 0) {
            _PyErr_FormatNote("when serializing %T item %R", obj, key);
            goto error;
        }
        Py_CLEAR(key);
        Py_CLEAR(value);
        if (_Pickler_Write(self, &setitem_op, 1) < 0)
            return -1;
        return 0;
    }

    /* Write in batches of BATCHSIZE. */
    do {
        i = 0;
        if (_Pickler_Write(self, &mark_op, 1) < 0)
            return -1;
        while (PyDict_Next(obj, &ppos, &key, &value)) {
            Py_INCREF(key);
            Py_INCREF(value);
            if (save(state, self, key, 0) < 0) {
                goto error;
            }
            if (save(state, self, value, 0) < 0) {
                _PyErr_FormatNote("when serializing %T item %R", obj, key);
                goto error;
            }
            Py_CLEAR(key);
            Py_CLEAR(value);
            if (++i == BATCHSIZE)
                break;
        }
        if (_Pickler_Write(self, &setitems_op, 1) < 0)
            return -1;
        if (PyDict_GET_SIZE(obj) != dict_size) {
            PyErr_Format(
                PyExc_RuntimeError,
                "dictionary changed size during iteration");
            return -1;
        }

    } while (i == BATCHSIZE);
    return 0;
error:
    Py_XDECREF(key);
    Py_XDECREF(value);
    return -1;
}

static int
save_dict(PickleState *state, PicklerObject *self, PyObject *obj)
{
    PyObject *items, *iter;
    char header[3];
    Py_ssize_t len;
    int status = 0;
    assert(PyDict_Check(obj));

    if (self->fast && !fast_save_enter(self, obj))
        goto error;

    /* Create an empty dict. */
    if (self->bin) {
        header[0] = EMPTY_DICT;
        len = 1;
    }
    else {
        header[0] = MARK;
        header[1] = DICT;
        len = 2;
    }

    if (_Pickler_Write(self, header, len) < 0)
        goto error;

    if (memo_put(state, self, obj) < 0)
        goto error;

    if (PyDict_GET_SIZE(obj)) {
        /* Save the dict items. */
        if (PyDict_CheckExact(obj) && self->proto > 0) {
            /* We can take certain shortcuts if we know this is a dict and
               not a dict subclass. */
            if (_Py_EnterRecursiveCall(" while pickling an object"))
                goto error;
            status = batch_dict_exact(state, self, obj);
            _Py_LeaveRecursiveCall();
        } else {
            items = PyObject_CallMethodNoArgs(obj, &_Py_ID(items));
            if (items == NULL)
                goto error;
            iter = PyObject_GetIter(items);
            Py_DECREF(items);
            if (iter == NULL)
                goto error;
            if (_Py_EnterRecursiveCall(" while pickling an object")) {
                Py_DECREF(iter);
                goto error;
            }
            status = batch_dict(state, self, iter, obj);
            _Py_LeaveRecursiveCall();
            Py_DECREF(iter);
        }
    }

    if (0) {
  error:
        status = -1;
    }

    if (self->fast && !fast_save_leave(self, obj))
        status = -1;

    return status;
}

static int
save_set(PickleState *state, PicklerObject *self, PyObject *obj)
{
    PyObject *item;
    int i;
    Py_ssize_t set_size, ppos = 0;
    Py_hash_t hash;

    const char empty_set_op = EMPTY_SET;
    const char mark_op = MARK;
    const char additems_op = ADDITEMS;

    if (self->proto < 4) {
        PyObject *items;
        PyObject *reduce_value;
        int status;

        items = PySequence_List(obj);
        if (items == NULL) {
            return -1;
        }
        reduce_value = Py_BuildValue("(O(O))", (PyObject*)&PySet_Type, items);
        Py_DECREF(items);
        if (reduce_value == NULL) {
            return -1;
        }
        /* save_reduce() will memoize the object automatically. */
        status = save_reduce(state, self, reduce_value, obj);
        Py_DECREF(reduce_value);
        return status;
    }

    if (_Pickler_Write(self, &empty_set_op, 1) < 0)
        return -1;

    if (memo_put(state, self, obj) < 0)
        return -1;

    set_size = PySet_GET_SIZE(obj);
    if (set_size == 0)
        return 0;  /* nothing to do */

    /* Write in batches of BATCHSIZE. */
    do {
        i = 0;
        if (_Pickler_Write(self, &mark_op, 1) < 0)
            return -1;

        int err = 0;
        Py_BEGIN_CRITICAL_SECTION(obj);
        while (_PySet_NextEntryRef(obj, &ppos, &item, &hash)) {
            err = save(state, self, item, 0);
            Py_CLEAR(item);
            if (err < 0) {
                _PyErr_FormatNote("when serializing %T element", obj);
                break;
            }
            if (++i == BATCHSIZE)
                break;
        }
        Py_END_CRITICAL_SECTION();
        if (err < 0) {
            return -1;
        }
        if (_Pickler_Write(self, &additems_op, 1) < 0)
            return -1;
        if (PySet_GET_SIZE(obj) != set_size) {
            PyErr_Format(
                PyExc_RuntimeError,
                "set changed size during iteration");
            return -1;
        }
    } while (i == BATCHSIZE);

    return 0;
}

static int
save_frozenset(PickleState *state, PicklerObject *self, PyObject *obj)
{
    PyObject *iter;

    const char mark_op = MARK;
    const char frozenset_op = FROZENSET;

    if (self->fast && !fast_save_enter(self, obj))
        return -1;

    if (self->proto < 4) {
        PyObject *items;
        PyObject *reduce_value;
        int status;

        items = PySequence_List(obj);
        if (items == NULL) {
            return -1;
        }
        reduce_value = Py_BuildValue("(O(O))", (PyObject*)&PyFrozenSet_Type,
                                     items);
        Py_DECREF(items);
        if (reduce_value == NULL) {
            return -1;
        }
        /* save_reduce() will memoize the object automatically. */
        status = save_reduce(state, self, reduce_value, obj);
        Py_DECREF(reduce_value);
        return status;
    }

    if (_Pickler_Write(self, &mark_op, 1) < 0)
        return -1;

    iter = PyObject_GetIter(obj);
    if (iter == NULL) {
        return -1;
    }
    for (;;) {
        PyObject *item;

        item = PyIter_Next(iter);
        if (item == NULL) {
            if (PyErr_Occurred()) {
                Py_DECREF(iter);
                return -1;
            }
            break;
        }
        if (save(state, self, item, 0) < 0) {
            _PyErr_FormatNote("when serializing %T element", obj);
            Py_DECREF(item);
            Py_DECREF(iter);
            return -1;
        }
        Py_DECREF(item);
    }
    Py_DECREF(iter);

    /* If the object is already in the memo, this means it is
       recursive. In this case, throw away everything we put on the
       stack, and fetch the object back from the memo. */
    if (PyMemoTable_Get(self->memo, obj)) {
        const char pop_mark_op = POP_MARK;

        if (_Pickler_Write(self, &pop_mark_op, 1) < 0)
            return -1;
        if (memo_get(state, self, obj) < 0)
            return -1;
        return 0;
    }

    if (_Pickler_Write(self, &frozenset_op, 1) < 0)
        return -1;
    if (memo_put(state, self, obj) < 0)
        return -1;

    return 0;
}

static int
fix_imports(PickleState *st, PyObject **module_name, PyObject **global_name)
{
    PyObject *key;
    PyObject *item;

    key = PyTuple_Pack(2, *module_name, *global_name);
    if (key == NULL)
        return -1;
    item = PyDict_GetItemWithError(st->name_mapping_3to2, key);
    Py_DECREF(key);
    if (item) {
        PyObject *fixed_module_name;
        PyObject *fixed_global_name;

        if (!PyTuple_Check(item) || PyTuple_GET_SIZE(item) != 2) {
            PyErr_Format(PyExc_RuntimeError,
                         "_compat_pickle.REVERSE_NAME_MAPPING values "
                         "should be 2-tuples, not %.200s",
                         Py_TYPE(item)->tp_name);
            return -1;
        }
        fixed_module_name = PyTuple_GET_ITEM(item, 0);
        fixed_global_name = PyTuple_GET_ITEM(item, 1);
        if (!PyUnicode_Check(fixed_module_name) ||
            !PyUnicode_Check(fixed_global_name)) {
            PyErr_Format(PyExc_RuntimeError,
                         "_compat_pickle.REVERSE_NAME_MAPPING values "
                         "should be pairs of str, not (%.200s, %.200s)",
                         Py_TYPE(fixed_module_name)->tp_name,
                         Py_TYPE(fixed_global_name)->tp_name);
            return -1;
        }

        Py_CLEAR(*module_name);
        Py_CLEAR(*global_name);
        *module_name = Py_NewRef(fixed_module_name);
        *global_name = Py_NewRef(fixed_global_name);
        return 0;
    }
    else if (PyErr_Occurred()) {
        return -1;
    }

    item = PyDict_GetItemWithError(st->import_mapping_3to2, *module_name);
    if (item) {
        if (!PyUnicode_Check(item)) {
            PyErr_Format(PyExc_RuntimeError,
                         "_compat_pickle.REVERSE_IMPORT_MAPPING values "
                         "should be strings, not %.200s",
                         Py_TYPE(item)->tp_name);
            return -1;
        }
        Py_XSETREF(*module_name, Py_NewRef(item));
    }
    else if (PyErr_Occurred()) {
        return -1;
    }

    return 0;
}

static int
save_global(PickleState *st, PicklerObject *self, PyObject *obj,
            PyObject *name)
{
    PyObject *global_name = NULL;
    PyObject *module_name = NULL;
    PyObject *dotted_path = NULL;
    int status = 0;

    const char global_op = GLOBAL;

    if (name) {
        global_name = Py_NewRef(name);
    }
    else {
        if (PyObject_GetOptionalAttr(obj, &_Py_ID(__qualname__), &global_name) < 0)
            goto error;
        if (global_name == NULL) {
            global_name = PyObject_GetAttr(obj, &_Py_ID(__name__));
            if (global_name == NULL)
                goto error;
        }
    }

    dotted_path = get_dotted_path(global_name);
    if (dotted_path == NULL)
        goto error;
    module_name = whichmodule(st, obj, global_name, dotted_path);
    if (module_name == NULL)
        goto error;

    if (self->proto >= 2) {
        /* See whether this is in the extension registry, and if
         * so generate an EXT opcode.
         */
        PyObject *extension_key;
        PyObject *code_obj;      /* extension code as Python object */
        long code;               /* extension code as C value */
        char pdata[5];
        Py_ssize_t n;

        extension_key = PyTuple_Pack(2, module_name, global_name);
        if (extension_key == NULL) {
            goto error;
        }
        if (PyDict_GetItemRef(st->extension_registry, extension_key, &code_obj) < 0) {
            Py_DECREF(extension_key);
            goto error;
        }
        Py_DECREF(extension_key);
        if (code_obj == NULL) {
            /* The object is not registered in the extension registry.
               This is the most likely code path. */
            goto gen_global;
        }

        code = PyLong_AsLong(code_obj);
        Py_DECREF(code_obj);
        if (code <= 0 || code > 0x7fffffffL) {
            /* Should never happen in normal circumstances, since the type and
               the value of the code are checked in copyreg.add_extension(). */
            if (!PyErr_Occurred())
                PyErr_Format(PyExc_RuntimeError, "extension code %ld is out of range", code);
            goto error;
        }

        /* Generate an EXT opcode. */
        if (code <= 0xff) {
            pdata[0] = EXT1;
            pdata[1] = (unsigned char)code;
            n = 2;
        }
        else if (code <= 0xffff) {
            pdata[0] = EXT2;
            pdata[1] = (unsigned char)(code & 0xff);
            pdata[2] = (unsigned char)((code >> 8) & 0xff);
            n = 3;
        }
        else {
            pdata[0] = EXT4;
            pdata[1] = (unsigned char)(code & 0xff);
            pdata[2] = (unsigned char)((code >> 8) & 0xff);
            pdata[3] = (unsigned char)((code >> 16) & 0xff);
            pdata[4] = (unsigned char)((code >> 24) & 0xff);
            n = 5;
        }

        if (_Pickler_Write(self, pdata, n) < 0)
            goto error;
    }
    else {
  gen_global:
        if (self->proto >= 4) {
            const char stack_global_op = STACK_GLOBAL;

            if (save(st, self, module_name, 0) < 0)
                goto error;
            if (save(st, self, global_name, 0) < 0)
                goto error;

            if (_Pickler_Write(self, &stack_global_op, 1) < 0)
                goto error;
        }
        else {
            /* Generate a normal global opcode if we are using a pickle
               protocol < 4, or if the object is not registered in the
               extension registry.

               Objects with multi-part __qualname__ are represented as
               getattr(getattr(..., attrname1), attrname2). */
            const char mark_op = MARK;
            const char tupletwo_op = (self->proto < 2) ? TUPLE : TUPLE2;
            const char reduce_op = REDUCE;
            Py_ssize_t i;
            if (dotted_path) {
                if (PyList_GET_SIZE(dotted_path) > 1) {
                    Py_SETREF(global_name, Py_NewRef(PyList_GET_ITEM(dotted_path, 0)));
                }
                for (i = 1; i < PyList_GET_SIZE(dotted_path); i++) {
                    if (save(st, self, st->getattr, 0) < 0 ||
                        (self->proto < 2 && _Pickler_Write(self, &mark_op, 1) < 0))
                    {
                        goto error;
                    }
                }
            }

            PyObject *encoded;
            PyObject *(*unicode_encoder)(PyObject *);

            if (_Pickler_Write(self, &global_op, 1) < 0)
                goto error;

            /* For protocol < 3 and if the user didn't request against doing
               so, we convert module names to the old 2.x module names. */
            if (self->proto < 3 && self->fix_imports) {
                if (fix_imports(st, &module_name, &global_name) < 0) {
                    goto error;
                }
            }

            /* Since Python 3.0 now supports non-ASCII identifiers, we encode
               both the module name and the global name using UTF-8. We do so
               only when we are using the pickle protocol newer than version
               3. This is to ensure compatibility with older Unpickler running
               on Python 2.x. */
            if (self->proto == 3) {
                unicode_encoder = PyUnicode_AsUTF8String;
            }
            else {
                unicode_encoder = PyUnicode_AsASCIIString;
            }
            encoded = unicode_encoder(module_name);
            if (encoded == NULL) {
                if (PyErr_ExceptionMatches(PyExc_UnicodeEncodeError)) {
                    PyObject *exc = PyErr_GetRaisedException();
                    PyErr_Format(st->PicklingError,
                                 "can't pickle module identifier %R using "
                                 "pickle protocol %i",
                                 module_name, self->proto);
                    _PyErr_ChainExceptions1(exc);
                }
                goto error;
            }
            if (_Pickler_Write(self, PyBytes_AS_STRING(encoded),
                               PyBytes_GET_SIZE(encoded)) < 0) {
                Py_DECREF(encoded);
                goto error;
            }
            Py_DECREF(encoded);
            if(_Pickler_Write(self, "\n", 1) < 0)
                goto error;

            /* Save the name of the module. */
            encoded = unicode_encoder(global_name);
            if (encoded == NULL) {
                if (PyErr_ExceptionMatches(PyExc_UnicodeEncodeError)) {
                    PyObject *exc = PyErr_GetRaisedException();
                    PyErr_Format(st->PicklingError,
                                 "can't pickle global identifier %R using "
                                 "pickle protocol %i",
                                 global_name, self->proto);
                    _PyErr_ChainExceptions1(exc);
                }
                goto error;
            }
            if (_Pickler_Write(self, PyBytes_AS_STRING(encoded),
                               PyBytes_GET_SIZE(encoded)) < 0) {
                Py_DECREF(encoded);
                goto error;
            }
            Py_DECREF(encoded);
            if (_Pickler_Write(self, "\n", 1) < 0)
                goto error;

            if (dotted_path) {
                for (i = 1; i < PyList_GET_SIZE(dotted_path); i++) {
                    if (save(st, self, PyList_GET_ITEM(dotted_path, i), 0) < 0 ||
                        _Pickler_Write(self, &tupletwo_op, 1) < 0 ||
                        _Pickler_Write(self, &reduce_op, 1) < 0)
                    {
                        goto error;
                    }
                }
            }
        }
        /* Memoize the object. */
        if (memo_put(st, self, obj) < 0)
            goto error;
    }

    if (0) {
  error:
        status = -1;
    }
    Py_XDECREF(module_name);
    Py_XDECREF(global_name);
    Py_XDECREF(dotted_path);

    return status;
}

static int
save_singleton_type(PickleState *state, PicklerObject *self, PyObject *obj,
                    PyObject *singleton)
{
    PyObject *reduce_value;
    int status;

    reduce_value = Py_BuildValue("O(O)", &PyType_Type, singleton);
    if (reduce_value == NULL) {
        return -1;
    }
    status = save_reduce(state, self, reduce_value, obj);
    Py_DECREF(reduce_value);
    return status;
}

static int
save_type(PickleState *state, PicklerObject *self, PyObject *obj)
{
    if (obj == (PyObject *)&_PyNone_Type) {
        return save_singleton_type(state, self, obj, Py_None);
    }
    else if (obj == (PyObject *)&PyEllipsis_Type) {
        return save_singleton_type(state, self, obj, Py_Ellipsis);
    }
    else if (obj == (PyObject *)&_PyNotImplemented_Type) {
        return save_singleton_type(state, self, obj, Py_NotImplemented);
    }
    return save_global(state, self, obj, NULL);
}

static int
save_pers(PickleState *state, PicklerObject *self, PyObject *obj)
{
    PyObject *pid = NULL;
    int status = 0;

    const char persid_op = PERSID;
    const char binpersid_op = BINPERSID;

    pid = PyObject_CallOneArg(self->persistent_id, obj);
    if (pid == NULL)
        return -1;

    if (pid != Py_None) {
        if (self->bin) {
            if (save(state, self, pid, 1) < 0 ||
                _Pickler_Write(self, &binpersid_op, 1) < 0)
                goto error;
        }
        else {
            PyObject *pid_str;

            pid_str = PyObject_Str(pid);
            if (pid_str == NULL)
                goto error;

            /* XXX: Should it check whether the pid contains embedded
               newlines? */
            if (!PyUnicode_IS_ASCII(pid_str)) {
                PyErr_SetString(state->PicklingError,
                                "persistent IDs in protocol 0 must be "
                                "ASCII strings");
                Py_DECREF(pid_str);
                goto error;
            }

            if (_Pickler_Write(self, &persid_op, 1) < 0 ||
                _Pickler_Write(self, PyUnicode_DATA(pid_str),
                               PyUnicode_GET_LENGTH(pid_str)) < 0 ||
                _Pickler_Write(self, "\n", 1) < 0) {
                Py_DECREF(pid_str);
                goto error;
            }
            Py_DECREF(pid_str);
        }
        status = 1;
    }

    if (0) {
  error:
        status = -1;
    }
    Py_XDECREF(pid);

    return status;
}

static PyObject *
get_class(PyObject *obj)
{
    PyObject *cls;

    if (PyObject_GetOptionalAttr(obj, &_Py_ID(__class__), &cls) == 0) {
        cls = Py_NewRef(Py_TYPE(obj));
    }
    return cls;
}

/* We're saving obj, and args is the 2-thru-5 tuple returned by the
 * appropriate __reduce__ method for obj.
 */
static int
save_reduce(PickleState *st, PicklerObject *self, PyObject *args,
            PyObject *obj)
{
    PyObject *callable;
    PyObject *argtup;
    PyObject *state = NULL;
    PyObject *listitems = Py_None;
    PyObject *dictitems = Py_None;
    PyObject *state_setter = Py_None;
    Py_ssize_t size;
    int use_newobj = 0, use_newobj_ex = 0;

    const char reduce_op = REDUCE;
    const char build_op = BUILD;
    const char newobj_op = NEWOBJ;
    const char newobj_ex_op = NEWOBJ_EX;

    size = PyTuple_Size(args);
    if (size < 2 || size > 6) {
        PyErr_SetString(st->PicklingError,
                        "tuple returned by __reduce__ "
                        "must contain 2 through 6 elements");
        return -1;
    }

    if (!PyArg_UnpackTuple(args, "save_reduce", 2, 6,
                           &callable, &argtup, &state, &listitems, &dictitems,
                           &state_setter))
        return -1;

    if (!PyCallable_Check(callable)) {
        PyErr_Format(st->PicklingError,
                     "first item of the tuple returned by __reduce__ "
                     "must be callable, not %T", callable);
        return -1;
    }
    if (!PyTuple_Check(argtup)) {
        PyErr_Format(st->PicklingError,
                     "second item of the tuple returned by __reduce__ "
                     "must be a tuple, not %T", argtup);
        return -1;
    }

    if (state == Py_None)
        state = NULL;

    if (listitems == Py_None)
        listitems = NULL;
    else if (!PyIter_Check(listitems)) {
        PyErr_Format(st->PicklingError,
                     "fourth item of the tuple returned by __reduce__ "
                     "must be an iterator, not %T", listitems);
        return -1;
    }

    if (dictitems == Py_None)
        dictitems = NULL;
    else if (!PyIter_Check(dictitems)) {
        PyErr_Format(st->PicklingError,
                     "fifth item of the tuple returned by __reduce__ "
                     "must be an iterator, not %T", dictitems);
        return -1;
    }

    if (state_setter == Py_None)
        state_setter = NULL;
    else if (!PyCallable_Check(state_setter)) {
        PyErr_Format(st->PicklingError,
                     "sixth item of the tuple returned by __reduce__ "
                     "must be callable, not %T", state_setter);
        return -1;
    }

    if (self->proto >= 2) {
        PyObject *name;

        if (PyObject_GetOptionalAttr(callable, &_Py_ID(__name__), &name) < 0) {
            return -1;
        }
        if (name != NULL && PyUnicode_Check(name)) {
            use_newobj_ex = _PyUnicode_Equal(name, &_Py_ID(__newobj_ex__));
            if (!use_newobj_ex) {
                use_newobj = _PyUnicode_Equal(name, &_Py_ID(__newobj__));
            }
        }
        Py_XDECREF(name);
    }

    if (use_newobj_ex) {
        PyObject *cls;
        PyObject *args;
        PyObject *kwargs;

        if (PyTuple_GET_SIZE(argtup) != 3) {
            PyErr_Format(st->PicklingError,
                         "__newobj_ex__ expected 3 arguments, got %zd",
                         PyTuple_GET_SIZE(argtup));
            return -1;
        }

        cls = PyTuple_GET_ITEM(argtup, 0);
        if (!PyType_Check(cls)) {
            PyErr_Format(st->PicklingError,
                         "first argument to __newobj_ex__() "
                         "must be a class, not %T", cls);
            return -1;
        }
        args = PyTuple_GET_ITEM(argtup, 1);
        if (!PyTuple_Check(args)) {
            PyErr_Format(st->PicklingError,
                         "second argument to __newobj_ex__() "
                         "must be a tuple, not %T", args);
            return -1;
        }
        kwargs = PyTuple_GET_ITEM(argtup, 2);
        if (!PyDict_Check(kwargs)) {
            PyErr_Format(st->PicklingError,
                         "third argument to __newobj_ex__() "
                         "must be a dict, not %T", kwargs);
            return -1;
        }

        if (self->proto >= 4) {
            if (save(st, self, cls, 0) < 0) {
                _PyErr_FormatNote("when serializing %T class", obj);
                return -1;
            }
            if (save(st, self, args, 0) < 0 ||
                save(st, self, kwargs, 0) < 0)
            {
                _PyErr_FormatNote("when serializing %T __new__ arguments", obj);
                return -1;
            }
            if (_Pickler_Write(self, &newobj_ex_op, 1) < 0) {
                return -1;
            }
        }
        else {
            PyObject *newargs;
            PyObject *cls_new;
            Py_ssize_t i;

            newargs = PyTuple_New(PyTuple_GET_SIZE(args) + 2);
            if (newargs == NULL)
                return -1;

            cls_new = PyObject_GetAttr(cls, &_Py_ID(__new__));
            if (cls_new == NULL) {
                Py_DECREF(newargs);
                return -1;
            }
            PyTuple_SET_ITEM(newargs, 0, cls_new);
            PyTuple_SET_ITEM(newargs, 1, Py_NewRef(cls));
            for (i = 0; i < PyTuple_GET_SIZE(args); i++) {
                PyObject *item = PyTuple_GET_ITEM(args, i);
                PyTuple_SET_ITEM(newargs, i + 2, Py_NewRef(item));
            }

            callable = PyObject_Call(st->partial, newargs, kwargs);
            Py_DECREF(newargs);
            if (callable == NULL)
                return -1;

            newargs = PyTuple_New(0);
            if (newargs == NULL) {
                Py_DECREF(callable);
                return -1;
            }

            if (save(st, self, callable, 0) < 0 ||
                save(st, self, newargs, 0) < 0)
            {
                _PyErr_FormatNote("when serializing %T reconstructor", obj);
                Py_DECREF(newargs);
                Py_DECREF(callable);
                return -1;
            }
            Py_DECREF(newargs);
            Py_DECREF(callable);
            if (_Pickler_Write(self, &reduce_op, 1) < 0) {
                return -1;
            }
        }
    }
    else if (use_newobj) {
        PyObject *cls;
        PyObject *newargtup;
        PyObject *obj_class;
        int p;

        /* Sanity checks. */
        if (PyTuple_GET_SIZE(argtup) < 1) {
            PyErr_Format(st->PicklingError,
                         "__newobj__ expected at least 1 argument, got %zd",
                         PyTuple_GET_SIZE(argtup));
            return -1;
        }

        cls = PyTuple_GET_ITEM(argtup, 0);
        if (!PyType_Check(cls)) {
            PyErr_Format(st->PicklingError,
                         "first argument to __newobj__() "
                         "must be a class, not %T", cls);
            return -1;
        }

        if (obj != NULL) {
            obj_class = get_class(obj);
            if (obj_class == NULL) {
                return -1;
            }
            if (obj_class != cls) {
                PyErr_Format(st->PicklingError,
                             "first argument to __newobj__() "
                             "must be %R, not %R", obj_class, cls);
                Py_DECREF(obj_class);
                return -1;
            }
            Py_DECREF(obj_class);
        }
        /* XXX: These calls save() are prone to infinite recursion. Imagine
           what happen if the value returned by the __reduce__() method of
           some extension type contains another object of the same type. Ouch!

           Here is a quick example, that I ran into, to illustrate what I
           mean:

             >>> import pickle, copyreg
             >>> copyreg.dispatch_table.pop(complex)
             >>> pickle.dumps(1+2j)
             Traceback (most recent call last):
               ...
             RecursionError: maximum recursion depth exceeded

           Removing the complex class from copyreg.dispatch_table made the
           __reduce_ex__() method emit another complex object:

             >>> (1+1j).__reduce_ex__(2)
             (<function __newobj__ at 0xb7b71c3c>,
               (<class 'complex'>, (1+1j)), None, None, None)

           Thus when save() was called on newargstup (the 2nd item) recursion
           ensued. Of course, the bug was in the complex class which had a
           broken __getnewargs__() that emitted another complex object. But,
           the point, here, is it is quite easy to end up with a broken reduce
           function. */

        /* Save the class and its __new__ arguments. */
        if (save(st, self, cls, 0) < 0) {
            _PyErr_FormatNote("when serializing %T class", obj);
            return -1;
        }

        newargtup = PyTuple_GetSlice(argtup, 1, PyTuple_GET_SIZE(argtup));
        if (newargtup == NULL)
            return -1;

        p = save(st, self, newargtup, 0);
        Py_DECREF(newargtup);
        if (p < 0) {
            _PyErr_FormatNote("when serializing %T __new__ arguments", obj);
            return -1;
        }

        /* Add NEWOBJ opcode. */
        if (_Pickler_Write(self, &newobj_op, 1) < 0)
            return -1;
    }
    else { /* Not using NEWOBJ. */
        if (save(st, self, callable, 0) < 0) {
            _PyErr_FormatNote("when serializing %T reconstructor", obj);
            return -1;
        }
        if (save(st, self, argtup, 0) < 0) {
            _PyErr_FormatNote("when serializing %T reconstructor arguments", obj);
            return -1;
        }
        if (_Pickler_Write(self, &reduce_op, 1) < 0) {
            return -1;
        }
    }

    /* obj can be NULL when save_reduce() is used directly. A NULL obj means
       the caller do not want to memoize the object. Not particularly useful,
       but that is to mimic the behavior save_reduce() in pickle.py when
       obj is None. */
    if (obj != NULL) {
        /* If the object is already in the memo, this means it is
           recursive. In this case, throw away everything we put on the
           stack, and fetch the object back from the memo. */
        if (PyMemoTable_Get(self->memo, obj)) {
            const char pop_op = POP;

            if (_Pickler_Write(self, &pop_op, 1) < 0)
                return -1;
            if (memo_get(st, self, obj) < 0)
                return -1;

            return 0;
        }
        else if (memo_put(st, self, obj) < 0)
            return -1;
    }

    if (listitems && batch_list(st, self, listitems, obj) < 0)
        return -1;

    if (dictitems && batch_dict(st, self, dictitems, obj) < 0)
        return -1;

    if (state) {
        if (state_setter == NULL) {
            if (save(st, self, state, 0) < 0) {
                _PyErr_FormatNote("when serializing %T state", obj);
                return -1;
            }
            if (_Pickler_Write(self, &build_op, 1) < 0)
                return -1;
        }
        else {

            /* If a state_setter is specified, call it instead of load_build to
             * update obj's with its previous state.
             * The first 4 save/write instructions push state_setter and its
             * tuple of expected arguments (obj, state) onto the stack. The
             * REDUCE opcode triggers the state_setter(obj, state) function
             * call. Finally, because state-updating routines only do in-place
             * modification, the whole operation has to be stack-transparent.
             * Thus, we finally pop the call's output from the stack.*/

            const char tupletwo_op = TUPLE2;
            const char pop_op = POP;
            if (save(st, self, state_setter, 0) < 0) {
                _PyErr_FormatNote("when serializing %T state setter", obj);
                return -1;
            }
            if (save(st, self, obj, 0) < 0) {
                return -1;
            }
            if (save(st, self, state, 0) < 0) {
                _PyErr_FormatNote("when serializing %T state", obj);
                return -1;
            }
            if (_Pickler_Write(self, &tupletwo_op, 1) < 0 ||
                _Pickler_Write(self, &reduce_op, 1) < 0 ||
                _Pickler_Write(self, &pop_op, 1) < 0)
                return -1;
        }
    }
    return 0;
}

static int
save(PickleState *st, PicklerObject *self, PyObject *obj, int pers_save)
{
    PyTypeObject *type;
    PyObject *reduce_func = NULL;
    PyObject *reduce_value = NULL;
    int status = 0;

    if (_Pickler_OpcodeBoundary(self) < 0)
        return -1;

    /* The extra pers_save argument is necessary to avoid calling save_pers()
       on its returned object. */
    if (!pers_save && self->persistent_id) {
        /* save_pers() returns:
            -1   to signal an error;
             0   if it did nothing successfully;
             1   if a persistent id was saved.
         */
        if ((status = save_pers(st, self, obj)) != 0)
            return status;
    }

    type = Py_TYPE(obj);

    /* The old cPickle had an optimization that used switch-case statement
       dispatching on the first letter of the type name.  This has was removed
       since benchmarks shown that this optimization was actually slowing
       things down. */

    /* Atom types; these aren't memoized, so don't check the memo. */

    if (obj == Py_None) {
        return save_none(self, obj);
    }
    else if (obj == Py_False || obj == Py_True) {
        return save_bool(self, obj);
    }
    else if (type == &PyLong_Type) {
        return save_long(self, obj);
    }
    else if (type == &PyFloat_Type) {
        return save_float(self, obj);
    }

    /* Check the memo to see if it has the object. If so, generate
       a GET (or BINGET) opcode, instead of pickling the object
       once again. */
    if (PyMemoTable_Get(self->memo, obj)) {
        return memo_get(st, self, obj);
    }

    if (type == &PyBytes_Type) {
        return save_bytes(st, self, obj);
    }
    else if (type == &PyUnicode_Type) {
        return save_unicode(st, self, obj);
    }

    /* We're only calling _Py_EnterRecursiveCall here so that atomic
       types above are pickled faster. */
    if (_Py_EnterRecursiveCall(" while pickling an object")) {
        return -1;
    }

    if (type == &PyDict_Type) {
        status = save_dict(st, self, obj);
        goto done;
    }
    else if (type == &PySet_Type) {
        status = save_set(st, self, obj);
        goto done;
    }
    else if (type == &PyFrozenSet_Type) {
        status = save_frozenset(st, self, obj);
        goto done;
    }
    else if (type == &PyList_Type) {
        status = save_list(st, self, obj);
        goto done;
    }
    else if (type == &PyTuple_Type) {
        status = save_tuple(st, self, obj);
        goto done;
    }
    else if (type == &PyByteArray_Type) {
        status = save_bytearray(st, self, obj);
        goto done;
    }
    else if (type == &PyPickleBuffer_Type) {
        status = save_picklebuffer(st, self, obj);
        goto done;
    }

    /* Now, check reducer_override.  If it returns NotImplemented,
     * fallback to save_type or save_global, and then perhaps to the
     * regular reduction mechanism.
     */
    if (self->reducer_override != NULL) {
        reduce_value = PyObject_CallOneArg(self->reducer_override, obj);
        if (reduce_value == NULL) {
            goto error;
        }
        if (reduce_value != Py_NotImplemented) {
            goto reduce;
        }
        Py_SETREF(reduce_value, NULL);
    }

    if (type == &PyType_Type) {
        status = save_type(st, self, obj);
        goto done;
    }
    else if (type == &PyFunction_Type) {
        status = save_global(st, self, obj, NULL);
        goto done;
    }

    /* XXX: This part needs some unit tests. */

    /* Get a reduction callable, and call it.  This may come from
     * self.dispatch_table, copyreg.dispatch_table, the object's
     * __reduce_ex__ method, or the object's __reduce__ method.
     */
    if (self->dispatch_table == NULL) {
        reduce_func = PyDict_GetItemWithError(st->dispatch_table,
                                              (PyObject *)type);
        if (reduce_func == NULL) {
            if (PyErr_Occurred()) {
                goto error;
            }
        } else {
            /* PyDict_GetItemWithError() returns a borrowed reference.
               Increase the reference count to be consistent with
               PyObject_GetItem and _PyObject_GetAttrId used below. */
            Py_INCREF(reduce_func);
        }
    }
    else if (PyMapping_GetOptionalItem(self->dispatch_table, (PyObject *)type,
                                       &reduce_func) < 0)
    {
        goto error;
    }

    if (reduce_func != NULL) {
        reduce_value = _Pickle_FastCall(reduce_func, Py_NewRef(obj));
    }
    else if (PyType_IsSubtype(type, &PyType_Type)) {
        status = save_global(st, self, obj, NULL);
        goto done;
    }
    else {
        /* XXX: If the __reduce__ method is defined, __reduce_ex__ is
           automatically defined as __reduce__. While this is convenient, this
           make it impossible to know which method was actually called. Of
           course, this is not a big deal. But still, it would be nice to let
           the user know which method was called when something go
           wrong. Incidentally, this means if __reduce_ex__ is not defined, we
           don't actually have to check for a __reduce__ method. */

        /* Check for a __reduce_ex__ method. */
        if (PyObject_GetOptionalAttr(obj, &_Py_ID(__reduce_ex__), &reduce_func) < 0) {
            goto error;
        }
        if (reduce_func != NULL) {
            PyObject *proto;
            proto = PyLong_FromLong(self->proto);
            if (proto != NULL) {
                reduce_value = _Pickle_FastCall(reduce_func, proto);
            }
        }
        else {
            /* Check for a __reduce__ method. */
            if (PyObject_GetOptionalAttr(obj, &_Py_ID(__reduce__), &reduce_func) < 0) {
                goto error;
            }
            if (reduce_func != NULL) {
                reduce_value = PyObject_CallNoArgs(reduce_func);
            }
            else {
                PyErr_Format(st->PicklingError,
                             "Can't pickle %T object", obj);
                goto error;
            }
        }
    }

    if (reduce_value == NULL)
        goto error;

  reduce:
    if (PyUnicode_Check(reduce_value)) {
        status = save_global(st, self, obj, reduce_value);
        goto done;
    }

    if (!PyTuple_Check(reduce_value)) {
        PyErr_Format(st->PicklingError,
                     "__reduce__ must return a string or tuple, not %T", reduce_value);
        _PyErr_FormatNote("when serializing %T object", obj);
        goto error;
    }

    status = save_reduce(st, self, reduce_value, obj);
    if (status < 0) {
        _PyErr_FormatNote("when serializing %T object", obj);
    }

    if (0) {
  error:
        status = -1;
    }
  done:

    _Py_LeaveRecursiveCall();
    Py_XDECREF(reduce_func);
    Py_XDECREF(reduce_value);

    return status;
}

static PyObject *
persistent_id(PyObject *self, PyObject *obj)
{
    Py_RETURN_NONE;
}

static int
dump(PickleState *state, PicklerObject *self, PyObject *obj)
{
    const char stop_op = STOP;
    int status = -1;
    PyObject *tmp;

    /* Cache the persistent_id method. */
    tmp = PyObject_GetAttr((PyObject *)self, &_Py_ID(persistent_id));
    if (tmp == NULL) {
        goto error;
    }
    if (PyCFunction_Check(tmp) &&
        PyCFunction_GET_SELF(tmp) == (PyObject *)self &&
        PyCFunction_GET_FUNCTION(tmp) == persistent_id)
    {
        Py_CLEAR(tmp);
    }
    Py_XSETREF(self->persistent_id, tmp);

    /* Cache the reducer_override method, if it exists. */
    if (PyObject_GetOptionalAttr((PyObject *)self, &_Py_ID(reducer_override),
                             &tmp) < 0) {
        goto error;
    }
    Py_XSETREF(self->reducer_override, tmp);

    if (self->proto >= 2) {
        char header[2];

        header[0] = PROTO;
        assert(self->proto >= 0 && self->proto < 256);
        header[1] = (unsigned char)self->proto;
        if (_Pickler_Write(self, header, 2) < 0)
            goto error;
        if (self->proto >= 4)
            self->framing = 1;
    }

    if (save(state, self, obj, 0) < 0 ||
        _Pickler_Write(self, &stop_op, 1) < 0 ||
        _Pickler_CommitFrame(self) < 0)
        goto error;

    // Success
    status = 0;

  error:
    self->framing = 0;

    /* Break the reference cycle we generated at the beginning this function
     * call when setting the persistent_id and the reducer_override attributes
     * of the Pickler instance to a bound method of the same instance.
     * This is important as the Pickler instance holds a reference to each
     * object it has pickled (through its memo): thus, these objects won't
     * be garbage-collected as long as the Pickler itself is not collected. */
    Py_CLEAR(self->persistent_id);
    Py_CLEAR(self->reducer_override);
    return status;
}

/*[clinic input]

_pickle.Pickler.clear_memo

Clears the pickler's "memo".

The memo is the data structure that remembers which objects the
pickler has already seen, so that shared or recursive objects are
pickled by reference and not by value.  This method is useful when
re-using picklers.
[clinic start generated code]*/

static PyObject *
_pickle_Pickler_clear_memo_impl(PicklerObject *self)
/*[clinic end generated code: output=8665c8658aaa094b input=01bdad52f3d93e56]*/
{
    if (self->memo)
        PyMemoTable_Clear(self->memo);

    Py_RETURN_NONE;
}

/*[clinic input]

_pickle.Pickler.dump

  cls: defining_class
  obj: object
  /

Write a pickled representation of the given object to the open file.
[clinic start generated code]*/

static PyObject *
_pickle_Pickler_dump_impl(PicklerObject *self, PyTypeObject *cls,
                          PyObject *obj)
/*[clinic end generated code: output=952cf7f68b1445bb input=f949d84151983594]*/
{
    PickleState *st = _Pickle_GetStateByClass(cls);
    /* Check whether the Pickler was initialized correctly (issue3664).
       Developers often forget to call __init__() in their subclasses, which
       would trigger a segfault without this check. */
    if (self->write == NULL) {
        PyErr_Format(st->PicklingError,
                     "Pickler.__init__() was not called by %s.__init__()",
                     Py_TYPE(self)->tp_name);
        return NULL;
    }

    if (_Pickler_ClearBuffer(self) < 0)
        return NULL;

    if (dump(st, self, obj) < 0)
        return NULL;

    if (_Pickler_FlushToFile(self) < 0)
        return NULL;

    Py_RETURN_NONE;
}

/*[clinic input]

_pickle.Pickler.__sizeof__ -> size_t

Returns size in memory, in bytes.
[clinic start generated code]*/

static size_t
_pickle_Pickler___sizeof___impl(PicklerObject *self)
/*[clinic end generated code: output=23ad75658d3b59ff input=d8127c8e7012ebd7]*/
{
    size_t res = _PyObject_SIZE(Py_TYPE(self));
    if (self->memo != NULL) {
        res += sizeof(PyMemoTable);
        res += self->memo->mt_allocated * sizeof(PyMemoEntry);
    }
    if (self->output_buffer != NULL) {
        size_t s = _PySys_GetSizeOf(self->output_buffer);
        if (s == (size_t)-1) {
            return -1;
        }
        res += s;
    }
    return res;
}

static struct PyMethodDef Pickler_methods[] = {
    {"persistent_id", persistent_id, METH_O,
        PyDoc_STR("persistent_id($self, obj, /)\n--\n\n")},
    _PICKLE_PICKLER_DUMP_METHODDEF
    _PICKLE_PICKLER_CLEAR_MEMO_METHODDEF
    _PICKLE_PICKLER___SIZEOF___METHODDEF
    {NULL, NULL}                /* sentinel */
};

static int
Pickler_clear(PyObject *op)
{
    PicklerObject *self = PicklerObject_CAST(op);
    Py_CLEAR(self->output_buffer);
    Py_CLEAR(self->write);
    Py_CLEAR(self->persistent_id);
    Py_CLEAR(self->persistent_id_attr);
    Py_CLEAR(self->dispatch_table);
    Py_CLEAR(self->fast_memo);
    Py_CLEAR(self->reducer_override);
    Py_CLEAR(self->buffer_callback);

    if (self->memo != NULL) {
        PyMemoTable *memo = self->memo;
        self->memo = NULL;
        PyMemoTable_Del(memo);
    }
    return 0;
}

static void
Pickler_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    (void)Pickler_clear(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static int
Pickler_traverse(PyObject *op, visitproc visit, void *arg)
{
    PicklerObject *self = PicklerObject_CAST(op);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->write);
    Py_VISIT(self->persistent_id);
    Py_VISIT(self->persistent_id_attr);
    Py_VISIT(self->dispatch_table);
    Py_VISIT(self->fast_memo);
    Py_VISIT(self->reducer_override);
    Py_VISIT(self->buffer_callback);
    PyMemoTable *memo = self->memo;
    if (memo && memo->mt_table) {
        Py_ssize_t i = memo->mt_allocated;
        while (--i >= 0) {
            Py_VISIT(memo->mt_table[i].me_key);
        }
    }

    return 0;
}


/*[clinic input]

_pickle.Pickler.__init__

  file: object
  protocol: object = None
  fix_imports: bool = True
  buffer_callback: object = None

This takes a binary file for writing a pickle data stream.

The optional *protocol* argument tells the pickler to use the given
protocol; supported protocols are 0, 1, 2, 3, 4 and 5.  The default
protocol is 5. It was introduced in Python 3.8, and is incompatible
with previous versions.

Specifying a negative protocol version selects the highest protocol
version supported.  The higher the protocol used, the more recent the
version of Python needed to read the pickle produced.

The *file* argument must have a write() method that accepts a single
bytes argument. It can thus be a file object opened for binary
writing, an io.BytesIO instance, or any other custom object that meets
this interface.

If *fix_imports* is True and protocol is less than 3, pickle will try
to map the new Python 3 names to the old module names used in Python
2, so that the pickle data stream is readable with Python 2.

If *buffer_callback* is None (the default), buffer views are
serialized into *file* as part of the pickle stream.

If *buffer_callback* is not None, then it can be called any number
of times with a buffer view.  If the callback returns a false value
(such as None), the given buffer is out-of-band; otherwise the
buffer is serialized in-band, i.e. inside the pickle stream.

It is an error if *buffer_callback* is not None and *protocol*
is None or smaller than 5.

[clinic start generated code]*/

static int
_pickle_Pickler___init___impl(PicklerObject *self, PyObject *file,
                              PyObject *protocol, int fix_imports,
                              PyObject *buffer_callback)
/*[clinic end generated code: output=0abedc50590d259b input=cddc50f66b770002]*/
{
    /* In case of multiple __init__() calls, clear previous content. */
    if (self->write != NULL)
        (void)Pickler_clear((PyObject *)self);

    if (_Pickler_SetProtocol(self, protocol, fix_imports) < 0)
        return -1;

    if (_Pickler_SetOutputStream(self, file) < 0)
        return -1;

    if (_Pickler_SetBufferCallback(self, buffer_callback) < 0)
        return -1;

    /* memo and output_buffer may have already been created in _Pickler_New */
    if (self->memo == NULL) {
        self->memo = PyMemoTable_New();
        if (self->memo == NULL)
            return -1;
    }
    self->output_len = 0;
    if (self->output_buffer == NULL) {
        self->max_output_len = WRITE_BUF_SIZE;
        self->output_buffer = PyBytes_FromStringAndSize(NULL,
                                                        self->max_output_len);
        if (self->output_buffer == NULL)
            return -1;
    }

    self->fast = 0;
    self->fast_nesting = 0;
    self->fast_memo = NULL;

    if (self->dispatch_table != NULL) {
        return 0;
    }
    if (PyObject_GetOptionalAttr((PyObject *)self, &_Py_ID(dispatch_table),
                             &self->dispatch_table) < 0) {
        return -1;
    }

    return 0;
}


/* Define a proxy object for the Pickler's internal memo object. This is to
 * avoid breaking code like:
 *  pickler.memo.clear()
 * and
 *  pickler.memo = saved_memo
 * Is this a good idea? Not really, but we don't want to break code that uses
 * it. Note that we don't implement the entire mapping API here. This is
 * intentional, as these should be treated as black-box implementation details.
 */

/*[clinic input]
_pickle.PicklerMemoProxy.clear

Remove all items from memo.
[clinic start generated code]*/

static PyObject *
_pickle_PicklerMemoProxy_clear_impl(PicklerMemoProxyObject *self)
/*[clinic end generated code: output=5fb9370d48ae8b05 input=ccc186dacd0f1405]*/
{
    if (self->pickler->memo)
        PyMemoTable_Clear(self->pickler->memo);
    Py_RETURN_NONE;
}

/*[clinic input]
_pickle.PicklerMemoProxy.copy

Copy the memo to a new object.
[clinic start generated code]*/

static PyObject *
_pickle_PicklerMemoProxy_copy_impl(PicklerMemoProxyObject *self)
/*[clinic end generated code: output=bb83a919d29225ef input=b73043485ac30b36]*/
{
    PyMemoTable *memo;
    PyObject *new_memo = PyDict_New();
    if (new_memo == NULL)
        return NULL;

    memo = self->pickler->memo;
    for (size_t i = 0; i < memo->mt_allocated; ++i) {
        PyMemoEntry entry = memo->mt_table[i];
        if (entry.me_key != NULL) {
            int status;
            PyObject *key, *value;

            key = PyLong_FromVoidPtr(entry.me_key);
            if (key == NULL) {
                goto error;
            }
            value = Py_BuildValue("nO", entry.me_value, entry.me_key);
            if (value == NULL) {
                Py_DECREF(key);
                goto error;
            }
            status = PyDict_SetItem(new_memo, key, value);
            Py_DECREF(key);
            Py_DECREF(value);
            if (status < 0)
                goto error;
        }
    }
    return new_memo;

  error:
    Py_XDECREF(new_memo);
    return NULL;
}

/*[clinic input]
_pickle.PicklerMemoProxy.__reduce__

Implement pickle support.
[clinic start generated code]*/

static PyObject *
_pickle_PicklerMemoProxy___reduce___impl(PicklerMemoProxyObject *self)
/*[clinic end generated code: output=bebba1168863ab1d input=2f7c540e24b7aae4]*/
{
    PyObject *reduce_value, *dict_args;
    PyObject *contents = _pickle_PicklerMemoProxy_copy_impl(self);
    if (contents == NULL)
        return NULL;

    reduce_value = PyTuple_New(2);
    if (reduce_value == NULL) {
        Py_DECREF(contents);
        return NULL;
    }
    dict_args = PyTuple_New(1);
    if (dict_args == NULL) {
        Py_DECREF(contents);
        Py_DECREF(reduce_value);
        return NULL;
    }
    PyTuple_SET_ITEM(dict_args, 0, contents);
    PyTuple_SET_ITEM(reduce_value, 0, Py_NewRef(&PyDict_Type));
    PyTuple_SET_ITEM(reduce_value, 1, dict_args);
    return reduce_value;
}

static PyMethodDef picklerproxy_methods[] = {
    _PICKLE_PICKLERMEMOPROXY_CLEAR_METHODDEF
    _PICKLE_PICKLERMEMOPROXY_COPY_METHODDEF
    _PICKLE_PICKLERMEMOPROXY___REDUCE___METHODDEF
    {NULL, NULL} /* sentinel */
};

static void
PicklerMemoProxy_dealloc(PyObject *op)
{
    PicklerMemoProxyObject *self = PicklerMemoProxyObject_CAST(op);
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    Py_CLEAR(self->pickler);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static int
PicklerMemoProxy_traverse(PyObject *op, visitproc visit, void *arg)
{
    PicklerMemoProxyObject *self = PicklerMemoProxyObject_CAST(op);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->pickler);
    return 0;
}

static int
PicklerMemoProxy_clear(PyObject *op)
{
    PicklerMemoProxyObject *self = PicklerMemoProxyObject_CAST(op);
    Py_CLEAR(self->pickler);
    return 0;
}

static PyType_Slot memoproxy_slots[] = {
    {Py_tp_dealloc, PicklerMemoProxy_dealloc},
    {Py_tp_traverse, PicklerMemoProxy_traverse},
    {Py_tp_clear, PicklerMemoProxy_clear},
    {Py_tp_methods, picklerproxy_methods},
    {Py_tp_hash, PyObject_HashNotImplemented},
    {0, NULL},
};

static PyType_Spec memoproxy_spec = {
    .name = "_pickle.PicklerMemoProxy",
    .basicsize = sizeof(PicklerMemoProxyObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = memoproxy_slots,
};

static PyObject *
PicklerMemoProxy_New(PicklerObject *pickler)
{
    PicklerMemoProxyObject *self;
    PickleState *st = _Pickle_FindStateByType(Py_TYPE(pickler));
    self = PyObject_GC_New(PicklerMemoProxyObject, st->PicklerMemoProxyType);
    if (self == NULL)
        return NULL;
    self->pickler = (PicklerObject*)Py_NewRef(pickler);
    PyObject_GC_Track(self);
    return (PyObject *)self;
}

/*****************************************************************************/

static PyObject *
Pickler_get_memo(PyObject *op, void *Py_UNUSED(closure))
{
    PicklerObject *self = PicklerObject_CAST(op);
    return PicklerMemoProxy_New(self);
}

static int
Pickler_set_memo(PyObject *op, PyObject *obj, void *Py_UNUSED(closure))
{
    PyMemoTable *new_memo = NULL;
    PicklerObject *self = PicklerObject_CAST(op);

    if (obj == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "attribute deletion is not supported");
        return -1;
    }

    PickleState *st = _Pickle_FindStateByType(Py_TYPE(self));
    if (Py_IS_TYPE(obj, st->PicklerMemoProxyType)) {
        PicklerObject *pickler = /* safe fast cast for 'obj' */
            ((PicklerMemoProxyObject *)obj)->pickler;

        new_memo = PyMemoTable_Copy(pickler->memo);
        if (new_memo == NULL)
            return -1;
    }
    else if (PyDict_Check(obj)) {
        Py_ssize_t i = 0;
        PyObject *key, *value;

        new_memo = PyMemoTable_New();
        if (new_memo == NULL)
            return -1;

        while (PyDict_Next(obj, &i, &key, &value)) {
            Py_ssize_t memo_id;
            PyObject *memo_obj;

            if (!PyTuple_Check(value) || PyTuple_GET_SIZE(value) != 2) {
                PyErr_SetString(PyExc_TypeError,
                                "'memo' values must be 2-item tuples");
                goto error;
            }
            memo_id = PyLong_AsSsize_t(PyTuple_GET_ITEM(value, 0));
            if (memo_id == -1 && PyErr_Occurred())
                goto error;
            memo_obj = PyTuple_GET_ITEM(value, 1);
            if (PyMemoTable_Set(new_memo, memo_obj, memo_id) < 0)
                goto error;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "'memo' attribute must be a PicklerMemoProxy object "
                     "or dict, not %.200s", Py_TYPE(obj)->tp_name);
        return -1;
    }

    PyMemoTable_Del(self->memo);
    self->memo = new_memo;

    return 0;

  error:
    if (new_memo)
        PyMemoTable_Del(new_memo);
    return -1;
}

static PyObject *
Pickler_getattr(PyObject *self, PyObject *name)
{
    PicklerObject *po = PicklerObject_CAST(self);
    if (PyUnicode_Check(name)
        && PyUnicode_EqualToUTF8(name, "persistent_id")
        && po->persistent_id_attr)
    {
        return Py_NewRef(po->persistent_id_attr);
    }

    return PyObject_GenericGetAttr(self, name);
}

static int
Pickler_setattr(PyObject *self, PyObject *name, PyObject *value)
{
    if (PyUnicode_Check(name)
        && PyUnicode_EqualToUTF8(name, "persistent_id"))
    {
        PicklerObject *po = PicklerObject_CAST(self);
        Py_XINCREF(value);
        Py_XSETREF(po->persistent_id_attr, value);
        return 0;
    }

    return PyObject_GenericSetAttr(self, name, value);
}

static PyMemberDef Pickler_members[] = {
    {"bin", Py_T_INT, offsetof(PicklerObject, bin)},
    {"fast", Py_T_INT, offsetof(PicklerObject, fast)},
    {"dispatch_table", Py_T_OBJECT_EX, offsetof(PicklerObject, dispatch_table)},
    {NULL}
};

static PyGetSetDef Pickler_getsets[] = {
    {"memo", Pickler_get_memo, Pickler_set_memo},
    {NULL}
};

static PyType_Slot pickler_type_slots[] = {
    {Py_tp_dealloc, Pickler_dealloc},
    {Py_tp_getattro, Pickler_getattr},
    {Py_tp_setattro, Pickler_setattr},
    {Py_tp_methods, Pickler_methods},
    {Py_tp_members, Pickler_members},
    {Py_tp_getset, Pickler_getsets},
    {Py_tp_clear, Pickler_clear},
    {Py_tp_doc, (char*)_pickle_Pickler___init____doc__},
    {Py_tp_traverse, Pickler_traverse},
    {Py_tp_init, _pickle_Pickler___init__},
    {Py_tp_new, PyType_GenericNew},
    {Py_tp_alloc, PyType_GenericAlloc},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec pickler_type_spec = {
    .name = "_pickle.Pickler",
    .basicsize = sizeof(PicklerObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = pickler_type_slots,
};

/* Temporary helper for calling self.find_class().

   XXX: It would be nice to able to avoid Python function call overhead, by
   using directly the C version of find_class(), when find_class() is not
   overridden by a subclass. Although, this could become rather hackish. A
   simpler optimization would be to call the C function when self is not a
   subclass instance. */
static PyObject *
find_class(UnpicklerObject *self, PyObject *module_name, PyObject *global_name)
{
    return PyObject_CallMethodObjArgs((PyObject *)self, &_Py_ID(find_class),
                                      module_name, global_name, NULL);
}

static Py_ssize_t
marker(PickleState *st, UnpicklerObject *self)
{
    if (self->num_marks < 1) {
        PyErr_SetString(st->UnpicklingError, "could not find MARK");
        return -1;
    }

    Py_ssize_t mark = self->marks[--self->num_marks];
    self->stack->mark_set = self->num_marks != 0;
    self->stack->fence = self->num_marks ?
            self->marks[self->num_marks - 1] : 0;
    return mark;
}

static int
load_none(PickleState *state, UnpicklerObject *self)
{
    PDATA_APPEND(self->stack, Py_None, -1);
    return 0;
}

static int
load_int(PickleState *state, UnpicklerObject *self)
{
    PyObject *value;
    char *endptr, *s;
    Py_ssize_t len;
    long x;

    if ((len = _Unpickler_Readline(state, self, &s)) < 0)
        return -1;
    if (len < 2)
        return bad_readline(state);

    errno = 0;
    /* XXX(avassalotti): Should this uses PyOS_strtol()? */
    x = strtol(s, &endptr, 10);

    if (errno || (*endptr != '\n' && *endptr != '\0')) {
        /* Hm, maybe we've got something long.  Let's try reading
         * it as a Python int object. */
        errno = 0;
        value = PyLong_FromString(s, NULL, 10);
        if (value == NULL) {
            PyErr_SetString(PyExc_ValueError,
                            "could not convert string to int");
            return -1;
        }
    }
    else {
        if (len == 3 && (x == 0 || x == 1)) {
            if ((value = PyBool_FromLong(x)) == NULL)
                return -1;
        }
        else {
            if ((value = PyLong_FromLong(x)) == NULL)
                return -1;
        }
    }

    PDATA_PUSH(self->stack, value, -1);
    return 0;
}

static int
load_bool(PickleState *state, UnpicklerObject *self, PyObject *boolean)
{
    assert(boolean == Py_True || boolean == Py_False);
    PDATA_APPEND(self->stack, boolean, -1);
    return 0;
}

/* s contains x bytes of an unsigned little-endian integer.  Return its value
 * as a C Py_ssize_t, or -1 if it's higher than PY_SSIZE_T_MAX.
 */
static Py_ssize_t
calc_binsize(char *bytes, int nbytes)
{
    unsigned char *s = (unsigned char *)bytes;
    int i;
    size_t x = 0;

    if (nbytes > (int)sizeof(size_t)) {
        /* Check for integer overflow.  BINBYTES8 and BINUNICODE8 opcodes
         * have 64-bit size that can't be represented on 32-bit platform.
         */
        for (i = (int)sizeof(size_t); i < nbytes; i++) {
            if (s[i])
                return -1;
        }
        nbytes = (int)sizeof(size_t);
    }
    for (i = 0; i < nbytes; i++) {
        x |= (size_t) s[i] << (8 * i);
    }

    if (x > PY_SSIZE_T_MAX)
        return -1;
    else
        return (Py_ssize_t) x;
}

/* s contains x bytes of a little-endian integer.  Return its value as a
 * C int.  Obscure:  when x is 1 or 2, this is an unsigned little-endian
 * int, but when x is 4 it's a signed one.  This is a historical source
 * of x-platform bugs.
 */
static long
calc_binint(char *bytes, int nbytes)
{
    unsigned char *s = (unsigned char *)bytes;
    Py_ssize_t i;
    long x = 0;

    for (i = 0; i < nbytes; i++) {
        x |= (long)s[i] << (8 * i);
    }

    /* Unlike BININT1 and BININT2, BININT (more accurately BININT4)
     * is signed, so on a box with longs bigger than 4 bytes we need
     * to extend a BININT's sign bit to the full width.
     */
    if (SIZEOF_LONG > 4 && nbytes == 4) {
        x |= -(x & (1L << 31));
    }

    return x;
}

static int
load_binintx(UnpicklerObject *self, char *s, int size)
{
    PyObject *value;
    long x;

    x = calc_binint(s, size);

    if ((value = PyLong_FromLong(x)) == NULL)
        return -1;

    PDATA_PUSH(self->stack, value, -1);
    return 0;
}

static int
load_binint(PickleState *state, UnpicklerObject *self)
{
    char *s;
    if (_Unpickler_Read(self, state, &s, 4) < 0)
        return -1;

    return load_binintx(self, s, 4);
}

static int
load_binint1(PickleState *state, UnpicklerObject *self)
{
    char *s;
    if (_Unpickler_Read(self, state, &s, 1) < 0)
        return -1;

    return load_binintx(self, s, 1);
}

static int
load_binint2(PickleState *state, UnpicklerObject *self)
{
    char *s;
    if (_Unpickler_Read(self, state, &s, 2) < 0)
        return -1;

    return load_binintx(self, s, 2);
}

static int
load_long(PickleState *state, UnpicklerObject *self)
{
    PyObject *value;
    char *s = NULL;
    Py_ssize_t len;

    if ((len = _Unpickler_Readline(state, self, &s)) < 0)
        return -1;
    if (len < 2)
        return bad_readline(state);

    /* s[len-2] will usually be 'L' (and s[len-1] is '\n'); we need to remove
       the 'L' before calling PyLong_FromString.  In order to maintain
       compatibility with Python 3.0.0, we don't actually *require*
       the 'L' to be present. */
    if (s[len-2] == 'L')
        s[len-2] = '\0';
    value = PyLong_FromString(s, NULL, 10);
    if (value == NULL)
        return -1;

    PDATA_PUSH(self->stack, value, -1);
    return 0;
}

/* 'size' bytes contain the # of bytes of little-endian 256's-complement
 * data following.
 */
static int
load_counted_long(PickleState *st, UnpicklerObject *self, int size)
{
    PyObject *value;
    char *nbytes;
    char *pdata;

    assert(size == 1 || size == 4);
    if (_Unpickler_Read(self, st, &nbytes, size) < 0)
        return -1;

    size = calc_binint(nbytes, size);
    if (size < 0) {
        /* Corrupt or hostile pickle -- we never write one like this */
        PyErr_SetString(st->UnpicklingError,
                        "LONG pickle has negative byte count");
        return -1;
    }

    if (size == 0)
        value = PyLong_FromLong(0L);
    else {
        /* Read the raw little-endian bytes and convert. */
        if (_Unpickler_Read(self, st, &pdata, size) < 0)
            return -1;
        value = _PyLong_FromByteArray((unsigned char *)pdata, (size_t)size,
                                      1 /* little endian */ , 1 /* signed */ );
    }
    if (value == NULL)
        return -1;
    PDATA_PUSH(self->stack, value, -1);
    return 0;
}

static int
load_float(PickleState *state, UnpicklerObject *self)
{
    PyObject *value;
    char *endptr, *s;
    Py_ssize_t len;
    double d;

    if ((len = _Unpickler_Readline(state, self, &s)) < 0)
        return -1;
    if (len < 2)
        return bad_readline(state);

    errno = 0;
    d = PyOS_string_to_double(s, &endptr, PyExc_OverflowError);
    if (d == -1.0 && PyErr_Occurred())
        return -1;
    if ((endptr[0] != '\n') && (endptr[0] != '\0')) {
        PyErr_SetString(PyExc_ValueError, "could not convert string to float");
        return -1;
    }
    value = PyFloat_FromDouble(d);
    if (value == NULL)
        return -1;

    PDATA_PUSH(self->stack, value, -1);
    return 0;
}

static int
load_binfloat(PickleState *state, UnpicklerObject *self)
{
    PyObject *value;
    double x;
    char *s;

    if (_Unpickler_Read(self, state, &s, 8) < 0)
        return -1;

    x = PyFloat_Unpack8(s, 0);
    if (x == -1.0 && PyErr_Occurred())
        return -1;

    if ((value = PyFloat_FromDouble(x)) == NULL)
        return -1;

    PDATA_PUSH(self->stack, value, -1);
    return 0;
}

static int
load_string(PickleState *st, UnpicklerObject *self)
{
    PyObject *bytes;
    PyObject *obj;
    Py_ssize_t len;
    char *s, *p;

    if ((len = _Unpickler_Readline(st, self, &s)) < 0)
        return -1;
    /* Strip the newline */
    len--;
    /* Strip outermost quotes */
    if (len >= 2 && s[0] == s[len - 1] && (s[0] == '\'' || s[0] == '"')) {
        p = s + 1;
        len -= 2;
    }
    else {
        PyErr_SetString(st->UnpicklingError,
                        "the STRING opcode argument must be quoted");
        return -1;
    }
    assert(len >= 0);

    /* Use the PyBytes API to decode the string, since that is what is used
       to encode, and then coerce the result to Unicode. */
    bytes = PyBytes_DecodeEscape(p, len, NULL, 0, NULL);
    if (bytes == NULL)
        return -1;

    /* Leave the Python 2.x strings as bytes if the *encoding* given to the
       Unpickler was 'bytes'. Otherwise, convert them to unicode. */
    if (strcmp(self->encoding, "bytes") == 0) {
        obj = bytes;
    }
    else {
        obj = PyUnicode_FromEncodedObject(bytes, self->encoding, self->errors);
        Py_DECREF(bytes);
        if (obj == NULL) {
            return -1;
        }
    }

    PDATA_PUSH(self->stack, obj, -1);
    return 0;
}

static int
load_counted_binstring(PickleState *st, UnpicklerObject *self, int nbytes)
{
    PyObject *obj;
    long size;
    char *s;

    if (_Unpickler_Read(self, st, &s, nbytes) < 0)
        return -1;

    size = calc_binint(s, nbytes);
    if (size < 0) {
        PyErr_SetString(st->UnpicklingError,
                     "BINSTRING pickle has negative byte count");
        return -1;
    }

    if (_Unpickler_Read(self, st, &s, size) < 0)
        return -1;

    /* Convert Python 2.x strings to bytes if the *encoding* given to the
       Unpickler was 'bytes'. Otherwise, convert them to unicode. */
    if (strcmp(self->encoding, "bytes") == 0) {
        obj = PyBytes_FromStringAndSize(s, size);
    }
    else {
        obj = PyUnicode_Decode(s, size, self->encoding, self->errors);
    }
    if (obj == NULL) {
        return -1;
    }

    PDATA_PUSH(self->stack, obj, -1);
    return 0;
}

static int
load_counted_binbytes(PickleState *state, UnpicklerObject *self, int nbytes)
{
    PyObject *bytes;
    Py_ssize_t size;
    char *s;

    if (_Unpickler_Read(self, state, &s, nbytes) < 0)
        return -1;

    size = calc_binsize(s, nbytes);
    if (size < 0) {
        PyErr_Format(PyExc_OverflowError,
                     "BINBYTES exceeds system's maximum size of %zd bytes",
                     PY_SSIZE_T_MAX);
        return -1;
    }

    bytes = PyBytes_FromStringAndSize(NULL, size);
    if (bytes == NULL)
        return -1;
    if (_Unpickler_ReadInto(state, self, PyBytes_AS_STRING(bytes), size) < 0) {
        Py_DECREF(bytes);
        return -1;
    }

    PDATA_PUSH(self->stack, bytes, -1);
    return 0;
}

static int
load_counted_bytearray(PickleState *state, UnpicklerObject *self)
{
    PyObject *bytearray;
    Py_ssize_t size;
    char *s;

    if (_Unpickler_Read(self, state, &s, 8) < 0) {
        return -1;
    }

    size = calc_binsize(s, 8);
    if (size < 0) {
        PyErr_Format(PyExc_OverflowError,
                     "BYTEARRAY8 exceeds system's maximum size of %zd bytes",
                     PY_SSIZE_T_MAX);
        return -1;
    }

    bytearray = PyByteArray_FromStringAndSize(NULL, size);
    if (bytearray == NULL) {
        return -1;
    }
    char *str = PyByteArray_AS_STRING(bytearray);
    if (_Unpickler_ReadInto(state, self, str, size) < 0) {
        Py_DECREF(bytearray);
        return -1;
    }

    PDATA_PUSH(self->stack, bytearray, -1);
    return 0;
}

static int
load_next_buffer(PickleState *st, UnpicklerObject *self)
{
    if (self->buffers == NULL) {
        PyErr_SetString(st->UnpicklingError,
                        "pickle stream refers to out-of-band data "
                        "but no *buffers* argument was given");
        return -1;
    }
    PyObject *buf = PyIter_Next(self->buffers);
    if (buf == NULL) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(st->UnpicklingError,
                            "not enough out-of-band buffers");
        }
        return -1;
    }

    PDATA_PUSH(self->stack, buf, -1);
    return 0;
}

static int
load_readonly_buffer(PickleState *state, UnpicklerObject *self)
{
    Py_ssize_t len = Py_SIZE(self->stack);
    if (len <= self->stack->fence) {
        return Pdata_stack_underflow(state, self->stack);
    }

    PyObject *obj = self->stack->data[len - 1];
    PyObject *view = PyMemoryView_FromObject(obj);
    if (view == NULL) {
        return -1;
    }
    if (!PyMemoryView_GET_BUFFER(view)->readonly) {
        /* Original object is writable */
        PyMemoryView_GET_BUFFER(view)->readonly = 1;
        self->stack->data[len - 1] = view;
        Py_DECREF(obj);
    }
    else {
        /* Original object is read-only, no need to replace it */
        Py_DECREF(view);
    }
    return 0;
}

static int
load_unicode(PickleState *state, UnpicklerObject *self)
{
    PyObject *str;
    Py_ssize_t len;
    char *s = NULL;

    if ((len = _Unpickler_Readline(state, self, &s)) < 0)
        return -1;
    if (len < 1)
        return bad_readline(state);

    str = PyUnicode_DecodeRawUnicodeEscape(s, len - 1, NULL);
    if (str == NULL)
        return -1;

    PDATA_PUSH(self->stack, str, -1);
    return 0;
}

static int
load_counted_binunicode(PickleState *state, UnpicklerObject *self, int nbytes)
{
    PyObject *str;
    Py_ssize_t size;
    char *s;

    if (_Unpickler_Read(self, state, &s, nbytes) < 0)
        return -1;

    size = calc_binsize(s, nbytes);
    if (size < 0) {
        PyErr_Format(PyExc_OverflowError,
                     "BINUNICODE exceeds system's maximum size of %zd bytes",
                     PY_SSIZE_T_MAX);
        return -1;
    }

    if (_Unpickler_Read(self, state, &s, size) < 0)
        return -1;

    str = PyUnicode_DecodeUTF8(s, size, "surrogatepass");
    if (str == NULL)
        return -1;

    PDATA_PUSH(self->stack, str, -1);
    return 0;
}

static int
load_counted_tuple(PickleState *state, UnpicklerObject *self, Py_ssize_t len)
{
    PyObject *tuple;

    if (Py_SIZE(self->stack) < len)
        return Pdata_stack_underflow(state, self->stack);

    tuple = Pdata_poptuple(state, self->stack, Py_SIZE(self->stack) - len);
    if (tuple == NULL)
        return -1;
    PDATA_PUSH(self->stack, tuple, -1);
    return 0;
}

static int
load_tuple(PickleState *state, UnpicklerObject *self)
{
    Py_ssize_t i;

    if ((i = marker(state, self)) < 0)
        return -1;

    return load_counted_tuple(state, self, Py_SIZE(self->stack) - i);
}

static int
load_empty_list(PickleState *state, UnpicklerObject *self)
{
    PyObject *list;

    if ((list = PyList_New(0)) == NULL)
        return -1;
    PDATA_PUSH(self->stack, list, -1);
    return 0;
}

static int
load_empty_dict(PickleState *state, UnpicklerObject *self)
{
    PyObject *dict;

    if ((dict = PyDict_New()) == NULL)
        return -1;
    PDATA_PUSH(self->stack, dict, -1);
    return 0;
}

static int
load_empty_set(PickleState *state, UnpicklerObject *self)
{
    PyObject *set;

    if ((set = PySet_New(NULL)) == NULL)
        return -1;
    PDATA_PUSH(self->stack, set, -1);
    return 0;
}

static int
load_list(PickleState *state, UnpicklerObject *self)
{
    PyObject *list;
    Py_ssize_t i;

    if ((i = marker(state, self)) < 0)
        return -1;

    list = Pdata_poplist(self->stack, i);
    if (list == NULL)
        return -1;
    PDATA_PUSH(self->stack, list, -1);
    return 0;
}

static int
load_dict(PickleState *st, UnpicklerObject *self)
{
    PyObject *dict, *key, *value;
    Py_ssize_t i, j, k;

    if ((i = marker(st, self)) < 0)
        return -1;
    j = Py_SIZE(self->stack);

    if ((dict = PyDict_New()) == NULL)
        return -1;

    if ((j - i) % 2 != 0) {
        PyErr_SetString(st->UnpicklingError, "odd number of items for DICT");
        Py_DECREF(dict);
        return -1;
    }

    for (k = i + 1; k < j; k += 2) {
        key = self->stack->data[k - 1];
        value = self->stack->data[k];
        if (PyDict_SetItem(dict, key, value) < 0) {
            Py_DECREF(dict);
            return -1;
        }
    }
    Pdata_clear(self->stack, i);
    PDATA_PUSH(self->stack, dict, -1);
    return 0;
}

static int
load_frozenset(PickleState *state, UnpicklerObject *self)
{
    PyObject *items;
    PyObject *frozenset;
    Py_ssize_t i;

    if ((i = marker(state, self)) < 0)
        return -1;

    items = Pdata_poptuple(state, self->stack, i);
    if (items == NULL)
        return -1;

    frozenset = PyFrozenSet_New(items);
    Py_DECREF(items);
    if (frozenset == NULL)
        return -1;

    PDATA_PUSH(self->stack, frozenset, -1);
    return 0;
}

static PyObject *
instantiate(PyObject *cls, PyObject *args)
{
    /* Caller must assure args are a tuple.  Normally, args come from
       Pdata_poptuple which packs objects from the top of the stack
       into a newly created tuple. */
    assert(PyTuple_Check(args));
    if (!PyTuple_GET_SIZE(args) && PyType_Check(cls)) {
        int rc = PyObject_HasAttrWithError(cls, &_Py_ID(__getinitargs__));
        if (rc < 0) {
            return NULL;
        }
        if (!rc) {
            return PyObject_CallMethodOneArg(cls, &_Py_ID(__new__), cls);
        }
    }
    return PyObject_CallObject(cls, args);
}

static int
load_obj(PickleState *state, UnpicklerObject *self)
{
    PyObject *cls, *args, *obj = NULL;
    Py_ssize_t i;

    if ((i = marker(state, self)) < 0)
        return -1;

    if (Py_SIZE(self->stack) - i < 1)
        return Pdata_stack_underflow(state, self->stack);

    args = Pdata_poptuple(state, self->stack, i + 1);
    if (args == NULL)
        return -1;

    PDATA_POP(state, self->stack, cls);
    if (cls) {
        obj = instantiate(cls, args);
        Py_DECREF(cls);
    }
    Py_DECREF(args);
    if (obj == NULL)
        return -1;

    PDATA_PUSH(self->stack, obj, -1);
    return 0;
}

static int
load_inst(PickleState *state, UnpicklerObject *self)
{
    PyObject *cls = NULL;
    PyObject *args = NULL;
    PyObject *obj = NULL;
    PyObject *module_name;
    PyObject *class_name;
    Py_ssize_t len;
    Py_ssize_t i;
    char *s;

    if ((i = marker(state, self)) < 0)
        return -1;
    if ((len = _Unpickler_Readline(state, self, &s)) < 0)
        return -1;
    if (len < 2)
        return bad_readline(state);

    /* Here it is safe to use PyUnicode_DecodeASCII(), even though non-ASCII
       identifiers are permitted in Python 3.0, since the INST opcode is only
       supported by older protocols on Python 2.x. */
    module_name = PyUnicode_DecodeASCII(s, len - 1, "strict");
    if (module_name == NULL)
        return -1;

    if ((len = _Unpickler_Readline(state, self, &s)) >= 0) {
        if (len < 2) {
            Py_DECREF(module_name);
            return bad_readline(state);
        }
        class_name = PyUnicode_DecodeASCII(s, len - 1, "strict");
        if (class_name != NULL) {
            cls = find_class(self, module_name, class_name);
            Py_DECREF(class_name);
        }
    }
    Py_DECREF(module_name);

    if (cls == NULL)
        return -1;

    if ((args = Pdata_poptuple(state, self->stack, i)) != NULL) {
        obj = instantiate(cls, args);
        Py_DECREF(args);
    }
    Py_DECREF(cls);

    if (obj == NULL)
        return -1;

    PDATA_PUSH(self->stack, obj, -1);
    return 0;
}

static void
newobj_unpickling_error(PickleState *st, const char *msg, int use_kwargs,
                        PyObject *arg)
{
    PyErr_Format(st->UnpicklingError, msg,
                 use_kwargs ? "NEWOBJ_EX" : "NEWOBJ",
                 Py_TYPE(arg)->tp_name);
}

static int
load_newobj(PickleState *state, UnpicklerObject *self, int use_kwargs)
{
    PyObject *cls, *args, *kwargs = NULL;
    PyObject *obj;

    /* Stack is ... cls args [kwargs], and we want to call
     * cls.__new__(cls, *args, **kwargs).
     */
    if (use_kwargs) {
        PDATA_POP(state, self->stack, kwargs);
        if (kwargs == NULL) {
            return -1;
        }
    }
    PDATA_POP(state, self->stack, args);
    if (args == NULL) {
        Py_XDECREF(kwargs);
        return -1;
    }
    PDATA_POP(state, self->stack, cls);
    if (cls == NULL) {
        Py_XDECREF(kwargs);
        Py_DECREF(args);
        return -1;
    }

    if (!PyType_Check(cls)) {
        newobj_unpickling_error(state,
                                "%s class argument must be a type, not %.200s",
                                use_kwargs, cls);
        goto error;
    }
    if (((PyTypeObject *)cls)->tp_new == NULL) {
        newobj_unpickling_error(state,
                                "%s class argument '%.200s' doesn't have __new__",
                                use_kwargs, cls);
        goto error;
    }
    if (!PyTuple_Check(args)) {
        newobj_unpickling_error(state,
                                "%s args argument must be a tuple, not %.200s",
                                use_kwargs, args);
        goto error;
    }
    if (use_kwargs && !PyDict_Check(kwargs)) {
        newobj_unpickling_error(state,
                                "%s kwargs argument must be a dict, not %.200s",
                                use_kwargs, kwargs);
        goto error;
    }

    obj = ((PyTypeObject *)cls)->tp_new((PyTypeObject *)cls, args, kwargs);
    if (obj == NULL) {
        goto error;
    }
    Py_XDECREF(kwargs);
    Py_DECREF(args);
    Py_DECREF(cls);
    PDATA_PUSH(self->stack, obj, -1);
    return 0;

error:
    Py_XDECREF(kwargs);
    Py_DECREF(args);
    Py_DECREF(cls);
    return -1;
}

static int
load_global(PickleState *state, UnpicklerObject *self)
{
    PyObject *global = NULL;
    PyObject *module_name;
    PyObject *global_name;
    Py_ssize_t len;
    char *s;

    if ((len = _Unpickler_Readline(state, self, &s)) < 0)
        return -1;
    if (len < 2)
        return bad_readline(state);
    module_name = PyUnicode_DecodeUTF8(s, len - 1, "strict");
    if (!module_name)
        return -1;

    if ((len = _Unpickler_Readline(state, self, &s)) >= 0) {
        if (len < 2) {
            Py_DECREF(module_name);
            return bad_readline(state);
        }
        global_name = PyUnicode_DecodeUTF8(s, len - 1, "strict");
        if (global_name) {
            global = find_class(self, module_name, global_name);
            Py_DECREF(global_name);
        }
    }
    Py_DECREF(module_name);

    if (global == NULL)
        return -1;
    PDATA_PUSH(self->stack, global, -1);
    return 0;
}

static int
load_stack_global(PickleState *st, UnpicklerObject *self)
{
    PyObject *global;
    PyObject *module_name;
    PyObject *global_name;

    PDATA_POP(st, self->stack, global_name);
    if (global_name == NULL) {
        return -1;
    }
    PDATA_POP(st, self->stack, module_name);
    if (module_name == NULL) {
        Py_DECREF(global_name);
        return -1;
    }
    if (!PyUnicode_CheckExact(module_name) ||
        !PyUnicode_CheckExact(global_name))
    {
        PyErr_SetString(st->UnpicklingError, "STACK_GLOBAL requires str");
        Py_DECREF(global_name);
        Py_DECREF(module_name);
        return -1;
    }
    global = find_class(self, module_name, global_name);
    Py_DECREF(global_name);
    Py_DECREF(module_name);
    if (global == NULL)
        return -1;
    PDATA_PUSH(self->stack, global, -1);
    return 0;
}

static int
load_persid(PickleState *st, UnpicklerObject *self)
{
    PyObject *pid, *obj;
    Py_ssize_t len;
    char *s;

    if ((len = _Unpickler_Readline(st, self, &s)) < 0)
        return -1;
    if (len < 1)
        return bad_readline(st);

    pid = PyUnicode_DecodeASCII(s, len - 1, "strict");
    if (pid == NULL) {
        if (PyErr_ExceptionMatches(PyExc_UnicodeDecodeError)) {
            PyErr_SetString(st->UnpicklingError,
                            "persistent IDs in protocol 0 must be "
                            "ASCII strings");
        }
        return -1;
    }

    obj = PyObject_CallOneArg(self->persistent_load, pid);
    Py_DECREF(pid);
    if (obj == NULL)
        return -1;

    PDATA_PUSH(self->stack, obj, -1);
    return 0;
}

static int
load_binpersid(PickleState *st, UnpicklerObject *self)
{
    PyObject *pid, *obj;

    PDATA_POP(st, self->stack, pid);
    if (pid == NULL)
        return -1;

    obj = PyObject_CallOneArg(self->persistent_load, pid);
    Py_DECREF(pid);
    if (obj == NULL)
        return -1;

    PDATA_PUSH(self->stack, obj, -1);
    return 0;
}

static int
load_pop(PickleState *state, UnpicklerObject *self)
{
    Py_ssize_t len = Py_SIZE(self->stack);

    /* Note that we split the (pickle.py) stack into two stacks,
     * an object stack and a mark stack. We have to be clever and
     * pop the right one. We do this by looking at the top of the
     * mark stack first, and only signalling a stack underflow if
     * the object stack is empty and the mark stack doesn't match
     * our expectations.
     */
    if (self->num_marks > 0 && self->marks[self->num_marks - 1] == len) {
        self->num_marks--;
        self->stack->mark_set = self->num_marks != 0;
        self->stack->fence = self->num_marks ?
                self->marks[self->num_marks - 1] : 0;
    } else if (len <= self->stack->fence)
        return Pdata_stack_underflow(state, self->stack);
    else {
        len--;
        Py_DECREF(self->stack->data[len]);
        Py_SET_SIZE(self->stack, len);
    }
    return 0;
}

static int
load_pop_mark(PickleState *state, UnpicklerObject *self)
{
    Py_ssize_t i;
    if ((i = marker(state, self)) < 0)
        return -1;

    Pdata_clear(self->stack, i);

    return 0;
}

static int
load_dup(PickleState *state, UnpicklerObject *self)
{
    PyObject *last;
    Py_ssize_t len = Py_SIZE(self->stack);

    if (len <= self->stack->fence)
        return Pdata_stack_underflow(state, self->stack);
    last = self->stack->data[len - 1];
    PDATA_APPEND(self->stack, last, -1);
    return 0;
}

static int
load_get(PickleState *st, UnpicklerObject *self)
{
    PyObject *key, *value;
    Py_ssize_t idx;
    Py_ssize_t len;
    char *s;

    if ((len = _Unpickler_Readline(st, self, &s)) < 0)
        return -1;
    if (len < 2)
        return bad_readline(st);

    key = PyLong_FromString(s, NULL, 10);
    if (key == NULL)
        return -1;
    idx = PyLong_AsSsize_t(key);
    if (idx == -1 && PyErr_Occurred()) {
        Py_DECREF(key);
        return -1;
    }

    value = _Unpickler_MemoGet(self, idx);
    if (value == NULL) {
        if (!PyErr_Occurred()) {
           PyErr_Format(st->UnpicklingError, "Memo value not found at index %ld", idx);
        }
        Py_DECREF(key);
        return -1;
    }
    Py_DECREF(key);

    PDATA_APPEND(self->stack, value, -1);
    return 0;
}

static int
load_binget(PickleState *st, UnpicklerObject *self)
{
    PyObject *value;
    Py_ssize_t idx;
    char *s;

    if (_Unpickler_Read(self, st, &s, 1) < 0)
        return -1;

    idx = Py_CHARMASK(s[0]);

    value = _Unpickler_MemoGet(self, idx);
    if (value == NULL) {
        PyObject *key = PyLong_FromSsize_t(idx);
        if (key != NULL) {
            PyErr_Format(st->UnpicklingError, "Memo value not found at index %ld", idx);
            Py_DECREF(key);
        }
        return -1;
    }

    PDATA_APPEND(self->stack, value, -1);
    return 0;
}

static int
load_long_binget(PickleState *st, UnpicklerObject *self)
{
    PyObject *value;
    Py_ssize_t idx;
    char *s;

    if (_Unpickler_Read(self, st, &s, 4) < 0)
        return -1;

    idx = calc_binsize(s, 4);

    value = _Unpickler_MemoGet(self, idx);
    if (value == NULL) {
        PyObject *key = PyLong_FromSsize_t(idx);
        if (key != NULL) {
            PyErr_Format(st->UnpicklingError, "Memo value not found at index %ld", idx);
            Py_DECREF(key);
        }
        return -1;
    }

    PDATA_APPEND(self->stack, value, -1);
    return 0;
}

/* Push an object from the extension registry (EXT[124]).  nbytes is
 * the number of bytes following the opcode, holding the index (code) value.
 */
static int
load_extension(PickleState *st, UnpicklerObject *self, int nbytes)
{
    char *codebytes;            /* the nbytes bytes after the opcode */
    long code;                  /* calc_binint returns long */
    PyObject *py_code;          /* code as a Python int */
    PyObject *obj;              /* the object to push */
    PyObject *pair;             /* (module_name, class_name) */
    PyObject *module_name, *class_name;

    assert(nbytes == 1 || nbytes == 2 || nbytes == 4);
    if (_Unpickler_Read(self, st, &codebytes, nbytes) < 0)
        return -1;
    code = calc_binint(codebytes, nbytes);
    if (code <= 0) {            /* note that 0 is forbidden */
        /* Corrupt or hostile pickle. */
        PyErr_SetString(st->UnpicklingError, "EXT specifies code <= 0");
        return -1;
    }

    /* Look for the code in the cache. */
    py_code = PyLong_FromLong(code);
    if (py_code == NULL)
        return -1;
    obj = PyDict_GetItemWithError(st->extension_cache, py_code);
    if (obj != NULL) {
        /* Bingo. */
        Py_DECREF(py_code);
        PDATA_APPEND(self->stack, obj, -1);
        return 0;
    }
    if (PyErr_Occurred()) {
        Py_DECREF(py_code);
        return -1;
    }

    /* Look up the (module_name, class_name) pair. */
    pair = PyDict_GetItemWithError(st->inverted_registry, py_code);
    if (pair == NULL) {
        Py_DECREF(py_code);
        if (!PyErr_Occurred()) {
            PyErr_Format(PyExc_ValueError, "unregistered extension "
                         "code %ld", code);
        }
        return -1;
    }
    /* Since the extension registry is manipulable via Python code,
     * confirm that pair is really a 2-tuple of strings.
     */
    if (!PyTuple_Check(pair) || PyTuple_Size(pair) != 2) {
        goto error;
    }

    module_name = PyTuple_GET_ITEM(pair, 0);
    if (!PyUnicode_Check(module_name)) {
        goto error;
    }

    class_name = PyTuple_GET_ITEM(pair, 1);
    if (!PyUnicode_Check(class_name)) {
        goto error;
    }

    /* Load the object. */
    obj = find_class(self, module_name, class_name);
    if (obj == NULL) {
        Py_DECREF(py_code);
        return -1;
    }
    /* Cache code -> obj. */
    code = PyDict_SetItem(st->extension_cache, py_code, obj);
    Py_DECREF(py_code);
    if (code < 0) {
        Py_DECREF(obj);
        return -1;
    }
    PDATA_PUSH(self->stack, obj, -1);
    return 0;

error:
    Py_DECREF(py_code);
    PyErr_Format(PyExc_ValueError, "_inverted_registry[%ld] "
                 "isn't a 2-tuple of strings", code);
    return -1;
}

static int
load_put(PickleState *state, UnpicklerObject *self)
{
    PyObject *key, *value;
    Py_ssize_t idx;
    Py_ssize_t len;
    char *s = NULL;

    if ((len = _Unpickler_Readline(state, self, &s)) < 0)
        return -1;
    if (len < 2)
        return bad_readline(state);
    if (Py_SIZE(self->stack) <= self->stack->fence)
        return Pdata_stack_underflow(state, self->stack);
    value = self->stack->data[Py_SIZE(self->stack) - 1];

    key = PyLong_FromString(s, NULL, 10);
    if (key == NULL)
        return -1;
    idx = PyLong_AsSsize_t(key);
    Py_DECREF(key);
    if (idx < 0) {
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_ValueError,
                            "negative PUT argument");
        return -1;
    }

    return _Unpickler_MemoPut(self, idx, value);
}

static int
load_binput(PickleState *state, UnpicklerObject *self)
{
    PyObject *value;
    Py_ssize_t idx;
    char *s;

    if (_Unpickler_Read(self, state, &s, 1) < 0)
        return -1;

    if (Py_SIZE(self->stack) <= self->stack->fence)
        return Pdata_stack_underflow(state, self->stack);
    value = self->stack->data[Py_SIZE(self->stack) - 1];

    idx = Py_CHARMASK(s[0]);

    return _Unpickler_MemoPut(self, idx, value);
}

static int
load_long_binput(PickleState *state, UnpicklerObject *self)
{
    PyObject *value;
    Py_ssize_t idx;
    char *s;

    if (_Unpickler_Read(self, state, &s, 4) < 0)
        return -1;

    if (Py_SIZE(self->stack) <= self->stack->fence)
        return Pdata_stack_underflow(state, self->stack);
    value = self->stack->data[Py_SIZE(self->stack) - 1];

    idx = calc_binsize(s, 4);
    if (idx < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "negative LONG_BINPUT argument");
        return -1;
    }

    return _Unpickler_MemoPut(self, idx, value);
}

static int
load_memoize(PickleState *state, UnpicklerObject *self)
{
    PyObject *value;

    if (Py_SIZE(self->stack) <= self->stack->fence)
        return Pdata_stack_underflow(state, self->stack);
    value = self->stack->data[Py_SIZE(self->stack) - 1];

    return _Unpickler_MemoPut(self, self->memo_len, value);
}

static int
do_append(PickleState *state, UnpicklerObject *self, Py_ssize_t x)
{
    PyObject *value;
    PyObject *slice;
    PyObject *list;
    PyObject *result;
    Py_ssize_t len, i;

    len = Py_SIZE(self->stack);
    if (x > len || x <= self->stack->fence)
        return Pdata_stack_underflow(state, self->stack);
    if (len == x)  /* nothing to do */
        return 0;

    list = self->stack->data[x - 1];

    if (PyList_CheckExact(list)) {
        Py_ssize_t list_len;
        int ret;

        slice = Pdata_poplist(self->stack, x);
        if (!slice)
            return -1;
        list_len = PyList_GET_SIZE(list);
        ret = PyList_SetSlice(list, list_len, list_len, slice);
        Py_DECREF(slice);
        return ret;
    }
    else {
        PyObject *extend_func;

        if (PyObject_GetOptionalAttr(list, &_Py_ID(extend), &extend_func) < 0) {
            return -1;
        }
        if (extend_func != NULL) {
            slice = Pdata_poplist(self->stack, x);
            if (!slice) {
                Py_DECREF(extend_func);
                return -1;
            }
            result = _Pickle_FastCall(extend_func, slice);
            Py_DECREF(extend_func);
            if (result == NULL)
                return -1;
            Py_DECREF(result);
        }
        else {
            PyObject *append_func;

            /* Even if the PEP 307 requires extend() and append() methods,
               fall back on append() if the object has no extend() method
               for backward compatibility. */
            append_func = PyObject_GetAttr(list, &_Py_ID(append));
            if (append_func == NULL)
                return -1;
            for (i = x; i < len; i++) {
                value = self->stack->data[i];
                result = _Pickle_FastCall(append_func, value);
                if (result == NULL) {
                    Pdata_clear(self->stack, i + 1);
                    Py_SET_SIZE(self->stack, x);
                    Py_DECREF(append_func);
                    return -1;
                }
                Py_DECREF(result);
            }
            Py_SET_SIZE(self->stack, x);
            Py_DECREF(append_func);
        }
    }

    return 0;
}

static int
load_append(PickleState *state, UnpicklerObject *self)
{
    if (Py_SIZE(self->stack) - 1 <= self->stack->fence)
        return Pdata_stack_underflow(state, self->stack);
    return do_append(state, self, Py_SIZE(self->stack) - 1);
}

static int
load_appends(PickleState *state, UnpicklerObject *self)
{
    Py_ssize_t i = marker(state, self);
    if (i < 0)
        return -1;
    return do_append(state, self, i);
}

static int
do_setitems(PickleState *st, UnpicklerObject *self, Py_ssize_t x)
{
    PyObject *value, *key;
    PyObject *dict;
    Py_ssize_t len, i;
    int status = 0;

    len = Py_SIZE(self->stack);
    if (x > len || x <= self->stack->fence)
        return Pdata_stack_underflow(st, self->stack);
    if (len == x)  /* nothing to do */
        return 0;
    if ((len - x) % 2 != 0) {
        /* Corrupt or hostile pickle -- we never write one like this. */
        PyErr_SetString(st->UnpicklingError,
                        "odd number of items for SETITEMS");
        return -1;
    }

    /* Here, dict does not actually need to be a PyDict; it could be anything
       that supports the __setitem__ attribute. */
    dict = self->stack->data[x - 1];

    for (i = x + 1; i < len; i += 2) {
        key = self->stack->data[i - 1];
        value = self->stack->data[i];
        if (PyObject_SetItem(dict, key, value) < 0) {
            status = -1;
            break;
        }
    }

    Pdata_clear(self->stack, x);
    return status;
}

static int
load_setitem(PickleState *state, UnpicklerObject *self)
{
    return do_setitems(state, self, Py_SIZE(self->stack) - 2);
}

static int
load_setitems(PickleState *state, UnpicklerObject *self)
{
    Py_ssize_t i = marker(state, self);
    if (i < 0)
        return -1;
    return do_setitems(state, self, i);
}

static int
load_additems(PickleState *state, UnpicklerObject *self)
{
    PyObject *set;
    Py_ssize_t mark, len, i;

    mark =  marker(state, self);
    if (mark < 0)
        return -1;
    len = Py_SIZE(self->stack);
    if (mark > len || mark <= self->stack->fence)
        return Pdata_stack_underflow(state, self->stack);
    if (len == mark)  /* nothing to do */
        return 0;

    set = self->stack->data[mark - 1];

    if (PySet_Check(set)) {
        PyObject *items;
        int status;

        items = Pdata_poptuple(state, self->stack, mark);
        if (items == NULL)
            return -1;

        status = _PySet_Update(set, items);
        Py_DECREF(items);
        return status;
    }
    else {
        PyObject *add_func;

        add_func = PyObject_GetAttr(set, &_Py_ID(add));
        if (add_func == NULL)
            return -1;
        for (i = mark; i < len; i++) {
            PyObject *result;
            PyObject *item;

            item = self->stack->data[i];
            result = _Pickle_FastCall(add_func, item);
            if (result == NULL) {
                Pdata_clear(self->stack, i + 1);
                Py_SET_SIZE(self->stack, mark);
                Py_DECREF(add_func);
                return -1;
            }
            Py_DECREF(result);
        }
        Py_SET_SIZE(self->stack, mark);
        Py_DECREF(add_func);
    }

    return 0;
}

static int
load_build(PickleState *st, UnpicklerObject *self)
{
    PyObject *inst, *slotstate;
    PyObject *setstate;
    int status = 0;

    /* Stack is ... instance, state.  We want to leave instance at
     * the stack top, possibly mutated via instance.__setstate__(state).
     */
    if (Py_SIZE(self->stack) - 2 < self->stack->fence)
        return Pdata_stack_underflow(st, self->stack);

    PyObject *state;
    PDATA_POP(st, self->stack, state);
    if (state == NULL)
        return -1;

    inst = self->stack->data[Py_SIZE(self->stack) - 1];

    if (PyObject_GetOptionalAttr(inst, &_Py_ID(__setstate__), &setstate) < 0) {
        Py_DECREF(state);
        return -1;
    }
    if (setstate != NULL) {
        PyObject *result;

        /* The explicit __setstate__ is responsible for everything. */
        result = _Pickle_FastCall(setstate, state);
        Py_DECREF(setstate);
        if (result == NULL)
            return -1;
        Py_DECREF(result);
        return 0;
    }

    /* A default __setstate__.  First see whether state embeds a
     * slot state dict too (a proto 2 addition).
     */
    if (PyTuple_Check(state) && PyTuple_GET_SIZE(state) == 2) {
        PyObject *tmp = state;

        state = PyTuple_GET_ITEM(tmp, 0);
        slotstate = PyTuple_GET_ITEM(tmp, 1);
        Py_INCREF(state);
        Py_INCREF(slotstate);
        Py_DECREF(tmp);
    }
    else
        slotstate = NULL;

    /* Set inst.__dict__ from the state dict (if any). */
    if (state != Py_None) {
        PyObject *dict;
        PyObject *d_key, *d_value;
        Py_ssize_t i;

        if (!PyDict_Check(state)) {
            PyErr_SetString(st->UnpicklingError, "state is not a dictionary");
            goto error;
        }
        dict = PyObject_GetAttr(inst, &_Py_ID(__dict__));
        if (dict == NULL)
            goto error;

        i = 0;
        while (PyDict_Next(state, &i, &d_key, &d_value)) {
            /* normally the keys for instance attributes are
               interned.  we should try to do that here. */
            Py_INCREF(d_key);
            if (PyUnicode_CheckExact(d_key)) {
                PyInterpreterState *interp = _PyInterpreterState_GET();
                _PyUnicode_InternMortal(interp, &d_key);
            }
            if (PyObject_SetItem(dict, d_key, d_value) < 0) {
                Py_DECREF(d_key);
                Py_DECREF(dict);
                goto error;
            }
            Py_DECREF(d_key);
        }
        Py_DECREF(dict);
    }

    /* Also set instance attributes from the slotstate dict (if any). */
    if (slotstate != NULL) {
        PyObject *d_key, *d_value;
        Py_ssize_t i;

        if (!PyDict_Check(slotstate)) {
            PyErr_SetString(st->UnpicklingError,
                            "slot state is not a dictionary");
            goto error;
        }
        i = 0;
        while (PyDict_Next(slotstate, &i, &d_key, &d_value)) {
            if (PyObject_SetAttr(inst, d_key, d_value) < 0)
                goto error;
        }
    }

    if (0) {
  error:
        status = -1;
    }

    Py_DECREF(state);
    Py_XDECREF(slotstate);
    return status;
}

static int
load_mark(PickleState *state, UnpicklerObject *self)
{

    /* Note that we split the (pickle.py) stack into two stacks, an
     * object stack and a mark stack. Here we push a mark onto the
     * mark stack.
     */

    if (self->num_marks >= self->marks_size) {
        size_t alloc = ((size_t)self->num_marks << 1) + 20;
        Py_ssize_t *marks_new = self->marks;
        PyMem_RESIZE(marks_new, Py_ssize_t, alloc);
        if (marks_new == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        self->marks = marks_new;
        self->marks_size = (Py_ssize_t)alloc;
    }

    self->stack->mark_set = 1;
    self->marks[self->num_marks++] = self->stack->fence = Py_SIZE(self->stack);

    return 0;
}

static int
load_reduce(PickleState *state, UnpicklerObject *self)
{
    PyObject *callable = NULL;
    PyObject *argtup = NULL;
    PyObject *obj = NULL;

    PDATA_POP(state, self->stack, argtup);
    if (argtup == NULL)
        return -1;
    PDATA_POP(state, self->stack, callable);
    if (callable) {
        obj = PyObject_CallObject(callable, argtup);
        Py_DECREF(callable);
    }
    Py_DECREF(argtup);

    if (obj == NULL)
        return -1;

    PDATA_PUSH(self->stack, obj, -1);
    return 0;
}

/* Just raises an error if we don't know the protocol specified.  PROTO
 * is the first opcode for protocols >= 2.
 */
static int
load_proto(PickleState *state, UnpicklerObject *self)
{
    char *s;
    int i;

    if (_Unpickler_Read(self, state, &s, 1) < 0)
        return -1;

    i = (unsigned char)s[0];
    if (i <= HIGHEST_PROTOCOL) {
        self->proto = i;
        return 0;
    }

    PyErr_Format(PyExc_ValueError, "unsupported pickle protocol: %d", i);
    return -1;
}

static int
load_frame(PickleState *state, UnpicklerObject *self)
{
    char *s;
    Py_ssize_t frame_len;

    if (_Unpickler_Read(self, state, &s, 8) < 0)
        return -1;

    frame_len = calc_binsize(s, 8);
    if (frame_len < 0) {
        PyErr_Format(PyExc_OverflowError,
                     "FRAME length exceeds system's maximum of %zd bytes",
                     PY_SSIZE_T_MAX);
        return -1;
    }

    if (_Unpickler_Read(self, state, &s, frame_len) < 0)
        return -1;

    /* Rewind to start of frame */
    self->next_read_idx -= frame_len;
    return 0;
}

static PyObject *
load(PickleState *st, UnpicklerObject *self)
{
    PyObject *value = NULL;
    PyObject *tmp;
    char *s = NULL;

    self->num_marks = 0;
    self->stack->mark_set = 0;
    self->stack->fence = 0;
    self->proto = 0;
    if (Py_SIZE(self->stack))
        Pdata_clear(self->stack, 0);

    /* Cache the persistent_load method. */
    tmp = PyObject_GetAttr((PyObject *)self, &_Py_ID(persistent_load));
    if (tmp == NULL) {
        goto error;
    }
    Py_XSETREF(self->persistent_load, tmp);

    /* Convenient macros for the dispatch while-switch loop just below. */
#define OP(opcode, load_func) \
    case opcode: if (load_func(st, self) < 0) break; continue;

#define OP_ARG(opcode, load_func, arg) \
    case opcode: if (load_func(st, self, (arg)) < 0) break; continue;

    while (1) {
        if (_Unpickler_Read(self, st, &s, 1) < 0) {
            if (PyErr_ExceptionMatches(st->UnpicklingError)) {
                PyErr_Format(PyExc_EOFError, "Ran out of input");
            }
            goto error;
        }

        switch ((enum opcode)s[0]) {
        OP(NONE, load_none)
        OP(BININT, load_binint)
        OP(BININT1, load_binint1)
        OP(BININT2, load_binint2)
        OP(INT, load_int)
        OP(LONG, load_long)
        OP_ARG(LONG1, load_counted_long, 1)
        OP_ARG(LONG4, load_counted_long, 4)
        OP(FLOAT, load_float)
        OP(BINFLOAT, load_binfloat)
        OP_ARG(SHORT_BINBYTES, load_counted_binbytes, 1)
        OP_ARG(BINBYTES, load_counted_binbytes, 4)
        OP_ARG(BINBYTES8, load_counted_binbytes, 8)
        OP(BYTEARRAY8, load_counted_bytearray)
        OP(NEXT_BUFFER, load_next_buffer)
        OP(READONLY_BUFFER, load_readonly_buffer)
        OP_ARG(SHORT_BINSTRING, load_counted_binstring, 1)
        OP_ARG(BINSTRING, load_counted_binstring, 4)
        OP(STRING, load_string)
        OP(UNICODE, load_unicode)
        OP_ARG(SHORT_BINUNICODE, load_counted_binunicode, 1)
        OP_ARG(BINUNICODE, load_counted_binunicode, 4)
        OP_ARG(BINUNICODE8, load_counted_binunicode, 8)
        OP_ARG(EMPTY_TUPLE, load_counted_tuple, 0)
        OP_ARG(TUPLE1, load_counted_tuple, 1)
        OP_ARG(TUPLE2, load_counted_tuple, 2)
        OP_ARG(TUPLE3, load_counted_tuple, 3)
        OP(TUPLE, load_tuple)
        OP(EMPTY_LIST, load_empty_list)
        OP(LIST, load_list)
        OP(EMPTY_DICT, load_empty_dict)
        OP(DICT, load_dict)
        OP(EMPTY_SET, load_empty_set)
        OP(ADDITEMS, load_additems)
        OP(FROZENSET, load_frozenset)
        OP(OBJ, load_obj)
        OP(INST, load_inst)
        OP_ARG(NEWOBJ, load_newobj, 0)
        OP_ARG(NEWOBJ_EX, load_newobj, 1)
        OP(GLOBAL, load_global)
        OP(STACK_GLOBAL, load_stack_global)
        OP(APPEND, load_append)
        OP(APPENDS, load_appends)
        OP(BUILD, load_build)
        OP(DUP, load_dup)
        OP(BINGET, load_binget)
        OP(LONG_BINGET, load_long_binget)
        OP(GET, load_get)
        OP(MARK, load_mark)
        OP(BINPUT, load_binput)
        OP(LONG_BINPUT, load_long_binput)
        OP(PUT, load_put)
        OP(MEMOIZE, load_memoize)
        OP(POP, load_pop)
        OP(POP_MARK, load_pop_mark)
        OP(SETITEM, load_setitem)
        OP(SETITEMS, load_setitems)
        OP(PERSID, load_persid)
        OP(BINPERSID, load_binpersid)
        OP(REDUCE, load_reduce)
        OP(PROTO, load_proto)
        OP(FRAME, load_frame)
        OP_ARG(EXT1, load_extension, 1)
        OP_ARG(EXT2, load_extension, 2)
        OP_ARG(EXT4, load_extension, 4)
        OP_ARG(NEWTRUE, load_bool, Py_True)
        OP_ARG(NEWFALSE, load_bool, Py_False)

        case STOP:
            break;

        default:
            {
                unsigned char c = (unsigned char) *s;
                if (0x20 <= c && c <= 0x7e && c != '\'' && c != '\\') {
                    PyErr_Format(st->UnpicklingError,
                                 "invalid load key, '%c'.", c);
                }
                else {
                    PyErr_Format(st->UnpicklingError,
                                 "invalid load key, '\\x%02x'.", c);
                }
                goto error;
            }
        }

        break;                  /* and we are done! */
    }

    if (PyErr_Occurred()) {
        goto error;
    }

    if (_Unpickler_SkipConsumed(self) < 0)
        goto error;

    Py_CLEAR(self->persistent_load);
    PDATA_POP(st, self->stack, value);
    return value;

error:
    Py_CLEAR(self->persistent_load);
    return NULL;
}

/*[clinic input]

_pickle.Unpickler.persistent_load

    cls: defining_class
    pid: object
    /

[clinic start generated code]*/

static PyObject *
_pickle_Unpickler_persistent_load_impl(UnpicklerObject *self,
                                       PyTypeObject *cls, PyObject *pid)
/*[clinic end generated code: output=9f4706f1330cb14d input=2f9554fae051276e]*/
{
    PickleState *st = _Pickle_GetStateByClass(cls);
    PyErr_SetString(st->UnpicklingError,
                    "A load persistent id instruction was encountered, "
                    "but no persistent_load function was specified.");
    return NULL;
}

/*[clinic input]

_pickle.Unpickler.load

    cls: defining_class

Load a pickle.

Read a pickled object representation from the open file object given
in the constructor, and return the reconstituted object hierarchy
specified therein.
[clinic start generated code]*/

static PyObject *
_pickle_Unpickler_load_impl(UnpicklerObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=cc88168f608e3007 input=f5d2f87e61d5f07f]*/
{
    UnpicklerObject *unpickler = (UnpicklerObject*)self;

    PickleState *st = _Pickle_GetStateByClass(cls);

    /* Check whether the Unpickler was initialized correctly. This prevents
       segfaulting if a subclass overridden __init__ with a function that does
       not call Unpickler.__init__(). Here, we simply ensure that self->read
       is not NULL. */
    if (unpickler->read == NULL) {
        PyErr_Format(st->UnpicklingError,
                     "Unpickler.__init__() was not called by %s.__init__()",
                     Py_TYPE(unpickler)->tp_name);
        return NULL;
    }

    return load(st, unpickler);
}

/* The name of find_class() is misleading. In newer pickle protocols, this
   function is used for loading any global (i.e., functions), not just
   classes. The name is kept only for backward compatibility. */

/*[clinic input]

_pickle.Unpickler.find_class

  cls: defining_class
  module_name: object
  global_name: object
  /

Return an object from a specified module.

If necessary, the module will be imported. Subclasses may override
this method (e.g. to restrict unpickling of arbitrary classes and
functions).

This method is called whenever a class or a function object is
needed.  Both arguments passed are str objects.
[clinic start generated code]*/

static PyObject *
_pickle_Unpickler_find_class_impl(UnpicklerObject *self, PyTypeObject *cls,
                                  PyObject *module_name,
                                  PyObject *global_name)
/*[clinic end generated code: output=99577948abb0be81 input=9577745719219fc7]*/
{
    PyObject *global;
    PyObject *module;

    if (PySys_Audit("pickle.find_class", "OO",
                    module_name, global_name) < 0) {
        return NULL;
    }

    /* Try to map the old names used in Python 2.x to the new ones used in
       Python 3.x.  We do this only with old pickle protocols and when the
       user has not disabled the feature. */
    if (self->proto < 3 && self->fix_imports) {
        PyObject *key;
        PyObject *item;
        PickleState *st = _Pickle_GetStateByClass(cls);

        /* Check if the global (i.e., a function or a class) was renamed
           or moved to another module. */
        key = PyTuple_Pack(2, module_name, global_name);
        if (key == NULL)
            return NULL;
        item = PyDict_GetItemWithError(st->name_mapping_2to3, key);
        Py_DECREF(key);
        if (item) {
            if (!PyTuple_Check(item) || PyTuple_GET_SIZE(item) != 2) {
                PyErr_Format(PyExc_RuntimeError,
                             "_compat_pickle.NAME_MAPPING values should be "
                             "2-tuples, not %.200s", Py_TYPE(item)->tp_name);
                return NULL;
            }
            module_name = PyTuple_GET_ITEM(item, 0);
            global_name = PyTuple_GET_ITEM(item, 1);
            if (!PyUnicode_Check(module_name) ||
                !PyUnicode_Check(global_name)) {
                PyErr_Format(PyExc_RuntimeError,
                             "_compat_pickle.NAME_MAPPING values should be "
                             "pairs of str, not (%.200s, %.200s)",
                             Py_TYPE(module_name)->tp_name,
                             Py_TYPE(global_name)->tp_name);
                return NULL;
            }
        }
        else if (PyErr_Occurred()) {
            return NULL;
        }
        else {
            /* Check if the module was renamed. */
            item = PyDict_GetItemWithError(st->import_mapping_2to3, module_name);
            if (item) {
                if (!PyUnicode_Check(item)) {
                    PyErr_Format(PyExc_RuntimeError,
                                "_compat_pickle.IMPORT_MAPPING values should be "
                                "strings, not %.200s", Py_TYPE(item)->tp_name);
                    return NULL;
                }
                module_name = item;
            }
            else if (PyErr_Occurred()) {
                return NULL;
            }
        }
    }

    /*
     * we don't use PyImport_GetModule here, because it can return partially-
     * initialised modules, which then cause the getattribute to fail.
     */
    module = PyImport_Import(module_name);
    if (module == NULL) {
        return NULL;
    }
    if (self->proto >= 4) {
        PyObject *dotted_path = get_dotted_path(global_name);
        if (dotted_path == NULL) {
            Py_DECREF(module);
            return NULL;
        }
        global = getattribute(module, dotted_path, 1);
        assert(global != NULL || PyErr_Occurred());
        if (global == NULL && PyList_GET_SIZE(dotted_path) > 1) {
            PyObject *exc = PyErr_GetRaisedException();
            PyErr_Format(PyExc_AttributeError,
                         "Can't resolve path %R on module %R",
                         global_name, module_name);
            _PyErr_ChainExceptions1(exc);
        }
        Py_DECREF(dotted_path);
    }
    else {
        global = PyObject_GetAttr(module, global_name);
    }
    Py_DECREF(module);
    return global;
}

/*[clinic input]

_pickle.Unpickler.__sizeof__ -> size_t

Returns size in memory, in bytes.
[clinic start generated code]*/

static size_t
_pickle_Unpickler___sizeof___impl(UnpicklerObject *self)
/*[clinic end generated code: output=4648d84c228196df input=27180b2b6b524012]*/
{
    size_t res = _PyObject_SIZE(Py_TYPE(self));
    if (self->memo != NULL)
        res += self->memo_size * sizeof(PyObject *);
    if (self->marks != NULL)
        res += (size_t)self->marks_size * sizeof(Py_ssize_t);
    if (self->input_line != NULL)
        res += strlen(self->input_line) + 1;
    if (self->encoding != NULL)
        res += strlen(self->encoding) + 1;
    if (self->errors != NULL)
        res += strlen(self->errors) + 1;
    return res;
}

static struct PyMethodDef Unpickler_methods[] = {
    _PICKLE_UNPICKLER_PERSISTENT_LOAD_METHODDEF
    _PICKLE_UNPICKLER_LOAD_METHODDEF
    _PICKLE_UNPICKLER_FIND_CLASS_METHODDEF
    _PICKLE_UNPICKLER___SIZEOF___METHODDEF
    {NULL, NULL}                /* sentinel */
};

static int
Unpickler_clear(PyObject *op)
{
    UnpicklerObject *self = UnpicklerObject_CAST(op);
    Py_CLEAR(self->readline);
    Py_CLEAR(self->readinto);
    Py_CLEAR(self->read);
    Py_CLEAR(self->peek);
    Py_CLEAR(self->stack);
    Py_CLEAR(self->persistent_load);
    Py_CLEAR(self->persistent_load_attr);
    Py_CLEAR(self->buffers);
    if (self->buffer.buf != NULL) {
        PyBuffer_Release(&self->buffer);
        self->buffer.buf = NULL;
    }

    _Unpickler_MemoCleanup(self);
    PyMem_Free(self->marks);
    self->marks = NULL;
    PyMem_Free(self->input_line);
    self->input_line = NULL;
    PyMem_Free(self->encoding);
    self->encoding = NULL;
    PyMem_Free(self->errors);
    self->errors = NULL;

    return 0;
}

static void
Unpickler_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    (void)Unpickler_clear(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static int
Unpickler_traverse(PyObject *op, visitproc visit, void *arg)
{
    UnpicklerObject *self = UnpicklerObject_CAST(op);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->readline);
    Py_VISIT(self->readinto);
    Py_VISIT(self->read);
    Py_VISIT(self->peek);
    Py_VISIT(self->stack);
    Py_VISIT(self->persistent_load);
    Py_VISIT(self->persistent_load_attr);
    Py_VISIT(self->buffers);
    PyObject **memo = self->memo;
    if (memo) {
        Py_ssize_t i = self->memo_size;
        while (--i >= 0) {
            Py_VISIT(memo[i]);
        }
    }
    return 0;
}

/*[clinic input]

_pickle.Unpickler.__init__

  file: object
  *
  fix_imports: bool = True
  encoding: str = 'ASCII'
  errors: str = 'strict'
  buffers: object(c_default="NULL") = ()

This takes a binary file for reading a pickle data stream.

The protocol version of the pickle is detected automatically, so no
protocol argument is needed.  Bytes past the pickled object's
representation are ignored.

The argument *file* must have two methods, a read() method that takes
an integer argument, and a readline() method that requires no
arguments.  Both methods should return bytes.  Thus *file* can be a
binary file object opened for reading, an io.BytesIO object, or any
other custom object that meets this interface.

Optional keyword arguments are *fix_imports*, *encoding* and *errors*,
which are used to control compatibility support for pickle stream
generated by Python 2.  If *fix_imports* is True, pickle will try to
map the old Python 2 names to the new names used in Python 3.  The
*encoding* and *errors* tell pickle how to decode 8-bit string
instances pickled by Python 2; these default to 'ASCII' and 'strict',
respectively.  The *encoding* can be 'bytes' to read these 8-bit
string instances as bytes objects.
[clinic start generated code]*/

static int
_pickle_Unpickler___init___impl(UnpicklerObject *self, PyObject *file,
                                int fix_imports, const char *encoding,
                                const char *errors, PyObject *buffers)
/*[clinic end generated code: output=09f0192649ea3f85 input=ca4c1faea9553121]*/
{
    /* In case of multiple __init__() calls, clear previous content. */
    if (self->read != NULL)
        (void)Unpickler_clear((PyObject *)self);

    if (_Unpickler_SetInputStream(self, file) < 0)
        return -1;

    if (_Unpickler_SetInputEncoding(self, encoding, errors) < 0)
        return -1;

    if (_Unpickler_SetBuffers(self, buffers) < 0)
        return -1;

    self->fix_imports = fix_imports;

    PyTypeObject *tp = Py_TYPE(self);
    PickleState *state = _Pickle_FindStateByType(tp);
    self->stack = (Pdata *)Pdata_New(state);
    if (self->stack == NULL)
        return -1;

    self->memo_size = 32;
    self->memo = _Unpickler_NewMemo(self->memo_size);
    if (self->memo == NULL)
        return -1;

    self->proto = 0;

    return 0;
}


/* Define a proxy object for the Unpickler's internal memo object. This is to
 * avoid breaking code like:
 *  unpickler.memo.clear()
 * and
 *  unpickler.memo = saved_memo
 * Is this a good idea? Not really, but we don't want to break code that uses
 * it. Note that we don't implement the entire mapping API here. This is
 * intentional, as these should be treated as black-box implementation details.
 *
 * We do, however, have to implement pickling/unpickling support because of
 * real-world code like cvs2svn.
 */

/*[clinic input]
_pickle.UnpicklerMemoProxy.clear

Remove all items from memo.
[clinic start generated code]*/

static PyObject *
_pickle_UnpicklerMemoProxy_clear_impl(UnpicklerMemoProxyObject *self)
/*[clinic end generated code: output=d20cd43f4ba1fb1f input=b1df7c52e7afd9bd]*/
{
    _Unpickler_MemoCleanup(self->unpickler);
    self->unpickler->memo = _Unpickler_NewMemo(self->unpickler->memo_size);
    if (self->unpickler->memo == NULL)
        return NULL;
    Py_RETURN_NONE;
}

/*[clinic input]
_pickle.UnpicklerMemoProxy.copy

Copy the memo to a new object.
[clinic start generated code]*/

static PyObject *
_pickle_UnpicklerMemoProxy_copy_impl(UnpicklerMemoProxyObject *self)
/*[clinic end generated code: output=e12af7e9bc1e4c77 input=97769247ce032c1d]*/
{
    size_t i;
    PyObject *new_memo = PyDict_New();
    if (new_memo == NULL)
        return NULL;

    for (i = 0; i < self->unpickler->memo_size; i++) {
        int status;
        PyObject *key, *value;

        value = self->unpickler->memo[i];
        if (value == NULL)
            continue;

        key = PyLong_FromSsize_t(i);
        if (key == NULL)
            goto error;
        status = PyDict_SetItem(new_memo, key, value);
        Py_DECREF(key);
        if (status < 0)
            goto error;
    }
    return new_memo;

error:
    Py_DECREF(new_memo);
    return NULL;
}

/*[clinic input]
_pickle.UnpicklerMemoProxy.__reduce__

Implement pickling support.
[clinic start generated code]*/

static PyObject *
_pickle_UnpicklerMemoProxy___reduce___impl(UnpicklerMemoProxyObject *self)
/*[clinic end generated code: output=6da34ac048d94cca input=6920862413407199]*/
{
    PyObject *reduce_value;
    PyObject *constructor_args;
    PyObject *contents = _pickle_UnpicklerMemoProxy_copy_impl(self);
    if (contents == NULL)
        return NULL;

    reduce_value = PyTuple_New(2);
    if (reduce_value == NULL) {
        Py_DECREF(contents);
        return NULL;
    }
    constructor_args = PyTuple_New(1);
    if (constructor_args == NULL) {
        Py_DECREF(contents);
        Py_DECREF(reduce_value);
        return NULL;
    }
    PyTuple_SET_ITEM(constructor_args, 0, contents);
    PyTuple_SET_ITEM(reduce_value, 0, Py_NewRef(&PyDict_Type));
    PyTuple_SET_ITEM(reduce_value, 1, constructor_args);
    return reduce_value;
}

static PyMethodDef unpicklerproxy_methods[] = {
    _PICKLE_UNPICKLERMEMOPROXY_CLEAR_METHODDEF
    _PICKLE_UNPICKLERMEMOPROXY_COPY_METHODDEF
    _PICKLE_UNPICKLERMEMOPROXY___REDUCE___METHODDEF
    {NULL, NULL}    /* sentinel */
};

static void
UnpicklerMemoProxy_dealloc(PyObject *op)
{
    UnpicklerMemoProxyObject *self = UnpicklerMemoProxyObject_CAST(op);
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    Py_CLEAR(self->unpickler);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static int
UnpicklerMemoProxy_traverse(PyObject *op, visitproc visit, void *arg)
{
    UnpicklerMemoProxyObject *self = UnpicklerMemoProxyObject_CAST(op);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->unpickler);
    return 0;
}

static int
UnpicklerMemoProxy_clear(PyObject *op)
{
    UnpicklerMemoProxyObject *self = UnpicklerMemoProxyObject_CAST(op);
    Py_CLEAR(self->unpickler);
    return 0;
}

static PyType_Slot unpickler_memoproxy_slots[] = {
    {Py_tp_dealloc, UnpicklerMemoProxy_dealloc},
    {Py_tp_traverse, UnpicklerMemoProxy_traverse},
    {Py_tp_clear, UnpicklerMemoProxy_clear},
    {Py_tp_methods, unpicklerproxy_methods},
    {Py_tp_hash, PyObject_HashNotImplemented},
    {0, NULL},
};

static PyType_Spec unpickler_memoproxy_spec = {
    .name = "_pickle.UnpicklerMemoProxy",
    .basicsize = sizeof(UnpicklerMemoProxyObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = unpickler_memoproxy_slots,
};

static PyObject *
UnpicklerMemoProxy_New(UnpicklerObject *unpickler)
{
    PickleState *state = _Pickle_FindStateByType(Py_TYPE(unpickler));
    UnpicklerMemoProxyObject *self;
    self = PyObject_GC_New(UnpicklerMemoProxyObject,
                           state->UnpicklerMemoProxyType);
    if (self == NULL)
        return NULL;
    self->unpickler = (UnpicklerObject*)Py_NewRef(unpickler);
    PyObject_GC_Track(self);
    return (PyObject *)self;
}

/*****************************************************************************/


static PyObject *
Unpickler_get_memo(PyObject *op, void *Py_UNUSED(closure))
{
    UnpicklerObject *self = UnpicklerObject_CAST(op);
    return UnpicklerMemoProxy_New(self);
}

static int
Unpickler_set_memo(PyObject *op, PyObject *obj, void *Py_UNUSED(closure))
{
    PyObject **new_memo;
    UnpicklerObject *self = UnpicklerObject_CAST(op);
    size_t new_memo_size = 0;

    if (obj == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "attribute deletion is not supported");
        return -1;
    }

    PickleState *state = _Pickle_FindStateByType(Py_TYPE(self));
    if (Py_IS_TYPE(obj, state->UnpicklerMemoProxyType)) {
        UnpicklerObject *unpickler = /* safe fast cast for 'obj' */
            ((UnpicklerMemoProxyObject *)obj)->unpickler;

        new_memo_size = unpickler->memo_size;
        new_memo = _Unpickler_NewMemo(new_memo_size);
        if (new_memo == NULL)
            return -1;

        for (size_t i = 0; i < new_memo_size; i++) {
            new_memo[i] = Py_XNewRef(unpickler->memo[i]);
        }
    }
    else if (PyDict_Check(obj)) {
        Py_ssize_t i = 0;
        PyObject *key, *value;

        new_memo_size = PyDict_GET_SIZE(obj);
        new_memo = _Unpickler_NewMemo(new_memo_size);
        if (new_memo == NULL)
            return -1;

        while (PyDict_Next(obj, &i, &key, &value)) {
            Py_ssize_t idx;
            if (!PyLong_Check(key)) {
                PyErr_SetString(PyExc_TypeError,
                                "memo key must be integers");
                goto error;
            }
            idx = PyLong_AsSsize_t(key);
            if (idx == -1 && PyErr_Occurred())
                goto error;
            if (idx < 0) {
                PyErr_SetString(PyExc_ValueError,
                                "memo key must be positive integers.");
                goto error;
            }
            if (_Unpickler_MemoPut(self, idx, value) < 0)
                goto error;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "'memo' attribute must be an UnpicklerMemoProxy object "
                     "or dict, not %.200s", Py_TYPE(obj)->tp_name);
        return -1;
    }

    _Unpickler_MemoCleanup(self);
    self->memo_size = new_memo_size;
    self->memo = new_memo;

    return 0;

  error:
    if (new_memo_size) {
        for (size_t i = new_memo_size - 1; i != SIZE_MAX; i--) {
            Py_XDECREF(new_memo[i]);
        }
        PyMem_Free(new_memo);
    }
    return -1;
}

static PyObject *
Unpickler_getattr(PyObject *self, PyObject *name)
{
    UnpicklerObject *obj = UnpicklerObject_CAST(self);
    if (PyUnicode_Check(name)
        && PyUnicode_EqualToUTF8(name, "persistent_load")
        && obj->persistent_load_attr)
    {
        return Py_NewRef(obj->persistent_load_attr);
    }

    return PyObject_GenericGetAttr(self, name);
}

static int
Unpickler_setattr(PyObject *self, PyObject *name, PyObject *value)
{
    if (PyUnicode_Check(name)
        && PyUnicode_EqualToUTF8(name, "persistent_load"))
    {
        UnpicklerObject *obj = UnpicklerObject_CAST(self);
        Py_XINCREF(value);
        Py_XSETREF(obj->persistent_load_attr, value);
        return 0;
    }

    return PyObject_GenericSetAttr(self, name, value);
}

static PyGetSetDef Unpickler_getsets[] = {
    {"memo", Unpickler_get_memo, Unpickler_set_memo},
    {NULL}
};

static PyType_Slot unpickler_type_slots[] = {
    {Py_tp_dealloc, Unpickler_dealloc},
    {Py_tp_doc, (char *)_pickle_Unpickler___init____doc__},
    {Py_tp_getattro, Unpickler_getattr},
    {Py_tp_setattro, Unpickler_setattr},
    {Py_tp_traverse, Unpickler_traverse},
    {Py_tp_clear, Unpickler_clear},
    {Py_tp_methods, Unpickler_methods},
    {Py_tp_getset, Unpickler_getsets},
    {Py_tp_init, _pickle_Unpickler___init__},
    {Py_tp_alloc, PyType_GenericAlloc},
    {Py_tp_new, PyType_GenericNew},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec unpickler_type_spec = {
    .name = "_pickle.Unpickler",
    .basicsize = sizeof(UnpicklerObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = unpickler_type_slots,
};

/*[clinic input]

_pickle.dump

  obj: object
  file: object
  protocol: object = None
  *
  fix_imports: bool = True
  buffer_callback: object = None

Write a pickled representation of obj to the open file object file.

This is equivalent to ``Pickler(file, protocol).dump(obj)``, but may
be more efficient.

The optional *protocol* argument tells the pickler to use the given
protocol; supported protocols are 0, 1, 2, 3, 4 and 5.  The default
protocol is 5. It was introduced in Python 3.8, and is incompatible
with previous versions.

Specifying a negative protocol version selects the highest protocol
version supported.  The higher the protocol used, the more recent the
version of Python needed to read the pickle produced.

The *file* argument must have a write() method that accepts a single
bytes argument.  It can thus be a file object opened for binary
writing, an io.BytesIO instance, or any other custom object that meets
this interface.

If *fix_imports* is True and protocol is less than 3, pickle will try
to map the new Python 3 names to the old module names used in Python
2, so that the pickle data stream is readable with Python 2.

If *buffer_callback* is None (the default), buffer views are serialized
into *file* as part of the pickle stream.  It is an error if
*buffer_callback* is not None and *protocol* is None or smaller than 5.

[clinic start generated code]*/

static PyObject *
_pickle_dump_impl(PyObject *module, PyObject *obj, PyObject *file,
                  PyObject *protocol, int fix_imports,
                  PyObject *buffer_callback)
/*[clinic end generated code: output=706186dba996490c input=b89ce8d0e911fd46]*/
{
    PickleState *state = _Pickle_GetState(module);
    PicklerObject *pickler = _Pickler_New(state);

    if (pickler == NULL)
        return NULL;

    if (_Pickler_SetProtocol(pickler, protocol, fix_imports) < 0)
        goto error;

    if (_Pickler_SetOutputStream(pickler, file) < 0)
        goto error;

    if (_Pickler_SetBufferCallback(pickler, buffer_callback) < 0)
        goto error;

    if (dump(state, pickler, obj) < 0)
        goto error;

    if (_Pickler_FlushToFile(pickler) < 0)
        goto error;

    Py_DECREF(pickler);
    Py_RETURN_NONE;

  error:
    Py_XDECREF(pickler);
    return NULL;
}

/*[clinic input]

_pickle.dumps

  obj: object
  protocol: object = None
  *
  fix_imports: bool = True
  buffer_callback: object = None

Return the pickled representation of the object as a bytes object.

The optional *protocol* argument tells the pickler to use the given
protocol; supported protocols are 0, 1, 2, 3, 4 and 5.  The default
protocol is 5. It was introduced in Python 3.8, and is incompatible
with previous versions.

Specifying a negative protocol version selects the highest protocol
version supported.  The higher the protocol used, the more recent the
version of Python needed to read the pickle produced.

If *fix_imports* is True and *protocol* is less than 3, pickle will
try to map the new Python 3 names to the old module names used in
Python 2, so that the pickle data stream is readable with Python 2.

If *buffer_callback* is None (the default), buffer views are serialized
into *file* as part of the pickle stream.  It is an error if
*buffer_callback* is not None and *protocol* is None or smaller than 5.

[clinic start generated code]*/

static PyObject *
_pickle_dumps_impl(PyObject *module, PyObject *obj, PyObject *protocol,
                   int fix_imports, PyObject *buffer_callback)
/*[clinic end generated code: output=fbab0093a5580fdf input=139fc546886c63ac]*/
{
    PyObject *result;
    PickleState *state = _Pickle_GetState(module);
    PicklerObject *pickler = _Pickler_New(state);

    if (pickler == NULL)
        return NULL;

    if (_Pickler_SetProtocol(pickler, protocol, fix_imports) < 0)
        goto error;

    if (_Pickler_SetBufferCallback(pickler, buffer_callback) < 0)
        goto error;

    if (dump(state, pickler, obj) < 0)
        goto error;

    result = _Pickler_GetString(pickler);
    Py_DECREF(pickler);
    return result;

  error:
    Py_XDECREF(pickler);
    return NULL;
}

/*[clinic input]

_pickle.load

  file: object
  *
  fix_imports: bool = True
  encoding: str = 'ASCII'
  errors: str = 'strict'
  buffers: object(c_default="NULL") = ()

Read and return an object from the pickle data stored in a file.

This is equivalent to ``Unpickler(file).load()``, but may be more
efficient.

The protocol version of the pickle is detected automatically, so no
protocol argument is needed.  Bytes past the pickled object's
representation are ignored.

The argument *file* must have two methods, a read() method that takes
an integer argument, and a readline() method that requires no
arguments.  Both methods should return bytes.  Thus *file* can be a
binary file object opened for reading, an io.BytesIO object, or any
other custom object that meets this interface.

Optional keyword arguments are *fix_imports*, *encoding* and *errors*,
which are used to control compatibility support for pickle stream
generated by Python 2.  If *fix_imports* is True, pickle will try to
map the old Python 2 names to the new names used in Python 3.  The
*encoding* and *errors* tell pickle how to decode 8-bit string
instances pickled by Python 2; these default to 'ASCII' and 'strict',
respectively.  The *encoding* can be 'bytes' to read these 8-bit
string instances as bytes objects.
[clinic start generated code]*/

static PyObject *
_pickle_load_impl(PyObject *module, PyObject *file, int fix_imports,
                  const char *encoding, const char *errors,
                  PyObject *buffers)
/*[clinic end generated code: output=250452d141c23e76 input=46c7c31c92f4f371]*/
{
    PyObject *result;
    UnpicklerObject *unpickler = _Unpickler_New(module);

    if (unpickler == NULL)
        return NULL;

    if (_Unpickler_SetInputStream(unpickler, file) < 0)
        goto error;

    if (_Unpickler_SetInputEncoding(unpickler, encoding, errors) < 0)
        goto error;

    if (_Unpickler_SetBuffers(unpickler, buffers) < 0)
        goto error;

    unpickler->fix_imports = fix_imports;

    PickleState *state = _Pickle_GetState(module);
    result = load(state, unpickler);
    Py_DECREF(unpickler);
    return result;

  error:
    Py_XDECREF(unpickler);
    return NULL;
}

/*[clinic input]

_pickle.loads

  data: object
  /
  *
  fix_imports: bool = True
  encoding: str = 'ASCII'
  errors: str = 'strict'
  buffers: object(c_default="NULL") = ()

Read and return an object from the given pickle data.

The protocol version of the pickle is detected automatically, so no
protocol argument is needed.  Bytes past the pickled object's
representation are ignored.

Optional keyword arguments are *fix_imports*, *encoding* and *errors*,
which are used to control compatibility support for pickle stream
generated by Python 2.  If *fix_imports* is True, pickle will try to
map the old Python 2 names to the new names used in Python 3.  The
*encoding* and *errors* tell pickle how to decode 8-bit string
instances pickled by Python 2; these default to 'ASCII' and 'strict',
respectively.  The *encoding* can be 'bytes' to read these 8-bit
string instances as bytes objects.
[clinic start generated code]*/

static PyObject *
_pickle_loads_impl(PyObject *module, PyObject *data, int fix_imports,
                   const char *encoding, const char *errors,
                   PyObject *buffers)
/*[clinic end generated code: output=82ac1e6b588e6d02 input=b3615540d0535087]*/
{
    PyObject *result;
    UnpicklerObject *unpickler = _Unpickler_New(module);

    if (unpickler == NULL)
        return NULL;

    if (_Unpickler_SetStringInput(unpickler, data) < 0)
        goto error;

    if (_Unpickler_SetInputEncoding(unpickler, encoding, errors) < 0)
        goto error;

    if (_Unpickler_SetBuffers(unpickler, buffers) < 0)
        goto error;

    unpickler->fix_imports = fix_imports;

    PickleState *state = _Pickle_GetState(module);
    result = load(state, unpickler);
    Py_DECREF(unpickler);
    return result;

  error:
    Py_XDECREF(unpickler);
    return NULL;
}

static struct PyMethodDef pickle_methods[] = {
    _PICKLE_DUMP_METHODDEF
    _PICKLE_DUMPS_METHODDEF
    _PICKLE_LOAD_METHODDEF
    _PICKLE_LOADS_METHODDEF
    {NULL, NULL} /* sentinel */
};

static int
pickle_clear(PyObject *m)
{
    _Pickle_ClearState(_Pickle_GetState(m));
    return 0;
}

static void
pickle_free(void *m)
{
    _Pickle_ClearState(_Pickle_GetState((PyObject*)m));
}

static int
pickle_traverse(PyObject *m, visitproc visit, void *arg)
{
    PickleState *st = _Pickle_GetState(m);
    Py_VISIT(st->PickleError);
    Py_VISIT(st->PicklingError);
    Py_VISIT(st->UnpicklingError);
    Py_VISIT(st->dispatch_table);
    Py_VISIT(st->extension_registry);
    Py_VISIT(st->extension_cache);
    Py_VISIT(st->inverted_registry);
    Py_VISIT(st->name_mapping_2to3);
    Py_VISIT(st->import_mapping_2to3);
    Py_VISIT(st->name_mapping_3to2);
    Py_VISIT(st->import_mapping_3to2);
    Py_VISIT(st->codecs_encode);
    Py_VISIT(st->getattr);
    Py_VISIT(st->partial);
    Py_VISIT(st->Pickler_Type);
    Py_VISIT(st->Unpickler_Type);
    Py_VISIT(st->Pdata_Type);
    Py_VISIT(st->PicklerMemoProxyType);
    Py_VISIT(st->UnpicklerMemoProxyType);
    return 0;
}

static int
_pickle_exec(PyObject *m)
{
    PickleState *st = _Pickle_GetState(m);

#define CREATE_TYPE(mod, type, spec)                                        \
    do {                                                                    \
        type = (PyTypeObject *)PyType_FromMetaclass(NULL, mod, spec, NULL); \
        if (type == NULL) {                                                 \
            return -1;                                                      \
        }                                                                   \
    } while (0)

    CREATE_TYPE(m, st->Pdata_Type, &pdata_spec);
    CREATE_TYPE(m, st->PicklerMemoProxyType, &memoproxy_spec);
    CREATE_TYPE(m, st->UnpicklerMemoProxyType, &unpickler_memoproxy_spec);
    CREATE_TYPE(m, st->Pickler_Type, &pickler_type_spec);
    CREATE_TYPE(m, st->Unpickler_Type, &unpickler_type_spec);

#undef CREATE_TYPE

    /* Add types */
    if (PyModule_AddType(m, &PyPickleBuffer_Type) < 0) {
        return -1;
    }
    if (PyModule_AddType(m, st->Pickler_Type) < 0) {
        return -1;
    }
    if (PyModule_AddType(m, st->Unpickler_Type) < 0) {
        return -1;
    }

    /* Initialize the exceptions. */
    st->PickleError = PyErr_NewException("_pickle.PickleError", NULL, NULL);
    if (st->PickleError == NULL)
        return -1;
    st->PicklingError = \
        PyErr_NewException("_pickle.PicklingError", st->PickleError, NULL);
    if (st->PicklingError == NULL)
        return -1;
    st->UnpicklingError = \
        PyErr_NewException("_pickle.UnpicklingError", st->PickleError, NULL);
    if (st->UnpicklingError == NULL)
        return -1;

    if (PyModule_AddObjectRef(m, "PickleError", st->PickleError) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "PicklingError", st->PicklingError) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "UnpicklingError", st->UnpicklingError) < 0) {
        return -1;
    }

    if (_Pickle_InitState(st) < 0)
        return -1;

    return 0;
}

static PyModuleDef_Slot pickle_slots[] = {
    {Py_mod_exec, _pickle_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL},
};

static struct PyModuleDef _picklemodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_pickle",
    .m_doc = pickle_module_doc,
    .m_size = sizeof(PickleState),
    .m_methods = pickle_methods,
    .m_slots = pickle_slots,
    .m_traverse = pickle_traverse,
    .m_clear = pickle_clear,
    .m_free = pickle_free,
};

PyMODINIT_FUNC
PyInit__pickle(void)
{
    return PyModuleDef_Init(&_picklemodule);
}
