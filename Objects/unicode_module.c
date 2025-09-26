#include "Python.h"
#include "pycore_codecs.h"        // _PyCodec_InitRegistry()
#include "pycore_initconfig.h"    // _PyStatus_ERR()
#include "pycore_interp.h"        // Py_RTFLAGS_USE_MAIN_OBMALLOC
#include "pycore_long.h"          // _PyLong_FormatWriter()
#include "pycore_object.h"        // _Py_SetMortal()
#include "pycore_pathconfig.h"    // _Py_DumpPathConfig()
#include "pycore_pylifecycle.h"   // _Py_SetFileSystemEncoding()
#include "pycore_pystate.h"       // _PyInterpreterState_Main()
#include "pycore_typeobject.h"    // _PyStaticType_FiniBuiltin()
#include "pycore_unicodeobject.h" // _Py_FieldNameIter_Type
#include "pycore_unicodeobject_generated.h"  // _PyUnicode_InitStaticStrings()


/* Uncomment to display statistics on interned strings at exit
   in _PyUnicode_ClearInterned(). */
/* #define INTERNED_STATS 1 */


#define _PyUnicode_STATE(op)                            \
    (_PyASCIIObject_CAST(op)->state)

#define INTERNED_STRINGS _PyRuntime.cached_objects.interned_strings

static inline PyObject *get_interned_dict(PyInterpreterState *interp)
{
    return _Py_INTERP_CACHED_OBJECT(interp, interned_strings);
}


static Py_uhash_t
hashtable_unicode_hash(const void *key)
{
    return _PyUnicode_Hash((PyObject *)key);
}

static int
hashtable_unicode_compare(const void *key1, const void *key2)
{
    PyObject *obj1 = (PyObject *)key1;
    PyObject *obj2 = (PyObject *)key2;
    if (obj1 != NULL && obj2 != NULL) {
        return _PyUnicode_Equal(obj1, obj2);
    }
    else {
        return obj1 == obj2;
    }
}


PyStatus
_PyUnicode_InitTypes(PyInterpreterState *interp)
{
    if (_PyStaticType_InitBuiltin(interp, &_Py_EncodingMapType) < 0) {
        goto error;
    }
    if (_PyStaticType_InitBuiltin(interp, &_Py_FieldNameIter_Type) < 0) {
        goto error;
    }
    if (_PyStaticType_InitBuiltin(interp, &_Py_FormatterIter_Type) < 0) {
        goto error;
    }
    return _PyStatus_OK();

error:
    return _PyStatus_ERR("Can't initialize unicode types");
}


/* Return true if this interpreter should share the main interpreter's
   intern_dict.  That's important for interpreters which load basic
   single-phase init extension modules (m_size == -1).  There could be interned
   immortal strings that are shared between interpreters, due to the
   PyDict_Update(mdict, m_copy) call in import_find_extension().

   It's not safe to deallocate those strings until all interpreters that
   potentially use them are freed.  By storing them in the main interpreter, we
   ensure they get freed after all other interpreters are freed.
*/
static bool
has_shared_intern_dict(PyInterpreterState *interp)
{
    PyInterpreterState *main_interp = _PyInterpreterState_Main();
    return interp != main_interp  && interp->feature_flags & Py_RTFLAGS_USE_MAIN_OBMALLOC;
}

static int
init_interned_dict(PyInterpreterState *interp)
{
    assert(get_interned_dict(interp) == NULL);
    PyObject *interned;
    if (has_shared_intern_dict(interp)) {
        interned = get_interned_dict(_PyInterpreterState_Main());
        Py_INCREF(interned);
    }
    else {
        interned = PyDict_New();
        if (interned == NULL) {
            return -1;
        }
    }
    _Py_INTERP_CACHED_OBJECT(interp, interned_strings) = interned;
    return 0;
}

static void
clear_interned_dict(PyInterpreterState *interp)
{
    PyObject *interned = get_interned_dict(interp);
    if (interned != NULL) {
        if (!has_shared_intern_dict(interp)) {
            // only clear if the dict belongs to this interpreter
            PyDict_Clear(interned);
        }
        Py_DECREF(interned);
        _Py_INTERP_CACHED_OBJECT(interp, interned_strings) = NULL;
    }
}

