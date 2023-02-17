#include "Python.h"
#include "stdlib.h"
#include "pycore_code.h"
#include "pycore_frame.h"
#include "pycore_opcode.h"
#include "pycore_pystate.h"

#include "opcode.h"

#define BB_DEBUG 1

// Number of potential extra instructions at end of a BB, for branch or cleanup purposes.
#define BB_EPILOG 0

static inline int IS_SCOPE_EXIT_OPCODE(int opcode);


////////// TYPE CONTEXT FUNCTIONS

static PyTypeObject **
initialize_type_context(PyCodeObject *co, int *type_context_len) {
    int nlocals = co->co_nlocals;
    PyTypeObject **type_context = PyMem_Malloc(nlocals * sizeof(PyTypeObject *));
    if (type_context == NULL) {
        return NULL;
    }
    // Initialize to uknown type.
    for (int i = 0; i < nlocals; i++) {
        type_context[i] = NULL;
    }
    *type_context_len = nlocals;
    return type_context;
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
    _PyTier2BBSpace *bb_space = PyMem_Malloc(space_to_alloc);
    if (bb_space == NULL) {
        return NULL;
    }
    bb_space->water_level = 0;
    bb_space->max_capacity = (space_to_alloc - sizeof(_PyTier2BBSpace));
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
        _PyTier2BBSpace *new_space = PyMem_Realloc(curr, new_size);
        if (new_space == NULL) {
            return NULL;
        }
        co->_tier2_info->_bb_space = new_space;
        new_space->max_capacity = new_size;
        PyMem_Free(curr);
        return new_space;
    }
    // We have enouogh space. Don't do anything, j
    return curr;
}

// BB METADATA FUNCTIONS

static _PyTier2BBMetadata *
allocate_bb_metadata(PyCodeObject *co, _Py_CODEUNIT *tier2_start,
    _Py_CODEUNIT *tier1_end, int type_context_len,
    PyTypeObject **type_context)
{
    _PyTier2BBMetadata *metadata = PyMem_Malloc(sizeof(_PyTier2BBMetadata));
    if (metadata == NULL) {
        return NULL;
    }

    metadata->tier2_start = tier2_start;
    metadata->tier1_end = tier1_end;
    metadata->type_context = type_context;
    metadata->type_context_len = type_context_len;
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
    _Py_CODEUNIT *tier1_end, int type_context_len,
    PyTypeObject **type_context)
{
    _PyTier2BBMetadata *meta = allocate_bb_metadata(co,
        tier2_start, tier1_end, type_context_len, type_context);
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
    return opcode == JUMP_BACKWARD_NO_INTERRUPT || opcode == JUMP_BACKWARD;
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
    // Generators and coroutines
    case SEND:
    case YIELD_VALUE:
    // Raise keyword
    case RAISE_VARARGS:
    // Exceptions, we could support these theoretically.
    // Just too much work for now
    case PUSH_EXC_INFO:
    case RERAISE:
    case POP_EXCEPT:
    // Closures
    case LOAD_DEREF:
    case MAKE_CELL:
    // DELETE_FAST
    case DELETE_FAST:
    // TODO: Pattern matching add here
        return 1;

    default:
        return 0;
    }
}

static inline PyTypeObject *
INSTR_LOCAL_READ_TYPE(PyCodeObject *co, _Py_CODEUNIT instr, PyTypeObject **type_context)
{
    int opcode = _PyOpcode_Deopt[_Py_OPCODE(instr)];
    int oparg = _Py_OPARG(instr);
    switch (opcode) {
    case LOAD_CONST:
        return Py_TYPE(PyTuple_GET_ITEM(co->co_consts, oparg));
    case LOAD_FAST:
        return type_context[oparg];
    // Note: Don't bother with LOAD_NAME, because those exist only in the global
    // scope.
    default:
        return NULL;
    }
}

