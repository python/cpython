#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_import.h"        // _PyImport_SetModule()
#include "pycore_pyhash.h"        // _Py_HashSecret
#include "pycore_traceback.h"     // _PyTraceback_Add()

#include <stdbool.h>
#include <stddef.h>               // offsetof()
#include "expat.h"
#include "pyexpat.h"

/* Do not emit Clinic output to a file as that wreaks havoc with conditionally
   included methods. */
/*[clinic input]
module pyexpat
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=b168d503a4490c15]*/

#define XML_COMBINED_VERSION (10000*XML_MAJOR_VERSION+100*XML_MINOR_VERSION+XML_MICRO_VERSION)

static XML_Memory_Handling_Suite ExpatMemoryHandler = {
    PyMem_Malloc, PyMem_Realloc, PyMem_Free};

enum HandlerTypes {
    StartElement,
    EndElement,
    ProcessingInstruction,
    CharacterData,
    UnparsedEntityDecl,
    NotationDecl,
    StartNamespaceDecl,
    EndNamespaceDecl,
    Comment,
    StartCdataSection,
    EndCdataSection,
    Default,
    DefaultHandlerExpand,
    NotStandalone,
    ExternalEntityRef,
    StartDoctypeDecl,
    EndDoctypeDecl,
    EntityDecl,
    XmlDecl,
    ElementDecl,
    AttlistDecl,
#if XML_COMBINED_VERSION >= 19504
    SkippedEntity,
#endif
    _DummyDecl
};

typedef struct {
    PyTypeObject *xml_parse_type;
    PyObject *error;
    PyObject *str_read;
} pyexpat_state;

static inline pyexpat_state*
pyexpat_get_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (pyexpat_state *)state;
}

/* ----------------------------------------------------- */

/* Declarations for objects of type xmlparser */

typedef struct {
    PyObject_HEAD

    XML_Parser itself;
    int ordered_attributes;     /* Return attributes as a list. */
    int specified_attributes;   /* Report only specified attributes. */
    int in_callback;            /* Is a callback active? */
    int ns_prefixes;            /* Namespace-triplets mode? */
    XML_Char *buffer;           /* Buffer used when accumulating characters */
                                /* NULL if not enabled */
    int buffer_size;            /* Size of buffer, in XML_Char units */
    int buffer_used;            /* Buffer units in use */
    bool reparse_deferral_enabled; /* Whether to defer reparsing of
                                   unfinished XML tokens; a de-facto cache of
                                   what Expat has the authority on, for lack
                                   of a getter API function
                                   "XML_GetReparseDeferralEnabled" in Expat
                                   2.6.0 */
    PyObject *intern;           /* Dictionary to intern strings */
    PyObject **handlers;
} xmlparseobject;

#define xmlparseobject_CAST(op) ((xmlparseobject *)(op))

#include "clinic/pyexpat.c.h"

#define CHARACTER_DATA_BUFFER_SIZE 8192

// A generic function type for storage.
// To avoid undefined behaviors, a handler must be cast to the correct
// function type before it's called; see SETTER_WRAPPER below.
typedef void (*xmlhandler)(void);

typedef void (*xmlhandlersetter)(XML_Parser self, xmlhandler handler);

struct HandlerInfo {
    const char *name;
    xmlhandlersetter setter;
    xmlhandler handler;
    PyGetSetDef getset;
};

static struct HandlerInfo handler_info[64];

static inline void
CALL_XML_HANDLER_SETTER(const struct HandlerInfo *handler_info,
                        XML_Parser xml_parser, xmlhandler xml_handler)
{
    xmlhandlersetter setter = (xmlhandlersetter)handler_info->setter;
    setter(xml_parser, xml_handler);
}

/* Set an integer attribute on the error object; return true on success,
 * false on an exception.
 */
static int
set_error_attr(PyObject *err, const char *name, int value)
{
    PyObject *v = PyLong_FromLong(value);

    if (v == NULL || PyObject_SetAttrString(err, name, v) == -1) {
        Py_XDECREF(v);
        return 0;
    }
    Py_DECREF(v);
    return 1;
}

/* Build and set an Expat exception, including positioning
 * information.  Always returns NULL.
 */
static PyObject *
set_error(pyexpat_state *state, xmlparseobject *self, enum XML_Error code)
{
    PyObject *err;
    PyObject *buffer;
    XML_Parser parser = self->itself;
    int lineno = XML_GetErrorLineNumber(parser);
    int column = XML_GetErrorColumnNumber(parser);

    buffer = PyUnicode_FromFormat("%s: line %i, column %i",
                                  XML_ErrorString(code), lineno, column);
    if (buffer == NULL)
        return NULL;
    err = PyObject_CallOneArg(state->error, buffer);
    Py_DECREF(buffer);
    if (  err != NULL
          && set_error_attr(err, "code", code)
          && set_error_attr(err, "offset", column)
          && set_error_attr(err, "lineno", lineno)) {
        PyErr_SetObject(state->error, err);
    }
    Py_XDECREF(err);
    return NULL;
}

static int
have_handler(xmlparseobject *self, int type)
{
    PyObject *handler = self->handlers[type];
    return handler != NULL;
}

/* Convert a string of XML_Chars into a Unicode string.
   Returns None if str is a null pointer. */

static PyObject *
conv_string_to_unicode(const XML_Char *str)
{
    /* XXX currently this code assumes that XML_Char is 8-bit,
       and hence in UTF-8.  */
    /* UTF-8 from Expat, Unicode desired */
    if (str == NULL) {
        Py_RETURN_NONE;
    }
    return PyUnicode_DecodeUTF8(str, strlen(str), "strict");
}

static PyObject *
conv_string_to_unicode_void(void *arg)
{
    return conv_string_to_unicode((const XML_Char *)arg);
}

static PyObject *
conv_string_len_to_unicode(const XML_Char *str, int len)
{
    /* XXX currently this code assumes that XML_Char is 8-bit,
       and hence in UTF-8.  */
    /* UTF-8 from Expat, Unicode desired */
    if (str == NULL) {
        Py_RETURN_NONE;
    }
    return PyUnicode_DecodeUTF8((const char *)str, len, "strict");
}

/* Callback routines */

static void clear_handlers(xmlparseobject *self, int initial);

/* This handler is used when an error has been detected, in the hope
   that actual parsing can be terminated early.  This will only help
   if an external entity reference is encountered. */
static int
error_external_entity_ref_handler(XML_Parser parser,
                                  const XML_Char *context,
                                  const XML_Char *base,
                                  const XML_Char *systemId,
                                  const XML_Char *publicId)
{
    return 0;
}

/* Dummy character data handler used when an error (exception) has
   been detected, and the actual parsing can be terminated early.
   This is needed since character data handler can't be safely removed
   from within the character data handler, but can be replaced.  It is
   used only from the character data handler trampoline, and must be
   used right after `flag_error()` is called. */
static void
noop_character_data_handler(void *userData, const XML_Char *data, int len)
{
    /* Do nothing. */
}

static void
flag_error(xmlparseobject *self)
{
    clear_handlers(self, 0);
    XML_SetExternalEntityRefHandler(self->itself,
                                    error_external_entity_ref_handler);
}

static PyObject*
call_with_frame(const char *funcname, int lineno, PyObject* func, PyObject* args,
                xmlparseobject *self)
{
    PyObject *res;

    res = PyObject_Call(func, args, NULL);
    if (res == NULL) {
        _PyTraceback_Add(funcname, __FILE__, lineno);
        XML_StopParser(self->itself, XML_FALSE);
    }
    return res;
}

static PyObject*
string_intern(xmlparseobject *self, const char* str)
{
    PyObject *result = conv_string_to_unicode(str);
    PyObject *value;
    /* result can be NULL if the unicode conversion failed. */
    if (!result)
        return result;
    if (!self->intern)
        return result;
    if (PyDict_GetItemRef(self->intern, result, &value) == 0 &&
        PyDict_SetItem(self->intern, result, result) == 0)
    {
        return result;
    }
    assert((value != NULL) == !PyErr_Occurred());
    Py_DECREF(result);
    return value;
}

/* Return 0 on success, -1 on exception.
 * flag_error() will be called before return if needed.
 */
static int
call_character_handler(xmlparseobject *self, const XML_Char *buffer, int len)
{
    PyObject *args;
    PyObject *temp;

    if (!have_handler(self, CharacterData))
        return -1;

    args = PyTuple_New(1);
    if (args == NULL)
        return -1;
    temp = (conv_string_len_to_unicode(buffer, len));
    if (temp == NULL) {
        Py_DECREF(args);
        flag_error(self);
        XML_SetCharacterDataHandler(self->itself,
                                    noop_character_data_handler);
        return -1;
    }
    PyTuple_SET_ITEM(args, 0, temp);
    /* temp is now a borrowed reference; consider it unused. */
    self->in_callback = 1;
    temp = call_with_frame("CharacterData", __LINE__,
                           self->handlers[CharacterData], args, self);
    /* temp is an owned reference again, or NULL */
    self->in_callback = 0;
    Py_DECREF(args);
    if (temp == NULL) {
        flag_error(self);
        XML_SetCharacterDataHandler(self->itself,
                                    noop_character_data_handler);
        return -1;
    }
    Py_DECREF(temp);
    return 0;
}

static int
flush_character_buffer(xmlparseobject *self)
{
    int rc;
    if (self->buffer == NULL || self->buffer_used == 0)
        return 0;
    rc = call_character_handler(self, self->buffer, self->buffer_used);
    self->buffer_used = 0;
    return rc;
}

