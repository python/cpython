typedef enum {
    HOLE_base,
    HOLE_continue,
    HOLE_next_instr,
    HOLE_next_trace,
    HOLE_oparg,
    LOAD__Py_Dealloc,
    LOAD__Py_DecRefTotal_DO_NOT_USE_THIS,
    LOAD__Py_NegativeRefcount,
    LOAD_PyThreadState_Get,
} HoleKind;

typedef struct {
    const uintptr_t offset;
    const uintptr_t addend;
    const HoleKind kind;
} Hole;

typedef struct {
    const size_t nbytes;
    const unsigned char * const bytes;
    const size_t nholes;
    const Hole * const holes;
} Stencil;
