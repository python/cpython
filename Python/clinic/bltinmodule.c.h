/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(builtin_abs__doc__,
"abs($module, x, /)\n"
"--\n"
"\n"
"Return the absolute value of the argument.");

#define BUILTIN_ABS_METHODDEF    \
    {"abs", (PyCFunction)builtin_abs, METH_O, builtin_abs__doc__},

PyDoc_STRVAR(builtin_all__doc__,
"all($module, iterable, /)\n"
"--\n"
"\n"
"Return True if bool(x) is True for all values x in the iterable.\n"
"\n"
"If the iterable is empty, return True.");

#define BUILTIN_ALL_METHODDEF    \
    {"all", (PyCFunction)builtin_all, METH_O, builtin_all__doc__},

PyDoc_STRVAR(builtin_any__doc__,
"any($module, iterable, /)\n"
"--\n"
"\n"
"Return True if bool(x) is True for any x in the iterable.\n"
"\n"
"If the iterable is empty, return False.");

#define BUILTIN_ANY_METHODDEF    \
    {"any", (PyCFunction)builtin_any, METH_O, builtin_any__doc__},

PyDoc_STRVAR(builtin_ascii__doc__,
"ascii($module, obj, /)\n"
"--\n"
"\n"
"Return an ASCII-only representation of an object.\n"
"\n"
"As repr(), return a string containing a printable representation of an\n"
"object, but escape the non-ASCII characters in the string returned by\n"
"repr() using \\\\x, \\\\u or \\\\U escapes. This generates a string similar\n"
"to that returned by repr() in Python 2.");

#define BUILTIN_ASCII_METHODDEF    \
    {"ascii", (PyCFunction)builtin_ascii, METH_O, builtin_ascii__doc__},

PyDoc_STRVAR(builtin_bin__doc__,
"bin($module, number, /)\n"
"--\n"
"\n"
"Return the binary representation of an integer.\n"
"\n"
"   >>> bin(2796202)\n"
"   \'0b1010101010101010101010\'");

#define BUILTIN_BIN_METHODDEF    \
    {"bin", (PyCFunction)builtin_bin, METH_O, builtin_bin__doc__},

PyDoc_STRVAR(builtin_callable__doc__,
"callable($module, obj, /)\n"
"--\n"
"\n"
"Return whether the object is callable (i.e., some kind of function).\n"
"\n"
"Note that classes are callable, as are instances of classes with a\n"
"__call__() method.");

#define BUILTIN_CALLABLE_METHODDEF    \
    {"callable", (PyCFunction)builtin_callable, METH_O, builtin_callable__doc__},

PyDoc_STRVAR(builtin_format__doc__,
"format($module, value, format_spec=\'\', /)\n"
"--\n"
"\n"
"Return value.__format__(format_spec)\n"
"\n"
"format_spec defaults to the empty string");

#define BUILTIN_FORMAT_METHODDEF    \
    {"format", (PyCFunction)builtin_format, METH_VARARGS, builtin_format__doc__},

static PyObject *
builtin_format_impl(PyObject *module, PyObject *value, PyObject *format_spec);

static PyObject *
builtin_format(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *value;
    PyObject *format_spec = NULL;

    if (!PyArg_ParseTuple(args, "O|U:format",
        &value, &format_spec)) {
        goto exit;
    }
    return_value = builtin_format_impl(module, value, format_spec);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_chr__doc__,
"chr($module, i, /)\n"
"--\n"
"\n"
"Return a Unicode string of one character with ordinal i; 0 <= i <= 0x10ffff.");

#define BUILTIN_CHR_METHODDEF    \
    {"chr", (PyCFunction)builtin_chr, METH_O, builtin_chr__doc__},

static PyObject *
builtin_chr_impl(PyObject *module, int i);

