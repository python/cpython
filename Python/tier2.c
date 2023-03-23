#include "Python.h"
#include "stdlib.h"
#include "pycore_code.h"
#include "pycore_frame.h"
#include "pycore_opcode.h"
#include "pycore_pystate.h"
#include "stdbool.h"

#include "opcode.h"

#define BB_DEBUG 1
#define TYPEPROP_DEBUG 1
// Max typed version basic blocks per basic block
#define MAX_BB_VERSIONS 5
// Number of potential extra instructions at end of a BB, for branch or cleanup purposes.
#define BB_EPILOG 0


/* Dummy types used by the types propagator */
PyTypeObject PyRawFloat_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "rawfloat",
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
#if TYPEPROP_DEBUG
                int offset_stack = (int)(parent - type_context->type_stack);
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
#if TYPEPROP_DEBUG
                int offset_stack = (int)(parent - type_context->type_stack);
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

static void
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

static void
__type_stack_shrink(_PyTier2TypeContext *type_context, _Py_TYPENODE_t **type_stackptr, int idx)
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

#define STACK_GROW(idx)             *type_stackptr += (idx)

// Stack shrinking has to NULL the nodes
#define STACK_SHRINK(idx)           __type_stack_shrink(type_context, type_stackptr, (idx))

#if TYPEPROP_DEBUG
    fprintf(stderr, "  [-] Type stack bef: %llu\n", (uint64_t)(*type_stackptr - type_stack));
#ifdef Py_DEBUG
    fprintf(stderr, "  [-] Type propagating across: %s : %d\n", _PyOpcode_OpName[opcode], oparg);
