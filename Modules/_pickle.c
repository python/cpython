#include "Python.h"
#include "structmember.h"

PyDoc_STRVAR(pickle_module_doc,
"Optimized C implementation for the Python pickle module.");

/* Bump this when new opcodes are added to the pickle protocol. */
enum {
    HIGHEST_PROTOCOL = 3,
    DEFAULT_PROTOCOL = 3
};

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
    SHORT_BINBYTES = 'C'
};

/* These aren't opcodes -- they're ways to pickle bools before protocol 2
 * so that unpicklers written before bools were introduced unpickle them
 * as ints, but unpicklers after can recognize that bools were intended.
 * Note that protocol 2 added direct ways to pickle bools.
 */
#undef TRUE
#define TRUE  "I01\n"
#undef FALSE
#define FALSE "I00\n"

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

    /* Maximum size of the write buffer of Pickler when pickling to a
       stream.  This is ignored for in-memory pickling. */
    MAX_WRITE_BUF_SIZE = 64 * 1024,

    /* Prefetch size when unpickling (disabled on unpeekable streams) */
    PREFETCH = 8192 * 16
};

/* Exception classes for pickle. These should override the ones defined in
   pickle.py, when the C-optimized Pickler and Unpickler are used. */
static PyObject *PickleError = NULL;
static PyObject *PicklingError = NULL;
static PyObject *UnpicklingError = NULL;

/* copyreg.dispatch_table, {type_object: pickling_function} */
static PyObject *dispatch_table = NULL;
/* For EXT[124] opcodes. */
/* copyreg._extension_registry, {(module_name, function_name): code} */
static PyObject *extension_registry = NULL;
/* copyreg._inverted_registry, {code: (module_name, function_name)} */
static PyObject *inverted_registry = NULL;
/* copyreg._extension_cache, {code: object} */
static PyObject *extension_cache = NULL;

/* _compat_pickle.NAME_MAPPING, {(oldmodule, oldname): (newmodule, newname)} */
static PyObject *name_mapping_2to3 = NULL;
/* _compat_pickle.IMPORT_MAPPING, {oldmodule: newmodule} */
static PyObject *import_mapping_2to3 = NULL;
/* Same, but with REVERSE_NAME_MAPPING / REVERSE_IMPORT_MAPPING */
static PyObject *name_mapping_3to2 = NULL;
static PyObject *import_mapping_3to2 = NULL;

/* XXX: Are these really nescessary? */
/* As the name says, an empty tuple. */
static PyObject *empty_tuple = NULL;
/* For looking up name pairs in copyreg._extension_registry. */
static PyObject *two_tuple = NULL;

static int
stack_underflow(void)
{
    PyErr_SetString(UnpicklingError, "unpickling stack underflow");
    return -1;
}

/* Internal data type used as the unpickling stack. */
typedef struct {
    PyObject_VAR_HEAD
    PyObject **data;
    Py_ssize_t allocated;  /* number of slots in data allocated */
} Pdata;

static void
Pdata_dealloc(Pdata *self)
{
    int i = Py_SIZE(self);
    while (--i >= 0) {
        Py_DECREF(self->data[i]);
    }
    PyMem_FREE(self->data);
    PyObject_Del(self);
}

static PyTypeObject Pdata_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pickle.Pdata",              /*tp_name*/
    sizeof(Pdata),                /*tp_basicsize*/
    0,                            /*tp_itemsize*/
    (destructor)Pdata_dealloc,    /*tp_dealloc*/
};

static PyObject *
Pdata_New(void)
{
    Pdata *self;

    if (!(self = PyObject_New(Pdata, &Pdata_Type)))
        return NULL;
    Py_SIZE(self) = 0;
    self->allocated = 8;
    self->data = PyMem_MALLOC(self->allocated * sizeof(PyObject *));
    if (self->data)
        return (PyObject *)self;
    Py_DECREF(self);
    return PyErr_NoMemory();
}


/* Retain only the initial clearto items.  If clearto >= the current
 * number of items, this is a (non-erroneous) NOP.
 */
static int
Pdata_clear(Pdata *self, int clearto)
{
    int i = Py_SIZE(self);

    if (clearto < 0)
        return stack_underflow();
    if (clearto >= i)
        return 0;

    while (--i >= clearto) {
        Py_CLEAR(self->data[i]);
    }
    Py_SIZE(self) = clearto;
    return 0;
}

static int
Pdata_grow(Pdata *self)
{
    PyObject **data = self->data;
    Py_ssize_t allocated = self->allocated;
    Py_ssize_t new_allocated;

    new_allocated = (allocated >> 3) + 6;
    /* check for integer overflow */
    if (new_allocated > PY_SSIZE_T_MAX - allocated)
        goto nomemory;
    new_allocated += allocated;
    if (new_allocated > (PY_SSIZE_T_MAX / sizeof(PyObject *)))
        goto nomemory;
    data = PyMem_REALLOC(data, new_allocated * sizeof(PyObject *));
    if (data == NULL)
        goto nomemory;

    self->data = data;
    self->allocated = new_allocated;
    return 0;

  nomemory:
    PyErr_NoMemory();
    return -1;
}

/* D is a Pdata*.  Pop the topmost element and store it into V, which
 * must be an lvalue holding PyObject*.  On stack underflow, UnpicklingError
 * is raised and V is set to NULL.
 */
static PyObject *
Pdata_pop(Pdata *self)
{
    if (Py_SIZE(self) == 0) {
        PyErr_SetString(UnpicklingError, "bad pickle data");
        return NULL;
    }
    return self->data[--Py_SIZE(self)];
}
#define PDATA_POP(D, V) do { (V) = Pdata_pop((D)); } while (0)

