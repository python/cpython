#include "Python.h"
#include <ctype.h>

#include "frameobject.h"
#include "expat.h"

#include "pyexpat.h"

#define XML_COMBINED_VERSION (10000*XML_MAJOR_VERSION+100*XML_MINOR_VERSION+XML_MICRO_VERSION)

#define FIX_TRACE

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

static PyObject *ErrorObject;

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
    PyObject *intern;           /* Dictionary to intern strings */
    PyObject **handlers;
} xmlparseobject;

#define CHARACTER_DATA_BUFFER_SIZE 8192

static PyTypeObject Xmlparsetype;

typedef void (*xmlhandlersetter)(XML_Parser self, void *meth);
typedef void* xmlhandler;

struct HandlerInfo {
    const char *name;
    xmlhandlersetter setter;
    xmlhandler handler;
    PyCodeObject *tb_code;
    PyObject *nameobj;
};

static struct HandlerInfo handler_info[64];

/* Set an integer attribute on the error object; return true on success,
 * false on an exception.
 */
static int
set_error_attr(PyObject *err, char *name, int value)
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
set_error(xmlparseobject *self, enum XML_Error code)
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
    err = PyObject_CallFunction(ErrorObject, "O", buffer);
    Py_DECREF(buffer);
    if (  err != NULL
          && set_error_attr(err, "code", code)
          && set_error_attr(err, "offset", column)
          && set_error_attr(err, "lineno", lineno)) {
        PyErr_SetObject(ErrorObject, err);
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

static PyObject *
get_handler_name(struct HandlerInfo *hinfo)
{
    PyObject *name = hinfo->nameobj;
    if (name == NULL) {
        name = PyUnicode_FromString(hinfo->name);
        hinfo->nameobj = name;
    }
    Py_XINCREF(name);
    return name;
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
        Py_INCREF(Py_None);
        return Py_None;
    }
    return PyUnicode_DecodeUTF8(str, strlen(str), "strict");
}

static PyObject *
conv_string_len_to_unicode(const XML_Char *str, int len)
{
    /* XXX currently this code assumes that XML_Char is 8-bit,
       and hence in UTF-8.  */
    /* UTF-8 from Expat, Unicode desired */
    if (str == NULL) {
        Py_INCREF(Py_None);
        return Py_None;
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

static PyCodeObject*
getcode(enum HandlerTypes slot, char* func_name, int lineno)
{
    if (handler_info[slot].tb_code == NULL) {
        handler_info[slot].tb_code =
            PyCode_NewEmpty(__FILE__, func_name, lineno);
    }
    return handler_info[slot].tb_code;
}

#ifdef FIX_TRACE
static int
trace_frame(PyThreadState *tstate, PyFrameObject *f, int code, PyObject *val)
{
    int result = 0;
    if (!tstate->use_tracing || tstate->tracing)
        return 0;
    if (tstate->c_profilefunc != NULL) {
        tstate->tracing++;
        result = tstate->c_profilefunc(tstate->c_profileobj,
                                       f, code , val);
        tstate->use_tracing = ((tstate->c_tracefunc != NULL)
                               || (tstate->c_profilefunc != NULL));
        tstate->tracing--;
        if (result)
            return result;
    }
    if (tstate->c_tracefunc != NULL) {
        tstate->tracing++;
        result = tstate->c_tracefunc(tstate->c_traceobj,
                                     f, code , val);
        tstate->use_tracing = ((tstate->c_tracefunc != NULL)
                               || (tstate->c_profilefunc != NULL));
        tstate->tracing--;
    }
    return result;
}

static int
trace_frame_exc(PyThreadState *tstate, PyFrameObject *f)
{
    PyObject *type, *value, *traceback, *arg;
    int err;

    if (tstate->c_tracefunc == NULL)
        return 0;

    PyErr_Fetch(&type, &value, &traceback);
    if (value == NULL) {
        value = Py_None;
        Py_INCREF(value);
    }
    arg = PyTuple_Pack(3, type, value, traceback);
    if (arg == NULL) {
        PyErr_Restore(type, value, traceback);
        return 0;
    }
    err = trace_frame(tstate, f, PyTrace_EXCEPTION, arg);
    Py_DECREF(arg);
    if (err == 0)
        PyErr_Restore(type, value, traceback);
    else {
        Py_XDECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(traceback);
    }
    return err;
}
#endif

static PyObject*
call_with_frame(PyCodeObject *c, PyObject* func, PyObject* args,
                xmlparseobject *self)
{
    PyThreadState *tstate = PyThreadState_GET();
    PyFrameObject *f;
    PyObject *res;

    if (c == NULL)
        return NULL;

    f = PyFrame_New(tstate, c, PyEval_GetGlobals(), NULL);
    if (f == NULL)
        return NULL;
    tstate->frame = f;
#ifdef FIX_TRACE
    if (trace_frame(tstate, f, PyTrace_CALL, Py_None) < 0) {
        return NULL;
    }
#endif
    res = PyEval_CallObject(func, args);
    if (res == NULL) {
        if (tstate->curexc_traceback == NULL)
            PyTraceBack_Here(f);
        XML_StopParser(self->itself, XML_FALSE);
#ifdef FIX_TRACE
        if (trace_frame_exc(tstate, f) < 0) {
            return NULL;
        }
    }
    else {
        if (trace_frame(tstate, f, PyTrace_RETURN, res) < 0) {
            Py_XDECREF(res);
            res = NULL;
        }
    }
#else
    }
#endif
    tstate->frame = f->f_back;
    Py_DECREF(f);
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
    value = PyDict_GetItem(self->intern, result);
    if (!value) {
        if (PyDict_SetItem(self->intern, result, result) == 0)
            return result;
        else
            return NULL;
    }
    Py_INCREF(value);
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
    temp = call_with_frame(getcode(CharacterData, "CharacterData", __LINE__),
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
    xmlparseobject *self = (xmlparseobject *) userData;
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
    xmlparseobject *self = (xmlparseobject *)userData;

    if (have_handler(self, StartElement)) {
        PyObject *container, *rv, *args;
        int i, max;

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
                return;
            }
            else {
                Py_DECREF(n);
                Py_DECREF(v);
            }
        }
        args = string_intern(self, name);
        if (args != NULL)
            args = Py_BuildValue("(NN)", args, container);
        if (args == NULL) {
            Py_DECREF(container);
            return;
        }
        /* Container is now a borrowed reference; ignore it. */
        self->in_callback = 1;
        rv = call_with_frame(getcode(StartElement, "StartElement", __LINE__),
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

#define RC_HANDLER(RC, NAME, PARAMS, INIT, PARAM_FORMAT, CONVERSION, \
                RETURN, GETUSERDATA) \
static RC \
my_##NAME##Handler PARAMS {\
    xmlparseobject *self = GETUSERDATA ; \
    PyObject *args = NULL; \
    PyObject *rv = NULL; \
    INIT \
\
    if (have_handler(self, NAME)) { \
        if (flush_character_buffer(self) < 0) \
            return RETURN; \
        args = Py_BuildValue PARAM_FORMAT ;\
        if (!args) { flag_error(self); return RETURN;} \
        self->in_callback = 1; \
        rv = call_with_frame(getcode(NAME,#NAME,__LINE__), \
                             self->handlers[NAME], args, self); \
        self->in_callback = 0; \
        Py_DECREF(args); \
        if (rv == NULL) { \
            flag_error(self); \
            return RETURN; \
        } \
        CONVERSION \
        Py_DECREF(rv); \
    } \
    return RETURN; \
}

#define VOID_HANDLER(NAME, PARAMS, PARAM_FORMAT) \
        RC_HANDLER(void, NAME, PARAMS, ;, PARAM_FORMAT, ;, ;,\
        (xmlparseobject *)userData)

#define INT_HANDLER(NAME, PARAMS, PARAM_FORMAT)\
        RC_HANDLER(int, NAME, PARAMS, int rc=0;, PARAM_FORMAT, \
                        rc = PyLong_AsLong(rv);, rc, \
        (xmlparseobject *)userData)

VOID_HANDLER(EndElement,
             (void *userData, const XML_Char *name),
             ("(N)", string_intern(self, name)))

VOID_HANDLER(ProcessingInstruction,
             (void *userData,
              const XML_Char *target,
              const XML_Char *data),
             ("(NO&)", string_intern(self, target), conv_string_to_unicode ,data))

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
              conv_string_to_unicode ,version, conv_string_to_unicode ,encoding,
              standalone))

