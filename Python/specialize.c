
#include "Python.h"
#include "pycore_code.h"
#include "pycore_dict.h"
#include "pycore_long.h"
#include "pycore_moduleobject.h"
#include "opcode.h"
#include "structmember.h"         // struct PyMemberDef, T_OFFSET_EX

/* For guidance on adding or extending families of instructions see
 * ./adaptive.md
 */


/* We layout the quickened data as a bi-directional array:
 * Instructions upwards, cache entries downwards.
 * first_instr is aligned to a SpecializedCacheEntry.
 * The nth instruction is located at first_instr[n]
 * The nth cache is located at ((SpecializedCacheEntry *)first_instr)[-1-n]
 * The first (index 0) cache entry is reserved for the count, to enable finding
 * the first instruction from the base pointer.
 * The cache_count argument must include space for the count.
 * We use the SpecializedCacheOrInstruction union to refer to the data
 * to avoid type punning.

 Layout of quickened data, each line 8 bytes for M cache entries and N instructions:

 <cache_count>                              <---- co->co_quickened
 <cache M-1>
 <cache M-2>
 ...
 <cache 0>
 <instr 0> <instr 1> <instr 2> <instr 3>    <--- co->co_first_instr
 <instr 4> <instr 5> <instr 6> <instr 7>
 ...
 <instr N-1>
*/

Py_ssize_t _Py_QuickenedCount = 0;
#if SPECIALIZATION_STATS
SpecializationStats _specialization_stats[256] = { 0 };

