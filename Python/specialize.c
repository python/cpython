#include "Python.h"
#include "pycore_code.h"
#include "pycore_dict.h"
#include "pycore_function.h"      // _PyFunction_GetVersionForCurrentState()
#include "pycore_global_strings.h"  // _Py_ID()
#include "pycore_long.h"
#include "pycore_moduleobject.h"
#include "pycore_object.h"
#include "pycore_opcode.h"        // _PyOpcode_Caches
#include "structmember.h"         // struct PyMemberDef, T_OFFSET_EX

#include <stdlib.h> // rand()

/* For guidance on adding or extending families of instructions see
 * ./adaptive.md
 */

/* Map from opcode to adaptive opcode.
  Values of zero are ignored. */
uint8_t _PyOpcode_Adaptive[256] = {
    [LOAD_ATTR] = LOAD_ATTR_ADAPTIVE,
    [LOAD_GLOBAL] = LOAD_GLOBAL_ADAPTIVE,
    [LOAD_METHOD] = LOAD_METHOD_ADAPTIVE,
    [BINARY_SUBSCR] = BINARY_SUBSCR_ADAPTIVE,
    [STORE_SUBSCR] = STORE_SUBSCR_ADAPTIVE,
    [CALL] = CALL_ADAPTIVE,
    [PRECALL] = PRECALL_ADAPTIVE,
    [STORE_ATTR] = STORE_ATTR_ADAPTIVE,
    [BINARY_OP] = BINARY_OP_ADAPTIVE,
    [COMPARE_OP] = COMPARE_OP_ADAPTIVE,
    [UNPACK_SEQUENCE] = UNPACK_SEQUENCE_ADAPTIVE,
};

Py_ssize_t _Py_QuickenedCount = 0;
#ifdef Py_STATS
PyStats _py_stats = { 0 };

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
    ADD_STAT_TO_DICT(res, success);
    ADD_STAT_TO_DICT(res, failure);
    ADD_STAT_TO_DICT(res, hit);
    ADD_STAT_TO_DICT(res, deferred);
    ADD_STAT_TO_DICT(res, miss);
    ADD_STAT_TO_DICT(res, deopt);
    PyObject *failure_kinds = PyTuple_New(SPECIALIZATION_FAILURE_KINDS);
    if (failure_kinds == NULL) {
        Py_DECREF(res);
        return NULL;
    }
    for (int i = 0; i < SPECIALIZATION_FAILURE_KINDS; i++) {
        PyObject *stat = PyLong_FromUnsignedLongLong(stats->failure_kinds[i]);
        if (stat == NULL) {
            Py_DECREF(res);
            Py_DECREF(failure_kinds);
            return NULL;
        }
        PyTuple_SET_ITEM(failure_kinds, i, stat);
    }
    if (PyDict_SetItemString(res, "failure_kinds", failure_kinds)) {
        Py_DECREF(res);
        Py_DECREF(failure_kinds);
        return NULL;
    }
    Py_DECREF(failure_kinds);
    return res;
}
#undef ADD_STAT_TO_DICT

static int
add_stat_dict(
    PyObject *res,
    int opcode,
    const char *name) {

    SpecializationStats *stats = &_py_stats.opcode_stats[opcode].specialization;
    PyObject *d = stats_to_dict(stats);
    if (d == NULL) {
        return -1;
    }
    int err = PyDict_SetItemString(res, name, d);
    Py_DECREF(d);
    return err;
}

#ifdef Py_STATS
PyObject*
_Py_GetSpecializationStats(void) {
    PyObject *stats = PyDict_New();
    if (stats == NULL) {
        return NULL;
    }
    int err = 0;
    err += add_stat_dict(stats, LOAD_ATTR, "load_attr");
    err += add_stat_dict(stats, LOAD_GLOBAL, "load_global");
    err += add_stat_dict(stats, LOAD_METHOD, "load_method");
    err += add_stat_dict(stats, BINARY_SUBSCR, "binary_subscr");
    err += add_stat_dict(stats, STORE_SUBSCR, "store_subscr");
    err += add_stat_dict(stats, STORE_ATTR, "store_attr");
    err += add_stat_dict(stats, CALL, "call");
    err += add_stat_dict(stats, BINARY_OP, "binary_op");
    err += add_stat_dict(stats, COMPARE_OP, "compare_op");
    err += add_stat_dict(stats, UNPACK_SEQUENCE, "unpack_sequence");
    err += add_stat_dict(stats, PRECALL, "precall");
    if (err < 0) {
        Py_DECREF(stats);
        return NULL;
    }
    return stats;
}
#endif