static PyObject *
conv_content_model(XML_Content * const model,
                   PyObject *(*conv_string)(const XML_Char *))
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
                               conv_string,model->name, children);
    }
    return result;
}

static void
my_ElementDeclHandler(void *userData,
                      const XML_Char *name,
                      XML_Content *model)
{
    xmlparseobject *self = (xmlparseobject *)userData;
    PyObject *args = NULL;

    if (have_handler(self, ElementDecl)) {
        PyObject *rv = NULL;
        PyObject *modelobj, *nameobj;

        if (flush_character_buffer(self) < 0)
            goto finally;
        modelobj = conv_content_model(model, (conv_string_to_unicode));
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
            Py_DECREF(modelobj);
            flag_error(self);
            goto finally;
        }
        self->in_callback = 1;
        rv = call_with_frame(getcode(ElementDecl, "ElementDecl", __LINE__),
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
              conv_string_to_unicode ,att_type, conv_string_to_unicode ,dflt,
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
                ("(O&)", conv_string_to_unicode ,data))

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
                 conv_string_to_unicode ,context, string_intern(self, base),
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

static PyObject *
get_parse_result(xmlparseobject *self, int rv)
{
    if (PyErr_Occurred()) {
        return NULL;
    }
    if (rv == 0) {
        return set_error(self, XML_GetErrorCode(self->itself));
    }
    if (flush_character_buffer(self) < 0) {
        return NULL;
    }
    return PyLong_FromLong(rv);
}

PyDoc_STRVAR(xmlparse_Parse__doc__,
"Parse(data[, isfinal])\n\
Parse XML data.  `isfinal' should be true at end of input.");

static PyObject *
xmlparse_Parse(xmlparseobject *self, PyObject *args)
{
    char *s;
    int slen;
    int isFinal = 0;

    if (!PyArg_ParseTuple(args, "s#|i:Parse", &s, &slen, &isFinal))
        return NULL;

    return get_parse_result(self, XML_Parse(self->itself, s, slen, isFinal));
}

/* File reading copied from cPickle */

#define BUF_SIZE 2048

static int
readinst(char *buf, int buf_size, PyObject *meth)
{
    PyObject *str;
    Py_ssize_t len;
    char *ptr;

    str = PyObject_CallFunction(meth, "n", buf_size);
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

PyDoc_STRVAR(xmlparse_ParseFile__doc__,
"ParseFile(file)\n\
Parse XML data from file-like object.");

static PyObject *
xmlparse_ParseFile(xmlparseobject *self, PyObject *f)
{
    int rv = 1;
    PyObject *readmethod = NULL;


    readmethod = PyObject_GetAttrString(f, "read");
    if (readmethod == NULL) {
        PyErr_Clear();
        PyErr_SetString(PyExc_TypeError,
                        "argument must have 'read' attribute");
        return NULL;
    }
    for (;;) {
        int bytes_read;
        void *buf = XML_GetBuffer(self->itself, BUF_SIZE);
        if (buf == NULL) {
            Py_XDECREF(readmethod);
            return PyErr_NoMemory();
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
    return get_parse_result(self, rv);
}

PyDoc_STRVAR(xmlparse_SetBase__doc__,
"SetBase(base_url)\n\
Set the base URL for the parser.");

static PyObject *
xmlparse_SetBase(xmlparseobject *self, PyObject *args)
{
    char *base;

    if (!PyArg_ParseTuple(args, "s:SetBase", &base))
        return NULL;
    if (!XML_SetBase(self->itself, base)) {
        return PyErr_NoMemory();
    }
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(xmlparse_GetBase__doc__,
"GetBase() -> url\n\
Return base URL string for the parser.");

static PyObject *
xmlparse_GetBase(xmlparseobject *self, PyObject *unused)
{
    return Py_BuildValue("z", XML_GetBase(self->itself));
}

PyDoc_STRVAR(xmlparse_GetInputContext__doc__,
"GetInputContext() -> string\n\
Return the untranslated text of the input that caused the current event.\n\
If the event was generated by a large amount of text (such as a start tag\n\
for an element with many attributes), not all of the text may be available.");

static PyObject *
xmlparse_GetInputContext(xmlparseobject *self, PyObject *unused)
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

PyDoc_STRVAR(xmlparse_ExternalEntityParserCreate__doc__,
"ExternalEntityParserCreate(context[, encoding])\n\
Create a parser for parsing an external entity based on the\n\
information passed to the ExternalEntityRefHandler.");

static PyObject *
xmlparse_ExternalEntityParserCreate(xmlparseobject *self, PyObject *args)
{
    char *context;
    char *encoding = NULL;
    xmlparseobject *new_parser;
    int i;

    if (!PyArg_ParseTuple(args, "z|s:ExternalEntityParserCreate",
                          &context, &encoding)) {
        return NULL;
    }

    new_parser = PyObject_GC_New(xmlparseobject, &Xmlparsetype);
    if (new_parser == NULL)
        return NULL;
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
    new_parser->intern = self->intern;
    Py_XINCREF(new_parser->intern);
    PyObject_GC_Track(new_parser);

    if (self->buffer != NULL) {
        new_parser->buffer = malloc(new_parser->buffer_size);
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
    for (i = 0; handler_info[i].name != NULL; i++)
        /* do nothing */;

    new_parser->handlers = malloc(sizeof(PyObject *) * i);
    if (!new_parser->handlers) {
        Py_DECREF(new_parser);
        return PyErr_NoMemory();
    }
    clear_handlers(new_parser, 1);

    /* then copy handlers from self */
    for (i = 0; handler_info[i].name != NULL; i++) {
        PyObject *handler = self->handlers[i];
        if (handler != NULL) {
            Py_INCREF(handler);
            new_parser->handlers[i] = handler;
            handler_info[i].setter(new_parser->itself,
                                   handler_info[i].handler);
        }
    }
    return (PyObject *)new_parser;
}

PyDoc_STRVAR(xmlparse_SetParamEntityParsing__doc__,
"SetParamEntityParsing(flag) -> success\n\
Controls parsing of parameter entities (including the external DTD\n\
subset). Possible flag values are XML_PARAM_ENTITY_PARSING_NEVER,\n\
XML_PARAM_ENTITY_PARSING_UNLESS_STANDALONE and\n\
XML_PARAM_ENTITY_PARSING_ALWAYS. Returns true if setting the flag\n\
was successful.");

static PyObject*
xmlparse_SetParamEntityParsing(xmlparseobject *p, PyObject* args)
{
    int flag;
    if (!PyArg_ParseTuple(args, "i", &flag))
        return NULL;
    flag = XML_SetParamEntityParsing(p->itself, flag);
    return PyLong_FromLong(flag);
}


#if XML_COMBINED_VERSION >= 19505
PyDoc_STRVAR(xmlparse_UseForeignDTD__doc__,
"UseForeignDTD([flag])\n\
Allows the application to provide an artificial external subset if one is\n\
not specified as part of the document instance.  This readily allows the\n\
use of a 'default' document type controlled by the application, while still\n\
getting the advantage of providing document type information to the parser.\n\
'flag' defaults to True if not provided.");

static PyObject *
xmlparse_UseForeignDTD(xmlparseobject *self, PyObject *args)
{
    PyObject *flagobj = NULL;
    XML_Bool flag = XML_TRUE;
    enum XML_Error rc;
    if (!PyArg_UnpackTuple(args, "UseForeignDTD", 0, 1, &flagobj))
        return NULL;
    if (flagobj != NULL)
        flag = PyObject_IsTrue(flagobj) ? XML_TRUE : XML_FALSE;
    rc = XML_UseForeignDTD(self->itself, flag);
    if (rc != XML_ERROR_NONE) {
        return set_error(self, rc);
    }
    Py_INCREF(Py_None);
    return Py_None;
}
#endif

static PyObject *xmlparse_dir(PyObject *self, PyObject* noargs);

static struct PyMethodDef xmlparse_methods[] = {
    {"Parse",     (PyCFunction)xmlparse_Parse,
                  METH_VARARGS, xmlparse_Parse__doc__},
    {"ParseFile", (PyCFunction)xmlparse_ParseFile,
                  METH_O,       xmlparse_ParseFile__doc__},
    {"SetBase",   (PyCFunction)xmlparse_SetBase,
                  METH_VARARGS, xmlparse_SetBase__doc__},
    {"GetBase",   (PyCFunction)xmlparse_GetBase,
                  METH_NOARGS, xmlparse_GetBase__doc__},
    {"ExternalEntityParserCreate", (PyCFunction)xmlparse_ExternalEntityParserCreate,
                  METH_VARARGS, xmlparse_ExternalEntityParserCreate__doc__},
    {"SetParamEntityParsing", (PyCFunction)xmlparse_SetParamEntityParsing,
                  METH_VARARGS, xmlparse_SetParamEntityParsing__doc__},
    {"GetInputContext", (PyCFunction)xmlparse_GetInputContext,
                  METH_NOARGS, xmlparse_GetInputContext__doc__},
#if XML_COMBINED_VERSION >= 19505
    {"UseForeignDTD", (PyCFunction)xmlparse_UseForeignDTD,
                  METH_VARARGS, xmlparse_UseForeignDTD__doc__},
#endif
    {"__dir__", xmlparse_dir, METH_NOARGS},
    {NULL,        NULL}         /* sentinel */
};

/* ---------- */



/* pyexpat international encoding support.
   Make it as simple as possible.
*/

static char template_buffer[257];

static void
init_template_buffer(void)
{
    int i;
    for (i = 0; i < 256; i++) {
        template_buffer[i] = i;
    }
    template_buffer[256] = 0;
}

static int
PyUnknownEncodingHandler(void *encodingHandlerData,
                         const XML_Char *name,
                         XML_Encoding *info)
{
    PyUnicodeObject *_u_string = NULL;
    int result = 0;
    int i;

    /* Yes, supports only 8bit encodings */
    _u_string = (PyUnicodeObject *)
        PyUnicode_Decode(template_buffer, 256, name, "replace");

    if (_u_string == NULL)
        return result;

    for (i = 0; i < 256; i++) {
        /* Stupid to access directly, but fast */
        Py_UNICODE c = _u_string->str[i];
        if (c == Py_UNICODE_REPLACEMENT_CHARACTER)
            info->map[i] = -1;
        else
            info->map[i] = c;
    }
    info->data = NULL;
    info->convert = NULL;
    info->release = NULL;
    result = 1;
    Py_DECREF(_u_string);
    return result;
}


static PyObject *
newxmlparseobject(char *encoding, char *namespace_separator, PyObject *intern)
{
    int i;
    xmlparseobject *self;

    self = PyObject_GC_New(xmlparseobject, &Xmlparsetype);
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
    if (namespace_separator != NULL) {
        self->itself = XML_ParserCreateNS(encoding, *namespace_separator);
    }
    else {
        self->itself = XML_ParserCreate(encoding);
    }
    self->intern = intern;
    Py_XINCREF(self->intern);
    PyObject_GC_Track(self);
    if (self->itself == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "XML_ParserCreate failed");
        Py_DECREF(self);
        return NULL;
    }
    XML_SetUserData(self->itself, (void *)self);
    XML_SetUnknownEncodingHandler(self->itself,
                  (XML_UnknownEncodingHandler) PyUnknownEncodingHandler, NULL);

    for (i = 0; handler_info[i].name != NULL; i++)
        /* do nothing */;

    self->handlers = malloc(sizeof(PyObject *) * i);
    if (!self->handlers) {
        Py_DECREF(self);
        return PyErr_NoMemory();
    }
    clear_handlers(self, 1);

    return (PyObject*)self;
}


static void
xmlparse_dealloc(xmlparseobject *self)
{
    int i;
    PyObject_GC_UnTrack(self);
    if (self->itself != NULL)
        XML_ParserFree(self->itself);
    self->itself = NULL;

    if (self->handlers != NULL) {
        PyObject *temp;
        for (i = 0; handler_info[i].name != NULL; i++) {
            temp = self->handlers[i];
            self->handlers[i] = NULL;
            Py_XDECREF(temp);
        }
        free(self->handlers);
        self->handlers = NULL;
    }
    if (self->buffer != NULL) {
        free(self->buffer);
        self->buffer = NULL;
    }
    Py_XDECREF(self->intern);
    PyObject_GC_Del(self);
}

static int
handlername2int(PyObject *name)
{
    int i;
    for (i = 0; handler_info[i].name != NULL; i++) {
        if (PyUnicode_CompareWithASCIIString(
                name, handler_info[i].name) == 0) {
            return i;
        }
    }
    return -1;
}

static PyObject *
get_pybool(int istrue)
{
    PyObject *result = istrue ? Py_True : Py_False;
    Py_INCREF(result);
    return result;
}

static PyObject *
xmlparse_getattro(xmlparseobject *self, PyObject *nameobj)
{
    Py_UNICODE *name;
    int handlernum = -1;

    if (!PyUnicode_Check(nameobj))
        goto generic;

    handlernum = handlername2int(nameobj);

    if (handlernum != -1) {
        PyObject *result = self->handlers[handlernum];
        if (result == NULL)
            result = Py_None;
        Py_INCREF(result);
        return result;
    }

    name = PyUnicode_AS_UNICODE(nameobj);
    if (name[0] == 'E') {
        if (PyUnicode_CompareWithASCIIString(nameobj, "ErrorCode") == 0)
            return PyLong_FromLong((long)
                                  XML_GetErrorCode(self->itself));
        if (PyUnicode_CompareWithASCIIString(nameobj, "ErrorLineNumber") == 0)
            return PyLong_FromLong((long)
                                  XML_GetErrorLineNumber(self->itself));
        if (PyUnicode_CompareWithASCIIString(nameobj, "ErrorColumnNumber") == 0)
            return PyLong_FromLong((long)
                                  XML_GetErrorColumnNumber(self->itself));
        if (PyUnicode_CompareWithASCIIString(nameobj, "ErrorByteIndex") == 0)
            return PyLong_FromLong((long)
                                  XML_GetErrorByteIndex(self->itself));
    }
    if (name[0] == 'C') {
        if (PyUnicode_CompareWithASCIIString(nameobj, "CurrentLineNumber") == 0)
            return PyLong_FromLong((long)
                                  XML_GetCurrentLineNumber(self->itself));
        if (PyUnicode_CompareWithASCIIString(nameobj, "CurrentColumnNumber") == 0)
            return PyLong_FromLong((long)
                                  XML_GetCurrentColumnNumber(self->itself));
        if (PyUnicode_CompareWithASCIIString(nameobj, "CurrentByteIndex") == 0)
            return PyLong_FromLong((long)
                                  XML_GetCurrentByteIndex(self->itself));
    }
    if (name[0] == 'b') {
        if (PyUnicode_CompareWithASCIIString(nameobj, "buffer_size") == 0)
            return PyLong_FromLong((long) self->buffer_size);
        if (PyUnicode_CompareWithASCIIString(nameobj, "buffer_text") == 0)
            return get_pybool(self->buffer != NULL);
        if (PyUnicode_CompareWithASCIIString(nameobj, "buffer_used") == 0)
            return PyLong_FromLong((long) self->buffer_used);
    }
    if (PyUnicode_CompareWithASCIIString(nameobj, "namespace_prefixes") == 0)
        return get_pybool(self->ns_prefixes);
    if (PyUnicode_CompareWithASCIIString(nameobj, "ordered_attributes") == 0)
        return get_pybool(self->ordered_attributes);
    if (PyUnicode_CompareWithASCIIString(nameobj, "specified_attributes") == 0)
        return get_pybool((long) self->specified_attributes);
    if (PyUnicode_CompareWithASCIIString(nameobj, "intern") == 0) {
        if (self->intern == NULL) {
            Py_INCREF(Py_None);
            return Py_None;
        }
        else {
            Py_INCREF(self->intern);
            return self->intern;
        }
    }
  generic:
    return PyObject_GenericGetAttr((PyObject*)self, nameobj);
}

static PyObject *
xmlparse_dir(PyObject *self, PyObject* noargs)
{
#define APPEND(list, str)                               \
        do {                                            \
                PyObject *o = PyUnicode_FromString(str);        \
                if (o != NULL)                          \
                        PyList_Append(list, o);         \
                Py_XDECREF(o);                          \
        } while (0)

    int i;
    PyObject *rc = PyList_New(0);
    if (!rc)
        return NULL;
    for (i = 0; handler_info[i].name != NULL; i++) {
        PyObject *o = get_handler_name(&handler_info[i]);
        if (o != NULL)
            PyList_Append(rc, o);
        Py_XDECREF(o);
    }
    APPEND(rc, "ErrorCode");
    APPEND(rc, "ErrorLineNumber");
    APPEND(rc, "ErrorColumnNumber");
    APPEND(rc, "ErrorByteIndex");
    APPEND(rc, "CurrentLineNumber");
    APPEND(rc, "CurrentColumnNumber");
    APPEND(rc, "CurrentByteIndex");
    APPEND(rc, "buffer_size");
    APPEND(rc, "buffer_text");
    APPEND(rc, "buffer_used");
    APPEND(rc, "namespace_prefixes");
    APPEND(rc, "ordered_attributes");
    APPEND(rc, "specified_attributes");
    APPEND(rc, "intern");

#undef APPEND

    if (PyErr_Occurred()) {
        Py_DECREF(rc);
        rc = NULL;
    }

    return rc;
}

static int
sethandler(xmlparseobject *self, PyObject *name, PyObject* v)
{
    int handlernum = handlername2int(name);
    if (handlernum >= 0) {
        xmlhandler c_handler = NULL;
        PyObject *temp = self->handlers[handlernum];

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
            if (handlernum == CharacterData && self->in_callback)
                c_handler = noop_character_data_handler;
            v = NULL;
        }
        else if (v != NULL) {
            Py_INCREF(v);
            c_handler = handler_info[handlernum].handler;
        }
        self->handlers[handlernum] = v;
        Py_XDECREF(temp);
        handler_info[handlernum].setter(self->itself, c_handler);
        return 1;
    }
    return 0;
}

static int
xmlparse_setattro(xmlparseobject *self, PyObject *name, PyObject *v)
{
    /* Set attribute 'name' to value 'v'. v==NULL means delete */
    if (v == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Cannot delete attribute");
        return -1;
    }
    assert(PyUnicode_Check(name));
    if (PyUnicode_CompareWithASCIIString(name, "buffer_text") == 0) {
        if (PyObject_IsTrue(v)) {
            if (self->buffer == NULL) {
                self->buffer = malloc(self->buffer_size);
                if (self->buffer == NULL) {
                    PyErr_NoMemory();
                    return -1;
                }
                self->buffer_used = 0;
            }
        }
        else if (self->buffer != NULL) {
            if (flush_character_buffer(self) < 0)
                return -1;
            free(self->buffer);
            self->buffer = NULL;
        }
        return 0;
    }
    if (PyUnicode_CompareWithASCIIString(name, "namespace_prefixes") == 0) {
        if (PyObject_IsTrue(v))
            self->ns_prefixes = 1;
        else
            self->ns_prefixes = 0;
        XML_SetReturnNSTriplet(self->itself, self->ns_prefixes);
        return 0;
    }
    if (PyUnicode_CompareWithASCIIString(name, "ordered_attributes") == 0) {
        if (PyObject_IsTrue(v))
            self->ordered_attributes = 1;
        else
            self->ordered_attributes = 0;
        return 0;
    }
    if (PyUnicode_CompareWithASCIIString(name, "specified_attributes") == 0) {
        if (PyObject_IsTrue(v))
            self->specified_attributes = 1;
        else
            self->specified_attributes = 0;
        return 0;
    }

    if (PyUnicode_CompareWithASCIIString(name, "buffer_size") == 0) {
      long new_buffer_size;
      if (!PyLong_Check(v)) {
        PyErr_SetString(PyExc_TypeError, "buffer_size must be an integer");
        return -1;
      }

      new_buffer_size=PyLong_AS_LONG(v);
      /* trivial case -- no change */
      if (new_buffer_size == self->buffer_size) {
        return 0;
      }

      if (new_buffer_size <= 0) {
        PyErr_SetString(PyExc_ValueError, "buffer_size must be greater than zero");
        return -1;
      }

      /* check maximum */
      if (new_buffer_size > INT_MAX) {
        char errmsg[100];
        sprintf(errmsg, "buffer_size must not be greater than %i", INT_MAX);
        PyErr_SetString(PyExc_ValueError, errmsg);
        return -1;
      }

      if (self->buffer != NULL) {
        /* there is already a buffer */
        if (self->buffer_used != 0) {
          flush_character_buffer(self);
        }
        /* free existing buffer */
        free(self->buffer);
      }
      self->buffer = malloc(new_buffer_size);
      if (self->buffer == NULL) {
        PyErr_NoMemory();
        return -1;
      }
      self->buffer_size = new_buffer_size;
      return 0;
    }

    if (PyUnicode_CompareWithASCIIString(name, "CharacterDataHandler") == 0) {
        /* If we're changing the character data handler, flush all
         * cached data with the old handler.  Not sure there's a
         * "right" thing to do, though, but this probably won't
         * happen.
         */
        if (flush_character_buffer(self) < 0)
            return -1;
    }
    if (sethandler(self, name, v)) {
        return 0;
    }
    PyErr_SetObject(PyExc_AttributeError, name);
    return -1;
}

static int
xmlparse_traverse(xmlparseobject *op, visitproc visit, void *arg)
{
    int i;
    for (i = 0; handler_info[i].name != NULL; i++)
        Py_VISIT(op->handlers[i]);
    return 0;
}

static int
xmlparse_clear(xmlparseobject *op)
{
    clear_handlers(op, 0);
    Py_CLEAR(op->intern);
    return 0;
}

PyDoc_STRVAR(Xmlparsetype__doc__, "XML parser");

static PyTypeObject Xmlparsetype = {
        PyVarObject_HEAD_INIT(NULL, 0)
        "pyexpat.xmlparser",            /*tp_name*/
        sizeof(xmlparseobject),         /*tp_basicsize*/
        0,                              /*tp_itemsize*/
        /* methods */
        (destructor)xmlparse_dealloc,   /*tp_dealloc*/
        (printfunc)0,           /*tp_print*/
        0,                      /*tp_getattr*/
        0,  /*tp_setattr*/
        0,                      /*tp_reserved*/
        (reprfunc)0,            /*tp_repr*/
        0,                      /*tp_as_number*/
        0,              /*tp_as_sequence*/
        0,              /*tp_as_mapping*/
        (hashfunc)0,            /*tp_hash*/
        (ternaryfunc)0,         /*tp_call*/
        (reprfunc)0,            /*tp_str*/
        (getattrofunc)xmlparse_getattro, /* tp_getattro */
        (setattrofunc)xmlparse_setattro,              /* tp_setattro */
        0,              /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
        Xmlparsetype__doc__, /* tp_doc - Documentation string */
        (traverseproc)xmlparse_traverse,        /* tp_traverse */
        (inquiry)xmlparse_clear,                /* tp_clear */
        0,                              /* tp_richcompare */
        0,                              /* tp_weaklistoffset */
        0,                              /* tp_iter */
        0,                              /* tp_iternext */
        xmlparse_methods,               /* tp_methods */
};

/* End of code for xmlparser objects */
/* -------------------------------------------------------- */

PyDoc_STRVAR(pyexpat_ParserCreate__doc__,
"ParserCreate([encoding[, namespace_separator]]) -> parser\n\
Return a new XML parser object.");

static PyObject *
pyexpat_ParserCreate(PyObject *notused, PyObject *args, PyObject *kw)
{
    char *encoding = NULL;
    char *namespace_separator = NULL;
    PyObject *intern = NULL;
    PyObject *result;
    int intern_decref = 0;
    static char *kwlist[] = {"encoding", "namespace_separator",
                                   "intern", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kw, "|zzO:ParserCreate", kwlist,
                                     &encoding, &namespace_separator, &intern))
        return NULL;
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

    result = newxmlparseobject(encoding, namespace_separator, intern);
    if (intern_decref) {
        Py_DECREF(intern);
    }
    return result;
}

PyDoc_STRVAR(pyexpat_ErrorString__doc__,
"ErrorString(errno) -> string\n\
Returns string error for given number.");

static PyObject *
pyexpat_ErrorString(PyObject *self, PyObject *args)
{
    long code = 0;

    if (!PyArg_ParseTuple(args, "l:ErrorString", &code))
        return NULL;
    return Py_BuildValue("z", XML_ErrorString((int)code));
}

/* List of methods defined in the module */

static struct PyMethodDef pyexpat_methods[] = {
    {"ParserCreate",    (PyCFunction)pyexpat_ParserCreate,
     METH_VARARGS|METH_KEYWORDS, pyexpat_ParserCreate__doc__},
    {"ErrorString",     (PyCFunction)pyexpat_ErrorString,
     METH_VARARGS,      pyexpat_ErrorString__doc__},

    {NULL,       (PyCFunction)NULL, 0, NULL}            /* sentinel */
};

/* Module docstring */

PyDoc_STRVAR(pyexpat_module_documentation,
"Python wrapper for Expat parser.");

/* Return a Python string that represents the version number without the
 * extra cruft added by revision control, even if the right options were
 * given to the "cvs export" command to make it not include the extra
 * cruft.
 */
static PyObject *
get_version_string(void)
{
    static char *rcsid = "$Revision$";
    char *rev = rcsid;
    int i = 0;

    while (!isdigit(Py_CHARMASK(*rev)))
        ++rev;
    while (rev[i] != ' ' && rev[i] != '\0')
        ++i;

    return PyUnicode_FromStringAndSize(rev, i);
}

/* Initialization function for the module */

#ifndef MODULE_NAME
#define MODULE_NAME "pyexpat"
#endif

#ifndef MODULE_INITFUNC
#define MODULE_INITFUNC PyInit_pyexpat
#endif

#ifndef PyMODINIT_FUNC
#   ifdef MS_WINDOWS
#       define PyMODINIT_FUNC __declspec(dllexport) void
#   else
#       define PyMODINIT_FUNC void
#   endif
#endif

PyMODINIT_FUNC MODULE_INITFUNC(void);  /* avoid compiler warnings */

static struct PyModuleDef pyexpatmodule = {
        PyModuleDef_HEAD_INIT,
        MODULE_NAME,
        pyexpat_module_documentation,
        -1,
        pyexpat_methods,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC
MODULE_INITFUNC(void)
{
    PyObject *m, *d;
    PyObject *errmod_name = PyUnicode_FromString(MODULE_NAME ".errors");
    PyObject *errors_module;
    PyObject *modelmod_name;
    PyObject *model_module;
    PyObject *sys_modules;
    PyObject *tmpnum, *tmpstr;
    PyObject *codes_dict;
    PyObject *rev_codes_dict;
    int res;
    static struct PyExpat_CAPI capi;
    PyObject *capi_object;

    if (errmod_name == NULL)
        return NULL;
    modelmod_name = PyUnicode_FromString(MODULE_NAME ".model");
    if (modelmod_name == NULL)
        return NULL;

    if (PyType_Ready(&Xmlparsetype) < 0)
        return NULL;

    /* Create the module and add the functions */
    m = PyModule_Create(&pyexpatmodule);
    if (m == NULL)
        return NULL;

    /* Add some symbolic constants to the module */
    if (ErrorObject == NULL) {
        ErrorObject = PyErr_NewException("xml.parsers.expat.ExpatError",
                                         NULL, NULL);
        if (ErrorObject == NULL)
            return NULL;
    }
    Py_INCREF(ErrorObject);
    PyModule_AddObject(m, "error", ErrorObject);
    Py_INCREF(ErrorObject);
    PyModule_AddObject(m, "ExpatError", ErrorObject);
    Py_INCREF(&Xmlparsetype);
    PyModule_AddObject(m, "XMLParserType", (PyObject *) &Xmlparsetype);

    PyModule_AddObject(m, "__version__", get_version_string());
    PyModule_AddStringConstant(m, "EXPAT_VERSION",
                               (char *) XML_ExpatVersion());
    {
        XML_Expat_Version info = XML_ExpatVersionInfo();
        PyModule_AddObject(m, "version_info",
                           Py_BuildValue("(iii)", info.major,
                                         info.minor, info.micro));
    }
    init_template_buffer();
    /* XXX When Expat supports some way of figuring out how it was
       compiled, this should check and set native_encoding
       appropriately.
    */
    PyModule_AddStringConstant(m, "native_encoding", "UTF-8");

    sys_modules = PySys_GetObject("modules");
    d = PyModule_GetDict(m);
    errors_module = PyDict_GetItem(d, errmod_name);
    if (errors_module == NULL) {
        errors_module = PyModule_New(MODULE_NAME ".errors");
        if (errors_module != NULL) {
            PyDict_SetItem(sys_modules, errmod_name, errors_module);
            /* gives away the reference to errors_module */
            PyModule_AddObject(m, "errors", errors_module);
        }
    }
    Py_DECREF(errmod_name);
    model_module = PyDict_GetItem(d, modelmod_name);
    if (model_module == NULL) {
        model_module = PyModule_New(MODULE_NAME ".model");
        if (model_module != NULL) {
            PyDict_SetItem(sys_modules, modelmod_name, model_module);
            /* gives away the reference to model_module */
            PyModule_AddObject(m, "model", model_module);
        }
    }
    Py_DECREF(modelmod_name);
    if (errors_module == NULL || model_module == NULL)
        /* Don't core dump later! */
        return NULL;

#if XML_COMBINED_VERSION > 19505
    {
        const XML_Feature *features = XML_GetFeatureList();
        PyObject *list = PyList_New(0);
        if (list == NULL)
            /* just ignore it */
            PyErr_Clear();
        else {
            int i = 0;
            for (; features[i].feature != XML_FEATURE_END; ++i) {
                int ok;
                PyObject *item = Py_BuildValue("si", features[i].name,
                                               features[i].value);
                if (item == NULL) {
                    Py_DECREF(list);
                    list = NULL;
                    break;
                }
                ok = PyList_Append(list, item);
                Py_DECREF(item);
                if (ok < 0) {
                    PyErr_Clear();
                    break;
                }
            }
            if (list != NULL)
                PyModule_AddObject(m, "features", list);
        }
    }
#endif

    codes_dict = PyDict_New();
    rev_codes_dict = PyDict_New();
    if (codes_dict == NULL || rev_codes_dict == NULL) {
        Py_XDECREF(codes_dict);
        Py_XDECREF(rev_codes_dict);
        return NULL;
    }

#define MYCONST(name) \
    if (PyModule_AddStringConstant(errors_module, #name,               \
                                   (char *)XML_ErrorString(name)) < 0) \
        return NULL;                                                   \
    tmpnum = PyLong_FromLong(name);                                    \
    if (tmpnum == NULL) return NULL;                                   \
    res = PyDict_SetItemString(codes_dict,                             \
                               XML_ErrorString(name), tmpnum);         \
    if (res < 0) return NULL;                                          \
    tmpstr = PyUnicode_FromString(XML_ErrorString(name));              \
    if (tmpstr == NULL) return NULL;                                   \
    res = PyDict_SetItem(rev_codes_dict, tmpnum, tmpstr);              \
    Py_DECREF(tmpstr);                                                 \
    Py_DECREF(tmpnum);                                                 \
    if (res < 0) return NULL;                                          \

    MYCONST(XML_ERROR_NO_MEMORY);
    MYCONST(XML_ERROR_SYNTAX);
    MYCONST(XML_ERROR_NO_ELEMENTS);
    MYCONST(XML_ERROR_INVALID_TOKEN);
    MYCONST(XML_ERROR_UNCLOSED_TOKEN);
    MYCONST(XML_ERROR_PARTIAL_CHAR);
    MYCONST(XML_ERROR_TAG_MISMATCH);
    MYCONST(XML_ERROR_DUPLICATE_ATTRIBUTE);
    MYCONST(XML_ERROR_JUNK_AFTER_DOC_ELEMENT);
    MYCONST(XML_ERROR_PARAM_ENTITY_REF);
    MYCONST(XML_ERROR_UNDEFINED_ENTITY);
    MYCONST(XML_ERROR_RECURSIVE_ENTITY_REF);
    MYCONST(XML_ERROR_ASYNC_ENTITY);
    MYCONST(XML_ERROR_BAD_CHAR_REF);
    MYCONST(XML_ERROR_BINARY_ENTITY_REF);
    MYCONST(XML_ERROR_ATTRIBUTE_EXTERNAL_ENTITY_REF);
    MYCONST(XML_ERROR_MISPLACED_XML_PI);
    MYCONST(XML_ERROR_UNKNOWN_ENCODING);
    MYCONST(XML_ERROR_INCORRECT_ENCODING);
    MYCONST(XML_ERROR_UNCLOSED_CDATA_SECTION);
    MYCONST(XML_ERROR_EXTERNAL_ENTITY_HANDLING);
    MYCONST(XML_ERROR_NOT_STANDALONE);
    MYCONST(XML_ERROR_UNEXPECTED_STATE);
    MYCONST(XML_ERROR_ENTITY_DECLARED_IN_PE);
    MYCONST(XML_ERROR_FEATURE_REQUIRES_XML_DTD);
    MYCONST(XML_ERROR_CANT_CHANGE_FEATURE_ONCE_PARSING);
    /* Added in Expat 1.95.7. */
    MYCONST(XML_ERROR_UNBOUND_PREFIX);
    /* Added in Expat 1.95.8. */
    MYCONST(XML_ERROR_UNDECLARING_PREFIX);
    MYCONST(XML_ERROR_INCOMPLETE_PE);
    MYCONST(XML_ERROR_XML_DECL);
    MYCONST(XML_ERROR_TEXT_DECL);
    MYCONST(XML_ERROR_PUBLICID);
    MYCONST(XML_ERROR_SUSPENDED);
    MYCONST(XML_ERROR_NOT_SUSPENDED);
    MYCONST(XML_ERROR_ABORTED);
    MYCONST(XML_ERROR_FINISHED);
    MYCONST(XML_ERROR_SUSPEND_PE);

    if (PyModule_AddStringConstant(errors_module, "__doc__",
                                   "Constants used to describe "
                                   "error conditions.") < 0)
        return NULL;

    if (PyModule_AddObject(errors_module, "codes", codes_dict) < 0)
        return NULL;
    if (PyModule_AddObject(errors_module, "messages", rev_codes_dict) < 0)
        return NULL;

#undef MYCONST

#define MYCONST(c) PyModule_AddIntConstant(m, #c, c)
    MYCONST(XML_PARAM_ENTITY_PARSING_NEVER);
    MYCONST(XML_PARAM_ENTITY_PARSING_UNLESS_STANDALONE);
    MYCONST(XML_PARAM_ENTITY_PARSING_ALWAYS);
#undef MYCONST

#define MYCONST(c) PyModule_AddIntConstant(model_module, #c, c)
    PyModule_AddStringConstant(model_module, "__doc__",
                     "Constants used to interpret content model information.");

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

    /* initialize pyexpat dispatch table */
    capi.size = sizeof(capi);
    capi.magic = PyExpat_CAPI_MAGIC;
    capi.MAJOR_VERSION = XML_MAJOR_VERSION;
    capi.MINOR_VERSION = XML_MINOR_VERSION;
    capi.MICRO_VERSION = XML_MICRO_VERSION;
    capi.ErrorString = XML_ErrorString;
    capi.GetErrorCode = XML_GetErrorCode;
    capi.GetErrorColumnNumber = XML_GetErrorColumnNumber;
    capi.GetErrorLineNumber = XML_GetErrorLineNumber;
    capi.Parse = XML_Parse;
    capi.ParserCreate_MM = XML_ParserCreate_MM;
    capi.ParserFree = XML_ParserFree;
    capi.SetCharacterDataHandler = XML_SetCharacterDataHandler;
    capi.SetCommentHandler = XML_SetCommentHandler;
    capi.SetDefaultHandlerExpand = XML_SetDefaultHandlerExpand;
    capi.SetElementHandler = XML_SetElementHandler;
    capi.SetNamespaceDeclHandler = XML_SetNamespaceDeclHandler;
    capi.SetProcessingInstructionHandler = XML_SetProcessingInstructionHandler;
    capi.SetUnknownEncodingHandler = XML_SetUnknownEncodingHandler;
    capi.SetUserData = XML_SetUserData;

    /* export using capsule */
    capi_object = PyCapsule_New(&capi, PyExpat_CAPSULE_NAME, NULL);
    if (capi_object)
        PyModule_AddObject(m, "expat_CAPI", capi_object);
    return m;
}

static void
clear_handlers(xmlparseobject *self, int initial)
{
    int i = 0;
    PyObject *temp;

    for (; handler_info[i].name != NULL; i++) {
        if (initial)
            self->handlers[i] = NULL;
        else {
            temp = self->handlers[i];
            self->handlers[i] = NULL;
            Py_XDECREF(temp);
            handler_info[i].setter(self->itself, NULL);
        }
    }
}

static struct HandlerInfo handler_info[] = {
    {"StartElementHandler",
     (xmlhandlersetter)XML_SetStartElementHandler,
     (xmlhandler)my_StartElementHandler},
    {"EndElementHandler",
     (xmlhandlersetter)XML_SetEndElementHandler,
     (xmlhandler)my_EndElementHandler},
    {"ProcessingInstructionHandler",
     (xmlhandlersetter)XML_SetProcessingInstructionHandler,
     (xmlhandler)my_ProcessingInstructionHandler},
    {"CharacterDataHandler",
     (xmlhandlersetter)XML_SetCharacterDataHandler,
     (xmlhandler)my_CharacterDataHandler},
    {"UnparsedEntityDeclHandler",
     (xmlhandlersetter)XML_SetUnparsedEntityDeclHandler,
     (xmlhandler)my_UnparsedEntityDeclHandler},
    {"NotationDeclHandler",
     (xmlhandlersetter)XML_SetNotationDeclHandler,
     (xmlhandler)my_NotationDeclHandler},
    {"StartNamespaceDeclHandler",
     (xmlhandlersetter)XML_SetStartNamespaceDeclHandler,
     (xmlhandler)my_StartNamespaceDeclHandler},
    {"EndNamespaceDeclHandler",
     (xmlhandlersetter)XML_SetEndNamespaceDeclHandler,
     (xmlhandler)my_EndNamespaceDeclHandler},
    {"CommentHandler",
     (xmlhandlersetter)XML_SetCommentHandler,
     (xmlhandler)my_CommentHandler},
    {"StartCdataSectionHandler",
     (xmlhandlersetter)XML_SetStartCdataSectionHandler,
     (xmlhandler)my_StartCdataSectionHandler},
    {"EndCdataSectionHandler",
     (xmlhandlersetter)XML_SetEndCdataSectionHandler,
     (xmlhandler)my_EndCdataSectionHandler},
    {"DefaultHandler",
     (xmlhandlersetter)XML_SetDefaultHandler,
     (xmlhandler)my_DefaultHandler},
    {"DefaultHandlerExpand",
     (xmlhandlersetter)XML_SetDefaultHandlerExpand,
     (xmlhandler)my_DefaultHandlerExpandHandler},
    {"NotStandaloneHandler",
     (xmlhandlersetter)XML_SetNotStandaloneHandler,
     (xmlhandler)my_NotStandaloneHandler},
    {"ExternalEntityRefHandler",
     (xmlhandlersetter)XML_SetExternalEntityRefHandler,
     (xmlhandler)my_ExternalEntityRefHandler},
    {"StartDoctypeDeclHandler",
     (xmlhandlersetter)XML_SetStartDoctypeDeclHandler,
     (xmlhandler)my_StartDoctypeDeclHandler},
    {"EndDoctypeDeclHandler",
     (xmlhandlersetter)XML_SetEndDoctypeDeclHandler,
     (xmlhandler)my_EndDoctypeDeclHandler},
    {"EntityDeclHandler",
     (xmlhandlersetter)XML_SetEntityDeclHandler,
     (xmlhandler)my_EntityDeclHandler},
    {"XmlDeclHandler",
     (xmlhandlersetter)XML_SetXmlDeclHandler,
     (xmlhandler)my_XmlDeclHandler},
    {"ElementDeclHandler",
     (xmlhandlersetter)XML_SetElementDeclHandler,
     (xmlhandler)my_ElementDeclHandler},
    {"AttlistDeclHandler",
     (xmlhandlersetter)XML_SetAttlistDeclHandler,
     (xmlhandler)my_AttlistDeclHandler},
#if XML_COMBINED_VERSION >= 19504
    {"SkippedEntityHandler",
     (xmlhandlersetter)XML_SetSkippedEntityHandler,
     (xmlhandler)my_SkippedEntityHandler},
#endif

    {NULL, NULL, NULL} /* sentinel */
};
