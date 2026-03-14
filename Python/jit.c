#ifdef _Py_JIT

#include "Python.h"

#include "pycore_abstract.h"
#include "pycore_bitutils.h"
#include "pycore_call.h"
#include "pycore_ceval.h"
#include "pycore_critical_section.h"
#include "pycore_dict.h"
#include "pycore_floatobject.h"
#include "pycore_frame.h"
#include "pycore_function.h"
#include "pycore_import.h"
#include "pycore_interpframe.h"
#include "pycore_interpolation.h"
#include "pycore_intrinsics.h"
#include "pycore_lazyimportobject.h"
#include "pycore_list.h"
#include "pycore_long.h"
#include "pycore_mmap.h"
#include "pycore_opcode_metadata.h"
#include "pycore_opcode_utils.h"
#include "pycore_optimizer.h"
#include "pycore_pyerrors.h"
#include "pycore_setobject.h"
#include "pycore_sliceobject.h"
#include "pycore_template.h"
#include "pycore_tuple.h"
#include "pycore_unicodeobject.h"

#include "pycore_jit.h"
#include "pycore_uop_metadata.h"

// Memory management stuff: ////////////////////////////////////////////////////

#ifndef MS_WINDOWS
    #include <sys/mman.h>
#endif

static size_t
get_page_size(void)
{
#ifdef MS_WINDOWS
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
#else
    return sysconf(_SC_PAGESIZE);
#endif
}

static void
jit_error(const char *message)
{
#ifdef MS_WINDOWS
    int hint = GetLastError();
#else
    int hint = errno;
#endif
    PyErr_Format(PyExc_RuntimeWarning, "JIT %s (%d)", message, hint);
}

static size_t _Py_jit_shim_size = 0;

static int
address_in_executor_list(_PyExecutorObject *head, uintptr_t addr)
{
    for (_PyExecutorObject *exec = head;
         exec != NULL;
         exec = exec->vm_data.links.next)
    {
        if (exec->jit_code == NULL || exec->jit_size == 0) {
            continue;
        }
        uintptr_t start = (uintptr_t)exec->jit_code;
        uintptr_t end = start + exec->jit_size;
        if (addr >= start && addr < end) {
            return 1;
        }
    }
    return 0;
}

PyAPI_FUNC(int)
_PyJIT_AddressInJitCode(PyInterpreterState *interp, uintptr_t addr)
{
    if (interp == NULL) {
        return 0;
    }
    if (_Py_jit_entry != _Py_LazyJitShim && _Py_jit_shim_size != 0) {
        uintptr_t start = (uintptr_t)_Py_jit_entry;
        uintptr_t end = start + _Py_jit_shim_size;
        if (addr >= start && addr < end) {
            return 1;
        }
    }
    if (address_in_executor_list(interp->executor_list_head, addr)) {
        return 1;
    }
    if (address_in_executor_list(interp->executor_deletion_list_head, addr)) {
        return 1;
    }
    return 0;
}

// Next mmap hint address for placing JIT code near CPython text.
// File-scope so jit_shrink() can rewind it when releasing unused pages.
#if defined(__linux__) && defined(__x86_64__)
static uintptr_t jit_next_hint = 0;
#endif

static unsigned char *
jit_alloc(size_t size)
{
    assert(size);
    assert(size % get_page_size() == 0);
#ifdef MS_WINDOWS
    int flags = MEM_COMMIT | MEM_RESERVE;
    unsigned char *memory = VirtualAlloc(NULL, size, flags, PAGE_READWRITE);
    int failed = memory == NULL;
#else
    int flags = MAP_ANONYMOUS | MAP_PRIVATE;
    int prot = PROT_READ | PROT_WRITE;
    void *hint = NULL;
#if defined(__linux__) && defined(__x86_64__)
    // Allocate JIT code near CPython text so emit_call_ext and emit_mov_imm
    // can use short RIP-relative encodings (within ±2GB).
    {
        if (jit_next_hint == 0) {
            size_t page_size = get_page_size();
            extern char _end[];
            // Start 25MB after the end of CPython text, rounded up to the next page.
            jit_next_hint = ((uintptr_t)_end + 25000000 + page_size - 1) & ~(uintptr_t)(page_size - 1);
        }
        hint = (void *)jit_next_hint;
    }
#endif
    unsigned char *memory = mmap(hint, size, prot, flags, -1, 0);
    if (memory == MAP_FAILED && hint != NULL) {
        memory = mmap(NULL, size, prot, flags, -1, 0);
    }
    int failed = memory == MAP_FAILED;
#if defined(__linux__) && defined(__x86_64__)
    if (!failed) {
        jit_next_hint = (uintptr_t)memory + size;
    }
#endif
    if (!failed) {
        (void)_PyAnnotateMemoryMap(memory, size, "cpython:jit");
    }
#endif
    if (failed) {
        jit_error("unable to allocate memory");
        return NULL;
    }
    return memory;
}

