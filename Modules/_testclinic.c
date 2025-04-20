#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

/* Always enable assertions */
#undef NDEBUG

#include "Python.h"
#include "pycore_object.h"        // _PyObject_IsFreed()


// Used for clone_with_conv_f1 and clone_with_conv_v2
typedef struct {
    const char *name;
} custom_t;

static int
custom_converter(PyObject *obj, custom_t *val)
{
    return 1;
}


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

static PyObject *
pack_arguments_2pos_varpos(PyObject *a, PyObject *b,
                           PyObject * const *args, Py_ssize_t args_length)
/*[clinic end generated code: output=267032f41bd039cc input=86ee3064b7853e86]*/
{
    PyObject *tuple = _PyTuple_FromArray(args, args_length);
    if (tuple == NULL) {
        return NULL;
    }
    PyObject *result = pack_arguments_newref(3, a, b, tuple);
    Py_DECREF(tuple);
    return result;
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
bool_converter_c_default

    a: bool = True
    b: bool = False
    c: bool(c_default="-2") = True
    d: bool(c_default="-3") = x
    /

[clinic start generated code]*/

static PyObject *
bool_converter_c_default_impl(PyObject *module, int a, int b, int c, int d)
/*[clinic end generated code: output=cf204382e1e4c30c input=185786302ab84081]*/
{
    return Py_BuildValue("iiii", a, b, c, d);
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
varpos

    *args: tuple

[clinic start generated code]*/

static PyObject *
varpos_impl(PyObject *module, PyObject *args)
/*[clinic end generated code: output=7b0b9545872bdca4 input=ae7ccecd997aaff4]*/
{
    return Py_NewRef(args);
}


/*[clinic input]
posonly_varpos

    a: object
    b: object
    /
    *args: tuple

[clinic start generated code]*/

static PyObject *
posonly_varpos_impl(PyObject *module, PyObject *a, PyObject *b,
                    PyObject *args)
/*[clinic end generated code: output=5dae5eb2a0d623cd input=6dd74417b62cbe67]*/
{
    return pack_arguments_newref(3, a, b, args);
}


/*[clinic input]
posonly_req_opt_varpos

    a: object
    b: object = False
    /
    *args: tuple

[clinic start generated code]*/

static PyObject *
posonly_req_opt_varpos_impl(PyObject *module, PyObject *a, PyObject *b,
                            PyObject *args)
/*[clinic end generated code: output=67f82f90838e166a input=e08ed48926a5b760]*/
{
    return pack_arguments_newref(3, a, b, args);
}


/*[clinic input]
posonly_poskw_varpos

    a: object
    /
    b: object
    *args: tuple

[clinic start generated code]*/

static PyObject *
posonly_poskw_varpos_impl(PyObject *module, PyObject *a, PyObject *b,
                          PyObject *args)
/*[clinic end generated code: output=bffdb7649941c939 input=e5a2c4cab6745ca1]*/
{
    return pack_arguments_newref(3, a, b, args);
}


/*[clinic input]
poskw_varpos

    a: object
    *args: tuple

[clinic start generated code]*/

static PyObject *
poskw_varpos_impl(PyObject *module, PyObject *a, PyObject *args)
/*[clinic end generated code: output=2413ddfb5ef22794 input=e78114436a07aefe]*/
{
    return pack_arguments_newref(2, a, args);
}


/*[clinic input]
poskw_varpos_kwonly_opt

    a: object
    *args: tuple
    b: bool = False

[clinic start generated code]*/

static PyObject *
poskw_varpos_kwonly_opt_impl(PyObject *module, PyObject *a, PyObject *args,
                             int b)
/*[clinic end generated code: output=f36d35ba6133463b input=ebecfb189f4a3380]*/
{
    PyObject *obj_b = b ? Py_True : Py_False;
    return pack_arguments_newref(3, a, args, obj_b);
}


/*[clinic input]
poskw_varpos_kwonly_opt2

    a: object
    *args: tuple
    b: object = False
    c: object = False

[clinic start generated code]*/

static PyObject *
poskw_varpos_kwonly_opt2_impl(PyObject *module, PyObject *a, PyObject *args,
                              PyObject *b, PyObject *c)
/*[clinic end generated code: output=846cef62c6c40463 input=1aff29829431f711]*/
{
    return pack_arguments_newref(4, a, args, b, c);
}


/*[clinic input]
varpos_kwonly_opt

    *args: tuple
    b: object = False

[clinic start generated code]*/

static PyObject *
varpos_kwonly_opt_impl(PyObject *module, PyObject *args, PyObject *b)
/*[clinic end generated code: output=3b7bf98b091f5731 input=1bec50dc49fca2eb]*/
{
    return pack_arguments_newref(2, args, b);
}


/*[clinic input]
varpos_kwonly_req_opt

    *args: tuple
    a: object
    b: object = False
    c: object = False

[clinic start generated code]*/

static PyObject *
varpos_kwonly_req_opt_impl(PyObject *module, PyObject *args, PyObject *a,
                           PyObject *b, PyObject *c)
/*[clinic end generated code: output=165274e1fd037ae9 input=355271f6119bb6c8]*/
{
    return pack_arguments_newref(4, args, a, b, c);
}


/*[clinic input]
varpos_array

    *args: array

[clinic start generated code]*/

static PyObject *
varpos_array_impl(PyObject *module, PyObject * const *args,
                  Py_ssize_t args_length)
/*[clinic end generated code: output=a25f42f39c9b13ad input=97b8bdcf87e019c7]*/
{
    return _PyTuple_FromArray(args, args_length);
}


/*[clinic input]
posonly_varpos_array

    a: object
    b: object
    /
    *args: array

[clinic start generated code]*/

static PyObject *
posonly_varpos_array_impl(PyObject *module, PyObject *a, PyObject *b,
                          PyObject * const *args, Py_ssize_t args_length)
/*[clinic end generated code: output=267032f41bd039cc input=86ee3064b7853e86]*/
{
    return pack_arguments_2pos_varpos(a, b, args, args_length);
}


/*[clinic input]
posonly_req_opt_varpos_array

    a: object
    b: object = False
    /
    *args: array

[clinic start generated code]*/

static PyObject *
posonly_req_opt_varpos_array_impl(PyObject *module, PyObject *a, PyObject *b,
                                  PyObject * const *args,
                                  Py_ssize_t args_length)
/*[clinic end generated code: output=f2f93c77ead93699 input=b01d7728164fd93e]*/
{
    return pack_arguments_2pos_varpos(a, b, args, args_length);
}


/*[clinic input]
posonly_poskw_varpos_array

    a: object
    /
    b: object
    *args: array

[clinic start generated code]*/

static PyObject *
posonly_poskw_varpos_array_impl(PyObject *module, PyObject *a, PyObject *b,
                                PyObject * const *args,
                                Py_ssize_t args_length)
/*[clinic end generated code: output=155811a8b2d65a12 input=5fb08cdc6afb9d7c]*/
{
    return pack_arguments_2pos_varpos(a, b, args, args_length);
}



/*[clinic input]
gh_32092_oob

    pos1: object
    pos2: object
    *varargs: tuple
    kw1: object = None
    kw2: object = None

Proof-of-concept of GH-32092 OOB bug.

[clinic start generated code]*/

static PyObject *
gh_32092_oob_impl(PyObject *module, PyObject *pos1, PyObject *pos2,
                  PyObject *varargs, PyObject *kw1, PyObject *kw2)
/*[clinic end generated code: output=ee259c130054653f input=63aeeca881979b91]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
gh_32092_kw_pass

    pos: object
    *args: tuple
    kw: object = None

Proof-of-concept of GH-32092 keyword args passing bug.

[clinic start generated code]*/

static PyObject *
gh_32092_kw_pass_impl(PyObject *module, PyObject *pos, PyObject *args,
                      PyObject *kw)
/*[clinic end generated code: output=4a2bbe4f7c8604e9 input=258987971f3ee97a]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
gh_99233_refcount

    *args: tuple

Proof-of-concept of GH-99233 refcount error bug.

[clinic start generated code]*/

static PyObject *
gh_99233_refcount_impl(PyObject *module, PyObject *args)
/*[clinic end generated code: output=585855abfbca9a7f input=f5204359fd852613]*/
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
null_or_tuple_for_varargs

    name: object
    *constraints: tuple
    covariant: bool = False

See https://github.com/python/cpython/issues/110864
[clinic start generated code]*/

static PyObject *
null_or_tuple_for_varargs_impl(PyObject *module, PyObject *name,
                               PyObject *constraints, int covariant)
/*[clinic end generated code: output=a785b35421358983 input=4df930e019f32878]*/
{
    assert(name != NULL);
    assert(constraints != NULL);
    PyObject *c = covariant ? Py_True : Py_False;
    return pack_arguments_newref(3, name, constraints, c);
}

/*[clinic input]
_testclinic.clone_f1 as clone_f1
   path: str
[clinic start generated code]*/

static PyObject *
clone_f1_impl(PyObject *module, const char *path)
/*[clinic end generated code: output=8c30b5620ba86715 input=9c614b7f025ebf70]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
_testclinic.clone_f2 as clone_f2 = _testclinic.clone_f1
[clinic start generated code]*/

static PyObject *
clone_f2_impl(PyObject *module, const char *path)
/*[clinic end generated code: output=6aa1c39bec3f5d9b input=1aaaf47d6ed2324a]*/
{
    Py_RETURN_NONE;
}


/*[python input]
class custom_t_converter(CConverter):
    type = 'custom_t'
    converter = 'custom_converter'

    def pre_render(self):
        self.c_default = f'''{{
            .name = "{self.function.name}",
        }}'''

[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=b2fb801e99a06bf6]*/


/*[clinic input]
_testclinic.clone_with_conv_f1 as clone_with_conv_f1
    path: custom_t = None
[clinic start generated code]*/

static PyObject *
clone_with_conv_f1_impl(PyObject *module, custom_t path)
/*[clinic end generated code: output=f7e030ffd5439cb0 input=bc77bc80dec3f46d]*/
{
    return PyUnicode_FromString(path.name);
}


/*[clinic input]
_testclinic.clone_with_conv_f2 as clone_with_conv_f2 = _testclinic.clone_with_conv_f1
[clinic start generated code]*/

static PyObject *
clone_with_conv_f2_impl(PyObject *module, custom_t path)
/*[clinic end generated code: output=9d7fdd6a75eecee4 input=cff459a205fa83bb]*/
{
    return PyUnicode_FromString(path.name);
}


/*[clinic input]
class _testclinic.TestClass "PyObject *" "&PyBaseObject_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=c991635bb3c91f1a]*/

/*[clinic input]
_testclinic.TestClass.get_defining_class
    cls: defining_class
[clinic start generated code]*/

static PyObject *
_testclinic_TestClass_get_defining_class_impl(PyObject *self,
                                              PyTypeObject *cls)
/*[clinic end generated code: output=94f9b0b5f7add930 input=537c59417471dee3]*/
{
    return Py_NewRef(cls);
}

/*[clinic input]
_testclinic.TestClass.get_defining_class_arg
    cls: defining_class
    arg: object
[clinic start generated code]*/

static PyObject *
_testclinic_TestClass_get_defining_class_arg_impl(PyObject *self,
                                                  PyTypeObject *cls,
                                                  PyObject *arg)
/*[clinic end generated code: output=fe7e49d96cbb7718 input=d1b83d3b853af6d9]*/
{
    return PyTuple_Pack(2, cls, arg);
}

/*[clinic input]
_testclinic.TestClass.defclass_varpos
    cls: defining_class
    *args: tuple
[clinic start generated code]*/

static PyObject *
_testclinic_TestClass_defclass_varpos_impl(PyObject *self, PyTypeObject *cls,
                                           PyObject *args)
/*[clinic end generated code: output=fad33f2d3a8d778d input=332043286e393d38]*/
{
    return PyTuple_Pack(2, cls, args);
}

/*[clinic input]
_testclinic.TestClass.defclass_posonly_varpos
    cls: defining_class
    a: object
    b: object
    /
    *args: tuple
[clinic start generated code]*/

static PyObject *
_testclinic_TestClass_defclass_posonly_varpos_impl(PyObject *self,
                                                   PyTypeObject *cls,
                                                   PyObject *a, PyObject *b,
                                                   PyObject *args)
/*[clinic end generated code: output=1740fcf48d230b07 input=191da4557203c413]*/
{
    return pack_arguments_newref(4, cls, a, b, args);
}


/*
 * # Do NOT use __new__ to generate this method. Compare:
 *
 * [1] With __new__ (METH_KEYWORDS must be added even if we don't want to)
 *
 *   varpos_no_fastcall(PyTypeObject *type, PyObject *args, PyObject *kwargs)
 *   varpos_no_fastcall_impl(PyTypeObject *type, PyObject *args)
 *   no auto-generated METHODDEF macro
 *
 * [2] Without __new__ (automatically METH_FASTCALL, not good for this test)
 *
 *   varpos_no_fastcall_impl(PyObject *type, PyObject *args)
 *   varpos_no_fastcall(PyObject *type, PyObject *const *args, Py_ssize_t nargs)
 *   flags = METH_FASTCALL|METH_CLASS
 *
 * [3] Without __new__ + "@disable fastcall" (what we want)
 *
 *   varpos_no_fastcall(PyObject *type, PyObject *args)
 *   varpos_no_fastcall_impl(PyTypeObject *type, PyObject *args)
 *   flags = METH_VARARGS|METH_CLASS
 *
 * We want to test a non-fastcall class method but without triggering an
 * undefined behaviour at runtime in cfunction_call().
 *
 * At runtime, a METH_VARARGS method called in cfunction_call() must be:
 *
 *       (PyObject *, PyObject *)             -> PyObject *
 *       (PyObject *, PyObject *, PyObject *) -> PyObject *
 *
 * depending on whether METH_KEYWORDS is present or not.
 *
 * AC determines whether a method is a __new__-like method solely bsaed
 * on the method name, and not on its usage or its c_basename, and those
 * methods must always be used with METH_VARARGS|METH_KEYWORDS|METH_CLASS.
 *
 * In particular, using [1] forces us to add METH_KEYWORDS even though
 * the test shouldn't be expecting keyword arguments. Using [2] is also
 * not possible since we want to test non-fastcalls. This is the reason
 * why we need to be able to disable the METH_FASTCALL flag.
 */

/*[clinic input]
@disable fastcall
@classmethod
_testclinic.TestClass.varpos_no_fastcall

    *args: tuple

[clinic start generated code]*/

static PyObject *
_testclinic_TestClass_varpos_no_fastcall_impl(PyTypeObject *type,
                                              PyObject *args)
/*[clinic end generated code: output=edfacec733aeb9c5 input=3f298d143aa98048]*/
{
    return Py_NewRef(args);
}


/*[clinic input]
@disable fastcall
@classmethod
_testclinic.TestClass.posonly_varpos_no_fastcall

    a: object
    b: object
    /
    *args: tuple

[clinic start generated code]*/

static PyObject *
_testclinic_TestClass_posonly_varpos_no_fastcall_impl(PyTypeObject *type,
                                                      PyObject *a,
                                                      PyObject *b,
                                                      PyObject *args)
/*[clinic end generated code: output=2c5184aebe020085 input=3621dd172c5193d8]*/
{
    return pack_arguments_newref(3, a, b, args);
}


/*[clinic input]
@disable fastcall
@classmethod
_testclinic.TestClass.posonly_req_opt_varpos_no_fastcall

    a: object
    b: object = False
    /
    *args: tuple

[clinic start generated code]*/

static PyObject *
_testclinic_TestClass_posonly_req_opt_varpos_no_fastcall_impl(PyTypeObject *type,
                                                              PyObject *a,
                                                              PyObject *b,
                                                              PyObject *args)
/*[clinic end generated code: output=08e533d59bceadf6 input=922fa7851b32e2dd]*/
{
    return pack_arguments_newref(3, a, b, args);
}


/*[clinic input]
@disable fastcall
@classmethod
_testclinic.TestClass.posonly_poskw_varpos_no_fastcall

    a: object
    /
    b: object
    *args: tuple

[clinic start generated code]*/

static PyObject *
_testclinic_TestClass_posonly_poskw_varpos_no_fastcall_impl(PyTypeObject *type,
                                                            PyObject *a,
                                                            PyObject *b,
                                                            PyObject *args)
/*[clinic end generated code: output=8ecfda20850e689f input=60443fe0bb8fe3e0]*/
{
    return pack_arguments_newref(3, a, b, args);
}


/*[clinic input]
@disable fastcall
@classmethod
_testclinic.TestClass.varpos_array_no_fastcall

    *args: array

[clinic start generated code]*/

static PyObject *
_testclinic_TestClass_varpos_array_no_fastcall_impl(PyTypeObject *type,
                                                    PyObject * const *args,
                                                    Py_ssize_t args_length)
/*[clinic end generated code: output=27c9da663e942617 input=9ba5ae1f1eb58777]*/
{
    return _PyTuple_FromArray(args, args_length);
}


/*[clinic input]
@disable fastcall
@classmethod
_testclinic.TestClass.posonly_varpos_array_no_fastcall

    a: object
    b: object
    /
    *args: array

[clinic start generated code]*/

static PyObject *
_testclinic_TestClass_posonly_varpos_array_no_fastcall_impl(PyTypeObject *type,
                                                            PyObject *a,
                                                            PyObject *b,
                                                            PyObject * const *args,
                                                            Py_ssize_t args_length)
/*[clinic end generated code: output=71e676f1870b5a7e input=18eadf4c6eaab613]*/
{
    return pack_arguments_2pos_varpos(a, b, args, args_length);
}


/*[clinic input]
@disable fastcall
@classmethod
_testclinic.TestClass.posonly_req_opt_varpos_array_no_fastcall

    a: object
    b: object = False
    /
    *args: array

[clinic start generated code]*/

static PyObject *
_testclinic_TestClass_posonly_req_opt_varpos_array_no_fastcall_impl(PyTypeObject *type,
                                                                    PyObject *a,
                                                                    PyObject *b,
                                                                    PyObject * const *args,
                                                                    Py_ssize_t args_length)
/*[clinic end generated code: output=abb395cae91d48ac input=5bf791fdad70b480]*/
{
    return pack_arguments_2pos_varpos(a, b, args, args_length);
}


/*[clinic input]
@disable fastcall
@classmethod
_testclinic.TestClass.posonly_poskw_varpos_array_no_fastcall

    a: object
    /
    b: object
    *args: array

[clinic start generated code]*/

static PyObject *
_testclinic_TestClass_posonly_poskw_varpos_array_no_fastcall_impl(PyTypeObject *type,
                                                                  PyObject *a,
                                                                  PyObject *b,
                                                                  PyObject * const *args,
                                                                  Py_ssize_t args_length)
/*[clinic end generated code: output=aaddd9530048b229 input=9ed3842f4d472d45]*/
{
    return pack_arguments_2pos_varpos(a, b, args, args_length);
}

static struct PyMethodDef test_class_methods[] = {
    _TESTCLINIC_TESTCLASS_GET_DEFINING_CLASS_METHODDEF
    _TESTCLINIC_TESTCLASS_GET_DEFINING_CLASS_ARG_METHODDEF
    _TESTCLINIC_TESTCLASS_DEFCLASS_VARPOS_METHODDEF
    _TESTCLINIC_TESTCLASS_DEFCLASS_POSONLY_VARPOS_METHODDEF

    _TESTCLINIC_TESTCLASS_VARPOS_NO_FASTCALL_METHODDEF
    _TESTCLINIC_TESTCLASS_POSONLY_VARPOS_NO_FASTCALL_METHODDEF
    _TESTCLINIC_TESTCLASS_POSONLY_REQ_OPT_VARPOS_NO_FASTCALL_METHODDEF
    _TESTCLINIC_TESTCLASS_POSONLY_POSKW_VARPOS_NO_FASTCALL_METHODDEF

    _TESTCLINIC_TESTCLASS_VARPOS_ARRAY_NO_FASTCALL_METHODDEF
    _TESTCLINIC_TESTCLASS_POSONLY_VARPOS_ARRAY_NO_FASTCALL_METHODDEF
    _TESTCLINIC_TESTCLASS_POSONLY_REQ_OPT_VARPOS_ARRAY_NO_FASTCALL_METHODDEF
    _TESTCLINIC_TESTCLASS_POSONLY_POSKW_VARPOS_ARRAY_NO_FASTCALL_METHODDEF

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


/*[clinic input]
output push
destination deprstar new file '{dirname}/clinic/_testclinic_depr.c.h'
output everything deprstar
#output methoddef_ifndef buffer 1
output docstring_prototype suppress
output parser_prototype suppress
output impl_definition block
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=32116eac48a42d34]*/


// Mock Python version 3.8
#define _SAVED_PY_VERSION PY_VERSION_HEX
#undef PY_VERSION_HEX
#define PY_VERSION_HEX 0x03080000


#include "clinic/_testclinic_depr.c.h"


/*[clinic input]
class _testclinic.DeprStarNew "PyObject *" "PyObject"
@classmethod
_testclinic.DeprStarNew.__new__ as depr_star_new
    * [from 3.14]
    a: object = None
The deprecation message should use the class name instead of __new__.
[clinic start generated code]*/

static PyObject *
depr_star_new_impl(PyTypeObject *type, PyObject *a)
/*[clinic end generated code: output=bdbb36244f90cf46 input=fdd640db964b4dc1]*/
{
    return type->tp_alloc(type, 0);
}

/*[clinic input]
_testclinic.DeprStarNew.cloned as depr_star_new_clone = _testclinic.DeprStarNew.__new__
[clinic start generated code]*/

static PyObject *
depr_star_new_clone_impl(PyObject *type, PyObject *a)
/*[clinic end generated code: output=3b17bf885fa736bc input=ea659285d5dbec6c]*/
{
    Py_RETURN_NONE;
}

static struct PyMethodDef depr_star_new_methods[] = {
    DEPR_STAR_NEW_CLONE_METHODDEF
    {NULL, NULL}
};

static PyTypeObject DeprStarNew = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testclinic.DeprStarNew",
    .tp_basicsize = sizeof(PyObject),
    .tp_new = depr_star_new,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = depr_star_new_methods,
};


/*[clinic input]
class _testclinic.DeprStarInit "PyObject *" "PyObject"
_testclinic.DeprStarInit.__init__ as depr_star_init
    * [from 3.14]
    a: object = None
The deprecation message should use the class name instead of __init__.
[clinic start generated code]*/

static int
depr_star_init_impl(PyObject *self, PyObject *a)
/*[clinic end generated code: output=8d27b43c286d3ecc input=5575b77229d5e2be]*/
{
    return 0;
}

/*[clinic input]
_testclinic.DeprStarInit.cloned as depr_star_init_clone = _testclinic.DeprStarInit.__init__
[clinic start generated code]*/

static PyObject *
depr_star_init_clone_impl(PyObject *self, PyObject *a)
/*[clinic end generated code: output=ddfe8a1b5531e7cc input=561e103fe7f8e94f]*/
{
    Py_RETURN_NONE;
}

static struct PyMethodDef depr_star_init_methods[] = {
    DEPR_STAR_INIT_CLONE_METHODDEF
    {NULL, NULL}
};

static PyTypeObject DeprStarInit = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testclinic.DeprStarInit",
    .tp_basicsize = sizeof(PyObject),
    .tp_new = PyType_GenericNew,
    .tp_init = depr_star_init,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = depr_star_init_methods,
};


/*[clinic input]
class _testclinic.DeprStarInitNoInline "PyObject *" "PyObject"
_testclinic.DeprStarInitNoInline.__init__ as depr_star_init_noinline
    a: object
    * [from 3.14]
    b: object
    c: object = None
    *
    # Force to use _PyArg_ParseTupleAndKeywordsFast.
    d: str(accept={str, robuffer}, zeroes=True) = ''
[clinic start generated code]*/

static int
depr_star_init_noinline_impl(PyObject *self, PyObject *a, PyObject *b,
                             PyObject *c, const char *d, Py_ssize_t d_length)
/*[clinic end generated code: output=9b31fc167f1bf9f7 input=5a887543122bca48]*/
{
    return 0;
}

static PyTypeObject DeprStarInitNoInline = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testclinic.DeprStarInitNoInline",
    .tp_basicsize = sizeof(PyObject),
    .tp_new = PyType_GenericNew,
    .tp_init = depr_star_init_noinline,
    .tp_flags = Py_TPFLAGS_DEFAULT,
};


