
#include "parse_c_type.c"
#include "realize_c_type.c"

#define CFFI_VERSION_MIN            0x2601
#define CFFI_VERSION_CHAR16CHAR32   0x2801
#define CFFI_VERSION_MAX            0x28FF

typedef struct FFIObject_s FFIObject;
typedef struct LibObject_s LibObject;

static PyTypeObject FFI_Type;   /* forward */
static PyTypeObject Lib_Type;   /* forward */

#include "ffi_obj.c"
#include "cglob.c"
#include "lib_obj.c"
#include "cdlopen.c"
#include "commontypes.c"
#include "call_python.c"


static int init_ffi_lib(PyObject *m)
{
    PyObject *x;
    int i, res;
    static char init_done = 0;

    if (!init_done) {
        if (init_global_types_dict(FFI_Type.tp_dict) < 0)
            return -1;

        FFIError = PyErr_NewException("ffi.error", NULL, NULL);
        if (FFIError == NULL)
            return -1;
        if (PyDict_SetItemString(FFI_Type.tp_dict, "error", FFIError) < 0)
            return -1;
        if (PyDict_SetItemString(FFI_Type.tp_dict, "CType",
                                 (PyObject *)&CTypeDescr_Type) < 0)
            return -1;
        if (PyDict_SetItemString(FFI_Type.tp_dict, "CData",
                                 (PyObject *)&CData_Type) < 0)
            return -1;
        if (PyDict_SetItemString(FFI_Type.tp_dict, "buffer",
                                 (PyObject *)&MiniBuffer_Type) < 0)
            return -1;

        for (i = 0; all_dlopen_flags[i].name != NULL; i++) {
            x = PyInt_FromLong(all_dlopen_flags[i].value);
            if (x == NULL)
                return -1;
            res = PyDict_SetItemString(FFI_Type.tp_dict,
                                       all_dlopen_flags[i].name, x);
            Py_DECREF(x);
            if (res < 0)
                return -1;
        }
        init_done = 1;
    }
    return 0;
}

static int make_included_tuples(char *module_name,
                                const char *const *ctx_includes,
                                PyObject **included_ffis,
                                PyObject **included_libs)
{
    Py_ssize_t num = 0;
    const char *const *p_include;

    if (ctx_includes == NULL)
        return 0;

    for (p_include = ctx_includes; *p_include; p_include++) {
        num++;
    }
    *included_ffis = PyTuple_New(num);
    *included_libs = PyTuple_New(num);
    if (*included_ffis == NULL || *included_libs == NULL)
        goto error;

    num = 0;
    for (p_include = ctx_includes; *p_include; p_include++) {
        PyObject *included_ffi, *included_lib;
        PyObject *m = PyImport_ImportModule(*p_include);
        if (m == NULL)
            goto import_error;

        included_ffi = PyObject_GetAttrString(m, "ffi");
        PyTuple_SET_ITEM(*included_ffis, num, included_ffi);

        included_lib = (included_ffi == NULL) ? NULL :
                       PyObject_GetAttrString(m, "lib");
        PyTuple_SET_ITEM(*included_libs, num, included_lib);

        Py_DECREF(m);
        if (included_lib == NULL)
            goto import_error;

        if (!FFIObject_Check(included_ffi) ||
            !LibObject_Check(included_lib))
            goto import_error;
        num++;
    }
    return 0;

 import_error:
    PyErr_Format(PyExc_ImportError,
                 "while loading %.200s: failed to import ffi, lib from %.200s",
                 module_name, *p_include);
 error:
    Py_XDECREF(*included_ffis); *included_ffis = NULL;
    Py_XDECREF(*included_libs); *included_libs = NULL;
    return -1;
}

static PyObject *_my_Py_InitModule(char *module_name)
{
#if PY_MAJOR_VERSION >= 3
    struct PyModuleDef *module_def, local_module_def = {
        PyModuleDef_HEAD_INIT,
        module_name,
        NULL,
        -1,
        NULL, NULL, NULL, NULL, NULL
    };
    /* note: the 'module_def' is allocated dynamically and leaks,
       but anyway the C extension module can never be unloaded */
    module_def = PyMem_Malloc(sizeof(struct PyModuleDef));
    if (module_def == NULL)
        return PyErr_NoMemory();
    *module_def = local_module_def;
    return PyModule_Create(module_def);
#else
    return Py_InitModule(module_name, NULL);
#endif
}

static PyObject *b_init_cffi_1_0_external_module(PyObject *self, PyObject *arg)
{
    PyObject *m, *modules_dict;
    FFIObject *ffi;
    LibObject *lib;
    Py_ssize_t version, num_exports;
    char *module_name, *exports, *module_name_with_lib;
    void **raw;
    const struct _cffi_type_context_s *ctx;

    raw = (void **)PyLong_AsVoidPtr(arg);
    if (raw == NULL)
        return NULL;

    module_name = (char *)raw[0];
    version = (Py_ssize_t)raw[1];
    exports = (char *)raw[2];
    ctx = (const struct _cffi_type_context_s *)raw[3];

    if (version < CFFI_VERSION_MIN || version > CFFI_VERSION_MAX) {
        if (!PyErr_Occurred())
            PyErr_Format(PyExc_ImportError,
                "cffi extension module '%s' uses an unknown version tag %p. "
                "This module might need a more recent version of cffi "
                "than the one currently installed, which is %s",
                module_name, (void *)version, CFFI_VERSION);
        return NULL;
    }

    /* initialize the exports array */
    num_exports = 25;
    if (ctx->flags & 1)    /* set to mean that 'extern "Python"' is used */
        num_exports = 26;
    if (version >= CFFI_VERSION_CHAR16CHAR32)
        num_exports = 28;
    memcpy(exports, (char *)cffi_exports, num_exports * sizeof(void *));

    /* make the module object */
    m = _my_Py_InitModule(module_name);
    if (m == NULL)
        return NULL;

    /* build the FFI and Lib object inside this new module */
    ffi = ffi_internal_new(&FFI_Type, ctx);
    Py_XINCREF(ffi);    /* make the ffi object really immortal */
    if (ffi == NULL || PyModule_AddObject(m, "ffi", (PyObject *)ffi) < 0)
        return NULL;

    lib = lib_internal_new(ffi, module_name, NULL, 0);
    if (lib == NULL || PyModule_AddObject(m, "lib", (PyObject *)lib) < 0)
        return NULL;

    if (make_included_tuples(module_name, ctx->includes,
                             &ffi->types_builder.included_ffis,
                             &lib->l_types_builder->included_libs) < 0)
        return NULL;

    /* add manually 'module_name.lib' in sys.modules:
       see test_import_from_lib */
    modules_dict = PySys_GetObject("modules");
    if (!modules_dict)
        return NULL;
    module_name_with_lib = alloca(strlen(module_name) + 5);
    strcpy(module_name_with_lib, module_name);
    strcat(module_name_with_lib, ".lib");
    if (PyDict_SetItemString(modules_dict, module_name_with_lib,
                             (PyObject *)lib) < 0)
        return NULL;

#if PY_MAJOR_VERSION >= 3
    /* add manually 'module_name' in sys.modules: it seems that 
       Py_InitModule() is not enough to do that */
    if (PyDict_SetItemString(modules_dict, module_name, m) < 0)
        return NULL;
#endif

    return m;
}
