#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

/* Always enable assertions */
#undef NDEBUG

#define PY_SSIZE_T_CLEAN

#include "Python.h"

#include "clinic/_testclinic.c.h"


/* Pack arguments to a tuple, implicitly increase all the arguments' refcount.
 * NULL arguments will be replaced to Py_None. */
static PyObject *
pack_arguments_newref(int argc, ...)
{
    assert(!PyErr_Occurred());
    PyObject *tuple = PyTuple_New(argc);
    if (!tuple) {
        return NULL;
    }

    va_list vargs;
    va_start(vargs, argc);
    for (int i = 0; i < argc; i++) {
        PyObject *arg = va_arg(vargs, PyObject *);
        if (arg) {
            if (_PyObject_IsFreed(arg)) {
                PyErr_Format(PyExc_AssertionError,
                             "argument %d at %p is freed or corrupted!",
                             i, arg);
                va_end(vargs);
                Py_DECREF(tuple);
                return NULL;
            }
        }
        else {
            arg = Py_None;
        }
        PyTuple_SET_ITEM(tuple, i, Py_NewRef(arg));
    }
    va_end(vargs);
    return tuple;
}

/* Pack arguments to a tuple.
 * `wrapper` is function which converts primitive type to PyObject.
 * `arg_type` is type that arguments should be converted to before wrapped. */
#define RETURN_PACKED_ARGS(argc, wrapper, arg_type, ...) do { \
        assert(!PyErr_Occurred()); \
        arg_type in[argc] = {__VA_ARGS__}; \
        PyObject *out[argc] = {NULL,}; \
        for (int _i = 0; _i < argc; _i++) { \
            out[_i] = wrapper(in[_i]); \
            assert(out[_i] || PyErr_Occurred()); \
            if (!out[_i]) { \
                for (int _j = 0; _j < _i; _j++) { \
                    Py_DECREF(out[_j]); \
                } \
                return NULL; \
            } \
        } \
        PyObject *tuple = PyTuple_New(argc); \
        if (!tuple) { \
            for (int _i = 0; _i < argc; _i++) { \
                Py_DECREF(out[_i]); \
            } \
            return NULL; \
        } \
        for (int _i = 0; _i < argc; _i++) { \
            PyTuple_SET_ITEM(tuple, _i, out[_i]); \
        } \
        return tuple; \
    } while (0)


/*[clinic input]
module  _testclinic
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=d4981b80d6efdb12]*/


/*[clinic input]
test_empty_function

[clinic start generated code]*/

static PyObject *
test_empty_function_impl(PyObject *module)
/*[clinic end generated code: output=0f8aeb3ddced55cb input=0dd7048651ad4ae4]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
objects_converter

    a: object
    b: object = NULL
    /

[clinic start generated code]*/

static PyObject *
objects_converter_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=3f9c9415ec86c695 input=1533b1bd94187de4]*/
{
    return pack_arguments_newref(2, a, b);
}


/*[clinic input]
bytes_object_converter

    a: PyBytesObject
    /

[clinic start generated code]*/

static PyObject *
bytes_object_converter_impl(PyObject *module, PyBytesObject *a)
/*[clinic end generated code: output=7732da869d74b784 input=94211751e7996236]*/
{
    if (!PyBytes_Check(a)) {
        PyErr_SetString(PyExc_AssertionError,
                        "argument a is not a PyBytesObject");
        return NULL;
    }
    return pack_arguments_newref(1, a);
}


/*[clinic input]
byte_array_object_converter

    a: PyByteArrayObject
    /

[clinic start generated code]*/

static PyObject *
byte_array_object_converter_impl(PyObject *module, PyByteArrayObject *a)
/*[clinic end generated code: output=51f15c76f302b1f7 input=b04d253db51c6f56]*/
{
    if (!PyByteArray_Check(a)) {
        PyErr_SetString(PyExc_AssertionError,
                        "argument a is not a PyByteArrayObject");
        return NULL;
    }
    return pack_arguments_newref(1, a);
}