/*[clinic input]
class _testclinic.DeprKwdNew "PyObject *" "PyObject"
@classmethod
_testclinic.DeprKwdNew.__new__ as depr_kwd_new
    a: object = None
    / [from 3.14]
The deprecation message should use the class name instead of __new__.
[clinic start generated code]*/

static PyObject *
depr_kwd_new_impl(PyTypeObject *type, PyObject *a)
/*[clinic end generated code: output=618d07afc5616149 input=6c7d13c471013c10]*/
{
    return type->tp_alloc(type, 0);
}

static PyTypeObject DeprKwdNew = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testclinic.DeprKwdNew",
    .tp_basicsize = sizeof(PyObject),
    .tp_new = depr_kwd_new,
    .tp_flags = Py_TPFLAGS_DEFAULT,
};


/*[clinic input]
class _testclinic.DeprKwdInit "PyObject *" "PyObject"
_testclinic.DeprKwdInit.__init__ as depr_kwd_init
    a: object = None
    / [from 3.14]
The deprecation message should use the class name instead of __init__.
[clinic start generated code]*/

static int
depr_kwd_init_impl(PyObject *self, PyObject *a)
/*[clinic end generated code: output=6e02eb724a85d840 input=b9bf3c20f012d539]*/
{
    return 0;
}

