
/* An FFI object has methods like ffi.new().  It is also a container
   for the type declarations (typedefs and structs) that you can use,
   say in ffi.new().

   CTypeDescrObjects are internally stored in the dict 'types_dict'.
   The types_dict is lazily filled with CTypeDescrObjects made from
   reading a _cffi_type_context_s structure.

   In "modern" mode, the FFI instance is made by the C extension
   module originally created by recompile().  The _cffi_type_context_s
   structure comes from global data in the C extension module.

   In "compatibility" mode, an FFI instance is created explicitly by
   the user, and its _cffi_type_context_s is initially empty.  You
   need to call ffi.cdef() to add more information to it.
*/

#define FFI_COMPLEXITY_OUTPUT   1200     /* xxx should grow as needed */

#define FFIObject_Check(op) PyObject_TypeCheck(op, &FFI_Type)
#define LibObject_Check(ob)  ((Py_TYPE(ob) == &Lib_Type))

struct FFIObject_s {
    PyObject_HEAD
    PyObject *gc_wrefs, *gc_wrefs_freelist;
    PyObject *init_once_cache;
    struct _cffi_parse_info_s info;
    char ctx_is_static, ctx_is_nonempty;
    builder_c_t types_builder;
};

static FFIObject *ffi_internal_new(PyTypeObject *ffitype,
                                 const struct _cffi_type_context_s *static_ctx)
{
    static _cffi_opcode_t internal_output[FFI_COMPLEXITY_OUTPUT];

    FFIObject *ffi;
    if (static_ctx != NULL) {
        ffi = (FFIObject *)PyObject_GC_New(FFIObject, ffitype);
        /* we don't call PyObject_GC_Track() here: from _cffi_init_module()
           it is not needed, because in this case the ffi object is immortal */
    }
    else {
        ffi = (FFIObject *)ffitype->tp_alloc(ffitype, 0);
    }
    if (ffi == NULL)
        return NULL;

    if (init_builder_c(&ffi->types_builder, static_ctx) < 0) {
        Py_DECREF(ffi);
        return NULL;
    }
    ffi->gc_wrefs = NULL;
    ffi->gc_wrefs_freelist = NULL;
    ffi->init_once_cache = NULL;
    ffi->info.ctx = &ffi->types_builder.ctx;
    ffi->info.output = internal_output;
    ffi->info.output_size = FFI_COMPLEXITY_OUTPUT;
    ffi->ctx_is_static = (static_ctx != NULL);
    ffi->ctx_is_nonempty = (static_ctx != NULL);
    return ffi;
}

static void ffi_dealloc(FFIObject *ffi)
{
    PyObject_GC_UnTrack(ffi);
    Py_XDECREF(ffi->gc_wrefs);
    Py_XDECREF(ffi->gc_wrefs_freelist);
    Py_XDECREF(ffi->init_once_cache);

    free_builder_c(&ffi->types_builder, ffi->ctx_is_static);

    Py_TYPE(ffi)->tp_free((PyObject *)ffi);
}

static int ffi_traverse(FFIObject *ffi, visitproc visit, void *arg)
{
    Py_VISIT(ffi->types_builder.types_dict);
    Py_VISIT(ffi->types_builder.included_ffis);
    Py_VISIT(ffi->types_builder.included_libs);
    Py_VISIT(ffi->gc_wrefs);
    return 0;
}

static PyObject *ffiobj_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    /* user-facing initialization code, for explicit FFI() calls */
    return (PyObject *)ffi_internal_new(type, NULL);
}

/* forward, declared in cdlopen.c because it's mostly useful for this case */
static int ffiobj_init(PyObject *self, PyObject *args, PyObject *kwds);

static PyObject *ffi_fetch_int_constant(FFIObject *ffi, const char *name,
                                        int recursion)
{
    int index;

    index = search_in_globals(&ffi->types_builder.ctx, name, strlen(name));
    if (index >= 0) {
        const struct _cffi_global_s *g;
        g = &ffi->types_builder.ctx.globals[index];

        switch (_CFFI_GETOP(g->type_op)) {
        case _CFFI_OP_CONSTANT_INT:
        case _CFFI_OP_ENUM:
            return realize_global_int(&ffi->types_builder, index);

        default:
            PyErr_Format(FFIError,
                         "function, global variable or non-integer constant "
                         "'%.200s' must be fetched from its original 'lib' "
                         "object", name);
            return NULL;
        }
    }

    if (ffi->types_builder.included_ffis != NULL) {
        Py_ssize_t i;
        PyObject *included_ffis = ffi->types_builder.included_ffis;

        if (recursion > 100) {
            PyErr_SetString(PyExc_RuntimeError,
                            "recursion overflow in ffi.include() delegations");
            return NULL;
        }

        for (i = 0; i < PyTuple_GET_SIZE(included_ffis); i++) {
            FFIObject *ffi1;
            PyObject *x;

            ffi1 = (FFIObject *)PyTuple_GET_ITEM(included_ffis, i);
            x = ffi_fetch_int_constant(ffi1, name, recursion + 1);
            if (x != NULL || PyErr_Occurred())
                return x;
        }
    }
    return NULL;     /* no exception set, means "not found" */
}

#define ACCEPT_STRING   1
#define ACCEPT_CTYPE    2
#define ACCEPT_CDATA    4
#define ACCEPT_ALL      (ACCEPT_STRING | ACCEPT_CTYPE | ACCEPT_CDATA)
#define CONSIDER_FN_AS_FNPTR  8