// Shrink a JIT allocation by releasing unused tail pages back to the OS.
// Updates jit_next_hint so the next allocation continues right after the
// trimmed region (avoids leaving gaps in the address space).
static void
jit_shrink(unsigned char *memory, size_t alloc_size, size_t used_size)
{
    assert(used_size <= alloc_size);
    assert(used_size % get_page_size() == 0);
    assert(alloc_size % get_page_size() == 0);
    if (used_size < alloc_size) {
#ifdef MS_WINDOWS
        VirtualFree(memory + used_size, alloc_size - used_size, MEM_DECOMMIT);
#else
        munmap(memory + used_size, alloc_size - used_size);
#endif
#if defined(__linux__) && defined(__x86_64__)
        // Rewind hint so the next allocation fills the gap we just freed.
        if (jit_next_hint == (uintptr_t)memory + alloc_size) {
            jit_next_hint = (uintptr_t)memory + used_size;
        }
#endif
    }
}

static int
jit_free(unsigned char *memory, size_t size)
{
    assert(size);
    assert(size % get_page_size() == 0);
#ifdef MS_WINDOWS
    int failed = !VirtualFree(memory, 0, MEM_RELEASE);
#else
    int failed = munmap(memory, size);
#endif
    if (failed) {
        jit_error("unable to free memory");
        return -1;
    }
    OPT_STAT_ADD(jit_freed_memory_size, size);
    return 0;
}

static int
mark_executable(unsigned char *memory, size_t size)
{
    if (size == 0) {
        return 0;
    }
    assert(size % get_page_size() == 0);
    // Do NOT ever leave the memory writable! Also, don't forget to flush the
    // i-cache (I cannot begin to tell you how horrible that is to debug):
#ifdef MS_WINDOWS
    if (!FlushInstructionCache(GetCurrentProcess(), memory, size)) {
        jit_error("unable to flush instruction cache");
        return -1;
    }
    DWORD old;
    int failed = !VirtualProtect(memory, size, PAGE_EXECUTE_READ, &old);
#else
    __builtin___clear_cache((char *)memory, (char *)memory + size);
    int failed = mprotect(memory, size, PROT_EXEC | PROT_READ);
#endif
    if (failed) {
        jit_error("unable to protect executable memory");
        return -1;
    }
    return 0;
}

// JIT compiler stuff: /////////////////////////////////////////////////////////
//
// DynASM-based JIT: We use Clang to compile each uop template to optimized
// assembly at build time, convert the assembly to DynASM directives via
// _asm_to_dasc.py, and run the DynASM preprocessor (dynasm.lua) to produce
// jit_stencils.h containing an action list and per-uop emit functions.
//
// At runtime, DynASM's tiny encoding engine (dasm_x86.h) assembles the trace
// by replaying the action list with concrete operand values, resolving labels
// and branches automatically.  This replaces the entire copy-and-patch
// relocation layer: no more patch_* functions, no trampolines, no GOT.

#include "dasm_proto.h"

// DynASM configuration: Dst is always dasm_State** passed as first argument
// to emit functions.
#define Dst_DECL    dasm_State **Dst
#define Dst_REF     (*Dst)

#include "dasm_x86.h"
#include "jit_stencils.h"

// Compiles executor in-place using DynASM.
//
// The DynASM flow:
//   1. Initialize DynASM state and pre-allocate PC labels for all uops
//      plus their internal branch targets.
//   2. Emit each uop stencil via the generated emit_*() functions.  These
//      call dasm_put() to append encoded instructions to the action buffer,
//      using PC labels for inter-uop jumps and DynASM sections for hot/cold
//      code separation.
//   3. Append a _FATAL_ERROR sentinel after the last uop to catch overruns.
//   4. dasm_link() computes the final code layout and resolves all labels.
//   5. Allocate executable memory (page-aligned) and dasm_encode() into it.
//   6. Mark memory executable and shrink unused pages.
//
// This replaces the old copy-and-patch approach and eliminates all manual
// relocation patching, GOT/trampoline generation.

/* Emit all uop stencils (Phase 3-4) into the DynASM state.
 *
 * Handles _SET_IP delta encoding, shared trace cleanup stubs, and the
 * _FATAL_ERROR sentinel.
 */