#define PRINT_STAT(i, field) \
    if (stats[i].field) { \
        fprintf(out, "    opcode[%d]." #field " : %" PRIu64 "\n", i, stats[i].field); \
    }

static void
print_spec_stats(FILE *out, OpcodeStats *stats)
{
    /* Mark some opcodes as specializable for stats,
     * even though we don't specialize them yet. */
    fprintf(out, "opcode[%d].specializable : 1\n", FOR_ITER);
    for (int i = 0; i < 256; i++) {
        if (_PyOpcode_Adaptive[i]) {
            fprintf(out, "opcode[%d].specializable : 1\n", i);
        }
        PRINT_STAT(i, specialization.success);
        PRINT_STAT(i, specialization.failure);
        PRINT_STAT(i, specialization.hit);
        PRINT_STAT(i, specialization.deferred);
        PRINT_STAT(i, specialization.miss);
        PRINT_STAT(i, specialization.deopt);
        PRINT_STAT(i, execution_count);
        for (int j = 0; j < SPECIALIZATION_FAILURE_KINDS; j++) {
            uint64_t val = stats[i].specialization.failure_kinds[j];
            if (val) {
                fprintf(out, "    opcode[%d].specialization.failure_kinds[%d] : %"
                    PRIu64 "\n", i, j, val);
            }
        }
        for(int j = 0; j < 256; j++) {
            if (stats[i].pair_count[j]) {
                fprintf(out, "opcode[%d].pair_count[%d] : %" PRIu64 "\n",
                        i, j, stats[i].pair_count[j]);
            }
        }
    }
}
#undef PRINT_STAT


static void
print_call_stats(FILE *out, CallStats *stats)
{
    fprintf(out, "Calls to PyEval_EvalDefault: %" PRIu64 "\n", stats->pyeval_calls);
    fprintf(out, "Calls to Python functions inlined: %" PRIu64 "\n", stats->inlined_py_calls);
    fprintf(out, "Frames pushed: %" PRIu64 "\n", stats->frames_pushed);
    fprintf(out, "Frame objects created: %" PRIu64 "\n", stats->frame_objects_created);
}

static void
print_object_stats(FILE *out, ObjectStats *stats)
{
    fprintf(out, "Object allocations from freelist: %" PRIu64 "\n", stats->from_freelist);
    fprintf(out, "Object frees to freelist: %" PRIu64 "\n", stats->to_freelist);
    fprintf(out, "Object allocations: %" PRIu64 "\n", stats->allocations);
    fprintf(out, "Object allocations to 512 bytes: %" PRIu64 "\n", stats->allocations512);
    fprintf(out, "Object allocations to 4 kbytes: %" PRIu64 "\n", stats->allocations4k);
    fprintf(out, "Object allocations over 4 kbytes: %" PRIu64 "\n", stats->allocations_big);
    fprintf(out, "Object frees: %" PRIu64 "\n", stats->frees);
    fprintf(out, "Object new values: %" PRIu64 "\n", stats->new_values);
    fprintf(out, "Object materialize dict (on request): %" PRIu64 "\n", stats->dict_materialized_on_request);
    fprintf(out, "Object materialize dict (new key): %" PRIu64 "\n", stats->dict_materialized_new_key);
    fprintf(out, "Object materialize dict (too big): %" PRIu64 "\n", stats->dict_materialized_too_big);
    fprintf(out, "Object materialize dict (str subclass): %" PRIu64 "\n", stats->dict_materialized_str_subclass);
}

static void
print_stats(FILE *out, PyStats *stats) {
    print_spec_stats(out, stats->opcode_stats);
    print_call_stats(out, &stats->call_stats);
    print_object_stats(out, &stats->object_stats);
}

void
_Py_PrintSpecializationStats(int to_file)
{
    FILE *out = stderr;
    if (to_file) {
        /* Write to a file instead of stderr. */
# ifdef MS_WINDOWS
        const char *dirname = "c:\\temp\\py_stats\\";
# else
        const char *dirname = "/tmp/py_stats/";
# endif
        /* Use random 160 bit number as file name,
        * to avoid both accidental collisions and
        * symlink attacks. */
        unsigned char rand[20];
        char hex_name[41];
        _PyOS_URandomNonblock(rand, 20);
        for (int i = 0; i < 20; i++) {
            hex_name[2*i] = "0123456789abcdef"[rand[i]&15];
            hex_name[2*i+1] = "0123456789abcdef"[(rand[i]>>4)&15];
        }
        hex_name[40] = '\0';
        char buf[64];
        assert(strlen(dirname) + 40 + strlen(".txt") < 64);
        sprintf(buf, "%s%s.txt", dirname, hex_name);
        FILE *fout = fopen(buf, "w");
        if (fout) {
            out = fout;
        }
    }
    else {
        fprintf(out, "Specialization stats:\n");
    }
    print_stats(out, &_py_stats);
    if (out != stderr) {
        fclose(out);
    }
}

#ifdef Py_STATS

#define SPECIALIZATION_FAIL(opcode, kind) _py_stats.opcode_stats[opcode].specialization.failure_kinds[kind]++


#endif
#endif

#ifndef SPECIALIZATION_FAIL
#define SPECIALIZATION_FAIL(opcode, kind) ((void)0)
#endif

// Insert adaptive instructions and superinstructions. This cannot fail.
void
_PyCode_Quicken(PyCodeObject *code)
{
    _Py_QuickenedCount++;
    int previous_opcode = -1;
    _Py_CODEUNIT *instructions = _PyCode_CODE(code);
    for (int i = 0; i < Py_SIZE(code); i++) {
        int opcode = _Py_OPCODE(instructions[i]);
        uint8_t adaptive_opcode = _PyOpcode_Adaptive[opcode];
        if (adaptive_opcode) {
            _Py_SET_OPCODE(instructions[i], adaptive_opcode);
            // Make sure the adaptive counter is zero:
            assert(instructions[i + 1] == 0);
            previous_opcode = -1;
            i += _PyOpcode_Caches[opcode];
        }
        else {
            assert(!_PyOpcode_Caches[opcode]);
            switch (opcode) {
                case EXTENDED_ARG:
                    _Py_SET_OPCODE(instructions[i], EXTENDED_ARG_QUICK);
                    break;
                case JUMP_BACKWARD:
                    _Py_SET_OPCODE(instructions[i], JUMP_BACKWARD_QUICK);
                    break;
                case RESUME:
                    _Py_SET_OPCODE(instructions[i], RESUME_QUICK);
                    break;
                case LOAD_FAST:
                    switch(previous_opcode) {
                        case LOAD_FAST:
                            _Py_SET_OPCODE(instructions[i - 1],
                                           LOAD_FAST__LOAD_FAST);
                            break;
                        case STORE_FAST:
                            _Py_SET_OPCODE(instructions[i - 1],
                                           STORE_FAST__LOAD_FAST);
                            break;
                        case LOAD_CONST:
                            _Py_SET_OPCODE(instructions[i - 1],
                                           LOAD_CONST__LOAD_FAST);
                            break;
                    }
                    break;
                case STORE_FAST:
                    if (previous_opcode == STORE_FAST) {
                        _Py_SET_OPCODE(instructions[i - 1],
                                       STORE_FAST__STORE_FAST);
                    }
                    break;
                case LOAD_CONST:
                    if (previous_opcode == LOAD_FAST) {
                        _Py_SET_OPCODE(instructions[i - 1],
                                       LOAD_FAST__LOAD_CONST);
                    }
                    break;
            }
            previous_opcode = opcode;
        }
    }
}

static inline int
miss_counter_start(void) {
    /* Starting value for the counter.
     * This value needs to be not too low, otherwise
     * it would cause excessive de-optimization.
     * Neither should it be too high, or that would delay
     * de-optimization excessively when it is needed.
     * A value around 50 seems to work, and we choose a
     * prime number to avoid artifacts.
     */
    return 53;
}

/* Common */

#define SPEC_FAIL_OTHER 0
#define SPEC_FAIL_NO_DICT 1
#define SPEC_FAIL_OVERRIDDEN 2
#define SPEC_FAIL_OUT_OF_VERSIONS 3
#define SPEC_FAIL_OUT_OF_RANGE 4
#define SPEC_FAIL_EXPECTED_ERROR 5
#define SPEC_FAIL_WRONG_NUMBER_ARGUMENTS 6

#define SPEC_FAIL_LOAD_GLOBAL_NON_STRING_OR_SPLIT 18

/* Attributes */

#define SPEC_FAIL_ATTR_OVERRIDING_DESCRIPTOR 8
#define SPEC_FAIL_ATTR_NON_OVERRIDING_DESCRIPTOR 9
#define SPEC_FAIL_ATTR_NOT_DESCRIPTOR 10
#define SPEC_FAIL_ATTR_METHOD 11
#define SPEC_FAIL_ATTR_MUTABLE_CLASS 12
#define SPEC_FAIL_ATTR_PROPERTY 13
#define SPEC_FAIL_ATTR_NON_OBJECT_SLOT 14
#define SPEC_FAIL_ATTR_READ_ONLY 15
#define SPEC_FAIL_ATTR_AUDITED_SLOT 16
#define SPEC_FAIL_ATTR_NOT_MANAGED_DICT 17
#define SPEC_FAIL_ATTR_NON_STRING_OR_SPLIT 18
#define SPEC_FAIL_ATTR_MODULE_ATTR_NOT_FOUND 19

/* Methods */

#define SPEC_FAIL_LOAD_METHOD_OVERRIDING_DESCRIPTOR 8
#define SPEC_FAIL_LOAD_METHOD_NON_OVERRIDING_DESCRIPTOR 9
#define SPEC_FAIL_LOAD_METHOD_NOT_DESCRIPTOR 10
#define SPEC_FAIL_LOAD_METHOD_METHOD 11
#define SPEC_FAIL_LOAD_METHOD_MUTABLE_CLASS 12
#define SPEC_FAIL_LOAD_METHOD_PROPERTY 13
#define SPEC_FAIL_LOAD_METHOD_NON_OBJECT_SLOT 14
#define SPEC_FAIL_LOAD_METHOD_IS_ATTR 15
#define SPEC_FAIL_LOAD_METHOD_DICT_SUBCLASS 16
#define SPEC_FAIL_LOAD_METHOD_BUILTIN_CLASS_METHOD 17
#define SPEC_FAIL_LOAD_METHOD_CLASS_METHOD_OBJ 18
#define SPEC_FAIL_LOAD_METHOD_OBJECT_SLOT 19
#define SPEC_FAIL_LOAD_METHOD_HAS_DICT 20
#define SPEC_FAIL_LOAD_METHOD_HAS_MANAGED_DICT 21
#define SPEC_FAIL_LOAD_METHOD_INSTANCE_ATTRIBUTE 22
#define SPEC_FAIL_LOAD_METHOD_METACLASS_ATTRIBUTE 23

/* Binary subscr and store subscr */

#define SPEC_FAIL_SUBSCR_ARRAY_INT 8
#define SPEC_FAIL_SUBSCR_ARRAY_SLICE 9
#define SPEC_FAIL_SUBSCR_LIST_SLICE 10
#define SPEC_FAIL_SUBSCR_TUPLE_SLICE 11
#define SPEC_FAIL_SUBSCR_STRING_INT 12
#define SPEC_FAIL_SUBSCR_STRING_SLICE 13
#define SPEC_FAIL_SUBSCR_BUFFER_INT 15
#define SPEC_FAIL_SUBSCR_BUFFER_SLICE 16
#define SPEC_FAIL_SUBSCR_SEQUENCE_INT 17

/* Store subscr */
#define SPEC_FAIL_SUBSCR_BYTEARRAY_INT 18
#define SPEC_FAIL_SUBSCR_BYTEARRAY_SLICE 19
#define SPEC_FAIL_SUBSCR_PY_SIMPLE 20
#define SPEC_FAIL_SUBSCR_PY_OTHER 21
#define SPEC_FAIL_SUBSCR_DICT_SUBCLASS_NO_OVERRIDE 22
#define SPEC_FAIL_SUBSCR_NOT_HEAP_TYPE 23

/* Binary op */

#define SPEC_FAIL_BINARY_OP_ADD_DIFFERENT_TYPES          8
#define SPEC_FAIL_BINARY_OP_ADD_OTHER                    9
#define SPEC_FAIL_BINARY_OP_AND_DIFFERENT_TYPES         10
#define SPEC_FAIL_BINARY_OP_AND_INT                     11
#define SPEC_FAIL_BINARY_OP_AND_OTHER                   12
#define SPEC_FAIL_BINARY_OP_FLOOR_DIVIDE                13
#define SPEC_FAIL_BINARY_OP_LSHIFT                      14
#define SPEC_FAIL_BINARY_OP_MATRIX_MULTIPLY             15
#define SPEC_FAIL_BINARY_OP_MULTIPLY_DIFFERENT_TYPES    16
#define SPEC_FAIL_BINARY_OP_MULTIPLY_OTHER              17
#define SPEC_FAIL_BINARY_OP_OR                          18
#define SPEC_FAIL_BINARY_OP_POWER                       19
#define SPEC_FAIL_BINARY_OP_REMAINDER                   20
#define SPEC_FAIL_BINARY_OP_RSHIFT                      21
#define SPEC_FAIL_BINARY_OP_SUBTRACT_DIFFERENT_TYPES    22
#define SPEC_FAIL_BINARY_OP_SUBTRACT_OTHER              23
#define SPEC_FAIL_BINARY_OP_TRUE_DIVIDE_DIFFERENT_TYPES 24
#define SPEC_FAIL_BINARY_OP_TRUE_DIVIDE_FLOAT           25
#define SPEC_FAIL_BINARY_OP_TRUE_DIVIDE_OTHER           26
#define SPEC_FAIL_BINARY_OP_XOR                         27

/* Calls */
#define SPEC_FAIL_CALL_COMPLEX_PARAMETERS 9
#define SPEC_FAIL_CALL_CO_NOT_OPTIMIZED 10
/* SPEC_FAIL_METHOD  defined as 11 above */

#define SPEC_FAIL_CALL_INSTANCE_METHOD 11
#define SPEC_FAIL_CALL_CMETHOD 12
#define SPEC_FAIL_CALL_PYCFUNCTION 13
#define SPEC_FAIL_CALL_PYCFUNCTION_WITH_KEYWORDS 14
#define SPEC_FAIL_CALL_PYCFUNCTION_FAST_WITH_KEYWORDS 15
#define SPEC_FAIL_CALL_PYCFUNCTION_NOARGS 16
#define SPEC_FAIL_CALL_BAD_CALL_FLAGS 17
#define SPEC_FAIL_CALL_CLASS 18
#define SPEC_FAIL_CALL_PYTHON_CLASS 19
#define SPEC_FAIL_CALL_METHOD_DESCRIPTOR 20
#define SPEC_FAIL_CALL_BOUND_METHOD 21
#define SPEC_FAIL_CALL_STR 22
#define SPEC_FAIL_CALL_CLASS_NO_VECTORCALL 23
#define SPEC_FAIL_CALL_CLASS_MUTABLE 24
#define SPEC_FAIL_CALL_KWNAMES 25
#define SPEC_FAIL_CALL_METHOD_WRAPPER 26
#define SPEC_FAIL_CALL_OPERATOR_WRAPPER 27
#define SPEC_FAIL_CALL_PYFUNCTION 28
#define SPEC_FAIL_CALL_PEP_523 29

/* COMPARE_OP */
#define SPEC_FAIL_COMPARE_OP_DIFFERENT_TYPES 12
#define SPEC_FAIL_COMPARE_OP_STRING 13
#define SPEC_FAIL_COMPARE_OP_NOT_FOLLOWED_BY_COND_JUMP 14
#define SPEC_FAIL_COMPARE_OP_BIG_INT 15
#define SPEC_FAIL_COMPARE_OP_BYTES 16
#define SPEC_FAIL_COMPARE_OP_TUPLE 17
#define SPEC_FAIL_COMPARE_OP_LIST 18
#define SPEC_FAIL_COMPARE_OP_SET 19
#define SPEC_FAIL_COMPARE_OP_BOOL 20
#define SPEC_FAIL_COMPARE_OP_BASEOBJECT 21
#define SPEC_FAIL_COMPARE_OP_FLOAT_LONG 22
#define SPEC_FAIL_COMPARE_OP_LONG_FLOAT 23
#define SPEC_FAIL_COMPARE_OP_EXTENDED_ARG 24

/* FOR_ITER */
#define SPEC_FAIL_FOR_ITER_GENERATOR 10
#define SPEC_FAIL_FOR_ITER_COROUTINE 11
#define SPEC_FAIL_FOR_ITER_ASYNC_GENERATOR 12
#define SPEC_FAIL_FOR_ITER_LIST 13
#define SPEC_FAIL_FOR_ITER_TUPLE 14
#define SPEC_FAIL_FOR_ITER_SET 15
#define SPEC_FAIL_FOR_ITER_STRING 16
#define SPEC_FAIL_FOR_ITER_BYTES 17
#define SPEC_FAIL_FOR_ITER_RANGE 18
#define SPEC_FAIL_FOR_ITER_ITERTOOLS 19
#define SPEC_FAIL_FOR_ITER_DICT_KEYS 20
#define SPEC_FAIL_FOR_ITER_DICT_ITEMS 21
#define SPEC_FAIL_FOR_ITER_DICT_VALUES 22
#define SPEC_FAIL_FOR_ITER_ENUMERATE 23

// UNPACK_SEQUENCE

#define SPEC_FAIL_UNPACK_SEQUENCE_ITERATOR 8
#define SPEC_FAIL_UNPACK_SEQUENCE_SEQUENCE 9


static int
specialize_module_load_attr(PyObject *owner, _Py_CODEUNIT *instr,
                            PyObject *name, int opcode, int opcode_module)
{
    _PyAttrCache *cache = (_PyAttrCache *)(instr + 1);
    PyModuleObject *m = (PyModuleObject *)owner;
    PyObject *value = NULL;
    assert((owner->ob_type->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0);
    PyDictObject *dict = (PyDictObject *)m->md_dict;
    if (dict == NULL) {
        SPECIALIZATION_FAIL(opcode, SPEC_FAIL_NO_DICT);
        return -1;
    }
    if (dict->ma_keys->dk_kind != DICT_KEYS_UNICODE) {
        SPECIALIZATION_FAIL(opcode, SPEC_FAIL_ATTR_NON_STRING_OR_SPLIT);
        return -1;
    }
    Py_ssize_t index = _PyDict_GetItemHint(dict, &_Py_ID(__getattr__), -1,
                                           &value);
    assert(index != DKIX_ERROR);
    if (index != DKIX_EMPTY) {
        SPECIALIZATION_FAIL(opcode, SPEC_FAIL_ATTR_MODULE_ATTR_NOT_FOUND);
        return -1;
    }
    index = _PyDict_GetItemHint(dict, name, -1, &value);
    assert (index != DKIX_ERROR);
    if (index != (uint16_t)index) {
        SPECIALIZATION_FAIL(opcode, SPEC_FAIL_OUT_OF_RANGE);
        return -1;
    }
    uint32_t keys_version = _PyDictKeys_GetVersionForCurrentState(dict->ma_keys);
    if (keys_version == 0) {
        SPECIALIZATION_FAIL(opcode, SPEC_FAIL_OUT_OF_VERSIONS);
        return -1;
    }
    write_u32(cache->version, keys_version);
    cache->index = (uint16_t)index;
    _Py_SET_OPCODE(*instr, opcode_module);
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
    BUILTIN_CLASSMETHOD, /* Builtin methods with METH_CLASS */
    PYTHON_CLASSMETHOD, /* Python classmethod(func) object */
    NON_DESCRIPTOR, /* Is not a descriptor, and is an instance of an immutable class */
    MUTABLE,   /* Instance of a mutable class; might, or might not, be a descriptor */
    ABSENT, /* Attribute is not present on the class */
    DUNDER_CLASS, /* __class__ attribute */
    GETSET_OVERRIDDEN /* __getattribute__ or __setattr__ has been overridden */
} DescriptorClassification;


static DescriptorClassification
analyze_descriptor(PyTypeObject *type, PyObject *name, PyObject **descr, int store)
{
    if (store) {
        if (type->tp_setattro != PyObject_GenericSetAttr) {
            *descr = NULL;
            return GETSET_OVERRIDDEN;
        }
    }
    else {
        if (type->tp_getattro != PyObject_GenericGetAttr) {
            *descr = NULL;
            return GETSET_OVERRIDDEN;
        }
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
        if (Py_IS_TYPE(descriptor, &PyClassMethodDescr_Type)) {
            return BUILTIN_CLASSMETHOD;
        }
        if (Py_IS_TYPE(descriptor, &PyClassMethod_Type)) {
            return PYTHON_CLASSMETHOD;
        }
        return NON_OVERRIDING;
    }
    return NON_DESCRIPTOR;
}

static int
specialize_dict_access(
    PyObject *owner, _Py_CODEUNIT *instr, PyTypeObject *type,
    DescriptorClassification kind, PyObject *name,
    int base_op, int values_op, int hint_op)
{
    assert(kind == NON_OVERRIDING || kind == NON_DESCRIPTOR || kind == ABSENT ||
        kind == BUILTIN_CLASSMETHOD || kind == PYTHON_CLASSMETHOD);
    // No descriptor, or non overriding.
    if ((type->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0) {
        SPECIALIZATION_FAIL(base_op, SPEC_FAIL_ATTR_NOT_MANAGED_DICT);
        return 0;
    }
    _PyAttrCache *cache = (_PyAttrCache *)(instr + 1);
    PyObject **dictptr = _PyObject_ManagedDictPointer(owner);
    PyDictObject *dict = (PyDictObject *)*dictptr;
    if (dict == NULL) {
        // Virtual dictionary
        PyDictKeysObject *keys = ((PyHeapTypeObject *)type)->ht_cached_keys;
        assert(PyUnicode_CheckExact(name));
        Py_ssize_t index = _PyDictKeys_StringLookup(keys, name);
        assert (index != DKIX_ERROR);
        if (index != (uint16_t)index) {
            SPECIALIZATION_FAIL(base_op, SPEC_FAIL_OUT_OF_RANGE);
            return 0;
        }
        write_u32(cache->version, type->tp_version_tag);
        cache->index = (uint16_t)index;
        _Py_SET_OPCODE(*instr, values_op);
    }
    else {
        if (!PyDict_CheckExact(dict)) {
            SPECIALIZATION_FAIL(base_op, SPEC_FAIL_NO_DICT);
            return 0;
        }
        // We found an instance with a __dict__.
        PyObject *value = NULL;
        Py_ssize_t hint =
            _PyDict_GetItemHint(dict, name, -1, &value);
        if (hint != (uint16_t)hint) {
            SPECIALIZATION_FAIL(base_op, SPEC_FAIL_OUT_OF_RANGE);
            return 0;
        }
        cache->index = (uint16_t)hint;
        write_u32(cache->version, type->tp_version_tag);
        _Py_SET_OPCODE(*instr, hint_op);
    }
    return 1;
}

int
_Py_Specialize_LoadAttr(PyObject *owner, _Py_CODEUNIT *instr, PyObject *name)
{
    assert(_PyOpcode_Caches[LOAD_ATTR] == INLINE_CACHE_ENTRIES_LOAD_ATTR);
    _PyAttrCache *cache = (_PyAttrCache *)(instr + 1);
    if (PyModule_CheckExact(owner)) {
        int err = specialize_module_load_attr(owner, instr, name, LOAD_ATTR,
                                              LOAD_ATTR_MODULE);
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
    DescriptorClassification kind = analyze_descriptor(type, name, &descr, 0);
    switch(kind) {
        case OVERRIDING:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_OVERRIDING_DESCRIPTOR);
            goto fail;
        case METHOD:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_METHOD);
            goto fail;
        case PROPERTY:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_PROPERTY);
            goto fail;
        case OBJECT_SLOT:
        {
            PyMemberDescrObject *member = (PyMemberDescrObject *)descr;
            struct PyMemberDef *dmem = member->d_member;
            Py_ssize_t offset = dmem->offset;
            if (!PyObject_TypeCheck(owner, member->d_common.d_type)) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_EXPECTED_ERROR);
                goto fail;
            }
            if (dmem->flags & PY_AUDIT_READ) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_AUDITED_SLOT);
                goto fail;
            }
            if (offset != (uint16_t)offset) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_RANGE);
                goto fail;
            }
            assert(dmem->type == T_OBJECT_EX);
            assert(offset > 0);
            cache->index = (uint16_t)offset;
            write_u32(cache->version, type->tp_version_tag);
            _Py_SET_OPCODE(*instr, LOAD_ATTR_SLOT);
            goto success;
        }
        case DUNDER_CLASS:
        {
            Py_ssize_t offset = offsetof(PyObject, ob_type);
            assert(offset == (uint16_t)offset);
            cache->index = (uint16_t)offset;
            write_u32(cache->version, type->tp_version_tag);
            _Py_SET_OPCODE(*instr, LOAD_ATTR_SLOT);
            goto success;
        }
        case OTHER_SLOT:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_NON_OBJECT_SLOT);
            goto fail;
        case MUTABLE:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_MUTABLE_CLASS);
            goto fail;
        case GETSET_OVERRIDDEN:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OVERRIDDEN);
            goto fail;
        case BUILTIN_CLASSMETHOD:
        case PYTHON_CLASSMETHOD:
        case NON_OVERRIDING:
        case NON_DESCRIPTOR:
        case ABSENT:
            break;
    }
    int err = specialize_dict_access(
        owner, instr, type, kind, name,
        LOAD_ATTR, LOAD_ATTR_INSTANCE_VALUE, LOAD_ATTR_WITH_HINT
    );
    if (err < 0) {
        return -1;
    }
    if (err) {
        goto success;
    }