static PyTypeObject DeprKwdInit = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testclinic.DeprKwdInit",
    .tp_basicsize = sizeof(PyObject),
    .tp_new = PyType_GenericNew,
    .tp_init = depr_kwd_init,
    .tp_flags = Py_TPFLAGS_DEFAULT,
};


/*[clinic input]
class _testclinic.DeprKwdInitNoInline "PyObject *" "PyObject"
_testclinic.DeprKwdInitNoInline.__init__ as depr_kwd_init_noinline
    a: object
    /
    b: object
    c: object = None
    / [from 3.14]
    # Force to use _PyArg_ParseTupleAndKeywordsFast.
    d: str(accept={str, robuffer}, zeroes=True) = ''
[clinic start generated code]*/

static int
depr_kwd_init_noinline_impl(PyObject *self, PyObject *a, PyObject *b,
                            PyObject *c, const char *d, Py_ssize_t d_length)
/*[clinic end generated code: output=27759d70ddd25873 input=c19d982c8c70a930]*/
{
    return 0;
}

static PyTypeObject DeprKwdInitNoInline = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testclinic.DeprKwdInitNoInline",
    .tp_basicsize = sizeof(PyObject),
    .tp_new = PyType_GenericNew,
    .tp_init = depr_kwd_init_noinline,
    .tp_flags = Py_TPFLAGS_DEFAULT,
};