static CTypeDescrObject *_ffi_bad_type(FFIObject *ffi, const char *input_text)
{
    size_t length = strlen(input_text);
    char *extra;

    if (length > 500) {
        extra = "";
    }
    else {
        char *p;
        size_t i, num_spaces = ffi->info.error_location;
        extra = alloca(length + num_spaces + 4);
        p = extra;
        *p++ = '\n';
        for (i = 0; i < length; i++) {
            if (' ' <= input_text[i] && input_text[i] < 0x7f)
                *p++ = input_text[i];
            else if (input_text[i] == '\t' || input_text[i] == '\n')
                *p++ = ' ';
            else
                *p++ = '?';
        }
        *p++ = '\n';
        memset(p, ' ', num_spaces);
        p += num_spaces;
        *p++ = '^';
        *p++ = 0;
    }
    PyErr_Format(FFIError, "%s%s", ffi->info.error_message, extra);
    return NULL;
}

static CTypeDescrObject *_ffi_type(FFIObject *ffi, PyObject *arg,
                                   int accept)
{
    /* Returns the CTypeDescrObject from the user-supplied 'arg'.
       Does not return a new reference!
    */
    if ((accept & ACCEPT_STRING) && PyText_Check(arg)) {
        PyObject *types_dict = ffi->types_builder.types_dict;
        PyObject *x = PyDict_GetItem(types_dict, arg);

        if (x == NULL) {
            const char *input_text = PyText_AS_UTF8(arg);
            int err, index = parse_c_type(&ffi->info, input_text);
            if (index < 0)
                return _ffi_bad_type(ffi, input_text);

            x = realize_c_type_or_func(&ffi->types_builder,
                                       ffi->info.output, index);
            if (x == NULL)
                return NULL;

            /* Cache under the name given by 'arg', in addition to the
               fact that the same ct is probably already cached under
               its standardized name.  In a few cases, it is not, e.g.
               if it is a primitive; for the purpose of this function,
               the important point is the following line, which makes
               sure that in any case the next _ffi_type() with the same
               'arg' will succeed early, in PyDict_GetItem() above.
            */
            err = PyDict_SetItem(types_dict, arg, x);
            Py_DECREF(x); /* we know it was written in types_dict (unless out
                             of mem), so there is at least that ref left */
            if (err < 0)
                return NULL;
        }

        if (CTypeDescr_Check(x))
            return (CTypeDescrObject *)x;
        else if (accept & CONSIDER_FN_AS_FNPTR)
            return unwrap_fn_as_fnptr(x);
        else
            return unexpected_fn_type(x);
    }
    else if ((accept & ACCEPT_CTYPE) && CTypeDescr_Check(arg)) {
        return (CTypeDescrObject *)arg;
    }
    else if ((accept & ACCEPT_CDATA) && CData_Check(arg)) {
        return ((CDataObject *)arg)->c_type;
    }
#if PY_MAJOR_VERSION < 3
    else if (PyUnicode_Check(arg)) {
        CTypeDescrObject *result;
        arg = PyUnicode_AsASCIIString(arg);
        if (arg == NULL)
            return NULL;
        result = _ffi_type(ffi, arg, accept);
        Py_DECREF(arg);
        return result;
    }
#endif
    else {
        const char *m1 = (accept & ACCEPT_STRING) ? "string" : "";
        const char *m2 = (accept & ACCEPT_CTYPE) ? "ctype object" : "";
        const char *m3 = (accept & ACCEPT_CDATA) ? "cdata object" : "";
        const char *s12 = (*m1 && (*m2 || *m3)) ? " or " : "";
        const char *s23 = (*m2 && *m3) ? " or " : "";
        PyErr_Format(PyExc_TypeError, "expected a %s%s%s%s%s, got '%.200s'",
                     m1, s12, m2, s23, m3,
                     Py_TYPE(arg)->tp_name);
        return NULL;
    }
}

PyDoc_STRVAR(ffi_sizeof_doc,
"Return the size in bytes of the argument.\n"
"It can be a string naming a C type, or a 'cdata' instance.");

static PyObject *ffi_sizeof(FFIObject *self, PyObject *arg)
{
    Py_ssize_t size;

    if (CData_Check(arg)) {
        size = direct_sizeof_cdata((CDataObject *)arg);
    }
    else {
        CTypeDescrObject *ct = _ffi_type(self, arg, ACCEPT_ALL);
        if (ct == NULL)
            return NULL;
        size = ct->ct_size;
        if (size < 0) {
            PyErr_Format(FFIError, "don't know the size of ctype '%s'",
                         ct->ct_name);
            return NULL;
        }
    }
    return PyInt_FromSsize_t(size);
}

PyDoc_STRVAR(ffi_alignof_doc,
"Return the natural alignment size in bytes of the argument.\n"
"It can be a string naming a C type, or a 'cdata' instance.");

static PyObject *ffi_alignof(FFIObject *self, PyObject *arg)
{
    int align;
    CTypeDescrObject *ct = _ffi_type(self, arg, ACCEPT_ALL);
    if (ct == NULL)
        return NULL;

    align = get_alignment(ct);
    if (align < 0)
        return NULL;
    return PyInt_FromLong(align);
}

PyDoc_STRVAR(ffi_typeof_doc,
"Parse the C type given as a string and return the\n"
"corresponding <ctype> object.\n"
"It can also be used on 'cdata' instance to get its C type.");

static PyObject *_cpyextfunc_type_index(PyObject *x);  /* forward */

static PyObject *ffi_typeof(FFIObject *self, PyObject *arg)
{
    PyObject *x = (PyObject *)_ffi_type(self, arg, ACCEPT_STRING|ACCEPT_CDATA);
    if (x != NULL) {
        Py_INCREF(x);
    }
    else {
        x = _cpyextfunc_type_index(arg);
    }
    return x;
}