/*[clinic input]
unicode_converter

    a: unicode
    /

[clinic start generated code]*/

static PyObject *
unicode_converter_impl(PyObject *module, PyObject *a)
/*[clinic end generated code: output=1b4a4adbb6ac6e34 input=de7b5adbf07435ba]*/
{
    if (!PyUnicode_Check(a)) {
        PyErr_SetString(PyExc_AssertionError,
                        "argument a is not a unicode object");
        return NULL;
    }
    return pack_arguments_newref(1, a);
}


/*[clinic input]
bool_converter

    a: bool = True
    b: bool(accept={object}) = True
    c: bool(accept={int}) = True
    /

[clinic start generated code]*/

static PyObject *
bool_converter_impl(PyObject *module, int a, int b, int c)
/*[clinic end generated code: output=17005b0c29afd590 input=7f6537705b2f32f4]*/
{
    PyObject *obj_a = a ? Py_True : Py_False;
    PyObject *obj_b = b ? Py_True : Py_False;
    PyObject *obj_c = c ? Py_True : Py_False;
    return pack_arguments_newref(3, obj_a, obj_b, obj_c);
}


/*[clinic input]
char_converter

    a: char = b'A'
    b: char = b'\a'
    c: char = b'\b'
    d: char = b'\t'
    e: char = b'\n'
    f: char = b'\v'
    g: char = b'\f'
    h: char = b'\r'
    i: char = b'"'
    j: char = b"'"
    k: char = b'?'
    l: char = b'\\'
    m: char = b'\000'
    n: char = b'\377'
    /

[clinic start generated code]*/

static PyObject *
char_converter_impl(PyObject *module, char a, char b, char c, char d, char e,
                    char f, char g, char h, char i, char j, char k, char l,
                    char m, char n)
/*[clinic end generated code: output=f929dbd2e55a9871 input=b601bc5bc7fe85e3]*/
{
    RETURN_PACKED_ARGS(14, PyLong_FromUnsignedLong, unsigned char,
                       a, b, c, d, e, f, g, h, i, j, k, l, m, n);
}


/*[clinic input]
unsigned_char_converter

    a: unsigned_char = 12
    b: unsigned_char(bitwise=False) = 34
    c: unsigned_char(bitwise=True) = 56
    /

[clinic start generated code]*/

static PyObject *
unsigned_char_converter_impl(PyObject *module, unsigned char a,
                             unsigned char b, unsigned char c)
/*[clinic end generated code: output=490af3b39ce0b199 input=e859502fbe0b3185]*/
{
    RETURN_PACKED_ARGS(3, PyLong_FromUnsignedLong, unsigned char, a, b, c);
}


/*[clinic input]
short_converter

    a: short = 12
    /

[clinic start generated code]*/

static PyObject *
short_converter_impl(PyObject *module, short a)
/*[clinic end generated code: output=1ebb7ddb64248988 input=b4e2309a66f650ae]*/
{
    RETURN_PACKED_ARGS(1, PyLong_FromLong, long, a);
}


/*[clinic input]
unsigned_short_converter

    a: unsigned_short = 12
    b: unsigned_short(bitwise=False) = 34
    c: unsigned_short(bitwise=True) = 56
    /

[clinic start generated code]*/

static PyObject *
unsigned_short_converter_impl(PyObject *module, unsigned short a,
                              unsigned short b, unsigned short c)
/*[clinic end generated code: output=5f92cc72fc8707a7 input=9d15cd11e741d0c6]*/
{
    RETURN_PACKED_ARGS(3, PyLong_FromUnsignedLong, unsigned long, a, b, c);
}


/*[clinic input]
int_converter

    a: int = 12
    b: int(accept={int}) = 34
    c: int(accept={str}) = 45
    /

[clinic start generated code]*/

static PyObject *
int_converter_impl(PyObject *module, int a, int b, int c)
/*[clinic end generated code: output=8e56b59be7d0c306 input=a1dbc6344853db7a]*/
{
    RETURN_PACKED_ARGS(3, PyLong_FromLong, long, a, b, c);
}


