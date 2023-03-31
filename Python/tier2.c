#include "Python.h"
#include "stdlib.h"
#include "pycore_code.h"
#include "pycore_frame.h"
#include "pycore_opcode.h"
#include "pycore_pystate.h"
#include "pycore_long.h"
#include "stdbool.h"

#include "opcode.h"

#define BB_DEBUG 1
#define TYPEPROP_DEBUG 1
// Max typed version basic blocks per basic block
#define MAX_BB_VERSIONS 10

#define OVERALLOCATE_FACTOR 6


/* Dummy types used by the types propagator */

// Represents a 64-bit unboxed double
PyTypeObject PyRawFloat_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "rawfloat",
    sizeof(PyFloatObject),
};

// Represents a PyLong that fits in a 64-bit long.
PyTypeObject PySmallInt_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "smallint",
    sizeof(PyFloatObject),
};


static inline int IS_SCOPE_EXIT_OPCODE(int opcode);


////////// TYPE CONTEXT FUNCTIONS

static _PyTier2TypeContext *
initialize_type_context(const PyCodeObject *co)
{

#if TYPEPROP_DEBUG
    fprintf(stderr, "  [*] Initialize type context\n");
#endif

    int nlocals = co->co_nlocals;
    int nstack = co->co_stacksize;

    _Py_TYPENODE_t *type_locals = PyMem_Malloc(nlocals * sizeof(_Py_TYPENODE_t));
    if (type_locals == NULL) {
        return NULL;
    }
    _Py_TYPENODE_t *type_stack = PyMem_Malloc(nstack * sizeof(_Py_TYPENODE_t));
    if (type_stack == NULL) {
        PyMem_Free(type_locals);
        return NULL;
    }

    // Initialize to unknown type.
    for (int i = 0; i < nlocals; i++) {
        type_locals[i] = _Py_TYPENODE_NULLROOT;
    }
    for (int i = 0; i < nstack; i++) {
        type_stack[i] = _Py_TYPENODE_NULLROOT;
    }

    _PyTier2TypeContext *type_context = PyMem_Malloc(sizeof(_PyTier2TypeContext));
    if (type_context == NULL) {
        PyMem_Free(type_locals);
        PyMem_Free(type_stack);
        return NULL;
    }
    type_context->type_locals_len = nlocals;
    type_context->type_stack_len = nstack;
    type_context->type_locals = type_locals;
    type_context->type_stack = type_stack;
    type_context->type_stack_ptr = type_stack; // init ptr at start of stack
    return type_context;
}

static _PyTier2TypeContext *
_PyTier2TypeContext_Copy(const _PyTier2TypeContext *type_context)
{

#if TYPEPROP_DEBUG
    fprintf(stderr, "  [*] Copying type context\n");
    static void print_typestack(const _PyTier2TypeContext * type_context);
    print_typestack(type_context);
#endif

    _Py_TYPENODE_t *orig_type_locals = type_context->type_locals;
    _Py_TYPENODE_t *orig_type_stack = type_context->type_stack;
    int nlocals = type_context->type_locals_len;
    int nstack = type_context->type_stack_len;

    _Py_TYPENODE_t *type_locals = PyMem_Malloc(nlocals * sizeof(_Py_TYPENODE_t));
    if (type_locals == NULL) {
        return NULL;
    }
    _Py_TYPENODE_t *type_stack = PyMem_Malloc(nstack * sizeof(_Py_TYPENODE_t));
    if (type_stack == NULL) {
        PyMem_Free(type_locals);
        return NULL;
    }

    _PyTier2TypeContext *new_type_context = PyMem_Malloc(sizeof(_PyTier2TypeContext));
    if (new_type_context == NULL) {
        PyMem_Free(type_locals);
        PyMem_Free(type_stack);
        return NULL;
    }

    for (int i = 0; i < nlocals; i++) {
        _Py_TYPENODE_t node = type_context->type_locals[i];
        switch _Py_TYPENODE_GET_TAG(node)
        {
        case TYPE_ROOT:
            type_locals[i] = node;
            break;
        case TYPE_REF: {
            // Check if part of locals
            _Py_TYPENODE_t *parent = (_Py_TYPENODE_t *)_Py_TYPENODE_CLEAR_TAG(node);

            // Check if part of locals
            int offset_locals = (int)(parent - type_context->type_locals);
            if (0 <= offset_locals && offset_locals < nlocals) {
                type_locals[i] = _Py_TYPENODE_MAKE_REF((_Py_TYPENODE_t)(&type_locals[offset_locals]));
            }
            // Is part of stack
            else {
                int offset_stack = (int)(parent - type_context->type_stack);
#if TYPEPROP_DEBUG
                assert(0 <= offset_stack && offset_stack < nstack);
#endif
                type_locals[i] = _Py_TYPENODE_MAKE_REF((_Py_TYPENODE_t)(&type_stack[offset_stack]));
            }
            break;
        }
        default:
            Py_UNREACHABLE();
        }
    }

    for (int i = 0; i < nstack; i++) {
        _Py_TYPENODE_t node = type_context->type_stack[i];
        switch _Py_TYPENODE_GET_TAG(node)
        {
        case TYPE_ROOT:
            type_stack[i] = node;
            break;
        case TYPE_REF: {
            // Check if part of locals
            _Py_TYPENODE_t *parent = (_Py_TYPENODE_t *)_Py_TYPENODE_CLEAR_TAG(node);

            // Check if part of locals
            int plocals = (int)(parent - type_context->type_locals);
            if (0 <= plocals && plocals < nlocals) {
                type_stack[i] = _Py_TYPENODE_MAKE_REF((_Py_TYPENODE_t)(&type_locals[plocals]));
            }
            // Is part of stack
            else {
                int offset_stack = (int)(parent - type_context->type_stack);
#if TYPEPROP_DEBUG
                assert(0 <= offset_stack && offset_stack < nstack);
#endif
                type_stack[i] = _Py_TYPENODE_MAKE_REF((_Py_TYPENODE_t)(&type_stack[offset_stack]));
            }
            break;
        }
        default:
            Py_UNREACHABLE();
        }
    }

    new_type_context->type_locals_len = nlocals;
    new_type_context->type_stack_len = nstack;
    new_type_context->type_locals = type_locals;
    new_type_context->type_stack = type_stack;
    new_type_context->type_stack_ptr = type_stack - type_context->type_stack + type_context->type_stack_ptr;
    return new_type_context;
}

void
_PyTier2TypeContext_Free(_PyTier2TypeContext *type_context)
{

#if TYPEPROP_DEBUG
    fprintf(stderr, "  [*] Freeing type context\n");
#endif

    PyMem_Free(type_context->type_locals);
    PyMem_Free(type_context->type_stack);
    PyMem_Free(type_context);
}

static _Py_TYPENODE_t*
__typenode_get_rootptr(_Py_TYPENODE_t ref)
{
    uintptr_t tag;
    _Py_TYPENODE_t *ref_ptr;
    do {
        ref_ptr = (_Py_TYPENODE_t *)(_Py_TYPENODE_CLEAR_TAG(ref));
        ref = *ref_ptr;
        tag = _Py_TYPENODE_GET_TAG(ref);
    } while (tag != TYPE_ROOT);
    return ref_ptr;
}

static PyTypeObject*
typenode_get_type(_Py_TYPENODE_t node)
{
    uintptr_t tag = _Py_TYPENODE_GET_TAG(node);
    switch (tag) {
    case TYPE_ROOT: {
        PyTypeObject *ret = (PyTypeObject *)_Py_TYPENODE_CLEAR_TAG(node);
        return ret;
    }
    case TYPE_REF: {
        _Py_TYPENODE_t *root_ref = __typenode_get_rootptr(node);
        PyTypeObject *ret = (PyTypeObject *)_Py_TYPENODE_CLEAR_TAG(*root_ref);
        return ret;
    }
    default:
        Py_UNREACHABLE();
    }
}

static void
__type_propagate_TYPE_SET(
    _Py_TYPENODE_t *src, _Py_TYPENODE_t *dst, bool src_is_new)
{

#ifdef Py_DEBUG
    // If `src_is_new` is set:
    //   - `src` doesn't belong inside the type context yet.
    //   - `src` has to be a TYPE_ROOT
    //   - `src` is to be interpreted as a _Py_TYPENODE_t
    if (src_is_new) {
        assert(_Py_TYPENODE_GET_TAG((_Py_TYPENODE_t)src) == TYPE_ROOT);
    }
#endif

    uintptr_t tag = _Py_TYPENODE_GET_TAG(*dst);
    switch (tag) {
    case TYPE_ROOT: {
        if (!src_is_new) {
            // Make dst a reference to src
            *dst = _Py_TYPENODE_MAKE_REF((_Py_TYPENODE_t)src);
            break;
        }
        // Make dst the src
        *dst = (_Py_TYPENODE_t)src;
        break;
    }
    case TYPE_REF: {
        _Py_TYPENODE_t *rootref = __typenode_get_rootptr(*dst);
        if (!src_is_new) {
            // Traverse up to the root of dst, make root a reference to src
            *rootref = _Py_TYPENODE_MAKE_REF((_Py_TYPENODE_t)src);
            break;
        }
        // Make root of dst the src
        *rootref = (_Py_TYPENODE_t)src;
        break;
    }
    default:
        Py_UNREACHABLE();
    }
}