PyDoc_STRVAR(ffi_new_doc,
"Allocate an instance according to the specified C type and return a\n"
"pointer to it.  The specified C type must be either a pointer or an\n"
"array: ``new('X *')`` allocates an X and returns a pointer to it,\n"
"whereas ``new('X[n]')`` allocates an array of n X'es and returns an\n"
"array referencing it (which works mostly like a pointer, like in C).\n"
"You can also use ``new('X[]', n)`` to allocate an array of a\n"
"non-constant length n.\n"
"\n"
"The memory is initialized following the rules of declaring a global\n"
"variable in C: by default it is zero-initialized, but an explicit\n"
"initializer can be given which can be used to fill all or part of the\n"
"memory.\n"
"\n"
"When the returned <cdata> object goes out of scope, the memory is\n"
"freed.  In other words the returned <cdata> object has ownership of\n"
"the value of type 'cdecl' that it points to.  This means that the raw\n"
"data can be used as long as this object is kept alive, but must not be\n"
"used for a longer time.  Be careful about that when copying the\n"
"pointer to the memory somewhere else, e.g. into another structure.");

static PyObject *_ffi_new(FFIObject *self, PyObject *args, PyObject *kwds,
                          const cffi_allocator_t *allocator)
{
    CTypeDescrObject *ct;
    PyObject *arg, *init = Py_None;
    static char *keywords[] = {"cdecl", "init", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O:new", keywords,
                                     &arg, &init))
        return NULL;

    ct = _ffi_type(self, arg, ACCEPT_STRING|ACCEPT_CTYPE);
    if (ct == NULL)
        return NULL;

    return direct_newp(ct, init, allocator);
}

static PyObject *ffi_new(FFIObject *self, PyObject *args, PyObject *kwds)
{
    return _ffi_new(self, args, kwds, &default_allocator);
}

static PyObject *_ffi_new_with_allocator(PyObject *allocator, PyObject *args,
                                         PyObject *kwds)
{
    cffi_allocator_t alloc1;
    PyObject *my_alloc, *my_free;
    my_alloc = PyTuple_GET_ITEM(allocator, 1);
    my_free  = PyTuple_GET_ITEM(allocator, 2);
    alloc1.ca_alloc = (my_alloc == Py_None ? NULL : my_alloc);
    alloc1.ca_free  = (my_free  == Py_None ? NULL : my_free);
    alloc1.ca_dont_clear = (PyTuple_GET_ITEM(allocator, 3) == Py_False);

    return _ffi_new((FFIObject *)PyTuple_GET_ITEM(allocator, 0),
                    args, kwds, &alloc1);
}

PyDoc_STRVAR(ffi_new_allocator_doc,
"Return a new allocator, i.e. a function that behaves like ffi.new()\n"
"but uses the provided low-level 'alloc' and 'free' functions.\n"
"\n"
"'alloc' is called with the size as argument.  If it returns NULL, a\n"
"MemoryError is raised.  'free' is called with the result of 'alloc'\n"
"as argument.  Both can be either Python functions or directly C\n"
"functions.  If 'free' is None, then no free function is called.\n"
"If both 'alloc' and 'free' are None, the default is used.\n"
"\n"
"If 'should_clear_after_alloc' is set to False, then the memory\n"
"returned by 'alloc' is assumed to be already cleared (or you are\n"
"fine with garbage); otherwise CFFI will clear it.");

static PyObject *ffi_new_allocator(FFIObject *self, PyObject *args,
                                   PyObject *kwds)
{
    PyObject *allocator, *result;
    PyObject *my_alloc = Py_None, *my_free = Py_None;
    int should_clear_after_alloc = 1;
    static char *keywords[] = {"alloc", "free", "should_clear_after_alloc",
                               NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOi:new_allocator", keywords,
                                     &my_alloc, &my_free,
                                     &should_clear_after_alloc))
        return NULL;

    if (my_alloc == Py_None && my_free != Py_None) {
        PyErr_SetString(PyExc_TypeError, "cannot pass 'free' without 'alloc'");
        return NULL;
    }

    allocator = PyTuple_Pack(4,
                             (PyObject *)self,
                             my_alloc,
                             my_free,
                             PyBool_FromLong(should_clear_after_alloc));
    if (allocator == NULL)
        return NULL;

    {
        static PyMethodDef md = {"allocator",
                                 (PyCFunction)_ffi_new_with_allocator,
                                 METH_VARARGS | METH_KEYWORDS};
        result = PyCFunction_New(&md, allocator);
    }
    Py_DECREF(allocator);
    return result;
}

PyDoc_STRVAR(ffi_cast_doc,
"Similar to a C cast: returns an instance of the named C\n"
"type initialized with the given 'source'.  The source is\n"
"casted between integers or pointers of any type.");

static PyObject *ffi_cast(FFIObject *self, PyObject *args)
{
    CTypeDescrObject *ct;
    PyObject *ob, *arg;
    if (!PyArg_ParseTuple(args, "OO:cast", &arg, &ob))
        return NULL;

    ct = _ffi_type(self, arg, ACCEPT_STRING|ACCEPT_CTYPE);
    if (ct == NULL)
        return NULL;

    return do_cast(ct, ob);
}

PyDoc_STRVAR(ffi_string_doc,
"Return a Python string (or unicode string) from the 'cdata'.  If\n"
"'cdata' is a pointer or array of characters or bytes, returns the\n"
"null-terminated string.  The returned string extends until the first\n"
"null character, or at most 'maxlen' characters.  If 'cdata' is an\n"
"array then 'maxlen' defaults to its length.\n"
"\n"
"If 'cdata' is a pointer or array of wchar_t, returns a unicode string\n"
"following the same rules.\n"
"\n"
"If 'cdata' is a single character or byte or a wchar_t, returns it as a\n"
"string or unicode string.\n"
"\n"
"If 'cdata' is an enum, returns the value of the enumerator as a\n"
"string, or 'NUMBER' if the value is out of range.");

