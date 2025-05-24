#ifndef Py_INTERNAL_OPCODE_UTILS_H
#define Py_INTERNAL_OPCODE_UTILS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#define MAX_REAL_OPCODE 254

#define IS_WITHIN_OPCODE_RANGE(opcode) \
        (((opcode) >= 0 && (opcode) <= MAX_REAL_OPCODE) || \
         IS_PSEUDO_INSTR(opcode))

#define IS_BLOCK_PUSH_OPCODE(opcode) \
        ((opcode) == SETUP_FINALLY || \
         (opcode) == SETUP_WITH || \
         (opcode) == SETUP_CLEANUP)

#define HAS_TARGET(opcode) \
        (OPCODE_HAS_JUMP(opcode) || IS_BLOCK_PUSH_OPCODE(opcode))

/* opcodes that must be last in the basicblock */
#define IS_TERMINATOR_OPCODE(opcode) \
        (OPCODE_HAS_JUMP(opcode) || IS_SCOPE_EXIT_OPCODE(opcode))

/* opcodes which are not emitted in codegen stage, only by the assembler */
#define IS_ASSEMBLER_OPCODE(opcode) \
        ((opcode) == JUMP_FORWARD || \
         (opcode) == JUMP_BACKWARD || \
         (opcode) == JUMP_BACKWARD_NO_INTERRUPT)

#define IS_BACKWARDS_JUMP_OPCODE(opcode) \
        ((opcode) == JUMP_BACKWARD || \
         (opcode) == JUMP_BACKWARD_NO_INTERRUPT)

#define IS_UNCONDITIONAL_JUMP_OPCODE(opcode) \
        ((opcode) == JUMP || \
         (opcode) == JUMP_NO_INTERRUPT || \
         (opcode) == JUMP_FORWARD || \
         (opcode) == JUMP_BACKWARD || \
         (opcode) == JUMP_BACKWARD_NO_INTERRUPT)

#define IS_CONDITIONAL_JUMP_OPCODE(opcode) \
        ((opcode) == POP_JUMP_IF_FALSE || \
         (opcode) == POP_JUMP_IF_TRUE || \
         (opcode) == POP_JUMP_IF_NONE || \
         (opcode) == POP_JUMP_IF_NOT_NONE)

#define IS_SCOPE_EXIT_OPCODE(opcode) \
        ((opcode) == RETURN_VALUE || \
         (opcode) == RAISE_VARARGS || \
         (opcode) == RERAISE)

#define IS_RETURN_OPCODE(opcode) \
        (opcode == RETURN_VALUE)
#define IS_RAISE_OPCODE(opcode) \
        (opcode == RAISE_VARARGS || opcode == RERAISE)


/* Flags used in the oparg for MAKE_FUNCTION */
#define MAKE_FUNCTION_DEFAULTS    0x01
#define MAKE_FUNCTION_KWDEFAULTS  0x02
#define MAKE_FUNCTION_ANNOTATIONS 0x04
#define MAKE_FUNCTION_CLOSURE     0x08
#define MAKE_FUNCTION_ANNOTATE    0x10

/* Values used as the oparg for LOAD_COMMON_CONSTANT */
#define CONSTANT_ASSERTIONERROR 0
#define CONSTANT_NOTIMPLEMENTEDERROR 1
#define CONSTANT_BUILTIN_TUPLE 2
#define CONSTANT_BUILTIN_ALL 3
#define CONSTANT_BUILTIN_ANY 4
#define NUM_COMMON_CONSTANTS 5

/* Values used in the oparg for RESUME */
#define RESUME_AT_FUNC_START 0
#define RESUME_AFTER_YIELD 1
#define RESUME_AFTER_YIELD_FROM 2
#define RESUME_AFTER_AWAIT 3

#define RESUME_OPARG_LOCATION_MASK 0x3
#define RESUME_OPARG_DEPTH1_MASK 0x4

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_OPCODE_UTILS_H */