static PyStatus
init_global_interned_strings(PyInterpreterState *interp)
{
    assert(INTERNED_STRINGS == NULL);
    _Py_hashtable_allocator_t hashtable_alloc = {PyMem_RawMalloc, PyMem_RawFree};

    INTERNED_STRINGS = _Py_hashtable_new_full(
        hashtable_unicode_hash,
        hashtable_unicode_compare,
        // Objects stored here are immortal and statically allocated,
        // so we don't need key_destroy_func & value_destroy_func:
        NULL,
        NULL,
        &hashtable_alloc
    );
    if (INTERNED_STRINGS == NULL) {
        PyErr_Clear();
        return _PyStatus_ERR("failed to create global interned dict");
    }

    /* Intern statically allocated string identifiers, deepfreeze strings,
        * and one-byte latin-1 strings.
        * This must be done before any module initialization so that statically
        * allocated string identifiers are used instead of heap allocated strings.
        * Deepfreeze uses the interned identifiers if present to save space
        * else generates them and they are interned to speed up dict lookups.
    */
    _PyUnicode_InitStaticStrings(interp);

    for (int i = 0; i < 256; i++) {
        PyObject *s = _Py_LATIN1_CHR(i);
        _PyUnicode_InternStatic(interp, &s);
        assert(s == _Py_LATIN1_CHR(i));
    }
#ifdef Py_DEBUG
    assert(_PyUnicode_CheckConsistency(&_Py_STR(empty), 1));

    for (int i = 0; i < 256; i++) {
        assert(_PyUnicode_CheckConsistency(_Py_LATIN1_CHR(i), 1));
    }
#endif
    return _PyStatus_OK();
}


PyStatus
_PyUnicode_InitGlobalObjects(PyInterpreterState *interp)
{
    if (_Py_IsMainInterpreter(interp)) {
        PyStatus status = init_global_interned_strings(interp);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }
    assert(INTERNED_STRINGS);

    if (init_interned_dict(interp)) {
        PyErr_Clear();
        return _PyStatus_ERR("failed to create interned dict");
    }

    return _PyStatus_OK();
}


static void clear_global_interned_strings(void)
{
    if (INTERNED_STRINGS != NULL) {
        _Py_hashtable_destroy(INTERNED_STRINGS);
        INTERNED_STRINGS = NULL;
    }
}


void
_PyUnicode_ClearInterned(PyInterpreterState *interp)
{
    PyObject *interned = get_interned_dict(interp);
    if (interned == NULL) {
        return;
    }
    assert(PyDict_CheckExact(interned));

    if (has_shared_intern_dict(interp)) {
        // the dict doesn't belong to this interpreter, skip the debug
        // checks on it and just clear the pointer to it
        clear_interned_dict(interp);
        return;
    }

#ifdef INTERNED_STATS
    fprintf(stderr, "releasing %zd interned strings\n",
            PyDict_GET_SIZE(interned));

    Py_ssize_t total_length = 0;
#endif
    Py_ssize_t pos = 0;
    PyObject *s, *ignored_value;
    while (PyDict_Next(interned, &pos, &s, &ignored_value)) {
        int shared = 0;
        switch (PyUnicode_CHECK_INTERNED(s)) {
        case SSTATE_INTERNED_IMMORTAL:
            /* Make immortal interned strings mortal again. */
            // Skip the Immortal Instance check and restore
            // the two references (key and value) ignored
            // by PyUnicode_InternInPlace().
            _Py_SetMortal(s, 2);
#ifdef Py_REF_DEBUG
            /* let's be pedantic with the ref total */
            _Py_IncRefTotal(_PyThreadState_GET());
            _Py_IncRefTotal(_PyThreadState_GET());
#endif
#ifdef INTERNED_STATS
            total_length += PyUnicode_GET_LENGTH(s);
#endif
            break;
        case SSTATE_INTERNED_IMMORTAL_STATIC:
            /* It is shared between interpreters, so we should unmark it
               only when this is the last interpreter in which it's
               interned.  We immortalize all the statically initialized
               strings during startup, so we can rely on the
               main interpreter to be the last one. */
            if (!_Py_IsMainInterpreter(interp)) {
                shared = 1;
            }
            break;
        case SSTATE_INTERNED_MORTAL:
            // Restore 2 references held by the interned dict; these will
            // be decref'd by clear_interned_dict's PyDict_Clear.
            _Py_RefcntAdd(s, 2);
#ifdef Py_REF_DEBUG
            /* let's be pedantic with the ref total */
            _Py_IncRefTotal(_PyThreadState_GET());
            _Py_IncRefTotal(_PyThreadState_GET());
#endif
            break;
        case SSTATE_NOT_INTERNED:
            _Py_FALLTHROUGH;
        default:
            Py_UNREACHABLE();
        }
        if (!shared) {
            FT_ATOMIC_STORE_UINT8_RELAXED(_PyUnicode_STATE(s).interned, SSTATE_NOT_INTERNED);
        }
    }
#ifdef INTERNED_STATS
    fprintf(stderr,
            "total length of all interned strings: %zd characters\n",
            total_length);
#endif

    struct _Py_unicode_state *state = &interp->unicode;
    struct _Py_unicode_ids *ids = &state->ids;
    for (Py_ssize_t i=0; i < ids->size; i++) {
        Py_XINCREF(ids->array[i]);
    }
    clear_interned_dict(interp);
    if (_Py_IsMainInterpreter(interp)) {
        clear_global_interned_strings();
    }
}