static void
my_CharacterDataHandler(void *userData, const XML_Char *data, int len)
{
    xmlparseobject *self = xmlparseobject_CAST(userData);

    if (PyErr_Occurred())
        return;

    if (self->buffer == NULL)
        call_character_handler(self, data, len);
    else {
        if ((self->buffer_used + len) > self->buffer_size) {
            if (flush_character_buffer(self) < 0)
                return;
            /* handler might have changed; drop the rest on the floor
             * if there isn't a handler anymore
             */
            if (!have_handler(self, CharacterData))
                return;
        }
        if (len > self->buffer_size) {
            call_character_handler(self, data, len);
            self->buffer_used = 0;
        }
        else {
            memcpy(self->buffer + self->buffer_used,
                   data, len * sizeof(XML_Char));
            self->buffer_used += len;
        }
    }
}

static void
my_StartElementHandler(void *userData,
                       const XML_Char *name, const XML_Char *atts[])
{
    xmlparseobject *self = xmlparseobject_CAST(userData);

    if (have_handler(self, StartElement)) {
        PyObject *container, *rv, *args;
        int i, max;

        if (PyErr_Occurred())
            return;

        if (flush_character_buffer(self) < 0)
            return;
        /* Set max to the number of slots filled in atts[]; max/2 is
         * the number of attributes we need to process.
         */
        if (self->specified_attributes) {
            max = XML_GetSpecifiedAttributeCount(self->itself);
        }
        else {
            max = 0;
            while (atts[max] != NULL)
                max += 2;
        }
        /* Build the container. */
        if (self->ordered_attributes)
            container = PyList_New(max);
        else
            container = PyDict_New();
        if (container == NULL) {
            flag_error(self);
            return;
        }
        for (i = 0; i < max; i += 2) {
            PyObject *n = string_intern(self, (XML_Char *) atts[i]);
            PyObject *v;
            if (n == NULL) {
                flag_error(self);
                Py_DECREF(container);
                return;
            }
            v = conv_string_to_unicode((XML_Char *) atts[i+1]);
            if (v == NULL) {
                flag_error(self);
                Py_DECREF(container);
                Py_DECREF(n);
                return;
            }
            if (self->ordered_attributes) {
                PyList_SET_ITEM(container, i, n);
                PyList_SET_ITEM(container, i+1, v);
            }
            else if (PyDict_SetItem(container, n, v)) {
                flag_error(self);
                Py_DECREF(n);
                Py_DECREF(v);
                Py_DECREF(container);
                return;
            }
            else {
                Py_DECREF(n);
                Py_DECREF(v);
            }
        }
        args = string_intern(self, name);
        if (args == NULL) {
            Py_DECREF(container);
            return;
        }
        args = Py_BuildValue("(NN)", args, container);
        if (args == NULL) {
            return;
        }
        /* Container is now a borrowed reference; ignore it. */
        self->in_callback = 1;
        rv = call_with_frame("StartElement", __LINE__,
                             self->handlers[StartElement], args, self);
        self->in_callback = 0;
        Py_DECREF(args);
        if (rv == NULL) {
            flag_error(self);
            return;
        }
        Py_DECREF(rv);
    }
}

#define RC_HANDLER(RETURN_TYPE, NAME, PARAMS,       \
                   INIT, PARSE_FORMAT, CONVERSION,  \
                   RETURN_VARIABLE, GETUSERDATA)    \
static RETURN_TYPE                                  \
my_ ## NAME ## Handler PARAMS {                     \
    xmlparseobject *self = GETUSERDATA;             \
    INIT                                            \
    if (!have_handler(self, NAME)) {                \
        return RETURN_VARIABLE;                     \
    }                                               \
    if (PyErr_Occurred()) {                         \
        return RETURN_VARIABLE;                     \
    }                                               \
    if (flush_character_buffer(self) < 0) {         \
        return RETURN_VARIABLE;                     \
    }                                               \
    PyObject *args = Py_BuildValue PARSE_FORMAT;    \
    if (args == NULL) {                             \
        flag_error(self);                           \
        return RETURN_VARIABLE;                     \
    }                                               \
    self->in_callback = 1;                          \
    PyObject *rv = call_with_frame(                 \
        #NAME, __LINE__,                            \
        self->handlers[NAME], args, self);          \
    self->in_callback = 0;                          \
    Py_DECREF(args);                                \
    if (rv == NULL) {                               \
        flag_error(self);                           \
        return RETURN_VARIABLE;                     \
    }                                               \
    CONVERSION                                      \
    Py_DECREF(rv);                                  \
    return RETURN_VARIABLE;                         \
}

#define VOID_HANDLER(NAME, PARAMS, PARAM_FORMAT)    \
    RC_HANDLER(                                     \
        void, NAME, PARAMS,                         \
        /* no init */, PARAM_FORMAT,                \
        /* no conversion */, /* no return var */,   \
        xmlparseobject_CAST(userData)               \
    )

#define INT_HANDLER(NAME, PARAMS, PARAM_FORMAT)     \
    RC_HANDLER(                                     \
        int, NAME, PARAMS,                          \
        int rc = 0;, PARAM_FORMAT,                  \
        rc = PyLong_AsLong(rv);, rc,                \
        xmlparseobject_CAST(userData)               \
    )

VOID_HANDLER(EndElement,
             (void *userData, const XML_Char *name),
             ("(N)", string_intern(self, name)))

VOID_HANDLER(ProcessingInstruction,
             (void *userData,
              const XML_Char *target,
              const XML_Char *data),
             ("(NO&)", string_intern(self, target), conv_string_to_unicode_void, data))

VOID_HANDLER(UnparsedEntityDecl,
             (void *userData,
              const XML_Char *entityName,
              const XML_Char *base,
              const XML_Char *systemId,
              const XML_Char *publicId,
              const XML_Char *notationName),
             ("(NNNNN)",
              string_intern(self, entityName), string_intern(self, base),
              string_intern(self, systemId), string_intern(self, publicId),
              string_intern(self, notationName)))

VOID_HANDLER(EntityDecl,
             (void *userData,
              const XML_Char *entityName,
              int is_parameter_entity,
              const XML_Char *value,
              int value_length,
              const XML_Char *base,
              const XML_Char *systemId,
              const XML_Char *publicId,
              const XML_Char *notationName),
             ("NiNNNNN",
              string_intern(self, entityName), is_parameter_entity,
              (conv_string_len_to_unicode(value, value_length)),
              string_intern(self, base), string_intern(self, systemId),
              string_intern(self, publicId),
              string_intern(self, notationName)))

VOID_HANDLER(XmlDecl,
             (void *userData,
              const XML_Char *version,
              const XML_Char *encoding,
              int standalone),
             ("(O&O&i)",
              conv_string_to_unicode_void, version,
              conv_string_to_unicode_void, encoding,
              standalone))

static PyObject *
conv_content_model(XML_Content * const model,
                   PyObject *(*conv_string)(void *))
{
    PyObject *result = NULL;
    PyObject *children = PyTuple_New(model->numchildren);
    int i;

    if (children != NULL) {
        assert(model->numchildren < INT_MAX);
        for (i = 0; i < (int)model->numchildren; ++i) {
            PyObject *child = conv_content_model(&model->children[i],
                                                 conv_string);
            if (child == NULL) {
                Py_XDECREF(children);
                return NULL;
            }
            PyTuple_SET_ITEM(children, i, child);
        }
        result = Py_BuildValue("(iiO&N)",
                               model->type, model->quant,
                               conv_string, model->name, children);
    }
    return result;
}

static void
my_ElementDeclHandler(void *userData,
                      const XML_Char *name,
                      XML_Content *model)
{
    xmlparseobject *self = xmlparseobject_CAST(userData);
    PyObject *args = NULL;

    if (have_handler(self, ElementDecl)) {
        PyObject *rv = NULL;
        PyObject *modelobj, *nameobj;

        if (PyErr_Occurred())
            return;

        if (flush_character_buffer(self) < 0)
            goto finally;
        modelobj = conv_content_model(model, conv_string_to_unicode_void);
        if (modelobj == NULL) {
            flag_error(self);
            goto finally;
        }
        nameobj = string_intern(self, name);
        if (nameobj == NULL) {
            Py_DECREF(modelobj);
            flag_error(self);
            goto finally;
        }
        args = Py_BuildValue("NN", nameobj, modelobj);
        if (args == NULL) {
            flag_error(self);
            goto finally;
        }
        self->in_callback = 1;
        rv = call_with_frame("ElementDecl", __LINE__,
                             self->handlers[ElementDecl], args, self);
        self->in_callback = 0;
        if (rv == NULL) {
            flag_error(self);
            goto finally;
        }
        Py_DECREF(rv);
    }
 finally:
    Py_XDECREF(args);
    XML_FreeContentModel(self->itself, model);
    return;
}

VOID_HANDLER(AttlistDecl,
             (void *userData,
              const XML_Char *elname,
              const XML_Char *attname,
              const XML_Char *att_type,
              const XML_Char *dflt,
              int isrequired),
             ("(NNO&O&i)",
              string_intern(self, elname), string_intern(self, attname),
              conv_string_to_unicode_void, att_type,
              conv_string_to_unicode_void, dflt,
              isrequired))

#if XML_COMBINED_VERSION >= 19504
VOID_HANDLER(SkippedEntity,
             (void *userData,
              const XML_Char *entityName,
              int is_parameter_entity),
             ("Ni",
              string_intern(self, entityName), is_parameter_entity))
