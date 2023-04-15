#ifndef Py_INTERNAL_OPCODE_UTILS_H
#define Py_INTERNAL_OPCODE_UTILS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_opcode.h"        // _PyOpcode_Jump


#define MAX_REAL_OPCODE 254

#define IS_WITHIN_OPCODE_RANGE(opcode) \
        (((opcode) >= 0 && (opcode) <= MAX_REAL_OPCODE) || \
         IS_PSEUDO_OPCODE(opcode))

#define IS_JUMP_OPCODE(opcode) \
         is_bit_set_in_table(_PyOpcode_Jump, opcode)

#define IS_BLOCK_PUSH_OPCODE(opcode) \
        ((opcode) == SETUP_FINALLY || \
         (opcode) == SETUP_WITH || \
         (opcode) == SETUP_CLEANUP)

#define HAS_TARGET(opcode) \
        (IS_JUMP_OPCODE(opcode) || IS_BLOCK_PUSH_OPCODE(opcode))

/* opcodes that must be last in the basicblock */
#define IS_TERMINATOR_OPCODE(opcode) \
        (IS_JUMP_OPCODE(opcode) || IS_SCOPE_EXIT_OPCODE(opcode))

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

#define IS_SCOPE_EXIT_OPCODE(opcode) \
        ((opcode) == RETURN_VALUE || \
         (opcode) == RETURN_CONST || \
         (opcode) == RAISE_VARARGS || \
         (opcode) == RERAISE)

#define IS_SUPERINSTRUCTION_OPCODE(opcode) \
        ((opcode) == LOAD_FAST__LOAD_FAST || \
         (opcode) == LOAD_FAST__LOAD_CONST || \
         (opcode) == LOAD_CONST__LOAD_FAST || \
         (opcode) == STORE_FAST__LOAD_FAST || \
         (opcode) == STORE_FAST__STORE_FAST)


#define LOG_BITS_PER_INT 5
#define MASK_LOW_LOG_BITS 31

static inline int
is_bit_set_in_table(const uint32_t *table, int bitindex) {
    /* Is the relevant bit set in the relevant word? */
    /* 512 bits fit into 9 32-bits words.
     * Word is indexed by (bitindex>>ln(size of int in bits)).
     * Bit within word is the low bits of bitindex.
     */
    if (bitindex >= 0 && bitindex < 512) {
        uint32_t word = table[bitindex >> LOG_BITS_PER_INT];
        return (word >> (bitindex & MASK_LOW_LOG_BITS)) & 1;
    }
    else {
        return 0;
    }
}

#undef LOG_BITS_PER_INT
#undef MASK_LOW_LOG_BITS


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_OPCODE_UTILS_H */
