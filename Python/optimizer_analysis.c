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
    PyObject *const_val;
    // For types (TODO)
} _Py_PartitionRootNode;

static void
partitionnode_dealloc(PyObject *o)
{
    _Py_PartitionRootNode *self = (_Py_PartitionRootNode *)o;
    Py_CLEAR(self->const_val);
    Py_TYPE(self)->tp_free(o);
}

PyTypeObject _Py_PartitionRootNode_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "uops abstract interpreter's root node",
    .tp_basicsize = sizeof(_Py_PartitionRootNode),
    .tp_dealloc = partitionnode_dealloc,
    .tp_free = PyObject_Free,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION
};

static inline _Py_TypeNodeTags
partitionnode_get_tag(_Py_PARTITIONNODE_t node)
{
    return node & 0b11;
}

static inline _Py_PARTITIONNODE_t
partitionnode_clear_tag(_Py_PARTITIONNODE_t node)
{
    return node & (~(uintptr_t)(0b11));
}

// static_or_dynamic
// 0 - static
// 1 - dynamic
// If static, const_value must be set!
static inline _Py_PARTITIONNODE_t
partitionnode_make_root(uint8_t static_or_dynamic, PyObject *const_val)
{
    _Py_PartitionRootNode *root = PyObject_New(_Py_PartitionRootNode, &_Py_PartitionRootNode_Type);
    if (root == NULL) {
        return 0;
    }
    root->static_or_dyanmic = static_or_dynamic;
    root->const_val = Py_NewRef(const_val);
    fprintf(stderr, "allocating ROOT\n");
    return (_Py_PARTITIONNODE_t)root;
}

static inline _Py_PARTITIONNODE_t
partitionnode_make_ref(_Py_PARTITIONNODE_t node)
{
    return partitionnode_clear_tag(node) | TYPE_REF;
}


static _Py_PARTITIONNODE_t PARTITIONNODE_NULLROOT = (_Py_PARTITIONNODE_t)_Py_NULL | TYPE_ROOT;


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
    // Traverse all nodes and decref the root objects (if they are not NULL).
    // Note: stack is after locals so this is safe
    int total = self->locals_len + self->stack_len;
    for (int i = 0; i < total; i++) {
        _Py_PARTITIONNODE_t node = self->locals[i];
        if (partitionnode_get_tag(node) == TYPE_ROOT) {
            if (node != PARTITIONNODE_NULLROOT) {
                fprintf(stderr, "DEALLOCATING ROOT\n");
            }
            Py_XDECREF(partitionnode_clear_tag(node));
        }
    }
    PyMem_Free(self->locals);
    // No need to free stack because it is allocated together with the locals.
    Py_TYPE(self)->tp_free((PyObject *)self);
}

PyTypeObject _Py_UOpsAbstractInterpContext_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "uops abstract interpreter's context",
    .tp_basicsize = sizeof(_Py_UOpsAbstractInterpContext),
    .tp_dealloc = abstractinterp_dealloc,
    .tp_free = PyObject_Free,
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
        locals_with_stack[i] = PARTITIONNODE_NULLROOT;
    }

    self->locals = locals_with_stack;
    self->stack = locals_with_stack + locals_len;
    self->stack_pointer = self->stack + curr_stacklen;

    return self;
}

static inline _Py_PARTITIONNODE_t *
partitionnode_get_rootptr(_Py_PARTITIONNODE_t *ref)
{
    _Py_TypeNodeTags tag = partitionnode_get_tag(*ref);
    while (tag != TYPE_ROOT) {
        ref = (_Py_PARTITIONNODE_t *)(partitionnode_clear_tag(*ref));
        tag = partitionnode_get_tag(*ref);
    }
    return ref;
}

