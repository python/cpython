#ifndef Py_CPYTHON_CODE_H
#  error "this header file must not be included directly"
#endif

/* Each instruction in a code object is a fixed-width value,
 * currently 2 bytes: 1-byte opcode + 1-byte oparg.  The EXTENDED_ARG
 * opcode allows for larger values but the current limit is 3 uses
 * of EXTENDED_ARG (see Python/wordcode_helpers.h), for a maximum
 * 32-bit value.  This aligns with the note in Python/compile.c
 * (compiler_addop_i_line) indicating that the max oparg value is
 * 2**32 - 1, rather than INT_MAX.
 */

typedef uint16_t _Py_CODEUNIT;

#ifdef WORDS_BIGENDIAN
#  define _Py_OPCODE(word) ((word) >> 8)
#  define _Py_OPARG(word) ((word) & 255)
#  define _Py_MAKECODEUNIT(opcode, oparg) (((opcode)<<8)|(oparg))
#else
#  define _Py_OPCODE(word) ((word) & 255)
#  define _Py_OPARG(word) ((word) >> 8)
#  define _Py_MAKECODEUNIT(opcode, oparg) ((opcode)|((oparg)<<8))
#endif

typedef struct _PyOpcache _PyOpcache;


/* Bytecode object */
struct PyCodeObject {
    PyObject_HEAD

    /* Note only the following fields are used in hash and/or comparisons
     *
     * - co_name
     * - co_argcount
     * - co_posonlyargcount
     * - co_kwonlyargcount
     * - co_nlocals
     * - co_stacksize
     * - co_flags
     * - co_firstlineno
     * - co_code
     * - co_consts
     * - co_names
     * - co_varnames
     * - co_freevars
     * - co_cellvars
     *
     * This is done to preserve the name and line number for tracebacks
     * and debuggers; otherwise, constant de-duplication would collapse
     * identical functions/lambdas defined on different lines.
     */

    /* These fields are set with provided values on new code objects. */

    // The hottest fields (in the eval loop) are grouped here at the top.
    PyObject *co_consts;        /* list (constants used) */
    PyObject *co_names;         /* list of strings (names used) */
    _Py_CODEUNIT *co_firstinstr; /* Pointer to first instruction, used for quickening.
                                    Unlike the other "hot" fields, this one is
                                    actually derived from co_code. */
    PyObject *co_exceptiontable; /* Byte string encoding exception handling table */
    int co_flags;               /* CO_..., see below */
    int co_warmup;              /* Warmup counter for quickening */

    // The rest are not so impactful on performance.
    int co_argcount;            /* #arguments, except *args */
    int co_posonlyargcount;     /* #positional only arguments */
    int co_kwonlyargcount;      /* #keyword only arguments */
    int co_stacksize;           /* #entries needed for evaluation stack */
    int co_firstlineno;         /* first source line number */
    PyObject *co_code;          /* instruction opcodes */
    PyObject *co_localsplusnames;  /* tuple mapping offsets to names */
    PyObject *co_localspluskinds; /* Bytes mapping to local kinds (one byte per variable) */
    PyObject *co_filename;      /* unicode (where it was loaded from) */
    PyObject *co_name;          /* unicode (name, for reference) */
    PyObject *co_linetable;     /* string (encoding addr<->lineno mapping) See
                                   Objects/lnotab_notes.txt for details. */

    /* These fields are set with computed values on new code objects. */

    // redundant values (derived from co_localsplusnames and co_localspluskinds)
    int co_nlocalsplus;         /* number of local + cell + free variables */
    int co_nlocals;             /* number of local variables */
    int co_nplaincellvars;      /* number of non-arg cell variables */
    int co_ncellvars;           /* total number of cell variables */
    int co_nfreevars;           /* number of free variables */
    // lazily-computed values
    PyObject *co_varnames;      /* tuple of strings (local variable names) */
    PyObject *co_cellvars;      /* tuple of strings (cell variable names) */
    PyObject *co_freevars;      /* tuple of strings (free variable names) */

    /* The remaining fields are zeroed out on new code objects. */

    PyObject *co_weakreflist;   /* to support weakrefs to code objects */
    /* Scratch space for extra data relating to the code object.
       Type is a void* to keep the format private in codeobject.c to force
       people to go through the proper APIs. */
    void *co_extra;
    /* Quickened instructions and cache, or NULL
     This should be treated as opaque by all code except the specializer and
     interpreter. */
    union _cache_or_instruction *co_quickened;

};

/* Masks for co_flags above */
#define CO_OPTIMIZED    0x0001
#define CO_NEWLOCALS    0x0002
#define CO_VARARGS      0x0004
#define CO_VARKEYWORDS  0x0008
#define CO_NESTED       0x0010
#define CO_GENERATOR    0x0020