/*
* This does multiple things:
* 1. Gets the result type of an instruction, if it can figure it out.
  2. Determines whether thet instruction has a corresponding macro/micro instruction
   that acts a guard, and whether it needs it.
  3. How many guards it needs, and what are the guards.
  4. Finally, the corresponding specialised micro-op to operate on this type.

  Returns the inferred type of an operation. NULL if unknown.

  how_many_guards returns the number of guards (0, 1)
  guards should be a opcode corresponding to the guard instruction to use.
*/
static PyTypeObject *
BINARY_OP_RESULT_TYPE(PyCodeObject *co, _Py_CODEUNIT *instr, int n_typecontext,
    PyTypeObject **type_context, int *how_many_guards, _Py_CODEUNIT *guard, _Py_CODEUNIT *action)
{
    int opcode = _PyOpcode_Deopt[_Py_OPCODE(*instr)];
    int oparg = _Py_OPARG(*instr);
    switch (opcode) {
    case BINARY_OP:
        if (oparg == NB_ADD) {
            // For BINARY OP, read the previous two load instructions
            // to see what variables we need to type check.
            PyTypeObject *lhs_type = INSTR_LOCAL_READ_TYPE(co, *(instr - 2), type_context);
            PyTypeObject *rhs_type = INSTR_LOCAL_READ_TYPE(co, *(instr - 1), type_context);
            // The two instruction types are known. 
            if (lhs_type == &PyLong_Type) {
                if (rhs_type == &PyLong_Type) {
                    *how_many_guards = 0;
                    action->opcode = BINARY_OP_ADD_INT_REST;
                    return &PyLong_Type;
                }
                //// Right side unknown. Emit single check.
                //else if (rhs_type == NULL) {
                //    *how_many_guards = 1;
                //    guard->opcode = 
                //    return NULL;
                //}
            }
            // We don't know anything, need to emit guard.
            // @TODO
        }
        break;
    }
    return NULL;
}

static inline _Py_CODEUNIT *
emit_type_guard(_Py_CODEUNIT *write_curr, _Py_CODEUNIT guard)
{
    *write_curr = guard;
    write_curr++;
    _py_set_opcode(write_curr, BB_BRANCH);
    write_curr++;
    return write_curr;
}

// Converts the tier 1 branch bytecode to tier 2 branch bytecode.
static inline _Py_CODEUNIT *
emit_logical_branch(_Py_CODEUNIT *write_curr, _Py_CODEUNIT branch, int bb_id)
{
    int opcode;
    int oparg = _Py_OPARG(branch);
    // @TODO handle JUMP_BACKWARDS and JUMP_BACKWARDS_NO_INTERRUPT
    switch (_Py_OPCODE(branch)) {
    case JUMP_BACKWARD:
        // The initial backwards jump needs to find the right basic block.
        // Subsequent jumps don't need to check this anymore. They can just
        // jump directly with JUMP_BACKWARD.
        opcode = BB_JUMP_BACKWARD_LAZY;
        break;
    case FOR_ITER:
        opcode = BB_TEST_ITER;
        break;
    case JUMP_IF_FALSE_OR_POP:
        opcode = BB_TEST_IF_FALSE_OR_POP;
        break;
    case JUMP_IF_TRUE_OR_POP:
        opcode = BB_TEST_IF_TRUE_OR_POP;
        break;
    case POP_JUMP_IF_FALSE:
        opcode = BB_TEST_POP_IF_FALSE;
        break;
    case POP_JUMP_IF_TRUE:
        opcode = BB_TEST_POP_IF_TRUE;
        break;
    case POP_JUMP_IF_NOT_NONE:
        opcode = BB_TEST_POP_IF_NOT_NONE;
        break;
    case POP_JUMP_IF_NONE:
        opcode = BB_TEST_POP_IF_NONE;
        break;
    default:
        // Honestly shouldn't happen because branches that
        // we can't handle are in IS_FORBIDDEN_OPCODE
#if BB_DEBUG
        fprintf(stderr, "emit_logical_branch unreachable\n");
#endif
        Py_UNREACHABLE();
    }
#if BB_DEBUG
    fprintf(stderr, "emitted logical branch\n");
#endif
    // We prefix with an empty EXTENDED_ARG, just in case the future jumps
    // are not large enough to handle the bytecode format when jumping to
    // the 2nd bb.
    _py_set_opcode(write_curr, EXTENDED_ARG);
    write_curr->oparg = 0;
    write_curr++;
    _py_set_opcode(write_curr, opcode);
    write_curr->oparg = oparg;
    write_curr++;
    _PyBBBranchCache *cache = (_PyBBBranchCache *)write_curr;
    for (int i = 0; i < INLINE_CACHE_ENTRIES_BB_BRANCH; i++) {
        _py_set_opcode(write_curr, CACHE);
        write_curr++;
    }
    assert((uint16_t)(bb_id) == bb_id);
    cache->bb_id = (uint16_t)(bb_id);
    return write_curr;
}