#endif
#endif

    switch (opcode) {
#include "tier2_typepropagator.c.h"
    default:
        fprintf(stderr, "Unsupported opcode in type propagator: %d\n", opcode);
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
        fprintf(stderr, "Allocating new BB of size %lld\n", new_size);
#endif
        // @TODO We can't Realloc, we actually need to do the linked list method.
        _PyTier2BBSpace *new_space = PyMem_Realloc(curr, new_size);
        if (new_space == NULL) {
            return NULL;
        }
        co->_tier2_info->_bb_space = new_space;
        new_space->max_capacity = new_size;
        return new_space;
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

static int
update_backward_jump_target_bb_ids(PyCodeObject *co, _PyTier2BBMetadata *metadata)
{
    assert(co->_tier2_info != NULL);

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
    case JUMP_IF_FALSE_OR_POP:
    case JUMP_IF_TRUE_OR_POP:
        // These two tend to be after a COMPARE_AND_BRANCH.
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
    return opcode == COMPARE_OP || opcode == COMPARE_AND_BRANCH;
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
IS_FORBIDDEN_OPCODE(int opcode)
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
        // Too large arguments, we can handle this, just
        // increases complexity
    case EXTENDED_ARG:
        return 1;

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

static inline void
write_bb_id(_PyBBBranchCache *cache, int bb_id, bool is_type_guard) {
    assert((uint16_t)(bb_id) == bb_id);
    uint16_t bb_id16 = (uint16_t)bb_id;
    // Make sure MSB is unset, because we need to shift it.
    assert((bb_id16 & 0x8000) == 0);
    bb_id16 <<= 1;
    bb_id16 |= is_type_guard;
    cache->bb_id_tagged = bb_id16;
}

#define BB_ID(bb_id_raw) (bb_id_raw >> 1)
#define BB_IS_TYPE_BRANCH(bb_id_raw) (bb_id_raw & 1)


// The order/hierarchy to emit type guards
// NEED TO ADD TO THIS EVERY TIME WE ADD A NEW ONE.
static int type_guard_ladder[256] = {
    -1,
    BINARY_CHECK_INT,
    BINARY_CHECK_FLOAT,
    -1,
};

// Type guard to index in the ladder.
// KEEP IN SYNC WITH INDEX IN type_guard_ladder
static int type_guard_to_index[256] = {
    [BINARY_CHECK_INT] = 1,
    [BINARY_CHECK_FLOAT] = 2,
};


static inline _Py_CODEUNIT *
emit_type_guard(_Py_CODEUNIT *write_curr, int guard_opcode, int bb_id)
{
#if BB_DEBUG && defined(Py_DEBUG)
    fprintf(stderr, "emitted type guard %p %s\n", write_curr,
        _PyOpcode_OpName[guard_opcode]);
#endif
    write_curr->op.code = guard_opcode;
    write_curr->op.arg = type_guard_to_index[BINARY_CHECK_INT];
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
// JUMP_IF_FALSE_OR_POP
// to
// BB_TEST_IF_FALSE_OR_POP
// BB_BRANCH
// CACHE (bb_id of the current BB << 1 | is_type_branch)
static inline _Py_CODEUNIT *
emit_logical_branch(_PyTier2TypeContext *type_context, _Py_CODEUNIT *write_curr, _Py_CODEUNIT branch, int bb_id)
{
    int opcode;
    int oparg = _Py_OPARG(branch);
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
        type_propagate(opcode, oparg, type_context, NULL);
        break;
    case JUMP_IF_FALSE_OR_POP:
        opcode = BB_TEST_IF_FALSE_OR_POP;
        // This inst has conditional stack effect according to whether the branch is taken.
        // This inst sets the `gen_bb_requires_pop` flag to handle stack effect of this opcode in BB_BRANCH
        break;
    case JUMP_IF_TRUE_OR_POP:
        opcode = BB_TEST_IF_TRUE_OR_POP;
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
    assert(oparg <= 0XFF);
    // Backwards jumps should be handled specially.
    if (opcode == BB_JUMP_BACKWARD_LAZY) {
#if BB_DEBUG
        fprintf(stderr, "emitted backwards jump %p %d\n", write_curr,
            _Py_OPCODE(branch));
#endif
        // Just in case
        _py_set_opcode(write_curr, EXTENDED_ARG);
        write_curr->op.arg = 0;
        write_curr++;
        // We don't need to recalculate the backward jump, because that only needs to be done
        // when it locates the next BB in JUMP_BACKWARD_LAZY.
        _py_set_opcode(write_curr, BB_JUMP_BACKWARD_LAZY);
        write_curr->op.arg = oparg;
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
        _py_set_opcode(write_curr, BB_TEST_ITER);
        write_curr->op.arg = oparg;
        write_curr++;
        write_curr = emit_cache_entries(write_curr, INLINE_CACHE_ENTRIES_FOR_ITER);
        _py_set_opcode(write_curr, BB_BRANCH);
        write_curr->op.arg = oparg;
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
        //_Py_CODEUNIT *start = write_curr;
        _py_set_opcode(write_curr, opcode);
        write_curr->op.arg = oparg;
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
    _py_set_opcode(write_curr, opcode);
    write_curr->op.arg = oparg;
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
    int backwards_jump_target)
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
        if (t2_info->backward_jump_target_bb_ids[backward_jump_offset_index][jump_i] == -1) {
            t2_info->backward_jump_target_bb_ids[backward_jump_offset_index][jump_i] = meta->id;
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
infer_BINARY_OP_ADD(
    bool *needs_guard,
    _Py_CODEUNIT *prev_type_guard,
    _Py_CODEUNIT raw_op,
    _Py_CODEUNIT *write_curr,
    _PyTier2TypeContext *type_context,
    int bb_id)
{
    *needs_guard = false;
    PyTypeObject *right = typenode_get_type(type_context->type_stack_ptr[-1]);
    PyTypeObject *left = typenode_get_type(type_context->type_stack_ptr[-2]);
    if (left == &PyLong_Type) {
        if (right == &PyLong_Type) {
            write_curr->op.code = BINARY_OP_ADD_INT_REST;
            write_curr++;
            type_propagate(BINARY_OP_ADD_INT_REST, 0, type_context, NULL);
            return write_curr;
        }
    }
    if (left == &PyRawFloat_Type) {
        if (right == &PyRawFloat_Type) {
            write_curr->op.code = BINARY_OP_ADD_FLOAT_UNBOXED;
            write_curr++;
            type_propagate(BINARY_OP_ADD_FLOAT_UNBOXED, 0, type_context, NULL);
            return write_curr;
        }
    }
    // Unknown, time to emit the chain of guards.
    write_curr = rebox_stack(write_curr, type_context, 2);
    *needs_guard = true;
    if (prev_type_guard == NULL) {
        return emit_type_guard(write_curr, BINARY_CHECK_INT, bb_id);
    }
    else {
        int next_guard = type_guard_ladder[
            type_guard_to_index[prev_type_guard->op.code] + 1];
        if (next_guard != -1) {
            return emit_type_guard(write_curr, next_guard, bb_id);
        }
        // End of ladder, fall through
    }
    // Unknown, just emit the same opcode, don't bother emitting guard.
    // Fall through and let the code generator handle.
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
        prev_type_guard->op.code == BINARY_CHECK_FLOAT
    );
#define END() goto end;
#define JUMPBY(x) i += x + 1;
#define DISPATCH()        write_i = emit_i(write_i, specop, oparg); \
                          write_i = copy_cache_entries(write_i, curr+1, caches); \
                          i += caches; \
                          type_propagate(opcode, oparg, starting_type_context, consts); \
                          continue;
#define DISPATCH_GOTO() goto dispatch_opcode;
#define TYPECONST_GET_RAWTYPE(idx) Py_TYPE(PyTuple_GET_ITEM(consts, idx))

    assert(co->_tier2_info != NULL);
    // There are only two cases that a BB ends.
    // 1. If there's a branch instruction / scope exit.
    // 2. If there's a type guard.
    bool needs_guard = 0;

    _PyTier2BBMetadata *meta = NULL;
    _PyTier2BBMetadata *temp_meta = NULL;

    _PyTier2Info *t2_info = co->_tier2_info;
    PyObject *consts = co->co_consts;
    _Py_CODEUNIT *t2_start = (_Py_CODEUNIT *)(((char *)bb_space->u_code) + bb_space->water_level);
    _Py_CODEUNIT *write_i = t2_start;
    int tos = -1;

    // For handling of backwards jumps
    bool starts_with_backwards_jump_target = false;
    int backwards_jump_target_offset = -1;
    bool virtual_start = false;

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
        switch (opcode) {
        case RESUME:
            opcode = RESUME_QUICK;
            DISPATCH();
        case COMPARE_AND_BRANCH:
            opcode = specop = COMPARE_OP;
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
        case LOAD_CONST: {
            if (TYPECONST_GET_RAWTYPE(oparg) == &PyFloat_Type) {
                write_i->op.code = LOAD_CONST;
                write_i->op.arg = oparg;
                write_i++;
                type_propagate(LOAD_CONST, oparg, starting_type_context, consts);
                write_i->op.code = UNBOX_FLOAT;
                write_i->op.arg = 0;
                write_i++;
                type_propagate(UNBOX_FLOAT, 0, starting_type_context, consts);
                continue;
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
            write_i = rebox_stack(write_i, starting_type_context,
                oparg + 2);
            DISPATCH();
        case BUILD_MAP:
            write_i = rebox_stack(write_i, starting_type_context,
                oparg * 2);
            DISPATCH();
        case BUILD_STRING:
        case BUILD_LIST:
            write_i = rebox_stack(write_i, starting_type_context,
                oparg);
            DISPATCH();
        case BINARY_OP:
            if (oparg == NB_ADD) {
                // Add operation. Need to check if we can infer types.
                _Py_CODEUNIT *possible_next = infer_BINARY_OP_ADD(&needs_guard,
                    prev_type_guard,
                    *curr,
                    write_i, starting_type_context,
                    co->_tier2_info->bb_data_curr);
                if (possible_next == NULL) {
                    DISPATCH();
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
            write_i = rebox_stack(write_i, starting_type_context, 2);
            DISPATCH()
        case LOAD_ATTR:
        case CALL_INTRINSIC_1:
        case UNARY_NEGATIVE:
        case UNARY_NOT:
        case UNARY_INVERT:
        case GET_LEN:
            write_i = rebox_stack(write_i, starting_type_context, 1);
            DISPATCH();
        case CALL_INTRINSIC_2:
        case BINARY_SUBSCR:
        case BINARY_SLICE:
            write_i = rebox_stack(write_i, starting_type_context, 2);
            DISPATCH();
        case STORE_SLICE:
            write_i = rebox_stack(write_i, starting_type_context, 4);
            DISPATCH();
        default:
#if BB_DEBUG || TYPEPROP_DEBUG
            fprintf(stderr, "offset: %Id\n", curr - _PyCode_CODE(co));
#endif
            if (IS_BACKWARDS_JUMP_TARGET(co, curr)) {
#if BB_DEBUG
                fprintf(stderr, "Encountered a backward jump target\n");
#endif
                // This should be the end of another basic block, or the start of a new.
                // Start of a new basic block, just ignore and continue.
                if (virtual_start) {
#if BB_DEBUG
                    fprintf(stderr, "Emitted virtual start of basic block\n");
#endif
                    starts_with_backwards_jump_target = true;
                    backwards_jump_target_offset = (int)(curr - _PyCode_CODE(co));
                    virtual_start = false;
                    goto fall_through;
                }
                // Else, create a virtual end to the basic block.
                // But generate the block after that so it can fall through.
                i--;
                // Make a copy of the type context
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
                virtual_start = true;
                starting_type_context = type_context_copy;

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
                    co->_tier2_info->bb_data_curr);
                i += caches;
                END();
            }
            DISPATCH();
        }

    }
    _PyTier2TypeContext *type_context_copy = NULL;
end:
    // Create the tier 2 BB

    // Make a copy of the type context
    type_context_copy = _PyTier2TypeContext_Copy(starting_type_context);
    if (type_context_copy == NULL) {
        return NULL;
    }
    temp_meta = _PyTier2_AllocateBBMetaData(co, t2_start,
        // + 1 because we want to start with the NEXT instruction for the scan
        _PyCode_CODE(co) + i + 1, type_context_copy);
    if (temp_meta == NULL) {
        _PyTier2TypeContext_Free(type_context_copy);
        return NULL;
    }
    // We need to return the first block to enter into. If there is already a block generated
    // before us, then we use that instead of the most recent block.
    if (meta == NULL) {
        meta = temp_meta;
    }
    if (starts_with_backwards_jump_target) {
        // Add the basic block to the jump ids
        if (add_metadata_to_jump_2d_array(t2_info, temp_meta,
            backwards_jump_target_offset) < 0) {
            PyMem_Free(meta);
            if (meta != temp_meta) {
                PyMem_Free(temp_meta);
            }
            _PyTier2TypeContext_Free(type_context_copy);
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
allocate_jump_offset_2d_array(int backwards_jump_count, int **backward_jump_target_bb_ids)
{
    int done = 0;
    for (int i = 0; i < backwards_jump_count; i++) {
        int *jump_offsets = PyMem_Malloc(sizeof(int) * MAX_BB_VERSIONS);
        if (jump_offsets == NULL) {
            goto error;
        }
        for (int i = 0; i < MAX_BB_VERSIONS; i++) {
            jump_offsets[i] = -1;
        }
        done++;
        backward_jump_target_bb_ids[i] = jump_offsets;
    }
    return 0;
error:
    for (int i = 0; i < done; i++) {
        PyMem_Free(backward_jump_target_bb_ids[i]);
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
    int **backward_jump_target_bb_ids = PyMem_Malloc(backwards_jump_count * sizeof(int *));
    if (backward_jump_target_bb_ids == NULL) {
        PyMem_Free(backward_jump_offsets);
        return 1;
    }
    if (allocate_jump_offset_2d_array((int)backwards_jump_count,
        backward_jump_target_bb_ids)) {
        PyMem_Free(backward_jump_offsets);
        PyMem_Free(backward_jump_target_bb_ids);
        return 1;
    }

    _Py_CODEUNIT *start = _PyCode_CODE(co);
    int curr_i = 0;
    for (Py_ssize_t i = 0; i < Py_SIZE(co); i++) {
        _Py_CODEUNIT *curr = start + i;
        _Py_CODEUNIT instr = *curr;
        int opcode = _PyOpcode_Deopt[_Py_OPCODE(instr)];
        int oparg = _Py_OPARG(instr);
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
        i += _PyOpcode_Caches[opcode];
    }
    assert(curr_i == backwards_jump_count);
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
    co->_tier2_info->backward_jump_target_bb_ids = backward_jump_target_bb_ids;
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
        if (IS_FORBIDDEN_OPCODE(deopt)) {
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

    Py_ssize_t space_to_alloc = (_PyCode_NBYTES(co)) * 3;

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

// Lazily generates successive BBs when required.
// The first basic block created will always be directly after the current tier 2 code.
// The second basic block created will always require a jump.
_Py_CODEUNIT *
_PyTier2_GenerateNextBB(
    _PyInterpreterFrame *frame,
    uint16_t bb_id_tagged,
    _Py_CODEUNIT *curr_executing_instr,
    int jumpby,
    _Py_CODEUNIT **tier1_fallback,
    char gen_bb_requires_pop,
    char gen_bb_is_successor)
{
    PyCodeObject *co = frame->f_code;
    assert(co->_tier2_info != NULL);
    assert(BB_ID(bb_id_tagged) <= co->_tier2_info->bb_data_curr);
    _PyTier2BBMetadata *meta = co->_tier2_info->bb_data[BB_ID(bb_id_tagged)];
    _Py_CODEUNIT *tier1_end = meta->tier1_end + jumpby;
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
    // Get type_context of previous BB
    _PyTier2TypeContext *type_context = meta->type_context;
    // Make a copy of the type context
    _PyTier2TypeContext *type_context_copy = _PyTier2TypeContext_Copy(type_context);
    if (type_context_copy == NULL) {
        return NULL;
    }
    // If this flag is set, it means that either BB_TEST_IF_FALSE_OR_POP or
    // BB_TEST_IF_TRUE_OR_POP was ran and the conditional stack effect was performed
    // This means we have to pop an element from the type stack.
    if (gen_bb_requires_pop) {
        type_context_copy->type_stack_ptr--;
    }
    // For type branches, they directly precede the bb branch instruction
    _Py_CODEUNIT *prev_type_guard = BB_IS_TYPE_BRANCH(bb_id_tagged)
        ? curr_executing_instr - 1 : NULL;
    if (gen_bb_is_successor && prev_type_guard != NULL) {
        // Is a type branch, so the previous instruction shouldn't be
        // one of those conditional pops.
        assert(!gen_bb_requires_pop);
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
    return metadata->tier2_start;
}

_Py_CODEUNIT *
_PyTier2_LocateJumpBackwardsBB(_PyInterpreterFrame *frame, uint16_t bb_id_tagged, int jumpby,
    _Py_CODEUNIT **tier1_fallback)
{
    PyCodeObject *co = frame->f_code;
    assert(co->_tier2_info != NULL);
    assert(BB_ID(bb_id_tagged) <= co->_tier2_info->bb_data_curr);
    _PyTier2BBMetadata *meta = co->_tier2_info->bb_data[BB_ID(bb_id_tagged)];
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
    _PyTier2TypeContext *type_context = meta->type_context;
    // Now, find the matching BB
    _PyTier2Info *t2_info = co->_tier2_info;
    int jump_offset = (int)(tier1_jump_target - _PyCode_CODE(co));
    int matching_bb_id = -1;

#if BB_DEBUG
    fprintf(stderr, "finding jump target: %d\n", jump_offset);
#endif
    for (int i = 0; i < t2_info->backward_jump_count; i++) {
#if BB_DEBUG
        fprintf(stderr, "jump offset checked: %d\n", t2_info->backward_jump_offsets[i]);
#endif
        if (t2_info->backward_jump_offsets[i] == jump_offset) {
            for (int x = 0; x < MAX_BB_VERSIONS; x++) {
#if BB_DEBUG
                fprintf(stderr, "jump target BB ID: %d\n",
                    t2_info->backward_jump_target_bb_ids[i][x]);
#endif
                // @TODO, this is where the diff function is supposed to be
                // it will calculate the closest type context BB
                // For now just any valid BB (>= 0) is used.
                if (t2_info->backward_jump_target_bb_ids[i][x] >= 0) {
                    matching_bb_id = t2_info->backward_jump_target_bb_ids[i][x];
                    break;
                }
            }
            break;
        }
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

Some instructions will be special since they need CACHE entries. E.g. FOR_ITER

BB_TEST_ITER
CACHE
BB_BRANCH_IF_FLAG_SET
CACHE

Backwards jumps are handled by another function.
*/

void
_PyTier2_RewriteForwardJump(_Py_CODEUNIT *bb_branch, _Py_CODEUNIT *target)
{
    _Py_CODEUNIT *write_curr = bb_branch;
    // -1 because the PC is auto incremented
    int oparg = (int)(target - bb_branch - 1);
    int branch = _Py_OPCODE(*bb_branch);
    assert(branch == BB_BRANCH_IF_FLAG_SET || branch == BB_BRANCH_IF_FLAG_UNSET);
    bool requires_extended = oparg > 0xFF;
    assert(oparg <= 0xFFFF);
    if (requires_extended) {
        _py_set_opcode(write_curr, EXTENDED_ARG);
        write_curr->op.arg = (oparg >> 8) & 0xFF;
        write_curr++;
        // -1 to oparg because now the jump instruction moves one unit forward.
        oparg--;
    }
    _py_set_opcode(write_curr,
        branch == BB_BRANCH_IF_FLAG_SET ? BB_JUMP_IF_FLAG_SET : BB_JUMP_IF_FLAG_UNSET);
    write_curr->op.arg = oparg & 0xFF;
    write_curr++;
    if (!requires_extended) {
        _py_set_opcode(write_curr, NOP);
        write_curr++;
    }

}


/*
Before:

EXTENDED_ARG/NOP
JUMP_BACKWARD_LAZY
CACHE


After:

EXTENDED_ARG (if needed, else NOP)
JUMP_BACKWARD_LAZY
END_FOR
*/

void
_PyTier2_RewriteBackwardJump(_Py_CODEUNIT *jump_backward_lazy, _Py_CODEUNIT *target)
{
    _Py_CODEUNIT *write_curr = jump_backward_lazy - 1;
    _Py_CODEUNIT *prev = jump_backward_lazy - 1;
    assert(_Py_OPCODE(*jump_backward_lazy) == BB_JUMP_BACKWARD_LAZY);
    assert(_Py_OPCODE(*prev) == EXTENDED_ARG);

    // +1 because we increment the PC before JUMPBY
    int oparg = (int)(target - (jump_backward_lazy + 1));
    assert(oparg < 0);
    oparg = -oparg;
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
    _py_set_opcode(write_curr, JUMP_BACKWARD_QUICK);
    write_curr->op.arg = oparg & 0xFF;
    write_curr++;
    _py_set_opcode(write_curr, END_FOR);
    write_curr++;
}

#undef TYPESTACK_PEEK
#undef TYPESTACK_POKE
#undef TYPELOCALS_SET
#undef TYPELOCALS_GET
#undef TYPE_SET
#undef TYPE_OVERWRITE