static void
unicode_clear_identifiers(struct _Py_unicode_state *state)
{
    struct _Py_unicode_ids *ids = &state->ids;
    for (Py_ssize_t i=0; i < ids->size; i++) {
        Py_XDECREF(ids->array[i]);
    }
    ids->size = 0;
    PyMem_Free(ids->array);
    ids->array = NULL;
    // Don't reset _PyRuntime next_index: _Py_Identifier.id remains valid
    // after Py_Finalize().
}


void
_PyUnicode_FiniTypes(PyInterpreterState *interp)
{
    _PyStaticType_FiniBuiltin(interp, &_Py_EncodingMapType);
    _PyStaticType_FiniBuiltin(interp, &_Py_FieldNameIter_Type);
    _PyStaticType_FiniBuiltin(interp, &_Py_FormatterIter_Type);
}


static int
encode_wstr_utf8(wchar_t *wstr, char **str, const char *name)
{
    int res;
    res = _Py_EncodeUTF8Ex(wstr, str, NULL, NULL, 1, _Py_ERROR_STRICT);
    if (res == -2) {
        PyErr_Format(PyExc_RuntimeError, "cannot encode %s", name);
        return -1;
    }
    if (res < 0) {
        PyErr_NoMemory();
        return -1;
    }
    return 0;
}


static int
config_get_codec_name(wchar_t **config_encoding)
{
    char *encoding;
    if (encode_wstr_utf8(*config_encoding, &encoding, "stdio_encoding") < 0) {
        return -1;
    }

    PyObject *name_obj = NULL;
    PyObject *codec = _PyCodec_Lookup(encoding);
    PyMem_RawFree(encoding);

    if (!codec)
        goto error;

    name_obj = PyObject_GetAttrString(codec, "name");
    Py_CLEAR(codec);
    if (!name_obj) {
        goto error;
    }

    wchar_t *wname = PyUnicode_AsWideCharString(name_obj, NULL);
    Py_DECREF(name_obj);
    if (wname == NULL) {
        goto error;
    }

    wchar_t *raw_wname = _PyMem_RawWcsdup(wname);
    if (raw_wname == NULL) {
        PyMem_Free(wname);
        PyErr_NoMemory();
        goto error;
    }

    PyMem_RawFree(*config_encoding);
    *config_encoding = raw_wname;

    PyMem_Free(wname);
    return 0;

error:
    Py_XDECREF(codec);
    Py_XDECREF(name_obj);
    return -1;
}


static PyStatus
init_stdio_encoding(PyInterpreterState *interp)
{
    /* Update the stdio encoding to the normalized Python codec name. */
    PyConfig *config = (PyConfig*)_PyInterpreterState_GetConfig(interp);
    if (config_get_codec_name(&config->stdio_encoding) < 0) {
        return _PyStatus_ERR("failed to get the Python codec name "
                             "of the stdio encoding");
    }
    return _PyStatus_OK();
}


static int
init_fs_codec(PyInterpreterState *interp)
{
    const PyConfig *config = _PyInterpreterState_GetConfig(interp);

    _Py_error_handler error_handler;
    error_handler = _Py_GetErrorHandlerWide(config->filesystem_errors);
    if (error_handler == _Py_ERROR_UNKNOWN) {
        PyErr_SetString(PyExc_RuntimeError, "unknown filesystem error handler");
        return -1;
    }

    char *encoding, *errors;
    if (encode_wstr_utf8(config->filesystem_encoding,
                         &encoding,
                         "filesystem_encoding") < 0) {
        return -1;
    }

    if (encode_wstr_utf8(config->filesystem_errors,
                         &errors,
                         "filesystem_errors") < 0) {
        PyMem_RawFree(encoding);
        return -1;
    }

    struct _Py_unicode_fs_codec *fs_codec = &interp->unicode.fs_codec;
    PyMem_RawFree(fs_codec->encoding);
    fs_codec->encoding = encoding;
    /* encoding has been normalized by init_fs_encoding() */
    fs_codec->utf8 = (strcmp(encoding, "utf-8") == 0);
    PyMem_RawFree(fs_codec->errors);
    fs_codec->errors = errors;
    fs_codec->error_handler = error_handler;

#ifdef _Py_FORCE_UTF8_FS_ENCODING
    assert(fs_codec->utf8 == 1);
#endif

    /* At this point, PyUnicode_EncodeFSDefault() and
       PyUnicode_DecodeFSDefault() can now use the Python codec rather than
       the C implementation of the filesystem encoding. */

    /* Set Py_FileSystemDefaultEncoding and Py_FileSystemDefaultEncodeErrors
       global configuration variables. */
    if (_Py_IsMainInterpreter(interp)) {

        if (_Py_SetFileSystemEncoding(fs_codec->encoding,
                                      fs_codec->errors) < 0) {
            PyErr_NoMemory();
            return -1;
        }
    }
    return 0;
}


