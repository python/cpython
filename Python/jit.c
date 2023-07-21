#include "Python.h"
#include "pycore_abstract.h"
#include "pycore_ceval.h"
#include "pycore_opcode.h"
#include "pycore_opcode_metadata.h"
#include "pycore_uops.h"
#include "pycore_jit.h"

#include "ceval_macros.h"
#include "jit_stencils.h"

#ifdef MS_WINDOWS
    #include <psapi.h>
    #include <windows.h>
    FARPROC
    LOOKUP(LPCSTR symbol)
    {
        DWORD cbNeeded;
        HMODULE hMods[1024];
        HANDLE hProcess = GetCurrentProcess();
        if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
            for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
                FARPROC value = GetProcAddress(hMods[i], symbol);
                if (value) {
                    return value;
                }
            }
        }
        return NULL;
    }
#else
    #include <sys/mman.h>
    #define LOOKUP(SYMBOL) dlsym(RTLD_DEFAULT, (SYMBOL))
#endif

#define JIT_POOL_SIZE  (1 << 30)
#define JIT_ALIGN      (1 << 14)
#define JIT_ALIGN_MASK (JIT_ALIGN - 1)

static unsigned char pool[JIT_POOL_SIZE];
static size_t pool_head;

static unsigned char *
alloc(size_t size)
{
    size_t padding = (JIT_ALIGN - ((uintptr_t)(pool + pool_head) & JIT_ALIGN_MASK)) & JIT_ALIGN_MASK;
    if (JIT_POOL_SIZE < pool_head + padding + size) {
        return NULL;
    }
    unsigned char *memory = pool + pool_head + padding;
    assert(((uintptr_t)memory & JIT_ALIGN_MASK) == 0);
    pool_head += padding + size;
    return memory;
}

static int stencils_loaded = 0;