/*[clinic input]
unsigned_int_converter

    a: unsigned_int = 12
    b: unsigned_int(bitwise=False) = 34
    c: unsigned_int(bitwise=True) = 56
    /

[clinic start generated code]*/

static PyObject *
unsigned_int_converter_impl(PyObject *module, unsigned int a, unsigned int b,
                            unsigned int c)
/*[clinic end generated code: output=399a57a05c494cc7 input=8427ed9a3f96272d]*/
{
    RETURN_PACKED_ARGS(3, PyLong_FromUnsignedLong, unsigned long, a, b, c);
}


/*[clinic input]
long_converter

    a: long = 12
    /

[clinic start generated code]*/

static PyObject *
long_converter_impl(PyObject *module, long a)
/*[clinic end generated code: output=9663d936a652707a input=84ad0ef28f24bd85]*/
{
    RETURN_PACKED_ARGS(1, PyLong_FromLong, long, a);
}


/*[clinic input]
unsigned_long_converter

    a: unsigned_long = 12
    b: unsigned_long(bitwise=False) = 34
    c: unsigned_long(bitwise=True) = 56
    /

[clinic start generated code]*/

static PyObject *
unsigned_long_converter_impl(PyObject *module, unsigned long a,
                             unsigned long b, unsigned long c)
/*[clinic end generated code: output=120b82ea9ebd93a8 input=440dd6f1817f5d91]*/
{
    RETURN_PACKED_ARGS(3, PyLong_FromUnsignedLong, unsigned long, a, b, c);
}


/*[clinic input]
long_long_converter

    a: long_long = 12
    /

[clinic start generated code]*/

static PyObject *
long_long_converter_impl(PyObject *module, long long a)
/*[clinic end generated code: output=5fb5f2220770c3e1 input=730fcb3eecf4d993]*/
{
    RETURN_PACKED_ARGS(1, PyLong_FromLongLong, long long, a);
}


/*[clinic input]
unsigned_long_long_converter

    a: unsigned_long_long = 12
    b: unsigned_long_long(bitwise=False) = 34
    c: unsigned_long_long(bitwise=True) = 56
    /

[clinic start generated code]*/

static PyObject *
unsigned_long_long_converter_impl(PyObject *module, unsigned long long a,
                                  unsigned long long b, unsigned long long c)
/*[clinic end generated code: output=65b7273e63501762 input=300737b0bdb230e9]*/
{
    RETURN_PACKED_ARGS(3, PyLong_FromUnsignedLongLong, unsigned long long,
                       a, b, c);
}


/*[clinic input]
py_ssize_t_converter

    a: Py_ssize_t = 12
    b: Py_ssize_t(accept={int}) = 34
    c: Py_ssize_t(accept={int, NoneType}) = 56
    /

[clinic start generated code]*/

static PyObject *
py_ssize_t_converter_impl(PyObject *module, Py_ssize_t a, Py_ssize_t b,
                          Py_ssize_t c)
/*[clinic end generated code: output=ce252143e0ed0372 input=76d0f342e9317a1f]*/
{
    RETURN_PACKED_ARGS(3, PyLong_FromSsize_t, Py_ssize_t, a, b, c);
}


/*[clinic input]
slice_index_converter

    a: slice_index = 12
    b: slice_index(accept={int}) = 34
    c: slice_index(accept={int, NoneType}) = 56
    /

[clinic start generated code]*/

static PyObject *
slice_index_converter_impl(PyObject *module, Py_ssize_t a, Py_ssize_t b,
                           Py_ssize_t c)
/*[clinic end generated code: output=923c6cac77666a6b input=64f99f3f83265e47]*/
{
    RETURN_PACKED_ARGS(3, PyLong_FromSsize_t, Py_ssize_t, a, b, c);
}


/*[clinic input]
size_t_converter

    a: size_t = 12
    /

[clinic start generated code]*/

static PyObject *
size_t_converter_impl(PyObject *module, size_t a)
/*[clinic end generated code: output=412b5b7334ab444d input=83ae7d9171fbf208]*/
{
    RETURN_PACKED_ARGS(1, PyLong_FromSize_t, size_t, a);
}


