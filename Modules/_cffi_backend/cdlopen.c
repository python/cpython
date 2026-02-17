/* ffi.dlopen() interface with dlopen()/dlsym()/dlclose() */

static void *cdlopen_fetch(PyObject *libname, void *libhandle,
                           const char *symbol)
{
    void *address;

    if (libhandle == NULL) {
        PyErr_Format(FFIError, "library '%s' has been closed",
                     PyText_AS_UTF8(libname));
        return NULL;
    }

    dlerror();   /* clear error condition */
    address = dlsym(libhandle, symbol);
    if (address == NULL) {
        const char *error = dlerror();
        PyErr_Format(FFIError, "symbol '%s' not found in library '%s': %s",
                     symbol, PyText_AS_UTF8(libname), error);
    }
    return address;
}

static void cdlopen_close_ignore_errors(void *libhandle)
{
    if (libhandle != NULL)
        dlclose(libhandle);
}

static int cdlopen_close(PyObject *libname, void *libhandle)
{
    if (libhandle != NULL && dlclose(libhandle) != 0) {
        const char *error = dlerror();
        PyErr_Format(FFIError, "closing library '%s': %s",
                     PyText_AS_UTF8(libname), error);
        return -1;
    }
    return 0;
}

static PyObject *ffi_dlopen(PyObject *self, PyObject *args)
{
    const char *modname;
    PyObject *temp, *result = NULL;
    void *handle;
    int auto_close;

    handle = b_do_dlopen(args, &modname, &temp, &auto_close);
    if (handle != NULL)
    {
        result = (PyObject *)lib_internal_new((FFIObject *)self,
                                              modname, handle, auto_close);
    }
    Py_XDECREF(temp);
    return result;
}