static inline _Py_CODEUNIT *
emit_scope_exit(_Py_CODEUNIT *write_curr, _Py_CODEUNIT exit)
{
    switch (_Py_OPCODE(exit)) {
    case RETURN_VALUE:
    case RETURN_CONST:
    case INTERPRETER_EXIT:
#if BB_DEBUG
        fprintf(stderr, "emitted scope exit\n");
#endif
        //// @TODO we can propogate and chain BBs across call boundaries
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
    write_curr->oparg = oparg;
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
_PyTier2_Code_DetectAndEmitBB(PyCodeObject *co, _PyTier2BBSpace *bb_space,
    _Py_CODEUNIT *start,
    int n_typecontext, PyTypeObject **type_context)
{
#define END() goto end;
#define JUMPBY(x) i += x + 1; continue;
#define BASIC_PUSH(v)     (*stack_pointer++ = (v))
#define BASIC_POP()       (*--stack_pointer)
#define DISPATCH()        write_i = emit_i(write_i, opcode, oparg); \
                          write_i = copy_cache_entries(write_i, curr+1, caches); \
                          i += caches; \
                          continue;
#define DISPATCH_GOTO() goto dispatch_opcode;

                             
    assert(co->_tier2_info != NULL);
    // There are only two cases that a BB ends.
    // 1. If there's a branch instruction / scope exit.
    // 2. If the instruction is a jump target.

    // Make a copy of the type context
    PyTypeObject **type_context_copy = PyMem_Malloc(n_typecontext * sizeof(PyTypeObject *));
    if (type_context_copy == NULL) {
        return NULL;
    }

    _PyTier2BBMetadata *meta = NULL;


    memcpy(type_context_copy, type_context, n_typecontext * sizeof(PyTypeObject *));

    _Py_CODEUNIT *t2_start = (_Py_CODEUNIT *)(((char *)bb_space->u_code) + bb_space->water_level);
    _Py_CODEUNIT *write_i = t2_start;
    PyTypeObject **stack_pointer = co->_tier2_info->types_stack;
    int tos = -1;


    // A meta-interpreter for types.
    Py_ssize_t i = 0;
    for (; i < Py_SIZE(co); i++) {
        _Py_CODEUNIT *curr = _PyCode_CODE(co) + i;
        _Py_CODEUNIT *next_instr = curr + 1;
        int opcode = _PyOpcode_Deopt[_Py_OPCODE(*curr)];
        int oparg = _Py_OPARG(*curr);
        int caches = _PyOpcode_Caches[opcode];

        // Just because an instruction requires a guard doesn't mean it's the end of a BB.
        // We need to check whether we can eliminate the guard based on the current type context.
        int how_many_guards = 0;
        _Py_CODEUNIT guard_instr;
        _Py_CODEUNIT action;

    dispatch_opcode:
        switch (opcode) {
        case EXTENDED_ARG:
            write_i = emit_i(write_i, EXTENDED_ARG, _Py_OPARG(*curr));
            curr++;
            next_instr++;
            oparg = oparg << 8 | _Py_OPARG(*curr);
            opcode = _Py_OPCODE(*curr);
            caches = _PyOpcode_Caches[opcode];
            DISPATCH_GOTO();
        // We need to rewrite the pseudo-branch instruction.
        case COMPARE_AND_BRANCH:
            opcode = COMPARE_OP;
            DISPATCH();
        //case LOAD_FAST:
        //    BASIC_PUSH(type_context[oparg]);
        //    DISPATCH();
        //case LOAD_NAME:
        //    BASIC_PUSH(NULL);
        //    DISPATCH();
        //case LOAD_CONST: {
        //    PyTypeObject *cont = Py_TYPE(PyTuple_GET_ITEM(co->co_consts, oparg));
        //    BASIC_PUSH(cont);
        //    DISPATCH();
        //}
        //case STORE_FAST: {
        //    type_context[oparg] = BASIC_POP();
        //    DISPATCH();
        //}
        //case STORE_NAME:
        //    BASIC_POP();
        //    DISPATCH();
        //case BINARY_OP: {
        //    PyTypeObject *res = BINARY_OP_RESULT_TYPE(co, curr, n_typecontext, type_context,
        //        &how_many_guards, &guard_instr, &action);
        //    // We need a guard. So this is the end of the basic block.
        //    // @TODO in the future, support multiple guards.
        //    if (how_many_guards > 0) {
        //        // Emit the guard
        //        emit_type_guard(&write_i, guard_instr);
        //        END();
        //    }
        //    else {
        //        BASIC_PUSH(res);
        //        // Don't need a guard, either is a micro op, or is a generic instruction
        //        // No type information known. Use a generic instruction.
        //        if (res == NULL) {
        //            DISPATCH();
        //        }
        //        else {
        //            emit_i(&write_i, _Py_OPCODE(action), 0);
        //            break;
        //        }
        //    }
        //}
        default:
            if (IS_BACKWARDS_JUMP_TARGET(co, curr)) {
                // Indicates it's the start a new basic block. That's fine. Just continue.
                if (write_i == t2_start) {
                    DISPATCH();
                }
                // Not a start of a new basic block, we need to log this as a basic block
                // and start a new BB for the sake of JUMP_BACKWARD.
                i--;
                meta = _PyTier2_AllocateBBMetaData(co, t2_start, _PyCode_CODE(co) + i, n_typecontext, type_context_copy);
                if (meta == NULL) {
                    PyMem_Free(type_context_copy);
                    return NULL;
                }
                // Set the new start
                t2_start = write_i;
                // Don't increment, just fall through and let the new BB start with this
                // instruction.
            }
            // These are definitely the end of a basic block.
            if (IS_SCOPE_EXIT_OPCODE(opcode)) {
                // Emit the scope exit instruction.
                write_i = emit_scope_exit(write_i, *curr);
                END();
            }

            

            // Jumps may be the end of a basic block if they are conditional (a branch).
            if (IS_JUMP_OPCODE(opcode)) {
                // Unconditional forward jump... continue with the BB without writing the jump.
                if (opcode == JUMP_FORWARD) {
                    // JUMP offset (oparg) + current instruction + cache entries
                    JUMPBY(oparg);
                }
                // Get the BB ID without incrementing it.
                // AllocateBBMetaData will increment.
                write_i = emit_logical_branch(write_i, *curr,
                    co->_tier2_info->bb_data_curr);
                END();
            }
            DISPATCH();
        }

    }
end:
//#if BB_DEBUG
//    fprintf(stderr, "i is %Id\n", i);
//#endif
    // Create the tier 2 BB
     meta = _PyTier2_AllocateBBMetaData(co, t2_start, _PyCode_CODE(co) + i, n_typecontext, type_context_copy);
     // Tell BB space the number of bytes we wrote.
     bb_space->water_level += (write_i - t2_start) * sizeof(_Py_CODEUNIT);
     return meta;

}


////////// _PyTier2Info FUNCTIONS

static int
compare_ints(const void *a, const void *b)
{
    return *(int *)a - *(int *)b;
}

// Returns 1 on error, 0 on success. Populates the backwards jump target offset
// array for a code object..
static int
_PyCode_Tier2FillJumpTargets(PyCodeObject *co)
{
    assert(co->_tier2_info != NULL);
    // Remove all the RESUME instructions.
    // Count all the jump targets.
    Py_ssize_t backwards_jump_count = 0;
    for (Py_ssize_t i = 0; i < Py_SIZE(co); i++) {
        _Py_CODEUNIT *instr_ptr = _PyCode_CODE(co) + i;
        _Py_CODEUNIT instr = *instr_ptr;
        int opcode = _PyOpcode_Deopt[_Py_OPCODE(instr)];
        int oparg = _Py_OPARG(instr);
        switch (opcode) {
        case RESUME:
            _py_set_opcode(instr_ptr, RESUME_QUICK);
            break;
        default:
            // We want to track all guard instructions as
            // jumps too.
            backwards_jump_count += IS_JUMP_BACKWARDS_OPCODE(opcode); // + INSTR_HAS_GUARD(instr);
        }
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
    int *backward_jump_target_bb_ids = PyMem_Malloc(backwards_jump_count * sizeof(int));
    if (backward_jump_target_bb_ids == NULL) {
        PyMem_Free(backward_jump_offsets);
        return 1;
    }
    for (int i = 0; i < backwards_jump_count; i++) {
        backward_jump_target_bb_ids[i] = -1;
    }
    _Py_CODEUNIT *start = _PyCode_CODE(co);
    int curr_i = 0;
    for (Py_ssize_t i = 0; i < Py_SIZE(co); i++) {
        _Py_CODEUNIT *curr = start + i;
        _Py_CODEUNIT instr = *curr;
        int opcode = _PyOpcode_Deopt[_Py_OPCODE(instr)];
        int oparg = _Py_OPARG(instr);
        if (IS_JUMP_BACKWARDS_OPCODE(opcode)) {
            _Py_CODEUNIT *target = curr + _Py_OPARG(instr);
            // (in terms of offset from start of co_code_adaptive)
            backward_jump_offsets[curr_i] = (int)(target - start);
            curr_i++;
        }
        i += _PyOpcode_Caches[opcode];
    }
    assert(curr_i == backwards_jump_count);
    qsort(backward_jump_offsets,backwards_jump_count,
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

    // Next is to intitialize stack space for the tier 2 types meta-interpretr.
    PyTypeObject **types_stack = PyMem_Malloc(co->co_stacksize * sizeof(PyObject *));
    if (types_stack == NULL) {
        PyMem_Free(t2_info);
        return NULL;
    }
    t2_info->types_stack = types_stack;
    t2_info->backward_jump_count = 0;
    t2_info->backward_jump_offsets = NULL;

    // Initialize BB data array
    t2_info->bb_data_len = 0;
    t2_info->bb_data = NULL;
    t2_info->bb_data_curr = 0;
    int bb_data_len = (Py_SIZE(co) / 5 + 1);
    _PyTier2Info **bb_data = PyMem_Malloc(bb_data_len * sizeof(_PyTier2Info *));
    if (bb_data == NULL) {
        PyMem_Free(t2_info);
        PyMem_Free(types_stack);
        return NULL;
    }
    t2_info->bb_data_len = bb_data_len;
    t2_info->bb_data = bb_data;
    co->_tier2_info = t2_info;

    return t2_info;
}


////////// OVERALL TIER2 FUNCTIONS


// 1. Initialize whatever we need.
// 2. Create the entry BB.
// 3. Jump into that BB.
static _Py_CODEUNIT *
_PyCode_Tier2Initialize(_PyInterpreterFrame *frame, _Py_CODEUNIT *next_instr)
{
    assert(_Py_OPCODE(*(next_instr - 1)) == RESUME);

    // First check for forbidden opcodes that we currently can't handle.
    PyCodeObject *co = frame->f_code;
    // Impossibly big.
    if ((int)Py_SIZE(co) != Py_SIZE(co)) {
        return NULL;
    }
    for (Py_ssize_t curr = 0; curr < Py_SIZE(co); curr++) {
        _Py_CODEUNIT *curr_instr = _PyCode_CODE(co) + curr;
        if (IS_FORBIDDEN_OPCODE(_PyOpcode_Deopt[_Py_OPCODE(*curr_instr)])) {
            return NULL;
        }
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

    int type_context_len = 0;
    PyTypeObject **type_context = initialize_type_context(co, &type_context_len);
    if (type_context == NULL) {
        goto cleanup;
    }
    _PyTier2BBMetadata *meta = _PyTier2_Code_DetectAndEmitBB(
        co, bb_space,
        _PyCode_CODE(co), type_context_len, type_context);
    if (meta == NULL) {
        PyMem_Free(type_context);
        goto cleanup;
    }
#if BB_DEBUG
    fprintf(stderr, "ENTRY BB END IS: %d\n", (int)(meta->tier1_end - _PyCode_CODE(co)));
#endif

    
    t2_info->_entry_bb = meta->tier2_start;

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
        if (code->_tier2_warmup == 0) {
            // If it fails, due to lack of memory or whatever,
            // just fall back to the tier 1 interpreter.
            _Py_CODEUNIT *next = _PyCode_Tier2Initialize(frame, next_instr);
            return next_instr;
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
_PyTier2_GenerateNextBB(_PyInterpreterFrame *frame, _Py_CODEUNIT *curr_tier1,
    _Py_CODEUNIT *curr_tier2)
{
    // Be a pessimist and assume we need to write the entire code into the BB.
    // The size of the BB generated will definitely be equal to or smaller than this.
    _PyTier2BBSpace *space = _PyTier2_BBSpaceCheckAndReallocIfNeeded(
        frame->f_code, _PyCode_NBYTES(frame->f_code));
    if (space == NULL) {
        // DEOPTIMIZE TO TIER 1?
        return NULL;
    }
    // Write to the top of the space. That is automatically where the next instruction
    // should execute.
    // start writing at curr_tier2
    int type_context_len = 0;
    PyTypeObject **type_context = initialize_type_context(frame->f_code, &type_context_len);
    if (type_context == NULL) {
        return NULL;
    }
    _PyTier2BBMetadata *metadata = _PyTier2_Code_DetectAndEmitBB(
        frame->f_code, space, curr_tier1, type_context_len,
        type_context);
    if (metadata == NULL) {
        PyMem_Free(type_context);
        return NULL;
    }
    return metadata->tier2_start;
}