static int
Pdata_push(Pdata *self, PyObject *obj)
{
    if (Py_SIZE(self) == self->allocated && Pdata_grow(self) < 0) {
        return -1;
    }
    self->data[Py_SIZE(self)++] = obj;
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
Pdata_poptuple(Pdata *self, Py_ssize_t start)
{
    PyObject *tuple;
    Py_ssize_t len, i, j;

    len = Py_SIZE(self) - start;
    tuple = PyTuple_New(len);
    if (tuple == NULL)
        return NULL;
    for (i = start, j = 0; j < len; i++, j++)
        PyTuple_SET_ITEM(tuple, j, self->data[i]);

    Py_SIZE(self) = start;
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

    Py_SIZE(self) = start;
    return list;
}

typedef struct {
    PyObject *me_key;
    long me_value;
} PyMemoEntry;

typedef struct {
    Py_ssize_t mt_mask;
    Py_ssize_t mt_used;
    Py_ssize_t mt_allocated;
    PyMemoEntry *mt_table;
} PyMemoTable;

typedef struct PicklerObject {
    PyObject_HEAD
    PyMemoTable *memo;          /* Memo table, keep track of the seen
                                   objects to support self-referential objects
                                   pickling. */
    PyObject *pers_func;        /* persistent_id() method, can be NULL */
    PyObject *arg;

    PyObject *write;            /* write() method of the output stream. */
    PyObject *output_buffer;    /* Write into a local bytearray buffer before
                                   flushing to the stream. */
    Py_ssize_t output_len;      /* Length of output_buffer. */
    Py_ssize_t max_output_len;  /* Allocation size of output_buffer. */
    int proto;                  /* Pickle protocol number, >= 0 */
    int bin;                    /* Boolean, true if proto > 0 */
    int buf_size;               /* Size of the current buffered pickle data */
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
} PicklerObject;

typedef struct UnpicklerObject {
    PyObject_HEAD
    Pdata *stack;               /* Pickle data stack, store unpickled objects. */

    /* The unpickler memo is just an array of PyObject *s. Using a dict
       is unnecessary, since the keys are contiguous ints. */
    PyObject **memo;
    Py_ssize_t memo_size;

    PyObject *arg;
    PyObject *pers_func;        /* persistent_load() method, can be NULL. */

    Py_buffer buffer;
    char *input_buffer;
    char *input_line;
    Py_ssize_t input_len;
    Py_ssize_t next_read_idx;
    Py_ssize_t prefetched_idx;  /* index of first prefetched byte */
    PyObject *read;             /* read() method of the input stream. */
    PyObject *readline;         /* readline() method of the input stream. */
    PyObject *peek;             /* peek() method of the input stream, or NULL */

    char *encoding;             /* Name of the encoding to be used for
                                   decoding strings pickled using Python
                                   2.x. The default value is "ASCII" */
    char *errors;               /* Name of errors handling scheme to used when
                                   decoding strings. The default value is
                                   "strict". */
    int *marks;                 /* Mark stack, used for unpickling container
                                   objects. */
    Py_ssize_t num_marks;       /* Number of marks in the mark stack. */
    Py_ssize_t marks_size;      /* Current allocated size of the mark stack. */
    int proto;                  /* Protocol of the pickle loaded. */
    int fix_imports;            /* Indicate whether Unpickler should fix
                                   the name of globals pickled by Python 2.x. */
} UnpicklerObject;

/* Forward declarations */
static int save(PicklerObject *, PyObject *, int);
static int save_reduce(PicklerObject *, PyObject *, PyObject *);
static PyTypeObject Pickler_Type;
static PyTypeObject Unpickler_Type;


/*************************************************************************
 A custom hashtable mapping void* to longs. This is used by the pickler for
 memoization. Using a custom hashtable rather than PyDict allows us to skip
 a bunch of unnecessary object creation. This makes a huge performance
 difference. */

#define MT_MINSIZE 8
#define PERTURB_SHIFT 5


static PyMemoTable *
PyMemoTable_New(void)
{
    PyMemoTable *memo = PyMem_MALLOC(sizeof(PyMemoTable));
    if (memo == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    memo->mt_used = 0;
    memo->mt_allocated = MT_MINSIZE;
    memo->mt_mask = MT_MINSIZE - 1;
    memo->mt_table = PyMem_MALLOC(MT_MINSIZE * sizeof(PyMemoEntry));
    if (memo->mt_table == NULL) {
        PyMem_FREE(memo);
        PyErr_NoMemory();
        return NULL;
    }
    memset(memo->mt_table, 0, MT_MINSIZE * sizeof(PyMemoEntry));

    return memo;
}

static PyMemoTable *
PyMemoTable_Copy(PyMemoTable *self)
{
    Py_ssize_t i;
    PyMemoTable *new = PyMemoTable_New();
    if (new == NULL)
        return NULL;

    new->mt_used = self->mt_used;
    new->mt_allocated = self->mt_allocated;
    new->mt_mask = self->mt_mask;
    /* The table we get from _New() is probably smaller than we wanted.
       Free it and allocate one that's the right size. */
    PyMem_FREE(new->mt_table);
    new->mt_table = PyMem_MALLOC(self->mt_allocated * sizeof(PyMemoEntry));
    if (new->mt_table == NULL) {
        PyMem_FREE(new);
        return NULL;
    }
    for (i = 0; i < self->mt_allocated; i++) {
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

    PyMem_FREE(self->mt_table);
    PyMem_FREE(self);
}

/* Since entries cannot be deleted from this hashtable, _PyMemoTable_Lookup()
   can be considerably simpler than dictobject.c's lookdict(). */
static PyMemoEntry *
_PyMemoTable_Lookup(PyMemoTable *self, PyObject *key)
{
    size_t i;
    size_t perturb;
    size_t mask = (size_t)self->mt_mask;
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
    assert(0);  /* Never reached */
    return NULL;
}

/* Returns -1 on failure, 0 on success. */
static int
_PyMemoTable_ResizeTable(PyMemoTable *self, Py_ssize_t min_size)
{
    PyMemoEntry *oldtable = NULL;
    PyMemoEntry *oldentry, *newentry;
    Py_ssize_t new_size = MT_MINSIZE;
    Py_ssize_t to_process;

    assert(min_size > 0);

    /* Find the smallest valid table size >= min_size. */
    while (new_size < min_size && new_size > 0)
        new_size <<= 1;
    if (new_size <= 0) {
        PyErr_NoMemory();
        return -1;
    }
    /* new_size needs to be a power of two. */
    assert((new_size & (new_size - 1)) == 0);

    /* Allocate new table. */
    oldtable = self->mt_table;
    self->mt_table = PyMem_MALLOC(new_size * sizeof(PyMemoEntry));
    if (self->mt_table == NULL) {
        PyMem_FREE(oldtable);
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
    PyMem_FREE(oldtable);
    return 0;
}

/* Returns NULL on failure, a pointer to the value otherwise. */
static long *
PyMemoTable_Get(PyMemoTable *self, PyObject *key)
{
    PyMemoEntry *entry = _PyMemoTable_Lookup(self, key);
    if (entry->me_key == NULL)
        return NULL;
    return &entry->me_value;
}

/* Returns -1 on failure, 0 on success. */
static int
PyMemoTable_Set(PyMemoTable *self, PyObject *key, long value)
{
    PyMemoEntry *entry;

    assert(key != NULL);

    entry = _PyMemoTable_Lookup(self, key);
    if (entry->me_key != NULL) {
        entry->me_value = value;
        return 0;
    }
    Py_INCREF(key);
    entry->me_key = key;
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
    if (!(self->mt_used * 3 >= (self->mt_mask + 1) * 2))
        return 0;
    return _PyMemoTable_ResizeTable(self,
        (self->mt_used > 50000 ? 2 : 4) * self->mt_used);
}

#undef MT_MINSIZE
#undef PERTURB_SHIFT

/*************************************************************************/

/* Helpers for creating the argument tuple passed to functions. This has the
   performance advantage of calling PyTuple_New() only once. 

   XXX(avassalotti): Inline directly in _Pickler_FastCall() and
   _Unpickler_FastCall(). */
#define ARG_TUP(self, obj) do {                             \
        if ((self)->arg || ((self)->arg=PyTuple_New(1))) {  \
            Py_XDECREF(PyTuple_GET_ITEM((self)->arg, 0));   \
            PyTuple_SET_ITEM((self)->arg, 0, (obj));        \
        }                                                   \
        else {                                              \
            Py_DECREF((obj));                               \
        }                                                   \
    } while (0)

#define FREE_ARG_TUP(self) do {                 \
        if ((self)->arg->ob_refcnt > 1)         \
            Py_CLEAR((self)->arg);              \
    } while (0)

/* A temporary cleaner API for fast single argument function call.

   XXX: Does caching the argument tuple provides any real performance benefits?

   A quick benchmark, on a 2.0GHz Athlon64 3200+ running Linux 2.6.24 with
   glibc 2.7, tells me that it takes roughly 20,000,000 PyTuple_New(1) calls
   when the tuple is retrieved from the freelist (i.e, call PyTuple_New() then
   immediately DECREF it) and 1,200,000 calls when allocating brand new tuples
   (i.e, call PyTuple_New() and store the returned value in an array), to save
   one second (wall clock time). Either ways, the loading time a pickle stream
   large enough to generate this number of calls would be massively
   overwhelmed by other factors, like I/O throughput, the GC traversal and
   object allocation overhead. So, I really doubt these functions provide any
   real benefits.

   On the other hand, oprofile reports that pickle spends a lot of time in
   these functions. But, that is probably more related to the function call
   overhead, than the argument tuple allocation.

   XXX: And, what is the reference behavior of these? Steal, borrow? At first
   glance, it seems to steal the reference of 'arg' and borrow the reference
   of 'func'. */
static PyObject *
_Pickler_FastCall(PicklerObject *self, PyObject *func, PyObject *arg)
{
    PyObject *result = NULL;

    ARG_TUP(self, arg);
    if (self->arg) {
        result = PyObject_Call(func, self->arg, NULL);
        FREE_ARG_TUP(self);
    }
    return result;
}

static int
_Pickler_ClearBuffer(PicklerObject *self)
{
    Py_CLEAR(self->output_buffer);
    self->output_buffer =
        PyBytes_FromStringAndSize(NULL, self->max_output_len);
    if (self->output_buffer == NULL)
        return -1;
    self->output_len = 0;
    return 0;
}

static PyObject *
_Pickler_GetString(PicklerObject *self)
{
    PyObject *output_buffer = self->output_buffer;

    assert(self->output_buffer != NULL);
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

    output = _Pickler_GetString(self);
    if (output == NULL)
        return -1;

    result = _Pickler_FastCall(self, self->write, output);
    Py_XDECREF(result);
    return (result == NULL) ? -1 : 0;
}

static int
_Pickler_Write(PicklerObject *self, const char *s, Py_ssize_t n)
{
    Py_ssize_t i, required;
    char *buffer;

    assert(s != NULL);

    required = self->output_len + n;
    if (required > self->max_output_len) {
        if (self->write != NULL && required > MAX_WRITE_BUF_SIZE) {
            /* XXX This reallocates a new buffer every time, which is a bit
               wasteful. */
            if (_Pickler_FlushToFile(self) < 0)
                return -1;
            if (_Pickler_ClearBuffer(self) < 0)
                return -1;
        }
        if (self->write != NULL && n > MAX_WRITE_BUF_SIZE) {
            /* we already flushed above, so the buffer is empty */
            PyObject *result;
            /* XXX we could spare an intermediate copy and pass
               a memoryview instead */
            PyObject *output = PyBytes_FromStringAndSize(s, n);
            if (s == NULL)
                return -1;
            result = _Pickler_FastCall(self, self->write, output);
            Py_XDECREF(result);
            return (result == NULL) ? -1 : 0;
        }
        else {
            if (self->output_len >= PY_SSIZE_T_MAX / 2 - n) {
                PyErr_NoMemory();
                return -1;
            }
            self->max_output_len = (self->output_len + n) * 2;
            if (_PyBytes_Resize(&self->output_buffer, self->max_output_len) < 0)
                return -1;
        }
    }
    buffer = PyBytes_AS_STRING(self->output_buffer);
    if (n < 8) {
        /* This is faster than memcpy when the string is short. */
        for (i = 0; i < n; i++) {
            buffer[self->output_len + i] = s[i];
        }
    }
    else {
        memcpy(buffer + self->output_len, s, n);
    }
    self->output_len += n;
    return n;
}

static PicklerObject *
_Pickler_New(void)
{
    PicklerObject *self;

    self = PyObject_GC_New(PicklerObject, &Pickler_Type);
    if (self == NULL)
        return NULL;

    self->pers_func = NULL;
    self->arg = NULL;
    self->write = NULL;
    self->proto = 0;
    self->bin = 0;
    self->fast = 0;
    self->fast_nesting = 0;
    self->fix_imports = 0;
    self->fast_memo = NULL;

    self->memo = PyMemoTable_New();
    if (self->memo == NULL) {
        Py_DECREF(self);
        return NULL;
    }
    self->max_output_len = WRITE_BUF_SIZE;
    self->output_len = 0;
    self->output_buffer = PyBytes_FromStringAndSize(NULL,
                                                    self->max_output_len);
    if (self->output_buffer == NULL) {
        Py_DECREF(self);
        return NULL;
    }
    return self;
}

static int
_Pickler_SetProtocol(PicklerObject *self, PyObject *proto_obj,
                     PyObject *fix_imports_obj)
{
    long proto = 0;
    int fix_imports;

    if (proto_obj == NULL || proto_obj == Py_None)
        proto = DEFAULT_PROTOCOL;
    else {
        proto = PyLong_AsLong(proto_obj);
        if (proto == -1 && PyErr_Occurred())
            return -1;
    }
    if (proto < 0)
        proto = HIGHEST_PROTOCOL;
    if (proto > HIGHEST_PROTOCOL) {
        PyErr_Format(PyExc_ValueError, "pickle protocol must be <= %d",
                     HIGHEST_PROTOCOL);
        return -1;
    }
    fix_imports = PyObject_IsTrue(fix_imports_obj);
    if (fix_imports == -1)
        return -1;
    
    self->proto = proto;
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
    self->write = PyObject_GetAttrString(file, "write");
    if (self->write == NULL) {
        if (PyErr_ExceptionMatches(PyExc_AttributeError))
            PyErr_SetString(PyExc_TypeError,
                            "file must have a 'write' attribute");
        return -1;
    }

    return 0;
}

/* See documentation for _Pickler_FastCall(). */
static PyObject *
_Unpickler_FastCall(UnpicklerObject *self, PyObject *func, PyObject *arg)
{
    PyObject *result = NULL;

    ARG_TUP(self, arg);
    if (self->arg) {
        result = PyObject_Call(func, self->arg, NULL);
        FREE_ARG_TUP(self);
    }
    return result;
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
_Unpickler_SkipConsumed(UnpicklerObject *self)
{
    Py_ssize_t consumed = self->next_read_idx - self->prefetched_idx;

    if (consumed > 0) {
        PyObject *r;
        assert(self->peek);  /* otherwise we did something wrong */
        /* This makes an useless copy... */
        r = PyObject_CallFunction(self->read, "n", consumed);
        if (r == NULL)
            return -1;
        Py_DECREF(r);
        self->prefetched_idx = self->next_read_idx;
    }
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
    Py_ssize_t read_size, prefetched_size = 0;

    assert(self->read != NULL);
    
    if (_Unpickler_SkipConsumed(self) < 0)
        return -1;

    if (n == READ_WHOLE_LINE)
        data = PyObject_Call(self->readline, empty_tuple, NULL);
    else {
        PyObject *len = PyLong_FromSsize_t(n);
        if (len == NULL)
            return -1;
        data = _Unpickler_FastCall(self, self->read, len);
    }
    if (data == NULL)
        return -1;

    /* Prefetch some data without advancing the file pointer, if possible */
    if (self->peek) {
        PyObject *len, *prefetched;
        len = PyLong_FromSsize_t(PREFETCH);
        if (len == NULL) {
            Py_DECREF(data);
            return -1;
        }
        prefetched = _Unpickler_FastCall(self, self->peek, len);
        if (prefetched == NULL) {
            if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
                /* peek() is probably not supported by the given file object */
                PyErr_Clear();
                Py_CLEAR(self->peek);
            }
            else {
                Py_DECREF(data);
                return -1;
            }
        }
        else {
            assert(PyBytes_Check(prefetched));
            prefetched_size = PyBytes_GET_SIZE(prefetched);
            PyBytes_ConcatAndDel(&data, prefetched);
            if (data == NULL)
                return -1;
        }
    }

    read_size = _Unpickler_SetStringInput(self, data) - prefetched_size;
    Py_DECREF(data);
    self->prefetched_idx = read_size;
    return read_size;
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
static Py_ssize_t
_Unpickler_Read(UnpicklerObject *self, char **s, Py_ssize_t n)
{
    Py_ssize_t num_read;

    if (self->next_read_idx + n <= self->input_len) {
        *s = self->input_buffer + self->next_read_idx;
        self->next_read_idx += n;
        return n;
    }
    if (!self->read) {
        PyErr_Format(PyExc_EOFError, "Ran out of input");
        return -1;
    }
    num_read = _Unpickler_ReadFromFile(self, n);
    if (num_read < 0)
        return -1;
    if (num_read < n) {
        PyErr_Format(PyExc_EOFError, "Ran out of input");
        return -1;
    }
    *s = self->input_buffer;
    self->next_read_idx = n;
    return n;
}

static Py_ssize_t
_Unpickler_CopyLine(UnpicklerObject *self, char *line, Py_ssize_t len,
                    char **result)
{
    char *input_line = PyMem_Realloc(self->input_line, len + 1);
    if (input_line == NULL)
        return -1;

    memcpy(input_line, line, len);
    input_line[len] = '\0';
    self->input_line = input_line;
    *result = self->input_line;
    return len;
}

/* Read a line from the input stream/buffer. If we run off the end of the input
   before hitting \n, return the data we found.

   Returns the number of chars read, or -1 on failure. */
static Py_ssize_t
_Unpickler_Readline(UnpicklerObject *self, char **result)
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
    if (self->read) {
        num_read = _Unpickler_ReadFromFile(self, READ_WHOLE_LINE);
        if (num_read < 0)
            return -1;
        *result = self->input_buffer;
        self->next_read_idx = num_read;
        return num_read;
    }
 
    /* If we get here, we've run off the end of the input string. Return the
       remaining string and let the caller figure it out. */
    *result = self->input_buffer + self->next_read_idx;
    num_read = i - self->next_read_idx;
    self->next_read_idx = i;
    return num_read;
}

/* Returns -1 (with an exception set) on failure, 0 on success. The memo array
   will be modified in place. */
static int
_Unpickler_ResizeMemoList(UnpicklerObject *self, Py_ssize_t new_size)
{
    Py_ssize_t i;
    PyObject **memo;

    assert(new_size > self->memo_size);

    memo = PyMem_REALLOC(self->memo, new_size * sizeof(PyObject *));
    if (memo == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    self->memo = memo;
    for (i = self->memo_size; i < new_size; i++)
        self->memo[i] = NULL;
    self->memo_size = new_size;
    return 0;
}

/* Returns NULL if idx is out of bounds. */
static PyObject *
_Unpickler_MemoGet(UnpicklerObject *self, Py_ssize_t idx)
{
    if (idx < 0 || idx >= self->memo_size)
        return NULL;

    return self->memo[idx];
}

/* Returns -1 (with an exception set) on failure, 0 on success.
   This takes its own reference to `value`. */
static int
_Unpickler_MemoPut(UnpicklerObject *self, Py_ssize_t idx, PyObject *value)
{
    PyObject *old_item;

    if (idx >= self->memo_size) {
        if (_Unpickler_ResizeMemoList(self, idx * 2) < 0)
            return -1;
        assert(idx < self->memo_size);
    }
    Py_INCREF(value);
    old_item = self->memo[idx];
    self->memo[idx] = value;
    Py_XDECREF(old_item);
    return 0;
}

static PyObject **
_Unpickler_NewMemo(Py_ssize_t new_size)
{
    PyObject **memo = PyMem_MALLOC(new_size * sizeof(PyObject *));
    if (memo == NULL)
        return NULL;
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
    PyMem_FREE(memo);
}

static UnpicklerObject *
_Unpickler_New(void)
{
    UnpicklerObject *self;

    self = PyObject_GC_New(UnpicklerObject, &Unpickler_Type);
    if (self == NULL)
        return NULL;

    self->stack = (Pdata *)Pdata_New();
    if (self->stack == NULL) {
        Py_DECREF(self);
        return NULL;
    }
    memset(&self->buffer, 0, sizeof(Py_buffer));

    self->memo_size = 32;
    self->memo = _Unpickler_NewMemo(self->memo_size);
    if (self->memo == NULL) {
        Py_DECREF(self);
        return NULL;
    }

    self->arg = NULL;
    self->pers_func = NULL;
    self->input_buffer = NULL;
    self->input_line = NULL;
    self->input_len = 0;
    self->next_read_idx = 0;
    self->prefetched_idx = 0;
    self->read = NULL;
    self->readline = NULL;
    self->peek = NULL;
    self->encoding = NULL;
    self->errors = NULL;
    self->marks = NULL;
    self->num_marks = 0;
    self->marks_size = 0;
    self->proto = 0;
    self->fix_imports = 0;

    return self;
}

/* Returns -1 (with an exception set) on failure, 0 on success. This may
   be called once on a freshly created Pickler. */
static int
_Unpickler_SetInputStream(UnpicklerObject *self, PyObject *file)
{
    self->peek = PyObject_GetAttrString(file, "peek");
    if (self->peek == NULL) {
        if (PyErr_ExceptionMatches(PyExc_AttributeError))
            PyErr_Clear();
        else
            return -1;
    }
    self->read = PyObject_GetAttrString(file, "read");
    self->readline = PyObject_GetAttrString(file, "readline");
    if (self->readline == NULL || self->read == NULL) {
        if (PyErr_ExceptionMatches(PyExc_AttributeError))
            PyErr_SetString(PyExc_TypeError,
                            "file must have 'read' and 'readline' attributes");
        Py_CLEAR(self->read);
        Py_CLEAR(self->readline);
        Py_CLEAR(self->peek);
        return -1;
    }
    return 0;
}

/* Returns -1 (with an exception set) on failure, 0 on success. This may
   be called once on a freshly created Pickler. */
static int
_Unpickler_SetInputEncoding(UnpicklerObject *self,
                            const char *encoding,
                            const char *errors)
{
    if (encoding == NULL)
        encoding = "ASCII";
    if (errors == NULL)
        errors = "strict";

    self->encoding = strdup(encoding);
    self->errors = strdup(errors);
    if (self->encoding == NULL || self->errors == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    return 0;
}

/* Generate a GET opcode for an object stored in the memo. */
static int
memo_get(PicklerObject *self, PyObject *key)
{
    long *value;
    char pdata[30];
    int len;

    value = PyMemoTable_Get(self->memo, key);
    if (value == NULL)  {
        PyErr_SetObject(PyExc_KeyError, key);
        return -1;
    }

    if (!self->bin) {
        pdata[0] = GET;
        PyOS_snprintf(pdata + 1, sizeof(pdata) - 1, "%ld\n", *value);
        len = (int)strlen(pdata);
    }
    else {
        if (*value < 256) {
            pdata[0] = BINGET;
            pdata[1] = (unsigned char)(*value & 0xff);
            len = 2;
        }
        else if (*value <= 0xffffffffL) {
            pdata[0] = LONG_BINGET;
            pdata[1] = (unsigned char)(*value & 0xff);
            pdata[2] = (unsigned char)((*value >> 8) & 0xff);
            pdata[3] = (unsigned char)((*value >> 16) & 0xff);
            pdata[4] = (unsigned char)((*value >> 24) & 0xff);
            len = 5;
        }
        else { /* unlikely */
            PyErr_SetString(PicklingError,
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
memo_put(PicklerObject *self, PyObject *obj)
{
    long x;
    char pdata[30];
    int len;
    int status = 0;

    if (self->fast)
        return 0;

    x = PyMemoTable_Size(self->memo);
    if (PyMemoTable_Set(self->memo, obj, x) < 0)
        goto error;

    if (!self->bin) {
        pdata[0] = PUT;
        PyOS_snprintf(pdata + 1, sizeof(pdata) - 1, "%ld\n", x);
        len = strlen(pdata);
    }
    else {
        if (x < 256) {
            pdata[0] = BINPUT;
            pdata[1] = (unsigned char)x;
            len = 2;
        }
        else if (x <= 0xffffffffL) {
            pdata[0] = LONG_BINPUT;
            pdata[1] = (unsigned char)(x & 0xff);
            pdata[2] = (unsigned char)((x >> 8) & 0xff);
            pdata[3] = (unsigned char)((x >> 16) & 0xff);
            pdata[4] = (unsigned char)((x >> 24) & 0xff);
            len = 5;
        }
        else { /* unlikely */
            PyErr_SetString(PicklingError,
                            "memo id too large for LONG_BINPUT");
            return -1;
        }
    }

    if (_Pickler_Write(self, pdata, len) < 0)
        goto error;

    if (0) {
  error:
        status = -1;
    }

    return status;
}

static PyObject *
whichmodule(PyObject *global, PyObject *global_name)
{
    Py_ssize_t i, j;
    static PyObject *module_str = NULL;
    static PyObject *main_str = NULL;
    PyObject *module_name;
    PyObject *modules_dict;
    PyObject *module;
    PyObject *obj;

    if (module_str == NULL) {
        module_str = PyUnicode_InternFromString("__module__");
        if (module_str == NULL)
            return NULL;
        main_str = PyUnicode_InternFromString("__main__");
        if (main_str == NULL)
            return NULL;
    }

    module_name = PyObject_GetAttr(global, module_str);

    /* In some rare cases (e.g., bound methods of extension types),
       __module__ can be None. If it is so, then search sys.modules
       for the module of global.  */
    if (module_name == Py_None) {
        Py_DECREF(module_name);
        goto search;
    }

    if (module_name) {
        return module_name;
    }
    if (PyErr_ExceptionMatches(PyExc_AttributeError))
        PyErr_Clear();
    else
        return NULL;

  search:
    modules_dict = PySys_GetObject("modules");
    if (modules_dict == NULL)
        return NULL;

    i = 0;
    module_name = NULL;
    while ((j = PyDict_Next(modules_dict, &i, &module_name, &module))) {
        if (PyObject_RichCompareBool(module_name, main_str, Py_EQ) == 1)
            continue;

        obj = PyObject_GetAttr(module, global_name);
        if (obj == NULL) {
            if (PyErr_ExceptionMatches(PyExc_AttributeError))
                PyErr_Clear();
            else
                return NULL;
            continue;
        }

        if (obj != global) {
            Py_DECREF(obj);
            continue;
        }

        Py_DECREF(obj);
        break;
    }

    /* If no module is found, use __main__. */
    if (!j) {
        module_name = main_str;
    }

    Py_INCREF(module_name);
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
        if (key == NULL)
            return 0;
        if (PyDict_GetItem(self->fast_memo, key)) {
            Py_DECREF(key);
            PyErr_Format(PyExc_ValueError,
                         "fast mode: can't pickle cyclic objects "
                         "including object type %.200s at %p",
                         obj->ob_type->tp_name, obj);
            self->fast_nesting = -1;
            return 0;
        }
        if (PyDict_SetItem(self->fast_memo, key, Py_None) < 0) {
            Py_DECREF(key);
            self->fast_nesting = -1;
            return 0;
        }
        Py_DECREF(key);
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
    static const char *buf[2] = { FALSE, TRUE };
    const char len[2] = {sizeof(FALSE) - 1, sizeof(TRUE) - 1};
    int p = (obj == Py_True);

    if (self->proto >= 2) {
        const char bool_op = p ? NEWTRUE : NEWFALSE;
        if (_Pickler_Write(self, &bool_op, 1) < 0)
            return -1;
    }
    else if (_Pickler_Write(self, buf[p], len[p]) < 0)
        return -1;

    return 0;
}

static int
save_int(PicklerObject *self, long x)
{
    char pdata[32];
    int len = 0;

    if (!self->bin
#if SIZEOF_LONG > 4
        || x > 0x7fffffffL || x < -0x80000000L
#endif
        ) {
        /* Text-mode pickle, or long too big to fit in the 4-byte
         * signed BININT format:  store as a string.
         */
        pdata[0] = LONG;        /* use LONG for consistency with pickle.py */
        PyOS_snprintf(pdata + 1, sizeof(pdata) - 1, "%ldL\n", x);
        if (_Pickler_Write(self, pdata, strlen(pdata)) < 0)
            return -1;
    }
    else {
        /* Binary pickle and x fits in a signed 4-byte int. */
        pdata[1] = (unsigned char)(x & 0xff);
        pdata[2] = (unsigned char)((x >> 8) & 0xff);
        pdata[3] = (unsigned char)((x >> 16) & 0xff);
        pdata[4] = (unsigned char)((x >> 24) & 0xff);

        if ((pdata[4] == 0) && (pdata[3] == 0)) {
            if (pdata[2] == 0) {
                pdata[0] = BININT1;
                len = 2;
            }
            else {
                pdata[0] = BININT2;
                len = 3;
            }
        }
        else {
            pdata[0] = BININT;
            len = 5;
        }

        if (_Pickler_Write(self, pdata, len) < 0)
            return -1;
    }

    return 0;
}

static int
save_long(PicklerObject *self, PyObject *obj)
{
    PyObject *repr = NULL;
    Py_ssize_t size;
    long val = PyLong_AsLong(obj);
    int status = 0;

    const char long_op = LONG;

    if (val == -1 && PyErr_Occurred()) {
        /* out of range for int pickling */
        PyErr_Clear();
    }
    else
        return save_int(self, val);

    if (self->proto >= 2) {
        /* Linear-time pickling. */
        size_t nbits;
        size_t nbytes;
        unsigned char *pdata;
        char header[5];
        int i;
        int sign = _PyLong_Sign(obj);

        if (sign == 0) {
            header[0] = LONG1;
            header[1] = 0;      /* It's 0 -- an empty bytestring. */
            if (_Pickler_Write(self, header, 2) < 0)
                goto error;
            return 0;
        }
        nbits = _PyLong_NumBits(obj);
        if (nbits == (size_t)-1 && PyErr_Occurred())
            goto error;
        /* How many bytes do we need?  There are nbits >> 3 full
         * bytes of data, and nbits & 7 leftover bits.  If there
         * are any leftover bits, then we clearly need another
         * byte.  Wnat's not so obvious is that we *probably*
         * need another byte even if there aren't any leftovers:
         * the most-significant bit of the most-significant byte
         * acts like a sign bit, and it's usually got a sense
         * opposite of the one we need.  The exception is longs
         * of the form -(2**(8*j-1)) for j > 0.  Such a long is
         * its own 256's-complement, so has the right sign bit
         * even without the extra byte.  That's a pain to check
         * for in advance, though, so we always grab an extra
         * byte at the start, and cut it back later if possible.
         */
        nbytes = (nbits >> 3) + 1;
        if (nbytes > INT_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "long too large to pickle");
            goto error;
        }
        repr = PyBytes_FromStringAndSize(NULL, (Py_ssize_t)nbytes);
        if (repr == NULL)
            goto error;
        pdata = (unsigned char *)PyBytes_AS_STRING(repr);
        i = _PyLong_AsByteArray((PyLongObject *)obj,
                                pdata, nbytes,
                                1 /* little endian */ , 1 /* signed */ );
        if (i < 0)
            goto error;
        /* If the long is negative, this may be a byte more than
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
            size = (int)nbytes;
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
        char *string;

        /* proto < 2: write the repr and newline.  This is quadratic-time (in
           the number of digits), in both directions.  We add a trailing 'L'
           to the repr, for compatibility with Python 2.x. */

        repr = PyObject_Repr(obj);
        if (repr == NULL)
            goto error;

        string = _PyUnicode_AsStringAndSize(repr, &size);
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
        if (_PyFloat_Pack8(x, (unsigned char *)&pdata[1], 0) < 0)
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

        buf = PyOS_double_to_string(x, 'g', 17, 0, NULL);
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

static int
save_bytes(PicklerObject *self, PyObject *obj)
{
    if (self->proto < 3) {
        /* Older pickle protocols do not have an opcode for pickling bytes
           objects. Therefore, we need to fake the copy protocol (i.e.,
           the __reduce__ method) to permit bytes object unpickling. */
        PyObject *reduce_value = NULL;
        PyObject *bytelist = NULL;
        int status;

        bytelist = PySequence_List(obj);
        if (bytelist == NULL)
            return -1;

        reduce_value = Py_BuildValue("(O(O))", (PyObject *)&PyBytes_Type,
                                     bytelist);
        if (reduce_value == NULL) {
            Py_DECREF(bytelist);
            return -1;
        }

        /* save_reduce() will memoize the object automatically. */
        status = save_reduce(self, reduce_value, obj);
        Py_DECREF(reduce_value);
        Py_DECREF(bytelist);
        return status;
    }
    else {
        Py_ssize_t size;
        char header[5];
        int len;

        size = PyBytes_Size(obj);
        if (size < 0)
            return -1;

        if (size < 256) {
            header[0] = SHORT_BINBYTES;
            header[1] = (unsigned char)size;
            len = 2;
        }
        else if (size <= 0xffffffffL) {
            header[0] = BINBYTES;
            header[1] = (unsigned char)(size & 0xff);
            header[2] = (unsigned char)((size >> 8) & 0xff);
            header[3] = (unsigned char)((size >> 16) & 0xff);
            header[4] = (unsigned char)((size >> 24) & 0xff);
            len = 5;
        }
        else {
            return -1;          /* string too large */
        }

        if (_Pickler_Write(self, header, len) < 0)
            return -1;

        if (_Pickler_Write(self, PyBytes_AS_STRING(obj), size) < 0)
            return -1;

        if (memo_put(self, obj) < 0)
            return -1;

        return 0;
    }
}

/* A copy of PyUnicode_EncodeRawUnicodeEscape() that also translates
   backslash and newline characters to \uXXXX escapes. */
static PyObject *
raw_unicode_escape(const Py_UNICODE *s, Py_ssize_t size)
{
    PyObject *repr, *result;
    char *p;
    char *q;

    static const char *hexdigits = "0123456789abcdef";

#ifdef Py_UNICODE_WIDE
    const Py_ssize_t expandsize = 10;
#else
    const Py_ssize_t expandsize = 6;
#endif
    
    if (size > PY_SSIZE_T_MAX / expandsize)
        return PyErr_NoMemory();
    
    repr = PyByteArray_FromStringAndSize(NULL, expandsize * size);
    if (repr == NULL)
        return NULL;
    if (size == 0)
        goto done;

    p = q = PyByteArray_AS_STRING(repr);
    while (size-- > 0) {
        Py_UNICODE ch = *s++;
#ifdef Py_UNICODE_WIDE
        /* Map 32-bit characters to '\Uxxxxxxxx' */
        if (ch >= 0x10000) {
            *p++ = '\\';
            *p++ = 'U';
            *p++ = hexdigits[(ch >> 28) & 0xf];
            *p++ = hexdigits[(ch >> 24) & 0xf];
            *p++ = hexdigits[(ch >> 20) & 0xf];
            *p++ = hexdigits[(ch >> 16) & 0xf];
            *p++ = hexdigits[(ch >> 12) & 0xf];
            *p++ = hexdigits[(ch >> 8) & 0xf];
            *p++ = hexdigits[(ch >> 4) & 0xf];
            *p++ = hexdigits[ch & 15];
        }
        else
#else
            /* Map UTF-16 surrogate pairs to '\U00xxxxxx' */
            if (ch >= 0xD800 && ch < 0xDC00) {
                Py_UNICODE ch2;
                Py_UCS4 ucs;

                ch2 = *s++;
                size--;
                if (ch2 >= 0xDC00 && ch2 <= 0xDFFF) {
                    ucs = (((ch & 0x03FF) << 10) | (ch2 & 0x03FF)) + 0x00010000;
                    *p++ = '\\';
                    *p++ = 'U';
                    *p++ = hexdigits[(ucs >> 28) & 0xf];
                    *p++ = hexdigits[(ucs >> 24) & 0xf];
                    *p++ = hexdigits[(ucs >> 20) & 0xf];
                    *p++ = hexdigits[(ucs >> 16) & 0xf];
                    *p++ = hexdigits[(ucs >> 12) & 0xf];
                    *p++ = hexdigits[(ucs >> 8) & 0xf];
                    *p++ = hexdigits[(ucs >> 4) & 0xf];
                    *p++ = hexdigits[ucs & 0xf];
                    continue;
                }
                /* Fall through: isolated surrogates are copied as-is */
                s--;
                size++;
            }
#endif
        /* Map 16-bit characters to '\uxxxx' */
        if (ch >= 256 || ch == '\\' || ch == '\n') {
            *p++ = '\\';
            *p++ = 'u';
            *p++ = hexdigits[(ch >> 12) & 0xf];
            *p++ = hexdigits[(ch >> 8) & 0xf];
            *p++ = hexdigits[(ch >> 4) & 0xf];
            *p++ = hexdigits[ch & 15];
        }
        /* Copy everything else as-is */
        else
            *p++ = (char) ch;
    }
    size = p - q;

  done:
    result = PyBytes_FromStringAndSize(PyByteArray_AS_STRING(repr), size);
    Py_DECREF(repr);
    return result;
}

static int
save_unicode(PicklerObject *self, PyObject *obj)
{
    Py_ssize_t size;
    PyObject *encoded = NULL;

    if (self->bin) {
        char pdata[5];

        encoded = PyUnicode_EncodeUTF8(PyUnicode_AS_UNICODE(obj),
                                    PyUnicode_GET_SIZE(obj),
                                    "surrogatepass");
        if (encoded == NULL)
            goto error;

        size = PyBytes_GET_SIZE(encoded);
        if (size < 0 || size > 0xffffffffL)
            goto error;          /* string too large */

        pdata[0] = BINUNICODE;
        pdata[1] = (unsigned char)(size & 0xff);
        pdata[2] = (unsigned char)((size >> 8) & 0xff);
        pdata[3] = (unsigned char)((size >> 16) & 0xff);
        pdata[4] = (unsigned char)((size >> 24) & 0xff);

        if (_Pickler_Write(self, pdata, 5) < 0)
            goto error;

        if (_Pickler_Write(self, PyBytes_AS_STRING(encoded), size) < 0)
            goto error;
    }
    else {
        const char unicode_op = UNICODE;

        encoded = raw_unicode_escape(PyUnicode_AS_UNICODE(obj),
                                     PyUnicode_GET_SIZE(obj));
        if (encoded == NULL)
            goto error;

        if (_Pickler_Write(self, &unicode_op, 1) < 0)
            goto error;

        size = PyBytes_GET_SIZE(encoded);
        if (_Pickler_Write(self, PyBytes_AS_STRING(encoded), size) < 0)
            goto error;

        if (_Pickler_Write(self, "\n", 1) < 0)
            goto error;
    }
    if (memo_put(self, obj) < 0)
        goto error;

    Py_DECREF(encoded);
    return 0;

  error:
    Py_XDECREF(encoded);
    return -1;
}

/* A helper for save_tuple.  Push the len elements in tuple t on the stack. */
static int
store_tuple_elements(PicklerObject *self, PyObject *t, int len)
{
    int i;

    assert(PyTuple_Size(t) == len);

    for (i = 0; i < len; i++) {
        PyObject *element = PyTuple_GET_ITEM(t, i);

        if (element == NULL)
            return -1;
        if (save(self, element, 0) < 0)
            return -1;
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
save_tuple(PicklerObject *self, PyObject *obj)
{
    int len, i;

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
        if (store_tuple_elements(self, obj, len) < 0)
            return -1;

        if (PyMemoTable_Get(self->memo, obj)) {
            /* pop the len elements */
            for (i = 0; i < len; i++)
                if (_Pickler_Write(self, &pop_op, 1) < 0)
                    return -1;
            /* fetch from memo */
            if (memo_get(self, obj) < 0)
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

    if (store_tuple_elements(self, obj, len) < 0)
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
        if (memo_get(self, obj) < 0)
            return -1;

        return 0;
    }
    else { /* Not recursive. */
        if (_Pickler_Write(self, &tuple_op, 1) < 0)
            return -1;
    }

  memoize:
    if (memo_put(self, obj) < 0)
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
batch_list(PicklerObject *self, PyObject *iter)
{
    PyObject *obj = NULL;
    PyObject *firstitem = NULL;
    int i, n;

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
        for (;;) {
            obj = PyIter_Next(iter);
            if (obj == NULL) {
                if (PyErr_Occurred())
                    return -1;
                break;
            }
            i = save(self, obj, 0);
            Py_DECREF(obj);
            if (i < 0)
                return -1;
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
            if (save(self, firstitem, 0) < 0)
                goto error;
            if (_Pickler_Write(self, &append_op, 1) < 0)
                goto error;
            Py_CLEAR(firstitem);
            break;
        }

        /* More than one item to write */

        /* Pump out MARK, items, APPENDS. */
        if (_Pickler_Write(self, &mark_op, 1) < 0)
            goto error;

        if (save(self, firstitem, 0) < 0)
            goto error;
        Py_CLEAR(firstitem);
        n = 1;

        /* Fetch and save up to BATCHSIZE items */
        while (obj) {
            if (save(self, obj, 0) < 0)
                goto error;
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
batch_list_exact(PicklerObject *self, PyObject *obj)
{
    PyObject *item = NULL;
    int this_batch, total;

    const char append_op = APPEND;
    const char appends_op = APPENDS;
    const char mark_op = MARK;

    assert(obj != NULL);
    assert(self->proto > 0);
    assert(PyList_CheckExact(obj));

    if (PyList_GET_SIZE(obj) == 1) {
        item = PyList_GET_ITEM(obj, 0);
        if (save(self, item, 0) < 0)
            return -1;
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
            if (save(self, item, 0) < 0)
                return -1;
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
save_list(PicklerObject *self, PyObject *obj)
{
    char header[3];
    int len;
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

    if (memo_put(self, obj) < 0)
        goto error;

    if (len != 0) {
        /* Materialize the list elements. */
        if (PyList_CheckExact(obj) && self->proto > 0) {
            if (Py_EnterRecursiveCall(" while pickling an object"))
                goto error;
            status = batch_list_exact(self, obj);
            Py_LeaveRecursiveCall();
        } else {
            PyObject *iter = PyObject_GetIter(obj);
            if (iter == NULL)
                goto error;

            if (Py_EnterRecursiveCall(" while pickling an object")) {
                Py_DECREF(iter);
                goto error;
            }
            status = batch_list(self, iter);
            Py_LeaveRecursiveCall();
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
batch_dict(PicklerObject *self, PyObject *iter)
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
                return -1;
            }
            i = save(self, PyTuple_GET_ITEM(obj, 0), 0);
            if (i >= 0)
                i = save(self, PyTuple_GET_ITEM(obj, 1), 0);
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
            if (save(self, PyTuple_GET_ITEM(firstitem, 0), 0) < 0)
                goto error;
            if (save(self, PyTuple_GET_ITEM(firstitem, 1), 0) < 0)
                goto error;
            if (_Pickler_Write(self, &setitem_op, 1) < 0)
                goto error;
            Py_CLEAR(firstitem);
            break;
        }

        /* More than one item to write */

        /* Pump out MARK, items, SETITEMS. */
        if (_Pickler_Write(self, &mark_op, 1) < 0)
            goto error;

        if (save(self, PyTuple_GET_ITEM(firstitem, 0), 0) < 0)
            goto error;
        if (save(self, PyTuple_GET_ITEM(firstitem, 1), 0) < 0)
            goto error;
        Py_CLEAR(firstitem);
        n = 1;

        /* Fetch and save up to BATCHSIZE items */
        while (obj) {
            if (!PyTuple_Check(obj) || PyTuple_Size(obj) != 2) {
                PyErr_SetString(PyExc_TypeError, "dict items "
                    "iterator must return 2-tuples");
                goto error;
            }
            if (save(self, PyTuple_GET_ITEM(obj, 0), 0) < 0 ||
                save(self, PyTuple_GET_ITEM(obj, 1), 0) < 0)
                goto error;
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
batch_dict_exact(PicklerObject *self, PyObject *obj)
{
    PyObject *key = NULL, *value = NULL;
    int i;
    Py_ssize_t dict_size, ppos = 0;

    const char mark_op = MARK;
    const char setitem_op = SETITEM;
    const char setitems_op = SETITEMS;

    assert(obj != NULL);
    assert(self->proto > 0);

    dict_size = PyDict_Size(obj);

    /* Special-case len(d) == 1 to save space. */
    if (dict_size == 1) {
        PyDict_Next(obj, &ppos, &key, &value);
        if (save(self, key, 0) < 0)
            return -1;
        if (save(self, value, 0) < 0)
            return -1;
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
            if (save(self, key, 0) < 0)
                return -1;
            if (save(self, value, 0) < 0)
                return -1;
            if (++i == BATCHSIZE)
                break;
        }
        if (_Pickler_Write(self, &setitems_op, 1) < 0)
            return -1;
        if (PyDict_Size(obj) != dict_size) {
            PyErr_Format(
                PyExc_RuntimeError,
                "dictionary changed size during iteration");
            return -1;
        }

    } while (i == BATCHSIZE);
    return 0;
}

static int
save_dict(PicklerObject *self, PyObject *obj)
{
    PyObject *items, *iter;
    char header[3];
    int len;
    int status = 0;

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

    /* Get dict size, and bow out early if empty. */
    if ((len = PyDict_Size(obj)) < 0)
        goto error;

    if (memo_put(self, obj) < 0)
        goto error;

    if (len != 0) {
        /* Save the dict items. */
        if (PyDict_CheckExact(obj) && self->proto > 0) {
            /* We can take certain shortcuts if we know this is a dict and
               not a dict subclass. */
            if (Py_EnterRecursiveCall(" while pickling an object"))
                goto error;
            status = batch_dict_exact(self, obj);
            Py_LeaveRecursiveCall();
        } else {
            items = PyObject_CallMethod(obj, "items", "()");
            if (items == NULL)
                goto error;
            iter = PyObject_GetIter(items);
            Py_DECREF(items);
            if (iter == NULL)
                goto error;
            if (Py_EnterRecursiveCall(" while pickling an object")) {
                Py_DECREF(iter);
                goto error;
            }
            status = batch_dict(self, iter);
            Py_LeaveRecursiveCall();
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
save_global(PicklerObject *self, PyObject *obj, PyObject *name)
{
    static PyObject *name_str = NULL;
    PyObject *global_name = NULL;
    PyObject *module_name = NULL;
    PyObject *module = NULL;
    PyObject *cls;
    int status = 0;

    const char global_op = GLOBAL;

    if (name_str == NULL) {
        name_str = PyUnicode_InternFromString("__name__");
        if (name_str == NULL)
            goto error;
    }

    if (name) {
        global_name = name;
        Py_INCREF(global_name);
    }
    else {
        global_name = PyObject_GetAttr(obj, name_str);
        if (global_name == NULL)
            goto error;
    }

    module_name = whichmodule(obj, global_name);
    if (module_name == NULL)
        goto error;

    /* XXX: Change to use the import C API directly with level=0 to disallow
       relative imports.

       XXX: PyImport_ImportModuleLevel could be used. However, this bypasses
       builtins.__import__. Therefore, _pickle, unlike pickle.py, will ignore
       custom import functions (IMHO, this would be a nice security
       feature). The import C API would need to be extended to support the
       extra parameters of __import__ to fix that. */
    module = PyImport_Import(module_name);
    if (module == NULL) {
        PyErr_Format(PicklingError,
                     "Can't pickle %R: import of module %R failed",
                     obj, module_name);
        goto error;
    }
    cls = PyObject_GetAttr(module, global_name);
    if (cls == NULL) {
        PyErr_Format(PicklingError,
                     "Can't pickle %R: attribute lookup %S.%S failed",
                     obj, module_name, global_name);
        goto error;
    }
    if (cls != obj) {
        Py_DECREF(cls);
        PyErr_Format(PicklingError,
                     "Can't pickle %R: it's not the same object as %S.%S",
                     obj, module_name, global_name);
        goto error;
    }
    Py_DECREF(cls);

    if (self->proto >= 2) {
        /* See whether this is in the extension registry, and if
         * so generate an EXT opcode.
         */
        PyObject *code_obj;      /* extension code as Python object */
        long code;               /* extension code as C value */
        char pdata[5];
        int n;

        PyTuple_SET_ITEM(two_tuple, 0, module_name);
        PyTuple_SET_ITEM(two_tuple, 1, global_name);
        code_obj = PyDict_GetItem(extension_registry, two_tuple);
        /* The object is not registered in the extension registry.
           This is the most likely code path. */
        if (code_obj == NULL)
            goto gen_global;

        /* XXX: pickle.py doesn't check neither the type, nor the range
           of the value returned by the extension_registry. It should for
           consistency. */

        /* Verify code_obj has the right type and value. */
        if (!PyLong_Check(code_obj)) {
            PyErr_Format(PicklingError,
                         "Can't pickle %R: extension code %R isn't an integer",
                         obj, code_obj);
            goto error;
        }
        code = PyLong_AS_LONG(code_obj);
        if (code <= 0 || code > 0x7fffffffL) {
            PyErr_Format(PicklingError,
                         "Can't pickle %R: extension code %ld is out of range",
                         obj, code);
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
        /* Generate a normal global opcode if we are using a pickle
           protocol <= 2, or if the object is not registered in the
           extension registry. */
        PyObject *encoded;
        PyObject *(*unicode_encoder)(PyObject *);

  gen_global:
        if (_Pickler_Write(self, &global_op, 1) < 0)
            goto error;

        /* Since Python 3.0 now supports non-ASCII identifiers, we encode both
           the module name and the global name using UTF-8. We do so only when
           we are using the pickle protocol newer than version 3. This is to
           ensure compatibility with older Unpickler running on Python 2.x. */
        if (self->proto >= 3) {
            unicode_encoder = PyUnicode_AsUTF8String;
        }
        else {
            unicode_encoder = PyUnicode_AsASCIIString;
        }

        /* For protocol < 3 and if the user didn't request against doing so,
           we convert module names to the old 2.x module names. */
        if (self->fix_imports) {
            PyObject *key;
            PyObject *item;

            key = PyTuple_Pack(2, module_name, global_name);
            if (key == NULL)
                goto error;
            item = PyDict_GetItemWithError(name_mapping_3to2, key);
            Py_DECREF(key);
            if (item) {
                if (!PyTuple_Check(item) || PyTuple_GET_SIZE(item) != 2) {
                    PyErr_Format(PyExc_RuntimeError,
                                 "_compat_pickle.REVERSE_NAME_MAPPING values "
                                 "should be 2-tuples, not %.200s",
                                 Py_TYPE(item)->tp_name);
                    goto error;
                }
                Py_CLEAR(module_name);
                Py_CLEAR(global_name);
                module_name = PyTuple_GET_ITEM(item, 0);
                global_name = PyTuple_GET_ITEM(item, 1);
                if (!PyUnicode_Check(module_name) ||
                    !PyUnicode_Check(global_name)) {
                    PyErr_Format(PyExc_RuntimeError,
                                 "_compat_pickle.REVERSE_NAME_MAPPING values "
                                 "should be pairs of str, not (%.200s, %.200s)",
                                 Py_TYPE(module_name)->tp_name,
                                 Py_TYPE(global_name)->tp_name);
                    goto error;
                }
                Py_INCREF(module_name);
                Py_INCREF(global_name);
            }
            else if (PyErr_Occurred()) {
                goto error;
            }

            item = PyDict_GetItemWithError(import_mapping_3to2, module_name);
            if (item) {
                if (!PyUnicode_Check(item)) {
                    PyErr_Format(PyExc_RuntimeError,
                                 "_compat_pickle.REVERSE_IMPORT_MAPPING values "
                                 "should be strings, not %.200s",
                                 Py_TYPE(item)->tp_name);
                    goto error;
                }
                Py_CLEAR(module_name);
                module_name = item;
                Py_INCREF(module_name);
            }
            else if (PyErr_Occurred()) {
                goto error;
            }
        }

        /* Save the name of the module. */
        encoded = unicode_encoder(module_name);
        if (encoded == NULL) {
            if (PyErr_ExceptionMatches(PyExc_UnicodeEncodeError))
                PyErr_Format(PicklingError,
                             "can't pickle module identifier '%S' using "
                             "pickle protocol %i", module_name, self->proto);
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
            if (PyErr_ExceptionMatches(PyExc_UnicodeEncodeError))
                PyErr_Format(PicklingError,
                             "can't pickle global identifier '%S' using "
                             "pickle protocol %i", global_name, self->proto);
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

        /* Memoize the object. */
        if (memo_put(self, obj) < 0)
            goto error;
    }

    if (0) {
  error:
        status = -1;
    }
    Py_XDECREF(module_name);
    Py_XDECREF(global_name);
    Py_XDECREF(module);

    return status;
}

static int
save_pers(PicklerObject *self, PyObject *obj, PyObject *func)
{
    PyObject *pid = NULL;
    int status = 0;

    const char persid_op = PERSID;
    const char binpersid_op = BINPERSID;

    Py_INCREF(obj);
    pid = _Pickler_FastCall(self, func, obj);
    if (pid == NULL)
        return -1;

    if (pid != Py_None) {
        if (self->bin) {
            if (save(self, pid, 1) < 0 ||
                _Pickler_Write(self, &binpersid_op, 1) < 0)
                goto error;
        }
        else {
            PyObject *pid_str = NULL;
            char *pid_ascii_bytes;
            Py_ssize_t size;

            pid_str = PyObject_Str(pid);
            if (pid_str == NULL)
                goto error;

            /* XXX: Should it check whether the persistent id only contains
               ASCII characters? And what if the pid contains embedded
               newlines? */
            pid_ascii_bytes = _PyUnicode_AsStringAndSize(pid_str, &size);
            Py_DECREF(pid_str);
            if (pid_ascii_bytes == NULL)
                goto error;

            if (_Pickler_Write(self, &persid_op, 1) < 0 ||
                _Pickler_Write(self, pid_ascii_bytes, size) < 0 ||
                _Pickler_Write(self, "\n", 1) < 0)
                goto error;
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
    static PyObject *str_class;

    if (str_class == NULL) {
        str_class = PyUnicode_InternFromString("__class__");
        if (str_class == NULL)
            return NULL;
    }
    cls = PyObject_GetAttr(obj, str_class);
    if (cls == NULL) {
        if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PyErr_Clear();
            cls = (PyObject *) Py_TYPE(obj);
            Py_INCREF(cls);
        }
    }
    return cls;
}

/* We're saving obj, and args is the 2-thru-5 tuple returned by the
 * appropriate __reduce__ method for obj.
 */
static int
save_reduce(PicklerObject *self, PyObject *args, PyObject *obj)
{
    PyObject *callable;
    PyObject *argtup;
    PyObject *state = NULL;
    PyObject *listitems = Py_None;
    PyObject *dictitems = Py_None;
    Py_ssize_t size;

    int use_newobj = self->proto >= 2;

    const char reduce_op = REDUCE;
    const char build_op = BUILD;
    const char newobj_op = NEWOBJ;

    size = PyTuple_Size(args);
    if (size < 2 || size > 5) {
        PyErr_SetString(PicklingError, "tuple returned by "
                        "__reduce__ must contain 2 through 5 elements");
        return -1;
    }

    if (!PyArg_UnpackTuple(args, "save_reduce", 2, 5,
                           &callable, &argtup, &state, &listitems, &dictitems))
        return -1;

    if (!PyCallable_Check(callable)) {
        PyErr_SetString(PicklingError, "first item of the tuple "
                        "returned by __reduce__ must be callable");
        return -1;
    }
    if (!PyTuple_Check(argtup)) {
        PyErr_SetString(PicklingError, "second item of the tuple "
                        "returned by __reduce__ must be a tuple");
        return -1;
    }

    if (state == Py_None)
        state = NULL;

    if (listitems == Py_None)
        listitems = NULL;
    else if (!PyIter_Check(listitems)) {
        PyErr_Format(PicklingError, "Fourth element of tuple"
                     "returned by __reduce__ must be an iterator, not %s",
                     Py_TYPE(listitems)->tp_name);
        return -1;
    }

    if (dictitems == Py_None)
        dictitems = NULL;
    else if (!PyIter_Check(dictitems)) {
        PyErr_Format(PicklingError, "Fifth element of tuple"
                     "returned by __reduce__ must be an iterator, not %s",
                     Py_TYPE(dictitems)->tp_name);
        return -1;
    }

    /* Protocol 2 special case: if callable's name is __newobj__, use
       NEWOBJ. */
    if (use_newobj) {
        static PyObject *newobj_str = NULL, *name_str = NULL;
        PyObject *name;

        if (newobj_str == NULL) {
            newobj_str = PyUnicode_InternFromString("__newobj__");
            name_str = PyUnicode_InternFromString("__name__");
            if (newobj_str == NULL || name_str == NULL)
                return -1;
        }

        name = PyObject_GetAttr(callable, name_str);
        if (name == NULL) {
            if (PyErr_ExceptionMatches(PyExc_AttributeError))
                PyErr_Clear();
            else
                return -1;
            use_newobj = 0;
        }
        else {
            use_newobj = PyUnicode_Check(name) &&
                         PyUnicode_Compare(name, newobj_str) == 0;
            Py_DECREF(name);
        }
    }
    if (use_newobj) {
        PyObject *cls;
        PyObject *newargtup;
        PyObject *obj_class;
        int p;

        /* Sanity checks. */
        if (Py_SIZE(argtup) < 1) {
            PyErr_SetString(PicklingError, "__newobj__ arglist is empty");
            return -1;
        }

        cls = PyTuple_GET_ITEM(argtup, 0);
        if (!PyType_Check(cls)) {
            PyErr_SetString(PicklingError, "args[0] from "
                            "__newobj__ args is not a type");
            return -1;
        }

        if (obj != NULL) {
            obj_class = get_class(obj);
            p = obj_class != cls;    /* true iff a problem */
            Py_DECREF(obj_class);
            if (p) {
                PyErr_SetString(PicklingError, "args[0] from "
                                "__newobj__ args has the wrong class");
                return -1;
            }
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
             RuntimeError: maximum recursion depth exceeded

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
        if (save(self, cls, 0) < 0)
            return -1;

        newargtup = PyTuple_GetSlice(argtup, 1, Py_SIZE(argtup));
        if (newargtup == NULL)
            return -1;

        p = save(self, newargtup, 0);
        Py_DECREF(newargtup);
        if (p < 0)
            return -1;

        /* Add NEWOBJ opcode. */
        if (_Pickler_Write(self, &newobj_op, 1) < 0)
            return -1;
    }
    else { /* Not using NEWOBJ. */
        if (save(self, callable, 0) < 0 ||
            save(self, argtup, 0) < 0 ||
            _Pickler_Write(self, &reduce_op, 1) < 0)
            return -1;
    }

    /* obj can be NULL when save_reduce() is used directly. A NULL obj means
       the caller do not want to memoize the object. Not particularly useful,
       but that is to mimic the behavior save_reduce() in pickle.py when
       obj is None. */
    if (obj && memo_put(self, obj) < 0)
        return -1;

    if (listitems && batch_list(self, listitems) < 0)
        return -1;

    if (dictitems && batch_dict(self, dictitems) < 0)
        return -1;

    if (state) {
        if (save(self, state, 0) < 0 || 
            _Pickler_Write(self, &build_op, 1) < 0)
            return -1;
    }

    return 0;
}

static int
save(PicklerObject *self, PyObject *obj, int pers_save)
{
    PyTypeObject *type;
    PyObject *reduce_func = NULL;
    PyObject *reduce_value = NULL;
    int status = 0;

    if (Py_EnterRecursiveCall(" while pickling an object"))
        return -1;

    /* The extra pers_save argument is necessary to avoid calling save_pers()
       on its returned object. */
    if (!pers_save && self->pers_func) {
        /* save_pers() returns:
            -1   to signal an error;
             0   if it did nothing successfully;
             1   if a persistent id was saved.
         */
        if ((status = save_pers(self, obj, self->pers_func)) != 0)
            goto done;
    }

    type = Py_TYPE(obj);

    /* The old cPickle had an optimization that used switch-case statement
       dispatching on the first letter of the type name.  This has was removed
       since benchmarks shown that this optimization was actually slowing
       things down. */

    /* Atom types; these aren't memoized, so don't check the memo. */

    if (obj == Py_None) {
        status = save_none(self, obj);
        goto done;
    }
    else if (obj == Py_False || obj == Py_True) {
        status = save_bool(self, obj);
        goto done;
    }
    else if (type == &PyLong_Type) {
        status = save_long(self, obj);
        goto done;
    }
    else if (type == &PyFloat_Type) {
        status = save_float(self, obj);
        goto done;
    }

    /* Check the memo to see if it has the object. If so, generate
       a GET (or BINGET) opcode, instead of pickling the object
       once again. */
    if (PyMemoTable_Get(self->memo, obj)) {
        if (memo_get(self, obj) < 0)
            goto error;
        goto done;
    }

    if (type == &PyBytes_Type) {
        status = save_bytes(self, obj);
        goto done;
    }
    else if (type == &PyUnicode_Type) {
        status = save_unicode(self, obj);
        goto done;
    }
    else if (type == &PyDict_Type) {
        status = save_dict(self, obj);
        goto done;
    }
    else if (type == &PyList_Type) {
        status = save_list(self, obj);
        goto done;
    }
    else if (type == &PyTuple_Type) {
        status = save_tuple(self, obj);
        goto done;
    }
    else if (type == &PyType_Type) {
        status = save_global(self, obj, NULL);
        goto done;
    }
    else if (type == &PyFunction_Type) {
        status = save_global(self, obj, NULL);
        if (status < 0 && PyErr_ExceptionMatches(PickleError)) {
            /* fall back to reduce */
            PyErr_Clear();
        }
        else {
            goto done;
        }
    }
    else if (type == &PyCFunction_Type) {
        status = save_global(self, obj, NULL);
        goto done;
    }
    else if (PyType_IsSubtype(type, &PyType_Type)) {
        status = save_global(self, obj, NULL);
        goto done;
    }

    /* XXX: This part needs some unit tests. */

    /* Get a reduction callable, and call it.  This may come from
     * copyreg.dispatch_table, the object's __reduce_ex__ method,
     * or the object's __reduce__ method.
     */
    reduce_func = PyDict_GetItem(dispatch_table, (PyObject *)type);
    if (reduce_func != NULL) {
        /* Here, the reference count of the reduce_func object returned by
           PyDict_GetItem needs to be increased to be consistent with the one
           returned by PyObject_GetAttr. This is allow us to blindly DECREF
           reduce_func at the end of the save() routine.
        */
        Py_INCREF(reduce_func);
        Py_INCREF(obj);
        reduce_value = _Pickler_FastCall(self, reduce_func, obj);
    }
    else {
        static PyObject *reduce_str = NULL;
        static PyObject *reduce_ex_str = NULL;

        /* Cache the name of the reduce methods. */
        if (reduce_str == NULL) {
            reduce_str = PyUnicode_InternFromString("__reduce__");
            if (reduce_str == NULL)
                goto error;
            reduce_ex_str = PyUnicode_InternFromString("__reduce_ex__");
            if (reduce_ex_str == NULL)
                goto error;
        }

        /* XXX: If the __reduce__ method is defined, __reduce_ex__ is
           automatically defined as __reduce__. While this is convenient, this
           make it impossible to know which method was actually called. Of
           course, this is not a big deal. But still, it would be nice to let
           the user know which method was called when something go
           wrong. Incidentally, this means if __reduce_ex__ is not defined, we
           don't actually have to check for a __reduce__ method. */

        /* Check for a __reduce_ex__ method. */
        reduce_func = PyObject_GetAttr(obj, reduce_ex_str);
        if (reduce_func != NULL) {
            PyObject *proto;
            proto = PyLong_FromLong(self->proto);
            if (proto != NULL) {
                reduce_value = _Pickler_FastCall(self, reduce_func, proto);
            }
        }
        else {
            if (PyErr_ExceptionMatches(PyExc_AttributeError))
                PyErr_Clear();
            else
                goto error;
            /* Check for a __reduce__ method. */
            reduce_func = PyObject_GetAttr(obj, reduce_str);
            if (reduce_func != NULL) {
                reduce_value = PyObject_Call(reduce_func, empty_tuple, NULL);
            }
            else {
                PyErr_Format(PicklingError, "can't pickle '%.200s' object: %R",
                             type->tp_name, obj);
                goto error;
            }
        }
    }

    if (reduce_value == NULL)
        goto error;

    if (PyUnicode_Check(reduce_value)) {
        status = save_global(self, obj, reduce_value);
        goto done;
    }

    if (!PyTuple_Check(reduce_value)) {
        PyErr_SetString(PicklingError,
                        "__reduce__ must return a string or tuple");
        goto error;
    }

    status = save_reduce(self, reduce_value, obj);

    if (0) {
  error:
        status = -1;
    }
  done:
    Py_LeaveRecursiveCall();
    Py_XDECREF(reduce_func);
    Py_XDECREF(reduce_value);

    return status;
}

static int
dump(PicklerObject *self, PyObject *obj)
{
    const char stop_op = STOP;

    if (self->proto >= 2) {
        char header[2];

        header[0] = PROTO;
        assert(self->proto >= 0 && self->proto < 256);
        header[1] = (unsigned char)self->proto;
        if (_Pickler_Write(self, header, 2) < 0)
            return -1;
    }

    if (save(self, obj, 0) < 0 ||
        _Pickler_Write(self, &stop_op, 1) < 0)
        return -1;

    return 0;
}

PyDoc_STRVAR(Pickler_clear_memo_doc,
"clear_memo() -> None. Clears the pickler's \"memo\"."
"\n"
"The memo is the data structure that remembers which objects the\n"
"pickler has already seen, so that shared or recursive objects are\n"
"pickled by reference and not by value.  This method is useful when\n"
"re-using picklers.");

static PyObject *
Pickler_clear_memo(PicklerObject *self)
{
    if (self->memo)
        PyMemoTable_Clear(self->memo);

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Pickler_dump_doc,
"dump(obj) -> None. Write a pickled representation of obj to the open file.");

static PyObject *
Pickler_dump(PicklerObject *self, PyObject *args)
{
    PyObject *obj;

    /* Check whether the Pickler was initialized correctly (issue3664).
       Developers often forget to call __init__() in their subclasses, which
       would trigger a segfault without this check. */
    if (self->write == NULL) {
        PyErr_Format(PicklingError, 
                     "Pickler.__init__() was not called by %s.__init__()",
                     Py_TYPE(self)->tp_name);
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O:dump", &obj))
        return NULL;

    if (_Pickler_ClearBuffer(self) < 0)
        return NULL;

    if (dump(self, obj) < 0)
        return NULL;

    if (_Pickler_FlushToFile(self) < 0)
        return NULL;

    Py_RETURN_NONE;
}

static struct PyMethodDef Pickler_methods[] = {
    {"dump", (PyCFunction)Pickler_dump, METH_VARARGS,
     Pickler_dump_doc},
    {"clear_memo", (PyCFunction)Pickler_clear_memo, METH_NOARGS,
     Pickler_clear_memo_doc},
    {NULL, NULL}                /* sentinel */
};

static void
Pickler_dealloc(PicklerObject *self)
{
    PyObject_GC_UnTrack(self);

    Py_XDECREF(self->output_buffer);
    Py_XDECREF(self->write);
    Py_XDECREF(self->pers_func);
    Py_XDECREF(self->arg);
    Py_XDECREF(self->fast_memo);

    PyMemoTable_Del(self->memo);

    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int
Pickler_traverse(PicklerObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->write);
    Py_VISIT(self->pers_func);
    Py_VISIT(self->arg);
    Py_VISIT(self->fast_memo);
    return 0;
}

static int
Pickler_clear(PicklerObject *self)
{
    Py_CLEAR(self->output_buffer);
    Py_CLEAR(self->write);
    Py_CLEAR(self->pers_func);
    Py_CLEAR(self->arg);
    Py_CLEAR(self->fast_memo);

    if (self->memo != NULL) {
        PyMemoTable *memo = self->memo;
        self->memo = NULL;
        PyMemoTable_Del(memo);
    }
    return 0;
}


PyDoc_STRVAR(Pickler_doc,
"Pickler(file, protocol=None)"
"\n"
"This takes a binary file for writing a pickle data stream.\n"
"\n"
"The optional protocol argument tells the pickler to use the\n"
"given protocol; supported protocols are 0, 1, 2, 3.  The default\n"
"protocol is 3; a backward-incompatible protocol designed for\n"
"Python 3.0.\n"
"\n"
"Specifying a negative protocol version selects the highest\n"
"protocol version supported.  The higher the protocol used, the\n"
"more recent the version of Python needed to read the pickle\n"
"produced.\n"
"\n"
"The file argument must have a write() method that accepts a single\n"
"bytes argument. It can thus be a file object opened for binary\n"
"writing, a io.BytesIO instance, or any other custom object that\n"
"meets this interface.\n"
"\n"
"If fix_imports is True and protocol is less than 3, pickle will try to\n"
"map the new Python 3.x names to the old module names used in Python\n"
"2.x, so that the pickle data stream is readable with Python 2.x.\n");

static int
Pickler_init(PicklerObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"file", "protocol", "fix_imports", 0};
    PyObject *file;
    PyObject *proto_obj = NULL;
    PyObject *fix_imports = Py_True;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|OO:Pickler",
                                     kwlist, &file, &proto_obj, &fix_imports))
        return -1;

    /* In case of multiple __init__() calls, clear previous content. */
    if (self->write != NULL)
        (void)Pickler_clear(self);

    if (_Pickler_SetProtocol(self, proto_obj, fix_imports) < 0)
        return -1;

    if (_Pickler_SetOutputStream(self, file) < 0)
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

    self->arg = NULL;
    self->fast = 0;
    self->fast_nesting = 0;
    self->fast_memo = NULL;
    self->pers_func = NULL;
    if (PyObject_HasAttrString((PyObject *)self, "persistent_id")) {
        self->pers_func = PyObject_GetAttrString((PyObject *)self,
                                                 "persistent_id");
        if (self->pers_func == NULL)
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

typedef struct {
    PyObject_HEAD
    PicklerObject *pickler; /* Pickler whose memo table we're proxying. */
} PicklerMemoProxyObject;

PyDoc_STRVAR(pmp_clear_doc,
"memo.clear() -> None.  Remove all items from memo.");

static PyObject *
pmp_clear(PicklerMemoProxyObject *self)
{
    if (self->pickler->memo)
        PyMemoTable_Clear(self->pickler->memo);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(pmp_copy_doc,
"memo.copy() -> new_memo.  Copy the memo to a new object.");

static PyObject *
pmp_copy(PicklerMemoProxyObject *self)
{
    Py_ssize_t i;
    PyMemoTable *memo;
    PyObject *new_memo = PyDict_New();
    if (new_memo == NULL)
        return NULL;

    memo = self->pickler->memo;
    for (i = 0; i < memo->mt_allocated; ++i) {
        PyMemoEntry entry = memo->mt_table[i];
        if (entry.me_key != NULL) {
            int status;
            PyObject *key, *value;

            key = PyLong_FromVoidPtr(entry.me_key);
            value = Py_BuildValue("lO", entry.me_value, entry.me_key);

            if (key == NULL || value == NULL) {
                Py_XDECREF(key);
                Py_XDECREF(value);
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

PyDoc_STRVAR(pmp_reduce_doc,
"memo.__reduce__(). Pickling support.");

static PyObject *
pmp_reduce(PicklerMemoProxyObject *self, PyObject *args)
{
    PyObject *reduce_value, *dict_args;
    PyObject *contents = pmp_copy(self);
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
    Py_INCREF((PyObject *)&PyDict_Type);
    PyTuple_SET_ITEM(reduce_value, 0, (PyObject *)&PyDict_Type);
    PyTuple_SET_ITEM(reduce_value, 1, dict_args);
    return reduce_value;
}

static PyMethodDef picklerproxy_methods[] = {
    {"clear",      (PyCFunction)pmp_clear,  METH_NOARGS,  pmp_clear_doc},
    {"copy",       (PyCFunction)pmp_copy,   METH_NOARGS,  pmp_copy_doc},
    {"__reduce__", (PyCFunction)pmp_reduce, METH_VARARGS, pmp_reduce_doc},
    {NULL, NULL} /* sentinel */
};

static void
PicklerMemoProxy_dealloc(PicklerMemoProxyObject *self)
{
    PyObject_GC_UnTrack(self);
    Py_XDECREF(self->pickler);
    PyObject_GC_Del((PyObject *)self);
}

static int
PicklerMemoProxy_traverse(PicklerMemoProxyObject *self,
                          visitproc visit, void *arg)
{
    Py_VISIT(self->pickler);
    return 0;
}

static int
PicklerMemoProxy_clear(PicklerMemoProxyObject *self)
{
    Py_CLEAR(self->pickler);
    return 0;
}

static PyTypeObject PicklerMemoProxyType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pickle.PicklerMemoProxy",                 /*tp_name*/
    sizeof(PicklerMemoProxyObject),             /*tp_basicsize*/
    0,
    (destructor)PicklerMemoProxy_dealloc,       /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_compare */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    PyObject_HashNotImplemented,                /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    PyObject_GenericSetAttr,                    /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    0,                                          /* tp_doc */
    (traverseproc)PicklerMemoProxy_traverse,    /* tp_traverse */
    (inquiry)PicklerMemoProxy_clear,            /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    picklerproxy_methods,                       /* tp_methods */
};

static PyObject *
PicklerMemoProxy_New(PicklerObject *pickler)
{
    PicklerMemoProxyObject *self;

    self = PyObject_GC_New(PicklerMemoProxyObject, &PicklerMemoProxyType);
    if (self == NULL)
        return NULL;
    Py_INCREF(pickler);
    self->pickler = pickler;
    PyObject_GC_Track(self);
    return (PyObject *)self;
}

/*****************************************************************************/

static PyObject *
Pickler_get_memo(PicklerObject *self)
{
    return PicklerMemoProxy_New(self);
}

static int
Pickler_set_memo(PicklerObject *self, PyObject *obj)
{
    PyMemoTable *new_memo = NULL;

    if (obj == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "attribute deletion is not supported");
        return -1;
    }

    if (Py_TYPE(obj) == &PicklerMemoProxyType) {
        PicklerObject *pickler =
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
            long memo_id;
            PyObject *memo_obj;

            if (!PyTuple_Check(value) || Py_SIZE(value) != 2) {
                PyErr_SetString(PyExc_TypeError,
                                "'memo' values must be 2-item tuples");
                goto error;
            }
            memo_id = PyLong_AsLong(PyTuple_GET_ITEM(value, 0));
            if (memo_id == -1 && PyErr_Occurred())
                goto error;
            memo_obj = PyTuple_GET_ITEM(value, 1);
            if (PyMemoTable_Set(new_memo, memo_obj, memo_id) < 0)
                goto error;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "'memo' attribute must be an PicklerMemoProxy object"
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
Pickler_get_persid(PicklerObject *self)
{
    if (self->pers_func == NULL)
        PyErr_SetString(PyExc_AttributeError, "persistent_id");
    else
        Py_INCREF(self->pers_func);
    return self->pers_func;
}

static int
Pickler_set_persid(PicklerObject *self, PyObject *value)
{
    PyObject *tmp;

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "attribute deletion is not supported");
        return -1;
    }
    if (!PyCallable_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "persistent_id must be a callable taking one argument");
        return -1;
    }

    tmp = self->pers_func;
    Py_INCREF(value);
    self->pers_func = value;
    Py_XDECREF(tmp);      /* self->pers_func can be NULL, so be careful. */

    return 0;
}

static PyMemberDef Pickler_members[] = {
    {"bin", T_INT, offsetof(PicklerObject, bin)},
    {"fast", T_INT, offsetof(PicklerObject, fast)},
    {NULL}
};

static PyGetSetDef Pickler_getsets[] = {
    {"memo",          (getter)Pickler_get_memo,
                      (setter)Pickler_set_memo},
    {"persistent_id", (getter)Pickler_get_persid,
                      (setter)Pickler_set_persid},
    {NULL}
};

static PyTypeObject Pickler_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pickle.Pickler"  ,                /*tp_name*/
    sizeof(PicklerObject),              /*tp_basicsize*/
    0,                                  /*tp_itemsize*/
    (destructor)Pickler_dealloc,        /*tp_dealloc*/
    0,                                  /*tp_print*/
    0,                                  /*tp_getattr*/
    0,                                  /*tp_setattr*/
    0,                                  /*tp_reserved*/
    0,                                  /*tp_repr*/
    0,                                  /*tp_as_number*/
    0,                                  /*tp_as_sequence*/
    0,                                  /*tp_as_mapping*/
    0,                                  /*tp_hash*/
    0,                                  /*tp_call*/
    0,                                  /*tp_str*/
    0,                                  /*tp_getattro*/
    0,                                  /*tp_setattro*/
    0,                                  /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    Pickler_doc,                        /*tp_doc*/
    (traverseproc)Pickler_traverse,     /*tp_traverse*/
    (inquiry)Pickler_clear,             /*tp_clear*/
    0,                                  /*tp_richcompare*/
    0,                                  /*tp_weaklistoffset*/
    0,                                  /*tp_iter*/
    0,                                  /*tp_iternext*/
    Pickler_methods,                    /*tp_methods*/
    Pickler_members,                    /*tp_members*/
    Pickler_getsets,                    /*tp_getset*/
    0,                                  /*tp_base*/
    0,                                  /*tp_dict*/
    0,                                  /*tp_descr_get*/
    0,                                  /*tp_descr_set*/
    0,                                  /*tp_dictoffset*/
    (initproc)Pickler_init,             /*tp_init*/
    PyType_GenericAlloc,                /*tp_alloc*/
    PyType_GenericNew,                  /*tp_new*/
    PyObject_GC_Del,                    /*tp_free*/
    0,                                  /*tp_is_gc*/
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
    return PyObject_CallMethod((PyObject *)self, "find_class", "OO",
                               module_name, global_name);
}

static int
marker(UnpicklerObject *self)
{
    if (self->num_marks < 1) {
        PyErr_SetString(UnpicklingError, "could not find MARK");
        return -1;
    }

    return self->marks[--self->num_marks];
}

static int
load_none(UnpicklerObject *self)
{
    PDATA_APPEND(self->stack, Py_None, -1);
    return 0;
}

static int
bad_readline(void)
{
    PyErr_SetString(UnpicklingError, "pickle data was truncated");
    return -1;
}

static int
load_int(UnpicklerObject *self)
{
    PyObject *value;
    char *endptr, *s;
    Py_ssize_t len;
    long x;

    if ((len = _Unpickler_Readline(self, &s)) < 0)
        return -1;
    if (len < 2)
        return bad_readline();

    errno = 0;
    /* XXX: Should the base argument of strtol() be explicitly set to 10? 
       XXX(avassalotti): Should this uses PyOS_strtol()? */
    x = strtol(s, &endptr, 0);

    if (errno || (*endptr != '\n' && *endptr != '\0')) {
        /* Hm, maybe we've got something long.  Let's try reading
         * it as a Python long object. */
        errno = 0;
        /* XXX: Same thing about the base here. */
        value = PyLong_FromString(s, NULL, 0);
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
load_bool(UnpicklerObject *self, PyObject *boolean)
{
    assert(boolean == Py_True || boolean == Py_False);
    PDATA_APPEND(self->stack, boolean, -1);
    return 0;
}

/* s contains x bytes of a little-endian integer.  Return its value as a
 * C int.  Obscure:  when x is 1 or 2, this is an unsigned little-endian
 * int, but when x is 4 it's a signed one.  This is an historical source
 * of x-platform bugs.
 */
static long
calc_binint(char *bytes, int size)
{
    unsigned char *s = (unsigned char *)bytes;
    int i = size;
    long x = 0;

    for (i = 0; i < size; i++) {
        x |= (long)s[i] << (i * 8);
    }

    /* Unlike BININT1 and BININT2, BININT (more accurately BININT4)
     * is signed, so on a box with longs bigger than 4 bytes we need
     * to extend a BININT's sign bit to the full width.
     */
    if (SIZEOF_LONG > 4 && size == 4) {
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
load_binint(UnpicklerObject *self)
{
    char *s;

    if (_Unpickler_Read(self, &s, 4) < 0)
        return -1;

    return load_binintx(self, s, 4);
}

static int
load_binint1(UnpicklerObject *self)
{
    char *s;

    if (_Unpickler_Read(self, &s, 1) < 0)
        return -1;

    return load_binintx(self, s, 1);
}

static int
load_binint2(UnpicklerObject *self)
{
    char *s;

    if (_Unpickler_Read(self, &s, 2) < 0)
        return -1;

    return load_binintx(self, s, 2);
}

static int
load_long(UnpicklerObject *self)
{
    PyObject *value;
    char *s;
    Py_ssize_t len;

    if ((len = _Unpickler_Readline(self, &s)) < 0)
        return -1;
    if (len < 2)
        return bad_readline();

    /* s[len-2] will usually be 'L' (and s[len-1] is '\n'); we need to remove
       the 'L' before calling PyLong_FromString.  In order to maintain
       compatibility with Python 3.0.0, we don't actually *require*
       the 'L' to be present. */
    if (s[len-2] == 'L')
        s[len-2] = '\0';
    /* XXX: Should the base argument explicitly set to 10? */
    value = PyLong_FromString(s, NULL, 0);
    if (value == NULL)
        return -1;

    PDATA_PUSH(self->stack, value, -1);
    return 0;
}

/* 'size' bytes contain the # of bytes of little-endian 256's-complement
 * data following.
 */
static int
load_counted_long(UnpicklerObject *self, int size)
{
    PyObject *value;
    char *nbytes;
    char *pdata;

    assert(size == 1 || size == 4);
    if (_Unpickler_Read(self, &nbytes, size) < 0)
        return -1;

    size = calc_binint(nbytes, size);
    if (size < 0) {
        /* Corrupt or hostile pickle -- we never write one like this */
        PyErr_SetString(UnpicklingError,
                        "LONG pickle has negative byte count");
        return -1;
    }

    if (size == 0)
        value = PyLong_FromLong(0L);
    else {
        /* Read the raw little-endian bytes and convert. */
        if (_Unpickler_Read(self, &pdata, size) < 0)
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
load_float(UnpicklerObject *self)
{
    PyObject *value;
    char *endptr, *s;
    Py_ssize_t len;
    double d;

    if ((len = _Unpickler_Readline(self, &s)) < 0)
        return -1;
    if (len < 2)
        return bad_readline();

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
load_binfloat(UnpicklerObject *self)
{
    PyObject *value;
    double x;
    char *s;

    if (_Unpickler_Read(self, &s, 8) < 0)
        return -1;

    x = _PyFloat_Unpack8((unsigned char *)s, 0);
    if (x == -1.0 && PyErr_Occurred())
        return -1;

    if ((value = PyFloat_FromDouble(x)) == NULL)
        return -1;

    PDATA_PUSH(self->stack, value, -1);
    return 0;
}

static int
load_string(UnpicklerObject *self)
{
    PyObject *bytes;
    PyObject *str = NULL;
    Py_ssize_t len;
    char *s, *p;

    if ((len = _Unpickler_Readline(self, &s)) < 0)
        return -1;
    if (len < 3)
        return bad_readline();
    if ((s = strdup(s)) == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    /* Strip outermost quotes */
    while (s[len - 1] <= ' ')
        len--;
    if (s[0] == '"' && s[len - 1] == '"') {
        s[len - 1] = '\0';
        p = s + 1;
        len -= 2;
    }
    else if (s[0] == '\'' && s[len - 1] == '\'') {
        s[len - 1] = '\0';
        p = s + 1;
        len -= 2;
    }
    else {
        free(s);
        PyErr_SetString(PyExc_ValueError, "insecure string pickle");
        return -1;
    }

    /* Use the PyBytes API to decode the string, since that is what is used
       to encode, and then coerce the result to Unicode. */
    bytes = PyBytes_DecodeEscape(p, len, NULL, 0, NULL);
    free(s);
    if (bytes == NULL)
        return -1;
    str = PyUnicode_FromEncodedObject(bytes, self->encoding, self->errors);
    Py_DECREF(bytes);
    if (str == NULL)
        return -1;

    PDATA_PUSH(self->stack, str, -1);
    return 0;
}

static int
load_binbytes(UnpicklerObject *self)
{
    PyObject *bytes;
    long x;
    char *s;

    if (_Unpickler_Read(self, &s, 4) < 0)
        return -1;

    x = calc_binint(s, 4);
    if (x < 0) {
        PyErr_SetString(UnpicklingError, 
                        "BINBYTES pickle has negative byte count");
        return -1;
    }

    if (_Unpickler_Read(self, &s, x) < 0)
        return -1;
    bytes = PyBytes_FromStringAndSize(s, x);
    if (bytes == NULL)
        return -1;

    PDATA_PUSH(self->stack, bytes, -1);
    return 0;
}

static int
load_short_binbytes(UnpicklerObject *self)
{
    PyObject *bytes;
    unsigned char x;
    char *s;

    if (_Unpickler_Read(self, &s, 1) < 0)
        return -1;

    x = (unsigned char)s[0];

    if (_Unpickler_Read(self, &s, x) < 0)
        return -1;

    bytes = PyBytes_FromStringAndSize(s, x);
    if (bytes == NULL)
        return -1;

    PDATA_PUSH(self->stack, bytes, -1);
    return 0;
}

static int
load_binstring(UnpicklerObject *self)
{
    PyObject *str;
    long x;
    char *s;

    if (_Unpickler_Read(self, &s, 4) < 0)
        return -1;

    x = calc_binint(s, 4);
    if (x < 0) {
        PyErr_SetString(UnpicklingError, 
                        "BINSTRING pickle has negative byte count");
        return -1;
    }

    if (_Unpickler_Read(self, &s, x) < 0)
        return -1;

    /* Convert Python 2.x strings to unicode. */
    str = PyUnicode_Decode(s, x, self->encoding, self->errors);
    if (str == NULL)
        return -1;

    PDATA_PUSH(self->stack, str, -1);
    return 0;
}

static int
load_short_binstring(UnpicklerObject *self)
{
    PyObject *str;
    unsigned char x;
    char *s;

    if (_Unpickler_Read(self, &s, 1) < 0)
        return -1;

    x = (unsigned char)s[0];

    if (_Unpickler_Read(self, &s, x) < 0)
        return -1;

    /* Convert Python 2.x strings to unicode. */
    str = PyUnicode_Decode(s, x, self->encoding, self->errors);
    if (str == NULL)
        return -1;

    PDATA_PUSH(self->stack, str, -1);
    return 0;
}

static int
load_unicode(UnpicklerObject *self)
{
    PyObject *str;
    Py_ssize_t len;
    char *s;

    if ((len = _Unpickler_Readline(self, &s)) < 0)
        return -1;
    if (len < 1)
        return bad_readline();

    str = PyUnicode_DecodeRawUnicodeEscape(s, len - 1, NULL);
    if (str == NULL)
        return -1;

    PDATA_PUSH(self->stack, str, -1);
    return 0;
}

static int
load_binunicode(UnpicklerObject *self)
{
    PyObject *str;
    long size;
    char *s;

    if (_Unpickler_Read(self, &s, 4) < 0)
        return -1;

    size = calc_binint(s, 4);
    if (size < 0) {
        PyErr_SetString(UnpicklingError, 
                        "BINUNICODE pickle has negative byte count");
        return -1;
    }

    if (_Unpickler_Read(self, &s, size) < 0)
        return -1;

    str = PyUnicode_DecodeUTF8(s, size, "surrogatepass");
    if (str == NULL)
        return -1;

    PDATA_PUSH(self->stack, str, -1);
    return 0;
}

static int
load_tuple(UnpicklerObject *self)
{
    PyObject *tuple;
    int i;

    if ((i = marker(self)) < 0)
        return -1;

    tuple = Pdata_poptuple(self->stack, i);
    if (tuple == NULL)
        return -1;
    PDATA_PUSH(self->stack, tuple, -1);
    return 0;
}

static int
load_counted_tuple(UnpicklerObject *self, int len)
{
    PyObject *tuple;

    tuple = PyTuple_New(len);
    if (tuple == NULL)
        return -1;

    while (--len >= 0) {
        PyObject *item;

        PDATA_POP(self->stack, item);
        if (item == NULL)
            return -1;
        PyTuple_SET_ITEM(tuple, len, item);
    }
    PDATA_PUSH(self->stack, tuple, -1);
    return 0;
}

static int
load_empty_list(UnpicklerObject *self)
{
    PyObject *list;

    if ((list = PyList_New(0)) == NULL)
        return -1;
    PDATA_PUSH(self->stack, list, -1);
    return 0;
}

static int
load_empty_dict(UnpicklerObject *self)
{
    PyObject *dict;

    if ((dict = PyDict_New()) == NULL)
        return -1;
    PDATA_PUSH(self->stack, dict, -1);
    return 0;
}

static int
load_list(UnpicklerObject *self)
{
    PyObject *list;
    int i;

    if ((i = marker(self)) < 0)
        return -1;

    list = Pdata_poplist(self->stack, i);
    if (list == NULL)
        return -1;
    PDATA_PUSH(self->stack, list, -1);
    return 0;
}

static int
load_dict(UnpicklerObject *self)
{
    PyObject *dict, *key, *value;
    int i, j, k;

    if ((i = marker(self)) < 0)
        return -1;
    j = Py_SIZE(self->stack);

    if ((dict = PyDict_New()) == NULL)
        return -1;

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

static PyObject *
instantiate(PyObject *cls, PyObject *args)
{
    PyObject *result = NULL;
    /* Caller must assure args are a tuple.  Normally, args come from
       Pdata_poptuple which packs objects from the top of the stack
       into a newly created tuple. */
    assert(PyTuple_Check(args));
    if (Py_SIZE(args) > 0 || !PyType_Check(cls) ||
        PyObject_HasAttrString(cls, "__getinitargs__")) {
        result = PyObject_CallObject(cls, args);
    }
    else {
        result = PyObject_CallMethod(cls, "__new__", "O", cls);
    }
    return result;
}

static int
load_obj(UnpicklerObject *self)
{
    PyObject *cls, *args, *obj = NULL;
    int i;

    if ((i = marker(self)) < 0)
        return -1;

    args = Pdata_poptuple(self->stack, i + 1);
    if (args == NULL)
        return -1;

    PDATA_POP(self->stack, cls);
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
load_inst(UnpicklerObject *self)
{
    PyObject *cls = NULL;
    PyObject *args = NULL;
    PyObject *obj = NULL;
    PyObject *module_name;
    PyObject *class_name;
    Py_ssize_t len;
    int i;
    char *s;

    if ((i = marker(self)) < 0)
        return -1;
    if ((len = _Unpickler_Readline(self, &s)) < 0)
        return -1;
    if (len < 2)
        return bad_readline();

    /* Here it is safe to use PyUnicode_DecodeASCII(), even though non-ASCII
       identifiers are permitted in Python 3.0, since the INST opcode is only
       supported by older protocols on Python 2.x. */
    module_name = PyUnicode_DecodeASCII(s, len - 1, "strict");
    if (module_name == NULL)
        return -1;

    if ((len = _Unpickler_Readline(self, &s)) >= 0) {
        if (len < 2)
            return bad_readline();
        class_name = PyUnicode_DecodeASCII(s, len - 1, "strict");
        if (class_name != NULL) {
            cls = find_class(self, module_name, class_name);
            Py_DECREF(class_name);
        }
    }
    Py_DECREF(module_name);

    if (cls == NULL)
        return -1;

    if ((args = Pdata_poptuple(self->stack, i)) != NULL) {
        obj = instantiate(cls, args);
        Py_DECREF(args);
    }
    Py_DECREF(cls);

    if (obj == NULL)
        return -1;

    PDATA_PUSH(self->stack, obj, -1);
    return 0;
}

static int
load_newobj(UnpicklerObject *self)
{
    PyObject *args = NULL;
    PyObject *clsraw = NULL;
    PyTypeObject *cls;          /* clsraw cast to its true type */
    PyObject *obj;

    /* Stack is ... cls argtuple, and we want to call
     * cls.__new__(cls, *argtuple).
     */
    PDATA_POP(self->stack, args);
    if (args == NULL)
        goto error;
    if (!PyTuple_Check(args)) {
        PyErr_SetString(UnpicklingError, "NEWOBJ expected an arg " "tuple.");
        goto error;
    }

    PDATA_POP(self->stack, clsraw);
    cls = (PyTypeObject *)clsraw;
    if (cls == NULL)
        goto error;
    if (!PyType_Check(cls)) {
        PyErr_SetString(UnpicklingError, "NEWOBJ class argument "
                        "isn't a type object");
        goto error;
    }
    if (cls->tp_new == NULL) {
        PyErr_SetString(UnpicklingError, "NEWOBJ class argument "
                        "has NULL tp_new");
        goto error;
    }

    /* Call __new__. */
    obj = cls->tp_new(cls, args, NULL);
    if (obj == NULL)
        goto error;

    Py_DECREF(args);
    Py_DECREF(clsraw);
    PDATA_PUSH(self->stack, obj, -1);
    return 0;

  error:
    Py_XDECREF(args);
    Py_XDECREF(clsraw);
    return -1;
}

static int
load_global(UnpicklerObject *self)
{
    PyObject *global = NULL;
    PyObject *module_name;
    PyObject *global_name;
    Py_ssize_t len;
    char *s;

    if ((len = _Unpickler_Readline(self, &s)) < 0)
        return -1;
    if (len < 2)
        return bad_readline();
    module_name = PyUnicode_DecodeUTF8(s, len - 1, "strict");
    if (!module_name)
        return -1;

    if ((len = _Unpickler_Readline(self, &s)) >= 0) {
        if (len < 2) {
            Py_DECREF(module_name);
            return bad_readline();
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
load_persid(UnpicklerObject *self)
{
    PyObject *pid;
    Py_ssize_t len;
    char *s;

    if (self->pers_func) {
        if ((len = _Unpickler_Readline(self, &s)) < 0)
            return -1;
        if (len < 2)
            return bad_readline();

        pid = PyBytes_FromStringAndSize(s, len - 1);
        if (pid == NULL)
            return -1;

        /* Ugh... this does not leak since _Unpickler_FastCall() steals the
           reference to pid first. */
        pid = _Unpickler_FastCall(self, self->pers_func, pid);
        if (pid == NULL)
            return -1;

        PDATA_PUSH(self->stack, pid, -1);
        return 0;
    }
    else {
        PyErr_SetString(UnpicklingError,
                        "A load persistent id instruction was encountered,\n"
                        "but no persistent_load function was specified.");
        return -1;
    }
}

static int
load_binpersid(UnpicklerObject *self)
{
    PyObject *pid;

    if (self->pers_func) {
        PDATA_POP(self->stack, pid);
        if (pid == NULL)
            return -1;

        /* Ugh... this does not leak since _Unpickler_FastCall() steals the
           reference to pid first. */
        pid = _Unpickler_FastCall(self, self->pers_func, pid);
        if (pid == NULL)
            return -1;

        PDATA_PUSH(self->stack, pid, -1);
        return 0;
    }
    else {
        PyErr_SetString(UnpicklingError,
                        "A load persistent id instruction was encountered,\n"
                        "but no persistent_load function was specified.");
        return -1;
    }
}

static int
load_pop(UnpicklerObject *self)
{
    int len = Py_SIZE(self->stack);

    /* Note that we split the (pickle.py) stack into two stacks,
     * an object stack and a mark stack. We have to be clever and
     * pop the right one. We do this by looking at the top of the
     * mark stack first, and only signalling a stack underflow if
     * the object stack is empty and the mark stack doesn't match
     * our expectations.
     */
    if (self->num_marks > 0 && self->marks[self->num_marks - 1] == len) {
        self->num_marks--;
    } else if (len > 0) {
        len--;
        Py_DECREF(self->stack->data[len]);
        Py_SIZE(self->stack) = len;
    } else {
        return stack_underflow();
    }
    return 0;
}

static int
load_pop_mark(UnpicklerObject *self)
{
    int i;

    if ((i = marker(self)) < 0)
        return -1;

    Pdata_clear(self->stack, i);

    return 0;
}

static int
load_dup(UnpicklerObject *self)
{
    PyObject *last;
    int len;

    if ((len = Py_SIZE(self->stack)) <= 0)
        return stack_underflow();
    last = self->stack->data[len - 1];
    PDATA_APPEND(self->stack, last, -1);
    return 0;
}

static int
load_get(UnpicklerObject *self)
{
    PyObject *key, *value;
    Py_ssize_t idx;
    Py_ssize_t len;
    char *s;

    if ((len = _Unpickler_Readline(self, &s)) < 0)
        return -1;
    if (len < 2)
        return bad_readline();

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
        if (!PyErr_Occurred())
            PyErr_SetObject(PyExc_KeyError, key);
        Py_DECREF(key);
        return -1;
    }
    Py_DECREF(key);

    PDATA_APPEND(self->stack, value, -1);
    return 0;
}

static int
load_binget(UnpicklerObject *self)
{
    PyObject *value;
    Py_ssize_t idx;
    char *s;

    if (_Unpickler_Read(self, &s, 1) < 0)
        return -1;

    idx = Py_CHARMASK(s[0]);

    value = _Unpickler_MemoGet(self, idx);
    if (value == NULL) {
        PyObject *key = PyLong_FromSsize_t(idx);
        if (!PyErr_Occurred())
            PyErr_SetObject(PyExc_KeyError, key);
        Py_DECREF(key);
        return -1;
    }

    PDATA_APPEND(self->stack, value, -1);
    return 0;
}

static int
load_long_binget(UnpicklerObject *self)
{
    PyObject *value;
    Py_ssize_t idx;
    char *s;

    if (_Unpickler_Read(self, &s, 4) < 0)
        return -1;

    idx =  (long)Py_CHARMASK(s[0]);
    idx |= (long)Py_CHARMASK(s[1]) << 8;
    idx |= (long)Py_CHARMASK(s[2]) << 16;
    idx |= (long)Py_CHARMASK(s[3]) << 24;

    value = _Unpickler_MemoGet(self, idx);
    if (value == NULL) {
        PyObject *key = PyLong_FromSsize_t(idx);
        if (!PyErr_Occurred())
            PyErr_SetObject(PyExc_KeyError, key);
        Py_DECREF(key);
        return -1;
    }

    PDATA_APPEND(self->stack, value, -1);
    return 0;
}

/* Push an object from the extension registry (EXT[124]).  nbytes is
 * the number of bytes following the opcode, holding the index (code) value.
 */
static int
load_extension(UnpicklerObject *self, int nbytes)
{
    char *codebytes;            /* the nbytes bytes after the opcode */
    long code;                  /* calc_binint returns long */
    PyObject *py_code;          /* code as a Python int */
    PyObject *obj;              /* the object to push */
    PyObject *pair;             /* (module_name, class_name) */
    PyObject *module_name, *class_name;

    assert(nbytes == 1 || nbytes == 2 || nbytes == 4);
    if (_Unpickler_Read(self, &codebytes, nbytes) < 0)
        return -1;
    code = calc_binint(codebytes, nbytes);
    if (code <= 0) {            /* note that 0 is forbidden */
        /* Corrupt or hostile pickle. */
        PyErr_SetString(UnpicklingError, "EXT specifies code <= 0");
        return -1;
    }

    /* Look for the code in the cache. */
    py_code = PyLong_FromLong(code);
    if (py_code == NULL)
        return -1;
    obj = PyDict_GetItem(extension_cache, py_code);
    if (obj != NULL) {
        /* Bingo. */
        Py_DECREF(py_code);
        PDATA_APPEND(self->stack, obj, -1);
        return 0;
    }

    /* Look up the (module_name, class_name) pair. */
    pair = PyDict_GetItem(inverted_registry, py_code);
    if (pair == NULL) {
        Py_DECREF(py_code);
        PyErr_Format(PyExc_ValueError, "unregistered extension "
                     "code %ld", code);
        return -1;
    }
    /* Since the extension registry is manipulable via Python code,
     * confirm that pair is really a 2-tuple of strings.
     */
    if (!PyTuple_Check(pair) || PyTuple_Size(pair) != 2 ||
        !PyUnicode_Check(module_name = PyTuple_GET_ITEM(pair, 0)) ||
        !PyUnicode_Check(class_name = PyTuple_GET_ITEM(pair, 1))) {
        Py_DECREF(py_code);
        PyErr_Format(PyExc_ValueError, "_inverted_registry[%ld] "
                     "isn't a 2-tuple of strings", code);
        return -1;
    }
    /* Load the object. */
    obj = find_class(self, module_name, class_name);
    if (obj == NULL) {
        Py_DECREF(py_code);
        return -1;
    }
    /* Cache code -> obj. */
    code = PyDict_SetItem(extension_cache, py_code, obj);
    Py_DECREF(py_code);
    if (code < 0) {
        Py_DECREF(obj);
        return -1;
    }
    PDATA_PUSH(self->stack, obj, -1);
    return 0;
}

static int
load_put(UnpicklerObject *self)
{
    PyObject *key, *value;
    Py_ssize_t idx;
    Py_ssize_t len;
    char *s;

    if ((len = _Unpickler_Readline(self, &s)) < 0)
        return -1;
    if (len < 2)
        return bad_readline();
    if (Py_SIZE(self->stack) <= 0)
        return stack_underflow();
    value = self->stack->data[Py_SIZE(self->stack) - 1];

    key = PyLong_FromString(s, NULL, 10);
    if (key == NULL)
        return -1;
    idx = PyLong_AsSsize_t(key);
    Py_DECREF(key);
    if (idx == -1 && PyErr_Occurred())
        return -1;

    return _Unpickler_MemoPut(self, idx, value);
}

static int
load_binput(UnpicklerObject *self)
{
    PyObject *value;
    Py_ssize_t idx;
    char *s;

    if (_Unpickler_Read(self, &s, 1) < 0)
        return -1;

    if (Py_SIZE(self->stack) <= 0)
        return stack_underflow();
    value = self->stack->data[Py_SIZE(self->stack) - 1];

    idx = Py_CHARMASK(s[0]);

    return _Unpickler_MemoPut(self, idx, value);
}

static int
load_long_binput(UnpicklerObject *self)
{
    PyObject *value;
    Py_ssize_t idx;
    char *s;

    if (_Unpickler_Read(self, &s, 4) < 0)
        return -1;

    if (Py_SIZE(self->stack) <= 0)
        return stack_underflow();
    value = self->stack->data[Py_SIZE(self->stack) - 1];

    idx =  (long)Py_CHARMASK(s[0]);
    idx |= (long)Py_CHARMASK(s[1]) << 8;
    idx |= (long)Py_CHARMASK(s[2]) << 16;
    idx |= (long)Py_CHARMASK(s[3]) << 24;

    return _Unpickler_MemoPut(self, idx, value);
}

static int
do_append(UnpicklerObject *self, int x)
{
    PyObject *value;
    PyObject *list;
    int len, i;

    len = Py_SIZE(self->stack);
    if (x > len || x <= 0)
        return stack_underflow();
    if (len == x)  /* nothing to do */
        return 0;

    list = self->stack->data[x - 1];

    if (PyList_Check(list)) {
        PyObject *slice;
        Py_ssize_t list_len;

        slice = Pdata_poplist(self->stack, x);
        if (!slice)
            return -1;
        list_len = PyList_GET_SIZE(list);
        i = PyList_SetSlice(list, list_len, list_len, slice);
        Py_DECREF(slice);
        return i;
    }
    else {
        PyObject *append_func;

        append_func = PyObject_GetAttrString(list, "append");
        if (append_func == NULL)
            return -1;
        for (i = x; i < len; i++) {
            PyObject *result;

            value = self->stack->data[i];
            result = _Unpickler_FastCall(self, append_func, value);
            if (result == NULL) {
                Pdata_clear(self->stack, i + 1);
                Py_SIZE(self->stack) = x;
                return -1;
            }
            Py_DECREF(result);
        }
        Py_SIZE(self->stack) = x;
    }

    return 0;
}

static int
load_append(UnpicklerObject *self)
{
    return do_append(self, Py_SIZE(self->stack) - 1);
}

static int
load_appends(UnpicklerObject *self)
{
    return do_append(self, marker(self));
}

static int
do_setitems(UnpicklerObject *self, int x)
{
    PyObject *value, *key;
    PyObject *dict;
    int len, i;
    int status = 0;

    len = Py_SIZE(self->stack);
    if (x > len || x <= 0)
        return stack_underflow();
    if (len == x)  /* nothing to do */
        return 0;
    if ((len - x) % 2 != 0) { 
        /* Currupt or hostile pickle -- we never write one like this. */
        PyErr_SetString(UnpicklingError, "odd number of items for SETITEMS");
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
load_setitem(UnpicklerObject *self)
{
    return do_setitems(self, Py_SIZE(self->stack) - 2);
}

static int
load_setitems(UnpicklerObject *self)
{
    return do_setitems(self, marker(self));
}

static int
load_build(UnpicklerObject *self)
{
    PyObject *state, *inst, *slotstate;
    PyObject *setstate;
    int status = 0;

    /* Stack is ... instance, state.  We want to leave instance at
     * the stack top, possibly mutated via instance.__setstate__(state).
     */
    if (Py_SIZE(self->stack) < 2)
        return stack_underflow();

    PDATA_POP(self->stack, state);
    if (state == NULL)
        return -1;

    inst = self->stack->data[Py_SIZE(self->stack) - 1];

    setstate = PyObject_GetAttrString(inst, "__setstate__");
    if (setstate == NULL) {
        if (PyErr_ExceptionMatches(PyExc_AttributeError))
            PyErr_Clear();
        else {
            Py_DECREF(state);
            return -1;
        }
    }
    else {
        PyObject *result;

        /* The explicit __setstate__ is responsible for everything. */
        /* Ugh... this does not leak since _Unpickler_FastCall() steals the
           reference to state first. */
        result = _Unpickler_FastCall(self, setstate, state);
        Py_DECREF(setstate);
        if (result == NULL)
            return -1;
        Py_DECREF(result);
        return 0;
    }

    /* A default __setstate__.  First see whether state embeds a
     * slot state dict too (a proto 2 addition).
     */
    if (PyTuple_Check(state) && Py_SIZE(state) == 2) {
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
            PyErr_SetString(UnpicklingError, "state is not a dictionary");
            goto error;
        }
        dict = PyObject_GetAttrString(inst, "__dict__");
        if (dict == NULL)
            goto error;

        i = 0;
        while (PyDict_Next(state, &i, &d_key, &d_value)) {
            /* normally the keys for instance attributes are
               interned.  we should try to do that here. */
            Py_INCREF(d_key);
            if (PyUnicode_CheckExact(d_key))
                PyUnicode_InternInPlace(&d_key);
            if (PyObject_SetItem(dict, d_key, d_value) < 0) {
                Py_DECREF(d_key);
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
            PyErr_SetString(UnpicklingError,
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
load_mark(UnpicklerObject *self)
{

    /* Note that we split the (pickle.py) stack into two stacks, an
     * object stack and a mark stack. Here we push a mark onto the
     * mark stack.
     */

    if ((self->num_marks + 1) >= self->marks_size) {
        size_t alloc;
        int *marks;

        /* Use the size_t type to check for overflow. */
        alloc = ((size_t)self->num_marks << 1) + 20;
        if (alloc > PY_SSIZE_T_MAX || 
            alloc <= ((size_t)self->num_marks + 1)) {
            PyErr_NoMemory();
            return -1;
        }

        if (self->marks == NULL)
            marks = (int *)PyMem_Malloc(alloc * sizeof(int));
        else
            marks = (int *)PyMem_Realloc(self->marks, alloc * sizeof(int));
        if (marks == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        self->marks = marks;
        self->marks_size = (Py_ssize_t)alloc;
    }

    self->marks[self->num_marks++] = Py_SIZE(self->stack);

    return 0;
}

static int
load_reduce(UnpicklerObject *self)
{
    PyObject *callable = NULL;
    PyObject *argtup = NULL;
    PyObject *obj = NULL;

    PDATA_POP(self->stack, argtup);
    if (argtup == NULL)
        return -1;
    PDATA_POP(self->stack, callable);
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
load_proto(UnpicklerObject *self)
{
    char *s;
    int i;

    if (_Unpickler_Read(self, &s, 1) < 0)
        return -1;

    i = (unsigned char)s[0];
    if (i <= HIGHEST_PROTOCOL) {
        self->proto = i;
        return 0;
    }

    PyErr_Format(PyExc_ValueError, "unsupported pickle protocol: %d", i);
    return -1;
}

static PyObject *
load(UnpicklerObject *self)
{
    PyObject *err;
    PyObject *value = NULL;
    char *s;

    self->num_marks = 0;
    if (Py_SIZE(self->stack))
        Pdata_clear(self->stack, 0);

    /* Convenient macros for the dispatch while-switch loop just below. */
#define OP(opcode, load_func) \
    case opcode: if (load_func(self) < 0) break; continue;

#define OP_ARG(opcode, load_func, arg) \
    case opcode: if (load_func(self, (arg)) < 0) break; continue;

    while (1) {
        if (_Unpickler_Read(self, &s, 1) < 0)
            break;

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
        OP(BINBYTES, load_binbytes)
        OP(SHORT_BINBYTES, load_short_binbytes)
        OP(BINSTRING, load_binstring)
        OP(SHORT_BINSTRING, load_short_binstring)
        OP(STRING, load_string)
        OP(UNICODE, load_unicode)
        OP(BINUNICODE, load_binunicode)
        OP_ARG(EMPTY_TUPLE, load_counted_tuple, 0)
        OP_ARG(TUPLE1, load_counted_tuple, 1)
        OP_ARG(TUPLE2, load_counted_tuple, 2)
        OP_ARG(TUPLE3, load_counted_tuple, 3)
        OP(TUPLE, load_tuple)
        OP(EMPTY_LIST, load_empty_list)
        OP(LIST, load_list)
        OP(EMPTY_DICT, load_empty_dict)
        OP(DICT, load_dict)
        OP(OBJ, load_obj)
        OP(INST, load_inst)
        OP(NEWOBJ, load_newobj)
        OP(GLOBAL, load_global)
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
        OP(POP, load_pop)
        OP(POP_MARK, load_pop_mark)
        OP(SETITEM, load_setitem)
        OP(SETITEMS, load_setitems)
        OP(PERSID, load_persid)
        OP(BINPERSID, load_binpersid)
        OP(REDUCE, load_reduce)
        OP(PROTO, load_proto)
        OP_ARG(EXT1, load_extension, 1)
        OP_ARG(EXT2, load_extension, 2)
        OP_ARG(EXT4, load_extension, 4)
        OP_ARG(NEWTRUE, load_bool, Py_True)
        OP_ARG(NEWFALSE, load_bool, Py_False)

        case STOP:
            break;

        case '\0':
            PyErr_SetNone(PyExc_EOFError);
            return NULL;

        default:
            PyErr_Format(UnpicklingError,
                         "invalid load key, '%c'.", s[0]);
            return NULL;
        }

        break;                  /* and we are done! */
    }

    if (_Unpickler_SkipConsumed(self) < 0)
        return NULL;

    /* XXX: It is not clear what this is actually for. */
    if ((err = PyErr_Occurred())) {
        if (err == PyExc_EOFError) {
            PyErr_SetNone(PyExc_EOFError);
        }
        return NULL;
    }

    PDATA_POP(self->stack, value);
    return value;
}

PyDoc_STRVAR(Unpickler_load_doc,
"load() -> object. Load a pickle."
"\n"
"Read a pickled object representation from the open file object given in\n"
"the constructor, and return the reconstituted object hierarchy specified\n"
"therein.\n");

static PyObject *
Unpickler_load(UnpicklerObject *self)
{
    /* Check whether the Unpickler was initialized correctly. This prevents
       segfaulting if a subclass overridden __init__ with a function that does
       not call Unpickler.__init__(). Here, we simply ensure that self->read
       is not NULL. */
    if (self->read == NULL) {
        PyErr_Format(UnpicklingError, 
                     "Unpickler.__init__() was not called by %s.__init__()",
                     Py_TYPE(self)->tp_name);
        return NULL;
    }

    return load(self);
}

/* The name of find_class() is misleading. In newer pickle protocols, this
   function is used for loading any global (i.e., functions), not just
   classes. The name is kept only for backward compatibility. */

PyDoc_STRVAR(Unpickler_find_class_doc,
"find_class(module_name, global_name) -> object.\n"
"\n"
"Return an object from a specified module, importing the module if\n"
"necessary.  Subclasses may override this method (e.g. to restrict\n"
"unpickling of arbitrary classes and functions).\n"
"\n"
"This method is called whenever a class or a function object is\n"
"needed.  Both arguments passed are str objects.\n");

static PyObject *
Unpickler_find_class(UnpicklerObject *self, PyObject *args)
{
    PyObject *global;
    PyObject *modules_dict;
    PyObject *module;
    PyObject *module_name, *global_name;

    if (!PyArg_UnpackTuple(args, "find_class", 2, 2,
                           &module_name, &global_name))
        return NULL;

    /* Try to map the old names used in Python 2.x to the new ones used in
       Python 3.x.  We do this only with old pickle protocols and when the
       user has not disabled the feature. */
    if (self->proto < 3 && self->fix_imports) {
        PyObject *key;
        PyObject *item;

        /* Check if the global (i.e., a function or a class) was renamed
           or moved to another module. */
        key = PyTuple_Pack(2, module_name, global_name);
        if (key == NULL)
            return NULL;
        item = PyDict_GetItemWithError(name_mapping_2to3, key);
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

        /* Check if the module was renamed. */
        item = PyDict_GetItemWithError(import_mapping_2to3, module_name);
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

    modules_dict = PySys_GetObject("modules");
    if (modules_dict == NULL)
        return NULL;

    module = PyDict_GetItemWithError(modules_dict, module_name);
    if (module == NULL) {
        if (PyErr_Occurred())
            return NULL;
        module = PyImport_Import(module_name);
        if (module == NULL)
            return NULL;
        global = PyObject_GetAttr(module, global_name);
        Py_DECREF(module);
    }
    else { 
        global = PyObject_GetAttr(module, global_name);
    }
    return global;
}

static struct PyMethodDef Unpickler_methods[] = {
    {"load", (PyCFunction)Unpickler_load, METH_NOARGS,
     Unpickler_load_doc},
    {"find_class", (PyCFunction)Unpickler_find_class, METH_VARARGS,
     Unpickler_find_class_doc},
    {NULL, NULL}                /* sentinel */
};

static void
Unpickler_dealloc(UnpicklerObject *self)
{
    PyObject_GC_UnTrack((PyObject *)self);
    Py_XDECREF(self->readline);
    Py_XDECREF(self->read);
    Py_XDECREF(self->peek);
    Py_XDECREF(self->stack);
    Py_XDECREF(self->pers_func);
    Py_XDECREF(self->arg);
    if (self->buffer.buf != NULL) {
        PyBuffer_Release(&self->buffer);
        self->buffer.buf = NULL;
    }

    _Unpickler_MemoCleanup(self);
    PyMem_Free(self->marks);
    PyMem_Free(self->input_line);
    free(self->encoding);
    free(self->errors);

    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int
Unpickler_traverse(UnpicklerObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->readline);
    Py_VISIT(self->read);
    Py_VISIT(self->peek);
    Py_VISIT(self->stack);
    Py_VISIT(self->pers_func);
    Py_VISIT(self->arg);
    return 0;
}

static int
Unpickler_clear(UnpicklerObject *self)
{
    Py_CLEAR(self->readline);
    Py_CLEAR(self->read);
    Py_CLEAR(self->peek);
    Py_CLEAR(self->stack);
    Py_CLEAR(self->pers_func);
    Py_CLEAR(self->arg);
    if (self->buffer.buf != NULL) {
        PyBuffer_Release(&self->buffer);
        self->buffer.buf = NULL;
    }

    _Unpickler_MemoCleanup(self);
    PyMem_Free(self->marks);
    self->marks = NULL;
    PyMem_Free(self->input_line);
    self->input_line = NULL;
    free(self->encoding);
    self->encoding = NULL;
    free(self->errors);
    self->errors = NULL;

    return 0;
}

PyDoc_STRVAR(Unpickler_doc,
"Unpickler(file, *, encoding='ASCII', errors='strict')"
"\n"
"This takes a binary file for reading a pickle data stream.\n"
"\n"
"The protocol version of the pickle is detected automatically, so no\n"
"proto argument is needed.\n"
"\n"
"The file-like object must have two methods, a read() method\n"
"that takes an integer argument, and a readline() method that\n"
"requires no arguments.  Both methods should return bytes.\n"
"Thus file-like object can be a binary file object opened for\n"
"reading, a BytesIO object, or any other custom object that\n"
"meets this interface.\n"
"\n"
"Optional keyword arguments are *fix_imports*, *encoding* and *errors*,\n"
"which are used to control compatiblity support for pickle stream\n"
"generated by Python 2.x.  If *fix_imports* is True, pickle will try to\n"
"map the old Python 2.x names to the new names used in Python 3.x.  The\n"
"*encoding* and *errors* tell pickle how to decode 8-bit string\n"
"instances pickled by Python 2.x; these default to 'ASCII' and\n"
"'strict', respectively.\n");

static int
Unpickler_init(UnpicklerObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"file", "fix_imports", "encoding", "errors", 0};
    PyObject *file;
    PyObject *fix_imports = Py_True;
    char *encoding = NULL;
    char *errors = NULL;

    /* XXX: That is an horrible error message. But, I don't know how to do
       better... */
    if (Py_SIZE(args) != 1) {
        PyErr_Format(PyExc_TypeError,
                     "%s takes exactly one positional argument (%zd given)",
                     Py_TYPE(self)->tp_name, Py_SIZE(args));
        return -1;
    }

    /* Arguments parsing needs to be done in the __init__() method to allow
       subclasses to define their own __init__() method, which may (or may
       not) support Unpickler arguments. However, this means we need to be
       extra careful in the other Unpickler methods, since a subclass could
       forget to call Unpickler.__init__() thus breaking our internal
       invariants. */
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|Oss:Unpickler", kwlist,
                                     &file, &fix_imports, &encoding, &errors))
        return -1;

    /* In case of multiple __init__() calls, clear previous content. */
    if (self->read != NULL)
        (void)Unpickler_clear(self);

    if (_Unpickler_SetInputStream(self, file) < 0)
        return -1;

    if (_Unpickler_SetInputEncoding(self, encoding, errors) < 0)
        return -1;

    self->fix_imports = PyObject_IsTrue(fix_imports);
    if (self->fix_imports == -1)
        return -1;

    if (PyObject_HasAttrString((PyObject *)self, "persistent_load")) {
        self->pers_func = PyObject_GetAttrString((PyObject *)self,
                                                 "persistent_load");
        if (self->pers_func == NULL)
            return -1;
    }
    else {
        self->pers_func = NULL;
    }

    self->stack = (Pdata *)Pdata_New();
    if (self->stack == NULL)
        return -1;

    self->memo_size = 32;
    self->memo = _Unpickler_NewMemo(self->memo_size);
    if (self->memo == NULL)
        return -1;

    self->arg = NULL;
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

typedef struct {
    PyObject_HEAD
    UnpicklerObject *unpickler;
} UnpicklerMemoProxyObject;

PyDoc_STRVAR(ump_clear_doc,
"memo.clear() -> None.  Remove all items from memo.");

static PyObject *
ump_clear(UnpicklerMemoProxyObject *self)
{
    _Unpickler_MemoCleanup(self->unpickler);
    self->unpickler->memo = _Unpickler_NewMemo(self->unpickler->memo_size);
    if (self->unpickler->memo == NULL)
        return NULL;
    Py_RETURN_NONE;
}

PyDoc_STRVAR(ump_copy_doc,
"memo.copy() -> new_memo.  Copy the memo to a new object.");

static PyObject *
ump_copy(UnpicklerMemoProxyObject *self)
{
    Py_ssize_t i;
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

PyDoc_STRVAR(ump_reduce_doc,
"memo.__reduce__(). Pickling support.");

static PyObject *
ump_reduce(UnpicklerMemoProxyObject *self, PyObject *args)
{
    PyObject *reduce_value;
    PyObject *constructor_args;
    PyObject *contents = ump_copy(self);
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
    Py_INCREF((PyObject *)&PyDict_Type);
    PyTuple_SET_ITEM(reduce_value, 0, (PyObject *)&PyDict_Type);
    PyTuple_SET_ITEM(reduce_value, 1, constructor_args);
    return reduce_value;
}

static PyMethodDef unpicklerproxy_methods[] = {
    {"clear",       (PyCFunction)ump_clear,  METH_NOARGS,  ump_clear_doc},
    {"copy",        (PyCFunction)ump_copy,   METH_NOARGS,  ump_copy_doc},
    {"__reduce__",  (PyCFunction)ump_reduce, METH_VARARGS, ump_reduce_doc},
    {NULL, NULL}    /* sentinel */
};

static void
UnpicklerMemoProxy_dealloc(UnpicklerMemoProxyObject *self)
{
    PyObject_GC_UnTrack(self);
    Py_XDECREF(self->unpickler);
    PyObject_GC_Del((PyObject *)self);
}

static int
UnpicklerMemoProxy_traverse(UnpicklerMemoProxyObject *self,
                            visitproc visit, void *arg)
{
    Py_VISIT(self->unpickler);
    return 0;
}

static int
UnpicklerMemoProxy_clear(UnpicklerMemoProxyObject *self)
{
    Py_CLEAR(self->unpickler);
    return 0;
}

static PyTypeObject UnpicklerMemoProxyType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pickle.UnpicklerMemoProxy",               /*tp_name*/
    sizeof(UnpicklerMemoProxyObject),           /*tp_basicsize*/
    0,
    (destructor)UnpicklerMemoProxy_dealloc,     /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_compare */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    PyObject_HashNotImplemented,                /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    PyObject_GenericSetAttr,                    /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    0,                                          /* tp_doc */
    (traverseproc)UnpicklerMemoProxy_traverse,  /* tp_traverse */
    (inquiry)UnpicklerMemoProxy_clear,          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    unpicklerproxy_methods,                     /* tp_methods */
};

static PyObject *
UnpicklerMemoProxy_New(UnpicklerObject *unpickler)
{
    UnpicklerMemoProxyObject *self;

    self = PyObject_GC_New(UnpicklerMemoProxyObject,
                           &UnpicklerMemoProxyType);
    if (self == NULL)
        return NULL;
    Py_INCREF(unpickler);
    self->unpickler = unpickler;
    PyObject_GC_Track(self);
    return (PyObject *)self;
}

/*****************************************************************************/


static PyObject *
Unpickler_get_memo(UnpicklerObject *self)
{
    return UnpicklerMemoProxy_New(self);
}

static int
Unpickler_set_memo(UnpicklerObject *self, PyObject *obj)
{
    PyObject **new_memo;
    Py_ssize_t new_memo_size = 0;
    Py_ssize_t i;

    if (obj == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "attribute deletion is not supported");
        return -1;
    }

    if (Py_TYPE(obj) == &UnpicklerMemoProxyType) {
        UnpicklerObject *unpickler =
            ((UnpicklerMemoProxyObject *)obj)->unpickler;

        new_memo_size = unpickler->memo_size;
        new_memo = _Unpickler_NewMemo(new_memo_size);
        if (new_memo == NULL)
            return -1;

        for (i = 0; i < new_memo_size; i++) {
            Py_XINCREF(unpickler->memo[i]);
            new_memo[i] = unpickler->memo[i];
        }
    }
    else if (PyDict_Check(obj)) {
        Py_ssize_t i = 0;
        PyObject *key, *value;

        new_memo_size = PyDict_Size(obj);
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
            if (_Unpickler_MemoPut(self, idx, value) < 0)
                goto error;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "'memo' attribute must be an UnpicklerMemoProxy object"
                     "or dict, not %.200s", Py_TYPE(obj)->tp_name);
        return -1;
    }

    _Unpickler_MemoCleanup(self);
    self->memo_size = new_memo_size;
    self->memo = new_memo;

    return 0;

  error:
    if (new_memo_size) {
        i = new_memo_size;
        while (--i >= 0) {
            Py_XDECREF(new_memo[i]);
        }
        PyMem_FREE(new_memo);
    }
    return -1;
}

static PyObject *
Unpickler_get_persload(UnpicklerObject *self)
{
    if (self->pers_func == NULL)
        PyErr_SetString(PyExc_AttributeError, "persistent_load");
    else
        Py_INCREF(self->pers_func);
    return self->pers_func;
}

static int
Unpickler_set_persload(UnpicklerObject *self, PyObject *value)
{
    PyObject *tmp;

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "attribute deletion is not supported");
        return -1;
    }
    if (!PyCallable_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "persistent_load must be a callable taking "
                        "one argument");
        return -1;
    }

    tmp = self->pers_func;
    Py_INCREF(value);
    self->pers_func = value;
    Py_XDECREF(tmp);      /* self->pers_func can be NULL, so be careful. */

    return 0;
}

static PyGetSetDef Unpickler_getsets[] = {
    {"memo", (getter)Unpickler_get_memo, (setter)Unpickler_set_memo},
    {"persistent_load", (getter)Unpickler_get_persload,
                        (setter)Unpickler_set_persload},
    {NULL}
};

static PyTypeObject Unpickler_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pickle.Unpickler",                /*tp_name*/
    sizeof(UnpicklerObject),            /*tp_basicsize*/
    0,                                  /*tp_itemsize*/
    (destructor)Unpickler_dealloc,      /*tp_dealloc*/
    0,                                  /*tp_print*/
    0,                                  /*tp_getattr*/
    0,                                  /*tp_setattr*/
    0,                                  /*tp_reserved*/
    0,                                  /*tp_repr*/
    0,                                  /*tp_as_number*/
    0,                                  /*tp_as_sequence*/
    0,                                  /*tp_as_mapping*/
    0,                                  /*tp_hash*/
    0,                                  /*tp_call*/
    0,                                  /*tp_str*/
    0,                                  /*tp_getattro*/
    0,                                  /*tp_setattro*/
    0,                                  /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    Unpickler_doc,                      /*tp_doc*/
    (traverseproc)Unpickler_traverse,   /*tp_traverse*/
    (inquiry)Unpickler_clear,           /*tp_clear*/
    0,                                  /*tp_richcompare*/
    0,                                  /*tp_weaklistoffset*/
    0,                                  /*tp_iter*/
    0,                                  /*tp_iternext*/
    Unpickler_methods,                  /*tp_methods*/
    0,                                  /*tp_members*/
    Unpickler_getsets,                  /*tp_getset*/
    0,                                  /*tp_base*/
    0,                                  /*tp_dict*/
    0,                                  /*tp_descr_get*/
    0,                                  /*tp_descr_set*/
    0,                                  /*tp_dictoffset*/
    (initproc)Unpickler_init,           /*tp_init*/
    PyType_GenericAlloc,                /*tp_alloc*/
    PyType_GenericNew,                  /*tp_new*/
    PyObject_GC_Del,                    /*tp_free*/
    0,                                  /*tp_is_gc*/
};

PyDoc_STRVAR(pickle_dump_doc,
"dump(obj, file, protocol=None, *, fix_imports=True) -> None\n"
"\n"
"Write a pickled representation of obj to the open file object file.  This\n"
"is equivalent to ``Pickler(file, protocol).dump(obj)``, but may be more\n"
"efficient.\n"
"\n"
"The optional protocol argument tells the pickler to use the given protocol;\n"
"supported protocols are 0, 1, 2, 3.  The default protocol is 3; a\n"
"backward-incompatible protocol designed for Python 3.0.\n"
"\n"
"Specifying a negative protocol version selects the highest protocol version\n"
"supported.  The higher the protocol used, the more recent the version of\n"
"Python needed to read the pickle produced.\n"
"\n"
"The file argument must have a write() method that accepts a single bytes\n"
"argument.  It can thus be a file object opened for binary writing, a\n"
"io.BytesIO instance, or any other custom object that meets this interface.\n"
"\n"
"If fix_imports is True and protocol is less than 3, pickle will try to\n"
"map the new Python 3.x names to the old module names used in Python 2.x,\n"
"so that the pickle data stream is readable with Python 2.x.\n");

static PyObject *
pickle_dump(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"obj", "file", "protocol", "fix_imports", 0};
    PyObject *obj;
    PyObject *file;
    PyObject *proto = NULL;
    PyObject *fix_imports = Py_True;
    PicklerObject *pickler;

    /* fix_imports is a keyword-only argument.  */
    if (Py_SIZE(args) > 3) {
        PyErr_Format(PyExc_TypeError,
                     "pickle.dump() takes at most 3 positional "
                     "argument (%zd given)", Py_SIZE(args));
        return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO|OO:dump", kwlist,
                                     &obj, &file, &proto, &fix_imports))
        return NULL;

    pickler = _Pickler_New();
    if (pickler == NULL)
        return NULL;

    if (_Pickler_SetProtocol(pickler, proto, fix_imports) < 0)
        goto error;

    if (_Pickler_SetOutputStream(pickler, file) < 0)
        goto error;

    if (dump(pickler, obj) < 0)
        goto error;

    if (_Pickler_FlushToFile(pickler) < 0)
        goto error;

    Py_DECREF(pickler);
    Py_RETURN_NONE;

  error:
    Py_XDECREF(pickler);
    return NULL;
}

PyDoc_STRVAR(pickle_dumps_doc,
"dumps(obj, protocol=None, *, fix_imports=True) -> bytes\n"
"\n"
"Return the pickled representation of the object as a bytes\n"
"object, instead of writing it to a file.\n"
"\n"
"The optional protocol argument tells the pickler to use the given protocol;\n"
"supported protocols are 0, 1, 2, 3.  The default protocol is 3; a\n"
"backward-incompatible protocol designed for Python 3.0.\n"
"\n"
"Specifying a negative protocol version selects the highest protocol version\n"
"supported.  The higher the protocol used, the more recent the version of\n"
"Python needed to read the pickle produced.\n"
"\n"
"If fix_imports is True and *protocol* is less than 3, pickle will try to\n"
"map the new Python 3.x names to the old module names used in Python 2.x,\n"
"so that the pickle data stream is readable with Python 2.x.\n");

static PyObject *
pickle_dumps(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"obj", "protocol", "fix_imports", 0};
    PyObject *obj;
    PyObject *proto = NULL;
    PyObject *result;
    PyObject *fix_imports = Py_True;
    PicklerObject *pickler;

    /* fix_imports is a keyword-only argument.  */
    if (Py_SIZE(args) > 2) {
        PyErr_Format(PyExc_TypeError,
                     "pickle.dumps() takes at most 2 positional "
                     "argument (%zd given)", Py_SIZE(args));
        return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|OO:dumps", kwlist,
                                     &obj, &proto, &fix_imports))
        return NULL;

    pickler = _Pickler_New();
    if (pickler == NULL)
        return NULL;

    if (_Pickler_SetProtocol(pickler, proto, fix_imports) < 0)
        goto error;

    if (dump(pickler, obj) < 0)
        goto error;

    result = _Pickler_GetString(pickler);
    Py_DECREF(pickler);
    return result;

  error:
    Py_XDECREF(pickler);
    return NULL;
}

PyDoc_STRVAR(pickle_load_doc,
"load(file, *, fix_imports=True, encoding='ASCII', errors='strict') -> object\n"
"\n"
"Read a pickled object representation from the open file object file and\n"
"return the reconstituted object hierarchy specified therein.  This is\n"
"equivalent to ``Unpickler(file).load()``, but may be more efficient.\n"
"\n"
"The protocol version of the pickle is detected automatically, so no protocol\n"
"argument is needed.  Bytes past the pickled object's representation are\n"
"ignored.\n"
"\n"
"The argument file must have two methods, a read() method that takes an\n"
"integer argument, and a readline() method that requires no arguments.  Both\n"
"methods should return bytes.  Thus *file* can be a binary file object opened\n"
"for reading, a BytesIO object, or any other custom object that meets this\n"
"interface.\n"
"\n"
"Optional keyword arguments are fix_imports, encoding and errors,\n"
"which are used to control compatiblity support for pickle stream generated\n"
"by Python 2.x.  If fix_imports is True, pickle will try to map the old\n"
"Python 2.x names to the new names used in Python 3.x.  The encoding and\n"
"errors tell pickle how to decode 8-bit string instances pickled by Python\n"
"2.x; these default to 'ASCII' and 'strict', respectively.\n");

static PyObject *
pickle_load(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"file", "fix_imports", "encoding", "errors", 0};
    PyObject *file;
    PyObject *fix_imports = Py_True;
    PyObject *result;
    char *encoding = NULL;
    char *errors = NULL;
    UnpicklerObject *unpickler;

    /* fix_imports, encoding and errors are a keyword-only argument.  */
    if (Py_SIZE(args) != 1) {
        PyErr_Format(PyExc_TypeError,
                     "pickle.load() takes exactly one positional "
                     "argument (%zd given)", Py_SIZE(args));
        return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|Oss:load", kwlist,
                                     &file, &fix_imports, &encoding, &errors))
        return NULL;

    unpickler = _Unpickler_New();
    if (unpickler == NULL)
        return NULL;

    if (_Unpickler_SetInputStream(unpickler, file) < 0)
        goto error;

    if (_Unpickler_SetInputEncoding(unpickler, encoding, errors) < 0)
        goto error;

    unpickler->fix_imports = PyObject_IsTrue(fix_imports);
    if (unpickler->fix_imports == -1)
        goto error;

    result = load(unpickler);
    Py_DECREF(unpickler);
    return result;

  error:
    Py_XDECREF(unpickler);
    return NULL;
}

PyDoc_STRVAR(pickle_loads_doc,
"loads(input, *, fix_imports=True, encoding='ASCII', errors='strict') -> object\n"
"\n"
"Read a pickled object hierarchy from a bytes object and return the\n"
"reconstituted object hierarchy specified therein\n"
"\n"
"The protocol version of the pickle is detected automatically, so no protocol\n"
"argument is needed.  Bytes past the pickled object's representation are\n"
"ignored.\n"
"\n"
"Optional keyword arguments are fix_imports, encoding and errors, which\n"
"are used to control compatiblity support for pickle stream generated\n"
"by Python 2.x.  If fix_imports is True, pickle will try to map the old\n"
"Python 2.x names to the new names used in Python 3.x.  The encoding and\n"
"errors tell pickle how to decode 8-bit string instances pickled by Python\n"
"2.x; these default to 'ASCII' and 'strict', respectively.\n");

static PyObject *
pickle_loads(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"input", "fix_imports", "encoding", "errors", 0};
    PyObject *input;
    PyObject *fix_imports = Py_True;
    PyObject *result;
    char *encoding = NULL;
    char *errors = NULL;
    UnpicklerObject *unpickler;

    /* fix_imports, encoding and errors are a keyword-only argument.  */
    if (Py_SIZE(args) != 1) {
        PyErr_Format(PyExc_TypeError,
                     "pickle.loads() takes exactly one positional "
                     "argument (%zd given)", Py_SIZE(args));
        return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|Oss:loads", kwlist,
                                     &input, &fix_imports, &encoding, &errors))
        return NULL;

    unpickler = _Unpickler_New();
    if (unpickler == NULL)
        return NULL;

    if (_Unpickler_SetStringInput(unpickler, input) < 0)
        goto error;

    if (_Unpickler_SetInputEncoding(unpickler, encoding, errors) < 0)
        goto error;

    unpickler->fix_imports = PyObject_IsTrue(fix_imports);
    if (unpickler->fix_imports == -1)
        goto error;

    result = load(unpickler);
    Py_DECREF(unpickler);
    return result;

  error:
    Py_XDECREF(unpickler);
    return NULL;
}


static struct PyMethodDef pickle_methods[] = {
    {"dump",  (PyCFunction)pickle_dump,  METH_VARARGS|METH_KEYWORDS,
     pickle_dump_doc},
    {"dumps", (PyCFunction)pickle_dumps, METH_VARARGS|METH_KEYWORDS,
     pickle_dumps_doc},
    {"load",  (PyCFunction)pickle_load,  METH_VARARGS|METH_KEYWORDS,
     pickle_load_doc},
    {"loads", (PyCFunction)pickle_loads, METH_VARARGS|METH_KEYWORDS,
     pickle_loads_doc},
    {NULL, NULL} /* sentinel */
};

static int
initmodule(void)
{
    PyObject *copyreg = NULL;
    PyObject *compat_pickle = NULL;

    /* XXX: We should ensure that the types of the dictionaries imported are
       exactly PyDict objects. Otherwise, it is possible to crash the pickle
       since we use the PyDict API directly to access these dictionaries. */

    copyreg = PyImport_ImportModule("copyreg");
    if (!copyreg)
        goto error;
    dispatch_table = PyObject_GetAttrString(copyreg, "dispatch_table");
    if (!dispatch_table)
        goto error;
    extension_registry = \
        PyObject_GetAttrString(copyreg, "_extension_registry");
    if (!extension_registry)
        goto error;
    inverted_registry = PyObject_GetAttrString(copyreg, "_inverted_registry");
    if (!inverted_registry)
        goto error;
    extension_cache = PyObject_GetAttrString(copyreg, "_extension_cache");
    if (!extension_cache)
        goto error;
    Py_CLEAR(copyreg);

    /* Load the 2.x -> 3.x stdlib module mapping tables */
    compat_pickle = PyImport_ImportModule("_compat_pickle");
    if (!compat_pickle)
        goto error;
    name_mapping_2to3 = PyObject_GetAttrString(compat_pickle, "NAME_MAPPING");
    if (!name_mapping_2to3)
        goto error;
    if (!PyDict_CheckExact(name_mapping_2to3)) {
        PyErr_Format(PyExc_RuntimeError,
                     "_compat_pickle.NAME_MAPPING should be a dict, not %.200s",
                     Py_TYPE(name_mapping_2to3)->tp_name);
        goto error;
    }
    import_mapping_2to3 = PyObject_GetAttrString(compat_pickle,
                                                 "IMPORT_MAPPING");
    if (!import_mapping_2to3)
        goto error;
    if (!PyDict_CheckExact(import_mapping_2to3)) {
        PyErr_Format(PyExc_RuntimeError,
                     "_compat_pickle.IMPORT_MAPPING should be a dict, "
                     "not %.200s", Py_TYPE(import_mapping_2to3)->tp_name);
        goto error;
    }
    /* ... and the 3.x -> 2.x mapping tables */
    name_mapping_3to2 = PyObject_GetAttrString(compat_pickle,
                                               "REVERSE_NAME_MAPPING");
    if (!name_mapping_3to2)
        goto error;
    if (!PyDict_CheckExact(name_mapping_3to2)) {
        PyErr_Format(PyExc_RuntimeError,
                     "_compat_pickle.REVERSE_NAME_MAPPING should be a dict, "
                     "not %.200s", Py_TYPE(name_mapping_3to2)->tp_name);
        goto error;
    }
    import_mapping_3to2 = PyObject_GetAttrString(compat_pickle,
                                                 "REVERSE_IMPORT_MAPPING");
    if (!import_mapping_3to2)
        goto error;
    if (!PyDict_CheckExact(import_mapping_3to2)) {
        PyErr_Format(PyExc_RuntimeError,
                     "_compat_pickle.REVERSE_IMPORT_MAPPING should be a dict, "
                     "not %.200s", Py_TYPE(import_mapping_3to2)->tp_name);
        goto error;
    }
    Py_CLEAR(compat_pickle);

    empty_tuple = PyTuple_New(0);
    if (empty_tuple == NULL)
        goto error;
    two_tuple = PyTuple_New(2);
    if (two_tuple == NULL)
        goto error;
    /* We use this temp container with no regard to refcounts, or to
     * keeping containees alive.  Exempt from GC, because we don't
     * want anything looking at two_tuple() by magic.
     */
    PyObject_GC_UnTrack(two_tuple);

    return 0;

  error:
    Py_CLEAR(copyreg);
    Py_CLEAR(dispatch_table);
    Py_CLEAR(extension_registry);
    Py_CLEAR(inverted_registry);
    Py_CLEAR(extension_cache);
    Py_CLEAR(compat_pickle);
    Py_CLEAR(name_mapping_2to3);
    Py_CLEAR(import_mapping_2to3);
    Py_CLEAR(name_mapping_3to2);
    Py_CLEAR(import_mapping_3to2);
    Py_CLEAR(empty_tuple);
    Py_CLEAR(two_tuple);
    return -1;
}

static struct PyModuleDef _picklemodule = {
    PyModuleDef_HEAD_INIT,
    "_pickle",
    pickle_module_doc,
    -1,
    pickle_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__pickle(void)
{
    PyObject *m;

    if (PyType_Ready(&Unpickler_Type) < 0)
        return NULL;
    if (PyType_Ready(&Pickler_Type) < 0)
        return NULL;
    if (PyType_Ready(&Pdata_Type) < 0)
        return NULL;
    if (PyType_Ready(&PicklerMemoProxyType) < 0)
        return NULL;
    if (PyType_Ready(&UnpicklerMemoProxyType) < 0)
        return NULL;

    /* Create the module and add the functions. */
    m = PyModule_Create(&_picklemodule);
    if (m == NULL)
        return NULL;

    if (PyModule_AddObject(m, "Pickler", (PyObject *)&Pickler_Type) < 0)
        return NULL;
    if (PyModule_AddObject(m, "Unpickler", (PyObject *)&Unpickler_Type) < 0)
        return NULL;

    /* Initialize the exceptions. */
    PickleError = PyErr_NewException("_pickle.PickleError", NULL, NULL);
    if (PickleError == NULL)
        return NULL;
    PicklingError = \
        PyErr_NewException("_pickle.PicklingError", PickleError, NULL);
    if (PicklingError == NULL)
        return NULL;
    UnpicklingError = \
        PyErr_NewException("_pickle.UnpicklingError", PickleError, NULL);
    if (UnpicklingError == NULL)
        return NULL;

    if (PyModule_AddObject(m, "PickleError", PickleError) < 0)
        return NULL;
    if (PyModule_AddObject(m, "PicklingError", PicklingError) < 0)
        return NULL;
    if (PyModule_AddObject(m, "UnpicklingError", UnpicklingError) < 0)
        return NULL;

    if (initmodule() < 0)
        return NULL;

    return m;
}
