#include "Python.h"
#include "opcode.h"
#include "pycore_interp.h"
#include "pycore_opcode.h"
#include "pycore_opcode_metadata.h"
#include "pycore_opcode_utils.h"
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_uops.h"
#include "cpython/optimizer.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "pycore_optimizer.h"

// TYPENODE is a tagged pointer that uses the last 2 LSB as the tag
#define _Py_PARTITIONNODE_t uintptr_t

// PARTITIONNODE Tags
typedef enum _Py_TypeNodeTags {
    // Node is unused
    TYPE_NULL = 0,
    // TYPE_ROOT_POSITIVE can point to a root struct or be a NULL
    TYPE_ROOT= 1,
    // TYPE_REF points to a TYPE_ROOT or a TYPE_REF
    TYPE_REF = 2,
} _Py_TypeNodeTags;

typedef struct _Py_PartitionRootNode {
    PyObject_HEAD
    // For partial evaluation
    uint8_t static_or_dyanmic;
    // For types (TODO)
} _Py_PartitionRootNode;

PyTypeObject _Py_PartitionRootNode_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "uops abstract interpreter's root node",
    .tp_basicsize = sizeof(_Py_PartitionRootNode),
    .tp_dealloc = PyObject_Del,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION
};

static inline _Py_PARTITIONNODE_t
partitionnode_get_tag(_Py_PARTITIONNODE_t node)
{
    return node & 0b11;
}

static inline _Py_PARTITIONNODE_t
partitionnode_clear_tag(_Py_PARTITIONNODE_t node)
{
    return node & (~(uintptr_t)(0b11));
}

static inline _Py_PARTITIONNODE_t
partitionnode_make_root(uint8_t static_or_dynamic)
{
    _Py_PartitionRootNode *root = PyObject_New(_Py_PartitionRootNode, &_Py_PartitionRootNode_Type);
    if (root == NULL) {
        return 0;
    }
    root->static_or_dyanmic = static_or_dynamic;
    return (_Py_PARTITIONNODE_t)root;
}

static inline _Py_PARTITIONNODE_t
partitionnode_make_ref(_Py_PARTITIONNODE_t node)
{
    return partitionnode_clear_tag(node) | TYPE_REF;
}

static inline _Py_PARTITIONNODE_t
partitionnode_null()
{
    return 0;
}


// Tier 2 types meta interpreter
typedef struct _Py_UOpsAbstractInterpContext {
    PyObject_HEAD
    // points to one element after the abstract stack
    _Py_PARTITIONNODE_t *stack_pointer;
    int stack_len;
    _Py_PARTITIONNODE_t *stack;
    int locals_len;
    _Py_PARTITIONNODE_t *locals;
} _Py_UOpsAbstractInterpContext;

static void
abstractinterp_dealloc(PyObject *o)
{
    _Py_UOpsAbstractInterpContext *self = (_Py_UOpsAbstractInterpContext *)o;
    PyMem_Free(self->stack);
    PyMem_Free(self->locals);
    // TODO traverse the nodes and decref all roots too.
    Py_TYPE(self)->tp_free((PyObject *)self);
}

PyTypeObject _Py_UOpsAbstractInterpContext_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "uops abstract interpreter's context",
    .tp_basicsize = sizeof(_Py_UOpsAbstractInterpContext),
    .tp_dealloc = abstractinterp_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION
};

_Py_UOpsAbstractInterpContext *
_Py_UOpsAbstractInterpContext_New(int stack_len, int locals_len, int curr_stacklen)
{
    _Py_UOpsAbstractInterpContext *self = (_Py_UOpsAbstractInterpContext *)PyType_GenericAlloc(
        (PyTypeObject *)&_Py_UOpsAbstractInterpContext_Type, 0);
    if (self == NULL) {
        return NULL;
    }

    // Setup
    self->stack_len = stack_len;
    self->locals_len = locals_len;

    _Py_PARTITIONNODE_t *locals_with_stack = PyMem_New(_Py_PARTITIONNODE_t, locals_len + stack_len);
    if (locals_with_stack == NULL) {
        Py_DECREF(self);
        return NULL;
    }


    for (int i = 0; i < locals_len + stack_len; i++) {
        locals_with_stack[i] = partitionnode_null();
    }

    self->locals = locals_with_stack;
    self->stack = locals_with_stack + locals_len;
    self->stack_pointer = self->stack + curr_stacklen;

    return self;
}

int
_Py_uop_analyze_and_optimize(
    PyCodeObject *co,
    _PyUOpInstruction *trace,
    int trace_len,
    int curr_stacklen
)
{
#define STACK_LEVEL()     ((int)(stack_pointer - ctx->stack))
#define STACK_SIZE()      (co->co_stacksize)
#define BASIC_STACKADJ(n) (stack_pointer += n)

#ifdef Py_DEBUG
#define STACK_GROW(n)   do { \
                            assert(n >= 0); \
                            BASIC_STACKADJ(n); \
                            assert(STACK_LEVEL() <= STACK_SIZE()); \
                        } while (0)
#define STACK_SHRINK(n) do { \
                            assert(n >= 0); \
                            assert(STACK_LEVEL() >= n); \
                            BASIC_STACKADJ(-(n)); \
                        } while (0)
#else
#define STACK_GROW(n)          BASIC_STACKADJ(n)
#define STACK_SHRINK(n)        BASIC_STACKADJ(-(n))
#endif
    _PyUOpInstruction *temp_writebuffer = PyMem_New(_PyUOpInstruction, trace_len);
    if (temp_writebuffer == NULL) {
        return trace_len;
    }

    _Py_UOpsAbstractInterpContext *ctx = _Py_UOpsAbstractInterpContext_New(co->co_stacksize, co->co_nlocals, curr_stacklen);
    if (ctx == NULL) {
        PyMem_Free(temp_writebuffer);
        return trace_len;
    }

    int oparg;
    int opcode;
    _Py_PARTITIONNODE_t *stack_pointer = ctx->stack_pointer;
    for (int i = 0; i < trace_len; i++) {
        oparg = trace[i].oparg;
        opcode = trace[i].opcode;
        switch (opcode) {
#include "abstract_interp_cases.c.h"
        default:
            fprintf(stderr, "Unknown opcode in abstract interpreter\n");
            Py_UNREACHABLE();
        }
        ctx->stack_pointer = stack_pointer;

    }
    assert(STACK_SIZE() >= 0);
    return trace_len;
}
