#ifndef Py_INTERNAL_OPTIMIZER_H
#define Py_INTERNAL_OPTIMIZER_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_uop_ids.h"
#include <stdbool.h>

// This is the length of the trace we project initially.
#define UOP_MAX_TRACE_LENGTH 800

#define TRACE_STACK_SIZE 5

int _Py_uop_analyze_and_optimize(_PyInterpreterFrame *frame,
    _PyUOpInstruction *trace, int trace_len, int curr_stackentries,
    _PyBloomFilter *dependencies);

extern PyTypeObject _PyCounterExecutor_Type;
extern PyTypeObject _PyCounterOptimizer_Type;
extern PyTypeObject _PyDefaultOptimizer_Type;
extern PyTypeObject _PyUOpExecutor_Type;
extern PyTypeObject _PyUOpOptimizer_Type;

/* Symbols */
/* See explanation in optimizer_symbols.c */

struct _Py_UopsSymbol {
    int flags;  // 0 bits: Top; 2 or more bits: Bottom
    PyTypeObject *typ;  // Borrowed reference
    PyObject *const_val;  // Owned reference (!)
};

// Holds locals, stack, locals, stack ... co_consts (in that order)
#define MAX_ABSTRACT_INTERP_SIZE 4096

#define TY_ARENA_SIZE (UOP_MAX_TRACE_LENGTH * 5)

// Need extras for root frame and for overflow frame (see TRACE_STACK_PUSH())
#define MAX_ABSTRACT_FRAME_DEPTH (TRACE_STACK_SIZE + 2)

typedef struct _Py_UopsSymbol _Py_UopsSymbol;

struct _Py_UOpsAbstractFrame {
    // Max stacklen
    int stack_len;
    int locals_len;

    _Py_UopsSymbol **stack_pointer;
    _Py_UopsSymbol **stack;
    _Py_UopsSymbol **locals;
};

typedef struct _Py_UOpsAbstractFrame _Py_UOpsAbstractFrame;

typedef struct ty_arena {
    int ty_curr_number;
    int ty_max_number;
    _Py_UopsSymbol arena[TY_ARENA_SIZE];
} ty_arena;

struct _Py_UOpsContext {
    PyObject_HEAD
    // The current "executing" frame.
    _Py_UOpsAbstractFrame *frame;
    _Py_UOpsAbstractFrame frames[MAX_ABSTRACT_FRAME_DEPTH];
    int curr_frame_depth;

    // Arena for the symbolic types.
    ty_arena t_arena;

    _Py_UopsSymbol **n_consumed;
    _Py_UopsSymbol **limit;
    _Py_UopsSymbol *locals_and_stack[MAX_ABSTRACT_INTERP_SIZE];
};

typedef struct _Py_UOpsContext _Py_UOpsContext;

extern bool _Py_uop_sym_is_null(_Py_UopsSymbol *sym);
extern bool _Py_uop_sym_is_not_null(_Py_UopsSymbol *sym);
extern bool _Py_uop_sym_is_const(_Py_UopsSymbol *sym);
extern PyObject *_Py_uop_sym_get_const(_Py_UopsSymbol *sym);
extern _Py_UopsSymbol *_Py_uop_sym_new_unknown(_Py_UOpsContext *ctx);
extern _Py_UopsSymbol *_Py_uop_sym_new_not_null(_Py_UOpsContext *ctx);
extern _Py_UopsSymbol *_Py_uop_sym_new_type(
    _Py_UOpsContext *ctx, PyTypeObject *typ);
extern _Py_UopsSymbol *_Py_uop_sym_new_const(_Py_UOpsContext *ctx, PyObject *const_val);
extern _Py_UopsSymbol *_Py_uop_sym_new_null(_Py_UOpsContext *ctx);
extern bool _Py_uop_sym_has_type(_Py_UopsSymbol *sym);
extern bool _Py_uop_sym_matches_type(_Py_UopsSymbol *sym, PyTypeObject *typ);
extern bool _Py_uop_sym_set_null(_Py_UopsSymbol *sym);
extern bool _Py_uop_sym_set_non_null(_Py_UopsSymbol *sym);
extern bool _Py_uop_sym_set_type(_Py_UopsSymbol *sym, PyTypeObject *typ);
extern bool _Py_uop_sym_set_const(_Py_UopsSymbol *sym, PyObject *const_val);
extern bool _Py_uop_sym_is_bottom(_Py_UopsSymbol *sym);
extern int _Py_uop_sym_truthiness(_Py_UopsSymbol *sym);
extern PyTypeObject *_Py_uop_sym_get_type(_Py_UopsSymbol *sym);


extern int _Py_uop_abstractcontext_init(_Py_UOpsContext *ctx);
extern void _Py_uop_abstractcontext_fini(_Py_UOpsContext *ctx);

extern _Py_UOpsAbstractFrame *_Py_uop_frame_new(
    _Py_UOpsContext *ctx,
    PyCodeObject *co,
    int curr_stackentries,
    _Py_UopsSymbol **args,
    int arg_len);
extern int _Py_uop_frame_pop(_Py_UOpsContext *ctx);

PyAPI_FUNC(PyObject *) _Py_uop_symbols_test(PyObject *self, PyObject *ignored);

PyAPI_FUNC(int) _PyOptimizer_Optimize(_PyInterpreterFrame *frame, _Py_CODEUNIT *start, PyObject **stack_pointer, _PyExecutorObject **exec_ptr);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_OPTIMIZER_H */
