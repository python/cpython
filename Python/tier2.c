#include "Python.h"
#include "stdlib.h"
#include "pycore_code.h"
#include "pycore_frame.h"
#include "pycore_opcode.h"
#include "pycore_pystate.h"

#include "opcode.h"

#define BB_DEBUG 1

// Number of potential extra instructions at end of a BB, for branch or cleanup purposes.
// BB_BRANCH instruction: 1
#define BB_EPILOG 1

static inline int IS_SCOPE_EXIT_OPCODE(int opcode);

////////// CEVAL functions

////////// Utility functions

// Checks that we have enough scratch space for the current code object. Else allocate more.
static _Py_CODEUNIT *
_PyInterpreter_GetScratchSpace(PyCodeObject *co)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    int n_instrs = (int)Py_SIZE(co);
    Py_ssize_t space_to_alloc = _PyCode_NBYTES(co);
    int overalloc = 2;
    if (interp->tier2_bytecode_scratchsize == 0 ||
        interp->tier2_bytecode_scratch == NULL) {
        interp->tier2_bytecode_scratch = PyMem_Malloc(space_to_alloc * overalloc);
        if (interp->tier2_bytecode_scratch == NULL) {
            return NULL;
        }
        interp->tier2_bytecode_scratchsize = Py_SIZE(co) * overalloc;
        return interp->tier2_bytecode_scratch;
    }
    if (interp->tier2_bytecode_scratchsize < n_instrs) {
        PyMem_Free(interp->tier2_bytecode_scratch);
        interp->tier2_bytecode_scratch = PyMem_Malloc(space_to_alloc * overalloc);
        if (interp->tier2_bytecode_scratch == NULL) {
            return NULL;
        }
        interp->tier2_bytecode_scratchsize = Py_SIZE(co) * overalloc;
    }
    return interp->tier2_bytecode_scratch;
}

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

// Gets end of the bytecode for a tier 2 BB.
_Py_CODEUNIT *
_PyTier2BB_UCodeEnd(_PyTier2BB *bb)
{
    return (_Py_CODEUNIT *)(bb->u_code + bb->n_instrs);
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
    bb_space->next = NULL;
    bb_space->water_level = 0;
    assert((int)(space_to_alloc - sizeof(_PyTier2BB)) == (space_to_alloc - sizeof(_PyTier2BB)));
    bb_space->max_capacity = (int)(space_to_alloc - sizeof(_PyTier2BB));
    return bb_space;
}

////////// TIER 2 BB FUNCTIONS

/* Init a BB in BB space without any checks for waterlevel. */
static _PyTier2BB *
_PyTier2_InitBBNoCheck(_PyTier2BBSpace *bb_space, _Py_CODEUNIT *tier1_end,
    _Py_CODEUNIT *instr_start, _Py_CODEUNIT *instr_end)
{
    Py_ssize_t ninstrs = (instr_end - instr_start) + BB_EPILOG;
    assert(ninstrs == (int)ninstrs);
    Py_ssize_t nbytes = ninstrs * sizeof(_Py_CODEUNIT);
    _PyTier2BB *bb_ptr = &bb_space->bbs[bb_space->water_level];
    bb_ptr->tier1_end = tier1_end;
    bb_ptr->n_instrs = (int)ninstrs;
    bb_ptr->successor_bb = NULL;
    bb_ptr->alternate_bb = NULL;
    // @TODO, add type context.
    memcpy(bb_ptr->u_code, (const void *)instr_start, nbytes);
    assert(bb_space->water_level + nbytes == (int)nbytes);
    bb_space->water_level += (int)nbytes;
    return bb_ptr;
}