static void
patch_one(unsigned char *location, HoleKind kind, uint64_t value, uint64_t addend)
{
    switch (kind) {
        case PATCH_ABS_12: {
            uint32_t *addr = (uint32_t *)location;
            uint32_t instruction = *addr;
            assert(((instruction & 0x3B000000) == 0x39000000) ||
                   ((instruction & 0x11C00000) == 0x11000000));
            value = (value + addend) & ((1 << 12) - 1);
            int implicit_shift = 0;
            if ((instruction & 0x3B000000) == 0x39000000) {
                implicit_shift = ((instruction >> 30) & 0x3);
                // XXX: We shouldn't have to rewrite these (we *should* be
                // able to just assert the alignment), but something's up with
                // aarch64 + ELF (at least with LLVM 14, I haven't tested 15
                // and 16):
                switch (implicit_shift) {
                    case 3:
                        if ((value & 0x7) == 0) {
                            break;
                        }
                        implicit_shift = 2;
                        instruction = (instruction & ~(0x3UL << 30)) | (implicit_shift << 30);
                        // Fall through...
                    case 2:
                        if ((value & 0x3) == 0) {
                            break;
                        }
                        implicit_shift = 1;
                        instruction = (instruction & ~(0x3UL << 30)) | (implicit_shift << 30);
                        // Fall through...
                    case 1:
                        if ((value & 0x1) == 0) {
                            break;
                        }
                        implicit_shift = 0;
                        instruction = (instruction & ~(0x3UL << 30)) | (implicit_shift << 30);
                        // Fall through...
                    case 0:
                        if ((instruction & 0x04800000) == 0x04800000) {
                            implicit_shift = 4;
                            assert(((value & 0xF) == 0));
                        }
                        break;
                }
            }
            value >>= implicit_shift;
            assert((value & ((1 << 12) - 1)) == value);
            instruction = (instruction & 0xFFC003FF) | ((uint32_t)(value << 10) & 0x003FFC00);
            assert(((instruction & 0x3B000000) == 0x39000000) ||
                   ((instruction & 0x11C00000) == 0x11000000));
            *addr = instruction;
            break;
        }
        case PATCH_ABS_16_A: {
            uint32_t *addr = (uint32_t *)location;
            uint32_t instruction = *addr;
            assert(((instruction >> 21) & 0x3) == 0);
            instruction = (instruction & 0xFFE0001F) | ((((value + addend) >> 0) & 0xFFFF) << 5);
            *addr = instruction;
            break;
        }
        case PATCH_ABS_16_B: {
            uint32_t *addr = (uint32_t *)location;
            uint32_t instruction = *addr;
            assert(((instruction >> 21) & 0x3) == 1);
            instruction = (instruction & 0xFFE0001F) | ((((value + addend) >> 16) & 0xFFFF) << 5);
            *addr = instruction;
            break;
        }
        case PATCH_ABS_16_C: {
            uint32_t *addr = (uint32_t *)location;
            uint32_t instruction = *addr;
            assert(((instruction >> 21) & 0x3) == 2);
            instruction = (instruction & 0xFFE0001F) | ((((value + addend) >> 32) & 0xFFFF) << 5);
            *addr = instruction;
            break;
        }
        case PATCH_ABS_16_D: {
            uint32_t *addr = (uint32_t *)location;
            uint32_t instruction = *addr;
            assert(((instruction >> 21) & 0x3) == 3);
            instruction = (instruction & 0xFFE0001F) | ((((value + addend) >> 48) & 0xFFFF) << 5);
            *addr = instruction;
            break;
        }
        case PATCH_ABS_32: {
            uint32_t *addr = (uint32_t *)location;
            uint32_t instruction = *addr;
            instruction = value + addend;
            *addr = instruction;
            break;
        }
        case PATCH_ABS_64: {
            uint64_t *addr = (uint64_t *)location;
            uint64_t instruction = *addr;
            instruction = value + addend;
            *addr = instruction;
            break;
        }
        case PATCH_REL_21: {
            uint32_t *addr = (uint32_t *)location;
            uint32_t instruction = *addr;
            assert((instruction & 0x9F000000) == 0x90000000);
            value = (((value + addend) >> 12) << 12) - (((uintptr_t)location >> 12) << 12);
            assert((value & 0xFFF) == 0);
            // assert((value & ((1ULL << 33) - 1)) == value);  // XXX: This should be signed.
            uint32_t lo = ((uint64_t)value << 17) & 0x60000000;
            uint32_t hi = ((uint64_t)value >> 9) & 0x00FFFFE0;
            instruction = (instruction & 0x9F00001F) | hi | lo;
            assert((instruction & 0x9F000000) == 0x90000000);
            *addr = instruction;
            break;
        }
        case PATCH_REL_26: {
            uint32_t *addr = (uint32_t *)location;
            uint32_t instruction = *addr;
            assert(((instruction & 0xFC000000) == 0x14000000) ||
                   ((instruction & 0xFC000000) == 0x94000000));
            value = value + addend - (uintptr_t)location;
            assert((value & 0x3) == 0);
            // assert((value & ((1ULL << 29) - 1)) == value);  // XXX: This should be signed.
            instruction = (instruction & 0xFC000000) | ((uint32_t)(value >> 2) & 0x03FFFFFF);
            assert(((instruction & 0xFC000000) == 0x14000000) ||
                   ((instruction & 0xFC000000) == 0x94000000));
            *addr = instruction;
            break;
        }
        case PATCH_REL_32: {
            uint32_t *addr = (uint32_t *)location;
            uint32_t instruction = *addr;
            instruction = value + addend - (uintptr_t)location;
            *addr = instruction;
            break;
        }
        case PATCH_REL_64: {
            uint64_t *addr = (uint64_t *)location;
            uint64_t instruction = *addr;
            instruction = value + addend - (uintptr_t)location;
            *addr = instruction;
            break;
        }
        default: {
            printf("XXX: %d!\n", kind);
            Py_UNREACHABLE();
        }
    }
}