/*[clinic input]
float_converter

    a: float = 12.5
    /

[clinic start generated code]*/

static PyObject *
float_converter_impl(PyObject *module, float a)
/*[clinic end generated code: output=1c98f64f2cf1d55c input=a625b59ad68047d8]*/
{
    RETURN_PACKED_ARGS(1, PyFloat_FromDouble, double, a);
}


/*[clinic input]
double_converter

    a: double = 12.5
    /

[clinic start generated code]*/

static PyObject *
double_converter_impl(PyObject *module, double a)
/*[clinic end generated code: output=a4e8532d284d035d input=098df188f24e7c62]*/
{
    RETURN_PACKED_ARGS(1, PyFloat_FromDouble, double, a);
}


/*[clinic input]
py_complex_converter

    a: Py_complex
    /

[clinic start generated code]*/

static PyObject *
py_complex_converter_impl(PyObject *module, Py_complex a)
/*[clinic end generated code: output=9e6ca2eb53b14846 input=e9148a8ca1dbf195]*/
{
    RETURN_PACKED_ARGS(1, PyComplex_FromCComplex, Py_complex, a);
}


/*[clinic input]
str_converter

    a: str = "a"
    b: str(accept={robuffer}) = "b"
    c: str(accept={robuffer, str}, zeroes=True) = "c"
    /

[clinic start generated code]*/

static PyObject *
str_converter_impl(PyObject *module, const char *a, const char *b,
                   const char *c, Py_ssize_t c_length)
/*[clinic end generated code: output=475bea40548c8cd6 input=bff2656c92ee25de]*/
{
    assert(!PyErr_Occurred());
    PyObject *out[3] = {NULL,};
    int i = 0;
    PyObject *arg;

    arg = PyUnicode_FromString(a);
    assert(arg || PyErr_Occurred());
    if (!arg) {
        goto error;
    }
    out[i++] = arg;

    arg = PyUnicode_FromString(b);
    assert(arg || PyErr_Occurred());
    if (!arg) {
        goto error;
    }
    out[i++] = arg;

    arg = PyUnicode_FromStringAndSize(c, c_length);
    assert(arg || PyErr_Occurred());
    if (!arg) {
        goto error;
    }
    out[i++] = arg;

    PyObject *tuple = PyTuple_New(3);
    if (!tuple) {
        goto error;
    }
    for (int j = 0; j < 3; j++) {
        PyTuple_SET_ITEM(tuple, j, out[j]);
    }
    return tuple;

error:
    for (int j = 0; j < i; j++) {
        Py_DECREF(out[j]);
    }
    return NULL;
}


/*[clinic input]
str_converter_encoding

    a: str(encoding="idna")
    b: str(encoding="idna", accept={bytes, bytearray, str})
    c: str(encoding="idna", accept={bytes, bytearray, str}, zeroes=True)
    /

[clinic start generated code]*/

static PyObject *
str_converter_encoding_impl(PyObject *module, char *a, char *b, char *c,
                            Py_ssize_t c_length)
/*[clinic end generated code: output=af68766049248a1c input=0c5cf5159d0e870d]*/
{
    assert(!PyErr_Occurred());
    PyObject *out[3] = {NULL,};
    int i = 0;
    PyObject *arg;

    arg = PyUnicode_FromString(a);
    assert(arg || PyErr_Occurred());
    if (!arg) {
        goto error;
    }
    out[i++] = arg;

    arg = PyUnicode_FromString(b);
    assert(arg || PyErr_Occurred());
    if (!arg) {
        goto error;
    }
    out[i++] = arg;

    arg = PyUnicode_FromStringAndSize(c, c_length);
    assert(arg || PyErr_Occurred());
    if (!arg) {
        goto error;
    }
    out[i++] = arg;

    PyObject *tuple = PyTuple_New(3);
    if (!tuple) {
        goto error;
    }
    for (int j = 0; j < 3; j++) {
        PyTuple_SET_ITEM(tuple, j, out[j]);
    }
    return tuple;

error:
    for (int j = 0; j < i; j++) {
        Py_DECREF(out[j]);
    }
    return NULL;
}