/*[clinic input]
depr_star_pos0_len1
    * [from 3.14]
    a: object
[clinic start generated code]*/

static PyObject *
depr_star_pos0_len1_impl(PyObject *module, PyObject *a)
/*[clinic end generated code: output=e1c6c2b423129499 input=089b9aee25381b69]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
depr_star_pos0_len2
    * [from 3.14]
    a: object
    b: object
[clinic start generated code]*/

static PyObject *
depr_star_pos0_len2_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=96df9be39859c7e4 input=65c83a32e01495c6]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
depr_star_pos0_len3_with_kwd
    * [from 3.14]
    a: object
    b: object
    c: object
    *
    d: object
[clinic start generated code]*/

static PyObject *
depr_star_pos0_len3_with_kwd_impl(PyObject *module, PyObject *a, PyObject *b,
                                  PyObject *c, PyObject *d)
/*[clinic end generated code: output=7f2531eda837052f input=b33f620f57d9270f]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
depr_star_pos1_len1_opt
    a: object
    * [from 3.14]
    b: object = None
[clinic start generated code]*/

static PyObject *
depr_star_pos1_len1_opt_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=b5b4e326ee3b216f input=4a4b8ff72eae9ff7]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
depr_star_pos1_len1
    a: object
    * [from 3.14]
    b: object
