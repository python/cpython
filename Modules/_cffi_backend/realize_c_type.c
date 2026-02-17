
typedef struct {
    struct _cffi_type_context_s ctx;   /* inlined substructure */
    PyObject *types_dict;
    PyObject *included_ffis;
    PyObject *included_libs;
    PyObject *_keepalive1;
    PyObject *_keepalive2;
} builder_c_t;


static PyObject *all_primitives[_CFFI__NUM_PRIM];
static CTypeDescrObject *g_ct_voidp, *g_ct_chararray;

static PyObject *build_primitive_type(int num);   /* forward */

#define primitive_in_range(num)   ((num) >= 0 && (num) < _CFFI__NUM_PRIM)
#define get_primitive_type(num)                                 \
    ((primitive_in_range(num) && all_primitives[num] != NULL) ? \
        all_primitives[num] : build_primitive_type(num))

static int init_global_types_dict(PyObject *ffi_type_dict)
{
    int err;
    PyObject *ct_void, *ct_char, *ct2, *pnull;
    /* XXX some leaks in case these functions fail, but well,
       MemoryErrors during importing an extension module are kind
       of bad anyway */

    ct_void = get_primitive_type(_CFFI_PRIM_VOID);         // 'void'
    if (ct_void == NULL)
        return -1;

    ct2 = new_pointer_type((CTypeDescrObject *)ct_void);   // 'void *'
    if (ct2 == NULL)
        return -1;
    g_ct_voidp = (CTypeDescrObject *)ct2;

    ct_char = get_primitive_type(_CFFI_PRIM_CHAR);         // 'char'
    if (ct_char == NULL)
        return -1;

    ct2 = new_pointer_type((CTypeDescrObject *)ct_char);   // 'char *'
    if (ct2 == NULL)
        return -1;

    ct2 = new_array_type((CTypeDescrObject *)ct2, -1);     // 'char[]'
    if (ct2 == NULL)
        return -1;
    g_ct_chararray = (CTypeDescrObject *)ct2;

    pnull = new_simple_cdata(NULL, g_ct_voidp);
    if (pnull == NULL)
        return -1;
    err = PyDict_SetItemString(ffi_type_dict, "NULL", pnull);
    Py_DECREF(pnull);
    return err;
}

static void free_builder_c(builder_c_t *builder, int ctx_is_static)
{
    if (!ctx_is_static) {
        size_t i;
        const void *mem[] = {builder->ctx.types,
                             builder->ctx.globals,
                             builder->ctx.struct_unions,
                             //builder->ctx.fields: allocated with struct_unions
                             builder->ctx.enums,
                             builder->ctx.typenames};
        for (i = 0; i < sizeof(mem) / sizeof(*mem); i++) {
            if (mem[i] != NULL)
                PyMem_Free((void *)mem[i]);
        }
    }
    Py_XDECREF(builder->included_ffis);
    Py_XDECREF(builder->included_libs);
    Py_XDECREF(builder->types_dict);
    Py_XDECREF(builder->_keepalive1);
    Py_XDECREF(builder->_keepalive2);
}

static int init_builder_c(builder_c_t *builder,
                          const struct _cffi_type_context_s *ctx)
{
    PyObject *ldict = PyDict_New();
    if (ldict == NULL)
        return -1;

    if (ctx)
        builder->ctx = *ctx;
    else
        memset(&builder->ctx, 0, sizeof(builder->ctx));

    builder->types_dict = ldict;
    builder->included_ffis = NULL;
    builder->included_libs = NULL;
    builder->_keepalive1 = NULL;
    builder->_keepalive2 = NULL;
    return 0;
}