fail:
    STAT_INC(LOAD_ATTR, failure);
    assert(!PyErr_Occurred());
    cache->counter = adaptive_counter_backoff(cache->counter);
    return 0;
success:
    STAT_INC(LOAD_ATTR, success);
    assert(!PyErr_Occurred());
    cache->counter = miss_counter_start();
    return 0;
}

int
_Py_Specialize_StoreAttr(PyObject *owner, _Py_CODEUNIT *instr, PyObject *name)
{
    assert(_PyOpcode_Caches[STORE_ATTR] == INLINE_CACHE_ENTRIES_STORE_ATTR);
    _PyAttrCache *cache = (_PyAttrCache *)(instr + 1);
    PyTypeObject *type = Py_TYPE(owner);
    if (PyModule_CheckExact(owner)) {
        SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_OVERRIDDEN);
        goto fail;
    }
    PyObject *descr;
    DescriptorClassification kind = analyze_descriptor(type, name, &descr, 1);
    switch(kind) {
        case OVERRIDING:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_OVERRIDING_DESCRIPTOR);
            goto fail;
        case METHOD:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_METHOD);
            goto fail;
        case PROPERTY:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_PROPERTY);
            goto fail;
        case OBJECT_SLOT:
        {
            PyMemberDescrObject *member = (PyMemberDescrObject *)descr;
            struct PyMemberDef *dmem = member->d_member;
            Py_ssize_t offset = dmem->offset;
            if (!PyObject_TypeCheck(owner, member->d_common.d_type)) {
                SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_EXPECTED_ERROR);
                goto fail;
            }
            if (dmem->flags & READONLY) {
                SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_READ_ONLY);
                goto fail;
            }
            if (offset != (uint16_t)offset) {
                SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_OUT_OF_RANGE);
                goto fail;
            }
            assert(dmem->type == T_OBJECT_EX);
            assert(offset > 0);
            cache->index = (uint16_t)offset;
            write_u32(cache->version, type->tp_version_tag);
            _Py_SET_OPCODE(*instr, STORE_ATTR_SLOT);
            goto success;
        }
        case DUNDER_CLASS:
        case OTHER_SLOT:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_NON_OBJECT_SLOT);
            goto fail;
        case MUTABLE:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_MUTABLE_CLASS);
            goto fail;
        case GETSET_OVERRIDDEN:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_OVERRIDDEN);
            goto fail;
        case BUILTIN_CLASSMETHOD:
        case PYTHON_CLASSMETHOD:
        case NON_OVERRIDING:
        case NON_DESCRIPTOR:
        case ABSENT:
            break;
    }

    int err = specialize_dict_access(
        owner, instr, type, kind, name,
        STORE_ATTR, STORE_ATTR_INSTANCE_VALUE, STORE_ATTR_WITH_HINT
    );
    if (err < 0) {
        return -1;
    }
    if (err) {
        goto success;
    }