/* The CO_COROUTINE flag is set for coroutine functions (defined with
   ``async def`` keywords) */
#define CO_COROUTINE            0x0080
#define CO_ITERABLE_COROUTINE   0x0100
#define CO_ASYNC_GENERATOR      0x0200

/* bpo-39562: These constant values are changed in Python 3.9
   to prevent collision with compiler flags. CO_FUTURE_ and PyCF_
   constants must be kept unique. PyCF_ constants can use bits from
   0x0100 to 0x10000. CO_FUTURE_ constants use bits starting at 0x20000. */
#define CO_FUTURE_DIVISION      0x20000
#define CO_FUTURE_ABSOLUTE_IMPORT 0x40000 /* do absolute imports by default */
#define CO_FUTURE_WITH_STATEMENT  0x80000
#define CO_FUTURE_PRINT_FUNCTION  0x100000
#define CO_FUTURE_UNICODE_LITERALS 0x200000

#define CO_FUTURE_BARRY_AS_BDFL  0x400000
#define CO_FUTURE_GENERATOR_STOP  0x800000
#define CO_FUTURE_ANNOTATIONS    0x1000000

/* This should be defined if a future statement modifies the syntax.
   For example, when a keyword is added.
*/
#define PY_PARSER_REQUIRES_FUTURE_KEYWORD

#define CO_MAXBLOCKS 20 /* Max static block nesting within a function */

PyAPI_DATA(PyTypeObject) PyCode_Type;

#define PyCode_Check(op) Py_IS_TYPE(op, &PyCode_Type)
#define PyCode_GetNumFree(op) ((op)->co_nfreevars)

/* Public interface */
PyAPI_FUNC(PyCodeObject *) PyCode_New(
        int, int, int, int, int, PyObject *, PyObject *,
        PyObject *, PyObject *, PyObject *, PyObject *,
        PyObject *, PyObject *, int, PyObject *, PyObject *);

PyAPI_FUNC(PyCodeObject *) PyCode_NewWithPosOnlyArgs(
        int, int, int, int, int, int, PyObject *, PyObject *,
        PyObject *, PyObject *, PyObject *, PyObject *,
        PyObject *, PyObject *, int, PyObject *, PyObject *);
        /* same as struct above */

/* Creates a new empty code object with the specified source location. */
PyAPI_FUNC(PyCodeObject *)
PyCode_NewEmpty(const char *filename, const char *funcname, int firstlineno);

/* Return the line number associated with the specified bytecode index
   in this code object.  If you just need the line number of a frame,
   use PyFrame_GetLineNumber() instead. */
PyAPI_FUNC(int) PyCode_Addr2Line(PyCodeObject *, int);

/* for internal use only */
struct _opaque {
    int computed_line;
    const char *lo_next;
    const char *limit;
};

typedef struct _line_offsets {
    int ar_start;
    int ar_end;
    int ar_line;
    struct _opaque opaque;
} PyCodeAddressRange;

/* Update *bounds to describe the first and one-past-the-last instructions in the
   same line as lasti.  Return the number of that line.
*/
PyAPI_FUNC(int) _PyCode_CheckLineNumber(int lasti, PyCodeAddressRange *bounds);

/* Create a comparable key used to compare constants taking in account the
 * object type. It is used to make sure types are not coerced (e.g., float and
 * complex) _and_ to distinguish 0.0 from -0.0 e.g. on IEEE platforms
 *
 * Return (type(obj), obj, ...): a tuple with variable size (at least 2 items)
 * depending on the type and the value. The type is the first item to not
 * compare bytes and str which can raise a BytesWarning exception. */
PyAPI_FUNC(PyObject*) _PyCode_ConstantKey(PyObject *obj);

PyAPI_FUNC(PyObject*) PyCode_Optimize(PyObject *code, PyObject* consts,
                                      PyObject *names, PyObject *lnotab);


PyAPI_FUNC(int) _PyCode_GetExtra(PyObject *code, Py_ssize_t index,
                                 void **extra);
PyAPI_FUNC(int) _PyCode_SetExtra(PyObject *code, Py_ssize_t index,
                                 void *extra);

/** API for initializing the line number table. */
int _PyCode_InitAddressRange(PyCodeObject* co, PyCodeAddressRange *bounds);

/** Out of process API for initializing the line number table. */
void PyLineTable_InitAddressRange(const char *linetable, Py_ssize_t length, int firstlineno, PyCodeAddressRange *range);

/** API for traversing the line number table. */
int PyLineTable_NextAddressRange(PyCodeAddressRange *range);
int PyLineTable_PreviousAddressRange(PyCodeAddressRange *range);