static PyObject *build_primitive_type(int num)
{
    /* XXX too many translations between here and new_primitive_type() */
    static const char *primitive_name[] = {
        NULL,
        "_Bool",
        "char",
        "signed char",
        "unsigned char",
        "short",
        "unsigned short",
        "int",
        "unsigned int",
        "long",
        "unsigned long",
        "long long",
        "unsigned long long",
        "float",
        "double",
        "long double",
        "wchar_t",
        "int8_t",
        "uint8_t",
        "int16_t",
        "uint16_t",
        "int32_t",
        "uint32_t",
        "int64_t",
        "uint64_t",
        "intptr_t",
        "uintptr_t",
        "ptrdiff_t",
        "size_t",
        "ssize_t",
        "int_least8_t",
        "uint_least8_t",
        "int_least16_t",
        "uint_least16_t",
        "int_least32_t",
        "uint_least32_t",
        "int_least64_t",
        "uint_least64_t",
        "int_fast8_t",
        "uint_fast8_t",
        "int_fast16_t",
        "uint_fast16_t",
        "int_fast32_t",
        "uint_fast32_t",
        "int_fast64_t",
        "uint_fast64_t",
        "intmax_t",
        "uintmax_t",
        "float _Complex",
        "double _Complex",
        "char16_t",
        "char32_t",
    };
    PyObject *x;

    assert(sizeof(primitive_name) == sizeof(*primitive_name) * _CFFI__NUM_PRIM);
    if (num == _CFFI_PRIM_VOID) {
        x = new_void_type();
    }
    else if (primitive_in_range(num) && primitive_name[num] != NULL) {
        x = new_primitive_type(primitive_name[num]);
    }
    else if (num == _CFFI__UNKNOWN_PRIM) {
        PyErr_SetString(FFIError, "primitive integer type with an unexpected "
                        "size (or not an integer type at all)");
        return NULL;
    }
    else if (num == _CFFI__UNKNOWN_FLOAT_PRIM) {
        PyErr_SetString(FFIError, "primitive floating-point type with an "
                        "unexpected size (or not a float type at all)");
        return NULL;
    }
    else if (num == _CFFI__UNKNOWN_LONG_DOUBLE) {
        PyErr_SetString(FFIError, "primitive floating-point type is "
                        "'long double', not supported for now with "
                        "the syntax 'typedef double... xxx;'");
        return NULL;
    }
    else {
        PyErr_Format(PyExc_NotImplementedError, "prim=%d", num);
        return NULL;
    }

    all_primitives[num] = x;
    return x;
}

static PyObject *realize_global_int(builder_c_t *builder, int gindex)
{
    int neg;
    char got[64];
    unsigned long long value;
    struct _cffi_getconst_s gc;
    const struct _cffi_global_s *g = &builder->ctx.globals[gindex];
    gc.ctx = &builder->ctx;
    gc.gindex = gindex;
    /* note: we cast g->address to this function type; we do the same
       in parse_c_type:parse_sequel() too.  Note that the called function
       may be declared simply with "unsigned long long *" as argument,
       which is fine as it is the first field in _cffi_getconst_s. */
    assert(&gc.value == (unsigned long long *)&gc);
    neg = ((int(*)(struct _cffi_getconst_s *))g->address)(&gc);
    value = gc.value;

    switch (neg) {

    case 0:
        if (value <= (unsigned long long)LONG_MAX)
            return PyInt_FromLong((long)value);
        else
            return PyLong_FromUnsignedLongLong(value);

    case 1:
        if ((long long)value >= (long long)LONG_MIN)
            return PyInt_FromLong((long)value);
        else
            return PyLong_FromLongLong((long long)value);

    default:
        break;
    }
    if (neg == 2)
        sprintf(got, "%llu (0x%llx)", value, value);
    else
        sprintf(got, "%lld", (long long)value);
    PyErr_Format(FFIError, "the C compiler says '%.200s' is equal to %s, "
                           "but the cdef disagrees", g->name, got);
    return NULL;
}

static CTypeDescrObject *
unwrap_fn_as_fnptr(PyObject *x)
{
    assert(PyTuple_Check(x));
    return (CTypeDescrObject *)PyTuple_GET_ITEM(x, 0);
}