static PyObject *ffi_dlclose(PyObject *self, PyObject *args)
{
    LibObject *lib;
    void *libhandle;
    if (!PyArg_ParseTuple(args, "O!", &Lib_Type, &lib))
        return NULL;

    libhandle = lib->l_libhandle;
    if (libhandle != NULL)
    {
        lib->l_libhandle = NULL;

        /* Clear the dict to force further accesses to do cdlopen_fetch()
           again, and fail because the library was closed. */
        PyDict_Clear(lib->l_dict);

        if (cdlopen_close(lib->l_libname, libhandle) < 0)
            return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}


static Py_ssize_t cdl_4bytes(char *src)
{
    /* read 4 bytes in little-endian order; return it as a signed integer */
    signed char *ssrc = (signed char *)src;
    unsigned char *usrc = (unsigned char *)src;
    return (ssrc[0] << 24) | (usrc[1] << 16) | (usrc[2] << 8) | usrc[3];
}

static _cffi_opcode_t cdl_opcode(char *src)
{
    return (_cffi_opcode_t)cdl_4bytes(src);
}

typedef struct {
    unsigned long long value;
    int neg;
} cdl_intconst_t;

static int _cdl_realize_global_int(struct _cffi_getconst_s *gc)
{
    /* The 'address' field of 'struct _cffi_global_s' is set to point
       to this function in case ffiobj_init() sees constant integers.
       This fishes around after the 'ctx->globals' array, which is
       initialized to contain another array, this time of
       'cdl_intconst_t' structures.  We get the nth one and it tells
       us what to return.
    */
    cdl_intconst_t *ic;
    ic = (cdl_intconst_t *)(gc->ctx->globals + gc->ctx->num_globals);
    ic += gc->gindex;
    gc->value = ic->value;
    return ic->neg;
}

static int ffiobj_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    FFIObject *ffi;
    static char *keywords[] = {"module_name", "_version", "_types",
                               "_globals", "_struct_unions", "_enums",
                               "_typenames", "_includes", NULL};
    char *ffiname = "?", *types = NULL, *building = NULL;
    Py_ssize_t version = -1;
    Py_ssize_t types_len = 0;
    PyObject *globals = NULL, *struct_unions = NULL, *enums = NULL;
    PyObject *typenames = NULL, *includes = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "|sns#O!O!O!O!O!:FFI", keywords,
                                     &ffiname, &version, &types, &types_len,
                                     &PyTuple_Type, &globals,
                                     &PyTuple_Type, &struct_unions,
                                     &PyTuple_Type, &enums,
                                     &PyTuple_Type, &typenames,
                                     &PyTuple_Type, &includes))
        return -1;

    ffi = (FFIObject *)self;
    if (ffi->ctx_is_nonempty) {
        PyErr_SetString(PyExc_ValueError,
                        "cannot call FFI.__init__() more than once");
        return -1;
    }
    ffi->ctx_is_nonempty = 1;

    if (version == -1 && types_len == 0)
        return 0;
    if (version < CFFI_VERSION_MIN || version > CFFI_VERSION_MAX) {
        PyErr_Format(PyExc_ImportError,
                     "cffi out-of-line Python module '%s' has unknown "
                     "version %p", ffiname, (void *)version);
        return -1;
    }

    if (types_len > 0) {
        /* unpack a string of 4-byte entries into an array of _cffi_opcode_t */
        _cffi_opcode_t *ntypes;
        Py_ssize_t i, n = types_len / 4;

        building = PyMem_Malloc(n * sizeof(_cffi_opcode_t));
        if (building == NULL)
            goto error;
        ntypes = (_cffi_opcode_t *)building;

        for (i = 0; i < n; i++) {
            ntypes[i] = cdl_opcode(types);
            types += 4;
        }
        ffi->types_builder.ctx.types = ntypes;
        ffi->types_builder.ctx.num_types = n;
        building = NULL;
    }

    if (globals != NULL) {
        /* unpack a tuple alternating strings and ints, each two together
           describing one global_s entry with no specified address or size.
           The int is only used with integer constants. */
        struct _cffi_global_s *nglobs;
        cdl_intconst_t *nintconsts;
        Py_ssize_t i, n = PyTuple_GET_SIZE(globals) / 2;

        i = n * (sizeof(struct _cffi_global_s) + sizeof(cdl_intconst_t));
        building = PyMem_Malloc(i);
        if (building == NULL)
            goto error;
        memset(building, 0, i);
        nglobs = (struct _cffi_global_s *)building;
        nintconsts = (cdl_intconst_t *)(nglobs + n);

        for (i = 0; i < n; i++) {
            char *g = PyBytes_AS_STRING(PyTuple_GET_ITEM(globals, i * 2));
            nglobs[i].type_op = cdl_opcode(g); g += 4;
            nglobs[i].name = g;
            if (_CFFI_GETOP(nglobs[i].type_op) == _CFFI_OP_CONSTANT_INT ||
                _CFFI_GETOP(nglobs[i].type_op) == _CFFI_OP_ENUM) {
                PyObject *o = PyTuple_GET_ITEM(globals, i * 2 + 1);
                nglobs[i].address = &_cdl_realize_global_int;
#if PY_MAJOR_VERSION < 3
                if (PyInt_Check(o)) {
                    nintconsts[i].neg = PyInt_AS_LONG(o) <= 0;
                    nintconsts[i].value = (long long)PyInt_AS_LONG(o);
                }
                else
#endif
                {
                    nintconsts[i].neg = PyObject_RichCompareBool(o, Py_False,
                                                                 Py_LE);
                    nintconsts[i].value = PyLong_AsUnsignedLongLongMask(o);
                    if (PyErr_Occurred())
                        goto error;
                }
            }
        }
        ffi->types_builder.ctx.globals = nglobs;
        ffi->types_builder.ctx.num_globals = n;
        building = NULL;
    }

    if (struct_unions != NULL) {
        /* unpack a tuple of struct/unions, each described as a sub-tuple;
           the item 0 of each sub-tuple describes the struct/union, and
           the items 1..N-1 describe the fields, if any */
        struct _cffi_struct_union_s *nstructs;
        struct _cffi_field_s *nfields;
        Py_ssize_t i, n = PyTuple_GET_SIZE(struct_unions);
        Py_ssize_t nf = 0;   /* total number of fields */

        for (i = 0; i < n; i++) {
            nf += PyTuple_GET_SIZE(PyTuple_GET_ITEM(struct_unions, i)) - 1;
        }
        i = (n * sizeof(struct _cffi_struct_union_s) +
             nf * sizeof(struct _cffi_field_s));
        building = PyMem_Malloc(i);
        if (building == NULL)
            goto error;
        memset(building, 0, i);
        nstructs = (struct _cffi_struct_union_s *)building;
        nfields = (struct _cffi_field_s *)(nstructs + n);
        nf = 0;

        for (i = 0; i < n; i++) {
            /* 'desc' is the tuple of strings (desc_struct, desc_field_1, ..) */
            PyObject *desc = PyTuple_GET_ITEM(struct_unions, i);
            Py_ssize_t j, nf1 = PyTuple_GET_SIZE(desc) - 1;
            char *s = PyBytes_AS_STRING(PyTuple_GET_ITEM(desc, 0));
            /* 's' is the first string, describing the struct/union */
            nstructs[i].type_index = cdl_4bytes(s); s += 4;
            nstructs[i].flags = cdl_4bytes(s); s += 4;
            nstructs[i].name = s;
            if (nstructs[i].flags & (_CFFI_F_OPAQUE | _CFFI_F_EXTERNAL)) {
                nstructs[i].size = (size_t)-1;
                nstructs[i].alignment = -1;
                nstructs[i].first_field_index = -1;
                nstructs[i].num_fields = 0;
                assert(nf1 == 0);
            }
            else {
                nstructs[i].size = (size_t)-2;
                nstructs[i].alignment = -2;
                nstructs[i].first_field_index = nf;
                nstructs[i].num_fields = nf1;
            }
            for (j = 0; j < nf1; j++) {
                char *f = PyBytes_AS_STRING(PyTuple_GET_ITEM(desc, j + 1));
                /* 'f' is one of the other strings beyond the first one,
                   describing one field each */
                nfields[nf].field_type_op = cdl_opcode(f); f += 4;
                nfields[nf].field_offset = (size_t)-1;
                if (_CFFI_GETOP(nfields[nf].field_type_op) != _CFFI_OP_NOOP) {
                    nfields[nf].field_size = cdl_4bytes(f); f += 4;
                }
                else {
                    nfields[nf].field_size = (size_t)-1;
                }
                nfields[nf].name = f;
                nf++;
            }
        }
        ffi->types_builder.ctx.struct_unions = nstructs;
        ffi->types_builder.ctx.fields = nfields;
        ffi->types_builder.ctx.num_struct_unions = n;
        building = NULL;
    }

    if (enums != NULL) {
        /* unpack a tuple of strings, each of which describes one enum_s
           entry */
        struct _cffi_enum_s *nenums;
        Py_ssize_t i, n = PyTuple_GET_SIZE(enums);

        i = n * sizeof(struct _cffi_enum_s);
        building = PyMem_Malloc(i);
        if (building == NULL)
            goto error;
        memset(building, 0, i);
        nenums = (struct _cffi_enum_s *)building;

        for (i = 0; i < n; i++) {
            char *e = PyBytes_AS_STRING(PyTuple_GET_ITEM(enums, i));
            /* 'e' is a string describing the enum */
            nenums[i].type_index = cdl_4bytes(e); e += 4;
            nenums[i].type_prim = cdl_4bytes(e); e += 4;
            nenums[i].name = e; e += strlen(e) + 1;
            nenums[i].enumerators = e;
        }
        ffi->types_builder.ctx.enums = nenums;
        ffi->types_builder.ctx.num_enums = n;
        building = NULL;
    }

    if (typenames != NULL) {
        /* unpack a tuple of strings, each of which describes one typename_s
           entry */
        struct _cffi_typename_s *ntypenames;
        Py_ssize_t i, n = PyTuple_GET_SIZE(typenames);

        i = n * sizeof(struct _cffi_typename_s);
        building = PyMem_Malloc(i);
        if (building == NULL)
            goto error;
        memset(building, 0, i);
        ntypenames = (struct _cffi_typename_s *)building;

        for (i = 0; i < n; i++) {
            char *t = PyBytes_AS_STRING(PyTuple_GET_ITEM(typenames, i));
            /* 't' is a string describing the typename */
            ntypenames[i].type_index = cdl_4bytes(t); t += 4;
            ntypenames[i].name = t;
        }
        ffi->types_builder.ctx.typenames = ntypenames;
        ffi->types_builder.ctx.num_typenames = n;
        building = NULL;
    }

    if (includes != NULL) {
        PyObject *included_libs;

        included_libs = PyTuple_New(PyTuple_GET_SIZE(includes));
        if (included_libs == NULL)
            return -1;

        Py_INCREF(includes);
        ffi->types_builder.included_ffis = includes;
        ffi->types_builder.included_libs = included_libs;
    }

    /* Above, we took directly some "char *" strings out of the strings,
       typically from somewhere inside tuples.  Keep them alive by
       incref'ing the whole input arguments. */
    Py_INCREF(args);
    Py_XINCREF(kwds);
    ffi->types_builder._keepalive1 = args;
    ffi->types_builder._keepalive2 = kwds;
    return 0;

 error:
    if (building != NULL)
        PyMem_Free(building);
    if (!PyErr_Occurred())
        PyErr_NoMemory();
    return -1;
}