fail:
    STAT_INC(STORE_ATTR, failure);
    assert(!PyErr_Occurred());
    cache->counter = adaptive_counter_backoff(cache->counter);
    return 0;
success:
    STAT_INC(STORE_ATTR, success);
    assert(!PyErr_Occurred());
    cache->counter = miss_counter_start();
    return 0;
}


#ifdef Py_STATS
static int
load_method_fail_kind(DescriptorClassification kind)
{
    switch (kind) {
        case OVERRIDING:
            return SPEC_FAIL_LOAD_METHOD_OVERRIDING_DESCRIPTOR;
        case METHOD:
            return SPEC_FAIL_LOAD_METHOD_METHOD;
        case PROPERTY:
            return SPEC_FAIL_LOAD_METHOD_PROPERTY;
        case OBJECT_SLOT:
            return SPEC_FAIL_LOAD_METHOD_OBJECT_SLOT;
        case OTHER_SLOT:
            return SPEC_FAIL_LOAD_METHOD_NON_OBJECT_SLOT;
        case DUNDER_CLASS:
            return SPEC_FAIL_OTHER;
        case MUTABLE:
            return SPEC_FAIL_LOAD_METHOD_MUTABLE_CLASS;
        case GETSET_OVERRIDDEN:
            return SPEC_FAIL_OVERRIDDEN;
        case BUILTIN_CLASSMETHOD:
            return SPEC_FAIL_LOAD_METHOD_BUILTIN_CLASS_METHOD;
        case PYTHON_CLASSMETHOD:
            return SPEC_FAIL_LOAD_METHOD_CLASS_METHOD_OBJ;
        case NON_OVERRIDING:
            return SPEC_FAIL_LOAD_METHOD_NON_OVERRIDING_DESCRIPTOR;
        case NON_DESCRIPTOR:
            return SPEC_FAIL_LOAD_METHOD_NOT_DESCRIPTOR;
        case ABSENT:
            return SPEC_FAIL_LOAD_METHOD_INSTANCE_ATTRIBUTE;
    }
    Py_UNREACHABLE();
}
#endif

static int
specialize_class_load_method(PyObject *owner, _Py_CODEUNIT *instr,
                             PyObject *name)
{
    _PyLoadMethodCache *cache = (_PyLoadMethodCache *)(instr + 1);
    if (!PyType_CheckExact(owner) || _PyType_Lookup(Py_TYPE(owner), name)) {
        SPECIALIZATION_FAIL(LOAD_METHOD, SPEC_FAIL_LOAD_METHOD_METACLASS_ATTRIBUTE);
        return -1;
    }
    PyObject *descr = NULL;
    DescriptorClassification kind = 0;
    kind = analyze_descriptor((PyTypeObject *)owner, name, &descr, 0);
    switch (kind) {
        case METHOD:
        case NON_DESCRIPTOR:
            write_u32(cache->type_version, ((PyTypeObject *)owner)->tp_version_tag);
            write_obj(cache->descr, descr);
            _Py_SET_OPCODE(*instr, LOAD_METHOD_CLASS);
            return 0;
#ifdef Py_STATS
        case ABSENT:
            SPECIALIZATION_FAIL(LOAD_METHOD, SPEC_FAIL_EXPECTED_ERROR);
            return -1;
#endif
        default:
            SPECIALIZATION_FAIL(LOAD_METHOD, load_method_fail_kind(kind));
            return -1;
    }
}

typedef enum {
    MANAGED_VALUES = 1,
    MANAGED_DICT = 2,
    OFFSET_DICT = 3,
    NO_DICT = 4
} ObjectDictKind;

// Please collect stats carefully before and after modifying. A subtle change
// can cause a significant drop in cache hits. A possible test is
// python.exe -m test_typing test_re test_dis test_zlib.
int
_Py_Specialize_LoadMethod(PyObject *owner, _Py_CODEUNIT *instr, PyObject *name)
{
    assert(_PyOpcode_Caches[LOAD_METHOD] == INLINE_CACHE_ENTRIES_LOAD_METHOD);
    _PyLoadMethodCache *cache = (_PyLoadMethodCache *)(instr + 1);
    PyTypeObject *owner_cls = Py_TYPE(owner);

    if (PyModule_CheckExact(owner)) {
        assert(INLINE_CACHE_ENTRIES_LOAD_ATTR <=
               INLINE_CACHE_ENTRIES_LOAD_METHOD);
        int err = specialize_module_load_attr(owner, instr, name, LOAD_METHOD,
                                              LOAD_METHOD_MODULE);
        if (err) {
            goto fail;
        }
        goto success;
    }
    if (owner_cls->tp_dict == NULL) {
        if (PyType_Ready(owner_cls) < 0) {
            return -1;
        }
    }
    if (PyType_Check(owner)) {
        int err = specialize_class_load_method(owner, instr, name);
        if (err) {
            goto fail;
        }
        goto success;
    }

    PyObject *descr = NULL;
    DescriptorClassification kind = 0;
    kind = analyze_descriptor(owner_cls, name, &descr, 0);
    assert(descr != NULL || kind == ABSENT || kind == GETSET_OVERRIDDEN);
    if (kind != METHOD) {
        SPECIALIZATION_FAIL(LOAD_METHOD, load_method_fail_kind(kind));
        goto fail;
    }
    ObjectDictKind dictkind;
    PyDictKeysObject *keys;
    if (owner_cls->tp_flags & Py_TPFLAGS_MANAGED_DICT) {
        PyObject *dict = *_PyObject_ManagedDictPointer(owner);
        keys = ((PyHeapTypeObject *)owner_cls)->ht_cached_keys;
        if (dict == NULL) {
            dictkind = MANAGED_VALUES;
        }
        else {
            dictkind = MANAGED_DICT;
        }
    }
    else {
        Py_ssize_t dictoffset = owner_cls->tp_dictoffset;
        if (dictoffset < 0 || dictoffset > INT16_MAX) {
            SPECIALIZATION_FAIL(LOAD_METHOD, SPEC_FAIL_OUT_OF_RANGE);
            goto fail;
        }
        if (dictoffset == 0) {
            dictkind = NO_DICT;
            keys = NULL;
        }
        else {
            PyObject *dict = *(PyObject **) ((char *)owner + dictoffset);
            if (dict == NULL) {
                SPECIALIZATION_FAIL(LOAD_METHOD, SPEC_FAIL_NO_DICT);
                goto fail;
            }
            keys = ((PyDictObject *)dict)->ma_keys;
            dictkind = OFFSET_DICT;
        }
    }
    if (dictkind != NO_DICT) {
        Py_ssize_t index = _PyDictKeys_StringLookup(keys, name);
        if (index != DKIX_EMPTY) {
            SPECIALIZATION_FAIL(LOAD_METHOD, SPEC_FAIL_LOAD_METHOD_IS_ATTR);
            goto fail;
        }
        uint32_t keys_version = _PyDictKeys_GetVersionForCurrentState(keys);
        if (keys_version == 0) {
            SPECIALIZATION_FAIL(LOAD_METHOD, SPEC_FAIL_OUT_OF_VERSIONS);
            goto fail;
        }
        write_u32(cache->keys_version, keys_version);
    }
    switch(dictkind) {
        case NO_DICT:
            _Py_SET_OPCODE(*instr, LOAD_METHOD_NO_DICT);
            break;
        case MANAGED_VALUES:
            _Py_SET_OPCODE(*instr, LOAD_METHOD_WITH_VALUES);
            break;
        case MANAGED_DICT:
            *(int16_t *)&cache->dict_offset = (int16_t)MANAGED_DICT_OFFSET;
            _Py_SET_OPCODE(*instr, LOAD_METHOD_WITH_DICT);
            break;
        case OFFSET_DICT:
            assert(owner_cls->tp_dictoffset > 0 && owner_cls->tp_dictoffset <= INT16_MAX);
            cache->dict_offset = (uint16_t)owner_cls->tp_dictoffset;
            _Py_SET_OPCODE(*instr, LOAD_METHOD_WITH_DICT);
            break;
    }
    /* `descr` is borrowed. This is safe for methods (even inherited ones from
    *  super classes!) as long as tp_version_tag is validated for two main reasons:
    *
    *  1. The class will always hold a reference to the method so it will
    *  usually not be GC-ed. Should it be deleted in Python, e.g.
    *  `del obj.meth`, tp_version_tag will be invalidated, because of reason 2.
    *
    *  2. The pre-existing type method cache (MCACHE) uses the same principles
    *  of caching a borrowed descriptor. The MCACHE infrastructure does all the
    *  heavy lifting for us. E.g. it invalidates tp_version_tag on any MRO
    *  modification, on any type object change along said MRO, etc. (see
    *  PyType_Modified usages in typeobject.c). The MCACHE has been
    *  working since Python 2.6 and it's battle-tested.
    */
    write_u32(cache->type_version, owner_cls->tp_version_tag);
    write_obj(cache->descr, descr);
    // Fall through.
success:
    STAT_INC(LOAD_METHOD, success);
    assert(!PyErr_Occurred());
    cache->counter = miss_counter_start();
    return 0;
fail:
    STAT_INC(LOAD_METHOD, failure);
    assert(!PyErr_Occurred());
    cache->counter = adaptive_counter_backoff(cache->counter);
    return 0;
}