static CTypeDescrObject *
unexpected_fn_type(PyObject *x)
{
    CTypeDescrObject *ct = unwrap_fn_as_fnptr(x);
    char *text1 = ct->ct_name;
    char *text2 = text1 + ct->ct_name_position + 1;
    assert(text2[-3] == '(');
    text2[-3] = '\0';
    PyErr_Format(FFIError, "the type '%s%s' is a function type, not a "
                           "pointer-to-function type", text1, text2);
    text2[-3] = '(';
    return NULL;
}

static PyObject *
realize_c_type_or_func(builder_c_t *builder,
                       _cffi_opcode_t opcodes[], int index);  /* forward */


/* Interpret an opcodes[] array.  If opcodes == ctx->types, store all
   the intermediate types back in the opcodes[].  Returns a new
   reference.
*/
static CTypeDescrObject *
realize_c_type(builder_c_t *builder, _cffi_opcode_t opcodes[], int index)
{
    PyObject *x = realize_c_type_or_func(builder, opcodes, index);
    if (x == NULL || CTypeDescr_Check(x))
        return (CTypeDescrObject *)x;
    else {
        unexpected_fn_type(x);
        Py_DECREF(x);
        return NULL;
    }
}

static void _realize_name(char *target, const char *prefix, const char *srcname)
{
    /* "xyz" => "struct xyz"
       "$xyz" => "xyz"
       "$1" => "struct $1"
    */
    if (srcname[0] == '$' && srcname[1] != '$' &&
            !('0' <= srcname[1] && srcname[1] <= '9')) {
        strcpy(target, &srcname[1]);
    }
    else {
        strcpy(target, prefix);
        strcat(target, srcname);
    }
}

static void _unrealize_name(char *target, const char *srcname)
{
    /* reverse of _realize_name() */
    if (strncmp(srcname, "struct ", 7) == 0) {
        strcpy(target, &srcname[7]);
    }
    else if (strncmp(srcname, "union ", 6) == 0) {
        strcpy(target, &srcname[6]);
    }
    else if (strncmp(srcname, "enum ", 5) == 0) {
        strcpy(target, &srcname[5]);
    }
    else {
        strcpy(target, "$");
        strcat(target, srcname);
    }
}

static PyObject *                                              /* forward */
_fetch_external_struct_or_union(const struct _cffi_struct_union_s *s,
                                PyObject *included_ffis, int recursion);