/**
 * @brief Performs SET operation. dst tree becomes part of src tree
 *
 * If src_is_new is set, src is interpreted as a TYPE_ROOT
 * not part of the type_context. Otherwise, it is interpreted as a pointer
 * to a _Py_PARTITIONNODE_t.
 *
 * If src_is_new:
 *   Overwrites the root of the dst tree with the src node
 * else:
 *   Makes the root of the dst tree a TYPE_REF to src
 *
*/
static void
partitionnode_set(_Py_PARTITIONNODE_t *src, _Py_PARTITIONNODE_t *dst, bool src_is_new)
{
    {

#ifdef Py_DEBUG
        // If `src_is_new` is set:
        //   - `src` doesn't belong inside the type context yet.
        //   - `src` has to be a TYPE_ROOT
        //   - `src` is to be interpreted as a _Py_TYPENODE_t
        if (src_is_new) {
            assert(partitionnode_get_tag(*src) == TYPE_ROOT);
        }
#endif

        _Py_TypeNodeTags tag = partitionnode_get_tag(*dst);
        switch (tag) {
            case TYPE_ROOT: {
                _Py_PARTITIONNODE_t old_root = partitionnode_clear_tag(*dst);
                if (!src_is_new) {
                    // Make dst a reference to src
                    *dst = partitionnode_make_ref(*src);
                    Py_XDECREF(old_root);
                    break;
                }
                // Make dst the src
                *dst = *src;
                Py_XDECREF(old_root);
                break;
            }
            case TYPE_REF: {
                _Py_PARTITIONNODE_t *rootptr = partitionnode_get_rootptr(dst);
                _Py_PARTITIONNODE_t old_root = partitionnode_clear_tag(*rootptr);
                if (!src_is_new) {
                    // Traverse up to the root of dst, make root a reference to src
                    *rootptr = partitionnode_make_ref(*src);
                    // Old root no longer used.
                    Py_XDECREF(old_root);
                    break;
                }
                // Make root of dst the src
                *rootptr = *src;
                // Old root no longer used.
                Py_XDECREF(old_root);
                break;
            }
            default:
                Py_UNREACHABLE();
            }
    }
}


/**
 * @brief Performs OVERWRITE operation. dst node gets overwritten by src node
 *
 * If src_is_new is set, src is interpreted as a TYPE_ROOT
 * not part of the ctx. Otherwise, it is interpreted as a pointer
 * to a _Py_PARTITIONNODE_t.
 *
 * If src_is_new:
 *   Removes dst node from its tree (+fixes all the references to dst)
 *   Overwrite the dst node with the src node
 * else:
 *   Removes dst node from its tree (+fixes all the references to dst)
 *   Makes the root of the dst tree a TYPE_REF to src
 *
*/
static void
partitionnode_overwrite(_Py_UOpsAbstractInterpContext *ctx,
    _Py_PARTITIONNODE_t *src, _Py_PARTITIONNODE_t *dst, bool src_is_new)
{
#ifdef Py_DEBUG
    if (src_is_new) {
        assert(partitionnode_get_tag(*src) == TYPE_ROOT);
    }
#endif
    _Py_TypeNodeTags tag = partitionnode_get_tag(*dst);
    switch (tag) {
        case TYPE_ROOT: {

            _Py_PARTITIONNODE_t old_dst = *dst;
            if (!src_is_new) {
                // Make dst a reference to src
                *dst = partitionnode_make_ref(*src);
            }
            else {
                // Make dst the src
                *dst = *src;
            }

            // No longer need the old root.
            Py_XDECREF(partitionnode_clear_tag(old_dst));

            /* Pick one child of dst and make that the new root of the dst tree */

            // Children of dst will have this form
            _Py_PARTITIONNODE_t child_test = partitionnode_make_ref(
                partitionnode_clear_tag(*dst));
            // Will be initialised to the first child we find (ptr to the new root)
            _Py_PARTITIONNODE_t *new_root_ptr = NULL;

            // Search locals for children
            int nlocals = ctx->locals_len;
            for (int i = 0; i < nlocals; i++) {
                _Py_PARTITIONNODE_t *node_ptr = &(ctx->locals[i]);
                if (*node_ptr == child_test) {
                    if (new_root_ptr == NULL) {
                        // First child encountered! initialise root
                        new_root_ptr = node_ptr;
                        *node_ptr = *dst;
                    }
                    else {
                        // Not the first child encounted, point it to the new root
                        *node_ptr = partitionnode_make_ref(*new_root_ptr);
                    }
                }
            }

            // Search stack for children
            int nstack = ctx->stack_len;
            for (int i = 0; i < nstack; i++) {
                _Py_PARTITIONNODE_t *node_ptr = &(ctx->stack[i]);
                if (*node_ptr == child_test) {
                    if (new_root_ptr == NULL) {
                        // First child encountered! initialise root
                        new_root_ptr = node_ptr;
                        *node_ptr = *dst;
                    }
                    else {
                        // Not the first child encounted, point it to the new root
                        *node_ptr = partitionnode_make_ref(*new_root_ptr);
                    }
                }
            }

            break;
        }
        case TYPE_REF: {

            _Py_PARTITIONNODE_t old_dst = *dst;
            // Make dst a reference to src
            if (!src_is_new) {
                // Make dst a reference to src
                *dst = partitionnode_make_ref(*src);
            }
            else {
                // Make dst the src
                *dst = *src;
            }

            /* Make all child of src be a reference to the parent of dst */

            // Children of dst will have this form
            _Py_PARTITIONNODE_t child_test = partitionnode_make_ref(
                partitionnode_clear_tag(*dst));

            // Search locals for children
            int nlocals = ctx->locals_len;
            for (int i = 0; i < nlocals; i++) {
                _Py_PARTITIONNODE_t *node_ptr = &(ctx->locals[i]);
                if (*node_ptr == child_test) {
                    // Is a child of dst. Point it to the parent of dst
                    *node_ptr = old_dst;
                }
            }

            // Search stack for children
            int nstack = ctx->stack_len;
            for (int i = 0; i < nstack; i++) {
                _Py_PARTITIONNODE_t *node_ptr = &(ctx->stack[i]);
                if (*node_ptr == child_test) {
                    // Is a child of dst. Point it to the parent of dst
                    *node_ptr = old_dst;
                }
            }
            break;
        }
    default:
        Py_UNREACHABLE();
    }
}