static PyStatus
init_fs_encoding(PyThreadState *tstate)
{
    PyInterpreterState *interp = tstate->interp;

    /* Update the filesystem encoding to the normalized Python codec name.
       For example, replace "ANSI_X3.4-1968" (locale encoding) with "ascii"
       (Python codec name). */
    PyConfig *config = (PyConfig*)_PyInterpreterState_GetConfig(interp);
    if (config_get_codec_name(&config->filesystem_encoding) < 0) {
        _Py_DumpPathConfig(tstate);
        return _PyStatus_ERR("failed to get the Python codec "
                             "of the filesystem encoding");
    }

    if (init_fs_codec(interp) < 0) {
        return _PyStatus_ERR("cannot initialize filesystem codec");
    }
    return _PyStatus_OK();
}


#ifdef MS_WINDOWS
int
_PyUnicode_EnableLegacyWindowsFSEncoding(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyConfig *config = (PyConfig *)_PyInterpreterState_GetConfig(interp);

    /* Set the filesystem encoding to mbcs/replace (PEP 529) */
    wchar_t *encoding = _PyMem_RawWcsdup(L"mbcs");
    wchar_t *errors = _PyMem_RawWcsdup(L"replace");
    if (encoding == NULL || errors == NULL) {
        PyMem_RawFree(encoding);
        PyMem_RawFree(errors);
        PyErr_NoMemory();
        return -1;
    }

    PyMem_RawFree(config->filesystem_encoding);
    config->filesystem_encoding = encoding;
    PyMem_RawFree(config->filesystem_errors);
    config->filesystem_errors = errors;

    return init_fs_codec(interp);
}
#endif


PyStatus
_PyUnicode_InitEncodings(PyThreadState *tstate)
{
    PyStatus status = _PyCodec_InitRegistry(tstate->interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }
    status = init_fs_encoding(tstate);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    return init_stdio_encoding(tstate->interp);
}


static void
_PyUnicode_FiniEncodings(struct _Py_unicode_fs_codec *fs_codec)
{
    PyMem_RawFree(fs_codec->encoding);
    fs_codec->encoding = NULL;
    fs_codec->utf8 = 0;
    PyMem_RawFree(fs_codec->errors);
    fs_codec->errors = NULL;
    fs_codec->error_handler = _Py_ERROR_UNKNOWN;
}


void
_PyUnicode_Fini(PyInterpreterState *interp)
{
    struct _Py_unicode_state *state = &interp->unicode;

    if (!has_shared_intern_dict(interp)) {
        // _PyUnicode_ClearInterned() must be called before _PyUnicode_Fini()
        assert(get_interned_dict(interp) == NULL);
    }

    _PyUnicode_FiniEncodings(&state->fs_codec);

    // bpo-47182: force a unicodedata CAPI capsule re-import on
    // subsequent initialization of interpreter.
    interp->unicode.ucnhash_capi = NULL;

    unicode_clear_identifiers(state);
}


// Implement:
// - formatter_field_name_split()
// - formatter_parser()
// - _PyUnicode_do_string_format()
// - _PyUnicode_do_string_format_map()
#include "stringlib/unicode_format.h"

/* A _string module, to export formatter_parser and formatter_field_name_split
   to the string.Formatter class implemented in Python. */

static PyMethodDef _string_methods[] = {
    {"formatter_field_name_split", formatter_field_name_split,
     METH_O, PyDoc_STR("split the argument as a field name")},
    {"formatter_parser", formatter_parser,
     METH_O, PyDoc_STR("parse the argument as a format string")},
    {NULL, NULL}
};

static PyModuleDef_Slot module_slots[] = {
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef _string_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_string",
    .m_doc = PyDoc_STR("string helper module"),
    .m_size = 0,
    .m_methods = _string_methods,
    .m_slots = module_slots,
};

PyMODINIT_FUNC
PyInit__string(void)
{
    return PyModuleDef_Init(&_string_module);
}