static PyObject *
_realize_c_struct_or_union(builder_c_t *builder, int sindex)
{
    PyObject *x;
    _cffi_opcode_t op2;
    const struct _cffi_struct_union_s *s;

    if (sindex == _CFFI__IO_FILE_STRUCT) {
        /* returns a single global cached opaque type */
        static PyObject *file_struct = NULL;
        if (file_struct == NULL)
            file_struct = new_struct_or_union_type("FILE",
                                                   CT_STRUCT | CT_IS_FILE);
        Py_XINCREF(file_struct);
        return file_struct;
    }

    s = &builder->ctx.struct_unions[sindex];
    op2 = builder->ctx.types[s->type_index];
    if ((((uintptr_t)op2) & 1) == 0) {
        x = (PyObject *)op2;     /* found already in the "primary" slot */
        Py_INCREF(x);
    }
    else {
        CTypeDescrObject *ct = NULL;

        if (!(s->flags & _CFFI_F_EXTERNAL)) {
            int flags = (s->flags & _CFFI_F_UNION) ? CT_UNION : CT_STRUCT;
            char *name = alloca(8 + strlen(s->name));
            _realize_name(name,
                          (s->flags & _CFFI_F_UNION) ? "union " : "struct ",
                          s->name);
            if (strcmp(name, "struct _IO_FILE") == 0)
                x = _realize_c_struct_or_union(builder, _CFFI__IO_FILE_STRUCT);
            else
                x = new_struct_or_union_type(name, flags);
            if (x == NULL)
                return NULL;

            if (!(s->flags & _CFFI_F_OPAQUE)) {
                assert(s->first_field_index >= 0);
                ct = (CTypeDescrObject *)x;
                ct->ct_size = (Py_ssize_t)s->size;
                ct->ct_length = s->alignment;   /* may be -1 */
                ct->ct_flags &= ~CT_IS_OPAQUE;
                ct->ct_flags |= CT_LAZY_FIELD_LIST;
                ct->ct_extra = builder;
            }
            else
                assert(s->first_field_index < 0);
        }
        else {
            assert(s->first_field_index < 0);
            x = _fetch_external_struct_or_union(s, builder->included_ffis, 0);
            if (x == NULL) {
                if (!PyErr_Occurred())
                    PyErr_Format(FFIError, "'%s %.200s' should come from "
                                 "ffi.include() but was not found",
                                 (s->flags & _CFFI_F_UNION) ? "union"
                                 : "struct", s->name);
                return NULL;
            }
            if (!(s->flags & _CFFI_F_OPAQUE)) {
                if (((CTypeDescrObject *)x)->ct_flags & CT_IS_OPAQUE) {
                    const char *prefix = (s->flags & _CFFI_F_UNION) ? "union"
                                         : "struct";
                    PyErr_Format(PyExc_NotImplementedError,
                                 "'%s %.200s' is opaque in the ffi.include(), "
                                 "but no longer in the ffi doing the include "
                                 "(workaround: don't use ffi.include() but "
                                 "duplicate the declarations of everything "
                                 "using %s %.200s)",
                                 prefix, s->name, prefix, s->name);
                    Py_DECREF(x);
                    return NULL;
                }
            }
        }

        /* Update the "primary" OP_STRUCT_UNION slot */
        assert((((uintptr_t)x) & 1) == 0);
        assert(builder->ctx.types[s->type_index] == op2);
        Py_INCREF(x);
        builder->ctx.types[s->type_index] = x;

        if (ct != NULL && s->size == (size_t)-2) {
            /* oops, this struct is unnamed and we couldn't generate
               a C expression to get its size.  We have to rely on
               complete_struct_or_union() to compute it now. */
            if (do_realize_lazy_struct(ct) < 0) {
                builder->ctx.types[s->type_index] = op2;
                return NULL;
            }
        }
    }
    return x;
}