static int
preload_stencil(const Stencil *loading)
{
    for (size_t i = 0; i < loading->nloads; i++) {
        const SymbolLoad *load = &loading->loads[i];
        uintptr_t value = (uintptr_t)LOOKUP(load->symbol);
        if (value == 0) {
            const char *w = "JIT initialization failed (can't find symbol \"%s\")";
            PyErr_WarnFormat(PyExc_RuntimeWarning, 0, w, load->symbol);
            return -1;
        }
    }
    return 0;
}

static void
copy_and_patch(unsigned char *memory, const Stencil *stencil, uintptr_t patches[])
{
    memcpy(memory, stencil->bytes, stencil->nbytes);
    for (size_t i = 0; i < stencil->nholes; i++) {
        const Hole *hole = &stencil->holes[i];
        patch_one(memory + hole->offset, hole->kind, patches[hole->value], hole->addend);
    }
    for (size_t i = 0; i < stencil->nloads; i++) {
        const SymbolLoad *load = &stencil->loads[i];
        // XXX: Cache these somehow...
        uintptr_t value = (uintptr_t)LOOKUP(load->symbol);
        patch_one(memory + load->offset, load->kind, value, load->addend);
    }
}

// The world's smallest compiler?
_PyJITFunction
_PyJIT_CompileTrace(_PyUOpInstruction *trace, int size)
{
    if (stencils_loaded < 0) {
        return NULL;
    }
    if (stencils_loaded == 0) {
        stencils_loaded = -1;
        for (size_t i = 0; i < Py_ARRAY_LENGTH(stencils); i++) {
            if (preload_stencil(&stencils[i])) {
                return NULL;
            }
        }
        if (preload_stencil(&trampoline_stencil)) {
            return NULL;
        }
        stencils_loaded = 1;
    }
    assert(stencils_loaded > 0);
    // First, loop over everything once to find the total compiled size:
    size_t nbytes = trampoline_stencil.nbytes;
    for (int i = 0; i < size; i++) {
        _PyUOpInstruction *instruction = &trace[i];
        const Stencil *stencil = &stencils[instruction->opcode];
        if (stencil->nbytes == 0) {
            return NULL;
        }
        nbytes += stencil->nbytes;
    };
    unsigned char *memory = alloc(nbytes);
    if (memory == NULL) {
        return NULL;
    }
#ifdef __APPLE__
    void *mapped = mmap(memory, nbytes, PROT_READ | PROT_WRITE,
                        MAP_ANONYMOUS | MAP_FIXED | MAP_PRIVATE, -1, 0);
    if (mapped == MAP_FAILED) {
        return NULL;
    }
    assert(memory == mapped);
#endif
    unsigned char *head = memory;
    uintptr_t patches[] = GET_PATCHES();
    // First, the trampoline:
    const Stencil *stencil = &trampoline_stencil;
    patches[HOLE_base] = (uintptr_t)head;
    patches[HOLE_continue] = (uintptr_t)head + stencil->nbytes;
    copy_and_patch(head, stencil, patches);
    head += stencil->nbytes;
    // Then, all of the stencils:
    for (int i = 0; i < size; i++) {
        _PyUOpInstruction *instruction = &trace[i];
        const Stencil *stencil = &stencils[instruction->opcode];
        patches[HOLE_base] = (uintptr_t)head;
        patches[HOLE_continue] = (i != size - 1)
                               ? (uintptr_t)head + stencil->nbytes
                               : (uintptr_t)memory + trampoline_stencil.nbytes;
        patches[HOLE_oparg_plus_one] = instruction->oparg + 1;
        patches[HOLE_operand_plus_one] = instruction->operand + 1;
        patches[HOLE_pc_plus_one] = i + 1;
        copy_and_patch(head, stencil, patches);
        head += stencil->nbytes;
    };
#ifdef MS_WINDOWS
    DWORD old = PAGE_READWRITE;
    if (!VirtualProtect(memory, nbytes, PAGE_EXECUTE_READ, &old)) {
        return NULL;
    }
#else
    if (mprotect(memory, nbytes, PROT_EXEC | PROT_READ)) {
        return NULL;
    }
#endif
    // Wow, done already?
    assert(memory + nbytes == head);
    return (_PyJITFunction)memory;
}
