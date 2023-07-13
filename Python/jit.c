#include "Python.h"
#include "pycore_abstract.h"
#include "pycore_ceval.h"
#include "pycore_jit.h"
#include "pycore_opcode.h"

#include "ceval_macros.h"
#include "jit_stencils.h"

// XXX: Pre-populate symbol addresses once.

#ifdef MS_WINDOWS
    #include <psapi.h>
    #include <windows.h>
    FARPROC
    dlsym(LPCSTR symbol)
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
    BOOL
    mprotect(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect)
    {
        DWORD flOldProtect = PAGE_READWRITE;
        return !VirtualProtect(lpAddress, dwSize, flNewProtect, &flOldProtect);
    }
    #define DLSYM(SYMBOL) \
        dlsym((SYMBOL))
    #define MAP_FAILED NULL
    #define MMAP(SIZE) \
        VirtualAlloc(NULL, (SIZE), MEM_COMMIT, PAGE_READWRITE)
    #define MPROTECT(MEMORY, SIZE) \
        mprotect((MEMORY), (SIZE), PAGE_EXECUTE_READ)
    #define MUNMAP(MEMORY, SIZE) \
        VirtualFree((MEMORY), 0, MEM_RELEASE)
#else
    #include <sys/mman.h>
    #define DLSYM(SYMBOL) \
        dlsym(RTLD_DEFAULT, (SYMBOL))
    #define MMAP(SIZE)                             \
        mmap(NULL, (SIZE), PROT_READ | PROT_WRITE, \
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
    #define MPROTECT(MEMORY, SIZE) \
        mprotect((MEMORY), (SIZE), PROT_READ | PROT_EXEC)
    #define MUNMAP(MEMORY, SIZE) \
        munmap((MEMORY), (SIZE))
#endif

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
            if ((value + addend) & 0x3) {  // XXX: Remote debugging info.
                printf("PATCH_ABS_12: unaligned value %llu + %llu (at %p)\n", value, addend, location);
                abort();
            };
            value = (value + addend) & ((1 << 12) - 1);
            int implicit_shift = 0;
            if ((instruction & 0x3B000000) == 0x39000000) {
                implicit_shift = ((instruction >> 30) & 0x3);
                switch (implicit_shift) {
                    case 0:
                        if ((instruction & 0x04800000) == 0x04800000) {
                            implicit_shift = 4;
                            assert(((value & 0xF) == 0));
                        }
                        break;
                    case 1:
                        assert(((value & 0x1) == 0));
                        break;
                    case 2:
                        assert(((value & 0x3) == 0));
                        break;
                    case 3:
                        assert(((value & 0x7) == 0));
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
        uintptr_t value = (uintptr_t)DLSYM(load->symbol);
        if (value == 0) {
            printf("XXX: Failed to preload symbol %s!\n", load->symbol);
            return -1;
        }
        patch_one(loading->bytes + load->offset, load->kind, value, load->addend);
    }
    return 0;
}

static unsigned char *
alloc(size_t nbytes)
{
    nbytes += sizeof(size_t);
    unsigned char *memory = MMAP(nbytes);
    if (memory == MAP_FAILED) {
        return NULL;
    }
    assert(memory);
    *(size_t *)memory = nbytes;
    return memory + sizeof(size_t);
}


void
_PyJIT_Free(_PyJITFunction trace)
{
    unsigned char *memory = (unsigned char *)trace - sizeof(size_t);
    size_t nbytes = *(size_t *)memory;
    MUNMAP(memory, nbytes);
}


static void
copy_and_patch(unsigned char *memory, const Stencil *stencil, uintptr_t patches[])
{
    memcpy(memory, stencil->bytes, stencil->nbytes);
    for (size_t i = 0; i < stencil->nholes; i++) {
        const Hole *hole = &stencil->holes[i];
        patch_one(memory + hole->offset, hole->kind, patches[hole->value], hole->addend);
    }
}

// The world's smallest compiler?
// Make sure to call _PyJIT_Free on the memory when you're done with it!
_PyJITFunction
_PyJIT_CompileTrace(int size, _Py_CODEUNIT **trace)
{
    if (!stencils_loaded) {
        stencils_loaded = 1;
        for (size_t i = 0; i < Py_ARRAY_LENGTH(stencils); i++) {
            if (preload_stencil(&stencils[i])) {
                stencils_loaded = -1;
                break;
            }
        }
    }
    if (stencils_loaded < 0) {
        printf("XXX: JIT disabled!\n");
        return NULL;
    }
    // First, loop over everything once to find the total compiled size:
    size_t nbytes = trampoline_stencil.nbytes;
    for (int i = 0; i < size; i++) {
        _Py_CODEUNIT *instruction = trace[i];
        const Stencil *stencil = &stencils[instruction->op.code];
        if (stencil->nbytes == 0) {
            return NULL;
        }
        nbytes += stencil->nbytes;
    };
    unsigned char *memory = alloc(nbytes);
    if (memory == NULL) {
        return NULL;
    }
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
        _Py_CODEUNIT *instruction = trace[i];
        const Stencil *stencil = &stencils[instruction->op.code];
        patches[HOLE_base] = (uintptr_t)head;
        patches[HOLE_continue] = (i != size - 1)
                               ? (uintptr_t)head + stencil->nbytes
                               : (uintptr_t)memory + trampoline_stencil.nbytes;
        patches[HOLE_next_instr] = (uintptr_t)instruction;
        patches[HOLE_oparg_plus_one] = instruction->op.arg + 1;
        copy_and_patch(head, stencil, patches);
        head += stencil->nbytes;
    };
    // Wow, done already?
    assert(memory + nbytes == head);
    if (MPROTECT(memory - sizeof(size_t), nbytes + sizeof(size_t))) {
        _PyJIT_Free((_PyJITFunction)memory);
        return NULL;
    }
    return (_PyJITFunction)memory;
}