#define ffi_string  b_string     /* ffi_string() => b_string()
                                    from _cffi_backend.c */

PyDoc_STRVAR(ffi_unpack_doc,
"Unpack an array of C data of the given length,\n"
"returning a Python string/unicode/list.\n"
"\n"
"If 'cdata' is a pointer to 'char', returns a byte string.\n"
"It does not stop at the first null.  This is equivalent to:\n"
"ffi.buffer(cdata, length)[:]\n"
"\n"
"If 'cdata' is a pointer to 'wchar_t', returns a unicode string.\n"
"'length' is measured in wchar_t's; it is not the size in bytes.\n"
"\n"
"If 'cdata' is a pointer to anything else, returns a list of\n"
"'length' items.  This is a faster equivalent to:\n"
"[cdata[i] for i in range(length)]");

#define ffi_unpack  b_unpack     /* ffi_unpack() => b_unpack()
                                    from _cffi_backend.c */


PyDoc_STRVAR(ffi_offsetof_doc,
"Return the offset of the named field inside the given structure or\n"
"array, which must be given as a C type name.  You can give several\n"
"field names in case of nested structures.  You can also give numeric\n"
"values which correspond to array items, in case of an array type.");

static PyObject *ffi_offsetof(FFIObject *self, PyObject *args)
{
    PyObject *arg;
    CTypeDescrObject *ct;
    Py_ssize_t i, offset;

    if (PyTuple_Size(args) < 2) {
        PyErr_SetString(PyExc_TypeError,
                        "offsetof() expects at least 2 arguments");
        return NULL;
    }

    arg = PyTuple_GET_ITEM(args, 0);
    ct = _ffi_type(self, arg, ACCEPT_STRING|ACCEPT_CTYPE);
    if (ct == NULL)
        return NULL;

    offset = 0;
    for (i = 1; i < PyTuple_GET_SIZE(args); i++) {
        Py_ssize_t ofs1;
        ct = direct_typeoffsetof(ct, PyTuple_GET_ITEM(args, i), i > 1, &ofs1);
        if (ct == NULL)
            return NULL;
        offset += ofs1;
    }
    return PyInt_FromSsize_t(offset);
}

PyDoc_STRVAR(ffi_addressof_doc,
"Limited equivalent to the '&' operator in C:\n"
"\n"
"1. ffi.addressof(<cdata 'struct-or-union'>) returns a cdata that is a\n"
"pointer to this struct or union.\n"
"\n"
"2. ffi.addressof(<cdata>, field-or-index...) returns the address of a\n"
"field or array item inside the given structure or array, recursively\n"
"in case of nested structures or arrays.\n"
"\n"
"3. ffi.addressof(<library>, \"name\") returns the address of the named\n"
"function or global variable.");

static PyObject *address_of_global_var(PyObject *args);  /* forward */

static PyObject *ffi_addressof(FFIObject *self, PyObject *args)
{
    PyObject *arg, *z, *result;
    CTypeDescrObject *ct;
    Py_ssize_t i, offset = 0;
    int accepted_flags;

    if (PyTuple_Size(args) < 1) {
        PyErr_SetString(PyExc_TypeError,
                        "addressof() expects at least 1 argument");
        return NULL;
    }

    arg = PyTuple_GET_ITEM(args, 0);
    if (LibObject_Check(arg)) {
        /* case 3 in the docstring */
        return address_of_global_var(args);
    }

    ct = _ffi_type(self, arg, ACCEPT_CDATA);
    if (ct == NULL)
        return NULL;

    if (PyTuple_GET_SIZE(args) == 1) {
        /* case 1 in the docstring */
        accepted_flags = CT_STRUCT | CT_UNION | CT_ARRAY;
        if ((ct->ct_flags & accepted_flags) == 0) {
            PyErr_SetString(PyExc_TypeError,
                            "expected a cdata struct/union/array object");
            return NULL;
        }
    }
    else {
        /* case 2 in the docstring */
        accepted_flags = CT_STRUCT | CT_UNION | CT_ARRAY | CT_POINTER;
        if ((ct->ct_flags & accepted_flags) == 0) {
            PyErr_SetString(PyExc_TypeError,
                        "expected a cdata struct/union/array/pointer object");
            return NULL;
        }
        for (i = 1; i < PyTuple_GET_SIZE(args); i++) {
            Py_ssize_t ofs1;
            ct = direct_typeoffsetof(ct, PyTuple_GET_ITEM(args, i),
                                     i > 1, &ofs1);
            if (ct == NULL)
                return NULL;
            offset += ofs1;
        }
    }

    z = new_pointer_type(ct);
    if (z == NULL)
        return NULL;

    result = new_simple_cdata(((CDataObject *)arg)->c_data + offset,
                              (CTypeDescrObject *)z);
    Py_DECREF(z);
    return result;
}

static PyObject *_combine_type_name_l(CTypeDescrObject *ct,
                                      size_t extra_text_len)
{
    size_t base_name_len;
    PyObject *result;
    char *p;

    base_name_len = strlen(ct->ct_name);
    result = PyBytes_FromStringAndSize(NULL, base_name_len + extra_text_len);
    if (result == NULL)
        return NULL;

    p = PyBytes_AS_STRING(result);
    memcpy(p, ct->ct_name, ct->ct_name_position);
    p += ct->ct_name_position;
    p += extra_text_len;
    memcpy(p, ct->ct_name + ct->ct_name_position,
           base_name_len - ct->ct_name_position);
    return result;
}