#endif

VOID_HANDLER(NotationDecl,
                (void *userData,
                        const XML_Char *notationName,
                        const XML_Char *base,
                        const XML_Char *systemId,
                        const XML_Char *publicId),
                ("(NNNN)",
                 string_intern(self, notationName), string_intern(self, base),
                 string_intern(self, systemId), string_intern(self, publicId)))

VOID_HANDLER(StartNamespaceDecl,
                (void *userData,
                      const XML_Char *prefix,
                      const XML_Char *uri),
                ("(NN)",
                 string_intern(self, prefix), string_intern(self, uri)))

VOID_HANDLER(EndNamespaceDecl,
                (void *userData,
                    const XML_Char *prefix),
                ("(N)", string_intern(self, prefix)))

VOID_HANDLER(Comment,
               (void *userData, const XML_Char *data),
                ("(O&)", conv_string_to_unicode_void, data))

VOID_HANDLER(StartCdataSection,
               (void *userData),
                ("()"))

VOID_HANDLER(EndCdataSection,
               (void *userData),
                ("()"))

VOID_HANDLER(Default,
              (void *userData, const XML_Char *s, int len),
              ("(N)", (conv_string_len_to_unicode(s,len))))

VOID_HANDLER(DefaultHandlerExpand,
              (void *userData, const XML_Char *s, int len),
              ("(N)", (conv_string_len_to_unicode(s,len))))
#define my_DefaultHandlerExpand my_DefaultHandlerExpandHandler

INT_HANDLER(NotStandalone,
                (void *userData),
                ("()"))

RC_HANDLER(int, ExternalEntityRef,
                (XML_Parser parser,
                    const XML_Char *context,
                    const XML_Char *base,
                    const XML_Char *systemId,
                    const XML_Char *publicId),
                int rc=0;,
                ("(O&NNN)",
                 conv_string_to_unicode_void, context,
                 string_intern(self, base),
                 string_intern(self, systemId), string_intern(self, publicId)),
                rc = PyLong_AsLong(rv);, rc,
                XML_GetUserData(parser))

/* XXX UnknownEncodingHandler */

VOID_HANDLER(StartDoctypeDecl,
             (void *userData, const XML_Char *doctypeName,
              const XML_Char *sysid, const XML_Char *pubid,
              int has_internal_subset),
             ("(NNNi)", string_intern(self, doctypeName),
              string_intern(self, sysid), string_intern(self, pubid),
              has_internal_subset))

VOID_HANDLER(EndDoctypeDecl, (void *userData), ("()"))

/* ---------------------------------------------------------------- */
/*[clinic input]
class pyexpat.xmlparser "xmlparseobject *" "&Xmlparsetype"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=2393162385232e1c]*/


static PyObject *
get_parse_result(pyexpat_state *state, xmlparseobject *self, int rv)
{
    if (PyErr_Occurred()) {
        return NULL;
    }
    if (rv == 0) {
        return set_error(state, self, XML_GetErrorCode(self->itself));
    }
    if (flush_character_buffer(self) < 0) {
        return NULL;
    }
    return PyLong_FromLong(rv);
}

#define MAX_CHUNK_SIZE (1 << 20)

/*[clinic input]
pyexpat.xmlparser.SetReparseDeferralEnabled

    enabled: bool
    /

Enable/Disable reparse deferral; enabled by default with Expat >=2.6.0.
[clinic start generated code]*/

static PyObject *
pyexpat_xmlparser_SetReparseDeferralEnabled_impl(xmlparseobject *self,
                                                 int enabled)
/*[clinic end generated code: output=5ec539e3b63c8c49 input=021eb9e0bafc32c5]*/
{
#if XML_COMBINED_VERSION >= 20600
    XML_SetReparseDeferralEnabled(self->itself, enabled ? XML_TRUE : XML_FALSE);
    self->reparse_deferral_enabled = (bool)enabled;
#endif
    Py_RETURN_NONE;
}

/*[clinic input]
pyexpat.xmlparser.GetReparseDeferralEnabled

Retrieve reparse deferral enabled status; always returns false with Expat <2.6.0.
[clinic start generated code]*/

static PyObject *
pyexpat_xmlparser_GetReparseDeferralEnabled_impl(xmlparseobject *self)
/*[clinic end generated code: output=4e91312e88a595a8 input=54b5f11d32b20f3e]*/
{
    return PyBool_FromLong(self->reparse_deferral_enabled);
}

/*[clinic input]
pyexpat.xmlparser.Parse

    cls: defining_class
    data: object
    isfinal: bool = False
    /

Parse XML data.

'isfinal' should be true at end of input.
[clinic start generated code]*/

static PyObject *
pyexpat_xmlparser_Parse_impl(xmlparseobject *self, PyTypeObject *cls,
                             PyObject *data, int isfinal)
/*[clinic end generated code: output=8faffe07fe1f862a input=053e0f047e55c05a]*/
{
    const char *s;
    Py_ssize_t slen;
    Py_buffer view;
    int rc;
    pyexpat_state *state = PyType_GetModuleState(cls);

    if (PyUnicode_Check(data)) {
        view.buf = NULL;
        s = PyUnicode_AsUTF8AndSize(data, &slen);
        if (s == NULL)
            return NULL;
        /* Explicitly set UTF-8 encoding. Return code ignored. */
        (void)XML_SetEncoding(self->itself, "utf-8");
    }
    else {
        if (PyObject_GetBuffer(data, &view, PyBUF_SIMPLE) < 0)
            return NULL;
        s = view.buf;
        slen = view.len;
    }

    static_assert(MAX_CHUNK_SIZE <= INT_MAX,
                  "MAX_CHUNK_SIZE is larger than INT_MAX");
    while (slen > MAX_CHUNK_SIZE) {
        rc = XML_Parse(self->itself, s, MAX_CHUNK_SIZE, 0);
        if (!rc)
            goto done;
        s += MAX_CHUNK_SIZE;
        slen -= MAX_CHUNK_SIZE;
    }

    assert(slen <= INT_MAX);
    rc = XML_Parse(self->itself, s, (int)slen, isfinal);

done:
    if (view.buf != NULL) {
        PyBuffer_Release(&view);
    }
    return get_parse_result(state, self, rc);
}

/* File reading copied from cPickle */

#define BUF_SIZE 2048

static int
readinst(char *buf, int buf_size, PyObject *meth)
{
    PyObject *str;
    Py_ssize_t len;
    const char *ptr;

    str = PyObject_CallFunction(meth, "i", buf_size);
    if (str == NULL)
        goto error;

    if (PyBytes_Check(str))
        ptr = PyBytes_AS_STRING(str);
    else if (PyByteArray_Check(str))
        ptr = PyByteArray_AS_STRING(str);
    else {
        PyErr_Format(PyExc_TypeError,
                     "read() did not return a bytes object (type=%.400s)",
                     Py_TYPE(str)->tp_name);
        goto error;
    }
    len = Py_SIZE(str);
    if (len > buf_size) {
        PyErr_Format(PyExc_ValueError,
                     "read() returned too much data: "
                     "%i bytes requested, %zd returned",
                     buf_size, len);
        goto error;
    }
    memcpy(buf, ptr, len);
    Py_DECREF(str);
    /* len <= buf_size <= INT_MAX */
    return (int)len;

error:
    Py_XDECREF(str);
    return -1;
}

/*[clinic input]
pyexpat.xmlparser.ParseFile

    cls: defining_class
    file: object
    /

Parse XML data from file-like object.
[clinic start generated code]*/

static PyObject *
pyexpat_xmlparser_ParseFile_impl(xmlparseobject *self, PyTypeObject *cls,
                                 PyObject *file)
/*[clinic end generated code: output=34780a094c8ca3ae input=ba4bc9c541684793]*/
{
    int rv = 1;
    PyObject *readmethod = NULL;

    pyexpat_state *state = PyType_GetModuleState(cls);

    if (PyObject_GetOptionalAttr(file, state->str_read, &readmethod) < 0) {
        return NULL;
    }
    if (readmethod == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "argument must have 'read' attribute");
        return NULL;
    }
    for (;;) {
        int bytes_read;
        void *buf = XML_GetBuffer(self->itself, BUF_SIZE);
        if (buf == NULL) {
            Py_XDECREF(readmethod);
            return get_parse_result(state, self, 0);
        }

        bytes_read = readinst(buf, BUF_SIZE, readmethod);
        if (bytes_read < 0) {
            Py_DECREF(readmethod);
            return NULL;
        }
        rv = XML_ParseBuffer(self->itself, bytes_read, bytes_read == 0);
        if (PyErr_Occurred()) {
            Py_XDECREF(readmethod);
            return NULL;
        }

        if (!rv || bytes_read == 0)
            break;
    }
    Py_XDECREF(readmethod);
    return get_parse_result(state, self, rv);
}

/*[clinic input]
pyexpat.xmlparser.SetBase

    base: str
    /

Set the base URL for the parser.
[clinic start generated code]*/

static PyObject *
pyexpat_xmlparser_SetBase_impl(xmlparseobject *self, const char *base)
/*[clinic end generated code: output=c212ddceb607b539 input=c684e5de895ee1a8]*/
{
    if (!XML_SetBase(self->itself, base)) {
        return PyErr_NoMemory();
    }
    Py_RETURN_NONE;
}

/*[clinic input]
pyexpat.xmlparser.GetBase

Return base URL string for the parser.
[clinic start generated code]*/