static void
emit_trace(dasm_State **Dst,
           const _PyUOpInstruction *trace, size_t length)
{
    int sentinel_label = (int)length;
    int label_base = sentinel_label + 1;
    uintptr_t last_ip = 0;  // track last _SET_IP value for delta encoding

    emit_trace_entry_frame(Dst);

    for (size_t i = 0; i < length; i++) {
        const _PyUOpInstruction *instruction = &trace[i];
        int uop_label = (int)i;
        int continue_label = (int)(i + 1);

        int opcode = instruction->opcode;
        if ((opcode == _SET_IP_r00 || opcode == _SET_IP_r11
             || opcode == _SET_IP_r22 || opcode == _SET_IP_r33)
            && last_ip != 0)
        {
            uintptr_t new_ip = (uintptr_t)instruction->operand0;
            intptr_t delta = (intptr_t)(new_ip - last_ip);
            if (delta != 0
                && delta >= INT32_MIN && delta <= INT32_MAX)
            {
                emit_set_ip_delta(Dst, uop_label, delta);
                label_base += jit_internal_label_count(opcode);
                last_ip = new_ip;
                // SET_IP delta only modifies [r13+56], preserves rax
                continue;
            }
        }

        jit_emit_one(Dst, instruction->opcode, instruction,
                     uop_label, continue_label, label_base);
        label_base += jit_internal_label_count(instruction->opcode);
        if (opcode == _SET_IP_r00 || opcode == _SET_IP_r11
            || opcode == _SET_IP_r22 || opcode == _SET_IP_r33)
        {
            last_ip = (uintptr_t)instruction->operand0;
        }
        else if (jit_invalidates_ip(opcode)) {
            last_ip = 0;
        }
    }

    // Emit _FATAL_ERROR sentinel after the last uop to catch overruns
    {
        _PyUOpInstruction sentinel = {0};
        sentinel.opcode = _FATAL_ERROR_r00;
        int sentinel_continue = sentinel_label;
        jit_emit_one(Dst, _FATAL_ERROR_r00, &sentinel,
                     sentinel_label, sentinel_continue, label_base);
    }
}

/* Initialize a DynASM state for trace compilation. */
static void
init_dasm(dasm_State **Dst, int total_labels)
{
    dasm_init(Dst, DASM_MAXSECTION);
    dasm_setup(Dst, jit_actionlist);
    dasm_growpc(Dst, total_labels);
}

int
_PyJIT_Compile(_PyExecutorObject *executor, const _PyUOpInstruction trace[], size_t length)
{
    // Phase 1: Count total PC labels needed.
    // Labels [0..length-1] are uop entry points; additional labels are
    // allocated for internal branch targets within each stencil.
    int total_labels = (int)length;
    for (size_t i = 0; i < length; i++) {
        total_labels += jit_internal_label_count(trace[i].opcode);
    }
    // One extra label for the _FATAL_ERROR sentinel.
    total_labels += 1;
    // Extra internal labels for _FATAL_ERROR
    total_labels += jit_internal_label_count(_FATAL_ERROR_r00);

    // Phase 2–6: Single-pass JIT compilation.
    //
    // Allocate PY_MAX_JIT_CODE_SIZE up front.  Since jit_alloc() places
    // code near CPython text (via mmap hints on Linux x86-64), the real
    // allocation address is always usable as jit_code_base — emit_mov_imm()
    // and emit_call_ext() will use short RIP-relative encodings.
    //
    // After encoding, unused tail pages are released back to the OS and
    // jit_next_hint is rewound so the next allocation fills the gap.
    dasm_State *d;
    size_t code_size;
    int status;

    size_t page_size = get_page_size();
    assert((page_size & (page_size - 1)) == 0);
    size_t alloc_size = (PY_MAX_JIT_CODE_SIZE + page_size - 1) & ~(page_size - 1);
    unsigned char *memory = jit_alloc(alloc_size);
    if (memory == NULL) {
        return -1;
    }

    jit_code_base = (uintptr_t)memory;

    init_dasm(&d, total_labels);
    emit_trace(&d, trace, length);
    status = dasm_link(&d, &code_size);
    if (status != DASM_S_OK) {
        jit_free(memory, alloc_size);
        dasm_free(&d);
        PyErr_Format(PyExc_RuntimeWarning,
                     "JIT DynASM link failed (status %d)", status);
        return -1;
    }
    if (code_size > PY_MAX_JIT_CODE_SIZE) {
        // Trace too large — give up on this trace.
        jit_free(memory, alloc_size);
        dasm_free(&d);
        jit_error("code too big; refactor bytecodes.c to keep uop size down, or reduce maximum trace length.");
        return -1;
    }
    if (code_size > alloc_size) {
        // Trace too large — give up on this trace.
        jit_free(memory, alloc_size);
        dasm_free(&d);
        PyErr_Format(PyExc_RuntimeWarning,
                     "JIT code too large (%zu bytes)", code_size);
        return -1;
    }

    // Phase 7: Encode — writes final machine code into memory.
    status = dasm_encode(&d, memory);
    if (status != DASM_S_OK) {
        jit_free(memory, alloc_size);
        dasm_free(&d);
        PyErr_Format(PyExc_RuntimeWarning,
                     "JIT DynASM encode failed (status %d)", status);
        return -1;
    }

    dasm_free(&d);

    // Release unused tail pages and rewind jit_next_hint.
    size_t total_size = (code_size + page_size - 1) & ~(page_size - 1);
    jit_shrink(memory, alloc_size, total_size);

    // Collect memory stats
    OPT_STAT_ADD(jit_total_memory_size, total_size);
    OPT_STAT_ADD(jit_code_size, code_size);
    OPT_STAT_ADD(jit_padding_size, total_size - code_size);
    OPT_HIST(total_size, trace_total_memory_hist);

    if (mark_executable(memory, total_size)) {
        jit_free(memory, total_size);
        return -1;
    }

    executor->jit_code = memory;
    executor->jit_size = total_size;
    return 0;
}