PyDoc_STRVAR(ffi_getctype_doc,
"Return a string giving the C type 'cdecl', which may be itself a\n"
"string or a <ctype> object.  If 'replace_with' is given, it gives\n"
"extra text to append (or insert for more complicated C types), like a\n"
"variable name, or '*' to get actually the C type 'pointer-to-cdecl'.");

static PyObject *ffi_getctype(FFIObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *c_decl, *res;
    char *p, *replace_with = "";
    int add_paren, add_space;
    CTypeDescrObject *ct;
    size_t replace_with_len;
    static char *keywords[] = {"cdecl", "replace_with", NULL};
#if PY_MAJOR_VERSION >= 3
    PyObject *u;
#endif

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|s:getctype", keywords,
                                     &c_decl, &replace_with))
        return NULL;

    ct = _ffi_type(self, c_decl, ACCEPT_STRING|ACCEPT_CTYPE);
    if (ct == NULL)
        return NULL;

    while (replace_with[0] != 0 && isspace(replace_with[0]))
        replace_with++;
    replace_with_len = strlen(replace_with);
    while (replace_with_len > 0 && isspace(replace_with[replace_with_len - 1]))
        replace_with_len--;

    add_paren = (replace_with[0] == '*' &&
                 ((ct->ct_flags & CT_ARRAY) != 0));
    add_space = (!add_paren && replace_with_len > 0 &&
                 replace_with[0] != '[' && replace_with[0] != '(');

    res = _combine_type_name_l(ct, replace_with_len + add_space + 2*add_paren);
    if (res == NULL)
        return NULL;

    p = PyBytes_AS_STRING(res) + ct->ct_name_position;
    if (add_paren)
        *p++ = '(';
    if (add_space)
        *p++ = ' ';
    memcpy(p, replace_with, replace_with_len);
    if (add_paren)
        p[replace_with_len] = ')';

#if PY_MAJOR_VERSION >= 3
    /* bytes -> unicode string */
    u = PyUnicode_DecodeLatin1(PyBytes_AS_STRING(res),
                               PyBytes_GET_SIZE(res),
                               NULL);
    Py_DECREF(res);
    res = u;
#endif

    return res;
}

PyDoc_STRVAR(ffi_new_handle_doc,
"Return a non-NULL cdata of type 'void *' that contains an opaque\n"
"reference to the argument, which can be any Python object.  To cast it\n"
"back to the original object, use from_handle().  You must keep alive\n"
"the cdata object returned by new_handle()!");

static PyObject *ffi_new_handle(FFIObject *self, PyObject *arg)
{
    /* g_ct_voidp is equal to <ctype 'void *'> */
    return newp_handle(g_ct_voidp, arg);
}

PyDoc_STRVAR(ffi_from_handle_doc,
"Cast a 'void *' back to a Python object.  Must be used *only* on the\n"
"pointers returned by new_handle(), and *only* as long as the exact\n"
"cdata object returned by new_handle() is still alive (somewhere else\n"
"in the program).  Failure to follow these rules will crash.");

#define ffi_from_handle  b_from_handle   /* ffi_from_handle => b_from_handle
                                            from _cffi_backend.c */

PyDoc_STRVAR(ffi_from_buffer_doc,
"Return a <cdata 'char[]'> that points to the data of the given Python\n"
"object, which must support the buffer interface.  Note that this is\n"
"not meant to be used on the built-in types str or unicode\n"
"(you can build 'char[]' arrays explicitly) but only on objects\n"
"containing large quantities of raw data in some other format, like\n"
"'array.array' or numpy arrays.");

static PyObject *ffi_from_buffer(FFIObject *self, PyObject *args,
                                 PyObject *kwds)
{
    PyObject *cdecl1, *python_buf = NULL;
    CTypeDescrObject *ct;
    int require_writable = 0;
    static char *keywords[] = {"cdecl", "python_buffer",
                               "require_writable", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|Oi:from_buffer", keywords,
                                     &cdecl1, &python_buf, &require_writable))
        return NULL;

    if (python_buf == NULL) {
        python_buf = cdecl1;
        ct = g_ct_chararray;
    }
    else {
        ct = _ffi_type(self, cdecl1, ACCEPT_STRING|ACCEPT_CTYPE);
        if (ct == NULL)
            return NULL;
    }
    return direct_from_buffer(ct, python_buf, require_writable);
}

PyDoc_STRVAR(ffi_gc_doc,
"Return a new cdata object that points to the same data.\n"
"Later, when this new cdata object is garbage-collected,\n"
"'destructor(old_cdata_object)' will be called.\n"
"\n"
"The optional 'size' gives an estimate of the size, used to\n"
"trigger the garbage collection more eagerly.  So far only used\n"
"on PyPy.  It tells the GC that the returned object keeps alive\n"
"roughly 'size' bytes of external memory.");

#define ffi_gc  b_gcp     /* ffi_gc() => b_gcp()
                             from _cffi_backend.c */

PyDoc_STRVAR(ffi_def_extern_doc,
"A decorator.  Attaches the decorated Python function to the C code\n"
"generated for the 'extern \"Python\"' function of the same name.\n"
"Calling the C function will then invoke the Python function.\n"
"\n"
"Optional arguments: 'name' is the name of the C function, if\n"
"different from the Python function; and 'error' and 'onerror'\n"
"handle what occurs if the Python function raises an exception\n"
"(see the docs for details).");

/* forward; see call_python.c */
static PyObject *_ffi_def_extern_decorator(PyObject *, PyObject *);