static PyObject *
pyexpat_xmlparser_GetBase_impl(xmlparseobject *self)
/*[clinic end generated code: output=2886cb21f9a8739a input=918d71c38009620e]*/
{
    return conv_string_to_unicode(XML_GetBase(self->itself));
}

/*[clinic input]
pyexpat.xmlparser.GetInputContext

Return the untranslated text of the input that caused the current event.

If the event was generated by a large amount of text (such as a start tag
for an element with many attributes), not all of the text may be available.
[clinic start generated code]*/

static PyObject *
pyexpat_xmlparser_GetInputContext_impl(xmlparseobject *self)
/*[clinic end generated code: output=a88026d683fc22cc input=034df8712db68379]*/
{
    if (self->in_callback) {
        int offset, size;
        const char *buffer
            = XML_GetInputContext(self->itself, &offset, &size);

        if (buffer != NULL)
            return PyBytes_FromStringAndSize(buffer + offset,
                                              size - offset);
        else
            Py_RETURN_NONE;
    }
    else
        Py_RETURN_NONE;
}

/*[clinic input]
pyexpat.xmlparser.ExternalEntityParserCreate

    cls: defining_class
    context: str(accept={str, NoneType})
    encoding: str = NULL
    /

Create a parser for parsing an external entity based on the information passed to the ExternalEntityRefHandler.
[clinic start generated code]*/

static PyObject *
pyexpat_xmlparser_ExternalEntityParserCreate_impl(xmlparseobject *self,
                                                  PyTypeObject *cls,
                                                  const char *context,
                                                  const char *encoding)
/*[clinic end generated code: output=01d4472b49cb3f92 input=ec70c6b9e6e9619a]*/
{
    xmlparseobject *new_parser;
    pyexpat_state *state = PyType_GetModuleState(cls);

    new_parser = PyObject_GC_New(xmlparseobject, state->xml_parse_type);
    if (new_parser == NULL) {
        return NULL;
    }

    new_parser->buffer_size = self->buffer_size;
    new_parser->buffer_used = 0;
    new_parser->buffer = NULL;
    new_parser->ordered_attributes = self->ordered_attributes;
    new_parser->specified_attributes = self->specified_attributes;
    new_parser->in_callback = 0;
    new_parser->ns_prefixes = self->ns_prefixes;
    new_parser->itself = XML_ExternalEntityParserCreate(self->itself, context,
                                                        encoding);
    new_parser->handlers = 0;
    new_parser->intern = Py_XNewRef(self->intern);

    if (self->buffer != NULL) {
        new_parser->buffer = PyMem_Malloc(new_parser->buffer_size);
        if (new_parser->buffer == NULL) {
            Py_DECREF(new_parser);
            return PyErr_NoMemory();
        }
    }
    if (!new_parser->itself) {
        Py_DECREF(new_parser);
        return PyErr_NoMemory();
    }

    XML_SetUserData(new_parser->itself, (void *)new_parser);

    /* allocate and clear handlers first */
    size_t i;
    for (i = 0; handler_info[i].name != NULL; i++)
        /* do nothing */;

    new_parser->handlers = PyMem_New(PyObject *, i);
    if (!new_parser->handlers) {
        Py_DECREF(new_parser);
        return PyErr_NoMemory();
    }
    clear_handlers(new_parser, 1);

    /* then copy handlers from self */
    for (size_t i = 0; handler_info[i].name != NULL; i++) {
        PyObject *handler = self->handlers[i];
        if (handler != NULL) {
            new_parser->handlers[i] = Py_NewRef(handler);
            struct HandlerInfo info = handler_info[i];
            CALL_XML_HANDLER_SETTER(&info, new_parser->itself, info.handler);
        }
    }

    PyObject_GC_Track(new_parser);
    return (PyObject *)new_parser;
}

/*[clinic input]
pyexpat.xmlparser.SetParamEntityParsing

    flag: int
    /

Controls parsing of parameter entities (including the external DTD subset).

Possible flag values are XML_PARAM_ENTITY_PARSING_NEVER,
XML_PARAM_ENTITY_PARSING_UNLESS_STANDALONE and
XML_PARAM_ENTITY_PARSING_ALWAYS. Returns true if setting the flag
was successful.
[clinic start generated code]*/

static PyObject *
pyexpat_xmlparser_SetParamEntityParsing_impl(xmlparseobject *self, int flag)
/*[clinic end generated code: output=18668ee8e760d64c input=8aea19b4b15e9af1]*/
{
    flag = XML_SetParamEntityParsing(self->itself, flag);
    return PyLong_FromLong(flag);
}


#if XML_COMBINED_VERSION >= 19505
/*[clinic input]
pyexpat.xmlparser.UseForeignDTD

    cls: defining_class
    flag: bool = True
    /

Allows the application to provide an artificial external subset if one is not specified as part of the document instance.

This readily allows the use of a 'default' document type controlled by the
application, while still getting the advantage of providing document type
information to the parser. 'flag' defaults to True if not provided.
[clinic start generated code]*/

static PyObject *
pyexpat_xmlparser_UseForeignDTD_impl(xmlparseobject *self, PyTypeObject *cls,
                                     int flag)
/*[clinic end generated code: output=d7d98252bd25a20f input=23440ecb0573fb29]*/
{
    pyexpat_state *state = PyType_GetModuleState(cls);
    enum XML_Error rc;

    rc = XML_UseForeignDTD(self->itself, flag ? XML_TRUE : XML_FALSE);
    if (rc != XML_ERROR_NONE) {
        return set_error(state, self, rc);
    }
    Py_RETURN_NONE;
}
#endif

static struct PyMethodDef xmlparse_methods[] = {
    PYEXPAT_XMLPARSER_PARSE_METHODDEF
    PYEXPAT_XMLPARSER_PARSEFILE_METHODDEF
    PYEXPAT_XMLPARSER_SETBASE_METHODDEF
    PYEXPAT_XMLPARSER_GETBASE_METHODDEF
    PYEXPAT_XMLPARSER_GETINPUTCONTEXT_METHODDEF
    PYEXPAT_XMLPARSER_EXTERNALENTITYPARSERCREATE_METHODDEF
    PYEXPAT_XMLPARSER_SETPARAMENTITYPARSING_METHODDEF
#if XML_COMBINED_VERSION >= 19505
    PYEXPAT_XMLPARSER_USEFOREIGNDTD_METHODDEF
#endif
    PYEXPAT_XMLPARSER_SETREPARSEDEFERRALENABLED_METHODDEF
    PYEXPAT_XMLPARSER_GETREPARSEDEFERRALENABLED_METHODDEF
    {NULL, NULL}  /* sentinel */
};

/* ---------- */



/* pyexpat international encoding support.
   Make it as simple as possible.
*/

static const unsigned char template_buffer[256] =
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
     20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37,
     38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55,
     56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73,
     74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
     92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107,
     108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
     123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137,
     138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152,
     153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167,
     168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182,
     183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197,
     198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212,
     213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227,
     228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242,
     243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255};


static int
PyUnknownEncodingHandler(void *encodingHandlerData,
                         const XML_Char *name,
                         XML_Encoding *info)
{
    PyObject *u;
    int i;
    const void *data;
    int kind;

    if (PyErr_Occurred())
        return XML_STATUS_ERROR;

    u = PyUnicode_Decode((const char*) template_buffer, 256, name, "replace");
    if (u == NULL) {
        Py_XDECREF(u);
        return XML_STATUS_ERROR;
    }

    if (PyUnicode_GET_LENGTH(u) != 256) {
        Py_DECREF(u);
        PyErr_SetString(PyExc_ValueError,
                        "multi-byte encodings are not supported");
        return XML_STATUS_ERROR;
    }

    kind = PyUnicode_KIND(u);
    data = PyUnicode_DATA(u);
    for (i = 0; i < 256; i++) {
        Py_UCS4 ch = PyUnicode_READ(kind, data, i);
        if (ch != Py_UNICODE_REPLACEMENT_CHARACTER)
            info->map[i] = ch;
        else
            info->map[i] = -1;
    }

    info->data = NULL;
    info->convert = NULL;
    info->release = NULL;
    Py_DECREF(u);

    return XML_STATUS_OK;
}


static PyObject *
newxmlparseobject(pyexpat_state *state, const char *encoding,
                  const char *namespace_separator, PyObject *intern)
{
    int i;
    xmlparseobject *self;

    self = PyObject_GC_New(xmlparseobject, state->xml_parse_type);
    if (self == NULL)
        return NULL;

    self->buffer = NULL;
    self->buffer_size = CHARACTER_DATA_BUFFER_SIZE;
    self->buffer_used = 0;
    self->ordered_attributes = 0;
    self->specified_attributes = 0;
    self->in_callback = 0;
    self->ns_prefixes = 0;
    self->handlers = NULL;
    self->intern = Py_XNewRef(intern);
#if XML_COMBINED_VERSION >= 20600
    self->reparse_deferral_enabled = true;
#else
    self->reparse_deferral_enabled = false;
#endif

    /* namespace_separator is either NULL or contains one char + \0 */
    self->itself = XML_ParserCreate_MM(encoding, &ExpatMemoryHandler,
                                       namespace_separator);
    if (self->itself == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "XML_ParserCreate failed");
        Py_DECREF(self);
        return NULL;
    }
#if XML_COMBINED_VERSION >= 20100
    /* This feature was added upstream in libexpat 2.1.0. */
    XML_SetHashSalt(self->itself,
                    (unsigned long)_Py_HashSecret.expat.hashsalt);