static PyObject *
builtin_chr(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int i;

    if (!PyArg_Parse(arg, "i:chr", &i)) {
        goto exit;
    }
    return_value = builtin_chr_impl(module, i);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_compile__doc__,
"compile($module, /, source, filename, mode, flags=0,\n"
"        dont_inherit=False, optimize=-1)\n"
"--\n"
"\n"
"Compile source into a code object that can be executed by exec() or eval().\n"
"\n"
"The source code may represent a Python module, statement or expression.\n"
"The filename will be used for run-time error messages.\n"
"The mode must be \'exec\' to compile a module, \'single\' to compile a\n"
"single (interactive) statement, or \'eval\' to compile an expression.\n"
"The flags argument, if present, controls which future statements influence\n"
"the compilation of the code.\n"
"The dont_inherit argument, if true, stops the compilation inheriting\n"
"the effects of any future statements in effect in the code calling\n"
"compile; if absent or false these statements do influence the compilation,\n"
"in addition to any features explicitly specified.");

#define BUILTIN_COMPILE_METHODDEF    \
    {"compile", (PyCFunction)builtin_compile, METH_FASTCALL, builtin_compile__doc__},

static PyObject *
builtin_compile_impl(PyObject *module, PyObject *source, PyObject *filename,
                     const char *mode, int flags, int dont_inherit,
                     int optimize);

static PyObject *
builtin_compile(PyObject *module, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"source", "filename", "mode", "flags", "dont_inherit", "optimize", NULL};
    static _PyArg_Parser _parser = {"OO&s|iii:compile", _keywords, 0};
    PyObject *source;
    PyObject *filename;
    const char *mode;
    int flags = 0;
    int dont_inherit = 0;
    int optimize = -1;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &source, PyUnicode_FSDecoder, &filename, &mode, &flags, &dont_inherit, &optimize)) {
        goto exit;
    }
    return_value = builtin_compile_impl(module, source, filename, mode, flags, dont_inherit, optimize);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_divmod__doc__,
"divmod($module, x, y, /)\n"
"--\n"
"\n"
"Return the tuple (x//y, x%y).  Invariant: div*y + mod == x.");

#define BUILTIN_DIVMOD_METHODDEF    \
    {"divmod", (PyCFunction)builtin_divmod, METH_VARARGS, builtin_divmod__doc__},

static PyObject *
builtin_divmod_impl(PyObject *module, PyObject *x, PyObject *y);

static PyObject *
builtin_divmod(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!PyArg_UnpackTuple(args, "divmod",
        2, 2,
        &x, &y)) {
        goto exit;
    }
    return_value = builtin_divmod_impl(module, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_eval__doc__,
"eval($module, source, globals=None, locals=None, /)\n"
"--\n"
"\n"
"Evaluate the given source in the context of globals and locals.\n"
"\n"
"The source may be a string representing a Python expression\n"
"or a code object as returned by compile().\n"
"The globals must be a dictionary and locals can be any mapping,\n"
"defaulting to the current globals and locals.\n"
"If only globals is given, locals defaults to it.");

#define BUILTIN_EVAL_METHODDEF    \
    {"eval", (PyCFunction)builtin_eval, METH_VARARGS, builtin_eval__doc__},

static PyObject *
builtin_eval_impl(PyObject *module, PyObject *source, PyObject *globals,
                  PyObject *locals);

static PyObject *
builtin_eval(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *source;
    PyObject *globals = Py_None;
    PyObject *locals = Py_None;

    if (!PyArg_UnpackTuple(args, "eval",
        1, 3,
        &source, &globals, &locals)) {
        goto exit;
    }
    return_value = builtin_eval_impl(module, source, globals, locals);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_exec__doc__,
"exec($module, source, globals=None, locals=None, /)\n"
"--\n"
"\n"
"Execute the given source in the context of globals and locals.\n"
"\n"
"The source may be a string representing one or more Python statements\n"
"or a code object as returned by compile().\n"
"The globals must be a dictionary and locals can be any mapping,\n"
"defaulting to the current globals and locals.\n"
"If only globals is given, locals defaults to it.");

