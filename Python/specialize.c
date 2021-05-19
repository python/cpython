
#define PY_LOCAL_AGGRESSIVE

#include "Python.h"
#include "pycore_code.h"


/* We layout the quickened data as a bi-directional array:
 * The nth instruction is located at ((_Py_CODEUNIT *)quickened)[n]
 * The nth cache is is located at ((HotPyCacheEntry *)quickened)[-1-n]
 * The first cache is reserved for the cache_count, to enable finding
 * the first instruction.
 * In order to not violate aliasing rules, we need to use the
 * HotPyCacheOrInstruction union to refer to the data.
 */

static HotPyCacheOrInstruction *
allocate(int cache_count, int instruction_count)
{
    assert(sizeof(HotPyCacheOrInstruction) == sizeof(void *));
    assert(sizeof(HotPyCacheEntry) == sizeof(void *));
    assert(cache_count > 0);
    int count = cache_count + (instruction_count + INSTRUCTIONS_PER_ENTRY -1)/INSTRUCTIONS_PER_ENTRY;
    HotPyCacheOrInstruction *array = (HotPyCacheOrInstruction *)
        PyMem_Malloc(sizeof(HotPyCacheOrInstruction) * count);
    if (array == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    array[0].entry.zero.cache_count = cache_count;
    return array;
}

static int
get_cache_count(HotPyCacheOrInstruction *quickened) {
    return quickened[0].entry.zero.cache_count;
}

static uint8_t adaptive[256] = {};

static uint8_t cache_requirements[256] = {};

int
_HotPyCacheEntriesNeeded(_Py_CODEUNIT *code, int len)
{
    int cache_offset = 0;
    for (int i = 0; i < len; i++) {
        uint8_t opcode = _Py_OPCODE(code[i]);
        uint8_t need = cache_requirements[opcode];
        if (need == 0) {
            continue;
        }
        assert(adaptive[opcode] != 0);
        int oparg = cache_offset-i/2;
        if (oparg < 0) {
            cache_offset = i/2;
        }
        else if (oparg > 255) {
            /* Cannot access required cache_offset */
            continue;
        }
        cache_offset += need;
    }
    return cache_offset+1;
}


static inline _Py_CODEUNIT *
first_instruction(HotPyCacheOrInstruction *quickened)
{
    return &quickened[get_cache_count(quickened)].code[0];
}

static void
optimize(HotPyCacheOrInstruction *quickened, int len)
{
    _Py_CODEUNIT *instructions = first_instruction(quickened);
    int cache_offset = 0;
    for(int i = 0; i < len; i++) {
        uint8_t opcode = _Py_OPCODE(instructions[i]);
        uint8_t adaptive_opcode = adaptive[opcode];
        if (adaptive_opcode) {
            /* offset = oparg + i/2  => oparg = offset - i/2 */
            int oparg = cache_offset-i/2;
            if (oparg < 0) {
                cache_offset = i/2;
                oparg = 0;
            }
            else if (oparg > 255) {
                /* Cannot access required cache_offset */
                continue;
            }
            instructions[i] = _Py_INSTRUCTION(adaptive_opcode, oparg);
            cache_offset += cache_requirements[opcode];
        }
        else {
            /* Insert superinstructions here */
        }
    }
    assert(cache_offset+1 == get_cache_count(quickened));
}

int
_HotPy_Quicken(PyCodeObject *code) {
    Py_ssize_t size = PyBytes_Size(code->co_code);
    int len = (int)(size/sizeof(_Py_CODEUNIT));
    if (len > MAX_SIZE_TO_QUICKEN) {
        code->co_warmup = 255;
        return 0;
    }
    if (code->co_quickened) {
        return 0;
    }
    int cache_count = _HotPyCacheEntriesNeeded(code->co_firstinstr, len);
    HotPyCacheOrInstruction *quickened = allocate(cache_count, len);
    if (quickened == NULL) {
        return -1;
    }
    _Py_CODEUNIT *new_instructions = first_instruction(quickened);
    memcpy(new_instructions, code->co_firstinstr, size);
    optimize(quickened, len);
    code->co_quickened = quickened;
    code->co_firstinstr = new_instructions;
    return 0;
}