int
_Py_Specialize_LoadGlobal(
    PyObject *globals, PyObject *builtins,
    _Py_CODEUNIT *instr, PyObject *name)
{
    assert(_PyOpcode_Caches[LOAD_GLOBAL] == INLINE_CACHE_ENTRIES_LOAD_GLOBAL);
    /* Use inline cache */
    _PyLoadGlobalCache *cache = (_PyLoadGlobalCache *)(instr + 1);
    assert(PyUnicode_CheckExact(name));
    if (!PyDict_CheckExact(globals)) {
        goto fail;
    }
    PyDictKeysObject * globals_keys = ((PyDictObject *)globals)->ma_keys;
    if (!DK_IS_UNICODE(globals_keys)) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_LOAD_GLOBAL_NON_STRING_OR_SPLIT);
        goto fail;
    }
    Py_ssize_t index = _PyDictKeys_StringLookup(globals_keys, name);
    if (index == DKIX_ERROR) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_LOAD_GLOBAL_NON_STRING_OR_SPLIT);
        goto fail;
    }
    if (index != DKIX_EMPTY) {
        if (index != (uint16_t)index) {
            goto fail;
        }
        uint32_t keys_version = _PyDictKeys_GetVersionForCurrentState(globals_keys);
        if (keys_version == 0) {
            goto fail;
        }
        cache->index = (uint16_t)index;
        write_u32(cache->module_keys_version, keys_version);
        _Py_SET_OPCODE(*instr, LOAD_GLOBAL_MODULE);
        goto success;
    }
    if (!PyDict_CheckExact(builtins)) {
        goto fail;
    }
    PyDictKeysObject * builtin_keys = ((PyDictObject *)builtins)->ma_keys;
    if (!DK_IS_UNICODE(builtin_keys)) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_LOAD_GLOBAL_NON_STRING_OR_SPLIT);
        goto fail;
    }
    index = _PyDictKeys_StringLookup(builtin_keys, name);
    if (index == DKIX_ERROR) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_LOAD_GLOBAL_NON_STRING_OR_SPLIT);
        goto fail;
    }
    if (index != (uint16_t)index) {
        goto fail;
    }
    uint32_t globals_version = _PyDictKeys_GetVersionForCurrentState(globals_keys);
    if (globals_version == 0) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_OUT_OF_VERSIONS);
        goto fail;
    }
    uint32_t builtins_version = _PyDictKeys_GetVersionForCurrentState(builtin_keys);
    if (builtins_version == 0) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_OUT_OF_VERSIONS);
        goto fail;
    }
    if (builtins_version > UINT16_MAX) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_OUT_OF_RANGE);
        goto fail;
    }
    cache->index = (uint16_t)index;
    write_u32(cache->module_keys_version, globals_version);
    cache->builtin_keys_version = (uint16_t)builtins_version;
    _Py_SET_OPCODE(*instr, LOAD_GLOBAL_BUILTIN);
    goto success;
fail:
    STAT_INC(LOAD_GLOBAL, failure);
    assert(!PyErr_Occurred());
    cache->counter = adaptive_counter_backoff(cache->counter);
    return 0;
success:
    STAT_INC(LOAD_GLOBAL, success);
    assert(!PyErr_Occurred());
    cache->counter = miss_counter_start();
    return 0;
}

#ifdef Py_STATS
static int
binary_subscr_fail_kind(PyTypeObject *container_type, PyObject *sub)
{
    if (container_type == &PyUnicode_Type) {
        if (PyLong_CheckExact(sub)) {
            return SPEC_FAIL_SUBSCR_STRING_INT;
        }
        if (PySlice_Check(sub)) {
            return SPEC_FAIL_SUBSCR_STRING_SLICE;
        }
        return SPEC_FAIL_OTHER;
    }
    else if (strcmp(container_type->tp_name, "array.array") == 0) {
        if (PyLong_CheckExact(sub)) {
            return SPEC_FAIL_SUBSCR_ARRAY_INT;
        }
        if (PySlice_Check(sub)) {
            return SPEC_FAIL_SUBSCR_ARRAY_SLICE;
        }
        return SPEC_FAIL_OTHER;
    }
    else if (container_type->tp_as_buffer) {
        if (PyLong_CheckExact(sub)) {
            return SPEC_FAIL_SUBSCR_BUFFER_INT;
        }
        if (PySlice_Check(sub)) {
            return SPEC_FAIL_SUBSCR_BUFFER_SLICE;
        }
        return SPEC_FAIL_OTHER;
    }
    else if (container_type->tp_as_sequence) {
        if (PyLong_CheckExact(sub) && container_type->tp_as_sequence->sq_item) {
            return SPEC_FAIL_SUBSCR_SEQUENCE_INT;
        }
    }
    return SPEC_FAIL_OTHER;
}
#endif


#define SIMPLE_FUNCTION 0

static int
function_kind(PyCodeObject *code) {
    int flags = code->co_flags;
    if ((flags & (CO_VARKEYWORDS | CO_VARARGS)) || code->co_kwonlyargcount) {
        return SPEC_FAIL_CALL_COMPLEX_PARAMETERS;
    }
    if ((flags & CO_OPTIMIZED) == 0) {
        return SPEC_FAIL_CALL_CO_NOT_OPTIMIZED;
    }
    return SIMPLE_FUNCTION;
}

int
_Py_Specialize_BinarySubscr(
     PyObject *container, PyObject *sub, _Py_CODEUNIT *instr)
{
    assert(_PyOpcode_Caches[BINARY_SUBSCR] ==
           INLINE_CACHE_ENTRIES_BINARY_SUBSCR);
    _PyBinarySubscrCache *cache = (_PyBinarySubscrCache *)(instr + 1);
    PyTypeObject *container_type = Py_TYPE(container);
    if (container_type == &PyList_Type) {
        if (PyLong_CheckExact(sub)) {
            _Py_SET_OPCODE(*instr, BINARY_SUBSCR_LIST_INT);
            goto success;
        }
        SPECIALIZATION_FAIL(BINARY_SUBSCR,
            PySlice_Check(sub) ? SPEC_FAIL_SUBSCR_LIST_SLICE : SPEC_FAIL_OTHER);
        goto fail;
    }
    if (container_type == &PyTuple_Type) {
        if (PyLong_CheckExact(sub)) {
            _Py_SET_OPCODE(*instr, BINARY_SUBSCR_TUPLE_INT);
            goto success;
        }
        SPECIALIZATION_FAIL(BINARY_SUBSCR,
            PySlice_Check(sub) ? SPEC_FAIL_SUBSCR_TUPLE_SLICE : SPEC_FAIL_OTHER);
        goto fail;
    }
    if (container_type == &PyDict_Type) {
        _Py_SET_OPCODE(*instr, BINARY_SUBSCR_DICT);
        goto success;
    }
    PyTypeObject *cls = Py_TYPE(container);
    PyObject *descriptor = _PyType_Lookup(cls, &_Py_ID(__getitem__));
    if (descriptor && Py_TYPE(descriptor) == &PyFunction_Type) {
        if (!(container_type->tp_flags & Py_TPFLAGS_HEAPTYPE)) {
            SPECIALIZATION_FAIL(BINARY_SUBSCR, SPEC_FAIL_SUBSCR_NOT_HEAP_TYPE);
            goto fail;
        }
        PyFunctionObject *func = (PyFunctionObject *)descriptor;
        PyCodeObject *fcode = (PyCodeObject *)func->func_code;
        int kind = function_kind(fcode);
        if (kind != SIMPLE_FUNCTION) {
            SPECIALIZATION_FAIL(BINARY_SUBSCR, kind);
            goto fail;
        }
        if (fcode->co_argcount != 2) {
            SPECIALIZATION_FAIL(BINARY_SUBSCR, SPEC_FAIL_WRONG_NUMBER_ARGUMENTS);
            goto fail;
        }
        assert(cls->tp_version_tag != 0);
        write_u32(cache->type_version, cls->tp_version_tag);
        int version = _PyFunction_GetVersionForCurrentState(func);
        if (version == 0 || version != (uint16_t)version) {
            SPECIALIZATION_FAIL(BINARY_SUBSCR, SPEC_FAIL_OUT_OF_VERSIONS);
            goto fail;
        }
        if (_PyInterpreterState_GET()->eval_frame) {
            SPECIALIZATION_FAIL(BINARY_SUBSCR, SPEC_FAIL_OTHER);
            goto fail;
        }
        cache->func_version = version;
        ((PyHeapTypeObject *)container_type)->_spec_cache.getitem = descriptor;
        _Py_SET_OPCODE(*instr, BINARY_SUBSCR_GETITEM);
        goto success;
    }
    SPECIALIZATION_FAIL(BINARY_SUBSCR,
                        binary_subscr_fail_kind(container_type, sub));
fail:
    STAT_INC(BINARY_SUBSCR, failure);
    assert(!PyErr_Occurred());
    cache->counter = adaptive_counter_backoff(cache->counter);
    return 0;
success:
    STAT_INC(BINARY_SUBSCR, success);
    assert(!PyErr_Occurred());
    cache->counter = miss_counter_start();
    return 0;
}

int
_Py_Specialize_StoreSubscr(PyObject *container, PyObject *sub, _Py_CODEUNIT *instr)
{
    _PyStoreSubscrCache *cache = (_PyStoreSubscrCache *)(instr + 1);
    PyTypeObject *container_type = Py_TYPE(container);
    if (container_type == &PyList_Type) {
        if (PyLong_CheckExact(sub)) {
            if ((Py_SIZE(sub) == 0 || Py_SIZE(sub) == 1)
                && ((PyLongObject *)sub)->ob_digit[0] < (size_t)PyList_GET_SIZE(container))
            {
                _Py_SET_OPCODE(*instr, STORE_SUBSCR_LIST_INT);
                goto success;
            }
            else {
                SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_OUT_OF_RANGE);
                goto fail;
            }
        }
        else if (PySlice_Check(sub)) {
            SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_SUBSCR_LIST_SLICE);
            goto fail;
        }
        else {
            SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_OTHER);
            goto fail;
        }
    }
    if (container_type == &PyDict_Type) {
        _Py_SET_OPCODE(*instr, STORE_SUBSCR_DICT);
         goto success;
    }