/* One-off compilation of the jit entry shim.
 *
 * The shim bridges the native C calling convention to the JIT's internal
 * calling convention.  It is compiled once and shared across all traces.
 * Uses DynASM just like trace compilation, but with a single emit_shim()
 * call instead of a loop over uops.
 */
static _PyJitEntryFuncPtr
compile_shim(void)
{
    int total_labels = 1 + jit_internal_label_count_shim();
    dasm_State *d;
    size_t code_size;
    int status;

    // The shim is tiny (~100 bytes).  Allocate one page, compile once.
    size_t page_size = get_page_size();
    size_t alloc_size = page_size;
    unsigned char *memory = jit_alloc(alloc_size);
    if (memory == NULL) {
        return NULL;
    }

    jit_code_base = (uintptr_t)memory;

    init_dasm(&d, total_labels);
    emit_shim(&d, 0, 1);
    status = dasm_link(&d, &code_size);
    if (status != DASM_S_OK) {
        jit_free(memory, alloc_size);
        dasm_free(&d);
        return NULL;
    }
    assert(code_size <= alloc_size);

    status = dasm_encode(&d, memory);
    dasm_free(&d);
    if (status != DASM_S_OK) {
        jit_free(memory, alloc_size);
        return NULL;
    }

    if (mark_executable(memory, alloc_size)) {
        jit_free(memory, alloc_size);
        return NULL;
    }
    _Py_jit_shim_size = alloc_size;
    return (_PyJitEntryFuncPtr)memory;
}

static PyMutex lazy_jit_mutex = { 0 };

_Py_CODEUNIT *
_Py_LazyJitShim(
    _PyExecutorObject *executor, _PyInterpreterFrame *frame, _PyStackRef *stack_pointer, PyThreadState *tstate
) {
    PyMutex_Lock(&lazy_jit_mutex);
    if (_Py_jit_entry == _Py_LazyJitShim) {
        _PyJitEntryFuncPtr shim = compile_shim();
        if (shim == NULL) {
            PyMutex_Unlock(&lazy_jit_mutex);
            Py_FatalError("Cannot allocate core JIT code");
        }
        _Py_jit_entry = shim;
    }
    PyMutex_Unlock(&lazy_jit_mutex);
    return _Py_jit_entry(executor, frame, stack_pointer, tstate);
}

// Free executor's memory allocated with _PyJIT_Compile
void
_PyJIT_Free(_PyExecutorObject *executor)
{
    unsigned char *memory = (unsigned char *)executor->jit_code;
    size_t size = executor->jit_size;
    if (memory) {
        executor->jit_code = NULL;
        executor->jit_size = 0;
        if (jit_free(memory, size)) {
            PyErr_FormatUnraisable("Exception ignored while "
                                   "freeing JIT memory");
        }
    }
}

// Free shim memory allocated with compile_shim
void
_PyJIT_Fini(void)
{
    PyMutex_Lock(&lazy_jit_mutex);
    unsigned char *memory = (unsigned char *)_Py_jit_entry;
    size_t size = _Py_jit_shim_size;
    if (size) {
        _Py_jit_entry = _Py_LazyJitShim;
        _Py_jit_shim_size = 0;
        if (jit_free(memory, size)) {
            PyErr_FormatUnraisable("Exception ignored while "
                                   "freeing JIT entry code");
        }
    }
    PyMutex_Unlock(&lazy_jit_mutex);
}

#endif  // _Py_JIT