static PyObject *
realize_c_type_or_func_now(builder_c_t *builder, _cffi_opcode_t op,
                           _cffi_opcode_t opcodes[], int index)
{
    PyObject *x, *y, *z;
    Py_ssize_t length = -1;

    switch (_CFFI_GETOP(op)) {

    case _CFFI_OP_PRIMITIVE:
        x = get_primitive_type(_CFFI_GETARG(op));
        Py_XINCREF(x);
        break;

    case _CFFI_OP_POINTER:
        y = realize_c_type_or_func(builder, opcodes, _CFFI_GETARG(op));
        if (y == NULL)
            return NULL;
        if (CTypeDescr_Check(y)) {
            x = new_pointer_type((CTypeDescrObject *)y);
        }
        else {
            assert(PyTuple_Check(y));   /* from _CFFI_OP_FUNCTION */
            x = PyTuple_GET_ITEM(y, 0);
            Py_INCREF(x);
        }
        Py_DECREF(y);
        break;

    case _CFFI_OP_ARRAY:
        length = (Py_ssize_t)opcodes[index + 1];
        /* fall-through */
    case _CFFI_OP_OPEN_ARRAY:
        y = (PyObject *)realize_c_type(builder, opcodes, _CFFI_GETARG(op));
        if (y == NULL)
            return NULL;
        z = new_pointer_type((CTypeDescrObject *)y);
        Py_DECREF(y);
        if (z == NULL)
            return NULL;
        x = new_array_type((CTypeDescrObject *)z, length);
        Py_DECREF(z);
        break;

    case _CFFI_OP_STRUCT_UNION:
        x = _realize_c_struct_or_union(builder, _CFFI_GETARG(op));
        break;

    case _CFFI_OP_ENUM:
    {
        const struct _cffi_enum_s *e;
        _cffi_opcode_t op2;

        e = &builder->ctx.enums[_CFFI_GETARG(op)];
        op2 = builder->ctx.types[e->type_index];
        if ((((uintptr_t)op2) & 1) == 0) {
            x = (PyObject *)op2;
            Py_INCREF(x);
        }
        else {
            PyObject *enumerators = NULL, *enumvalues = NULL, *tmp;
            Py_ssize_t i, j, n = 0;
            const char *p;
            int gindex;
            PyObject *args;
            PyObject *basetd = get_primitive_type(e->type_prim);
            if (basetd == NULL)
                return NULL;

            if (*e->enumerators != '\0') {
                n++;
                for (p = e->enumerators; *p != '\0'; p++)
                    n += (*p == ',');
            }
            enumerators = PyTuple_New(n);
            if (enumerators == NULL)
                return NULL;

            enumvalues = PyTuple_New(n);
            if (enumvalues == NULL) {
                Py_DECREF(enumerators);
                return NULL;
            }

            p = e->enumerators;
            for (i = 0; i < n; i++) {
                j = 0;
                while (p[j] != ',' && p[j] != '\0')
                    j++;
                tmp = PyText_FromStringAndSize(p, j);
                if (tmp == NULL)
                    break;
                PyTuple_SET_ITEM(enumerators, i, tmp);

                gindex = search_in_globals(&builder->ctx, p, j);
                assert(gindex >= 0);
                assert(builder->ctx.globals[gindex].type_op ==
                       _CFFI_OP(_CFFI_OP_ENUM, -1));

                tmp = realize_global_int(builder, gindex);
                if (tmp == NULL)
                    break;
                PyTuple_SET_ITEM(enumvalues, i, tmp);

                p += j + 1;
            }

            args = NULL;
            if (!PyErr_Occurred()) {
                char *name = alloca(6 + strlen(e->name));
                _realize_name(name, "enum ", e->name);
                args = Py_BuildValue("(sOOO)", name, enumerators,
                                     enumvalues, basetd);
            }
            Py_DECREF(enumerators);
            Py_DECREF(enumvalues);
            if (args == NULL)
                return NULL;

            x = b_new_enum_type(NULL, args);
            Py_DECREF(args);
            if (x == NULL)
                return NULL;

            /* Update the "primary" _CFFI_OP_ENUM slot, which
               may be the same or a different slot than the "current" one */
            assert((((uintptr_t)x) & 1) == 0);
            assert(builder->ctx.types[e->type_index] == op2);
            Py_INCREF(x);
            builder->ctx.types[e->type_index] = x;

            /* Done, leave without updating the "current" slot because
               it may be done already above.  If not, never mind, the
               next call to realize_c_type() will do it. */
            return x;
        }
        break;
    }

    case _CFFI_OP_FUNCTION:
    {
        PyObject *fargs;
        int i, base_index, num_args, ellipsis, abi;

        y = (PyObject *)realize_c_type(builder, opcodes, _CFFI_GETARG(op));
        if (y == NULL)
            return NULL;

        base_index = index + 1;
        num_args = 0;
        /* note that if the arguments are already built, they have a
           pointer in the 'opcodes' array, and GETOP() returns a
           random even value.  But OP_FUNCTION_END is odd, so the
           condition below still works correctly. */
        while (_CFFI_GETOP(opcodes[base_index + num_args]) !=
                   _CFFI_OP_FUNCTION_END)
            num_args++;

        ellipsis = _CFFI_GETARG(opcodes[base_index + num_args]) & 0x01;
        abi      = _CFFI_GETARG(opcodes[base_index + num_args]) & 0xFE;
        switch (abi) {
        case 0:
            abi = FFI_DEFAULT_ABI;
            break;
        case 2:
#if defined(MS_WIN32) && !defined(_WIN64)
            abi = FFI_STDCALL;
#else
            abi = FFI_DEFAULT_ABI;
#endif
            break;
        default:
            PyErr_Format(FFIError, "abi number %d not supported", abi);
            Py_DECREF(y);
            return NULL;
        }

        fargs = PyTuple_New(num_args);
        if (fargs == NULL) {
            Py_DECREF(y);
            return NULL;
        }

        for (i = 0; i < num_args; i++) {
            z = (PyObject *)realize_c_type(builder, opcodes, base_index + i);
            if (z == NULL) {
                Py_DECREF(fargs);
                Py_DECREF(y);
                return NULL;
            }
            PyTuple_SET_ITEM(fargs, i, z);
        }

        z = new_function_type(fargs, (CTypeDescrObject *)y, ellipsis, abi);
        Py_DECREF(fargs);
        Py_DECREF(y);
        if (z == NULL)
            return NULL;

        x = PyTuple_Pack(1, z);   /* hack: hide the CT_FUNCTIONPTR.  it will
                                     be revealed again by the OP_POINTER */
        Py_DECREF(z);
        break;
    }

    case _CFFI_OP_NOOP:
        x = realize_c_type_or_func(builder, opcodes, _CFFI_GETARG(op));
        break;

    case _CFFI_OP_TYPENAME:
    {
        /* essential: the TYPENAME opcode resolves the type index looked
           up in the 'ctx->typenames' array, but it does so in 'ctx->types'
           instead of in 'opcodes'! */
        int type_index = builder->ctx.typenames[_CFFI_GETARG(op)].type_index;
        x = realize_c_type_or_func(builder, builder->ctx.types, type_index);
        break;
    }

    default:
        PyErr_Format(PyExc_NotImplementedError, "op=%d", (int)_CFFI_GETOP(op));
        return NULL;
    }

    return x;
}