#ifdef Py_STATS
    PyMappingMethods *as_mapping = container_type->tp_as_mapping;
    if (as_mapping && (as_mapping->mp_ass_subscript
                       == PyDict_Type.tp_as_mapping->mp_ass_subscript)) {
        SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_SUBSCR_DICT_SUBCLASS_NO_OVERRIDE);
        goto fail;
    }
    if (PyObject_CheckBuffer(container)) {
        if (PyLong_CheckExact(sub) && (((size_t)Py_SIZE(sub)) > 1)) {
            SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_OUT_OF_RANGE);
        }
        else if (strcmp(container_type->tp_name, "array.array") == 0) {
            if (PyLong_CheckExact(sub)) {
                SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_SUBSCR_ARRAY_INT);
            }
            else if (PySlice_Check(sub)) {
                SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_SUBSCR_ARRAY_SLICE);
            }
            else {
                SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_OTHER);
            }
        }
        else if (PyByteArray_CheckExact(container)) {
            if (PyLong_CheckExact(sub)) {
                SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_SUBSCR_BYTEARRAY_INT);
            }
            else if (PySlice_Check(sub)) {
                SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_SUBSCR_BYTEARRAY_SLICE);
            }
            else {
                SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_OTHER);
            }
        }
        else {
            if (PyLong_CheckExact(sub)) {
                SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_SUBSCR_BUFFER_INT);
            }
            else if (PySlice_Check(sub)) {
                SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_SUBSCR_BUFFER_SLICE);
            }
            else {
                SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_OTHER);
            }
        }
        goto fail;
    }
    PyObject *descriptor = _PyType_Lookup(container_type, &_Py_ID(__setitem__));
    if (descriptor && Py_TYPE(descriptor) == &PyFunction_Type) {
        PyFunctionObject *func = (PyFunctionObject *)descriptor;
        PyCodeObject *code = (PyCodeObject *)func->func_code;
        int kind = function_kind(code);
        if (kind == SIMPLE_FUNCTION) {
            SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_SUBSCR_PY_SIMPLE);
        }
        else {
            SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_SUBSCR_PY_OTHER);
        }
        goto fail;
    }
#endif
    SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_OTHER);
fail:
    STAT_INC(STORE_SUBSCR, failure);
    assert(!PyErr_Occurred());
    cache->counter = adaptive_counter_backoff(cache->counter);
    return 0;
success:
    STAT_INC(STORE_SUBSCR, success);
    assert(!PyErr_Occurred());
    cache->counter = miss_counter_start();
    return 0;
}

static int
specialize_class_call(PyObject *callable, _Py_CODEUNIT *instr, int nargs,
                      PyObject *kwnames, int oparg)
{
    assert(_Py_OPCODE(*instr) == PRECALL_ADAPTIVE);
    PyTypeObject *tp = _PyType_CAST(callable);
    if (tp->tp_new == PyBaseObject_Type.tp_new) {
        SPECIALIZATION_FAIL(PRECALL, SPEC_FAIL_CALL_PYTHON_CLASS);
        return -1;
    }
    if (tp->tp_flags & Py_TPFLAGS_IMMUTABLETYPE) {
        if (nargs == 1 && kwnames == NULL && oparg == 1) {
            if (tp == &PyUnicode_Type) {
                _Py_SET_OPCODE(*instr, PRECALL_NO_KW_STR_1);
                return 0;
            }
            else if (tp == &PyType_Type) {
                _Py_SET_OPCODE(*instr, PRECALL_NO_KW_TYPE_1);
                return 0;
            }
            else if (tp == &PyTuple_Type) {
                _Py_SET_OPCODE(*instr, PRECALL_NO_KW_TUPLE_1);
                return 0;
            }
        }
        if (tp->tp_vectorcall != NULL) {
            _Py_SET_OPCODE(*instr, PRECALL_BUILTIN_CLASS);
            return 0;
        }
        SPECIALIZATION_FAIL(PRECALL, tp == &PyUnicode_Type ?
            SPEC_FAIL_CALL_STR : SPEC_FAIL_CALL_CLASS_NO_VECTORCALL);
        return -1;
    }
    SPECIALIZATION_FAIL(PRECALL, SPEC_FAIL_CALL_CLASS_MUTABLE);
    return -1;
}

#ifdef Py_STATS
static int
builtin_call_fail_kind(int ml_flags)
{
    switch (ml_flags & (METH_VARARGS | METH_FASTCALL | METH_NOARGS | METH_O |
        METH_KEYWORDS | METH_METHOD)) {
        case METH_VARARGS:
            return SPEC_FAIL_CALL_PYCFUNCTION;
        case METH_VARARGS | METH_KEYWORDS:
            return SPEC_FAIL_CALL_PYCFUNCTION_WITH_KEYWORDS;
        case METH_FASTCALL | METH_KEYWORDS:
            return SPEC_FAIL_CALL_PYCFUNCTION_FAST_WITH_KEYWORDS;
        case METH_NOARGS:
            return SPEC_FAIL_CALL_PYCFUNCTION_NOARGS;
        /* This case should never happen with PyCFunctionObject -- only
            PyMethodObject. See zlib.compressobj()'s methods for an example.
        */
        case METH_METHOD | METH_FASTCALL | METH_KEYWORDS:
        default:
            return SPEC_FAIL_CALL_BAD_CALL_FLAGS;
    }
}
#endif

static int
specialize_method_descriptor(PyMethodDescrObject *descr, _Py_CODEUNIT *instr,
                             int nargs, PyObject *kwnames, int oparg)
{
    assert(_Py_OPCODE(*instr) == PRECALL_ADAPTIVE);
    if (kwnames) {
        SPECIALIZATION_FAIL(PRECALL, SPEC_FAIL_CALL_KWNAMES);
        return -1;
    }

    switch (descr->d_method->ml_flags &
        (METH_VARARGS | METH_FASTCALL | METH_NOARGS | METH_O |
        METH_KEYWORDS | METH_METHOD)) {
        case METH_NOARGS: {
            if (nargs != 1) {
                SPECIALIZATION_FAIL(PRECALL, SPEC_FAIL_WRONG_NUMBER_ARGUMENTS);
                return -1;
            }
            _Py_SET_OPCODE(*instr, PRECALL_NO_KW_METHOD_DESCRIPTOR_NOARGS);
            return 0;
        }
        case METH_O: {
            if (nargs != 2) {
                SPECIALIZATION_FAIL(PRECALL, SPEC_FAIL_WRONG_NUMBER_ARGUMENTS);
                return -1;
            }
            PyInterpreterState *interp = _PyInterpreterState_GET();
            PyObject *list_append = interp->callable_cache.list_append;
            _Py_CODEUNIT next = instr[INLINE_CACHE_ENTRIES_PRECALL + 1
                                      + INLINE_CACHE_ENTRIES_CALL + 1];
            bool pop = (_Py_OPCODE(next) == POP_TOP);
            if ((PyObject *)descr == list_append && oparg == 1 && pop) {
                _Py_SET_OPCODE(*instr, PRECALL_NO_KW_LIST_APPEND);
                return 0;
            }
            _Py_SET_OPCODE(*instr, PRECALL_NO_KW_METHOD_DESCRIPTOR_O);
            return 0;
        }
        case METH_FASTCALL: {
            _Py_SET_OPCODE(*instr, PRECALL_NO_KW_METHOD_DESCRIPTOR_FAST);
            return 0;
        }
        case METH_FASTCALL|METH_KEYWORDS: {
            _Py_SET_OPCODE(*instr, PRECALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS);
            return 0;
        }
    }
    SPECIALIZATION_FAIL(PRECALL, builtin_call_fail_kind(descr->d_method->ml_flags));
    return -1;
}

static int
specialize_py_call(PyFunctionObject *func, _Py_CODEUNIT *instr, int nargs,
                   PyObject *kwnames)
{
    _PyCallCache *cache = (_PyCallCache *)(instr + 1);
    assert(_Py_OPCODE(*instr) == CALL_ADAPTIVE);
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    int kind = function_kind(code);
    /* Don't specialize if PEP 523 is active */
    if (_PyInterpreterState_GET()->eval_frame) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_PEP_523);
        return -1;
    }
    if (kwnames) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_KWNAMES);
        return -1;
    }
    if (kind != SIMPLE_FUNCTION) {
        SPECIALIZATION_FAIL(CALL, kind);
        return -1;
    }
    int argcount = code->co_argcount;
    int defcount = func->func_defaults == NULL ? 0 : (int)PyTuple_GET_SIZE(func->func_defaults);
    int min_args = argcount-defcount;
    // GH-105840: min_args is negative when somebody sets too many __defaults__!
    if (min_args < 0 || nargs > argcount || nargs < min_args) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_WRONG_NUMBER_ARGUMENTS);
        return -1;
    }
    assert(nargs <= argcount && nargs >= min_args);
    assert(min_args >= 0 && defcount >= 0);
    assert(defcount == 0 || func->func_defaults != NULL);
    if (min_args > 0xffff) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_OUT_OF_RANGE);
        return -1;
    }
    int version = _PyFunction_GetVersionForCurrentState(func);
    if (version == 0) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_OUT_OF_VERSIONS);
        return -1;
    }
    write_u32(cache->func_version, version);
    cache->min_args = min_args;
    if (argcount == nargs) {
        _Py_SET_OPCODE(*instr, CALL_PY_EXACT_ARGS);
    }
    else {
        _Py_SET_OPCODE(*instr, CALL_PY_WITH_DEFAULTS);
    }
    return 0;
}

static int
specialize_c_call(PyObject *callable, _Py_CODEUNIT *instr, int nargs,
                  PyObject *kwnames)
{
    assert(_Py_OPCODE(*instr) == PRECALL_ADAPTIVE);
    if (PyCFunction_GET_FUNCTION(callable) == NULL) {
        return 1;
    }
    switch (PyCFunction_GET_FLAGS(callable) &
        (METH_VARARGS | METH_FASTCALL | METH_NOARGS | METH_O |
        METH_KEYWORDS | METH_METHOD)) {
        case METH_O: {
            if (kwnames) {
                SPECIALIZATION_FAIL(PRECALL, SPEC_FAIL_CALL_KWNAMES);
                return -1;
            }
            if (nargs != 1) {
                SPECIALIZATION_FAIL(PRECALL, SPEC_FAIL_WRONG_NUMBER_ARGUMENTS);
                return 1;
            }
            /* len(o) */
            PyInterpreterState *interp = _PyInterpreterState_GET();
            if (callable == interp->callable_cache.len) {
                _Py_SET_OPCODE(*instr, PRECALL_NO_KW_LEN);
                return 0;
            }
            _Py_SET_OPCODE(*instr, PRECALL_NO_KW_BUILTIN_O);
            return 0;
        }
        case METH_FASTCALL: {
            if (kwnames) {
                SPECIALIZATION_FAIL(PRECALL, SPEC_FAIL_CALL_KWNAMES);
                return -1;
            }
            if (nargs == 2) {
                /* isinstance(o1, o2) */
                PyInterpreterState *interp = _PyInterpreterState_GET();
                if (callable == interp->callable_cache.isinstance) {
                    _Py_SET_OPCODE(*instr, PRECALL_NO_KW_ISINSTANCE);
                    return 0;
                }
            }
            _Py_SET_OPCODE(*instr, PRECALL_NO_KW_BUILTIN_FAST);
            return 0;
        }
        case METH_FASTCALL | METH_KEYWORDS: {
            _Py_SET_OPCODE(*instr, PRECALL_BUILTIN_FAST_WITH_KEYWORDS);
            return 0;
        }
        default:
            SPECIALIZATION_FAIL(PRECALL,
                builtin_call_fail_kind(PyCFunction_GET_FLAGS(callable)));
            return 1;
    }
}