#define BUILTIN_EXEC_METHODDEF    \
    {"exec", (PyCFunction)builtin_exec, METH_VARARGS, builtin_exec__doc__},

static PyObject *
builtin_exec_impl(PyObject *module, PyObject *source, PyObject *globals,
                  PyObject *locals);

static PyObject *
builtin_exec(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *source;
    PyObject *globals = Py_None;
    PyObject *locals = Py_None;

    if (!PyArg_UnpackTuple(args, "exec",
        1, 3,
        &source, &globals, &locals)) {
        goto exit;
    }
    return_value = builtin_exec_impl(module, source, globals, locals);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_globals__doc__,
"globals($module, /)\n"
"--\n"
"\n"
"Return the dictionary containing the current scope\'s global variables.\n"
"\n"
"NOTE: Updates to this dictionary *will* affect name lookups in the current\n"
"global scope and vice-versa.");

#define BUILTIN_GLOBALS_METHODDEF    \
    {"globals", (PyCFunction)builtin_globals, METH_NOARGS, builtin_globals__doc__},

static PyObject *
builtin_globals_impl(PyObject *module);

static PyObject *
builtin_globals(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return builtin_globals_impl(module);
}

PyDoc_STRVAR(builtin_hasattr__doc__,
"hasattr($module, obj, name, /)\n"
"--\n"
"\n"
"Return whether the object has an attribute with the given name.\n"
"\n"
"This is done by calling getattr(obj, name) and catching AttributeError.");

#define BUILTIN_HASATTR_METHODDEF    \
    {"hasattr", (PyCFunction)builtin_hasattr, METH_VARARGS, builtin_hasattr__doc__},

static PyObject *
builtin_hasattr_impl(PyObject *module, PyObject *obj, PyObject *name);

static PyObject *
builtin_hasattr(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *obj;
    PyObject *name;

    if (!PyArg_UnpackTuple(args, "hasattr",
        2, 2,
        &obj, &name)) {
        goto exit;
    }
    return_value = builtin_hasattr_impl(module, obj, name);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_id__doc__,
"id($module, obj, /)\n"
"--\n"
"\n"
"Return the identity of an object.\n"
"\n"
"This is guaranteed to be unique among simultaneously existing objects.\n"
"(CPython uses the object\'s memory address.)");

#define BUILTIN_ID_METHODDEF    \
    {"id", (PyCFunction)builtin_id, METH_O, builtin_id__doc__},

PyDoc_STRVAR(builtin_setattr__doc__,
"setattr($module, obj, name, value, /)\n"
"--\n"
"\n"
"Sets the named attribute on the given object to the specified value.\n"
"\n"
"setattr(x, \'y\', v) is equivalent to ``x.y = v\'\'");

#define BUILTIN_SETATTR_METHODDEF    \
    {"setattr", (PyCFunction)builtin_setattr, METH_VARARGS, builtin_setattr__doc__},

static PyObject *
builtin_setattr_impl(PyObject *module, PyObject *obj, PyObject *name,
                     PyObject *value);

static PyObject *
builtin_setattr(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *obj;
    PyObject *name;
    PyObject *value;

    if (!PyArg_UnpackTuple(args, "setattr",
        3, 3,
        &obj, &name, &value)) {
        goto exit;
    }
    return_value = builtin_setattr_impl(module, obj, name, value);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_delattr__doc__,
"delattr($module, obj, name, /)\n"
"--\n"
"\n"
"Deletes the named attribute from the given object.\n"
"\n"
"delattr(x, \'y\') is equivalent to ``del x.y\'\'");

#define BUILTIN_DELATTR_METHODDEF    \
    {"delattr", (PyCFunction)builtin_delattr, METH_VARARGS, builtin_delattr__doc__},

static PyObject *
builtin_delattr_impl(PyObject *module, PyObject *obj, PyObject *name);