/* Allocates and initializes a new basic block. If there's not enough space in
   the overallocated array, create a new array.

   Make sure to call _PyCode_Tier2Initialize before this!
*/
static _PyTier2BB *
_PyCode_Tier2BBNew(PyCodeObject *co, _Py_CODEUNIT *tier1_end, _Py_CODEUNIT *instr_start, _Py_CODEUNIT *instr_end)
{
    assert(co->_tier2_info != NULL);
    assert(co->_tier2_info->_bb_space != NULL);

    _PyTier2BBSpace *bb_space = co->_tier2_info->_bb_space;
    Py_ssize_t amount_to_alloc = (instr_start - instr_end) * sizeof(_Py_CODEUNIT *) + sizeof(_PyTier2BB);
    assert(bb_space->water_level + amount_to_alloc == (int)(bb_space->water_level + amount_to_alloc));

    // Need to allocate a new array.
    if (bb_space->water_level + amount_to_alloc > bb_space->max_capacity) {
        _PyTier2BBSpace *next_bb_space = _PyTier2_CreateBBSpace(bb_space->max_capacity + amount_to_alloc);
        if (next_bb_space == NULL) {
            return NULL;
        }
        next_bb_space->next = bb_space;
        // We want to make our bb_space point to the most recent one to get O(1) BB allocations.
        co->_tier2_info->_bb_space = next_bb_space;
        bb_space = next_bb_space;
    }
    return _PyTier2_InitBBNoCheck(bb_space, tier1_end, instr_start, instr_end);
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
    // @TODO backward jumps should be supported!
    case JUMP_BACKWARD:
    case JUMP_BACKWARD_NO_INTERRUPT:
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
    _py_set_opcode(write_curr, BB_TYPE_BRANCH);
    write_curr++;
    return write_curr;
}