#endif
    XML_SetUserData(self->itself, (void *)self);
    XML_SetUnknownEncodingHandler(self->itself,
                  (XML_UnknownEncodingHandler) PyUnknownEncodingHandler, NULL);

    for (i = 0; handler_info[i].name != NULL; i++)
        /* do nothing */;

    self->handlers = PyMem_New(PyObject *, i);
    if (!self->handlers) {
        Py_DECREF(self);
        return PyErr_NoMemory();
    }
    clear_handlers(self, 1);

    PyObject_GC_Track(self);
    return (PyObject*)self;
}

static int
xmlparse_traverse(PyObject *op, visitproc visit, void *arg)
{
    xmlparseobject *self = xmlparseobject_CAST(op);
    for (size_t i = 0; handler_info[i].name != NULL; i++) {
        Py_VISIT(self->handlers[i]);
    }
    Py_VISIT(Py_TYPE(op));
    return 0;
}

static int
xmlparse_clear(PyObject *op)
{
    xmlparseobject *self = xmlparseobject_CAST(op);
    clear_handlers(self, 0);
    Py_CLEAR(self->intern);
    return 0;
}

static void
xmlparse_dealloc(PyObject *op)
{
    xmlparseobject *self = xmlparseobject_CAST(op);
    PyObject_GC_UnTrack(self);
    (void)xmlparse_clear(op);
    if (self->itself != NULL) {
        XML_ParserFree(self->itself);
    }
    self->itself = NULL;

    if (self->handlers != NULL) {
        PyMem_Free(self->handlers);
        self->handlers = NULL;
    }
    if (self->buffer != NULL) {
        PyMem_Free(self->buffer);
        self->buffer = NULL;
    }
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_Del(self);
    Py_DECREF(tp);
}


static PyObject *
xmlparse_handler_getter(PyObject *op, void *closure)
{
    xmlparseobject *self = xmlparseobject_CAST(op);
    struct HandlerInfo *hi = (struct HandlerInfo *)closure;
    assert((hi - handler_info) < (Py_ssize_t)Py_ARRAY_LENGTH(handler_info));
    int handlernum = (int)(hi - handler_info);
    PyObject *result = self->handlers[handlernum];
    if (result == NULL) {
        result = Py_None;
    }
    return Py_NewRef(result);
}

static int
xmlparse_handler_setter(PyObject *op, PyObject *v, void *closure)
{
    xmlparseobject *self = xmlparseobject_CAST(op);
    struct HandlerInfo *hi = (struct HandlerInfo *)closure;
    assert((hi - handler_info) < (Py_ssize_t)Py_ARRAY_LENGTH(handler_info));
    int handlernum = (int)(hi - handler_info);
    if (v == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Cannot delete attribute");
        return -1;
    }
    if (handlernum == CharacterData) {
        /* If we're changing the character data handler, flush all
         * cached data with the old handler.  Not sure there's a
         * "right" thing to do, though, but this probably won't
         * happen.
         */
        if (flush_character_buffer(self) < 0)
            return -1;
    }

    xmlhandler c_handler = NULL;
    if (v == Py_None) {
        /* If this is the character data handler, and a character
           data handler is already active, we need to be more
           careful.  What we can safely do is replace the existing
           character data handler callback function with a no-op
           function that will refuse to call Python.  The downside
           is that this doesn't completely remove the character
           data handler from the C layer if there's any callback
           active, so Expat does a little more work than it
           otherwise would, but that's really an odd case.  A more
           elaborate system of handlers and state could remove the
           C handler more effectively. */
        if (handlernum == CharacterData && self->in_callback) {
            c_handler = (xmlhandler)noop_character_data_handler;
        }
        v = NULL;
    }
    else if (v != NULL) {
        Py_INCREF(v);
        c_handler = handler_info[handlernum].handler;
    }
    Py_XSETREF(self->handlers[handlernum], v);
    CALL_XML_HANDLER_SETTER(&handler_info[handlernum], self->itself, c_handler);
    return 0;
}

#define INT_GETTER(name)                                                \
    static PyObject *                                                   \
    xmlparse_##name##_getter(PyObject *op, void *Py_UNUSED(closure))    \
    {                                                                   \
        xmlparseobject *self = xmlparseobject_CAST(op);                 \
        return PyLong_FromLong((long)XML_Get##name(self->itself));      \
    }
INT_GETTER(ErrorCode)
INT_GETTER(ErrorLineNumber)
INT_GETTER(ErrorColumnNumber)
INT_GETTER(ErrorByteIndex)
INT_GETTER(CurrentLineNumber)
INT_GETTER(CurrentColumnNumber)
INT_GETTER(CurrentByteIndex)

#undef INT_GETTER

static PyObject *
xmlparse_buffer_text_getter(PyObject *op, void *Py_UNUSED(closure))
{
    xmlparseobject *self = xmlparseobject_CAST(op);
    return PyBool_FromLong(self->buffer != NULL);
}

static int
xmlparse_buffer_text_setter(PyObject *op, PyObject *v, void *Py_UNUSED(closure))
{
    xmlparseobject *self = xmlparseobject_CAST(op);
    if (v == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Cannot delete attribute");
        return -1;
    }
    int b = PyObject_IsTrue(v);
    if (b < 0) {
        return -1;
    }
    if (b) {
        if (self->buffer == NULL) {
            self->buffer = PyMem_Malloc(self->buffer_size);
            if (self->buffer == NULL) {
                PyErr_NoMemory();
                return -1;
            }
            self->buffer_used = 0;
        }
    }
    else if (self->buffer != NULL) {
        if (flush_character_buffer(self) < 0) {
            return -1;
        }
        PyMem_Free(self->buffer);
        self->buffer = NULL;
    }
    return 0;
}

static PyObject *
xmlparse_buffer_size_getter(PyObject *op, void *Py_UNUSED(closure))
{
    xmlparseobject *self = xmlparseobject_CAST(op);
    return PyLong_FromLong(self->buffer_size);
}

static int
xmlparse_buffer_size_setter(PyObject *op, PyObject *v, void *Py_UNUSED(closure))
{
    xmlparseobject *self = xmlparseobject_CAST(op);
    if (v == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Cannot delete attribute");
        return -1;
    }
    long new_buffer_size;
    if (!PyLong_Check(v)) {
        PyErr_SetString(PyExc_TypeError, "buffer_size must be an integer");
        return -1;
    }

    new_buffer_size = PyLong_AsLong(v);
    if (new_buffer_size <= 0) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_ValueError, "buffer_size must be greater than zero");
        }
        return -1;
    }

    /* trivial case -- no change */
    if (new_buffer_size == self->buffer_size) {
        return 0;
    }

    /* check maximum */
    if (new_buffer_size > INT_MAX) {
        PyErr_Format(PyExc_ValueError, "buffer_size must not be greater than %i", INT_MAX);
        return -1;
    }

    if (self->buffer != NULL) {
        /* there is already a buffer */
        if (self->buffer_used != 0) {
            if (flush_character_buffer(self) < 0) {
                return -1;
            }
        }
        /* free existing buffer */
        PyMem_Free(self->buffer);
    }
    self->buffer = PyMem_Malloc(new_buffer_size);
    if (self->buffer == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    self->buffer_size = new_buffer_size;
    return 0;
}

static PyObject *
xmlparse_buffer_used_getter(PyObject *op, void *Py_UNUSED(closure))
{
    xmlparseobject *self = xmlparseobject_CAST(op);
    return PyLong_FromLong(self->buffer_used);
}

static PyObject *
xmlparse_namespace_prefixes_getter(PyObject *op, void *Py_UNUSED(closure))
{
    xmlparseobject *self = xmlparseobject_CAST(op);
    return PyBool_FromLong(self->ns_prefixes);
}

static int
xmlparse_namespace_prefixes_setter(PyObject *op, PyObject *v, void *Py_UNUSED(closure))
{
    xmlparseobject *self = xmlparseobject_CAST(op);
    if (v == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Cannot delete attribute");
        return -1;
    }
    int b = PyObject_IsTrue(v);
    if (b < 0) {
        return -1;
    }
    self->ns_prefixes = b;
    XML_SetReturnNSTriplet(self->itself, self->ns_prefixes);
    return 0;
}

static PyObject *
xmlparse_ordered_attributes_getter(PyObject *op, void *Py_UNUSED(closure))
{
    xmlparseobject *self = xmlparseobject_CAST(op);
    return PyBool_FromLong(self->ordered_attributes);
}

static int
xmlparse_ordered_attributes_setter(PyObject *op, PyObject *v, void *Py_UNUSED(closure))
{
    xmlparseobject *self = xmlparseobject_CAST(op);
    if (v == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Cannot delete attribute");
        return -1;
    }
    int b = PyObject_IsTrue(v);
    if (b < 0) {
        return -1;
    }
    self->ordered_attributes = b;
    return 0;
}

static PyObject *
xmlparse_specified_attributes_getter(PyObject *op, void *Py_UNUSED(closure))
{
    xmlparseobject *self = xmlparseobject_CAST(op);
    return PyBool_FromLong(self->specified_attributes);
}

static int
xmlparse_specified_attributes_setter(PyObject *op, PyObject *v, void *Py_UNUSED(closure))
{
    xmlparseobject *self = xmlparseobject_CAST(op);
    if (v == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Cannot delete attribute");
        return -1;
    }
    int b = PyObject_IsTrue(v);
    if (b < 0) {
        return -1;
    }
    self->specified_attributes = b;
    return 0;
}