static PyObject *
bytes_from_buffer(Py_buffer *buf)
{
    PyObject *bytes_obj = PyBytes_FromStringAndSize(NULL, buf->len);
    if (!bytes_obj) {
        return NULL;
    }
    void *bytes_obj_buf = ((PyBytesObject *)bytes_obj)->ob_sval;
    if (PyBuffer_ToContiguous(bytes_obj_buf, buf, buf->len, 'C') < 0) {
        Py_DECREF(bytes_obj);
        return NULL;
    }
    return bytes_obj;
}

/*[clinic input]
py_buffer_converter

    a: Py_buffer(accept={str, buffer, NoneType})
    b: Py_buffer(accept={rwbuffer})
    /

[clinic start generated code]*/

static PyObject *
py_buffer_converter_impl(PyObject *module, Py_buffer *a, Py_buffer *b)
/*[clinic end generated code: output=52fb13311e3d6d03 input=775de727de5c7421]*/
{
    RETURN_PACKED_ARGS(2, bytes_from_buffer, Py_buffer *, a, b);
}


/*[clinic input]
keywords

    a: object
    b: object

[clinic start generated code]*/

static PyObject *
keywords_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=850aaed53e26729e input=f44b89e718c1a93b]*/
{
    return pack_arguments_newref(2, a, b);
}


/*[clinic input]
keywords_kwonly

    a: object
    *
    b: object

[clinic start generated code]*/

static PyObject *
keywords_kwonly_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=a45c48241da584dc input=1f08e39c3312b015]*/
{
    return pack_arguments_newref(2, a, b);
}


/*[clinic input]
keywords_opt

    a: object
    b: object = None
    c: object = None

[clinic start generated code]*/

static PyObject *
keywords_opt_impl(PyObject *module, PyObject *a, PyObject *b, PyObject *c)
/*[clinic end generated code: output=25e4b67d91c76a66 input=b0ba0e4f04904556]*/
{
    return pack_arguments_newref(3, a, b, c);
}


/*[clinic input]
keywords_opt_kwonly

    a: object
    b: object = None
    *
    c: object = None
    d: object = None

[clinic start generated code]*/

static PyObject *
keywords_opt_kwonly_impl(PyObject *module, PyObject *a, PyObject *b,
                         PyObject *c, PyObject *d)
/*[clinic end generated code: output=6aa5b655a6e9aeb0 input=f79da689d6c51076]*/
{
    return pack_arguments_newref(4, a, b, c, d);
}


/*[clinic input]
keywords_kwonly_opt

    a: object
    *
    b: object = None
    c: object = None

[clinic start generated code]*/

static PyObject *
keywords_kwonly_opt_impl(PyObject *module, PyObject *a, PyObject *b,
                         PyObject *c)
/*[clinic end generated code: output=707f78eb0f55c2b1 input=e0fa1a0e46dca791]*/
{
    return pack_arguments_newref(3, a, b, c);
}


/*[clinic input]
posonly_keywords

    a: object
    /
    b: object

[clinic start generated code]*/

static PyObject *
posonly_keywords_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=6ac88f4a5f0bfc8d input=fde0a2f79fe82b06]*/
{
    return pack_arguments_newref(2, a, b);
}


/*[clinic input]
posonly_kwonly

    a: object
    /
    *
    b: object

[clinic start generated code]*/

static PyObject *
posonly_kwonly_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=483e6790d3482185 input=78b3712768da9a19]*/
{
    return pack_arguments_newref(2, a, b);
}


/*[clinic input]
posonly_keywords_kwonly

    a: object
    /
    b: object
    *
    c: object

[clinic start generated code]*/

static PyObject *
posonly_keywords_kwonly_impl(PyObject *module, PyObject *a, PyObject *b,
                             PyObject *c)
/*[clinic end generated code: output=2fae573e8cc3fad8 input=a1ad5d2295eb803c]*/
{
    return pack_arguments_newref(3, a, b, c);
}


