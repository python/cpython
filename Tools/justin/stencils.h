typedef enum {
    _PyJustin_HOLE_base,
    _PyJustin_HOLE_continue,
    _PyJustin_HOLE_next_instr,
    _PyJustin_HOLE_next_trace_0,
    _PyJustin_HOLE_oparg_0,
    _PyJustin_LOAD__Py_Dealloc,
    _PyJustin_LOAD__Py_DecRefTotal_DO_NOT_USE_THIS,
    _PyJustin_LOAD__Py_NegativeRefcount,
    _PyJustin_LOAD_PyThreadState_Get,
} _PyJustin_HoleKind;

typedef struct {
    const uintptr_t offset;
    const uintptr_t addend;
    const _PyJustin_HoleKind kind;
} _PyJustin_Hole;

typedef struct {
    const size_t size;
    const unsigned char * const body;
    const size_t nholes;
    const _PyJustin_Hole * const holes;
} _PyJustin_Stencil;