static PyObject *
builtin_delattr(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *obj;
    PyObject *name;

    if (!PyArg_UnpackTuple(args, "delattr",
        2, 2,
        &obj, &name)) {
        goto exit;
    }
    return_value = builtin_delattr_impl(module, obj, name);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_hash__doc__,
"hash($module, obj, /)\n"
"--\n"
"\n"
"Return the hash value for the given object.\n"
"\n"
"Two objects that compare equal must also have the same hash value, but the\n"
"reverse is not necessarily true.");

#define BUILTIN_HASH_METHODDEF    \
    {"hash", (PyCFunction)builtin_hash, METH_O, builtin_hash__doc__},

PyDoc_STRVAR(builtin_hex__doc__,
"hex($module, number, /)\n"
"--\n"
"\n"
"Return the hexadecimal representation of an integer.\n"
"\n"
"   >>> hex(12648430)\n"
"   \'0xc0ffee\'");

#define BUILTIN_HEX_METHODDEF    \
    {"hex", (PyCFunction)builtin_hex, METH_O, builtin_hex__doc__},

PyDoc_STRVAR(builtin_len__doc__,
"len($module, obj, /)\n"
"--\n"
"\n"
"Return the number of items in a container.");

#define BUILTIN_LEN_METHODDEF    \
    {"len", (PyCFunction)builtin_len, METH_O, builtin_len__doc__},

PyDoc_STRVAR(builtin_locals__doc__,
"locals($module, /)\n"
"--\n"
"\n"
"Return a dictionary containing the current scope\'s local variables.\n"
"\n"
"NOTE: Whether or not updates to this dictionary will affect name lookups in\n"
"the local scope and vice-versa is *implementation dependent* and not\n"
"covered by any backwards compatibility guarantees.");

#define BUILTIN_LOCALS_METHODDEF    \
    {"locals", (PyCFunction)builtin_locals, METH_NOARGS, builtin_locals__doc__},

static PyObject *
builtin_locals_impl(PyObject *module);

static PyObject *
builtin_locals(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return builtin_locals_impl(module);
}

PyDoc_STRVAR(builtin_oct__doc__,
"oct($module, number, /)\n"
"--\n"
"\n"
"Return the octal representation of an integer.\n"
"\n"
"   >>> oct(342391)\n"
"   \'0o1234567\'");

#define BUILTIN_OCT_METHODDEF    \
    {"oct", (PyCFunction)builtin_oct, METH_O, builtin_oct__doc__},

PyDoc_STRVAR(builtin_ord__doc__,
"ord($module, c, /)\n"
"--\n"
"\n"
"Return the Unicode code point for a one-character string.");

#define BUILTIN_ORD_METHODDEF    \
    {"ord", (PyCFunction)builtin_ord, METH_O, builtin_ord__doc__},

PyDoc_STRVAR(builtin_pow__doc__,
"pow($module, x, y, z=None, /)\n"
"--\n"
"\n"
"Equivalent to x**y (with two arguments) or x**y % z (with three arguments)\n"
"\n"
"Some types, such as ints, are able to use a more efficient algorithm when\n"
"invoked using the three argument form.");

#define BUILTIN_POW_METHODDEF    \
    {"pow", (PyCFunction)builtin_pow, METH_VARARGS, builtin_pow__doc__},

static PyObject *
builtin_pow_impl(PyObject *module, PyObject *x, PyObject *y, PyObject *z);

static PyObject *
builtin_pow(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;
    PyObject *z = Py_None;

    if (!PyArg_UnpackTuple(args, "pow",
        2, 3,
        &x, &y, &z)) {
        goto exit;
    }
    return_value = builtin_pow_impl(module, x, y, z);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_input__doc__,
"input($module, prompt=None, /)\n"
"--\n"
"\n"
"Read a string from standard input.  The trailing newline is stripped.\n"
"\n"
"The prompt string, if given, is printed to standard output without a\n"
"trailing newline before reading input.\n"
"\n"
"If the user hits EOF (*nix: Ctrl-D, Windows: Ctrl-Z+Return), raise EOFError.\n"
"On *nix systems, readline is used if available.");