/*[clinic input]
posonly_keywords_opt

    a: object
    /
    b: object
    c: object = None
    d: object = None

[clinic start generated code]*/

static PyObject *
posonly_keywords_opt_impl(PyObject *module, PyObject *a, PyObject *b,
                          PyObject *c, PyObject *d)
/*[clinic end generated code: output=f5eb66241bcf68fb input=51c10de2a120e279]*/
{
    return pack_arguments_newref(4, a, b, c, d);
}


/*[clinic input]
posonly_opt_keywords_opt

    a: object
    b: object = None
    /
    c: object = None
    d: object = None

[clinic start generated code]*/

static PyObject *
posonly_opt_keywords_opt_impl(PyObject *module, PyObject *a, PyObject *b,
                              PyObject *c, PyObject *d)
/*[clinic end generated code: output=d54a30e549296ffd input=f408a1de7dfaf31f]*/
{
    return pack_arguments_newref(4, a, b, c, d);
}


/*[clinic input]
posonly_kwonly_opt

    a: object
    /
    *
    b: object
    c: object = None
    d: object = None

[clinic start generated code]*/

static PyObject *
posonly_kwonly_opt_impl(PyObject *module, PyObject *a, PyObject *b,
                        PyObject *c, PyObject *d)
/*[clinic end generated code: output=a20503fe36b4fd62 input=3494253975272f52]*/
{
    return pack_arguments_newref(4, a, b, c, d);
}


/*[clinic input]
posonly_opt_kwonly_opt

    a: object
    b: object = None
    /
    *
    c: object = None
    d: object = None

[clinic start generated code]*/

static PyObject *
posonly_opt_kwonly_opt_impl(PyObject *module, PyObject *a, PyObject *b,
                            PyObject *c, PyObject *d)
/*[clinic end generated code: output=64f3204a3a0413b6 input=d17516581e478412]*/
{
    return pack_arguments_newref(4, a, b, c, d);
}


/*[clinic input]
posonly_keywords_kwonly_opt

    a: object
    /
    b: object
    *
    c: object
    d: object = None
    e: object = None

[clinic start generated code]*/

static PyObject *
posonly_keywords_kwonly_opt_impl(PyObject *module, PyObject *a, PyObject *b,
                                 PyObject *c, PyObject *d, PyObject *e)
/*[clinic end generated code: output=dbd7e7ddd6257fa0 input=33529f29e97e5adb]*/
{
    return pack_arguments_newref(5, a, b, c, d, e);
}


/*[clinic input]
posonly_keywords_opt_kwonly_opt

    a: object
    /
    b: object
    c: object = None
    *
    d: object = None
    e: object = None

[clinic start generated code]*/

static PyObject *
posonly_keywords_opt_kwonly_opt_impl(PyObject *module, PyObject *a,
                                     PyObject *b, PyObject *c, PyObject *d,
                                     PyObject *e)
/*[clinic end generated code: output=775d12ae44653045 input=4d4cc62f11441301]*/
{
    return pack_arguments_newref(5, a, b, c, d, e);
}


/*[clinic input]
posonly_opt_keywords_opt_kwonly_opt

    a: object
    b: object = None
    /
    c: object = None
    *
    d: object = None

[clinic start generated code]*/

static PyObject *
posonly_opt_keywords_opt_kwonly_opt_impl(PyObject *module, PyObject *a,
                                         PyObject *b, PyObject *c,
                                         PyObject *d)
/*[clinic end generated code: output=40c6dc422591eade input=3964960a68622431]*/
{
    return pack_arguments_newref(4, a, b, c, d);
}


/*[clinic input]
keyword_only_parameter

    *
    a: object

[clinic start generated code]*/

static PyObject *
keyword_only_parameter_impl(PyObject *module, PyObject *a)
/*[clinic end generated code: output=c454b6ce98232787 input=8d2868b8d0b27bdb]*/
{
    return pack_arguments_newref(1, a);
}