#ifdef Py_STATS
static int
call_fail_kind(PyObject *callable)
{
    if (PyCFunction_CheckExact(callable)) {
        return SPEC_FAIL_CALL_PYCFUNCTION;
    }
    else if (PyFunction_Check(callable)) {
        return SPEC_FAIL_CALL_PYFUNCTION;
    }
    else if (PyInstanceMethod_Check(callable)) {
        return SPEC_FAIL_CALL_INSTANCE_METHOD;
    }
    else if (PyMethod_Check(callable)) {
        return SPEC_FAIL_CALL_BOUND_METHOD;
    }
    // builtin method
    else if (PyCMethod_Check(callable)) {
        return SPEC_FAIL_CALL_CMETHOD;
    }
    else if (PyType_Check(callable)) {
        if (((PyTypeObject *)callable)->tp_new == PyBaseObject_Type.tp_new) {
            return SPEC_FAIL_CALL_PYTHON_CLASS;
        }
        else {
            return SPEC_FAIL_CALL_CLASS;
        }
    }
    else if (Py_IS_TYPE(callable, &PyMethodDescr_Type)) {
        return SPEC_FAIL_CALL_METHOD_DESCRIPTOR;
    }
    else if (Py_TYPE(callable) == &PyWrapperDescr_Type) {
        return SPEC_FAIL_CALL_OPERATOR_WRAPPER;
    }
    else if (Py_TYPE(callable) == &_PyMethodWrapper_Type) {
        return SPEC_FAIL_CALL_METHOD_WRAPPER;
    }
    return SPEC_FAIL_OTHER;
}
#endif


int
_Py_Specialize_Precall(PyObject *callable, _Py_CODEUNIT *instr, int nargs,
                       PyObject *kwnames, int oparg)
{
    assert(_PyOpcode_Caches[PRECALL] == INLINE_CACHE_ENTRIES_PRECALL);
    _PyPrecallCache *cache = (_PyPrecallCache *)(instr + 1);
    int fail;
    if (PyCFunction_CheckExact(callable)) {
        fail = specialize_c_call(callable, instr, nargs, kwnames);
    }
    else if (PyFunction_Check(callable)) {
        _Py_SET_OPCODE(*instr, PRECALL_PYFUNC);
        fail = 0;
    }
    else if (PyType_Check(callable)) {
        fail = specialize_class_call(callable, instr, nargs, kwnames, oparg);
    }
    else if (Py_IS_TYPE(callable, &PyMethodDescr_Type)) {
        fail = specialize_method_descriptor((PyMethodDescrObject *)callable,
                                            instr, nargs, kwnames, oparg);
    }
    else if (Py_TYPE(callable) == &PyMethod_Type) {
        _Py_SET_OPCODE(*instr, PRECALL_BOUND_METHOD);
        fail = 0;
    }
    else {
        SPECIALIZATION_FAIL(PRECALL, call_fail_kind(callable));
        fail = -1;
    }
    if (fail) {
        STAT_INC(PRECALL, failure);
        assert(!PyErr_Occurred());
        cache->counter = adaptive_counter_backoff(cache->counter);
    }
    else {
        STAT_INC(PRECALL, success);
        assert(!PyErr_Occurred());
        cache->counter = miss_counter_start();
    }
    return 0;
}


/* TODO:
    - Specialize calling classes.
*/
int
_Py_Specialize_Call(PyObject *callable, _Py_CODEUNIT *instr, int nargs,
                    PyObject *kwnames)
{
    assert(_PyOpcode_Caches[CALL] == INLINE_CACHE_ENTRIES_CALL);
    _PyCallCache *cache = (_PyCallCache *)(instr + 1);
    int fail;
    if (PyFunction_Check(callable)) {
        fail = specialize_py_call((PyFunctionObject *)callable, instr, nargs,
                                  kwnames);
    }
    else {
        SPECIALIZATION_FAIL(CALL, call_fail_kind(callable));
        fail = -1;
    }
    if (fail) {
        STAT_INC(CALL, failure);
        assert(!PyErr_Occurred());
        cache->counter = adaptive_counter_backoff(cache->counter);
    }
    else {
        STAT_INC(CALL, success);
        assert(!PyErr_Occurred());
        cache->counter = miss_counter_start();
    }
    return 0;
}

#ifdef Py_STATS
static int
binary_op_fail_kind(int oparg, PyObject *lhs, PyObject *rhs)
{
    switch (oparg) {
        case NB_ADD:
        case NB_INPLACE_ADD:
            if (!Py_IS_TYPE(lhs, Py_TYPE(rhs))) {
                return SPEC_FAIL_BINARY_OP_ADD_DIFFERENT_TYPES;
            }
            return SPEC_FAIL_BINARY_OP_ADD_OTHER;
        case NB_AND:
        case NB_INPLACE_AND:
            if (!Py_IS_TYPE(lhs, Py_TYPE(rhs))) {
                return SPEC_FAIL_BINARY_OP_AND_DIFFERENT_TYPES;
            }
            if (PyLong_CheckExact(lhs)) {
                return SPEC_FAIL_BINARY_OP_AND_INT;
            }
            return SPEC_FAIL_BINARY_OP_AND_OTHER;
        case NB_FLOOR_DIVIDE:
        case NB_INPLACE_FLOOR_DIVIDE:
            return SPEC_FAIL_BINARY_OP_FLOOR_DIVIDE;
        case NB_LSHIFT:
        case NB_INPLACE_LSHIFT:
            return SPEC_FAIL_BINARY_OP_LSHIFT;
        case NB_MATRIX_MULTIPLY:
        case NB_INPLACE_MATRIX_MULTIPLY:
            return SPEC_FAIL_BINARY_OP_MATRIX_MULTIPLY;
        case NB_MULTIPLY:
        case NB_INPLACE_MULTIPLY:
            if (!Py_IS_TYPE(lhs, Py_TYPE(rhs))) {
                return SPEC_FAIL_BINARY_OP_MULTIPLY_DIFFERENT_TYPES;
            }
            return SPEC_FAIL_BINARY_OP_MULTIPLY_OTHER;
        case NB_OR:
        case NB_INPLACE_OR:
            return SPEC_FAIL_BINARY_OP_OR;
        case NB_POWER:
        case NB_INPLACE_POWER:
            return SPEC_FAIL_BINARY_OP_POWER;
        case NB_REMAINDER:
        case NB_INPLACE_REMAINDER:
            return SPEC_FAIL_BINARY_OP_REMAINDER;
        case NB_RSHIFT:
        case NB_INPLACE_RSHIFT:
            return SPEC_FAIL_BINARY_OP_RSHIFT;
        case NB_SUBTRACT:
        case NB_INPLACE_SUBTRACT:
            if (!Py_IS_TYPE(lhs, Py_TYPE(rhs))) {
                return SPEC_FAIL_BINARY_OP_SUBTRACT_DIFFERENT_TYPES;
            }
            return SPEC_FAIL_BINARY_OP_SUBTRACT_OTHER;
        case NB_TRUE_DIVIDE:
        case NB_INPLACE_TRUE_DIVIDE:
            if (!Py_IS_TYPE(lhs, Py_TYPE(rhs))) {
                return SPEC_FAIL_BINARY_OP_TRUE_DIVIDE_DIFFERENT_TYPES;
            }
            if (PyFloat_CheckExact(lhs)) {
                return SPEC_FAIL_BINARY_OP_TRUE_DIVIDE_FLOAT;
            }
            return SPEC_FAIL_BINARY_OP_TRUE_DIVIDE_OTHER;
        case NB_XOR:
        case NB_INPLACE_XOR:
            return SPEC_FAIL_BINARY_OP_XOR;
    }
    Py_UNREACHABLE();
}
#endif

void
_Py_Specialize_BinaryOp(PyObject *lhs, PyObject *rhs, _Py_CODEUNIT *instr,
                        int oparg, PyObject **locals)
{
    assert(_PyOpcode_Caches[BINARY_OP] == INLINE_CACHE_ENTRIES_BINARY_OP);
    _PyBinaryOpCache *cache = (_PyBinaryOpCache *)(instr + 1);
    switch (oparg) {
        case NB_ADD:
        case NB_INPLACE_ADD:
            if (!Py_IS_TYPE(lhs, Py_TYPE(rhs))) {
                break;
            }
            if (PyUnicode_CheckExact(lhs)) {
                _Py_CODEUNIT next = instr[INLINE_CACHE_ENTRIES_BINARY_OP + 1];
                bool to_store = (_Py_OPCODE(next) == STORE_FAST ||
                                 _Py_OPCODE(next) == STORE_FAST__LOAD_FAST);
                if (to_store && locals[_Py_OPARG(next)] == lhs) {
                    _Py_SET_OPCODE(*instr, BINARY_OP_INPLACE_ADD_UNICODE);
                    goto success;
                }
                _Py_SET_OPCODE(*instr, BINARY_OP_ADD_UNICODE);
                goto success;
            }
            if (PyLong_CheckExact(lhs)) {
                _Py_SET_OPCODE(*instr, BINARY_OP_ADD_INT);
                goto success;
            }
            if (PyFloat_CheckExact(lhs)) {
                _Py_SET_OPCODE(*instr, BINARY_OP_ADD_FLOAT);
                goto success;
            }
            break;
        case NB_MULTIPLY:
        case NB_INPLACE_MULTIPLY:
            if (!Py_IS_TYPE(lhs, Py_TYPE(rhs))) {
                break;
            }
            if (PyLong_CheckExact(lhs)) {
                _Py_SET_OPCODE(*instr, BINARY_OP_MULTIPLY_INT);
                goto success;
            }
            if (PyFloat_CheckExact(lhs)) {
                _Py_SET_OPCODE(*instr, BINARY_OP_MULTIPLY_FLOAT);
                goto success;
            }
            break;
        case NB_SUBTRACT:
        case NB_INPLACE_SUBTRACT:
            if (!Py_IS_TYPE(lhs, Py_TYPE(rhs))) {
                break;
            }
            if (PyLong_CheckExact(lhs)) {
                _Py_SET_OPCODE(*instr, BINARY_OP_SUBTRACT_INT);
                goto success;
            }
            if (PyFloat_CheckExact(lhs)) {
                _Py_SET_OPCODE(*instr, BINARY_OP_SUBTRACT_FLOAT);
                goto success;
            }
            break;
#ifndef Py_STATS
        default:
            // These operators don't have any available specializations. Rather
            // than repeatedly attempting to specialize them, just convert them
            // back to BINARY_OP (unless we're collecting stats, where it's more
            // important to get accurate hit counts for the unadaptive version
            // and each of the different failure types):
            _Py_SET_OPCODE(*instr, BINARY_OP);
            return;
#endif
    }
    SPECIALIZATION_FAIL(BINARY_OP, binary_op_fail_kind(oparg, lhs, rhs));
    STAT_INC(BINARY_OP, failure);
    cache->counter = adaptive_counter_backoff(cache->counter);
    return;
success:
    STAT_INC(BINARY_OP, success);
    cache->counter = miss_counter_start();
}