static inline _Py_CODEUNIT *
emit_logical_branch(_Py_CODEUNIT *write_curr, int branch)
{
    int opcode;
    int oparg = 0;
    // @TODO handle JUMP_BACKWARDS and JUMP_BACKWARDS_NO_INTERRUPT
    switch (branch) {
    case FOR_ITER:
        opcode = BB_ITER;
        break;
    case JUMP_IF_FALSE_OR_POP:
        oparg = 1;
    case JUMP_IF_TRUE_OR_POP:
        opcode = BB_BRANCH_OR_POP;
        break;
    case POP_JUMP_IF_FALSE:
        oparg = 1;
    case POP_JUMP_IF_TRUE:
        opcode = BB_POP_THEN_BRANCH;
        break;
    case POP_JUMP_IF_NOT_NONE:
        oparg = 1;
    case POP_JUMP_IF_NONE:
        opcode = BB_POP_BRANCH;
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
    _py_set_opcode(write_curr, opcode);
    write_curr->oparg = oparg;
    write_curr++;
    return write_curr;
}

static inline _Py_CODEUNIT *
emit_scope_exit(_Py_CODEUNIT *write_curr, int exit)
{
    switch (exit) {
    case RETURN_VALUE:
    case RETURN_CONST:
    case INTERPRETER_EXIT:
#if BB_DEBUG
        fprintf(stderr, "emitted scope exit\n");
#endif
        // @TODO we can propogate and chain BBs across call boundaries
        // Thanks to CPython's inlined call frames.
        _py_set_opcode(write_curr, BB_EXIT_FRAME);
        return write_curr + 1;
    default:
        // The rest are forbidden.
#if BB_DEBUG
        fprintf(stderr, "emit_scope_exit unreachable %d\n", exit);
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

// Detects a BB from the current instruction start to the end of the first basic block it sees.
// Then emits the instructions for a _PyTier2BB.
//
// Instructions emitted depend on the type_context.
// For example, if it sees a BINARY_ADD instruction, but it knows the two operands are already of
// type PyLongObject, a BINARY_ADD_INT_REST will be emitted without an type checks.
// 
// However, if one of the operands are unknown, a logical chain of CHECK instructions will be emitted,
// and the basic block will end at the first of the chain.
// 
// Note: a BB end also includes a type guard.
_PyTier2BB *
_PyTier2_Code_DetectAndEmitBB(PyCodeObject *co, _Py_CODEUNIT *start,
    int n_typecontext, PyTypeObject **type_context)
{
#define END() goto end;
#define JUMPBY(x) i += x + 1; continue;
#define BASIC_PUSH(v)     (*stack_pointer++ = (v))
#define BASIC_POP()       (*--stack_pointer)
#define DISPATCH()        write_i = emit_i(write_i, opcode, oparg); write_i = copy_cache_entries(write_i, curr+1, caches); i += caches; continue;
    assert(co->_tier2_info != NULL);
    // There are only two cases that a BB ends.
    // 1. If there's a branch instruction / scope exit.
    // 2. If the instruction is a jump target.

    // Make a copy of the type context
    PyTypeObject **type_context_copy = PyMem_Malloc(n_typecontext * sizeof(PyTypeObject *));
    if (type_context_copy == NULL) {
        return NULL;
    }
    memcpy(type_context_copy, type_context, n_typecontext * sizeof(PyTypeObject *));

    _Py_CODEUNIT *start_i = _PyInterpreter_GetScratchSpace(co);
    _Py_CODEUNIT *write_i = start_i;
    PyTypeObject **stack_pointer = co->_tier2_info->types_stack;
    int tos = -1;

    // A meta-interpreter for types.
    Py_ssize_t i = 0;
    for (; i < Py_SIZE(co); i++) {
        _Py_CODEUNIT *curr = _PyCode_CODE(co) + i;
        _Py_CODEUNIT instr = *curr;
        int opcode = _PyOpcode_Deopt[_Py_OPCODE(instr)];
        int oparg = _Py_OPARG(instr);
        int caches = _PyOpcode_Caches[opcode];

        // Just because an instruction requires a guard doesn't mean it's the end of a BB.
        // We need to check whether we can eliminate the guard based on the current type context.
        int how_many_guards = 0;
        _Py_CODEUNIT guard_instr;
        _Py_CODEUNIT action;

        switch (opcode) {
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
            // These are definitely the end of a basic block.
            if (IS_SCOPE_EXIT_OPCODE(opcode)) {
                // Emit the scope exit instruction.
                write_i = emit_scope_exit(write_i, opcode);
                END();
            }

            // Jumps may be the end of a basic block if they are conditional (a branch).
            if (IS_JUMP_OPCODE(opcode)) {
                // Unconditional forward jump... continue with the BB without writing the jump.
                if (opcode == JUMP_FORWARD) {
                    // JUMP offset (oparg) + current instruction + cache entries
                    JUMPBY(oparg);
                }
                write_i = emit_logical_branch(write_i, opcode);
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
    return _PyCode_Tier2BBNew(co, _PyCode_CODE(co) + i, start_i, write_i);
}


//// The exits of BBs need to use BB_BRANCH instruction.
//static void
//_PyTier2BB_RewriteExitBranch(_PyTier2BB *bb)
//{
//    _Py_CODEUNIT *last_instr = _PyTier2BB_UCodeEnd(bb) - BB_EPILOG;
//    int op = _PyOpcode_Deopt[_Py_OPCODE(*last_instr)];
//    assert(IS_TERMINATOR_OPCODE(op));
//    if (IS_JUMP_OPCODE(op)) {
//        _Py_CODEUNIT lasti = *last_instr;
//        _py_set_opcode(last_instr, BB_BRANCH);
//        // Write the original jump instruction after to know where
//        // to start generating the next BB from.
//        _Py_SET_OPCODE
//        return;
//    }
//    assert(IS_SCOPE_EXIT_OPCODE(op));
//    assert(op == RETURN_VALUE || op == RETURN_CONST);
//    // Need to FRAME_EXIT.
//
//}


////////// _PyTier2Info FUNCTIONS

static int
compare_ints(const void *a, const void *b)
{
    return *(int *)a - *(int *)b;
}

// Returns 1 on error, 0 on success. Populates the jump target offset
// array for a code object.
// NOTE TO SELF: WE MIGHT NOT EVEN NEED TO FILL UP JUMP TARGETS.
// JUMP TARGETS CONTINUE BEING PART OF THE BASIC BLOCK AS LONG
// AS NO BRANCH IS DETECTED.
// REMOVE IN THE FUTURE IF UNECESSARY.
static int
_PyCode_Tier2FillJumpTargets(PyCodeObject *co)
{
    assert(co->_tier2_info != NULL);
    // Remove all the RESUME instructions.
    // Count all the jump targets.
    Py_ssize_t jump_target_count = 0;
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
            jump_target_count += IS_JUMP_OPCODE(opcode); // + INSTR_HAS_GUARD(instr);
        }
        i += _PyOpcode_Caches[opcode];
    }

    // Impossibly big.
    if (jump_target_count != (int)jump_target_count) {
        return 1;
    }

    // Find all the jump target instructions
    // Don't allocate a zero byte space as this may be undefined behavior.
    if (jump_target_count == 0) {
        co->_tier2_info->jump_targets = NULL;
        // Successful (no jump targets)!
        co->_tier2_info->jump_target_count = (int)jump_target_count;
        return 0;
    }
    int *jump_targets = PyMem_Malloc(jump_target_count * sizeof(int));
    if (jump_targets == NULL) {
        return 1;
    }
    _Py_CODEUNIT *start = _PyCode_CODE(co);
    int curr_i = 0;
    for (Py_ssize_t i = 0; i < Py_SIZE(co); i++) {
        _Py_CODEUNIT *curr = start + i;
        _Py_CODEUNIT instr = *curr;
        int opcode = _PyOpcode_Deopt[_Py_OPCODE(instr)];
        int oparg = _Py_OPARG(instr);
        //switch (opcode) {
        //// KEEP IN SYNC WITH INSTR_HAS_GUARD
        //case BINARY_OP:
        //    if (oparg == NB_ADD) {
        //        jump_targets[curr_i] = (int)(curr - start) + INLINE_CACHE_ENTRIES_BINARY_OP;
        //        curr_i++;
        //    }
        //    break;
        //default:
        if (IS_JUMP_OPCODE(opcode)) {
            _Py_CODEUNIT *target = curr + _Py_OPARG(instr);
            // (in terms of offset from start of co_code_adaptive)
            jump_targets[curr_i] = (int)(target - start);
            curr_i++;
        }
        //}
        i += _PyOpcode_Caches[opcode];
    }
    assert(curr_i == jump_target_count);
    qsort(jump_targets, jump_target_count, sizeof(int), compare_ints);
#if BB_DEBUG
    fprintf(stderr, "JUMP TARGET COUNT: %lld\n", jump_target_count);
    fprintf(stderr, "JUMP TARGET OFFSETS (FROM START OF CODE): ");
    for (Py_ssize_t i = 0; i < jump_target_count; i++) {
        fprintf(stderr, "%d ,", jump_targets[i]);
    }
    fprintf(stderr, "\n");
#endif
    co->_tier2_info->jump_target_count = (int)jump_target_count;
    co->_tier2_info->jump_targets = jump_targets;
    return 0;
}

//static _PyTier2IntermediateCode *
//_PyIntermediateCode_Initialize(PyCodeObject *co)
//{
//    assert(co->_tier2_info != NULL);
//    Py_ssize_t space_to_alloc = (sizeof(PyCodeObject) + _PyCode_NBYTES(co)) + BB_EPILOG;
//    _PyTier2IntermediateCode *i_code = PyMem_Malloc(space_to_alloc);
//    if (i_code == NULL) {
//        return NULL;
//    }
//
//    // Copy over bytecode
//    for (Py_ssize_t curr = 0; curr < Py_SIZE(co); curr++) {
//        _Py_CODEUNIT *curr_instr = _PyCode_CODE(co) + curr;
//        // NEED TO TRANSFORM BINARY OPS TO 
//        i_code->code[curr] = *curr_instr;
//    }
//    return i_code;
//}


static _PyTier2Info *
_PyTier2Info_Initialize(PyCodeObject *co)
{
    assert(co->_tier2_info == NULL);
    _PyTier2Info *t2_info = PyMem_Malloc(sizeof(_PyTier2Info));
    if (t2_info == NULL) {
        return NULL;
    }
    co->_tier2_info = t2_info;

    //if (_PyIntermediateCode_Initialize(co) == NULL) {
    //    PyMem_FREE(t2_info);
    //    return NULL;
    //}

    // Next is to intitialize stack space for the tier 2 types meta-interpretr.
    t2_info->types_stack = PyMem_Malloc(co->co_stacksize * sizeof(PyObject *));
    if (t2_info->types_stack == NULL) {
        PyMem_Free(t2_info);
    }
    return t2_info;
}

////////// TYPE CONTEXT FUNCTIONS

static PyTypeObject **
initialize_type_context(PyCodeObject *co) {
    int nlocals = co->co_nlocals;
    PyTypeObject **type_context = PyMem_Malloc(nlocals * sizeof(PyTypeObject *));
    if (type_context == NULL) {
        return NULL;
    }
    // Initialize to uknown type.
    for (int i = 0; i < nlocals; i++) {
        type_context[i] = NULL;
    }
    return type_context;
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

    if (_PyInterpreter_GetScratchSpace(co) == NULL) {
        return NULL;
    }

    _PyTier2Info *t2_info = _PyTier2Info_Initialize(co);
    if (t2_info == NULL) {
        return NULL;
    }

#if BB_DEBUG
    fprintf(stderr, "INITIALIZING\n");
#endif

    Py_ssize_t space_to_alloc = (sizeof(_PyTier2BB) + _PyCode_NBYTES(co)) * 2;

    _PyTier2BBSpace *bb_space = _PyTier2_CreateBBSpace(space_to_alloc);
    if (bb_space == NULL) {
        PyMem_Free(t2_info);
        return NULL;
    }
    if (_PyCode_Tier2FillJumpTargets(co)) {
        goto cleanup;
    }

    t2_info->_bb_space = bb_space;

    PyTypeObject **type_context = initialize_type_context(co);
    if (type_context == NULL) {
        goto cleanup;
    }
    _PyTier2BB *bb_ptr = _PyTier2_Code_DetectAndEmitBB(co, _PyCode_CODE(co), co->co_nlocals, type_context);
    if (bb_ptr == NULL) {
        goto cleanup;
    }
#if BB_DEBUG
    fprintf(stderr, "ENTRY BB END IS: %d\n", (int)(bb_ptr->tier1_end - _PyCode_CODE(co)));
#endif

    
    t2_info->_entry_bb = bb_ptr;

    // Set the starting instruction to the entry BB.
    // frame->prev_instr = bb_ptr->u_code - 1;
    return bb_ptr->u_code;

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

// Executes successor/alternate BB, depending on direction
// Lazily generates successive BBs when required.
// direction: 1 for successor, 0 for alternative.
_Py_CODEUNIT *
_PyTier2BB_ExecuteNextBB(_PyInterpreterFrame *frame, _PyTier2BB **curr_bb, int direction)
{
    PyCodeObject *co = frame->f_code;
    _PyTier2BB *curr = *curr_bb;
    if (BB_SUC) {
        if (curr->successor_bb == NULL) {
            _PyTier2BB *succ = _PyTier2_Code_DetectAndEmitBB(co,
                curr->tier1_end, curr->type_context_len, curr->type_context);
            curr->successor_bb = succ;
            *curr_bb = succ;
            return succ->u_code;
        }
        else {
            *curr_bb = curr->successor_bb;
            return curr->successor_bb->u_code;
        }
    }
    else {
        if (curr->alternate_bb == NULL) {
            _PyTier2BB *alt = _PyTier2_Code_DetectAndEmitBB(co,
                curr->tier1_end, curr->type_context_len, curr->type_context);
            curr->alternate_bb = alt;
            *curr_bb = alt;
            return alt->u_code;
        }
        else {
            *curr_bb = curr->alternate_bb;
            return curr->alternate_bb->u_code;
        }
    }
}