/*[clinic input]
posonly_vararg

    a: object
    /
    b: object
    *args: object

[clinic start generated code]*/

static PyObject *
posonly_vararg_impl(PyObject *module, PyObject *a, PyObject *b,
                    PyObject *args)
/*[clinic end generated code: output=ee6713acda6b954e input=783427fe7ec2b67a]*/
{
    return pack_arguments_newref(3, a, b, args);
}


/*[clinic input]
vararg_and_posonly

    a: object
    *args: object
    /

[clinic start generated code]*/

static PyObject *
vararg_and_posonly_impl(PyObject *module, PyObject *a, PyObject *args)
/*[clinic end generated code: output=42792f799465a14d input=defe017b19ba52e8]*/
{
    return pack_arguments_newref(2, a, args);
}


/*[clinic input]
vararg

    a: object
    *args: object

[clinic start generated code]*/

static PyObject *
vararg_impl(PyObject *module, PyObject *a, PyObject *args)
/*[clinic end generated code: output=91ab7a0efc52dd5e input=02c0f772d05f591e]*/
{
    return pack_arguments_newref(2, a, args);
}


/*[clinic input]
vararg_with_default

    a: object
    *args: object
    b: bool = False

[clinic start generated code]*/

static PyObject *
vararg_with_default_impl(PyObject *module, PyObject *a, PyObject *args,
                         int b)
/*[clinic end generated code: output=182c01035958ce92 input=68cafa6a79f89e36]*/
{
    PyObject *obj_b = b ? Py_True : Py_False;
    return pack_arguments_newref(3, a, args, obj_b);
}


/*[clinic input]
vararg_with_only_defaults

    *args: object
    b: object = None

[clinic start generated code]*/

static PyObject *
vararg_with_only_defaults_impl(PyObject *module, PyObject *args, PyObject *b)
/*[clinic end generated code: output=c06b1826d91f2f7b input=678c069bc67550e1]*/
{
    return pack_arguments_newref(2, args, b);
}



/*[clinic input]
gh_32092_oob

    pos1: object
    pos2: object
    *varargs: object
    kw1: object = None
    kw2: object = None

Proof-of-concept of GH-32092 OOB bug.

[clinic start generated code]*/

static PyObject *
gh_32092_oob_impl(PyObject *module, PyObject *pos1, PyObject *pos2,
                  PyObject *varargs, PyObject *kw1, PyObject *kw2)
/*[clinic end generated code: output=ee259c130054653f input=46d15c881608f8ff]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
gh_32092_kw_pass

    pos: object
    *args: object
    kw: object = None

Proof-of-concept of GH-32092 keyword args passing bug.

[clinic start generated code]*/

static PyObject *
gh_32092_kw_pass_impl(PyObject *module, PyObject *pos, PyObject *args,
                      PyObject *kw)
/*[clinic end generated code: output=4a2bbe4f7c8604e9 input=5c0bd5b9079a0cce]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
gh_99233_refcount

    *args: object
    /

Proof-of-concept of GH-99233 refcount error bug.

[clinic start generated code]*/

static PyObject *
gh_99233_refcount_impl(PyObject *module, PyObject *args)
/*[clinic end generated code: output=585855abfbca9a7f input=85f5fb47ac91a626]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
gh_99240_double_free

    a: str(encoding="idna")
    b: str(encoding="idna")
    /

Proof-of-concept of GH-99240 double-free bug.

[clinic start generated code]*/

static PyObject *
gh_99240_double_free_impl(PyObject *module, char *a, char *b)
/*[clinic end generated code: output=586dc714992fe2ed input=23db44aa91870fc7]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
class _testclinic.TestClass "PyObject *" "PyObject"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=668a591c65bec947]*/

/*[clinic input]
_testclinic.TestClass.meth_method_no_params
    cls: defining_class
    /
[clinic start generated code]*/

static PyObject *
_testclinic_TestClass_meth_method_no_params_impl(PyObject *self,
                                                 PyTypeObject *cls)
/*[clinic end generated code: output=c140f100080c2fc8 input=6bd34503d11c63c1]*/
{
    Py_RETURN_NONE;
}

