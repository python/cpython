
typedef void *(*gs_fetch_addr_fn)(void);

typedef struct {
    PyObject_HEAD

    PyObject         *gs_name;
    CTypeDescrObject *gs_type;
    char             *gs_data;
    gs_fetch_addr_fn  gs_fetch_addr;

} GlobSupportObject;

static void glob_support_dealloc(GlobSupportObject *gs)
{
    Py_DECREF(gs->gs_name);
    Py_DECREF(gs->gs_type);
    PyObject_Del(gs);
}

static PyTypeObject GlobSupport_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_cffi_backend.__FFIGlobSupport",
    sizeof(GlobSupportObject),
    0,
    (destructor)glob_support_dealloc,           /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_compare */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                         /* tp_flags */
};

#define GlobSupport_Check(ob)  (Py_TYPE(ob) == &GlobSupport_Type)

static PyObject *make_global_var(PyObject *name, CTypeDescrObject *type,
                                 char *addr, gs_fetch_addr_fn fetch_addr)
{
    GlobSupportObject *gs = PyObject_New(GlobSupportObject, &GlobSupport_Type);
    if (gs == NULL)
        return NULL;

    Py_INCREF(name);
    Py_INCREF(type);
    gs->gs_name = name;
    gs->gs_type = type;
    gs->gs_data = addr;
    gs->gs_fetch_addr = fetch_addr;
    return (PyObject *)gs;
}

static void *fetch_global_var_addr(GlobSupportObject *gs)
{
    void *data;
    if (gs->gs_data != NULL) {
        data = gs->gs_data;
    }
    else {
        Py_BEGIN_ALLOW_THREADS
        restore_errno();
        data = gs->gs_fetch_addr();
        save_errno();
        Py_END_ALLOW_THREADS
    }
    if (data == NULL) {
        PyErr_Format(FFIError, "global variable '%s' is at address NULL",
                     PyText_AS_UTF8(gs->gs_name));
        return NULL;
    }
    return data;
}

static PyObject *read_global_var(GlobSupportObject *gs)
{
    void *data = fetch_global_var_addr(gs);
    if (data == NULL)
        return NULL;
    return convert_to_object(data, gs->gs_type);
}

static int write_global_var(GlobSupportObject *gs, PyObject *obj)
{
    void *data = fetch_global_var_addr(gs);
    if (data == NULL)
        return -1;
    return convert_from_object(data, gs->gs_type, obj);
}

static PyObject *cg_addressof_global_var(GlobSupportObject *gs)
{
    void *data;
    PyObject *x, *ptrtype = new_pointer_type(gs->gs_type);
    if (ptrtype == NULL)
        return NULL;

    data = fetch_global_var_addr(gs);
    if (data != NULL)
        x = new_simple_cdata(data, (CTypeDescrObject *)ptrtype);
    else
        x = NULL;
    Py_DECREF(ptrtype);
    return x;
}