static void
__type_propagate_TYPE_OVERWRITE(
    _PyTier2TypeContext *type_context,
    _Py_TYPENODE_t* src, _Py_TYPENODE_t* dst, bool src_is_new)
{

#ifdef Py_DEBUG
    // See: __type_propagate_TYPE_SET
    if (src_is_new) {
        assert(_Py_TYPENODE_GET_TAG((_Py_TYPENODE_t)src) == TYPE_ROOT);
    }
#endif

    uintptr_t tag = _Py_TYPENODE_GET_TAG(*dst);
    switch (tag) {
    case TYPE_ROOT: {
        
        _Py_TYPENODE_t old_dst = *dst;
        if (!src_is_new) {
            // Make dst a reference to src
            *dst = _Py_TYPENODE_MAKE_REF((_Py_TYPENODE_t)src);
        }
        else {
            // Make dst the src
            *dst = (_Py_TYPENODE_t)src;
        }

        /* Pick one child of dst andmake that the new root of the dst tree */

        // Children of dst will have this form
        _Py_TYPENODE_t child_test = _Py_TYPENODE_MAKE_REF(
            _Py_TYPENODE_CLEAR_TAG((_Py_TYPENODE_t)dst));
        // Will be initialised to the first child we find (ptr to the new root)
        _Py_TYPENODE_t *new_root_ptr = NULL;

        // Search locals for children
        int nlocals = type_context->type_locals_len;
        for (int i = 0; i < nlocals; i++) {
            _Py_TYPENODE_t *node_ptr = &(type_context->type_locals[i]);
            if (*node_ptr == child_test) {
                if (new_root_ptr == NULL) {
                    // First child encountered! initialise root
                    new_root_ptr = node_ptr;
                    *node_ptr = old_dst;
                }
                else {
                    // Not the first child encounted, point it to the new root
                    *node_ptr = _Py_TYPENODE_MAKE_REF((_Py_TYPENODE_t)new_root_ptr);
                }
            }
        }

        // Search stack for children
        int nstack = type_context->type_stack_len;
        for (int i = 0; i < nstack; i++) {
            _Py_TYPENODE_t *node_ptr = &(type_context->type_stack[i]);
            if (*node_ptr == child_test) {
                if (new_root_ptr == NULL) {
                    // First child encountered! initialise root
                    new_root_ptr = node_ptr;
                    *node_ptr = old_dst;
                }
                else {
                    // Not the first child encounted, point it to the new root
                    *node_ptr = _Py_TYPENODE_MAKE_REF((_Py_TYPENODE_t)new_root_ptr);
                }
            }
        }
        break;
    }
    case TYPE_REF: {

        // Make dst a reference to src
        _Py_TYPENODE_t old_dst = *dst;
        if (!src_is_new) {
            // Make dst a reference to src
            *dst = _Py_TYPENODE_MAKE_REF((_Py_TYPENODE_t)src);
        }
        else {
            // Make dst the src
            *dst = (_Py_TYPENODE_t)src;
        }

        /* Make all child of src be a reference to the parent of dst */

        // Children of dst will have this form
        _Py_TYPENODE_t child_test = _Py_TYPENODE_MAKE_REF(
            _Py_TYPENODE_CLEAR_TAG((_Py_TYPENODE_t)dst));

        // Search locals for children
        int nlocals = type_context->type_locals_len;
        for (int i = 0; i < nlocals; i++) {
            _Py_TYPENODE_t *node_ptr = &(type_context->type_locals[i]);
            if (*node_ptr == child_test) {
                // Is a child of dst. Point it to the parent of dst
                *node_ptr = old_dst;
            }
        }

        // Search stack for children
        int nstack = type_context->type_stack_len;
        for (int i = 0; i < nstack; i++) {
            _Py_TYPENODE_t *node_ptr = &(type_context->type_stack[i]);
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

// src and dst are assumed to already be within the type context
static void
__type_propagate_TYPE_SWAP(
    _PyTier2TypeContext *type_context,
    _Py_TYPENODE_t *src, _Py_TYPENODE_t *dst)
{
    // Check if they are the same tree
    _Py_TYPENODE_t *srcrootref = src;
    _Py_TYPENODE_t *dstrootref = dst;
    uintptr_t dsttag = _Py_TYPENODE_GET_TAG(*dst);
    uintptr_t srctag = _Py_TYPENODE_GET_TAG(*src);
    switch (dsttag) {
    case TYPE_REF: dstrootref = __typenode_get_rootptr(*dst);
    case TYPE_ROOT:
        switch (srctag) {
        case TYPE_REF: srcrootref = __typenode_get_rootptr(*src);
        case TYPE_ROOT:
            if (srcrootref == dstrootref) {
                // Same tree, no point swapping
                return;
            }
            break;
        default:
            Py_UNREACHABLE();
        }
        break;
    default:
        Py_UNREACHABLE();
    }

    // src and dst are different tree,
    // Make all children of src be children of dst and vice versa

    _Py_TYPENODE_t src_child_test = _Py_TYPENODE_MAKE_REF(
        _Py_TYPENODE_CLEAR_TAG((_Py_TYPENODE_t)src));
    _Py_TYPENODE_t dst_child_test = _Py_TYPENODE_MAKE_REF(
        _Py_TYPENODE_CLEAR_TAG((_Py_TYPENODE_t)dst));

    // Search locals for children
    int nlocals = type_context->type_locals_len;
    for (int i = 0; i < nlocals; i++) {
        _Py_TYPENODE_t *node_ptr = &(type_context->type_locals[i]);
        if (*node_ptr == src_child_test) {
            *node_ptr = dst_child_test;
        }
        else if (*node_ptr == dst_child_test) {
            *node_ptr = src_child_test;
        }
    }

    // Search stack for children
    int nstack = type_context->type_stack_len;
    for (int i = 0; i < nstack; i++) {
        _Py_TYPENODE_t *node_ptr = &(type_context->type_stack[i]);
        if (*node_ptr == src_child_test) {
            *node_ptr = dst_child_test;
        }
        else if (*node_ptr == dst_child_test) {
            *node_ptr = src_child_test;
        }
    }

    // Finally, actually swap the nodes
    *src ^= *dst;
    *dst ^= *src;
    *src ^= *dst;
}

static void
__type_stack_shrink(_Py_TYPENODE_t **type_stackptr, int idx)
{
    // TODO:
    //   If we don't touch the stack elements
    //   when shrinking, we need to check for references
    //   on these elements.
    //   Otherwise, if we NULL these elements, we need to refactor
    //   the type propagator to perform shrinking last.
    //while (idx--) {
    //    __type_propagate_TYPE_OVERWRITE(
    //        type_context,
    //        (_Py_TYPENODE_t *)_Py_TYPENODE_NULLROOT, *type_stackptr,
    //        true);
    //    *type_stackptr -= 1;
    //}
    *type_stackptr -= idx;
}

#if TYPEPROP_DEBUG

static void
print_typestack(const _PyTier2TypeContext *type_context)
{
    _Py_TYPENODE_t *type_stack = type_context->type_stack;
    _Py_TYPENODE_t *type_locals = type_context->type_locals;
    _Py_TYPENODE_t *type_stackptr = type_context->type_stack_ptr;

    int nstack_use = (int)(type_stackptr - type_stack);
    int nstack = type_context->type_stack_len;
    int nlocals = type_context->type_locals_len;

    int plocals = 0;
    int pstack = 0;
    bool is_local = false;

    fprintf(stderr, "      Stack: %p: [", type_stack);
    for (int i = 0; i < nstack; i++) {
        _Py_TYPENODE_t node = type_stack[i];
        PyTypeObject *type = typenode_get_type(node);
        _Py_TYPENODE_t tag = _Py_TYPENODE_GET_TAG(node);

        if (tag == TYPE_REF) {
            _Py_TYPENODE_t *parent = (_Py_TYPENODE_t *)(_Py_TYPENODE_CLEAR_TAG(node));
            plocals = (int)(parent - type_context->type_locals);
            pstack = (int)(parent - type_context->type_stack);
            is_local = (0 <= plocals) && (plocals < nlocals);
            if (!is_local) {
                assert((0 <= pstack) && (pstack < nstack));
            }
        }

        fprintf(stderr, "%s%s",
            i == nstack_use ? "." : " ",
            type == NULL ? "?" : type->tp_name);
        if (tag == TYPE_REF) {
            fprintf(stderr, "%s%d]",
                is_local ? "->locals[" : "->stack[",
                is_local ? plocals : pstack);
        }
    }
    fprintf(stderr, "]\n");

    fprintf(stderr, "      Locals %p: [", type_locals);
    for (int i = 0; i < nlocals; i++) {
        _Py_TYPENODE_t node = type_locals[i];
        PyTypeObject *type = typenode_get_type(node);
        _Py_TYPENODE_t tag = _Py_TYPENODE_GET_TAG(node);

        if (tag == TYPE_REF) {
            _Py_TYPENODE_t *parent = (_Py_TYPENODE_t *)(_Py_TYPENODE_CLEAR_TAG(node));
            plocals = (int)(parent - type_context->type_locals);
            pstack = (int)(parent - type_context->type_stack);
            is_local = (0 <= plocals) && (plocals < nlocals);
            if (!is_local) {
                assert((0 <= pstack) && (pstack < nstack));
            }
        }

        fprintf(stderr, " %s",
            type == NULL ? "?" : type->tp_name);
        if (tag == TYPE_REF) {
            fprintf(stderr, "%s%d]",
                is_local ? "->locals[" : "->stack[",
                is_local ? plocals : pstack);
        }
    }
    fprintf(stderr, "]\n");
}
#endif

// Type propagates across a single function. 
static void
type_propagate(
    int opcode, int oparg,
    _PyTier2TypeContext *type_context,
    const PyObject *consts)
{
    _Py_TYPENODE_t *type_stack = type_context->type_stack;
    _Py_TYPENODE_t *type_locals = type_context->type_locals;
    _Py_TYPENODE_t **type_stackptr = &type_context->type_stack_ptr;

#define TARGET(op) case op:
#define TYPESTACK_PEEK(idx)         (&((*type_stackptr)[-(idx)]))
#define TYPELOCALS_GET(idx)         (&(type_locals[idx]))

// Get the type of the const and make into a TYPENODE ROOT
#define TYPECONST_GET(idx)          _Py_TYPENODE_MAKE_ROOT( \
                                        (_Py_TYPENODE_t)Py_TYPE( \
                                            PyTuple_GET_ITEM(consts, idx)))

#define TYPE_SET(src, dst, flag)        __type_propagate_TYPE_SET((src), (dst), (flag))
#define TYPE_OVERWRITE(src, dst, flag)  __type_propagate_TYPE_OVERWRITE(type_context, (src), (dst), (flag))
#define TYPE_SWAP(src, dst)             __type_propagate_TYPE_SWAP(type_context, (src), (dst))

#define STACK_GROW(idx)             *type_stackptr += (idx)

// Stack shrinking has to NULL the nodes
#define STACK_SHRINK(idx)           __type_stack_shrink(type_stackptr, (idx))

#if TYPEPROP_DEBUG
    fprintf(stderr, "  [-] Type stack bef: %llu\n", (uint64_t)(*type_stackptr - type_stack));
#ifdef Py_DEBUG
    fprintf(stderr, "  [-] Type propagating across: %s : %d\n", _PyOpcode_OpName[opcode], oparg);
#endif
#endif

    switch (opcode) {
#include "tier2_typepropagator.c.h"
    TARGET(SWAP) {
        _Py_TYPENODE_t *top = TYPESTACK_PEEK(1);
        _Py_TYPENODE_t * bottom = TYPESTACK_PEEK(2 + (oparg - 2));
        TYPE_SWAP(top, bottom);
        break;
    }
    default:
#ifdef Py_DEBUG
        fprintf(stderr, "Unsupported opcode in type propagator: %s : %d\n", _PyOpcode_OpName[opcode], oparg);
#else
        fprintf(stderr, "Unsupported opcode in type propagator: %d\n", opcode);
#endif
        Py_UNREACHABLE();
    }

#if TYPEPROP_DEBUG
    fprintf(stderr, "  [-] Type stack aft: %llu\n", (uint64_t)(*type_stackptr - type_stack));
    print_typestack(type_context);
#endif

#undef TARGET
#undef STACK_ADJUST
#undef STACK_GROW
#undef STACK_SHRINK
#undef TYPECONST_GET
}

////////// Utility functions

// Gets end of the bytecode for a code object.
_Py_CODEUNIT *
_PyCode_GetEnd(PyCodeObject *co)
{
    _Py_CODEUNIT *end = (_Py_CODEUNIT *)(co->co_code_adaptive + _PyCode_NBYTES(co));
    return end;
}

// Gets end of actual bytecode executed. _PyCode_GetEnd might return a CACHE instruction.
_Py_CODEUNIT *
_PyCode_GetLogicalEnd(PyCodeObject *co)
{
    _Py_CODEUNIT *end = _PyCode_GetEnd(co);
    while (_Py_OPCODE(*end) == CACHE) {
        end--;
    }
#if BB_DEBUG
    if (!IS_SCOPE_EXIT_OPCODE(_Py_OPCODE(*end))) {
        fprintf(stderr, "WRONG EXIT OPCODE: %d\n", _Py_OPCODE(*end));
        assert(0);
    }
#endif
    assert(IS_SCOPE_EXIT_OPCODE(_Py_OPCODE(*end)));
    return end;
}


////////// BB SPACE FUNCTIONS

// Creates the overallocated array for the BBs.
static _PyTier2BBSpace *
_PyTier2_CreateBBSpace(Py_ssize_t space_to_alloc)
{
    _PyTier2BBSpace *bb_space = PyMem_Malloc(space_to_alloc + sizeof(_PyTier2BBSpace));
    if (bb_space == NULL) {
        return NULL;
    }
    bb_space->water_level = 0;
    bb_space->max_capacity = space_to_alloc;
    return bb_space;
}

// Checks if there's enough space in the BBSpace for space_requested.
// Reallocates if neccessary.
// DOES NOT ADJUST THE WATER LEVEL AS THIS IS JUST A CHECK. ONLY ADJUSTS THE MAX SPACE.
static _PyTier2BBSpace *
_PyTier2_BBSpaceCheckAndReallocIfNeeded(PyCodeObject *co, Py_ssize_t space_requested)
{
    assert(co->_tier2_info != NULL);
    assert(co->_tier2_info->_bb_space != NULL);
    _PyTier2BBSpace *curr = co->_tier2_info->_bb_space;
    // Over max capacity
    if (curr->water_level + space_requested > curr->max_capacity) {
        // Note: overallocate
        Py_ssize_t new_size = sizeof(_PyTier2BBSpace) + (curr->water_level + space_requested) * 2;
#if BB_DEBUG
        fprintf(stderr, "Space requested: %lld, Allocating new BB of size %lld\n", (int64_t)space_requested, (int64_t)new_size);
#endif
        // @TODO We can't Realloc, we actually need to do the linked list method.
        Py_UNREACHABLE();
        //_PyTier2BBSpace *new_space = PyMem_Realloc(curr, new_size);
        //if (new_space == NULL) {
        //    return NULL;
        //}
        //co->_tier2_info->_bb_space = new_space;
        //new_space->max_capacity = new_size;
        //return new_space;
    }
    // We have enouogh space. Don't do anything, j
    return curr;
}

// BB METADATA FUNCTIONS

static _PyTier2BBMetadata *
allocate_bb_metadata(PyCodeObject *co, _Py_CODEUNIT *tier2_start,
    _Py_CODEUNIT *tier1_end,
    _PyTier2TypeContext *type_context)
{
    _PyTier2BBMetadata *metadata = PyMem_Malloc(sizeof(_PyTier2BBMetadata));
    if (metadata == NULL) {
        return NULL;

    }

    metadata->tier2_start = tier2_start;
    metadata->tier1_end = tier1_end;
    metadata->type_context = type_context;
    return metadata;
}


// Writes BB metadata to code object's tier2info bb_data field.
// 0 on success, 1 on error.
static int
write_bb_metadata(PyCodeObject *co, _PyTier2BBMetadata *metadata)
{
    assert(co->_tier2_info != NULL);
    // Not enough space left in bb_data, allocate new one.
    if (co->_tier2_info->bb_data == NULL ||
        co->_tier2_info->bb_data_curr >= co->_tier2_info->bb_data_len) {
        int new_len = (co->_tier2_info->bb_data_len + 1) * 2;
        Py_ssize_t new_space = new_len * sizeof(_PyTier2BBMetadata *);
        // Overflow
        if (new_len < 0) {
            return 1;
        }
        _PyTier2BBMetadata **new_data = PyMem_Realloc(co->_tier2_info->bb_data, new_space);
        if (new_data == NULL) {
            return 1;
        }
        co->_tier2_info->bb_data = new_data;
        co->_tier2_info->bb_data_len = new_len;
    }
    int id = co->_tier2_info->bb_data_curr;
    co->_tier2_info->bb_data[id] = metadata;
    metadata->id = id;
    co->_tier2_info->bb_data_curr++;
#if BB_DEBUG
    fprintf(stderr, "Creating a BB Metadata with ID %d\n", id);
#endif
    return 0;
}

static _PyTier2BBMetadata *
_PyTier2_AllocateBBMetaData(PyCodeObject *co, _Py_CODEUNIT *tier2_start,
    _Py_CODEUNIT *tier1_end,
    _PyTier2TypeContext *type_context)
{
    _PyTier2BBMetadata *meta = allocate_bb_metadata(co,
        tier2_start, tier1_end, type_context);
    if (meta == NULL) {
        return NULL;
    }
    if (write_bb_metadata(co, meta)) {
        PyMem_Free(meta);
        return NULL;
    }

    return meta;

}

/* Opcode detection functions. Keep in sync with compile.c and dis! */

// dis.hasjabs
static inline int
IS_JABS_OPCODE(int opcode)
{
    return 0;
}

// dis.hasjrel
static inline int
IS_JREL_OPCODE(int opcode)
{
    switch (opcode) {
    case FOR_ITER:
    case JUMP_FORWARD:
        // These two tend to be after a COMPARE_OP
    case POP_JUMP_IF_FALSE:
    case POP_JUMP_IF_TRUE:
    case SEND:
    case POP_JUMP_IF_NOT_NONE:
    case POP_JUMP_IF_NONE:
    case JUMP_BACKWARD_QUICK:
    case JUMP_BACKWARD_NO_INTERRUPT:
    case JUMP_BACKWARD:
        return 1;
    default:
        return 0;

    }
}

static inline int
IS_JUMP_BACKWARDS_OPCODE(int opcode)
{
    return opcode == JUMP_BACKWARD_NO_INTERRUPT ||
        opcode == JUMP_BACKWARD ||
        opcode == JUMP_BACKWARD_QUICK;
}


// dis.hasjrel || dis.hasjabs
static inline int
IS_JUMP_OPCODE(int opcode)
{
    return IS_JREL_OPCODE(opcode) || IS_JABS_OPCODE(opcode);
}

// dis.hascompare
static inline int
IS_COMPARE_OPCODE(int opcode)
{
    return opcode == COMPARE_OP;
}

static inline int
IS_SCOPE_EXIT_OPCODE(int opcode)
{
    switch (opcode) {
    case RETURN_VALUE:
    case RETURN_CONST:
    case RAISE_VARARGS:
    case RERAISE:
    case INTERPRETER_EXIT:
        return 1;
    default:
        return 0;
    }
}

// KEEP IN SYNC WITH COMPILE.c!!!!
static int
IS_TERMINATOR_OPCODE(int opcode)
{
    return IS_JUMP_OPCODE(opcode) || IS_SCOPE_EXIT_OPCODE(opcode);
}

// Opcodes that we can't handle at the moment. If we see them,
// ditch tier 2 attempts.
static inline int
IS_FORBIDDEN_OPCODE(int opcode, int nextop)
{
    switch (opcode) {
        // Modifying containers
    case LIST_EXTEND:
    case SET_UPDATE:
    case DICT_UPDATE:
        // f-strings
    case FORMAT_VALUE:
        // Type hinting
    case SETUP_ANNOTATIONS:
        // Context manager
    case BEFORE_WITH:
        // Generators and coroutines
    case SEND:
    case YIELD_VALUE:
    case GET_AITER:
    case GET_ANEXT:
    case BEFORE_ASYNC_WITH:
    case END_ASYNC_FOR:
        // Raise keyword
    case RAISE_VARARGS:
        // Exceptions, we could support these theoretically.
        // Just too much work for now
    case PUSH_EXC_INFO:
    case RERAISE:
    case POP_EXCEPT:
    case CHECK_EXC_MATCH:
    case CLEANUP_THROW:
        // Closures
    case LOAD_DEREF:
    case LOAD_CLASSDEREF:
    case MAKE_CELL:
        // DELETE_FAST
    case DELETE_FAST:
        // Pattern matching
    case MATCH_MAPPING:
    case MATCH_SEQUENCE:
    case MATCH_KEYS:
        return 1;
        // Two simultaneous EXTENDED_ARG
    case EXTENDED_ARG:
        return nextop == EXTENDED_ARG;
    default:
        return 0;
    }
}

// Decides what values we need to rebox.
// num_elements is how many stack entries and thus how far from the TOS we want to rebox.
static inline _Py_CODEUNIT *
rebox_stack(_Py_CODEUNIT *write_curr,
    _PyTier2TypeContext *type_context, int num_elements)
{
    for (int i = 0; i < num_elements; i++) {
        _Py_TYPENODE_t *curr = type_context->type_stack_ptr - 1 - i;
        if (typenode_get_type(*curr) == &PyRawFloat_Type) {
            write_curr->op.code = BOX_FLOAT;
            write_curr->op.arg = i;
            write_curr++;
            type_propagate(BOX_FLOAT, i, type_context, NULL);
        }
    }
    return write_curr;
}

static inline _Py_CODEUNIT *
emit_cache_entries(_Py_CODEUNIT *write_curr, int cache_entries)
{
    for (int i = 0; i < cache_entries; i++) {
        _py_set_opcode(write_curr, CACHE);
        write_curr++;
    }
    return write_curr;
}

#define BB_ID(bb_id_raw) (bb_id_raw >> 1)
#define BB_IS_TYPE_BRANCH(bb_id_raw) (bb_id_raw & 1)
#define MAKE_TAGGED_BB_ID(bb_id, type_branch) (bb_id << 1 | type_branch)

static inline void
write_bb_id(_PyBBBranchCache *cache, int bb_id, bool is_type_guard) {
    assert((uint16_t)(bb_id) == bb_id);
    // Make sure MSB is unset, because we need to shift it.
    assert((bb_id & 0x8000) == 0);
    cache->bb_id_tagged = MAKE_TAGGED_BB_ID((uint16_t)bb_id, is_type_guard);
}

// The order/hierarchy to emit type guards
// NEED TO ADD TO THIS EVERY TIME WE ADD A NEW ONE.
static int type_guard_ladder[256] = {
    -1,
    BINARY_CHECK_FLOAT,
    BINARY_CHECK_INT,
    -1,
    CHECK_LIST,
    -1,
};

// Type guard to index in the ladder.
// KEEP IN SYNC WITH INDEX IN type_guard_ladder
static int type_guard_to_index[256] = {
    [BINARY_CHECK_FLOAT] = 1,
    [BINARY_CHECK_INT] = 2,
    [CHECK_LIST] = 4,
};


static inline _Py_CODEUNIT *
emit_type_guard(_Py_CODEUNIT *write_curr, int guard_opcode, int guard_oparg, int bb_id)
{
#if BB_DEBUG && defined(Py_DEBUG)
    fprintf(stderr, "emitted type guard %p %s\n", write_curr,
        _PyOpcode_OpName[guard_opcode]);
#endif
    write_curr->op.code = guard_opcode;
    write_curr->op.arg = guard_oparg;
    write_curr++;
    _py_set_opcode(write_curr, BB_BRANCH);
    write_curr->op.arg = 0;
    write_curr++;
    _PyBBBranchCache *cache = (_PyBBBranchCache *)write_curr;
    write_curr = emit_cache_entries(write_curr, INLINE_CACHE_ENTRIES_BB_BRANCH);
    write_bb_id(cache, bb_id, true);
    return write_curr;
}

// Converts the tier 1 branch bytecode to tier 2 branch bytecode.
// This converts sequence of instructions like
// POP_JUMP_IF_FALSE
// to
// BB_TEST_POP_IF_FALSE
// BB_BRANCH
// CACHE (bb_id of the current BB << 1 | is_type_branch)
static inline _Py_CODEUNIT *
emit_logical_branch(_PyTier2TypeContext *type_context, _Py_CODEUNIT *write_curr,
    _Py_CODEUNIT branch, int bb_id, int oparg)
{
    int opcode;
    // @TODO handle JUMP_BACKWARDS and JUMP_BACKWARDS_NO_INTERRUPT
    switch (_PyOpcode_Deopt[_Py_OPCODE(branch)]) {
    case JUMP_BACKWARD_QUICK:
    case JUMP_BACKWARD:
        // The initial backwards jump needs to find the right basic block.
        // Subsequent jumps don't need to check this anymore. They can just
        // jump directly with JUMP_BACKWARD.
        opcode = BB_JUMP_BACKWARD_LAZY;
        // v BB_JUMP_BACKWARD_LAZY has nothing to propagate
        // type_propagate(opcode, oparg, type_context, NULL);
        break;
    case FOR_ITER:
        opcode = BB_TEST_ITER;
        // This inst has conditional stack effect according to whether the branch is taken.
        // This inst sets the `gen_bb_requires_pop` flag to handle stack effect of this opcode in BB_BRANCH
        break;
    case POP_JUMP_IF_FALSE:
        opcode = BB_TEST_POP_IF_FALSE;
        type_propagate(opcode, oparg, type_context, NULL);
        break;
    case POP_JUMP_IF_TRUE:
        opcode = BB_TEST_POP_IF_TRUE;
        type_propagate(opcode, oparg, type_context, NULL);
        break;
    case POP_JUMP_IF_NOT_NONE:
        opcode = BB_TEST_POP_IF_NOT_NONE;
        type_propagate(opcode, oparg, type_context, NULL);
        break;
    case POP_JUMP_IF_NONE:
        opcode = BB_TEST_POP_IF_NONE;
        type_propagate(opcode, oparg, type_context, NULL);
        break;
    default:
        // Honestly shouldn't happen because branches that
        // we can't handle are in IS_FORBIDDEN_OPCODE
#if BB_DEBUG
        fprintf(stderr,
            "emit_logical_branch unreachable opcode %d\n", _Py_OPCODE(branch));
#endif
        Py_UNREACHABLE();
    }
    assert(oparg <= 0XFFFF);
    bool requires_extended_arg = oparg > 0xFF;
    // Backwards jumps should be handled specially.
    if (opcode == BB_JUMP_BACKWARD_LAZY) {
#if BB_DEBUG
        fprintf(stderr, "emitted backwards jump %p %d\n", write_curr,
            _Py_OPCODE(branch));
#endif
        // Just in case, can be swapped out with an EXTENDED_ARG
        _py_set_opcode(write_curr, requires_extended_arg ? EXTENDED_ARG : NOP);
        write_curr->op.arg = (oparg >> 8) & 0xFF;
        write_curr++;
        // We don't need to recalculate the backward jump, because that only needs to be done
        // when it locates the next BB in JUMP_BACKWARD_LAZY.
        _py_set_opcode(write_curr, BB_JUMP_BACKWARD_LAZY);
        write_curr->op.arg = oparg & 0xFF;
        write_curr++;
        _PyBBBranchCache *cache = (_PyBBBranchCache *)write_curr;
        write_curr = emit_cache_entries(write_curr, INLINE_CACHE_ENTRIES_BB_BRANCH);
        write_bb_id(cache, bb_id, false);
        return write_curr;
    }
    // FOR_ITER is also a special jump
    else if (opcode == BB_TEST_ITER) {
#if BB_DEBUG
        fprintf(stderr, "emitted iter branch %p %d\n", write_curr,
            _Py_OPCODE(branch));
#endif
        // The oparg of FOR_ITER is a little special, the actual jump has to jump over
        // its own cache entries, the oparg, -1 to tell it to start generating from the
        // END_FOR. However, at runtime, we will skip this END_FOR.
        // NOTE: IF YOU CHANGE ANY OF THE INSTRUCTIONS BELOW, MAKE SURE
        // TO UPDATE THE CALCULATION OF OPARG. THIS IS EXTREMELY IMPORTANT.
        oparg = INLINE_CACHE_ENTRIES_FOR_ITER + oparg;
        requires_extended_arg = oparg > 0xFF;
        _py_set_opcode(write_curr, requires_extended_arg ? EXTENDED_ARG : NOP);
        write_curr->op.arg = (oparg >> 8) & 0xFF;
        write_curr++;
        _py_set_opcode(write_curr, BB_TEST_ITER);
        write_curr->op.arg = oparg & 0xFF;
        write_curr++;
        // Initialize adaptive interpreter counter
        write_curr->cache = adaptive_counter_warmup();
        write_curr = emit_cache_entries(write_curr, INLINE_CACHE_ENTRIES_FOR_ITER);
        type_propagate(BB_TEST_ITER, oparg, type_context, NULL);
        _py_set_opcode(write_curr, requires_extended_arg ? EXTENDED_ARG : NOP);
        write_curr->op.arg = (oparg >> 8) & 0xFF;
        write_curr++;
        _py_set_opcode(write_curr, BB_BRANCH);
        write_curr->op.arg = oparg & 0xFF;
        write_curr++;
        _PyBBBranchCache *cache = (_PyBBBranchCache *)write_curr;
        write_curr = emit_cache_entries(write_curr, INLINE_CACHE_ENTRIES_BB_BRANCH);
        write_bb_id(cache, bb_id, false);
        return write_curr;
    }
    else {
#if BB_DEBUG
        fprintf(stderr, "emitted logical branch %p %d\n", write_curr,
            _Py_OPCODE(branch));
#endif
        _py_set_opcode(write_curr, requires_extended_arg ? EXTENDED_ARG : NOP);
        write_curr->op.arg = (oparg >> 8) & 0xFF;
        write_curr++;
        _py_set_opcode(write_curr, opcode);
        write_curr->op.arg = oparg & 0xFF;
        write_curr++;
        _py_set_opcode(write_curr, requires_extended_arg ? EXTENDED_ARG : NOP);
        write_curr->op.arg = (oparg >> 8) & 0xFF;
        write_curr++;
        _py_set_opcode(write_curr, BB_BRANCH);
        write_curr->op.arg = oparg & 0xFF;
        write_curr++;
        _PyBBBranchCache *cache = (_PyBBBranchCache *)write_curr;
        write_curr = emit_cache_entries(write_curr, INLINE_CACHE_ENTRIES_BB_BRANCH);
        write_bb_id(cache, bb_id, false);
        return write_curr;
    }
}

static inline _Py_CODEUNIT *
emit_scope_exit(_Py_CODEUNIT *write_curr, _Py_CODEUNIT exit,
    _PyTier2TypeContext *type_context)
{
    switch (_Py_OPCODE(exit)) {
    case RETURN_VALUE:
        write_curr = rebox_stack(write_curr, type_context, 1);
        *write_curr = exit;
        write_curr++;
        return write_curr;
    case RETURN_CONST:
    case INTERPRETER_EXIT:
#if BB_DEBUG
        fprintf(stderr, "emitted scope exit\n");
#endif
        //// @TODO we can propagate and chain BBs across call boundaries
        //// Thanks to CPython's inlined call frames.
        //_py_set_opcode(write_curr, BB_EXIT_FRAME);
        *write_curr = exit;
        write_curr++;
        return write_curr;
    default:
        // The rest are forbidden.
#if BB_DEBUG
        fprintf(stderr, "emit_scope_exit unreachable %d\n", _Py_OPCODE(exit));
#endif
        Py_UNREACHABLE();
    }
}

static inline _Py_CODEUNIT *
emit_i(_Py_CODEUNIT *write_curr, int opcode, int oparg)
{
    if (oparg > 0xFF) {
        _py_set_opcode(write_curr, EXTENDED_ARG);
        write_curr->op.arg = (oparg >> 8) & 0xFF;
        write_curr++;
    }
    _py_set_opcode(write_curr, opcode);
    write_curr->op.arg = oparg & 0xFF;
    write_curr++;
    return write_curr;
}

// Note: we're copying over the actual caches to preserve information!
// This way instructions that we can't type propagate over still stay
// optimized.
static inline _Py_CODEUNIT *
copy_cache_entries(_Py_CODEUNIT *write_curr, _Py_CODEUNIT *cache, int n_entries)
{
    for (int i = 0; i < n_entries; i++) {
        *write_curr = *cache;
        cache++;
        write_curr++;
    }
    return write_curr;
}



static int
IS_BACKWARDS_JUMP_TARGET(PyCodeObject *co, _Py_CODEUNIT *curr)
{
    assert(co->_tier2_info != NULL);
    int backward_jump_count = co->_tier2_info->backward_jump_count;
    int *backward_jump_offsets = co->_tier2_info->backward_jump_offsets;
    _Py_CODEUNIT *start = _PyCode_CODE(co);
    // TODO: CHANGE TO BINARY SEARCH WHEN i > 40. For smaller values, linear search is quicker.
    for (int i = 0; i < backward_jump_count; i++) {
        if (curr == start + backward_jump_offsets[i]) {
            return 1;
        }
    }
    return 0;
}

// 1 for error, 0 for success.
static inline int
add_metadata_to_jump_2d_array(_PyTier2Info *t2_info, _PyTier2BBMetadata *meta,
    int backwards_jump_target, _PyTier2TypeContext *starting_context,
    _Py_CODEUNIT *tier1_start)
{
    // Locate where to insert the BB ID
    int backward_jump_offset_index = 0;
    bool found = false;
    for (; backward_jump_offset_index < t2_info->backward_jump_count;
        backward_jump_offset_index++) {
        if (t2_info->backward_jump_offsets[backward_jump_offset_index] ==
            backwards_jump_target) {
            found = true;
            break;
        }
    }
    assert(found);
    int jump_i = 0;
    found = false;
    for (; jump_i < MAX_BB_VERSIONS; jump_i++) {
        if (t2_info->backward_jump_target_bb_pairs[backward_jump_offset_index][jump_i].id ==
            -1) {
            t2_info->backward_jump_target_bb_pairs[backward_jump_offset_index][jump_i].id =
                meta->id;
            t2_info->backward_jump_target_bb_pairs[backward_jump_offset_index][jump_i].start_type_context = starting_context;
            t2_info->backward_jump_target_bb_pairs[backward_jump_offset_index][jump_i].tier1_start = tier1_start;
            found = true;
            break;
        }
    }
    // Out of basic blocks versions.
    if (!found) {
        return 1;
    }
    assert(found);
    return 0;
}

// This converts sequence of instructions like
// BINARY_OP (ADD)
// to
// BINARY_CHECK_INT
// BB_BRANCH
// CACHE (bb_id of the current BB << 1 | is_type_branch)
// // The BINARY_ADD then goes to the next BB
static inline _Py_CODEUNIT *
infer_BINARY_OP(
    _Py_CODEUNIT *t2_start,
    int oparg,
    bool *needs_guard,
    _Py_CODEUNIT *prev_type_guard,
    _Py_CODEUNIT raw_op,
    _Py_CODEUNIT *write_curr,
    _PyTier2TypeContext *type_context,
    int bb_id)
{
    assert(oparg == NB_ADD || oparg == NB_SUBTRACT || oparg == NB_MULTIPLY);
    bool is_first_instr = (write_curr == t2_start);
    *needs_guard = false;
    PyTypeObject *right = typenode_get_type(type_context->type_stack_ptr[-1]);
    PyTypeObject *left = typenode_get_type(type_context->type_stack_ptr[-2]);
    if (left == &PyLong_Type) {
        if (right == &PyLong_Type) {
            int opcode = oparg == NB_ADD
                ? BINARY_OP_ADD_INT_REST
                : oparg == NB_SUBTRACT
                ? BINARY_OP_SUBTRACT_INT_REST
                : oparg == NB_MULTIPLY
                ? BINARY_OP_MULTIPLY_INT_REST
                : (Py_UNREACHABLE(), 1);
            write_curr->op.code = opcode;
            write_curr++;
            type_propagate(opcode, 0, type_context, NULL);
            return write_curr;
        }
    }
    if ((left == &PyRawFloat_Type || left == &PyFloat_Type) &&
        (right == &PyRawFloat_Type || right == &PyFloat_Type)) {
        int opcode = oparg == NB_ADD
            ? BINARY_OP_ADD_FLOAT_UNBOXED
            : oparg == NB_SUBTRACT
            ? BINARY_OP_SUBTRACT_FLOAT_UNBOXED
            : oparg == NB_MULTIPLY
            ? BINARY_OP_MULTIPLY_FLOAT_UNBOXED
            : (Py_UNREACHABLE(), 1);
        if (right == &PyFloat_Type) {
            write_curr->op.code = UNBOX_FLOAT;
            write_curr->op.arg = 0;
            write_curr++;
            type_propagate(UNBOX_FLOAT, 0, type_context, NULL);
        }
        if (left == &PyFloat_Type) {
            write_curr->op.code = UNBOX_FLOAT;
            write_curr->op.arg = 1;
            write_curr++;
            type_propagate(UNBOX_FLOAT, 1, type_context, NULL);
        }
        write_curr->op.code = opcode;
        write_curr++;
        type_propagate(opcode, 0, type_context, NULL);
        return write_curr;
    }
    // Unknown, time to emit the chain of guards.
    // No type guard before this, or it's not the first in the new BB.
    // First in new BB usually indicates it's already part of a pre-existing ladder.
    if (prev_type_guard == NULL || !is_first_instr) {
        write_curr = rebox_stack(write_curr, type_context, 2);
        *needs_guard = true;
        return emit_type_guard(write_curr, BINARY_CHECK_FLOAT, 0, bb_id);
    }
    else {
        int next_guard = type_guard_ladder[
            type_guard_to_index[prev_type_guard->op.code] + 1];
        if (next_guard != -1) {
            write_curr = rebox_stack(write_curr, type_context, 2);
            *needs_guard = true;
            return emit_type_guard(write_curr, next_guard, 0, bb_id);
        }
        // End of ladder, fall through
    }
    return NULL;
}

static inline _Py_CODEUNIT *
infer_BINARY_SUBSCR(
    _Py_CODEUNIT *t2_start,
    int oparg,
    bool *needs_guard,
    _Py_CODEUNIT *prev_type_guard,
    _Py_CODEUNIT raw_op,
    _Py_CODEUNIT *write_curr,
    _PyTier2TypeContext *type_context,
    int bb_id,
    bool store)
{
    assert(oparg == NB_ADD || oparg == NB_SUBTRACT || oparg == NB_MULTIPLY);
    bool is_first_instr = (write_curr == t2_start);
    *needs_guard = false;
    PyTypeObject *sub = typenode_get_type(type_context->type_stack_ptr[-1]);
    PyTypeObject *container = typenode_get_type(type_context->type_stack_ptr[-2]);
    if (container == &PyList_Type) {
        if (sub == &PySmallInt_Type) {
            int opcode = store
                ? STORE_SUBSCR_LIST_INT_REST : BINARY_SUBSCR_LIST_INT_REST;
            write_curr->op.code = opcode;
            write_curr++;
            type_propagate(opcode, 0, type_context, NULL);
            return write_curr;
        }
    }
    // Unknown, time to emit the chain of guards.
    // No type guard before this, or it's not the first in the new BB.
    // First in new BB usually indicates it's already part of a pre-existing ladder.
    if (prev_type_guard == NULL || !is_first_instr) {
        write_curr = rebox_stack(write_curr, type_context, 2);
        *needs_guard = true;
        return emit_type_guard(write_curr, CHECK_LIST, 1, bb_id);
    }
    else {
        int next_guard = type_guard_ladder[
            type_guard_to_index[prev_type_guard->op.code] + 1];
        if (next_guard != -1) {
            write_curr = rebox_stack(write_curr, type_context, store ? 3 : 2);
            *needs_guard = true;
            return emit_type_guard(write_curr, next_guard, 1, bb_id);
        }
        // End of ladder, fall through
    }
    return NULL;
}

static inline bool
is_unboxed_type(PyTypeObject *t)
{
    return t == &PyRawFloat_Type;
}

// Detects a BB from the current instruction start to the end of the first basic block it sees.
// Then emits the instructions into the bb space.
//
// Instructions emitted depend on the type_context.
// For example, if it sees a BINARY_ADD instruction, but it knows the two operands are already of
// type PyLongObject, a BINARY_ADD_INT_REST will be emitted without an type checks.
// 
// However, if one of the operands are unknown, a logical chain of CHECK instructions will be emitted,
// and the basic block will end at the first of the chain.
// 
// Note: a BB end also includes a type guard.
_PyTier2BBMetadata *
_PyTier2_Code_DetectAndEmitBB(
    PyCodeObject *co,
    _PyTier2BBSpace *bb_space,
    _Py_CODEUNIT *prev_type_guard,
    _Py_CODEUNIT *tier1_start,
    // starting_type_context will be modified in this function,
    // do make a copy if needed before calling this function
    _PyTier2TypeContext *starting_type_context)
{
    assert(
        prev_type_guard == NULL ||
        prev_type_guard->op.code == BINARY_CHECK_INT ||
        prev_type_guard->op.code == BINARY_CHECK_FLOAT ||
        prev_type_guard->op.code == CHECK_LIST
    );
#define END() goto end;
#define JUMPBY(x) i += x + 1;
#define DISPATCH()        write_i = emit_i(write_i, specop, curr->op.arg); \
                          write_i = copy_cache_entries(write_i, curr+1, caches); \
                          i += caches; \
                          type_propagate(opcode, oparg, starting_type_context, consts); \
                          continue;

#define DISPATCH_REBOX(x) write_i = rebox_stack(write_i, starting_type_context, x); \
                          write_i = emit_i(write_i, specop, curr->op.arg); \
                          write_i = copy_cache_entries(write_i, curr+1, caches); \
                          i += caches; \
                          type_propagate(opcode, oparg, starting_type_context, consts); \
                          continue;
#define DISPATCH_GOTO() goto dispatch_opcode;
#define TYPECONST_GET_RAWTYPE(idx) Py_TYPE(PyTuple_GET_ITEM(consts, idx))
#define GET_CONST(idx) PyTuple_GET_ITEM(consts, idx)

    assert(co->_tier2_info != NULL);
    // There are only two cases that a BB ends.
    // 1. If there's a branch instruction / scope exit.
    // 2. If there's a type guard.
    bool needs_guard = 0;

    _PyTier2BBMetadata *meta = NULL;
    _PyTier2BBMetadata *temp_meta = NULL;
    _PyTier2BBMetadata *jump_end_meta = NULL;

    _PyTier2Info *t2_info = co->_tier2_info;
    PyObject *consts = co->co_consts;
    _Py_CODEUNIT *t2_start = (_Py_CODEUNIT *)(((char *)bb_space->u_code) + bb_space->water_level);
    _Py_CODEUNIT *write_i = t2_start;
    int tos = -1;

    // For handling of backwards jumps
    bool starts_with_backwards_jump_target = false;
    int backwards_jump_target_offset = -1;
    bool virtual_start = false;
    _PyTier2TypeContext *start_type_context_copy = NULL;
    _Py_CODEUNIT *virtual_tier1_start = NULL;

    // A meta-interpreter for types.
    Py_ssize_t i = (tier1_start - _PyCode_CODE(co));
    for (; i < Py_SIZE(co); i++) {
        _Py_CODEUNIT *curr = _PyCode_CODE(co) + i;
        _Py_CODEUNIT *next_instr = curr + 1;
        int specop = _Py_OPCODE(*curr);
        int opcode = _PyOpcode_Deopt[specop];
        int oparg = _Py_OPARG(*curr);
        int caches = _PyOpcode_Caches[opcode];

        // Just because an instruction requires a guard doesn't mean it's the end of a BB.
        // We need to check whether we can eliminate the guard based on the current type context.

    dispatch_opcode:
#if TYPEPROP_DEBUG
        fprintf(stderr, "offset: %Id\n", curr - _PyCode_CODE(co));
#endif
        switch (opcode) {
        case RESUME:
            opcode = specop = RESUME_QUICK;
            DISPATCH();
        case END_FOR:
            // Assert that we are the start of a BB
            assert(t2_start == write_i);
            // Though we want to emit this, we don't want to start execution from END_FOR.
            // So we tell the BB to skip over it.
            t2_start++;
            DISPATCH();
        case POP_TOP: {
            // Read-only, only for us to inspect the types. DO NOT MODIFY HERE.
            // ONLY THE TYPES PROPAGATOR SHOULD MODIFY THEIR INTERNAL VALUES.
            _Py_TYPENODE_t **type_stackptr = &starting_type_context->type_stack_ptr;
            PyTypeObject *pop = typenode_get_type(*TYPESTACK_PEEK(1));
            // Writing unboxed val to a boxed val. 
            if (is_unboxed_type(pop)) {
                opcode = specop = POP_TOP_NO_DECREF;
            }
            DISPATCH();
        }
        case COPY: {
            // Read-only, only for us to inspect the types. DO NOT MODIFY HERE.
            // ONLY THE TYPES PROPAGATOR SHOULD MODIFY THEIR INTERNAL VALUES.
            _Py_TYPENODE_t **type_stackptr = &starting_type_context->type_stack_ptr;
            PyTypeObject *pop = typenode_get_type(*TYPESTACK_PEEK(1 + (oparg - 1)));
            // Writing unboxed val to a boxed val. 
            if (is_unboxed_type(pop)) {
                opcode = specop = COPY_NO_INCREF;
            }
            DISPATCH();
        }
        case LOAD_CONST: {
            PyTypeObject *typ = TYPECONST_GET_RAWTYPE(oparg);
            if (typ == &PyFloat_Type) {
                write_i = emit_i(write_i, LOAD_CONST, curr->op.arg);
                type_propagate(LOAD_CONST, oparg, starting_type_context, consts);
                write_i->op.code = UNBOX_FLOAT;
                write_i->op.arg = 0;
                write_i++;
                type_propagate(UNBOX_FLOAT, 0, starting_type_context, consts);
                continue;
            }
            else if (typ == &PyLong_Type) {
                // We break our own rules for more efficient code here.
                // NOTE: THIS MODIFIES THE TYPE CONTEXT.
                if (_PyLong_IsNonNegativeCompact((PyLongObject *)GET_CONST(oparg))) {
                    write_i = emit_i(write_i, LOAD_CONST, curr->op.arg);

                    // Type propagate
                    _PyTier2TypeContext *type_context = starting_type_context;
                    _Py_TYPENODE_t **type_stackptr = &type_context->type_stack_ptr;
                    *type_stackptr += 1;
                    TYPE_OVERWRITE((_Py_TYPENODE_t *)_Py_TYPENODE_MAKE_ROOT((_Py_TYPENODE_t)&PySmallInt_Type), TYPESTACK_PEEK(1), true);
                    continue;
                }
            }
            DISPATCH();
        }
        case LOAD_FAST: {
            // Read-only, only for us to inspect the types. DO NOT MODIFY HERE.
            // ONLY THE TYPES PROPAGATOR SHOULD MODIFY THEIR INTERNAL VALUES.
            _Py_TYPENODE_t *type_locals = starting_type_context->type_locals;
            // Writing unboxed val to a boxed val.
            PyTypeObject *local = typenode_get_type(*TYPELOCALS_GET(oparg));
            if (is_unboxed_type(local)) {
                opcode = specop = LOAD_FAST_NO_INCREF;
            }
            else {
                if (local == &PyFloat_Type) {
                    write_i->op.code = LOAD_FAST;
                    write_i->op.arg = oparg;
                    write_i++;
                    type_propagate(LOAD_FAST,
                        oparg, starting_type_context, consts);
                    write_i->op.code = UNBOX_FLOAT;
                    write_i->op.arg = 0;
                    write_i++;
                    type_propagate(UNBOX_FLOAT, 0, starting_type_context, consts);
                    write_i->op.code = STORE_FAST_UNBOXED_BOXED;
                    write_i->op.arg = oparg;
                    write_i++;
                    type_propagate(STORE_FAST_UNBOXED_BOXED,
                        oparg, starting_type_context, consts);
                    write_i->op.code = LOAD_FAST_NO_INCREF;
                    write_i->op.arg = oparg;
                    write_i++;
                    type_propagate(LOAD_FAST_NO_INCREF,
                        oparg, starting_type_context, consts);
                    continue;
                }
                opcode = specop = LOAD_FAST;
            }
            DISPATCH();
        }
        case LOAD_FAST_CHECK: {
            // Read-only, only for us to inspect the types. DO NOT MODIFY HERE.
            // ONLY THE TYPES PROPAGATOR SHOULD MODIFY THEIR INTERNAL VALUES.
            _Py_TYPENODE_t *type_locals = starting_type_context->type_locals;
            // Writing unboxed val to a boxed val.
            PyTypeObject *local = typenode_get_type(*TYPELOCALS_GET(oparg));
            if (is_unboxed_type(local)) {
                opcode = specop = LOAD_FAST_NO_INCREF;
            }
            else {
                if (local == &PyFloat_Type) {
                    write_i->op.code = LOAD_FAST;
                    write_i->op.arg = oparg;
                    write_i++;
                    type_propagate(LOAD_FAST,
                        oparg, starting_type_context, consts);
                    write_i->op.code = UNBOX_FLOAT;
                    write_i->op.arg = 0;
                    write_i++;
                    type_propagate(UNBOX_FLOAT, 0, starting_type_context, consts);
                    write_i->op.code = STORE_FAST_UNBOXED_BOXED;
                    write_i->op.arg = oparg;
                    write_i++;
                    type_propagate(STORE_FAST_UNBOXED_BOXED,
                        oparg, starting_type_context, consts);
                    write_i->op.code = LOAD_FAST_NO_INCREF;
                    write_i->op.arg = oparg;
                    write_i++;
                    type_propagate(LOAD_FAST_NO_INCREF,
                        oparg, starting_type_context, consts);
                    continue;
                }
                opcode = specop = LOAD_FAST_CHECK;
            }
            DISPATCH();
        }
        case STORE_FAST: {
            // Read-only, only for us to inspect the types. DO NOT MODIFY HERE.
            // ONLY THE TYPES PROPAGATOR SHOULD MODIFY THEIR INTERNAL VALUES.
            _Py_TYPENODE_t *type_locals = starting_type_context->type_locals;
            _Py_TYPENODE_t **type_stackptr = &starting_type_context->type_stack_ptr;
            PyTypeObject *local = typenode_get_type(*TYPESTACK_PEEK(1));
            PyTypeObject *store = typenode_get_type(*TYPELOCALS_GET(oparg));
            // Writing unboxed val to a boxed val. 
            if (is_unboxed_type(local)) {
                if (!is_unboxed_type(store)) {
                    opcode = specop = STORE_FAST_UNBOXED_BOXED;
                }
                else {
                    opcode = specop = STORE_FAST_UNBOXED_UNBOXED;
                }
            }
            else {
                if (is_unboxed_type(store)) {
                    opcode = specop = STORE_FAST_BOXED_UNBOXED;
                }
                else {
                    opcode = specop = STORE_FAST;
                }
            }
            DISPATCH();
        }
        // Need to handle reboxing at these boundaries.
        case CALL:
            DISPATCH_REBOX(oparg + 2);
        case BUILD_MAP:
            DISPATCH_REBOX(oparg * 2);
        case BUILD_STRING:
        case BUILD_LIST:
            DISPATCH_REBOX(oparg);
        case BINARY_OP:
            if (oparg == NB_ADD || oparg == NB_SUBTRACT || oparg == NB_MULTIPLY) {
                // Add operation. Need to check if we can infer types.
                _Py_CODEUNIT *possible_next = infer_BINARY_OP(t2_start,
                    oparg, &needs_guard,
                    prev_type_guard,
                    *curr,
                    write_i, starting_type_context,
                    co->_tier2_info->bb_data_curr);
                if (possible_next == NULL) {
                    DISPATCH_REBOX(2);
                }
                write_i = possible_next;
                if (needs_guard) {
                    // Point to the same instruction, because in this BB we emit
                    // the guard.
                    // The next BB emits the instruction.
                    i--;
                    END();
                }
                i += caches;
                continue;
            }
            DISPATCH_REBOX(2);
        case BINARY_SUBSCR: {
            _Py_CODEUNIT *possible_next = infer_BINARY_SUBSCR(
                t2_start, oparg, &needs_guard,
                prev_type_guard,
                *curr,
                write_i, starting_type_context,
                co->_tier2_info->bb_data_curr, false);
            if (possible_next == NULL) {
                DISPATCH_REBOX(2);
            }
            write_i = possible_next;
            if (needs_guard) {
                // Point to the same instruction, because in this BB we emit
                // the guard.
                // The next BB emits the instruction.
                i--;
                END();
            }
            i += caches;
            continue;
        }
        case STORE_SUBSCR: {
            _Py_CODEUNIT *possible_next = infer_BINARY_SUBSCR(
                t2_start, oparg, &needs_guard,
                prev_type_guard,
                *curr,
                write_i, starting_type_context,
                co->_tier2_info->bb_data_curr, true);
            if (possible_next == NULL) {
                DISPATCH_REBOX(3);
            }
            write_i = possible_next;
            if (needs_guard) {
                // Point to the same instruction, because in this BB we emit
                // the guard.
                // The next BB emits the instruction.
                i--;
                END();
            }
            i += caches;
            continue;
        }
        case LOAD_ATTR:
        case CALL_INTRINSIC_1:
        case UNARY_NEGATIVE:
        case UNARY_NOT:
        case UNARY_INVERT:
        case GET_LEN:
        case UNPACK_SEQUENCE:
            DISPATCH_REBOX(1);
        case CALL_INTRINSIC_2:
        case BINARY_SLICE:
            DISPATCH_REBOX(2);
        case STORE_SLICE:
            DISPATCH_REBOX(4);
        default:
#if BB_DEBUG && !TYPEPROP_DEBUG
            fprintf(stderr, "offset: %Id\n", curr - _PyCode_CODE(co));
#endif
            // This should be the end of another basic block, or the start of a new.
            // Start of a new basic block, just ignore and continue.
            if (virtual_start) {
#if BB_DEBUG
                fprintf(stderr, "Emitted virtual start of basic block\n");
#endif
                starts_with_backwards_jump_target = true;
                virtual_start = false;
                start_type_context_copy = _PyTier2TypeContext_Copy(starting_type_context);
                if (start_type_context_copy == NULL) {
                    _PyTier2TypeContext_Free(starting_type_context);
                    return NULL;
                }
                goto fall_through;
            }
            if (IS_BACKWARDS_JUMP_TARGET(co, curr)) {
#if BB_DEBUG
                fprintf(stderr, "Encountered a backward jump target\n");
#endif
#if TYPEPROP_DEBUG
                print_typestack(starting_type_context);
#endif
                // Else, create a virtual end to the basic block.
                // But generate the block after that so it can fall through.
                i--;
                _PyTier2TypeContext *type_context_copy = _PyTier2TypeContext_Copy(starting_type_context);
                if (type_context_copy == NULL) {
                    return NULL;
                }
                meta = _PyTier2_AllocateBBMetaData(co,
                    t2_start, _PyCode_CODE(co) + i, type_context_copy);
                if (meta == NULL) {
                    _PyTier2TypeContext_Free(type_context_copy);
                    return NULL;
                }
                bb_space->water_level += (write_i - t2_start) * sizeof(_Py_CODEUNIT);
                // Reset all our values
                t2_start = write_i;
                i++;
                virtual_tier1_start = _PyCode_CODE(co) + i;
                backwards_jump_target_offset = (int)(curr - _PyCode_CODE(co));
                virtual_start = true;

                if (opcode == EXTENDED_ARG) {
                    // Note: EXTENDED_ARG could be a jump target!!!!!
                    specop = next_instr->op.code;
                    opcode = _PyOpcode_Deopt[specop];
                    caches = _PyOpcode_Caches[opcode];
                    oparg = oparg << 8 | next_instr->op.arg;
                    curr++;
                    next_instr++;
                    i += 1;
                    DISPATCH_GOTO();
                }
                // Don't change opcode or oparg, let us handle it again.
                DISPATCH_GOTO();
            }
        fall_through:
            // These are definitely the end of a basic block.
            if (IS_SCOPE_EXIT_OPCODE(opcode)) {
                // Emit the scope exit instruction.
                write_i = emit_scope_exit(write_i, *curr, starting_type_context);
                END();
            }

            // Jumps may be the end of a basic block if they are conditional (a branch).
            if (IS_JUMP_OPCODE(opcode)) {
                // Unconditional forward jump... continue with the BB without writing the jump.
                if (opcode == JUMP_FORWARD) {
                    // JUMP offset (oparg) + current instruction + cache entries
                    JUMPBY(oparg);
                    continue;
                }
                // Get the BB ID without incrementing it.
                // AllocateBBMetaData will increment.
                write_i = emit_logical_branch(starting_type_context, write_i, *curr,
                    co->_tier2_info->bb_data_curr, oparg);
                i += caches;
                END();
            }
            if (opcode == EXTENDED_ARG) {
                // Note: EXTENDED_ARG could be a jump target!!!!!
                specop = next_instr->op.code;
                opcode = _PyOpcode_Deopt[specop];
                caches = _PyOpcode_Caches[opcode];
                oparg = oparg << 8 | next_instr->op.arg;
                curr++;
                next_instr++;
                i += 1;
                DISPATCH_GOTO();
            }
            DISPATCH();
        }

    }
end:
    // Create the tier 2 BB

    temp_meta = _PyTier2_AllocateBBMetaData(co, t2_start,
        // + 1 because we want to start with the NEXT instruction for the scan
        _PyCode_CODE(co) + i + 1, starting_type_context);
    if (temp_meta == NULL) {
        _PyTier2TypeContext_Free(starting_type_context);
        return NULL;
    }
    // We need to return the first block to enter into. If there is already a block generated
    // before us, then we use that instead of the most recent block.
    if (meta == NULL) {
        meta = temp_meta;
    }
    if (starts_with_backwards_jump_target) {
        // Add the basic block to the jump ids
        assert(start_type_context_copy != NULL);
        assert(virtual_tier1_start != NULL);
        if (add_metadata_to_jump_2d_array(t2_info, temp_meta,
            backwards_jump_target_offset, start_type_context_copy,
            virtual_tier1_start) < 0) {
            PyMem_Free(meta);
            if (meta != temp_meta) {
                PyMem_Free(temp_meta);
            }
            _PyTier2TypeContext_Free(starting_type_context);
            return NULL;
        }
    }
    // Tell BB space the number of bytes we wrote.
    // -1 becaues write_i points to the instruction AFTER the end
    bb_space->water_level += (write_i - t2_start) * sizeof(_Py_CODEUNIT);
#if BB_DEBUG
    fprintf(stderr, "Generated BB T2 Start: %p, T1 offset: %zu\n", meta->tier2_start,
        meta->tier1_end - _PyCode_CODE(co));
#endif
    return meta;

}


////////// _PyTier2Info FUNCTIONS

static int
compare_ints(const void *a, const void *b)
{
    return *(int *)a - *(int *)b;
}

static int
allocate_jump_offset_2d_array(int backwards_jump_count,
    _PyTier2BBStartTypeContextTriplet **backward_jump_target_bb_pairs)
{
    int done = 0;
    for (int i = 0; i < backwards_jump_count; i++) {
        _PyTier2BBStartTypeContextTriplet *pair =
            PyMem_Malloc(sizeof(_PyTier2BBStartTypeContextTriplet) * MAX_BB_VERSIONS);
        if (pair == NULL) {
            goto error;
        }
        for (int i = 0; i < MAX_BB_VERSIONS; i++) {
            pair[i].id = -1;
        }
        done++;
        backward_jump_target_bb_pairs[i] = pair;
    }
    return 0;
error:
    for (int i = 0; i < done; i++) {
        PyMem_Free(backward_jump_target_bb_pairs[i]);
    }
    return 1;
}

// Returns 1 on error, 0 on success. Populates the backwards jump target offset
// array for a code object..
static int
_PyCode_Tier2FillJumpTargets(PyCodeObject *co)
{
    assert(co->_tier2_info != NULL);
    // Count all the backwards jump targets.
    Py_ssize_t backwards_jump_count = 0;
    for (Py_ssize_t i = 0; i < Py_SIZE(co); i++) {
        _Py_CODEUNIT *instr_ptr = _PyCode_CODE(co) + i;
        _Py_CODEUNIT instr = *instr_ptr;
        int opcode = _PyOpcode_Deopt[_Py_OPCODE(instr)];
        backwards_jump_count += IS_JUMP_BACKWARDS_OPCODE(opcode);
        i += _PyOpcode_Caches[opcode];
    }

    // Impossibly big.
    if (backwards_jump_count != (int)backwards_jump_count) {
        return 1;
    }

    // Find all the jump target instructions
    // Don't allocate a zero byte space as this may be undefined behavior.
    if (backwards_jump_count == 0) {
        co->_tier2_info->backward_jump_offsets = NULL;
        // Successful (no jump targets)!
        co->_tier2_info->backward_jump_count = (int)backwards_jump_count;
        return 0;
    }
    int *backward_jump_offsets = PyMem_Malloc(backwards_jump_count * sizeof(int));
    if (backward_jump_offsets == NULL) {
        return 1;
    }
    _PyTier2BBStartTypeContextTriplet **backward_jump_target_bb_pairs =
        PyMem_Malloc(backwards_jump_count * sizeof(_PyTier2BBStartTypeContextTriplet *));
    if (backward_jump_target_bb_pairs == NULL) {
        PyMem_Free(backward_jump_offsets);
        return 1;
    }
    if (allocate_jump_offset_2d_array((int)backwards_jump_count,
        backward_jump_target_bb_pairs)) {
        PyMem_Free(backward_jump_offsets);
        PyMem_Free(backward_jump_target_bb_pairs);
        return 1;
    }

    _Py_CODEUNIT *start = _PyCode_CODE(co);
    int curr_i = 0;
    int oparg = 0;
    for (Py_ssize_t i = 0; i < Py_SIZE(co); i++) {
        _Py_CODEUNIT *curr = start + i;
        int opcode = _PyOpcode_Deopt[curr->op.code];
        oparg = curr->op.arg;
    dispatch_same_oparg:
        if (IS_JUMP_BACKWARDS_OPCODE(opcode)) {
            // + 1 because it's calculated from nextinstr (see JUMPBY in ceval.c)
            _Py_CODEUNIT *target = curr + 1 - oparg;
#if BB_DEBUG
            fprintf(stderr, "jump target opcode is %d\n", _Py_OPCODE(*target));
#endif
            // (in terms of offset from start of co_code_adaptive)
            backward_jump_offsets[curr_i] = (int)(target - start);
            curr_i++;
        }
        else if (opcode == EXTENDED_ARG) {
            oparg = oparg << 8 | (curr+1)->op.arg;
            opcode = _PyOpcode_Deopt[(curr+1)->op.code];
            i++;
            curr++;
            goto dispatch_same_oparg;
        }
        i += _PyOpcode_Caches[opcode];
    }
    assert(curr_i == backwards_jump_count);
    qsort(backward_jump_offsets, backwards_jump_count,
        sizeof(int), compare_ints);
    // Deduplicate
    for (int i = 0; i < backwards_jump_count - 1; i++) {
        for (int x = i + 1; x < backwards_jump_count; x++) {
            if (backward_jump_offsets[i] == backward_jump_offsets[x]) {
                backward_jump_offsets[x] = -1;
            }
        }
    }
    qsort(backward_jump_offsets, backwards_jump_count,
        sizeof(int), compare_ints);
#if BB_DEBUG
    fprintf(stderr, "BACKWARD JUMP COUNT : %Id\n", backwards_jump_count);
    fprintf(stderr, "BACKWARD JUMP TARGET OFFSETS (FROM START OF CODE): ");
    for (Py_ssize_t i = 0; i < backwards_jump_count; i++) {
        fprintf(stderr, "%d ,", backward_jump_offsets[i]);
    }
    fprintf(stderr, "\n");
#endif
    co->_tier2_info->backward_jump_count = (int)backwards_jump_count;
    co->_tier2_info->backward_jump_offsets = backward_jump_offsets;
    co->_tier2_info->backward_jump_target_bb_pairs = backward_jump_target_bb_pairs;
    return 0;
}



static _PyTier2Info *
_PyTier2Info_Initialize(PyCodeObject *co)
{
    assert(co->_tier2_info == NULL);
    _PyTier2Info *t2_info = PyMem_Malloc(sizeof(_PyTier2Info));
    if (t2_info == NULL) {
        return NULL;
    }

    t2_info->backward_jump_count = 0;
    t2_info->backward_jump_offsets = NULL;

    // Initialize BB data array
    t2_info->bb_data_len = 0;
    t2_info->bb_data = NULL;
    t2_info->bb_data_curr = 0;
    Py_ssize_t bb_data_len = (Py_SIZE(co) / 5 + 1);
    assert((int)bb_data_len == bb_data_len);
    _PyTier2BBMetadata **bb_data = PyMem_Calloc(bb_data_len, sizeof(_PyTier2BBMetadata *));
    if (bb_data == NULL) {
        PyMem_Free(t2_info);
        return NULL;
    }
    t2_info->bb_data_len = (int)bb_data_len;
    t2_info->bb_data = bb_data;
    co->_tier2_info = t2_info;

    return t2_info;
}

////////// OVERALL TIER2 FUNCTIONS


// We use simple heuristics to determine if there are operations
// we can optimize in this.
// Specifically, we are looking for the presence of PEP 659
// specialized forms of bytecode, because this indicates
// that it's a known form.
// ADD MORE HERE AS WE GO ALONG.
static inline int
IS_OPTIMIZABLE_OPCODE(int opcode, int oparg)
{
    switch (_PyOpcode_Deopt[opcode]) {
    case BINARY_OP:
        switch (oparg) {
        case NB_SUBTRACT:
        case NB_MULTIPLY:
        case NB_ADD:
            // We want a specialised form, not the generic BINARY_OP.
            return opcode != _PyOpcode_Deopt[opcode];
        default:
            return 0;
        }
    default:
        return 0;
    }
}

static inline void
replace_resume_and_jump_backwards(PyCodeObject *co)
{
    for (Py_ssize_t i = 0; i < Py_SIZE(co); i++) {
        _Py_CODEUNIT *instr_ptr = _PyCode_CODE(co) + i;
        _Py_CODEUNIT instr = *instr_ptr;
        int opcode = _PyOpcode_Deopt[_Py_OPCODE(instr)];
        int oparg = _Py_OPARG(instr);
        switch (opcode) {
        case RESUME:
            _py_set_opcode(instr_ptr, RESUME_QUICK);
            break;
        case JUMP_BACKWARD:
            _py_set_opcode(instr_ptr, JUMP_BACKWARD_QUICK);
            break;
        }
        i += _PyOpcode_Caches[opcode];
    }
}

// 1. Initialize whatever we need.
// 2. Create the entry BB.
// 3. Jump into that BB.
static _Py_CODEUNIT *
_PyCode_Tier2Initialize(_PyInterpreterFrame *frame, _Py_CODEUNIT *next_instr)
{
    assert(_Py_OPCODE(*(next_instr - 1)) == RESUME);
    PyCodeObject *co = frame->f_code;
    // Replace all the RESUME and JUMP_BACKWARDS so that it doesn't waste time again.
    replace_resume_and_jump_backwards(co);
    // Impossibly big.
    if ((int)Py_SIZE(co) != Py_SIZE(co)) {
        return NULL;
    }
    // First check for forbidden opcodes that we currently can't handle.
    int optimizable = 0;
    for (Py_ssize_t curr = 0; curr < Py_SIZE(co); curr++) {
        _Py_CODEUNIT *curr_instr = _PyCode_CODE(co) + curr;
        int deopt = _PyOpcode_Deopt[_Py_OPCODE(*curr_instr)];
        int next = curr < Py_SIZE(co) - 1
            ? _PyOpcode_Deopt[(curr_instr + 1)->op.code]
            : 255;
        if (IS_FORBIDDEN_OPCODE(deopt, next)) {
#if BB_DEBUG
#ifdef Py_DEBUG
            fprintf(stderr, "FORBIDDEN OPCODE %s\n", _PyOpcode_OpName[_Py_OPCODE(*curr_instr)]);
#else
            fprintf(stderr, "FORBIDDEN OPCODE %d\n", _Py_OPCODE(*curr_instr));
#endif
#endif
            return NULL;
        }
        optimizable |= IS_OPTIMIZABLE_OPCODE(_Py_OPCODE(*curr_instr), _Py_OPARG(*curr_instr));
        // Skip the cache entries
        curr += _PyOpcode_Caches[deopt];
    }

    if (!optimizable) {
#if BB_DEBUG
        fprintf(stderr, "NOT OPTIMIZABLE\n");
#endif
        return NULL;
    }

    _PyTier2Info *t2_info = _PyTier2Info_Initialize(co);
    if (t2_info == NULL) {
        return NULL;
    }

#if BB_DEBUG
    fprintf(stderr, "INITIALIZING\n");
#endif

    Py_ssize_t space_to_alloc = (_PyCode_NBYTES(co)) * OVERALLOCATE_FACTOR;

    _PyTier2BBSpace *bb_space = _PyTier2_CreateBBSpace(space_to_alloc);
    if (bb_space == NULL) {
        PyMem_Free(t2_info);
        return NULL;
    }
    if (_PyCode_Tier2FillJumpTargets(co)) {
        goto cleanup;
    }

    t2_info->_bb_space = bb_space;

    _PyTier2TypeContext *type_context = initialize_type_context(co);
    if (type_context == NULL) {
        goto cleanup;
    }
    _PyTier2BBMetadata *meta = _PyTier2_Code_DetectAndEmitBB(
        co, bb_space, NULL,
        _PyCode_CODE(co), type_context);
    if (meta == NULL) {
        _PyTier2TypeContext_Free(type_context);
        goto cleanup;
    }
#if BB_DEBUG
    fprintf(stderr, "ENTRY BB END IS: %d\n", (int)(meta->tier1_end - _PyCode_CODE(co)));
#endif


    t2_info->_entry_bb = meta;

    // SET THE FRAME INFO
    frame->prev_instr = meta->tier2_start - 1;
    // Set the starting instruction to the entry BB.
    // frame->prev_instr = bb_ptr->u_code - 1;
    return meta->tier2_start;

cleanup:
    PyMem_Free(t2_info);
    PyMem_Free(bb_space);
    return NULL;
}

////////// CEVAL FUNCTIONS

// Tier 2 warmup counter
_Py_CODEUNIT *
_PyCode_Tier2Warmup(_PyInterpreterFrame *frame, _Py_CODEUNIT *next_instr)
{
    PyCodeObject *code = frame->f_code;
    if (code->_tier2_warmup != 0) {
        code->_tier2_warmup++;
        if (code->_tier2_warmup >= 0) {
            assert(code->_tier2_info == NULL);
            // If it fails, due to lack of memory or whatever,
            // just fall back to the tier 1 interpreter.
            _Py_CODEUNIT *next = _PyCode_Tier2Initialize(frame, next_instr);
            if (next != NULL) {
                return next;
            }
        }
    }
    return next_instr;
}

_PyTier2BBMetadata *
_PyTier2_GenerateNextBBMetaWithTypeContext(
    _PyInterpreterFrame *frame,
    uint16_t bb_id_tagged,
    _Py_CODEUNIT *curr_executing_instr,
    int jumpby,
    _Py_CODEUNIT **tier1_fallback,
    char bb_flag,
    _PyTier2TypeContext *type_context_copy,
    _Py_CODEUNIT *custom_tier1_end)
{
    PyCodeObject *co = frame->f_code;
    assert(co->_tier2_info != NULL);
    assert(BB_ID(bb_id_tagged) <= co->_tier2_info->bb_data_curr);
    _PyTier2BBMetadata *meta = co->_tier2_info->bb_data[BB_ID(bb_id_tagged)];
    _Py_CODEUNIT *tier1_end = custom_tier1_end == NULL
        ? meta->tier1_end + jumpby : custom_tier1_end;
    *tier1_fallback = tier1_end;
    // Be a pessimist and assume we need to write the entire rest of code into the BB.
    // The size of the BB generated will definitely be equal to or smaller than this.
    _PyTier2BBSpace *space = _PyTier2_BBSpaceCheckAndReallocIfNeeded(
        frame->f_code,
        _PyCode_NBYTES(co) -
        (tier1_end - _PyCode_CODE(co)) * sizeof(_Py_CODEUNIT));
    if (space == NULL) {
        // DEOPTIMIZE TO TIER 1?
        return NULL;
    }

    int n_required_pop = BB_TEST_GET_N_REQUIRES_POP(bb_flag);
    if (n_required_pop) {
        __type_stack_shrink(&(type_context_copy->type_stack_ptr), n_required_pop);
    }
    // For type branches, they directly precede the bb branch instruction
    _Py_CODEUNIT *prev_type_guard = BB_IS_TYPE_BRANCH(bb_id_tagged)
        ? curr_executing_instr - 1 : NULL;
    if (BB_TEST_IS_SUCCESSOR(bb_flag) && prev_type_guard != NULL) {
        // Propagate the type guard information.
#if TYPEPROP_DEBUG && defined(Py_DEBUG)
        fprintf(stderr,
            "  [-] Previous predicate BB ended with a type guard: %s\n",
            _PyOpcode_OpName[prev_type_guard->op.code]);
#endif
        type_propagate(prev_type_guard->op.code,
            prev_type_guard->op.arg, type_context_copy, NULL);
    }
    _PyTier2BBMetadata *metadata = _PyTier2_Code_DetectAndEmitBB(
        frame->f_code, space,
        prev_type_guard,
        tier1_end,
        type_context_copy);
    if (metadata == NULL) {
        _PyTier2TypeContext_Free(type_context_copy);
        return NULL;
    }
    return metadata;
}

// Lazily generates successive BBs when required.
// The first basic block created will always be directly after the current tier 2 code.
// The second basic block created will always require a jump.
_PyTier2BBMetadata *
_PyTier2_GenerateNextBBMeta(
    _PyInterpreterFrame *frame,
    uint16_t bb_id_tagged,
    _Py_CODEUNIT *curr_executing_instr,
    int jumpby,
    _Py_CODEUNIT **tier1_fallback,
    char bb_flag)
{
    _PyTier2BBMetadata *meta = frame->f_code->_tier2_info->bb_data[BB_ID(bb_id_tagged)];

    // Get type_context of previous BB
    _PyTier2TypeContext *type_context = meta->type_context;
    // Make a copy of the type context
    _PyTier2TypeContext *type_context_copy = _PyTier2TypeContext_Copy(type_context);
    if (type_context_copy == NULL) {
        return NULL;
    }

    _PyTier2BBMetadata *next = _PyTier2_GenerateNextBBMetaWithTypeContext(
        frame,
        bb_id_tagged,
        curr_executing_instr,
        jumpby,
        tier1_fallback,
        bb_flag,
        type_context_copy,
        NULL
    );

    if (next == NULL) {
        PyMem_Free(type_context_copy);
        return NULL;
    }
    return next;
}

_Py_CODEUNIT *
_PyTier2_GenerateNextBB(
    _PyInterpreterFrame *frame,
    uint16_t bb_id_tagged,
    _Py_CODEUNIT *curr_executing_instr,
    int jumpby,
    _Py_CODEUNIT **tier1_fallback,
    char bb_flag)
{
    _PyTier2BBMetadata *metadata = _PyTier2_GenerateNextBBMeta(
        frame,
        bb_id_tagged,
        curr_executing_instr,
        jumpby,
        tier1_fallback,
        bb_flag);
    if (metadata == NULL) {
        return NULL;
    }
    return metadata->tier2_start;
}

// Calculates the difference between two type contexts.
// A positive number indicating the distance is returned.
// Incompatible type contexts return INT_MAX.
static int
diff_typecontext(_PyTier2TypeContext *ctx1, _PyTier2TypeContext *ctx2)
{
    assert(ctx1 != NULL);
    assert(ctx2 != NULL);
#if BB_DEBUG
    fprintf(stderr, "  [*] Diffing type contexts\n");
    static void print_typestack(const _PyTier2TypeContext * type_context);
    print_typestack(ctx1);
    print_typestack(ctx2);
#endif
    assert(ctx1->type_locals_len == ctx2->type_locals_len);
    assert(ctx1->type_stack_len == ctx2->type_stack_len);
    int stack_elems1 = (int)(ctx1->type_stack_ptr - ctx1->type_stack);
    int stack_elems2 = (int)(ctx2->type_stack_ptr - ctx2->type_stack);
    assert(stack_elems1 == stack_elems2);

    int diff = 0;
    // Check the difference in the type locals
    for (int i = 0; i < ctx1->type_locals_len; i++) {
        PyTypeObject *a = typenode_get_type(ctx1->type_locals[i]);
        PyTypeObject *b = typenode_get_type(ctx2->type_locals[i]);
        // We allow type widening but not narrowing or conversion/casts.
        // 1. Int -> Int (bueno, diff + 0)
        // 2. Int -> Unknown/NULL (bueno, diff + 1)
        // 3. Unknown -> Int (no bueno)
        // 4. Int -> Float (no bueno)
        // 5. Unboxed type -> Unknown/Boxed type (no bueno)

        // Case 3. Widening operation.
        if (a == NULL && b != NULL) {
            return INT_MAX;
        }

        // Case 4. Incompatible type conversion.
        if (a != b && b != NULL) {
            return INT_MAX;
        }

        // Case 5. Boxed to unboxed conversion.
        if (is_unboxed_type(a) && a != b) {
            return INT_MAX;
        }

        // Case 1 and 2. Diff increases if 2.
        diff += (a != b);
    }

    // Check the difference in the type stack.
    for (int i = 0; i < stack_elems1; i++) {
        // Exact same as above.
        PyTypeObject *a = typenode_get_type(ctx1->type_stack[i]);
        PyTypeObject *b = typenode_get_type(ctx2->type_stack[i]);

        if (a == NULL && b != NULL) {
            return INT_MAX;
        }

        if (a != b && b != NULL) {
            return INT_MAX;
        }

        // Case 5. Boxed to unboxed conversion.
        if (is_unboxed_type(a) && a != b) {
            return INT_MAX;
        }

        diff += (a != b);
    }
    return diff;
}

_Py_CODEUNIT *
_PyTier2_LocateJumpBackwardsBB(_PyInterpreterFrame *frame, uint16_t bb_id_tagged, int jumpby,
    _Py_CODEUNIT **tier1_fallback,
    _Py_CODEUNIT *curr, int stacklevel)
{
    PyCodeObject *co = frame->f_code;
    assert(co->_tier2_info != NULL);
    assert(BB_ID(bb_id_tagged) <= co->_tier2_info->bb_data_curr);
    _PyTier2BBMetadata *meta = co->_tier2_info->bb_data[BB_ID(bb_id_tagged)];
#ifdef Py_DEBUG
    // We assert that there are as many items on the operand stack as there are on the
    // saved type stack.
    Py_ssize_t typestack_level = meta->type_context->type_stack_ptr - meta->type_context->type_stack;
    assert(typestack_level == stacklevel);
#endif
    // The jump target
    _Py_CODEUNIT *tier1_jump_target = meta->tier1_end + jumpby;
    *tier1_fallback = tier1_jump_target;
    // Be a pessimist and assume we need to write the entire rest of code into the BB.
    // The size of the BB generated will definitely be equal to or smaller than this.
    _PyTier2BBSpace *space = _PyTier2_BBSpaceCheckAndReallocIfNeeded(
        frame->f_code,
        _PyCode_NBYTES(co) -
        (tier1_jump_target - _PyCode_CODE(co)) * sizeof(_Py_CODEUNIT));
    if (space == NULL) {
        // DEOPTIMIZE TO TIER 1?
        return NULL;
    }

    // Get type_context of previous BB
    _PyTier2TypeContext *curr_type_context = meta->type_context;
    // Now, find the matching BB
    _PyTier2Info *t2_info = co->_tier2_info;
    int jump_offset = (int)(tier1_jump_target - _PyCode_CODE(co));
    int matching_bb_id = -1;
    int candidate_bb_id = -1;
    int min_diff = INT_MAX;
    int jump_offset_id = -1;
    _Py_CODEUNIT *candidate_bb_tier1_start = NULL;

#if BB_DEBUG
    fprintf(stderr, "finding jump target: %d\n", jump_offset);
#endif
    for (int i = 0; i < t2_info->backward_jump_count; i++) {
#if BB_DEBUG
        fprintf(stderr, "jump offset checked: %d\n", t2_info->backward_jump_offsets[i]);
#endif
        if (t2_info->backward_jump_offsets[i] == jump_offset) {
            jump_offset_id = i;
            for (int x = 0; x < MAX_BB_VERSIONS; x++) {
                int target_bb_id = t2_info->backward_jump_target_bb_pairs[i][x].id;
                if (target_bb_id >= 0) {
                    candidate_bb_id = target_bb_id;
                    candidate_bb_tier1_start = t2_info->backward_jump_target_bb_pairs[i][x].tier1_start;
#if BB_DEBUG
                    fprintf(stderr, "candidate jump target BB ID: %d\n",
                        candidate_bb_id);
#endif
                    int diff = diff_typecontext(curr_type_context,
                        t2_info->backward_jump_target_bb_pairs[i][x].start_type_context);
                    if (diff < min_diff) {
                        min_diff = diff;
                        matching_bb_id = target_bb_id;
                    }
                }
            }
            break;
        }
    }
    assert(jump_offset_id >= 0);
    assert(candidate_bb_id >= 0);
    assert(candidate_bb_tier1_start != NULL);
    // We couldn't find a matching BB to jump to. Time to generate our own.
    // This also requires rewriting our backwards jump to a forward jump later.
    if (matching_bb_id == -1) {
        // We should use the type context occuring at the end of the loop.
        _PyTier2TypeContext *copied = _PyTier2TypeContext_Copy(curr_type_context);
        if (copied == NULL) {
            return NULL;
        }
        _PyTier2TypeContext *second_copy = _PyTier2TypeContext_Copy(curr_type_context);
        if (second_copy == NULL) {
            return NULL;
        }
        _PyTier2BBMetadata *meta = _PyTier2_GenerateNextBBMetaWithTypeContext(
            frame, MAKE_TAGGED_BB_ID(candidate_bb_id, 0),
            NULL,
             0,
            tier1_fallback,
            0,
            copied,
            candidate_bb_tier1_start);
        if (meta == NULL) {
            _PyTier2TypeContext_Free(copied);
            _PyTier2TypeContext_Free(second_copy);
            return NULL;
        }
        // Store the metadata in the jump ids.
        assert(t2_info->backward_jump_offsets[jump_offset_id] == jump_offset);
        bool found = false;
        for (int x = 0; x < MAX_BB_VERSIONS; x++) {
            int target_bb_id = t2_info->backward_jump_target_bb_pairs[jump_offset_id][x].id;
            // Write to an available space
            if (target_bb_id < 0) {
                t2_info->backward_jump_target_bb_pairs[jump_offset_id][x].id = meta->id;
                t2_info->backward_jump_target_bb_pairs[jump_offset_id][x].start_type_context = second_copy;
                t2_info->backward_jump_target_bb_pairs[jump_offset_id][x].tier1_start = candidate_bb_tier1_start;
                found = true;
                break;
            }
        }
        assert(found);
        return meta->tier2_start;
    }
    assert(matching_bb_id >= 0);
    assert(matching_bb_id <= t2_info->bb_data_curr);
#if BB_DEBUG
    fprintf(stderr, "Found jump target BB ID: %d\n", matching_bb_id);
#endif
    _PyTier2BBMetadata *target_metadata = t2_info->bb_data[matching_bb_id];
    return target_metadata->tier2_start;
}

/*
At generation of the second outgoing edge (basic block), the instructions look like this:

BB_TEST_POP_IF_TRUE
BB_BRANCH_IF_FLAG_SET
CACHE

Since both edges are now generated, we want to rewrite it to:

BB_TEST_POP_IF_TRUE
BB_JUMP_IF_FLAG_SET
CACHE (will be converted to EXTENDED_ARGS if we need a bigger jump)

Backwards jumps are handled by another function.
*/

void
_PyTier2_RewriteForwardJump(_Py_CODEUNIT *bb_branch, _Py_CODEUNIT *target)
{
    int branch = _Py_OPCODE(*bb_branch);
    assert(branch == BB_BRANCH_IF_FLAG_SET ||
        branch == BB_BRANCH_IF_FLAG_UNSET);
    _Py_CODEUNIT *write_curr = bb_branch - 1;
    // -1 because the PC is auto incremented
    int oparg = (int)(target - bb_branch - 1);
    assert(oparg > 0);
    bool requires_extended = oparg > 0xFF;
    assert(oparg <= 0xFFFF);
    if (requires_extended) {
        _py_set_opcode(write_curr, EXTENDED_ARG);
        write_curr->op.arg = (oparg >> 8) & 0xFF;
        write_curr++;
    }
    else {
        _py_set_opcode(write_curr, NOP);
        write_curr++;
    }
    _py_set_opcode(write_curr,
        branch == BB_BRANCH_IF_FLAG_SET ? BB_JUMP_IF_FLAG_SET : BB_JUMP_IF_FLAG_UNSET);
    write_curr->op.arg = oparg & 0xFF;
    write_curr++;
}


/*
Before:

EXTENDED_ARG/NOP
BB_JUMP_BACKWARD_LAZY
CACHE


After:

EXTENDED_ARG (if needed, else NOP)
JUMP_BACKWARD_QUICK
END_FOR
*/

void
_PyTier2_RewriteBackwardJump(_Py_CODEUNIT *jump_backward_lazy, _Py_CODEUNIT *target)
{
    _Py_CODEUNIT *write_curr = jump_backward_lazy - 1;
    _Py_CODEUNIT *prev = jump_backward_lazy - 1;
    assert(_Py_OPCODE(*jump_backward_lazy) == BB_JUMP_BACKWARD_LAZY);
    assert(_Py_OPCODE(*prev) == EXTENDED_ARG ||
        _Py_OPCODE(*prev) == NOP);

    // +1 because we increment the PC before JUMPBY
    int oparg = (int)(target - (jump_backward_lazy + 1));
    assert(oparg != 0);
    // Is backwards jump.
    bool is_backwards_jump = oparg < 0;
    if (is_backwards_jump) {
        oparg = -oparg;
    }
    assert(oparg > 0);
    assert(oparg <= 0xFFFF);

    bool requires_extended = oparg > 0xFF;
    if (requires_extended) {
        _py_set_opcode(write_curr, EXTENDED_ARG);
        write_curr->op.arg = (oparg >> 8) & 0xFF;
        write_curr++;
    }
    else {
        _py_set_opcode(write_curr, NOP);
        write_curr++;
    }
    _py_set_opcode(write_curr, is_backwards_jump
        ? JUMP_BACKWARD_QUICK
        : JUMP_FORWARD);
    write_curr->op.arg = oparg & 0xFF;
    write_curr++;
    _py_set_opcode(write_curr, END_FOR);
    write_curr++;
    return;
}

#undef TYPESTACK_PEEK
#undef TYPESTACK_POKE
#undef TYPELOCALS_SET
#undef TYPELOCALS_GET
#undef TYPE_SET
#undef TYPE_OVERWRITE
#undef GET_CONST