static struct PyMethodDef test_class_methods[] = {
    _TESTCLINIC_TESTCLASS_METH_METHOD_NO_PARAMS_METHODDEF
    {NULL, NULL}
};

static PyTypeObject TestClass = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testclinic.TestClass",
    .tp_basicsize = sizeof(PyObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_methods = test_class_methods,
};


static PyMethodDef tester_methods[] = {
    TEST_EMPTY_FUNCTION_METHODDEF
    OBJECTS_CONVERTER_METHODDEF
    BYTES_OBJECT_CONVERTER_METHODDEF
    BYTE_ARRAY_OBJECT_CONVERTER_METHODDEF
    UNICODE_CONVERTER_METHODDEF
    BOOL_CONVERTER_METHODDEF
    CHAR_CONVERTER_METHODDEF
    UNSIGNED_CHAR_CONVERTER_METHODDEF
    SHORT_CONVERTER_METHODDEF
    UNSIGNED_SHORT_CONVERTER_METHODDEF
    INT_CONVERTER_METHODDEF
    UNSIGNED_INT_CONVERTER_METHODDEF
    LONG_CONVERTER_METHODDEF
    UNSIGNED_LONG_CONVERTER_METHODDEF
    LONG_LONG_CONVERTER_METHODDEF
    UNSIGNED_LONG_LONG_CONVERTER_METHODDEF
    PY_SSIZE_T_CONVERTER_METHODDEF
    SLICE_INDEX_CONVERTER_METHODDEF
    SIZE_T_CONVERTER_METHODDEF
    FLOAT_CONVERTER_METHODDEF
    DOUBLE_CONVERTER_METHODDEF
    PY_COMPLEX_CONVERTER_METHODDEF
    STR_CONVERTER_METHODDEF
    STR_CONVERTER_ENCODING_METHODDEF
    PY_BUFFER_CONVERTER_METHODDEF
    KEYWORDS_METHODDEF
    KEYWORDS_KWONLY_METHODDEF
    KEYWORDS_OPT_METHODDEF
    KEYWORDS_OPT_KWONLY_METHODDEF
    KEYWORDS_KWONLY_OPT_METHODDEF
    POSONLY_KEYWORDS_METHODDEF
    POSONLY_KWONLY_METHODDEF
    POSONLY_KEYWORDS_KWONLY_METHODDEF
    POSONLY_KEYWORDS_OPT_METHODDEF
    POSONLY_OPT_KEYWORDS_OPT_METHODDEF
    POSONLY_KWONLY_OPT_METHODDEF
    POSONLY_OPT_KWONLY_OPT_METHODDEF
    POSONLY_KEYWORDS_KWONLY_OPT_METHODDEF
    POSONLY_KEYWORDS_OPT_KWONLY_OPT_METHODDEF
    POSONLY_OPT_KEYWORDS_OPT_KWONLY_OPT_METHODDEF
    KEYWORD_ONLY_PARAMETER_METHODDEF
    POSONLY_VARARG_METHODDEF
    VARARG_AND_POSONLY_METHODDEF
    VARARG_METHODDEF
    VARARG_WITH_DEFAULT_METHODDEF
    VARARG_WITH_ONLY_DEFAULTS_METHODDEF
    GH_32092_OOB_METHODDEF
    GH_32092_KW_PASS_METHODDEF
    GH_99233_REFCOUNT_METHODDEF
    GH_99240_DOUBLE_FREE_METHODDEF
    {NULL, NULL}
};

static struct PyModuleDef _testclinic_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_testclinic",
    .m_size = 0,
    .m_methods = tester_methods,
};

PyMODINIT_FUNC
PyInit__testclinic(void)
{
    PyObject *m = PyModule_Create(&_testclinic_module);
    if (m == NULL) {
        return NULL;
    }
    if (PyModule_AddType(m, &TestClass) < 0) {
        goto error;
    }
    return m;

error:
    Py_DECREF(m);
    return NULL;
}

#undef RETURN_PACKED_ARGS