[clinic start generated code]*/

static PyObject *
depr_star_pos1_len1_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=eab92e37d5b0a480 input=1e7787a9fe5f62a0]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
depr_star_pos1_len2_with_kwd
    a: object
    * [from 3.14]
    b: object
    c: object
    *
    d: object
[clinic start generated code]*/

static PyObject *
depr_star_pos1_len2_with_kwd_impl(PyObject *module, PyObject *a, PyObject *b,
                                  PyObject *c, PyObject *d)
/*[clinic end generated code: output=3bccab672b7cfbb8 input=6bc7bd742fa8be15]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
depr_star_pos2_len1
    a: object
    b: object
    * [from 3.14]
    c: object
[clinic start generated code]*/

static PyObject *
depr_star_pos2_len1_impl(PyObject *module, PyObject *a, PyObject *b,
                         PyObject *c)
/*[clinic end generated code: output=20f5b230e9beeb70 input=5fc3e1790dec00d5]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
depr_star_pos2_len2
    a: object
    b: object
    * [from 3.14]
    c: object
    d: object
[clinic start generated code]*/

static PyObject *
depr_star_pos2_len2_impl(PyObject *module, PyObject *a, PyObject *b,
                         PyObject *c, PyObject *d)
/*[clinic end generated code: output=9f90ed8fbce27d7a input=9cc8003b89d38779]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
depr_star_pos2_len2_with_kwd
    a: object
    b: object
    * [from 3.14]
    c: object
    d: object
    *
    e: object
[clinic start generated code]*/