static int _realize_recursion_level;

static PyObject *
realize_c_type_or_func(builder_c_t *builder,
                        _cffi_opcode_t opcodes[], int index)
{
    PyObject *x;
     _cffi_opcode_t op = opcodes[index];

    if ((((uintptr_t)op) & 1) == 0) {
        x = (PyObject *)op;
        Py_INCREF(x);
        return x;
    }

    if (_realize_recursion_level >= 1000) {
        PyErr_Format(PyExc_RuntimeError,
            "type-building recursion too deep or infinite.  "
            "This is known to occur e.g. in ``struct s { void(*callable)"
            "(struct s); }''.  Please report if you get this error and "
            "really need support for your case.");
        return NULL;
    }
    _realize_recursion_level++;
    x = realize_c_type_or_func_now(builder, op, opcodes, index);
    _realize_recursion_level--;

    if (x != NULL && opcodes == builder->ctx.types && opcodes[index] != x) {
        assert((((uintptr_t)x) & 1) == 0);
        assert((((uintptr_t)opcodes[index]) & 1) == 1);
        Py_INCREF(x);
        opcodes[index] = x;
    }
    return x;
}

static CTypeDescrObject *
realize_c_func_return_type(builder_c_t *builder,
                           _cffi_opcode_t opcodes[], int index)
{
    PyObject *x;
    _cffi_opcode_t op = opcodes[index];

    if ((((uintptr_t)op) & 1) == 0) {
        /* already built: assert that it is a function and fish
           for the return type */
        x = (PyObject *)op;
        assert(PyTuple_Check(x));   /* from _CFFI_OP_FUNCTION */
        x = PyTuple_GET_ITEM(x, 0);
        assert(CTypeDescr_Check(x));
        assert(((CTypeDescrObject *)x)->ct_flags & CT_FUNCTIONPTR);
        x = PyTuple_GET_ITEM(((CTypeDescrObject *)x)->ct_stuff, 1);
        assert(CTypeDescr_Check(x));
        Py_INCREF(x);
        return (CTypeDescrObject *)x;
    }
    else {
        assert(_CFFI_GETOP(op) == _CFFI_OP_FUNCTION);
        return realize_c_type(builder, opcodes, _CFFI_GETARG(opcodes[index]));
    }
}