#ifndef Py_DEBUG
#define GETITEM(v, i) PyTuple_GET_ITEM((v), (i))
#else
static inline PyObject *
GETITEM(PyObject *v, Py_ssize_t i) {
    assert(PyTuple_Check(v));
    assert(i >= 0);
    assert(i < PyTuple_GET_SIZE(v));
    return PyTuple_GET_ITEM(v, i);
}
#endif

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
#define PEEK(idx)              (&(stack_pointer[-(idx)]))
#define GETLOCAL(idx)          (&(locals[idx]))

#define PARTITIONNODE_SET(src, dst, flag)        partitionnode_set((src), (dst), (flag))
#define PARTITIONNODE_OVERWRITE(src, dst, flag)  partitionnode_overwrite(ctx, (src), (dst), (flag))
#define MAKE_STATIC_ROOT(val)                    partitionnode_make_root(0, (val))
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
    _Py_PARTITIONNODE_t *locals = ctx->locals;
    for (int i = 0; i < trace_len; i++) {
        oparg = trace[i].oparg;
        opcode = trace[i].opcode;
        /*
            "LOAD_FAST",
    "LOAD_FAST_CHECK",
    "LOAD_FAST_AND_CLEAR",
    "LOAD_CONST",
    "STORE_FAST",
    "STORE_FAST_MAYBE_NULL",
    "COPY",
        */
        switch (opcode) {
#include "abstract_interp_cases.c.h"
        // @TODO convert these to autogenerated using DSL
        case LOAD_FAST:
        case LOAD_FAST_CHECK:
            STACK_GROW(1);
            PARTITIONNODE_OVERWRITE(GETLOCAL(oparg), PEEK(1), false);
            break;
        case LOAD_FAST_AND_CLEAR: {
            STACK_GROW(1);
            PARTITIONNODE_OVERWRITE(GETLOCAL(oparg), PEEK(1), false);
            PARTITIONNODE_OVERWRITE(&PARTITIONNODE_NULLROOT, GETLOCAL(oparg), false);
            break;
        }
        case LOAD_CONST: {
            _Py_PARTITIONNODE_t value = MAKE_STATIC_ROOT(GETITEM(co->co_consts, oparg));
            STACK_GROW(1);
            PARTITIONNODE_OVERWRITE(&value, PEEK(1), false);
            break;
        }
        case STORE_FAST:
        case STORE_FAST_MAYBE_NULL: {
            _Py_PARTITIONNODE_t *value = PEEK(1);
            PARTITIONNODE_OVERWRITE(value, GETLOCAL(oparg), false);
            STACK_SHRINK(1);
            break;
        }
        case COPY: {
            _Py_PARTITIONNODE_t *bottom = PEEK(1 + (oparg - 1));
            STACK_GROW(1);
            PARTITIONNODE_SET(bottom, PEEK(1), false);
            break;
        }
        default:
            fprintf(stderr, "Unknown opcode in abstract interpreter\n");
            Py_UNREACHABLE();
        }
        ctx->stack_pointer = stack_pointer;

    }
    assert(STACK_SIZE() >= 0);
    Py_DECREF(ctx);
    PyMem_Free(temp_writebuffer);
    return trace_len;
}