#ifdef Py_STATS
static int
compare_op_fail_kind(PyObject *lhs, PyObject *rhs)
{
    if (Py_TYPE(lhs) != Py_TYPE(rhs)) {
        if (PyFloat_CheckExact(lhs) && PyLong_CheckExact(rhs)) {
            return SPEC_FAIL_COMPARE_OP_FLOAT_LONG;
        }
        if (PyLong_CheckExact(lhs) && PyFloat_CheckExact(rhs)) {
            return SPEC_FAIL_COMPARE_OP_LONG_FLOAT;
        }
        return SPEC_FAIL_COMPARE_OP_DIFFERENT_TYPES;
    }
    if (PyBytes_CheckExact(lhs)) {
        return SPEC_FAIL_COMPARE_OP_BYTES;
    }
    if (PyTuple_CheckExact(lhs)) {
        return SPEC_FAIL_COMPARE_OP_TUPLE;
    }
    if (PyList_CheckExact(lhs)) {
        return SPEC_FAIL_COMPARE_OP_LIST;
    }
    if (PySet_CheckExact(lhs) || PyFrozenSet_CheckExact(lhs)) {
        return SPEC_FAIL_COMPARE_OP_SET;
    }
    if (PyBool_Check(lhs)) {
        return SPEC_FAIL_COMPARE_OP_BOOL;
    }
    if (Py_TYPE(lhs)->tp_richcompare == PyBaseObject_Type.tp_richcompare) {
        return SPEC_FAIL_COMPARE_OP_BASEOBJECT;
    }
    return SPEC_FAIL_OTHER;
}
#endif


static int compare_masks[] = {
    // 1-bit: jump if less than
    // 2-bit: jump if equal
    // 4-bit: jump if greater
    [Py_LT] = 1 | 0 | 0,
    [Py_LE] = 1 | 2 | 0,
    [Py_EQ] = 0 | 2 | 0,
    [Py_NE] = 1 | 0 | 4,
    [Py_GT] = 0 | 0 | 4,
    [Py_GE] = 0 | 2 | 4,
};

void
_Py_Specialize_CompareOp(PyObject *lhs, PyObject *rhs, _Py_CODEUNIT *instr,
                         int oparg)
{
    assert(_PyOpcode_Caches[COMPARE_OP] == INLINE_CACHE_ENTRIES_COMPARE_OP);
    _PyCompareOpCache *cache = (_PyCompareOpCache *)(instr + 1);
    int next_opcode = _Py_OPCODE(instr[INLINE_CACHE_ENTRIES_COMPARE_OP + 1]);
    if (next_opcode != POP_JUMP_FORWARD_IF_FALSE &&
        next_opcode != POP_JUMP_BACKWARD_IF_FALSE &&
        next_opcode != POP_JUMP_FORWARD_IF_TRUE &&
        next_opcode != POP_JUMP_BACKWARD_IF_TRUE) {
        // Can't ever combine, so don't don't bother being adaptive (unless
        // we're collecting stats, where it's more important to get accurate hit
        // counts for the unadaptive version and each of the different failure
        // types):
#ifndef Py_STATS
        _Py_SET_OPCODE(*instr, COMPARE_OP);
        return;
#else
        if (next_opcode == EXTENDED_ARG) {
            SPECIALIZATION_FAIL(COMPARE_OP, SPEC_FAIL_COMPARE_OP_EXTENDED_ARG);
            goto failure;
        }
        SPECIALIZATION_FAIL(COMPARE_OP, SPEC_FAIL_COMPARE_OP_NOT_FOLLOWED_BY_COND_JUMP);
        goto failure;
#endif
    }
    assert(oparg <= Py_GE);
    int when_to_jump_mask = compare_masks[oparg];
    if (next_opcode == POP_JUMP_FORWARD_IF_FALSE ||
        next_opcode == POP_JUMP_BACKWARD_IF_FALSE) {
        when_to_jump_mask = (1 | 2 | 4) & ~when_to_jump_mask;
    }
    if (next_opcode == POP_JUMP_BACKWARD_IF_TRUE ||
        next_opcode == POP_JUMP_BACKWARD_IF_FALSE) {
        when_to_jump_mask <<= 3;
    }
    if (Py_TYPE(lhs) != Py_TYPE(rhs)) {
        SPECIALIZATION_FAIL(COMPARE_OP, compare_op_fail_kind(lhs, rhs));
        goto failure;
    }
    if (PyFloat_CheckExact(lhs)) {
        _Py_SET_OPCODE(*instr, COMPARE_OP_FLOAT_JUMP);
        cache->mask = when_to_jump_mask;
        goto success;
    }
    if (PyLong_CheckExact(lhs)) {
        if (Py_ABS(Py_SIZE(lhs)) <= 1 && Py_ABS(Py_SIZE(rhs)) <= 1) {
            _Py_SET_OPCODE(*instr, COMPARE_OP_INT_JUMP);
            cache->mask = when_to_jump_mask;
            goto success;
        }
        else {
            SPECIALIZATION_FAIL(COMPARE_OP, SPEC_FAIL_COMPARE_OP_BIG_INT);
            goto failure;
        }
    }
    if (PyUnicode_CheckExact(lhs)) {
        if (oparg != Py_EQ && oparg != Py_NE) {
            SPECIALIZATION_FAIL(COMPARE_OP, SPEC_FAIL_COMPARE_OP_STRING);
            goto failure;
        }
        else {
            _Py_SET_OPCODE(*instr, COMPARE_OP_STR_JUMP);
            cache->mask = when_to_jump_mask;
            goto success;
        }
    }
    SPECIALIZATION_FAIL(COMPARE_OP, compare_op_fail_kind(lhs, rhs));
failure:
    STAT_INC(COMPARE_OP, failure);
    cache->counter = adaptive_counter_backoff(cache->counter);
    return;
success:
    STAT_INC(COMPARE_OP, success);
    cache->counter = miss_counter_start();
}

#ifdef Py_STATS
static int
unpack_sequence_fail_kind(PyObject *seq)
{
    if (PySequence_Check(seq)) {
        return SPEC_FAIL_UNPACK_SEQUENCE_SEQUENCE;
    }
    if (PyIter_Check(seq)) {
        return SPEC_FAIL_UNPACK_SEQUENCE_ITERATOR;
    }
    return SPEC_FAIL_OTHER;
}
#endif

void
_Py_Specialize_UnpackSequence(PyObject *seq, _Py_CODEUNIT *instr, int oparg)
{
    assert(_PyOpcode_Caches[UNPACK_SEQUENCE] ==
           INLINE_CACHE_ENTRIES_UNPACK_SEQUENCE);
    _PyUnpackSequenceCache *cache = (_PyUnpackSequenceCache *)(instr + 1);
    if (PyTuple_CheckExact(seq)) {
        if (PyTuple_GET_SIZE(seq) != oparg) {
            SPECIALIZATION_FAIL(UNPACK_SEQUENCE, SPEC_FAIL_EXPECTED_ERROR);
            goto failure;
        }
        if (PyTuple_GET_SIZE(seq) == 2) {
            _Py_SET_OPCODE(*instr, UNPACK_SEQUENCE_TWO_TUPLE);
            goto success;
        }
        _Py_SET_OPCODE(*instr, UNPACK_SEQUENCE_TUPLE);
        goto success;
    }
    if (PyList_CheckExact(seq)) {
        if (PyList_GET_SIZE(seq) != oparg) {
            SPECIALIZATION_FAIL(UNPACK_SEQUENCE, SPEC_FAIL_EXPECTED_ERROR);
            goto failure;
        }
        _Py_SET_OPCODE(*instr, UNPACK_SEQUENCE_LIST);
        goto success;
    }
    SPECIALIZATION_FAIL(UNPACK_SEQUENCE, unpack_sequence_fail_kind(seq));
failure:
    STAT_INC(UNPACK_SEQUENCE, failure);
    cache->counter = adaptive_counter_backoff(cache->counter);
    return;
success:
    STAT_INC(UNPACK_SEQUENCE, success);
    cache->counter = miss_counter_start();
}

#ifdef Py_STATS

int
 _PySpecialization_ClassifyIterator(PyObject *iter)
{
    if (PyGen_CheckExact(iter)) {
        return SPEC_FAIL_FOR_ITER_GENERATOR;
    }
    if (PyCoro_CheckExact(iter)) {
        return SPEC_FAIL_FOR_ITER_COROUTINE;
    }
    if (PyAsyncGen_CheckExact(iter)) {
        return SPEC_FAIL_FOR_ITER_ASYNC_GENERATOR;
    }
    PyTypeObject *t = Py_TYPE(iter);
    if (t == &PyListIter_Type) {
        return SPEC_FAIL_FOR_ITER_LIST;
    }
    if (t == &PyTupleIter_Type) {
        return SPEC_FAIL_FOR_ITER_TUPLE;
    }
    if (t == &PyDictIterKey_Type) {
        return SPEC_FAIL_FOR_ITER_DICT_KEYS;
    }
    if (t == &PyDictIterValue_Type) {
        return SPEC_FAIL_FOR_ITER_DICT_VALUES;
    }
    if (t == &PyDictIterItem_Type) {
        return SPEC_FAIL_FOR_ITER_DICT_ITEMS;
    }
    if (t == &PySetIter_Type) {
        return SPEC_FAIL_FOR_ITER_SET;
    }
    if (t == &PyUnicodeIter_Type) {
        return SPEC_FAIL_FOR_ITER_STRING;
    }
    if (t == &PyBytesIter_Type) {
        return SPEC_FAIL_FOR_ITER_BYTES;
    }
    if (t == &PyRangeIter_Type) {
        return SPEC_FAIL_FOR_ITER_RANGE;
    }
    if (t == &PyEnum_Type) {
        return SPEC_FAIL_FOR_ITER_ENUMERATE;
    }

    if (strncmp(t->tp_name, "itertools", 8) == 0) {
        return SPEC_FAIL_FOR_ITER_ITERTOOLS;
    }
    return SPEC_FAIL_OTHER;
}

#endif