static int do_realize_lazy_struct(CTypeDescrObject *ct)
{
    /* This is called by force_lazy_struct() in _cffi_backend.c */
    assert(ct->ct_flags & (CT_STRUCT | CT_UNION));

    if (ct->ct_flags & CT_LAZY_FIELD_LIST) {
        builder_c_t *builder;
        char *p;
        int n, i, sflags;
        const struct _cffi_struct_union_s *s;
        const struct _cffi_field_s *fld;
        PyObject *fields, *args, *res;

        assert(!(ct->ct_flags & CT_IS_OPAQUE));

        builder = ct->ct_extra;
        assert(builder != NULL);

        p = alloca(2 + strlen(ct->ct_name));
        _unrealize_name(p, ct->ct_name);

        n = search_in_struct_unions(&builder->ctx, p, strlen(p));
        if (n < 0)
            Py_FatalError("lost a struct/union!");

        s = &builder->ctx.struct_unions[n];
        fld = &builder->ctx.fields[s->first_field_index];

        /* XXX painfully build all the Python objects that are the args
           to b_complete_struct_or_union() */

        fields = PyList_New(s->num_fields);
        if (fields == NULL)
            return -1;

        for (i = 0; i < s->num_fields; i++, fld++) {
            _cffi_opcode_t op = fld->field_type_op;
            int fbitsize = -1;
            PyObject *f;
            CTypeDescrObject *ctf;

            switch (_CFFI_GETOP(op)) {

            case _CFFI_OP_BITFIELD:
                assert(fld->field_size >= 0);
                fbitsize = (int)fld->field_size;
                /* fall-through */
            case _CFFI_OP_NOOP:
                ctf = realize_c_type(builder, builder->ctx.types,
                                     _CFFI_GETARG(op));
                break;

            default:
                Py_DECREF(fields);
                PyErr_Format(PyExc_NotImplementedError, "field op=%d",
                             (int)_CFFI_GETOP(op));
                return -1;
            }

            if (ctf != NULL && fld->field_offset == (size_t)-1) {
                /* unnamed struct, with field positions and sizes entirely
                   determined by complete_struct_or_union() and not checked.
                   Or, bitfields (field_size >= 0), similarly not checked. */
                assert(fld->field_size == (size_t)-1 || fbitsize >= 0);
            }
            else if (ctf == NULL || detect_custom_layout(ct, SF_STD_FIELD_POS,
                                     ctf->ct_size, fld->field_size,
                                     "wrong size for field '",
                                     fld->name, "'") < 0) {
                Py_DECREF(fields);
                return -1;
            }

            f = Py_BuildValue("(sOin)", fld->name, ctf,
                              fbitsize, (Py_ssize_t)fld->field_offset);
            if (f == NULL) {
                Py_DECREF(fields);
                return -1;
            }
            PyList_SET_ITEM(fields, i, f);
        }

        sflags = 0;
        if (s->flags & _CFFI_F_CHECK_FIELDS)
            sflags |= SF_STD_FIELD_POS;
        if (s->flags & _CFFI_F_PACKED)
            sflags |= SF_PACKED;

        args = Py_BuildValue("(OOOnii)", ct, fields, Py_None,
                             (Py_ssize_t)s->size,
                             s->alignment,
                             sflags);
        Py_DECREF(fields);
        if (args == NULL)
            return -1;

        ct->ct_extra = NULL;
        ct->ct_flags |= CT_IS_OPAQUE;
        res = b_complete_struct_or_union(NULL, args);
        ct->ct_flags &= ~CT_IS_OPAQUE;
        Py_DECREF(args);

        if (res == NULL) {
            ct->ct_extra = builder;
            return -1;
        }

        assert(ct->ct_stuff != NULL);
        ct->ct_flags &= ~CT_LAZY_FIELD_LIST;
        Py_DECREF(res);
        return 1;
    }
    else {
        assert(ct->ct_flags & CT_IS_OPAQUE);
        return 0;
    }
}