static PyObject *ffi_def_extern(FFIObject *self, PyObject *args,
                                PyObject *kwds)
{
    static PyMethodDef md = {"def_extern_decorator",
                             (PyCFunction)_ffi_def_extern_decorator, METH_O};
    PyObject *name = Py_None, *error = Py_None;
    PyObject *res, *onerror = Py_None;
    static char *keywords[] = {"name", "error", "onerror", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOO", keywords,
                                     &name, &error, &onerror))
        return NULL;

    args = Py_BuildValue("(OOOO)", (PyObject *)self, name, error, onerror);
    if (args == NULL)
        return NULL;

    res = PyCFunction_New(&md, args);
    Py_DECREF(args);
    return res;
}

PyDoc_STRVAR(ffi_callback_doc,
"Return a callback object or a decorator making such a callback object.\n"
"'cdecl' must name a C function pointer type.  The callback invokes the\n"
"specified 'python_callable' (which may be provided either directly or\n"
"via a decorator).  Important: the callback object must be manually\n"
"kept alive for as long as the callback may be invoked from the C code.");

static PyObject *_ffi_callback_decorator(PyObject *outer_args, PyObject *fn)
{
    PyObject *res, *old;

    old = PyTuple_GET_ITEM(outer_args, 1);
    PyTuple_SET_ITEM(outer_args, 1, fn);
    res = b_callback(NULL, outer_args);
    PyTuple_SET_ITEM(outer_args, 1, old);
    return res;
}

static PyObject *ffi_callback(FFIObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *c_decl, *python_callable = Py_None, *error = Py_None;
    PyObject *res, *onerror = Py_None;
    static char *keywords[] = {"cdecl", "python_callable", "error",
                               "onerror", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO", keywords,
                                     &c_decl, &python_callable, &error,
                                     &onerror))
        return NULL;

    c_decl = (PyObject *)_ffi_type(self, c_decl, ACCEPT_STRING | ACCEPT_CTYPE |
                                                 CONSIDER_FN_AS_FNPTR);
    if (c_decl == NULL)
        return NULL;

    args = Py_BuildValue("(OOOO)", c_decl, python_callable, error, onerror);
    if (args == NULL)
        return NULL;

    if (python_callable != Py_None) {
        res = b_callback(NULL, args);
    }
    else {
        static PyMethodDef md = {"callback_decorator",
                                 (PyCFunction)_ffi_callback_decorator, METH_O};
        res = PyCFunction_New(&md, args);
    }
    Py_DECREF(args);
    return res;
}

#ifdef MS_WIN32
PyDoc_STRVAR(ffi_getwinerror_doc,
"Return either the GetLastError() or the error number given by the\n"
"optional 'code' argument, as a tuple '(code, message)'.");

#define ffi_getwinerror  b_getwinerror  /* ffi_getwinerror() => b_getwinerror()
                                           from misc_win32.h */
#endif

PyDoc_STRVAR(ffi_errno_doc, "the value of 'errno' from/to the C calls");

static PyObject *ffi_get_errno(PyObject *self, void *closure)
{
    /* xxx maybe think about how to make the saved errno local
       to an ffi instance */
    return b_get_errno(NULL, NULL);
}

static int ffi_set_errno(PyObject *self, PyObject *newval, void *closure)
{
    PyObject *x = b_set_errno(NULL, newval);
    if (x == NULL)
        return -1;
    Py_DECREF(x);
    return 0;
}

PyDoc_STRVAR(ffi_dlopen_doc,
"Load and return a dynamic library identified by 'name'.  The standard\n"
"C library can be loaded by passing None.\n"
"\n"
"Note that functions and types declared with 'ffi.cdef()' are not\n"
"linked to a particular library, just like C headers.  In the library\n"
"we only look for the actual (untyped) symbols at the time of their\n"
"first access.");

PyDoc_STRVAR(ffi_dlclose_doc,
"Close a library obtained with ffi.dlopen().  After this call, access to\n"
"functions or variables from the library will fail (possibly with a\n"
"segmentation fault).");

static PyObject *ffi_dlopen(PyObject *self, PyObject *args);  /* forward */
static PyObject *ffi_dlclose(PyObject *self, PyObject *args);  /* forward */

PyDoc_STRVAR(ffi_int_const_doc,
"Get the value of an integer constant.\n"
"\n"
"'ffi.integer_const(\"xxx\")' is equivalent to 'lib.xxx' if xxx names an\n"
"integer constant.  The point of this function is limited to use cases\n"
"where you have an 'ffi' object but not any associated 'lib' object.");