static PyObject *
depr_star_pos2_len2_with_kwd_impl(PyObject *module, PyObject *a, PyObject *b,
                                  PyObject *c, PyObject *d, PyObject *e)
/*[clinic end generated code: output=05432c4f20527215 input=831832d90534da91]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
depr_star_noinline
    a: object
    * [from 3.14]
    b: object
    c: object = None
    *
    # Force to use _PyArg_ParseStackAndKeywords.
    d: str(accept={str, robuffer}, zeroes=True) = ''
[clinic start generated code]*/

static PyObject *
depr_star_noinline_impl(PyObject *module, PyObject *a, PyObject *b,
                        PyObject *c, const char *d, Py_ssize_t d_length)
/*[clinic end generated code: output=cc27dacf5c2754af input=d36cc862a2daef98]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
depr_star_multi
    a: object
    * [from 3.16]
    b: object
    * [from 3.15]
    c: object
    d: object
    * [from 3.14]
    e: object
    f: object
    g: object
    *
    h: object
[clinic start generated code]*/

static PyObject *
depr_star_multi_impl(PyObject *module, PyObject *a, PyObject *b, PyObject *c,
                     PyObject *d, PyObject *e, PyObject *f, PyObject *g,
                     PyObject *h)
/*[clinic end generated code: output=77681653f4202068 input=3ebd05d888a957ea]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
depr_kwd_required_1
    a: object
    /
    b: object
    / [from 3.14]
[clinic start generated code]*/