#define ADD_STAT_TO_DICT(res, field) \
    do { \
        PyObject *val = PyLong_FromUnsignedLongLong(stats->field); \
        if (val == NULL) { \
            Py_DECREF(res); \
            return NULL; \
        } \
        if (PyDict_SetItemString(res, #field, val) == -1) { \
            Py_DECREF(res); \
            Py_DECREF(val); \
            return NULL; \
        } \
        Py_DECREF(val); \
    } while(0);

static PyObject*
stats_to_dict(SpecializationStats *stats)
{
    PyObject *res = PyDict_New();
    if (res == NULL) {
        return NULL;
    }
    ADD_STAT_TO_DICT(res, specialization_success);
    ADD_STAT_TO_DICT(res, specialization_failure);
    ADD_STAT_TO_DICT(res, hit);
    ADD_STAT_TO_DICT(res, deferred);
    ADD_STAT_TO_DICT(res, miss);
    ADD_STAT_TO_DICT(res, deopt);
    ADD_STAT_TO_DICT(res, unquickened);
#if SPECIALIZATION_STATS_DETAILED
    if (stats->miss_types != NULL) {
        if (PyDict_SetItemString(res, "fails", stats->miss_types) == -1) {
            Py_DECREF(res);
            return NULL;
        }
    }
#endif
    return res;
}
#undef ADD_STAT_TO_DICT

static int
add_stat_dict(
    PyObject *res,
    int opcode,
    const char *name) {

    SpecializationStats *stats = &_specialization_stats[opcode];
    PyObject *d = stats_to_dict(stats);
    if (d == NULL) {
        return -1;
    }
    int err = PyDict_SetItemString(res, name, d);
    Py_DECREF(d);
    return err;
}

#if SPECIALIZATION_STATS
PyObject*
_Py_GetSpecializationStats(void) {
    PyObject *stats = PyDict_New();
    if (stats == NULL) {
        return NULL;
    }
    int err = 0;
    err += add_stat_dict(stats, LOAD_ATTR, "load_attr");
    err += add_stat_dict(stats, LOAD_GLOBAL, "load_global");
    err += add_stat_dict(stats, BINARY_SUBSCR, "binary_subscr");
    if (err < 0) {
        Py_DECREF(stats);
        return NULL;
    }
    return stats;
}
#endif


#define PRINT_STAT(name, field) fprintf(out, "    %s." #field " : %" PRIu64 "\n", name, stats->field);

static void
print_stats(FILE *out, SpecializationStats *stats, const char *name)
{
    PRINT_STAT(name, specialization_success);
    PRINT_STAT(name, specialization_failure);
    PRINT_STAT(name, hit);
    PRINT_STAT(name, deferred);
    PRINT_STAT(name, miss);
    PRINT_STAT(name, deopt);
    PRINT_STAT(name, unquickened);
#if SPECIALIZATION_STATS_DETAILED
    if (stats->miss_types == NULL) {
        return;
    }
    fprintf(out, "    %s.fails:\n", name);
    PyObject *key, *count;
    Py_ssize_t pos = 0;
    while (PyDict_Next(stats->miss_types, &pos, &key, &count)) {
        PyObject *type = PyTuple_GetItem(key, 0);
        PyObject *name = PyTuple_GetItem(key, 1);
        PyObject *kind = PyTuple_GetItem(key, 2);
        fprintf(out, "        %s.", ((PyTypeObject *)type)->tp_name);
        PyObject_Print(name, out, Py_PRINT_RAW);
        fprintf(out, " (");
        PyObject_Print(kind, out, Py_PRINT_RAW);
        fprintf(out, "): %ld\n", PyLong_AsLong(count));
    }
#endif
}
#undef PRINT_STAT

void
_Py_PrintSpecializationStats(void)
{
    FILE *out = stderr;
#if SPECIALIZATION_STATS_TO_FILE
    /* Write to a file instead of stderr. */
# ifdef MS_WINDOWS
    const char *dirname = "c:\\temp\\py_stats\\";
# else
    const char *dirname = "/tmp/py_stats/";
# endif
    char buf[48];
    sprintf(buf, "%s%u_%u.txt", dirname, (unsigned)clock(), (unsigned)rand());
    FILE *fout = fopen(buf, "w");
    if (fout) {
        out = fout;
    }
#else
    fprintf(out, "Specialization stats:\n");
#endif
    print_stats(out, &_specialization_stats[LOAD_ATTR], "load_attr");
    print_stats(out, &_specialization_stats[LOAD_GLOBAL], "load_global");
    print_stats(out, &_specialization_stats[BINARY_SUBSCR], "binary_subscr");
    if (out != stderr) {
        fclose(out);
    }
}

#if SPECIALIZATION_STATS_DETAILED
void
_Py_IncrementTypeCounter(int opcode, PyObject *type, PyObject *name, const char *kind)
{
    PyObject *counter = _specialization_stats[opcode].miss_types;
    if (counter == NULL) {
        _specialization_stats[opcode].miss_types = PyDict_New();
        counter = _specialization_stats[opcode].miss_types;
        if (counter == NULL) {
            return;
        }
    }
    PyObject *key = NULL;
    PyObject *kind_object = _PyUnicode_FromASCII(kind, strlen(kind));
    if (kind_object == NULL) {
        PyErr_Clear();
        goto done;
    }
    key = PyTuple_Pack(3, type, name, kind_object);
    if (key == NULL) {
        PyErr_Clear();
        goto done;
    }
    PyObject *count = PyDict_GetItem(counter, key);
    if (count == NULL) {
        count = _PyLong_GetZero();
        if (PyDict_SetItem(counter, key, count) < 0) {
            PyErr_Clear();
            goto done;
        }
    }
    count = PyNumber_Add(count, _PyLong_GetOne());
    if (count == NULL) {
        PyErr_Clear();
        goto done;
    }
    if (PyDict_SetItem(counter, key, count)) {
        PyErr_Clear();
    }
done:
    Py_XDECREF(kind_object);
    Py_XDECREF(key);
}

#define SPECIALIZATION_FAIL(opcode, type, attribute, kind) _Py_IncrementTypeCounter(opcode, (PyObject *)(type), (PyObject *)(attribute), kind)


#endif
#endif

#ifndef SPECIALIZATION_FAIL
#define SPECIALIZATION_FAIL(opcode, type, attribute, kind) ((void)0)
#endif

static SpecializedCacheOrInstruction *
allocate(int cache_count, int instruction_count)
{
    assert(sizeof(SpecializedCacheOrInstruction) == 2*sizeof(int32_t));
    assert(sizeof(SpecializedCacheEntry) == 2*sizeof(int32_t));
    assert(cache_count > 0);
    assert(instruction_count > 0);
    int count = cache_count + (instruction_count + INSTRUCTIONS_PER_ENTRY -1)/INSTRUCTIONS_PER_ENTRY;
    SpecializedCacheOrInstruction *array = (SpecializedCacheOrInstruction *)
        PyMem_Malloc(sizeof(SpecializedCacheOrInstruction) * count);
    if (array == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    _Py_QuickenedCount++;
    array[0].entry.zero.cache_count = cache_count;
    return array;
}

static int
get_cache_count(SpecializedCacheOrInstruction *quickened) {
    return quickened[0].entry.zero.cache_count;
}

/* Map from opcode to adaptive opcode.
  Values of zero are ignored. */
static uint8_t adaptive_opcodes[256] = {
    [LOAD_ATTR] = LOAD_ATTR_ADAPTIVE,
    [LOAD_GLOBAL] = LOAD_GLOBAL_ADAPTIVE,
    [BINARY_SUBSCR] = BINARY_SUBSCR_ADAPTIVE,
};

/* The number of cache entries required for a "family" of instructions. */
static uint8_t cache_requirements[256] = {
    [LOAD_ATTR] = 2, /* _PyAdaptiveEntry and _PyLoadAttrCache */
    [LOAD_GLOBAL] = 2, /* _PyAdaptiveEntry and _PyLoadGlobalCache */
    [BINARY_SUBSCR] = 0,
};

/* Return the oparg for the cache_offset and instruction index.
 *
 * If no cache is needed then return the original oparg.
 * If a cache is needed, but cannot be accessed because
 * oparg would be too large, then return -1.
 *
 * Also updates the cache_offset, as it may need to be incremented by
 * more than the cache requirements, if many instructions do not need caches.
 *
 * See pycore_code.h for details of how the cache offset,
 * instruction index and oparg are related */
static int
oparg_from_instruction_and_update_offset(int index, int opcode, int original_oparg, int *cache_offset) {
    /* The instruction pointer in the interpreter points to the next
     * instruction, so we compute the offset using nexti (index + 1) */
    int nexti = index + 1;
    uint8_t need = cache_requirements[opcode];
    if (need == 0) {
        return original_oparg;
    }
    assert(adaptive_opcodes[opcode] != 0);
    int oparg = oparg_from_offset_and_nexti(*cache_offset, nexti);
    assert(*cache_offset == offset_from_oparg_and_nexti(oparg, nexti));
    /* Some cache space is wasted here as the minimum possible offset is (nexti>>1) */
    if (oparg < 0) {
        oparg = 0;
        *cache_offset = offset_from_oparg_and_nexti(oparg, nexti);
    }
    else if (oparg > 255) {
        return -1;
    }
    *cache_offset += need;
    return oparg;
}

static int
entries_needed(const _Py_CODEUNIT *code, int len)
{
    int cache_offset = 0;
    int previous_opcode = -1;
    for (int i = 0; i < len; i++) {
        uint8_t opcode = _Py_OPCODE(code[i]);
        if (previous_opcode != EXTENDED_ARG) {
            oparg_from_instruction_and_update_offset(i, opcode, 0, &cache_offset);
        }
        previous_opcode = opcode;
    }
    return cache_offset + 1;   // One extra for the count entry
}

static inline _Py_CODEUNIT *
first_instruction(SpecializedCacheOrInstruction *quickened)
{
    return &quickened[get_cache_count(quickened)].code[0];
}

/** Insert adaptive instructions and superinstructions.
 *
 * Skip instruction preceded by EXTENDED_ARG for adaptive
 * instructions as those are both very rare and tricky
 * to handle.
 */
static void
optimize(SpecializedCacheOrInstruction *quickened, int len)
{
    _Py_CODEUNIT *instructions = first_instruction(quickened);
    int cache_offset = 0;
    int previous_opcode = -1;
    for(int i = 0; i < len; i++) {
        int opcode = _Py_OPCODE(instructions[i]);
        int oparg = _Py_OPARG(instructions[i]);
        uint8_t adaptive_opcode = adaptive_opcodes[opcode];
        if (adaptive_opcode && previous_opcode != EXTENDED_ARG) {
            int new_oparg = oparg_from_instruction_and_update_offset(
                i, opcode, oparg, &cache_offset
            );
            if (new_oparg < 0) {
                /* Not possible to allocate a cache for this instruction */
                previous_opcode = opcode;
                continue;
            }
            previous_opcode = adaptive_opcode;
            int entries_needed = cache_requirements[opcode];
            if (entries_needed) {
                /* Initialize the adpative cache entry */
                int cache0_offset = cache_offset-entries_needed;
                SpecializedCacheEntry *cache =
                    _GetSpecializedCacheEntry(instructions, cache0_offset);
                cache->adaptive.original_oparg = oparg;
                cache->adaptive.counter = 0;
            } else {
                // oparg is the adaptive cache counter
                new_oparg = 0;
            }
            instructions[i] = _Py_MAKECODEUNIT(adaptive_opcode, new_oparg);
        }
        else {
            /* Super instructions don't use the cache,
             * so no need to update the offset. */
            switch (opcode) {
                case JUMP_ABSOLUTE:
                    instructions[i] = _Py_MAKECODEUNIT(JUMP_ABSOLUTE_QUICK, oparg);
                    break;
                /* Insert superinstructions here
                 E.g.
                case LOAD_FAST:
                    if (previous_opcode == LOAD_FAST)
                        instructions[i-1] = _Py_MAKECODEUNIT(LOAD_FAST__LOAD_FAST, oparg);
                 */
            }
            previous_opcode = opcode;
        }
    }
    assert(cache_offset+1 == get_cache_count(quickened));
}

int
_Py_Quicken(PyCodeObject *code) {
    if (code->co_quickened) {
        return 0;
    }
    Py_ssize_t size = PyBytes_GET_SIZE(code->co_code);
    int instr_count = (int)(size/sizeof(_Py_CODEUNIT));
    if (instr_count > MAX_SIZE_TO_QUICKEN) {
        code->co_warmup = QUICKENING_WARMUP_COLDEST;
        return 0;
    }
    int entry_count = entries_needed(code->co_firstinstr, instr_count);
    SpecializedCacheOrInstruction *quickened = allocate(entry_count, instr_count);
    if (quickened == NULL) {
        return -1;
    }
    _Py_CODEUNIT *new_instructions = first_instruction(quickened);
    memcpy(new_instructions, code->co_firstinstr, size);
    optimize(quickened, instr_count);
    code->co_quickened = quickened;
    code->co_firstinstr = new_instructions;
    return 0;
}



static int
specialize_module_load_attr(
    PyObject *owner, _Py_CODEUNIT *instr, PyObject *name,
    _PyAdaptiveEntry *cache0, _PyLoadAttrCache *cache1)
{
    PyModuleObject *m = (PyModuleObject *)owner;
    PyObject *value = NULL;
    PyObject *getattr;
    _Py_IDENTIFIER(__getattr__);
    PyDictObject *dict = (PyDictObject *)m->md_dict;
    if (dict == NULL) {
        SPECIALIZATION_FAIL(LOAD_ATTR, Py_TYPE(owner), name, "no __dict__");
        return -1;
    }
    if (dict->ma_keys->dk_kind != DICT_KEYS_UNICODE) {
        SPECIALIZATION_FAIL(LOAD_ATTR, Py_TYPE(owner), name, "non-string keys (or split)");
        return -1;
    }
    getattr = _PyUnicode_FromId(&PyId___getattr__); /* borrowed */
    if (getattr == NULL) {
        SPECIALIZATION_FAIL(LOAD_ATTR, Py_TYPE(owner), name, "module.__getattr__ overridden");
        PyErr_Clear();
        return -1;
    }
    Py_ssize_t index = _PyDict_GetItemHint(dict, getattr, -1,  &value);
    assert(index != DKIX_ERROR);
    if (index != DKIX_EMPTY) {
        SPECIALIZATION_FAIL(LOAD_ATTR, Py_TYPE(owner), name, "module attribute not found");
        return -1;
    }
    index = _PyDict_GetItemHint(dict, name, -1, &value);
    assert (index != DKIX_ERROR);
    if (index != (uint16_t)index) {
        SPECIALIZATION_FAIL(LOAD_ATTR, Py_TYPE(owner), name, "index out of range");
        return -1;
    }
    uint32_t keys_version = _PyDictKeys_GetVersionForCurrentState(dict);
    if (keys_version == 0) {
        SPECIALIZATION_FAIL(LOAD_ATTR, Py_TYPE(owner), name, "no more key versions");
        return -1;
    }
    cache1->dk_version_or_hint = keys_version;
    cache0->index = (uint16_t)index;
    *instr = _Py_MAKECODEUNIT(LOAD_ATTR_MODULE, _Py_OPARG(*instr));
    return 0;
}



/* Attribute specialization */

typedef enum {
    OVERRIDING, /* Is an overriding descriptor, and will remain so. */
    METHOD, /* Attribute has Py_TPFLAGS_METHOD_DESCRIPTOR set */
    PROPERTY, /* Is a property */
    OBJECT_SLOT, /* Is an object slot descriptor */
    OTHER_SLOT, /* Is a slot descriptor of another type */
    NON_OVERRIDING, /* Is another non-overriding descriptor, and is an instance of an immutable class*/
    NON_DESCRIPTOR, /* Is not a descriptor, and is an instance of an immutable class */
    MUTABLE,   /* Instance of a mutable class; might, or might not, be a descriptor */
    ABSENT, /* Attribute is not present on the class */
    DUNDER_CLASS, /* __class__ attribute */
    GETATTRIBUTE_OVERRIDDEN /* __getattribute__ has been overridden */
} DesciptorClassification;

static DesciptorClassification
analyze_descriptor(PyTypeObject *type, PyObject *name, PyObject **descr)
{
    if (type->tp_getattro != PyObject_GenericGetAttr) {
        *descr = NULL;
        return GETATTRIBUTE_OVERRIDDEN;
    }
    PyObject *descriptor = _PyType_Lookup(type, name);
    *descr = descriptor;
    if (descriptor == NULL) {
        return ABSENT;
    }
    PyTypeObject *desc_cls = Py_TYPE(descriptor);
    if (!(desc_cls->tp_flags & Py_TPFLAGS_IMMUTABLETYPE)) {
        return MUTABLE;
    }
    if (desc_cls->tp_descr_set) {
        if (desc_cls == &PyMemberDescr_Type) {
            PyMemberDescrObject *member = (PyMemberDescrObject *)descriptor;
            struct PyMemberDef *dmem = member->d_member;
            if (dmem->type == T_OBJECT_EX) {
                return OBJECT_SLOT;
            }
            return OTHER_SLOT;
        }
        if (desc_cls == &PyProperty_Type) {
            return PROPERTY;
        }
        if (PyUnicode_CompareWithASCIIString(name, "__class__") == 0) {
            if (descriptor == _PyType_Lookup(&PyBaseObject_Type, name)) {
                return DUNDER_CLASS;
            }
        }
        return OVERRIDING;
    }
    if (desc_cls->tp_descr_get) {
        if (desc_cls->tp_flags & Py_TPFLAGS_METHOD_DESCRIPTOR) {
            return METHOD;
        }
        return NON_OVERRIDING;
    }
    return NON_DESCRIPTOR;
}

int
_Py_Specialize_LoadAttr(PyObject *owner, _Py_CODEUNIT *instr, PyObject *name, SpecializedCacheEntry *cache)
{
    _PyAdaptiveEntry *cache0 = &cache->adaptive;
    _PyLoadAttrCache *cache1 = &cache[-1].load_attr;
    if (PyModule_CheckExact(owner)) {
        int err = specialize_module_load_attr(owner, instr, name, cache0, cache1);
        if (err) {
            goto fail;
        }
        goto success;
    }
    PyTypeObject *type = Py_TYPE(owner);
    if (type->tp_dict == NULL) {
        if (PyType_Ready(type) < 0) {
            return -1;
        }
    }
    PyObject *descr;
    DesciptorClassification kind = analyze_descriptor(type, name, &descr);
    switch(kind) {
        case OVERRIDING:
            SPECIALIZATION_FAIL(LOAD_ATTR, type, name, "overriding descriptor");
            goto fail;
        case METHOD:
            SPECIALIZATION_FAIL(LOAD_ATTR, type, name, "method");
            goto fail;
        case PROPERTY:
            SPECIALIZATION_FAIL(LOAD_ATTR, type, name, "property");
            goto fail;
        case OBJECT_SLOT:
        {
            PyMemberDescrObject *member = (PyMemberDescrObject *)descr;
            struct PyMemberDef *dmem = member->d_member;
            Py_ssize_t offset = dmem->offset;
            if (offset != (uint16_t)offset) {
                SPECIALIZATION_FAIL(LOAD_ATTR, type, name, "offset out of range");
                goto fail;
            }
            assert(dmem->type == T_OBJECT_EX);
            assert(offset > 0);
            cache0->index = (uint16_t)offset;
            cache1->tp_version = type->tp_version_tag;
            *instr = _Py_MAKECODEUNIT(LOAD_ATTR_SLOT, _Py_OPARG(*instr));
            goto success;
        }
        case DUNDER_CLASS:
        {
            Py_ssize_t offset = offsetof(PyObject, ob_type);
            assert(offset == (uint16_t)offset);
            cache0->index = (uint16_t)offset;
            cache1->tp_version = type->tp_version_tag;
            *instr = _Py_MAKECODEUNIT(LOAD_ATTR_SLOT, _Py_OPARG(*instr));
            goto success;
        }
        case OTHER_SLOT:
            SPECIALIZATION_FAIL(LOAD_ATTR, type, name, "non-object slot");
            goto fail;
        case MUTABLE:
            SPECIALIZATION_FAIL(LOAD_ATTR, type, name, "mutable class attribute");
            goto fail;
        case GETATTRIBUTE_OVERRIDDEN:
            SPECIALIZATION_FAIL(LOAD_ATTR, type, name, "__getattribute__ overridden");
            goto fail;
        case NON_OVERRIDING:
        case NON_DESCRIPTOR:
        case ABSENT:
            break;
    }
    assert(kind == NON_OVERRIDING || kind == NON_DESCRIPTOR || kind == ABSENT);
    // No desciptor, or non overriding.
    if (type->tp_dictoffset < 0) {
        SPECIALIZATION_FAIL(LOAD_ATTR, type, name, "negative offset");
        goto fail;
    }
    if (type->tp_dictoffset > 0) {
        PyObject **dictptr = (PyObject **) ((char *)owner + type->tp_dictoffset);
        if (*dictptr == NULL || !PyDict_CheckExact(*dictptr)) {
            SPECIALIZATION_FAIL(LOAD_ATTR, type, name, "no dict or not a dict");
            goto fail;
        }
        // We found an instance with a __dict__.
        PyDictObject *dict = (PyDictObject *)*dictptr;
        if ((type->tp_flags & Py_TPFLAGS_HEAPTYPE)
            && dict->ma_keys == ((PyHeapTypeObject*)type)->ht_cached_keys
        ) {
            // Keys are shared
            assert(PyUnicode_CheckExact(name));
            Py_hash_t hash = PyObject_Hash(name);
            if (hash == -1) {
                return -1;
            }
            PyObject *value;
            Py_ssize_t index = _Py_dict_lookup(dict, name, hash, &value);
            assert (index != DKIX_ERROR);
            if (index != (uint16_t)index) {
                SPECIALIZATION_FAIL(LOAD_ATTR, type, name,
                                    index < 0 ? "attribute not in dict" : "index out of range");
                goto fail;
            }
            uint32_t keys_version = _PyDictKeys_GetVersionForCurrentState(dict);
            if (keys_version == 0) {
                SPECIALIZATION_FAIL(LOAD_ATTR, type, name, "no more key versions");
                goto fail;
            }
            cache1->dk_version_or_hint = keys_version;
            cache1->tp_version = type->tp_version_tag;
            cache0->index = (uint16_t)index;
            *instr = _Py_MAKECODEUNIT(LOAD_ATTR_SPLIT_KEYS, _Py_OPARG(*instr));
            goto success;
        }
        else {
            PyObject *value = NULL;
            Py_ssize_t hint =
                _PyDict_GetItemHint(dict, name, -1, &value);
            if (hint != (uint32_t)hint) {
                SPECIALIZATION_FAIL(LOAD_ATTR, type, name, "hint out of range");
                goto fail;
            }
            cache1->dk_version_or_hint = (uint32_t)hint;
            cache1->tp_version = type->tp_version_tag;
            *instr = _Py_MAKECODEUNIT(LOAD_ATTR_WITH_HINT, _Py_OPARG(*instr));
            goto success;
        }
    }
    assert(type->tp_dictoffset == 0);
    /* No attribute in instance dictionary */
    switch(kind) {
        case NON_OVERRIDING:
            SPECIALIZATION_FAIL(LOAD_ATTR, type, name, "non-overriding descriptor");
            goto fail;
        case NON_DESCRIPTOR:
            /* To do -- Optimize this case */
            SPECIALIZATION_FAIL(LOAD_ATTR, type, name, "non descriptor");
            goto fail;
        case ABSENT:
            SPECIALIZATION_FAIL(LOAD_ATTR, type, name, "no attribute");
            goto fail;
        default:
            Py_UNREACHABLE();
    }
fail:
    STAT_INC(LOAD_ATTR, specialization_failure);
    assert(!PyErr_Occurred());
    cache_backoff(cache0);
    return 0;
success:
    STAT_INC(LOAD_ATTR, specialization_success);
    assert(!PyErr_Occurred());
    cache0->counter = saturating_start();
    return 0;
}

int
_Py_Specialize_LoadGlobal(
    PyObject *globals, PyObject *builtins,
    _Py_CODEUNIT *instr, PyObject *name,
    SpecializedCacheEntry *cache)
{
    _PyAdaptiveEntry *cache0 = &cache->adaptive;
    _PyLoadGlobalCache *cache1 = &cache[-1].load_global;
    assert(PyUnicode_CheckExact(name));
    if (!PyDict_CheckExact(globals)) {
        goto fail;
    }
    if (((PyDictObject *)globals)->ma_keys->dk_kind != DICT_KEYS_UNICODE) {
        goto fail;
    }
    PyObject *value = NULL;
    Py_ssize_t index = _PyDict_GetItemHint((PyDictObject *)globals, name, -1, &value);
    assert (index != DKIX_ERROR);
    if (index != DKIX_EMPTY) {
        if (index != (uint16_t)index) {
            goto fail;
        }
        uint32_t keys_version = _PyDictKeys_GetVersionForCurrentState((PyDictObject *)globals);
        if (keys_version == 0) {
            goto fail;
        }
        cache1->module_keys_version = keys_version;
        cache0->index = (uint16_t)index;
        *instr = _Py_MAKECODEUNIT(LOAD_GLOBAL_MODULE, _Py_OPARG(*instr));
        goto success;
    }
    if (!PyDict_CheckExact(builtins)) {
        goto fail;
    }
    if (((PyDictObject *)builtins)->ma_keys->dk_kind != DICT_KEYS_UNICODE) {
        goto fail;
    }
    index = _PyDict_GetItemHint((PyDictObject *)builtins, name, -1, &value);
    assert (index != DKIX_ERROR);
    if (index != (uint16_t)index) {
        goto fail;
    }
    uint32_t globals_version = _PyDictKeys_GetVersionForCurrentState((PyDictObject *)globals);
    if (globals_version == 0) {
        goto fail;
    }
    uint32_t builtins_version = _PyDictKeys_GetVersionForCurrentState((PyDictObject *)builtins);
    if (builtins_version == 0) {
        goto fail;
    }
    cache1->module_keys_version = globals_version;
    cache1->builtin_keys_version = builtins_version;
    cache0->index = (uint16_t)index;
    *instr = _Py_MAKECODEUNIT(LOAD_GLOBAL_BUILTIN, _Py_OPARG(*instr));
    goto success;
fail:
    STAT_INC(LOAD_GLOBAL, specialization_failure);
    assert(!PyErr_Occurred());
    cache_backoff(cache0);
    return 0;
success:
    STAT_INC(LOAD_GLOBAL, specialization_success);
    assert(!PyErr_Occurred());
    cache0->counter = saturating_start();
    return 0;
}


int
_Py_Specialize_BinarySubscr(
     PyObject *container, PyObject *sub, _Py_CODEUNIT *instr)
{
    PyTypeObject *container_type = Py_TYPE(container);
    if (container_type == &PyList_Type) {
        if (PyLong_CheckExact(sub)) {
            *instr = _Py_MAKECODEUNIT(BINARY_SUBSCR_LIST_INT, saturating_start());
            goto success;
        } else {
            SPECIALIZATION_FAIL(BINARY_SUBSCR, Py_TYPE(container), Py_TYPE(sub), "list; non-integer subscr");
            goto fail;
        }
    }
    if (container_type == &PyTuple_Type) {
        if (PyLong_CheckExact(sub)) {
            *instr = _Py_MAKECODEUNIT(BINARY_SUBSCR_TUPLE_INT, saturating_start());
            goto success;
        } else {
            SPECIALIZATION_FAIL(BINARY_SUBSCR, Py_TYPE(container), Py_TYPE(sub), "tuple; non-integer subscr");
            goto fail;
        }
    }
    if (container_type == &PyDict_Type) {
        *instr = _Py_MAKECODEUNIT(BINARY_SUBSCR_DICT, saturating_start());
        goto success;
    }
    SPECIALIZATION_FAIL(BINARY_SUBSCR, Py_TYPE(container), Py_TYPE(sub), "not list|tuple|dict");
    goto fail;
fail:
    STAT_INC(BINARY_SUBSCR, specialization_failure);
    assert(!PyErr_Occurred());
    *instr = _Py_MAKECODEUNIT(_Py_OPCODE(*instr), ADAPTIVE_CACHE_BACKOFF);
    return 0;
success:
    STAT_INC(BINARY_SUBSCR, specialization_success);
    assert(!PyErr_Occurred());
    return 0;
}