static PyObject *ffi_int_const(FFIObject *self, PyObject *args, PyObject *kwds)
{
    char *name;
    PyObject *x;
    static char *keywords[] = {"name", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", keywords, &name))
        return NULL;

    x = ffi_fetch_int_constant(self, name, 0);

    if (x == NULL && !PyErr_Occurred()) {
        PyErr_Format(PyExc_AttributeError,
                     "integer constant '%.200s' not found", name);
    }
    return x;
}

PyDoc_STRVAR(ffi_list_types_doc,
"Returns the user type names known to this FFI instance.\n"
"This returns a tuple containing three lists of names:\n"
"(typedef_names, names_of_structs, names_of_unions)");

static PyObject *ffi_list_types(FFIObject *self, PyObject *noargs)
{
    Py_ssize_t i, n1 = self->types_builder.ctx.num_typenames;
    Py_ssize_t n23 = self->types_builder.ctx.num_struct_unions;
    PyObject *o, *lst[3] = {NULL, NULL, NULL}, *result = NULL;

    lst[0] = PyList_New(n1);
    if (lst[0] == NULL)
        goto error;
    lst[1] = PyList_New(0);
    if (lst[1] == NULL)
        goto error;
    lst[2] = PyList_New(0);
    if (lst[2] == NULL)
        goto error;

    for (i = 0; i < n1; i++) {
        o = PyText_FromString(self->types_builder.ctx.typenames[i].name);
        if (o == NULL)
            goto error;
        PyList_SET_ITEM(lst[0], i, o);
    }

    for (i = 0; i < n23; i++) {
        const struct _cffi_struct_union_s *s;
        int err, index;

        s = &self->types_builder.ctx.struct_unions[i];
        if (s->name[0] == '$')
            continue;

        o = PyText_FromString(s->name);
        if (o == NULL)
            goto error;
        index = (s->flags & _CFFI_F_UNION) ? 2 : 1;
        err = PyList_Append(lst[index], o);
        Py_DECREF(o);
        if (err < 0)
            goto error;
    }
    result = PyTuple_Pack(3, lst[0], lst[1], lst[2]);
    /* fall-through */
 error:
    Py_XDECREF(lst[2]);
    Py_XDECREF(lst[1]);
    Py_XDECREF(lst[0]);
    return result;
}

PyDoc_STRVAR(ffi_memmove_doc,
"ffi.memmove(dest, src, n) copies n bytes of memory from src to dest.\n"
"\n"
"Like the C function memmove(), the memory areas may overlap;\n"
"apart from that it behaves like the C function memcpy().\n"
"\n"
"'src' can be any cdata ptr or array, or any Python buffer object.\n"
"'dest' can be any cdata ptr or array, or a writable Python buffer\n"
"object.  The size to copy, 'n', is always measured in bytes.\n"
"\n"
"Unlike other methods, this one supports all Python buffer including\n"
"byte strings and bytearrays---but it still does not support\n"
"non-contiguous buffers.");

#define ffi_memmove  b_memmove     /* ffi_memmove() => b_memmove()
                                      from _cffi_backend.c */

PyDoc_STRVAR(ffi_init_once_doc,
"init_once(function, tag): run function() once.  More precisely,\n"
"'function()' is called the first time we see a given 'tag'.\n"
"\n"
"The return value of function() is remembered and returned by the current\n"
"and all future init_once() with the same tag.  If init_once() is called\n"
"from multiple threads in parallel, all calls block until the execution\n"
"of function() is done.  If function() raises an exception, it is\n"
"propagated and nothing is cached.");

#if PY_MAJOR_VERSION < 3
/* PyCapsule_New is redefined to be PyCObject_FromVoidPtr in _cffi_backend,
   which gives 2.6 compatibility; but the destructor signature is different */
static void _free_init_once_lock(void *lock)
{
    PyThread_free_lock((PyThread_type_lock)lock);
}
#else
static void _free_init_once_lock(PyObject *capsule)
{
    PyThread_type_lock lock;
    lock = PyCapsule_GetPointer(capsule, "cffi_init_once_lock");
    if (lock != NULL)
        PyThread_free_lock(lock);
}
#endif

static PyObject *ffi_init_once(FFIObject *self, PyObject *args, PyObject *kwds)
{
    static char *keywords[] = {"func", "tag", NULL};
    PyObject *cache, *func, *tag, *tup, *res, *x, *lockobj;
    PyThread_type_lock lock;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO", keywords, &func, &tag))
        return NULL;

    /* a lot of fun with reference counting and error checking
       in this function */

    /* atomically get or create a new dict (no GIL release) */
    cache = self->init_once_cache;
    if (cache == NULL) {
        cache = PyDict_New();
        if (cache == NULL)
            return NULL;
        self->init_once_cache = cache;
    }

    /* get the tuple from cache[tag], or make a new one: (False, lock) */
    tup = PyDict_GetItem(cache, tag);
    if (tup == NULL) {
        lock = PyThread_allocate_lock();
        if (lock == NULL)
            return NULL;
        x = PyCapsule_New(lock, "cffi_init_once_lock", _free_init_once_lock);
        if (x == NULL) {
            PyThread_free_lock(lock);
            return NULL;
        }
        tup = PyTuple_Pack(2, Py_False, x);
        Py_DECREF(x);
        if (tup == NULL)
            return NULL;
        x = tup;

        /* Possible corner case if 'tag' is an object overriding __eq__
           in pure Python: the GIL may be released when we are running it.
           We really need to call dict.setdefault(). */
        tup = PyObject_CallMethod(cache, "setdefault", "OO", tag, x);
        Py_DECREF(x);
        if (tup == NULL)
            return NULL;

        Py_DECREF(tup);   /* there is still a ref inside the dict */
    }

    res = PyTuple_GET_ITEM(tup, 1);
    Py_INCREF(res);

    if (PyTuple_GET_ITEM(tup, 0) == Py_True) {
        /* tup == (True, result): return the result. */
        return res;
    }

    /* tup == (False, lock) */
    lockobj = res;
    lock = (PyThread_type_lock)PyCapsule_GetPointer(lockobj,
                                                    "cffi_init_once_lock");
    if (lock == NULL) {
        Py_DECREF(lockobj);
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    PyThread_acquire_lock(lock, WAIT_LOCK);
    Py_END_ALLOW_THREADS

    x = PyDict_GetItem(cache, tag);
    if (x != NULL && PyTuple_GET_ITEM(x, 0) == Py_True) {
        /* the real result was put in the dict while we were waiting
           for PyThread_acquire_lock() above */
        res = PyTuple_GET_ITEM(x, 1);
        Py_INCREF(res);
    }
    else {
        res = PyObject_CallFunction(func, "");
        if (res != NULL) {
            tup = PyTuple_Pack(2, Py_True, res);
            if (tup == NULL || PyDict_SetItem(cache, tag, tup) < 0) {
                Py_DECREF(res);
                res = NULL;
            }
            Py_XDECREF(tup);
        }
    }

    PyThread_release_lock(lock);
    Py_DECREF(lockobj);
    return res;
}

PyDoc_STRVAR(ffi_release_doc,
"Release now the resources held by a 'cdata' object from ffi.new(),\n"
"ffi.gc() or ffi.from_buffer().  The cdata object must not be used\n"
"afterwards.\n"
"\n"
"'ffi.release(cdata)' is equivalent to 'cdata.__exit__()'.\n"
"\n"
"Note that on CPython this method has no effect (so far) on objects\n"
"returned by ffi.new(), because the memory is allocated inline with the\n"
"cdata object and cannot be freed independently.  It might be fixed in\n"
"future releases of cffi.");

#define ffi_release  b_release     /* ffi_release() => b_release()
                                      from _cffi_backend.c */


#define METH_VKW  (METH_VARARGS | METH_KEYWORDS)
static PyMethodDef ffi_methods[] = {
 {"addressof",  (PyCFunction)ffi_addressof,  METH_VARARGS, ffi_addressof_doc},
 {"alignof",    (PyCFunction)ffi_alignof,    METH_O,       ffi_alignof_doc},
 {"def_extern", (PyCFunction)ffi_def_extern, METH_VKW,     ffi_def_extern_doc},
 {"callback",   (PyCFunction)ffi_callback,   METH_VKW,     ffi_callback_doc},
 {"cast",       (PyCFunction)ffi_cast,       METH_VARARGS, ffi_cast_doc},
 {"dlclose",    (PyCFunction)ffi_dlclose,    METH_VARARGS, ffi_dlclose_doc},
 {"dlopen",     (PyCFunction)ffi_dlopen,     METH_VARARGS, ffi_dlopen_doc},
 {"from_buffer",(PyCFunction)ffi_from_buffer,METH_VKW,     ffi_from_buffer_doc},
 {"from_handle",(PyCFunction)ffi_from_handle,METH_O,       ffi_from_handle_doc},
 {"gc",         (PyCFunction)ffi_gc,         METH_VKW,     ffi_gc_doc},
 {"getctype",   (PyCFunction)ffi_getctype,   METH_VKW,     ffi_getctype_doc},
#ifdef MS_WIN32
 {"getwinerror",(PyCFunction)ffi_getwinerror,METH_VKW,     ffi_getwinerror_doc},
#endif
 {"init_once",  (PyCFunction)ffi_init_once,  METH_VKW,     ffi_init_once_doc},
 {"integer_const",(PyCFunction)ffi_int_const,METH_VKW,     ffi_int_const_doc},
 {"list_types", (PyCFunction)ffi_list_types, METH_NOARGS,  ffi_list_types_doc},
 {"memmove",    (PyCFunction)ffi_memmove,    METH_VKW,     ffi_memmove_doc},
 {"new",        (PyCFunction)ffi_new,        METH_VKW,     ffi_new_doc},
{"new_allocator",(PyCFunction)ffi_new_allocator,METH_VKW,ffi_new_allocator_doc},
 {"new_handle", (PyCFunction)ffi_new_handle, METH_O,       ffi_new_handle_doc},
 {"offsetof",   (PyCFunction)ffi_offsetof,   METH_VARARGS, ffi_offsetof_doc},
 {"release",    (PyCFunction)ffi_release,    METH_O,       ffi_release_doc},
 {"sizeof",     (PyCFunction)ffi_sizeof,     METH_O,       ffi_sizeof_doc},
 {"string",     (PyCFunction)ffi_string,     METH_VKW,     ffi_string_doc},
 {"typeof",     (PyCFunction)ffi_typeof,     METH_O,       ffi_typeof_doc},
 {"unpack",     (PyCFunction)ffi_unpack,     METH_VKW,     ffi_unpack_doc},
 {NULL}
};

static PyGetSetDef ffi_getsets[] = {
    {"errno",  ffi_get_errno,  ffi_set_errno,  ffi_errno_doc},
    {NULL}
};

static PyTypeObject FFI_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_cffi_backend.FFI",
    sizeof(FFIObject),
    0,
    (destructor)ffi_dealloc,                    /* tp_dealloc */
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE,                    /* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)ffi_traverse,                 /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    ffi_methods,                                /* tp_methods */
    0,                                          /* tp_members */
    ffi_getsets,                                /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    ffiobj_init,                                /* tp_init */
    0,                                          /* tp_alloc */
    ffiobj_new,                                 /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
};


static PyObject *
_fetch_external_struct_or_union(const struct _cffi_struct_union_s *s,
                                PyObject *included_ffis, int recursion)
{
    Py_ssize_t i;

    if (included_ffis == NULL)
        return NULL;

    if (recursion > 100) {
        PyErr_SetString(PyExc_RuntimeError,
                        "recursion overflow in ffi.include() delegations");
        return NULL;
    }

    for (i = 0; i < PyTuple_GET_SIZE(included_ffis); i++) {
        FFIObject *ffi1;
        const struct _cffi_struct_union_s *s1;
        int sindex;
        PyObject *x;

        ffi1 = (FFIObject *)PyTuple_GET_ITEM(included_ffis, i);
        sindex = search_in_struct_unions(&ffi1->types_builder.ctx, s->name,
                                         strlen(s->name));
        if (sindex < 0)  /* not found at all */
            continue;
        s1 = &ffi1->types_builder.ctx.struct_unions[sindex];
        if ((s1->flags & (_CFFI_F_EXTERNAL | _CFFI_F_UNION))
                == (s->flags & _CFFI_F_UNION)) {
            /* s1 is not external, and the same kind (struct or union) as s */
            return _realize_c_struct_or_union(&ffi1->types_builder, sindex);
        }
        /* not found, look more recursively */
        x = _fetch_external_struct_or_union(
                s, ffi1->types_builder.included_ffis, recursion + 1);
        if (x != NULL || PyErr_Occurred())
            return x;   /* either found, or got an error */
    }
    return NULL;   /* not found at all, leave without an error */
}