static PyObject *
depr_kwd_required_1_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=1d8ab19ea78418af input=53f2c398b828462d]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
depr_kwd_required_2
    a: object
    /
    b: object
    c: object
    / [from 3.14]
[clinic start generated code]*/

static PyObject *
depr_kwd_required_2_impl(PyObject *module, PyObject *a, PyObject *b,
                         PyObject *c)
/*[clinic end generated code: output=44a89cb82509ddde input=a2b0ef37de8a01a7]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
depr_kwd_optional_1
    a: object
    /
    b: object = None
    / [from 3.14]
[clinic start generated code]*/

static PyObject *
depr_kwd_optional_1_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=a8a3d67efcc7b058 input=e416981eb78c3053]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
depr_kwd_optional_2
    a: object
    /
    b: object = None
    c: object = None
    / [from 3.14]
[clinic start generated code]*/

static PyObject *
depr_kwd_optional_2_impl(PyObject *module, PyObject *a, PyObject *b,
                         PyObject *c)
/*[clinic end generated code: output=aa2d967f26fdb9f6 input=cae3afb783bfc855]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
depr_kwd_optional_3
    a: object = None
    b: object = None
    c: object = None
    / [from 3.14]
[clinic start generated code]*/

static PyObject *
depr_kwd_optional_3_impl(PyObject *module, PyObject *a, PyObject *b,
                         PyObject *c)
/*[clinic end generated code: output=a26025bf6118fd07 input=c9183b2f9ccaf992]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
depr_kwd_required_optional
    a: object
    /
    b: object
    c: object = None
    / [from 3.14]
[clinic start generated code]*/

static PyObject *
depr_kwd_required_optional_impl(PyObject *module, PyObject *a, PyObject *b,
                                PyObject *c)
/*[clinic end generated code: output=e53a8b7a250d8ffc input=23237a046f8388f5]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
depr_kwd_noinline
    a: object
    /
    b: object
    c: object = None
    / [from 3.14]
    # Force to use _PyArg_ParseStackAndKeywords.
    d: str(accept={str, robuffer}, zeroes=True) = ''
[clinic start generated code]*/

static PyObject *
depr_kwd_noinline_impl(PyObject *module, PyObject *a, PyObject *b,
                       PyObject *c, const char *d, Py_ssize_t d_length)
/*[clinic end generated code: output=f59da8113f2bad7c input=1d6db65bebb069d7]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
depr_kwd_multi
    a: object
    /
    b: object
    / [from 3.14]
    c: object
    d: object
    / [from 3.15]
    e: object
    f: object
    g: object
    / [from 3.16]
    h: object
[clinic start generated code]*/

static PyObject *
depr_kwd_multi_impl(PyObject *module, PyObject *a, PyObject *b, PyObject *c,
                    PyObject *d, PyObject *e, PyObject *f, PyObject *g,
                    PyObject *h)
