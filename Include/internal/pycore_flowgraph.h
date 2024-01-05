#ifndef Py_INTERNAL_CFG_H
#define Py_INTERNAL_CFG_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_opcode_utils.h"
#include "pycore_compile.h"


typedef struct {
    int i_opcode;
    int i_oparg;
    _PyCompilerSrcLocation i_loc;
    struct _PyCfgBasicblock_ *i_target; /* target block (if jump instruction) */
    struct _PyCfgBasicblock_ *i_except; /* target block when exception is raised */
} _PyCfgInstruction;

typedef struct {
    int id;
} _PyCfgJumpTargetLabel;


typedef struct {
    struct _PyCfgBasicblock_ *handlers[CO_MAXBLOCKS+2];
    int depth;
} _PyCfgExceptStack;

typedef struct _PyCfgBasicblock_ {
    /* Each basicblock in a compilation unit is linked via b_list in the
       reverse order that the block are allocated.  b_list points to the next
       block in this list, not to be confused with b_next, which is next by
       control flow. */
    struct _PyCfgBasicblock_ *b_list;
    /* The label of this block if it is a jump target, -1 otherwise */
    _PyCfgJumpTargetLabel b_label;
    /* Exception stack at start of block, used by assembler to create the exception handling table */
    _PyCfgExceptStack *b_exceptstack;
    /* pointer to an array of instructions, initially NULL */
    _PyCfgInstruction *b_instr;
    /* If b_next is non-NULL, it is a pointer to the next
       block reached by normal control flow. */
    struct _PyCfgBasicblock_ *b_next;
    /* number of instructions used */
    int b_iused;
    /* length of instruction array (b_instr) */
    int b_ialloc;
    /* Used by add_checks_for_loads_of_unknown_variables */
    uint64_t b_unsafe_locals_mask;
    /* Number of predecessors that a block has. */
    int b_predecessors;
    /* depth of stack upon entry of block, computed by stackdepth() */
    int b_startdepth;
    /* instruction offset for block, computed by assemble_jump_offsets() */
    int b_offset;
    /* Basic block is an exception handler that preserves lasti */
    unsigned b_preserve_lasti : 1;
    /* Used by compiler passes to mark whether they have visited a basic block. */
    unsigned b_visited : 1;
    /* b_except_handler is used by the cold-detection algorithm to mark exception targets */
    unsigned b_except_handler : 1;
    /* b_cold is true if this block is not perf critical (like an exception handler) */
    unsigned b_cold : 1;
    /* b_warm is used by the cold-detection algorithm to mark blocks which are definitely not cold */
    unsigned b_warm : 1;
} _PyCfgBasicblock;

int _PyBasicblock_InsertInstruction(_PyCfgBasicblock *block, int pos, _PyCfgInstruction *instr);

typedef struct cfg_builder_ {
    /* The entryblock, at which control flow begins. All blocks of the
       CFG are reachable through the b_next links */
    _PyCfgBasicblock *g_entryblock;
    /* Pointer to the most recently allocated block.  By following
       b_list links, you can reach all allocated blocks. */
    _PyCfgBasicblock *g_block_list;
    /* pointer to the block currently being constructed */
    _PyCfgBasicblock *g_curblock;
    /* label for the next instruction to be placed */
    _PyCfgJumpTargetLabel g_current_label;
} _PyCfgBuilder;

int _PyCfgBuilder_UseLabel(_PyCfgBuilder *g, _PyCfgJumpTargetLabel lbl);
int _PyCfgBuilder_Addop(_PyCfgBuilder *g, int opcode, int oparg, _PyCompilerSrcLocation loc);

int _PyCfgBuilder_Init(_PyCfgBuilder *g);
void _PyCfgBuilder_Fini(_PyCfgBuilder *g);

_PyCfgInstruction* _PyCfg_BasicblockLastInstr(const _PyCfgBasicblock *b);
int _PyCfg_OptimizeCodeUnit(_PyCfgBuilder *g, PyObject *consts, PyObject *const_cache,
                            int code_flags, int nlocals, int nparams, int firstlineno);
int _PyCfg_Stackdepth(_PyCfgBasicblock *entryblock, int code_flags);
void _PyCfg_ConvertPseudoOps(_PyCfgBasicblock *entryblock);
int _PyCfg_ResolveJumps(_PyCfgBuilder *g);


static inline int
basicblock_nofallthrough(const _PyCfgBasicblock *b) {
    _PyCfgInstruction *last = _PyCfg_BasicblockLastInstr(b);
    return (last &&
            (IS_SCOPE_EXIT_OPCODE(last->i_opcode) ||
             IS_UNCONDITIONAL_JUMP_OPCODE(last->i_opcode)));
}

#define BB_NO_FALLTHROUGH(B) (basicblock_nofallthrough(B))
#define BB_HAS_FALLTHROUGH(B) (!basicblock_nofallthrough(B))

PyCodeObject *
_PyAssemble_MakeCodeObject(_PyCompile_CodeUnitMetadata *u, PyObject *const_cache,
                           PyObject *consts, int maxdepth, _PyCompile_InstructionSequence *instrs,
                           int nlocalsplus, int code_flags, PyObject *filename);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CFG_H */
