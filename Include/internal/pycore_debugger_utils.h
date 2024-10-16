#ifndef Py_INTERNAL_DEBUGGER_UTILS_H
#define Py_INTERNAL_DEBUGGER_UTILS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


#if defined(__APPLE__)
#  include <mach-o/loader.h>
#endif


// Macros to burn global values in custom sections so out-of-process
// profilers can locate them easily

#define GENERATE_DEBUG_SECTION(name, declaration) \
    _GENERATE_DEBUG_SECTION_WINDOWS(name)         \
    _GENERATE_DEBUG_SECTION_APPLE(name)           \
    declaration                                   \
    _GENERATE_DEBUG_SECTION_LINUX(name)

#if defined(MS_WINDOWS)
#define _GENERATE_DEBUG_SECTION_WINDOWS(name)                       \
    _Pragma(Py_STRINGIFY(section(Py_STRINGIFY(name), read, write))) \
    __declspec(allocate(Py_STRINGIFY(name)))
#else
#define _GENERATE_DEBUG_SECTION_WINDOWS(name)
#endif

#if defined(__APPLE__)
#define _GENERATE_DEBUG_SECTION_APPLE(name) \
    __attribute__((section(SEG_DATA "," Py_STRINGIFY(name))))
#else
#define _GENERATE_DEBUG_SECTION_APPLE(name)
#endif

#if defined(__linux__) && (defined(__GNUC__) || defined(__clang__))
#define _GENERATE_DEBUG_SECTION_LINUX(name) \
    __attribute__((section("." Py_STRINGIFY(name))))
#else
#define _GENERATE_DEBUG_SECTION_LINUX(name)
#endif


#define _Py_Debug_Cookie "xdebugpy"

#ifdef Py_GIL_DISABLED
# define _Py_Debug_gilruntimestate_enabled offsetof(struct _gil_runtime_state, enabled)
# define _Py_Debug_Free_Threaded 1
#else
# define _Py_Debug_gilruntimestate_enabled 0
# define _Py_Debug_Free_Threaded 0
#endif


typedef struct _Py_DebugOffsets {
    char cookie[8];
    uint64_t version;
    uint64_t free_threaded;
    // Runtime state offset;
    struct _runtime_state {
        uint64_t size;
        uint64_t finalizing;
        uint64_t interpreters_head;
    } runtime_state;

    // Interpreter state offset;
    struct _interpreter_state {
        uint64_t size;
        uint64_t id;
        uint64_t next;
        uint64_t threads_head;
        uint64_t gc;
        uint64_t imports_modules;
        uint64_t sysdict;
        uint64_t builtins;
        uint64_t ceval_gil;
        uint64_t gil_runtime_state;
        uint64_t gil_runtime_state_enabled;
        uint64_t gil_runtime_state_locked;
        uint64_t gil_runtime_state_holder;
    } interpreter_state;

    // Thread state offset;
    struct _thread_state{
        uint64_t size;
        uint64_t prev;
        uint64_t next;
        uint64_t interp;
        uint64_t current_frame;
        uint64_t thread_id;
        uint64_t native_thread_id;
        uint64_t datastack_chunk;
        uint64_t status;
    } thread_state;

    // InterpreterFrame offset;
    struct _interpreter_frame {
        uint64_t size;
        uint64_t previous;
        uint64_t executable;
        uint64_t instr_ptr;
        uint64_t localsplus;
        uint64_t owner;
    } interpreter_frame;

    // Code object offset;
    struct _code_object {
        uint64_t size;
        uint64_t filename;
        uint64_t name;
        uint64_t qualname;
        uint64_t linetable;
        uint64_t firstlineno;
        uint64_t argcount;
        uint64_t localsplusnames;
        uint64_t localspluskinds;
        uint64_t co_code_adaptive;
    } code_object;

    // PyObject offset;
    struct _pyobject {
        uint64_t size;
        uint64_t ob_type;
    } pyobject;

    // PyTypeObject object offset;
    struct _type_object {
        uint64_t size;
        uint64_t tp_name;
        uint64_t tp_repr;
        uint64_t tp_flags;
    } type_object;

    // PyTuple object offset;
    struct _tuple_object {
        uint64_t size;
        uint64_t ob_item;
        uint64_t ob_size;
    } tuple_object;

    // PyList object offset;
    struct _list_object {
        uint64_t size;
        uint64_t ob_item;
        uint64_t ob_size;
    } list_object;

    // PyDict object offset;
    struct _dict_object {
        uint64_t size;
        uint64_t ma_keys;
        uint64_t ma_values;
    } dict_object;

    // PyFloat object offset;
    struct _float_object {
        uint64_t size;
        uint64_t ob_fval;
    } float_object;

    // PyLong object offset;
    struct _long_object {
        uint64_t size;
        uint64_t lv_tag;
        uint64_t ob_digit;
    } long_object;

    // PyBytes object offset;
    struct _bytes_object {
        uint64_t size;
        uint64_t ob_size;
        uint64_t ob_sval;
    } bytes_object;

    // Unicode object offset;
    struct _unicode_object {
        uint64_t size;
        uint64_t state;
        uint64_t length;
        uint64_t asciiobject_size;
    } unicode_object;

    // GC runtime state offset;
    struct _gc {
        uint64_t size;
        uint64_t collecting;
    } gc;
} _Py_DebugOffsets;


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_DEBUGGER_UTILS_H */