/*[clinic end generated code: output=ddfbde80fe1942e1 input=7a074e621c79efd7]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
depr_multi
    a: object
    /
    b: object
    / [from 3.14]
    c: object
    / [from 3.15]
    d: object
    * [from 3.15]
    e: object
    * [from 3.14]
    f: object
    *
    g: object
[clinic start generated code]*/

static PyObject *
depr_multi_impl(PyObject *module, PyObject *a, PyObject *b, PyObject *c,
                PyObject *d, PyObject *e, PyObject *f, PyObject *g)
/*[clinic end generated code: output=f81c92852ca2d4ee input=5b847c5e44bedd02]*/
{
    Py_RETURN_NONE;
}


// Reset PY_VERSION_HEX
#undef PY_VERSION_HEX
#define PY_VERSION_HEX _SAVED_PY_VERSION
#undef _SAVED_PY_VERSION


/*[clinic input]
output pop
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=e7c7c42daced52b0]*/

static PyMethodDef tester_methods[] = {
    TEST_EMPTY_FUNCTION_METHODDEF
    OBJECTS_CONVERTER_METHODDEF
    BYTES_OBJECT_CONVERTER_METHODDEF
    BYTE_ARRAY_OBJECT_CONVERTER_METHODDEF
    UNICODE_CONVERTER_METHODDEF
    BOOL_CONVERTER_METHODDEF
    BOOL_CONVERTER_C_DEFAULT_METHODDEF
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

    VARPOS_METHODDEF
    POSONLY_VARPOS_METHODDEF
    POSONLY_REQ_OPT_VARPOS_METHODDEF
    POSONLY_POSKW_VARPOS_METHODDEF
    POSKW_VARPOS_METHODDEF
    POSKW_VARPOS_KWONLY_OPT_METHODDEF
    POSKW_VARPOS_KWONLY_OPT2_METHODDEF
    VARPOS_KWONLY_OPT_METHODDEF
    VARPOS_KWONLY_REQ_OPT_METHODDEF

    VARPOS_ARRAY_METHODDEF
    POSONLY_VARPOS_ARRAY_METHODDEF
    POSONLY_REQ_OPT_VARPOS_ARRAY_METHODDEF
    POSONLY_POSKW_VARPOS_ARRAY_METHODDEF

    GH_32092_OOB_METHODDEF
    GH_32092_KW_PASS_METHODDEF
    GH_99233_REFCOUNT_METHODDEF
    GH_99240_DOUBLE_FREE_METHODDEF
    NULL_OR_TUPLE_FOR_VARARGS_METHODDEF

    CLONE_F1_METHODDEF
    CLONE_F2_METHODDEF
    CLONE_WITH_CONV_F1_METHODDEF
    CLONE_WITH_CONV_F2_METHODDEF

    DEPR_STAR_POS0_LEN1_METHODDEF
    DEPR_STAR_POS0_LEN2_METHODDEF
    DEPR_STAR_POS0_LEN3_WITH_KWD_METHODDEF
    DEPR_STAR_POS1_LEN1_OPT_METHODDEF
    DEPR_STAR_POS1_LEN1_METHODDEF
    DEPR_STAR_POS1_LEN2_WITH_KWD_METHODDEF
    DEPR_STAR_POS2_LEN1_METHODDEF
    DEPR_STAR_POS2_LEN2_METHODDEF
    DEPR_STAR_POS2_LEN2_WITH_KWD_METHODDEF
    DEPR_STAR_NOINLINE_METHODDEF
    DEPR_STAR_MULTI_METHODDEF
    DEPR_KWD_REQUIRED_1_METHODDEF
    DEPR_KWD_REQUIRED_2_METHODDEF
    DEPR_KWD_OPTIONAL_1_METHODDEF
    DEPR_KWD_OPTIONAL_2_METHODDEF
    DEPR_KWD_OPTIONAL_3_METHODDEF
    DEPR_KWD_REQUIRED_OPTIONAL_METHODDEF
    DEPR_KWD_NOINLINE_METHODDEF
    DEPR_KWD_MULTI_METHODDEF
    DEPR_MULTI_METHODDEF
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
#ifdef Py_GIL_DISABLED
    PyUnstable_Module_SetGIL(m, Py_MOD_GIL_NOT_USED);
#endif
    if (PyModule_AddType(m, &TestClass) < 0) {
        goto error;
    }
    if (PyModule_AddType(m, &DeprStarNew) < 0) {
        goto error;
    }
    if (PyModule_AddType(m, &DeprStarInit) < 0) {
        goto error;
    }
    if (PyModule_AddType(m, &DeprStarInitNoInline) < 0) {
        goto error;
    }
    if (PyModule_AddType(m, &DeprKwdNew) < 0) {
        goto error;
    }
    if (PyModule_AddType(m, &DeprKwdInit) < 0) {
        goto error;
    }
    if (PyModule_AddType(m, &DeprKwdInitNoInline) < 0) {
        goto error;
    }
    return m;

error:
    Py_DECREF(m);
    return NULL;
}

#undef RETURN_PACKED_ARGS