#define BUILTIN_INPUT_METHODDEF    \
    {"input", (PyCFunction)builtin_input, METH_VARARGS, builtin_input__doc__},

static PyObject *
builtin_input_impl(PyObject *module, PyObject *prompt);

static PyObject *
builtin_input(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *prompt = NULL;

    if (!PyArg_UnpackTuple(args, "input",
        0, 1,
        &prompt)) {
        goto exit;
    }
    return_value = builtin_input_impl(module, prompt);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_repr__doc__,
"repr($module, obj, /)\n"
"--\n"
"\n"
"Return the canonical string representation of the object.\n"
"\n"
"For many object types, including most builtins, eval(repr(obj)) == obj.");

#define BUILTIN_REPR_METHODDEF    \
    {"repr", (PyCFunction)builtin_repr, METH_O, builtin_repr__doc__},

PyDoc_STRVAR(builtin_sum__doc__,
"sum($module, iterable, start=0, /)\n"
"--\n"
"\n"
"Return the sum of a \'start\' value (default: 0) plus an iterable of numbers\n"
"\n"
"When the iterable is empty, return the start value.\n"
"This function is intended specifically for use with numeric values and may\n"
"reject non-numeric types.");

#define BUILTIN_SUM_METHODDEF    \
    {"sum", (PyCFunction)builtin_sum, METH_VARARGS, builtin_sum__doc__},

static PyObject *
builtin_sum_impl(PyObject *module, PyObject *iterable, PyObject *start);

static PyObject *
builtin_sum(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *iterable;
    PyObject *start = NULL;

    if (!PyArg_UnpackTuple(args, "sum",
        1, 2,
        &iterable, &start)) {
        goto exit;
    }
    return_value = builtin_sum_impl(module, iterable, start);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_isinstance__doc__,
"isinstance($module, obj, class_or_tuple, /)\n"
"--\n"
"\n"
"Return whether an object is an instance of a class or of a subclass thereof.\n"
"\n"
"A tuple, as in ``isinstance(x, (A, B, ...))``, may be given as the target to\n"
"check against. This is equivalent to ``isinstance(x, A) or isinstance(x, B)\n"
"or ...`` etc.");

#define BUILTIN_ISINSTANCE_METHODDEF    \
    {"isinstance", (PyCFunction)builtin_isinstance, METH_VARARGS, builtin_isinstance__doc__},

static PyObject *
builtin_isinstance_impl(PyObject *module, PyObject *obj,
                        PyObject *class_or_tuple);

static PyObject *
builtin_isinstance(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *obj;
    PyObject *class_or_tuple;

    if (!PyArg_UnpackTuple(args, "isinstance",
        2, 2,
        &obj, &class_or_tuple)) {
        goto exit;
    }
    return_value = builtin_isinstance_impl(module, obj, class_or_tuple);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_issubclass__doc__,
"issubclass($module, cls, class_or_tuple, /)\n"
"--\n"
"\n"
"Return whether \'cls\' is a derived from another class or is the same class.\n"
"\n"
"A tuple, as in ``issubclass(x, (A, B, ...))``, may be given as the target to\n"
"check against. This is equivalent to ``issubclass(x, A) or issubclass(x, B)\n"
"or ...`` etc.");

#define BUILTIN_ISSUBCLASS_METHODDEF    \
    {"issubclass", (PyCFunction)builtin_issubclass, METH_VARARGS, builtin_issubclass__doc__},

static PyObject *
builtin_issubclass_impl(PyObject *module, PyObject *cls,
                        PyObject *class_or_tuple);

static PyObject *
builtin_issubclass(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *cls;
    PyObject *class_or_tuple;

    if (!PyArg_UnpackTuple(args, "issubclass",
        2, 2,
        &cls, &class_or_tuple)) {
        goto exit;
    }
    return_value = builtin_issubclass_impl(module, cls, class_or_tuple);

exit:
    return return_value;
}
/*[clinic end generated code: output=63483deb75805f7c input=a9049054013a1b77]*/