static PyMemberDef xmlparse_members[] = {
    {"intern", _Py_T_OBJECT, offsetof(xmlparseobject, intern), Py_READONLY, NULL},
    {NULL}
};

#define XMLPARSE_GETTER_DEF(name) \
    {#name, xmlparse_##name##_getter, NULL, NULL},
#define XMLPARSE_GETTER_SETTER_DEF(name) \
    {#name, xmlparse_##name##_getter, \
            xmlparse_##name##_setter, NULL},

static PyGetSetDef xmlparse_getsetlist[] = {
    XMLPARSE_GETTER_DEF(ErrorCode)
    XMLPARSE_GETTER_DEF(ErrorLineNumber)
    XMLPARSE_GETTER_DEF(ErrorColumnNumber)
    XMLPARSE_GETTER_DEF(ErrorByteIndex)
    XMLPARSE_GETTER_DEF(CurrentLineNumber)
    XMLPARSE_GETTER_DEF(CurrentColumnNumber)
    XMLPARSE_GETTER_DEF(CurrentByteIndex)
    XMLPARSE_GETTER_SETTER_DEF(buffer_size)
    XMLPARSE_GETTER_SETTER_DEF(buffer_text)
    XMLPARSE_GETTER_DEF(buffer_used)
    XMLPARSE_GETTER_SETTER_DEF(namespace_prefixes)
    XMLPARSE_GETTER_SETTER_DEF(ordered_attributes)
    XMLPARSE_GETTER_SETTER_DEF(specified_attributes)
    {NULL},
};

#undef XMLPARSE_GETTER_DEF
#undef XMLPARSE_GETTER_SETTER_DEF

PyDoc_STRVAR(Xmlparsetype__doc__, "XML parser");

static PyType_Slot _xml_parse_type_spec_slots[] = {
    {Py_tp_dealloc, xmlparse_dealloc},
    {Py_tp_doc, (void *)Xmlparsetype__doc__},
    {Py_tp_traverse, xmlparse_traverse},
    {Py_tp_clear, xmlparse_clear},
    {Py_tp_methods, xmlparse_methods},
    {Py_tp_members, xmlparse_members},
    {Py_tp_getset, xmlparse_getsetlist},
    {0, 0}
};

static PyType_Spec _xml_parse_type_spec = {
    .name = "pyexpat.xmlparser",
    .basicsize = sizeof(xmlparseobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = _xml_parse_type_spec_slots,
};

/* End of code for xmlparser objects */
/* -------------------------------------------------------- */

/*[clinic input]
pyexpat.ParserCreate

    encoding: str(accept={str, NoneType}) = None
    namespace_separator: str(accept={str, NoneType}) = None
    intern: object = NULL

Return a new XML parser object.
[clinic start generated code]*/

static PyObject *
pyexpat_ParserCreate_impl(PyObject *module, const char *encoding,
                          const char *namespace_separator, PyObject *intern)
/*[clinic end generated code: output=295c0cf01ab1146c input=e8da8e8d7122cb5d]*/
{
    PyObject *result;
    int intern_decref = 0;

    if (namespace_separator != NULL
        && strlen(namespace_separator) > 1) {
        PyErr_SetString(PyExc_ValueError,
                        "namespace_separator must be at most one"
                        " character, omitted, or None");
        return NULL;
    }
    /* Explicitly passing None means no interning is desired.
       Not passing anything means that a new dictionary is used. */
    if (intern == Py_None)
        intern = NULL;
    else if (intern == NULL) {
        intern = PyDict_New();
        if (!intern)
            return NULL;
        intern_decref = 1;
    }
    else if (!PyDict_Check(intern)) {
        PyErr_SetString(PyExc_TypeError, "intern must be a dictionary");
        return NULL;
    }

    pyexpat_state *state = pyexpat_get_state(module);
    result = newxmlparseobject(state, encoding, namespace_separator, intern);
    if (intern_decref) {
        Py_DECREF(intern);
    }
    return result;
}

/*[clinic input]
pyexpat.ErrorString

    code: long
    /

Returns string error for given number.
[clinic start generated code]*/

static PyObject *
pyexpat_ErrorString_impl(PyObject *module, long code)
/*[clinic end generated code: output=2feae50d166f2174 input=cc67de010d9e62b3]*/
{
    return conv_string_to_unicode(XML_ErrorString((int)code));
}

/* List of methods defined in the module */

static struct PyMethodDef pyexpat_methods[] = {
    PYEXPAT_PARSERCREATE_METHODDEF
    PYEXPAT_ERRORSTRING_METHODDEF
    {NULL, NULL}  /* sentinel */
};

/* Module docstring */

PyDoc_STRVAR(pyexpat_module_documentation,
"Python wrapper for Expat parser.");

/* Initialization function for the module */

#ifndef MODULE_NAME
#define MODULE_NAME "pyexpat"
#endif

static int init_handler_descrs(pyexpat_state *state)
{
    int i;
    assert(state->xml_parse_type->tp_version_tag == 0);
    for (i = 0; handler_info[i].name != NULL; i++) {
        struct HandlerInfo *hi = &handler_info[i];
        hi->getset.name = hi->name;
        hi->getset.get = xmlparse_handler_getter;
        hi->getset.set = xmlparse_handler_setter;
        hi->getset.closure = &handler_info[i];

        PyObject *descr = PyDescr_NewGetSet(state->xml_parse_type, &hi->getset);
        if (descr == NULL)
            return -1;

        if (PyDict_SetDefaultRef(state->xml_parse_type->tp_dict,
                                 PyDescr_NAME(descr), descr, NULL) < 0) {
            Py_DECREF(descr);
            return -1;
        }
        Py_DECREF(descr);
    }
    return 0;
}

static PyObject *
add_submodule(PyObject *mod, const char *fullname)
{
    const char *name = strrchr(fullname, '.') + 1;

    PyObject *submodule = PyModule_New(fullname);
    if (submodule == NULL) {
        return NULL;
    }

    PyObject *mod_name = PyUnicode_FromString(fullname);
    if (mod_name == NULL) {
        Py_DECREF(submodule);
        return NULL;
    }

    if (_PyImport_SetModule(mod_name, submodule) < 0) {
        Py_DECREF(submodule);
        Py_DECREF(mod_name);
        return NULL;
    }
    Py_DECREF(mod_name);

    /* gives away the reference to the submodule */
    if (PyModule_Add(mod, name, submodule) < 0) {
        return NULL;
    }

    return submodule;
}

struct ErrorInfo {
    const char * name;  /* Error constant name, e.g. "XML_ERROR_NO_MEMORY" */
    const char * description;  /* Error description as returned by XML_ErrorString(<int>) */
};

static
struct ErrorInfo error_info_of[] = {
    {NULL, NULL},  /* XML_ERROR_NONE (value 0) is not exposed */

    {"XML_ERROR_NO_MEMORY", "out of memory"},
    {"XML_ERROR_SYNTAX", "syntax error"},
    {"XML_ERROR_NO_ELEMENTS", "no element found"},
    {"XML_ERROR_INVALID_TOKEN", "not well-formed (invalid token)"},
    {"XML_ERROR_UNCLOSED_TOKEN", "unclosed token"},
    {"XML_ERROR_PARTIAL_CHAR", "partial character"},
    {"XML_ERROR_TAG_MISMATCH", "mismatched tag"},
    {"XML_ERROR_DUPLICATE_ATTRIBUTE", "duplicate attribute"},
    {"XML_ERROR_JUNK_AFTER_DOC_ELEMENT", "junk after document element"},
    {"XML_ERROR_PARAM_ENTITY_REF", "illegal parameter entity reference"},
    {"XML_ERROR_UNDEFINED_ENTITY", "undefined entity"},
    {"XML_ERROR_RECURSIVE_ENTITY_REF", "recursive entity reference"},
    {"XML_ERROR_ASYNC_ENTITY", "asynchronous entity"},
    {"XML_ERROR_BAD_CHAR_REF", "reference to invalid character number"},
    {"XML_ERROR_BINARY_ENTITY_REF", "reference to binary entity"},
    {"XML_ERROR_ATTRIBUTE_EXTERNAL_ENTITY_REF", "reference to external entity in attribute"},
    {"XML_ERROR_MISPLACED_XML_PI", "XML or text declaration not at start of entity"},
    {"XML_ERROR_UNKNOWN_ENCODING", "unknown encoding"},
    {"XML_ERROR_INCORRECT_ENCODING", "encoding specified in XML declaration is incorrect"},
    {"XML_ERROR_UNCLOSED_CDATA_SECTION", "unclosed CDATA section"},
    {"XML_ERROR_EXTERNAL_ENTITY_HANDLING", "error in processing external entity reference"},
    {"XML_ERROR_NOT_STANDALONE", "document is not standalone"},
    {"XML_ERROR_UNEXPECTED_STATE", "unexpected parser state - please send a bug report"},
    {"XML_ERROR_ENTITY_DECLARED_IN_PE", "entity declared in parameter entity"},
    {"XML_ERROR_FEATURE_REQUIRES_XML_DTD", "requested feature requires XML_DTD support in Expat"},
    {"XML_ERROR_CANT_CHANGE_FEATURE_ONCE_PARSING", "cannot change setting once parsing has begun"},

    /* Added in Expat 1.95.7. */
    {"XML_ERROR_UNBOUND_PREFIX", "unbound prefix"},

    /* Added in Expat 1.95.8. */
    {"XML_ERROR_UNDECLARING_PREFIX", "must not undeclare prefix"},
    {"XML_ERROR_INCOMPLETE_PE", "incomplete markup in parameter entity"},
    {"XML_ERROR_XML_DECL", "XML declaration not well-formed"},
    {"XML_ERROR_TEXT_DECL", "text declaration not well-formed"},
    {"XML_ERROR_PUBLICID", "illegal character(s) in public id"},
    {"XML_ERROR_SUSPENDED", "parser suspended"},
    {"XML_ERROR_NOT_SUSPENDED", "parser not suspended"},
    {"XML_ERROR_ABORTED", "parsing aborted"},
    {"XML_ERROR_FINISHED", "parsing finished"},
    {"XML_ERROR_SUSPEND_PE", "cannot suspend in external parameter entity"},

    /* Added in 2.0.0. */
    {"XML_ERROR_RESERVED_PREFIX_XML", "reserved prefix (xml) must not be undeclared or bound to another namespace name"},
    {"XML_ERROR_RESERVED_PREFIX_XMLNS", "reserved prefix (xmlns) must not be declared or undeclared"},
    {"XML_ERROR_RESERVED_NAMESPACE_URI", "prefix must not be bound to one of the reserved namespace names"},

    /* Added in 2.2.1. */
    {"XML_ERROR_INVALID_ARGUMENT", "invalid argument"},

    /* Added in 2.3.0. */
    {"XML_ERROR_NO_BUFFER", "a successful prior call to function XML_GetBuffer is required"},

    /* Added in 2.4.0. */
    {"XML_ERROR_AMPLIFICATION_LIMIT_BREACH", "limit on input amplification factor (from DTD and entities) breached"},

    /* Added in 2.6.4. */
    {"XML_ERROR_NOT_STARTED", "parser not started"},
};

static int
add_error(PyObject *errors_module, PyObject *codes_dict,
          PyObject *rev_codes_dict, size_t error_index)
{
    const char * const name = error_info_of[error_index].name;
    const int error_code = (int)error_index;

    /* NOTE: This keeps the source of truth regarding error
     *       messages with libexpat and (by definition) in bulletproof sync
     *       with the other uses of the XML_ErrorString function
     *       elsewhere within this file.  pyexpat's copy of the messages
     *       only acts as a fallback in case of outdated runtime libexpat,
     *       where it returns NULL.
     *
     *       In addition, XML_ErrorString is assumed to return UTF-8 encoded
     *       strings (in conv_string_to_unicode, we decode them using 'strict'
     *       error handling).
     */
    const char *error_string = XML_ErrorString(error_code);
    if (error_string == NULL) {
        error_string = error_info_of[error_index].description;
    }

    if (PyModule_AddStringConstant(errors_module, name, error_string) < 0) {
        return -1;
    }

    PyObject *num = PyLong_FromLong(error_code);
    if (num == NULL) {
        return -1;
    }

    if (PyDict_SetItemString(codes_dict, error_string, num) < 0) {
        Py_DECREF(num);
        return -1;
    }

    PyObject *str = PyUnicode_FromString(error_string);
    if (str == NULL) {
        Py_DECREF(num);
        return -1;
    }

    int res = PyDict_SetItem(rev_codes_dict, num, str);
    Py_DECREF(str);
    Py_DECREF(num);
    if (res < 0) {
        return -1;
    }

    return 0;
}

static int
add_errors_module(PyObject *mod)
{
    // add_submodule() returns a borrowed ref.
    PyObject *errors_module = add_submodule(mod, MODULE_NAME ".errors");
    if (errors_module == NULL) {
        return -1;
    }

    PyObject *codes_dict = PyDict_New();
    if (codes_dict == NULL) {
        return -1;
    }
    PyObject *rev_codes_dict = PyDict_New();
    if (rev_codes_dict == NULL) {
        goto error;
    }

    size_t error_index = 0;
    for (; error_index < sizeof(error_info_of) / sizeof(struct ErrorInfo); error_index++) {
        if (error_info_of[error_index].name == NULL) {
            continue;
        }

        if (add_error(errors_module, codes_dict, rev_codes_dict, error_index) < 0) {
            goto error;
        }
    }

    if (PyModule_AddStringConstant(errors_module, "__doc__",
                                   "Constants used to describe "
                                   "error conditions.") < 0) {
        goto error;
    }

    if (PyModule_Add(errors_module, "codes", codes_dict) < 0) {
        Py_DECREF(rev_codes_dict);
        return -1;
    }

    if (PyModule_Add(errors_module, "messages", rev_codes_dict) < 0) {
        return -1;
    }

    return 0;

error:
    Py_XDECREF(codes_dict);
    Py_XDECREF(rev_codes_dict);
    return -1;
}

static int
add_model_module(PyObject *mod)
{
    PyObject *model_module = add_submodule(mod, MODULE_NAME ".model");
    if (model_module == NULL) {
        return -1;
    }

#define MYCONST(c)  do {                                        \
        if (PyModule_AddIntConstant(model_module, #c, c) < 0) { \
            return -1;                                          \
        }                                                       \
    } while(0)

    if (PyModule_AddStringConstant(
        model_module, "__doc__",
        "Constants used to interpret content model information.") < 0) {
        return -1;
    }

    MYCONST(XML_CTYPE_EMPTY);
    MYCONST(XML_CTYPE_ANY);
    MYCONST(XML_CTYPE_MIXED);
    MYCONST(XML_CTYPE_NAME);
    MYCONST(XML_CTYPE_CHOICE);
    MYCONST(XML_CTYPE_SEQ);

    MYCONST(XML_CQUANT_NONE);
    MYCONST(XML_CQUANT_OPT);
    MYCONST(XML_CQUANT_REP);
    MYCONST(XML_CQUANT_PLUS);
#undef MYCONST
    return 0;
}

#if XML_COMBINED_VERSION > 19505
static int
add_features(PyObject *mod)
{
    PyObject *list = PyList_New(0);
    if (list == NULL) {
        return -1;
    }

    const XML_Feature *features = XML_GetFeatureList();
    for (size_t i = 0; features[i].feature != XML_FEATURE_END; ++i) {
        PyObject *item = Py_BuildValue("si", features[i].name,
                                       features[i].value);
        if (item == NULL) {
            goto error;
        }
        int ok = PyList_Append(list, item);
        Py_DECREF(item);
        if (ok < 0) {
            goto error;
        }
    }
    return PyModule_Add(mod, "features", list);

error:
    Py_DECREF(list);
    return -1;
}
#endif

static void
pyexpat_capsule_destructor(PyObject *capsule)
{
    void *p = PyCapsule_GetPointer(capsule, PyExpat_CAPSULE_NAME);
    if (p == NULL) {
        PyErr_FormatUnraisable("Exception ignored while destroying "
                               "pyexact capsule");
        return;
    }
    PyMem_Free(p);
}


static int
pyexpat_exec(PyObject *mod)
{
    pyexpat_state *state = pyexpat_get_state(mod);
    state->str_read = PyUnicode_InternFromString("read");
    if (state->str_read == NULL) {
        return -1;
    }
    state->xml_parse_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        mod, &_xml_parse_type_spec, NULL);

    if (state->xml_parse_type == NULL) {
        return -1;
    }

    if (init_handler_descrs(state) < 0) {
        return -1;
    }
    state->error = PyErr_NewException("xml.parsers.expat.ExpatError",
                                      NULL, NULL);
    if (state->error == NULL) {
        return -1;
    }

    /* Add some symbolic constants to the module */

    if (PyModule_AddObjectRef(mod, "error", state->error) < 0) {
        return -1;
    }

    if (PyModule_AddObjectRef(mod, "ExpatError", state->error) < 0) {
        return -1;
    }

    if (PyModule_AddObjectRef(mod, "XMLParserType",
                           (PyObject *) state->xml_parse_type) < 0) {
        return -1;
    }

    if (PyModule_AddStringConstant(mod, "EXPAT_VERSION",
                                   XML_ExpatVersion()) < 0) {
        return -1;
    }
    {
        XML_Expat_Version info = XML_ExpatVersionInfo();
        PyObject *versionInfo = Py_BuildValue("(iii)",
                                              info.major,
                                              info.minor,
                                              info.micro);
        if (PyModule_Add(mod, "version_info", versionInfo) < 0) {
            return -1;
        }
    }
    /* XXX When Expat supports some way of figuring out how it was
       compiled, this should check and set native_encoding
       appropriately.
    */
    if (PyModule_AddStringConstant(mod, "native_encoding", "UTF-8") < 0) {
        return -1;
    }

    if (add_errors_module(mod) < 0) {
        return -1;
    }

    if (add_model_module(mod) < 0) {
        return -1;
    }

#if XML_COMBINED_VERSION > 19505
    if (add_features(mod) < 0) {
        return -1;
    }
#endif

#define MYCONST(c) do {                                 \
        if (PyModule_AddIntConstant(mod, #c, c) < 0) {  \
            return -1;                                  \
        }                                               \
    } while(0)

    MYCONST(XML_PARAM_ENTITY_PARSING_NEVER);
    MYCONST(XML_PARAM_ENTITY_PARSING_UNLESS_STANDALONE);
    MYCONST(XML_PARAM_ENTITY_PARSING_ALWAYS);
#undef MYCONST

    struct PyExpat_CAPI *capi = PyMem_Malloc(sizeof(*capi));
    if (capi == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    /* initialize pyexpat dispatch table */
    capi->size = sizeof(*capi);
    capi->magic = PyExpat_CAPI_MAGIC;
    capi->MAJOR_VERSION = XML_MAJOR_VERSION;
    capi->MINOR_VERSION = XML_MINOR_VERSION;
    capi->MICRO_VERSION = XML_MICRO_VERSION;
    capi->ErrorString = XML_ErrorString;
    capi->GetErrorCode = XML_GetErrorCode;
    capi->GetErrorColumnNumber = XML_GetErrorColumnNumber;
    capi->GetErrorLineNumber = XML_GetErrorLineNumber;
    capi->Parse = XML_Parse;
    capi->ParserCreate_MM = XML_ParserCreate_MM;
    capi->ParserFree = XML_ParserFree;
    capi->SetCharacterDataHandler = XML_SetCharacterDataHandler;
    capi->SetCommentHandler = XML_SetCommentHandler;
    capi->SetDefaultHandlerExpand = XML_SetDefaultHandlerExpand;
    capi->SetElementHandler = XML_SetElementHandler;
    capi->SetNamespaceDeclHandler = XML_SetNamespaceDeclHandler;
    capi->SetProcessingInstructionHandler = XML_SetProcessingInstructionHandler;
    capi->SetUnknownEncodingHandler = XML_SetUnknownEncodingHandler;
    capi->SetUserData = XML_SetUserData;
    capi->SetStartDoctypeDeclHandler = XML_SetStartDoctypeDeclHandler;
    capi->SetEncoding = XML_SetEncoding;
    capi->DefaultUnknownEncodingHandler = PyUnknownEncodingHandler;
#if XML_COMBINED_VERSION >= 20100
    capi->SetHashSalt = XML_SetHashSalt;
#else
    capi->SetHashSalt = NULL;
#endif
#if XML_COMBINED_VERSION >= 20600
    capi->SetReparseDeferralEnabled = XML_SetReparseDeferralEnabled;
#else
    capi->SetReparseDeferralEnabled = NULL;
#endif

    /* export using capsule */
    PyObject *capi_object = PyCapsule_New(capi, PyExpat_CAPSULE_NAME,
                                          pyexpat_capsule_destructor);
    if (capi_object == NULL) {
        PyMem_Free(capi);
        return -1;
    }

    if (PyModule_Add(mod, "expat_CAPI", capi_object) < 0) {
        return -1;
    }

    return 0;
}

static int
pyexpat_traverse(PyObject *module, visitproc visit, void *arg)
{
    pyexpat_state *state = pyexpat_get_state(module);
    Py_VISIT(state->xml_parse_type);
    Py_VISIT(state->error);
    Py_VISIT(state->str_read);
    return 0;
}

static int
pyexpat_clear(PyObject *module)
{
    pyexpat_state *state = pyexpat_get_state(module);
    Py_CLEAR(state->xml_parse_type);
    Py_CLEAR(state->error);
    Py_CLEAR(state->str_read);
    return 0;
}

static void
pyexpat_free(void *module)
{
    (void)pyexpat_clear((PyObject *)module);
}

static PyModuleDef_Slot pyexpat_slots[] = {
    {Py_mod_exec, pyexpat_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef pyexpatmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = MODULE_NAME,
    .m_doc = pyexpat_module_documentation,
    .m_size = sizeof(pyexpat_state),
    .m_methods = pyexpat_methods,
    .m_slots = pyexpat_slots,
    .m_traverse = pyexpat_traverse,
    .m_clear = pyexpat_clear,
    .m_free = pyexpat_free
};

PyMODINIT_FUNC
PyInit_pyexpat(void)
{
    return PyModuleDef_Init(&pyexpatmodule);
}

static void
clear_handlers(xmlparseobject *self, int initial)
{
    for (size_t i = 0; handler_info[i].name != NULL; i++) {
        if (initial) {
            self->handlers[i] = NULL;
        }
        else {
            Py_CLEAR(self->handlers[i]);
            CALL_XML_HANDLER_SETTER(&handler_info[i], self->itself, NULL);
        }
    }
}

/* To avoid undefined behaviors, a function must be *called* via a function
 * pointer of the correct type.
 * So, for each `XML_Set*` function, we define a wrapper that calls `XML_Set*`
 * with its argument cast to the appropriate type.
 */

typedef void (*parser_only)(void *);
typedef int (*not_standalone)(void *);
typedef void (*parser_and_data)(void *, const XML_Char *);
typedef void (*parser_and_data_and_int)(void *, const XML_Char *, int);
typedef void (*parser_and_data_and_data)(
    void *, const XML_Char *, const XML_Char *);
typedef void (*start_element)(void *, const XML_Char *, const XML_Char **);
typedef void (*element_decl)(void *, const XML_Char *, XML_Content *);
typedef void (*xml_decl)(
    void *, const XML_Char *, const XML_Char *, int);
typedef void (*start_doctype_decl)(
    void *, const XML_Char *, const XML_Char *, const XML_Char *, int);
typedef void (*notation_decl)(
    void *,
    const XML_Char *, const XML_Char *, const XML_Char *, const XML_Char *);
typedef void (*attlist_decl)(
    void *,
    const XML_Char *, const XML_Char *, const XML_Char *, const XML_Char *,
    int);
typedef void (*unparsed_entity_decl)(
    void *,
    const XML_Char *, const XML_Char *,
    const XML_Char *, const XML_Char *, const XML_Char *);
typedef void (*entity_decl)(
    void *,
    const XML_Char *, int,
    const XML_Char *, int,
    const XML_Char *, const XML_Char *, const XML_Char *, const XML_Char *);
typedef int (*external_entity_ref)(
    XML_Parser,
    const XML_Char *, const XML_Char *, const XML_Char *, const XML_Char *);

#define SETTER_WRAPPER(NAME, TYPE)                                      \
    static inline void                                                  \
    pyexpat_Set ## NAME (XML_Parser parser, xmlhandler handler)         \
    {                                                                   \
        (void)XML_Set ## NAME (parser, (TYPE)handler);                  \
    }

SETTER_WRAPPER(StartElementHandler, start_element)
SETTER_WRAPPER(EndElementHandler, parser_and_data)
SETTER_WRAPPER(ProcessingInstructionHandler, parser_and_data_and_data)
SETTER_WRAPPER(CharacterDataHandler, parser_and_data_and_int)
SETTER_WRAPPER(UnparsedEntityDeclHandler, unparsed_entity_decl)
SETTER_WRAPPER(NotationDeclHandler, notation_decl)
SETTER_WRAPPER(StartNamespaceDeclHandler, parser_and_data_and_data)
SETTER_WRAPPER(EndNamespaceDeclHandler, parser_and_data)
SETTER_WRAPPER(CommentHandler, parser_and_data)
SETTER_WRAPPER(StartCdataSectionHandler, parser_only)
SETTER_WRAPPER(EndCdataSectionHandler, parser_only)
SETTER_WRAPPER(DefaultHandler, parser_and_data_and_int)
SETTER_WRAPPER(DefaultHandlerExpand, parser_and_data_and_int)
SETTER_WRAPPER(NotStandaloneHandler, not_standalone)
SETTER_WRAPPER(ExternalEntityRefHandler, external_entity_ref)
SETTER_WRAPPER(StartDoctypeDeclHandler, start_doctype_decl)
SETTER_WRAPPER(EndDoctypeDeclHandler, parser_only)
SETTER_WRAPPER(EntityDeclHandler, entity_decl)
SETTER_WRAPPER(XmlDeclHandler, xml_decl)
SETTER_WRAPPER(ElementDeclHandler, element_decl)
SETTER_WRAPPER(AttlistDeclHandler, attlist_decl)
#if XML_COMBINED_VERSION >= 19504
SETTER_WRAPPER(SkippedEntityHandler, parser_and_data_and_int)
#endif
#undef SETTER_WRAPPER

static struct HandlerInfo handler_info[] = {

    // The cast to `xmlhandlersetter` is needed as the signature of XML
    // handler functions is not compatible with `xmlhandlersetter` since
    // their second parameter is narrower than a `const void *`.
#define HANDLER_INFO(name) \
    {#name, (xmlhandlersetter)pyexpat_Set##name, (xmlhandler)my_##name},

    HANDLER_INFO(StartElementHandler)
    HANDLER_INFO(EndElementHandler)
    HANDLER_INFO(ProcessingInstructionHandler)
    HANDLER_INFO(CharacterDataHandler)
    HANDLER_INFO(UnparsedEntityDeclHandler)
    HANDLER_INFO(NotationDeclHandler)
    HANDLER_INFO(StartNamespaceDeclHandler)
    HANDLER_INFO(EndNamespaceDeclHandler)
    HANDLER_INFO(CommentHandler)
    HANDLER_INFO(StartCdataSectionHandler)
    HANDLER_INFO(EndCdataSectionHandler)
    HANDLER_INFO(DefaultHandler)
    HANDLER_INFO(DefaultHandlerExpand)
    HANDLER_INFO(NotStandaloneHandler)
    HANDLER_INFO(ExternalEntityRefHandler)
    HANDLER_INFO(StartDoctypeDeclHandler)
    HANDLER_INFO(EndDoctypeDeclHandler)
    HANDLER_INFO(EntityDeclHandler)
    HANDLER_INFO(XmlDeclHandler)
    HANDLER_INFO(ElementDeclHandler)
    HANDLER_INFO(AttlistDeclHandler)
#if XML_COMBINED_VERSION >= 19504
    HANDLER_INFO(SkippedEntityHandler)
#endif

#undef HANDLER_INFO

    {NULL, NULL, NULL} /* sentinel */
};
