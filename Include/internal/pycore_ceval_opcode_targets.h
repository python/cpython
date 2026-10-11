#ifndef Py_INTERNAL_CEVAL_OPCODE_TARGETS_H
#define Py_INTERNAL_CEVAL_OPCODE_TARGETS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#ifdef Py_STATS
#   define TAIL_CALL_PARAMS _PyInterpreterFrame *frame, _PyStackRef *stack_pointer, PyThreadState *tstate, _Py_CODEUNIT *next_instr, const void *instruction_funcptr_table, int oparg, int lastopcode
#   define TAIL_CALL_ARGS frame, stack_pointer, tstate, next_instr, instruction_funcptr_table, oparg, lastopcode
#else
#   define TAIL_CALL_PARAMS _PyInterpreterFrame *frame, _PyStackRef *stack_pointer, PyThreadState *tstate, _Py_CODEUNIT *next_instr, const void *instruction_funcptr_table, int oparg
#   define TAIL_CALL_ARGS frame, stack_pointer, tstate, next_instr, instruction_funcptr_table, oparg
#endif

#if _Py_TAIL_CALL_INTERP
#   if defined(__clang__) || defined(__GNUC__)
#       if !_Py__has_attribute(preserve_none) || !_Py__has_attribute(musttail)
#           error "This compiler does not have support for efficient tail calling."
#       endif
#   elif defined(_MSC_VER) && (_MSC_VER < 1950)
#       error "You need at least VS 2026 / PlatformToolset v145 for tail calling."
#   endif
#   if defined(_MSC_VER) && !defined(__clang__)
#      define Py_MUSTTAIL [[msvc::musttail]]
#      define Py_PRESERVE_NONE_CC __preserve_none
#   else
#       define Py_MUSTTAIL __attribute__((musttail))
#       define Py_PRESERVE_NONE_CC __attribute__((preserve_none))
#   endif
#   define DISPATCH_TABLE_VAR instruction_funcptr_table
#   define DISPATCH_TABLE instruction_funcptr_handler_table
#   define TRACING_DISPATCH_TABLE instruction_funcptr_tracing_table
#   define TARGET(op) Py_NO_INLINE PyObject *Py_PRESERVE_NONE_CC _TAIL_CALL_##op(TAIL_CALL_PARAMS)
    typedef PyObject *(Py_PRESERVE_NONE_CC *py_tail_call_funcptr)(TAIL_CALL_PARAMS);

#   define DISPATCH_GOTO() \
        do { \
            Py_MUSTTAIL return (((py_tail_call_funcptr *)instruction_funcptr_table)[opcode])(TAIL_CALL_ARGS); \
        } while (0)
#   define DISPATCH_GOTO_NON_TRACING() \
        do { \
            Py_MUSTTAIL return (((py_tail_call_funcptr *)DISPATCH_TABLE)[opcode])(TAIL_CALL_ARGS); \
        } while (0)
#   define JUMP_TO_LABEL(name) \
        do { \
            Py_MUSTTAIL return (_TAIL_CALL_##name)(TAIL_CALL_ARGS); \
        } while (0)
#   ifdef Py_STATS
#       define JUMP_TO_PREDICTED(name) \
            do { \
                Py_MUSTTAIL return (_TAIL_CALL_##name)(frame, stack_pointer, tstate, this_instr, instruction_funcptr_table, oparg, lastopcode); \
            } while (0)
#   else
#       define JUMP_TO_PREDICTED(name) \
            do { \
                Py_MUSTTAIL return (_TAIL_CALL_##name)(frame, stack_pointer, tstate, this_instr, instruction_funcptr_table, oparg); \
            } while (0)
#   endif
#    define LABEL(name) TARGET(name)
#   include "opcode_targets.h"
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CEVAL_OPCODE_TARGETS_H */
