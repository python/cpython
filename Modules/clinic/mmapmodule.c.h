/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(mmap_mmap_close__doc__,
"close($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_CLOSE_METHODDEF    \
    {"close", (PyCFunction)mmap_mmap_close, METH_NOARGS, mmap_mmap_close__doc__},

static PyObject *
mmap_mmap_close_impl(mmap_object *self);

static PyObject *
mmap_mmap_close(mmap_object *self, PyObject *Py_UNUSED(ignored))
{
    return mmap_mmap_close_impl(self);
}

PyDoc_STRVAR(mmap_mmap_read_byte__doc__,
"read_byte($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_READ_BYTE_METHODDEF    \
    {"read_byte", (PyCFunction)mmap_mmap_read_byte, METH_NOARGS, mmap_mmap_read_byte__doc__},

static PyObject *
mmap_mmap_read_byte_impl(mmap_object *self);

static PyObject *
mmap_mmap_read_byte(mmap_object *self, PyObject *Py_UNUSED(ignored))
{
    return mmap_mmap_read_byte_impl(self);
}

PyDoc_STRVAR(mmap_mmap_readline__doc__,
"readline($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_READLINE_METHODDEF    \
    {"readline", (PyCFunction)mmap_mmap_readline, METH_NOARGS, mmap_mmap_readline__doc__},

static PyObject *
mmap_mmap_readline_impl(mmap_object *self);

static PyObject *
mmap_mmap_readline(mmap_object *self, PyObject *Py_UNUSED(ignored))
{
    return mmap_mmap_readline_impl(self);
}

PyDoc_STRVAR(mmap_mmap_read__doc__,
"read($self, num_bytes=sys.maxsize, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_READ_METHODDEF    \
    {"read", (PyCFunction)(void(*)(void))mmap_mmap_read, METH_FASTCALL, mmap_mmap_read__doc__},

static PyObject *
mmap_mmap_read_impl(mmap_object *self, Py_ssize_t num_bytes);

static PyObject *
mmap_mmap_read(mmap_object *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t num_bytes = PY_SSIZE_T_MAX;

    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!_Py_convert_optional_to_ssize_t(args[0], &num_bytes)) {
        goto exit;
    }
skip_optional:
    return_value = mmap_mmap_read_impl(self, num_bytes);

exit:
    return return_value;
}

PyDoc_STRVAR(mmap_mmap_find__doc__,
"find($self, view, start=-1, end=-1, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_FIND_METHODDEF    \
    {"find", (PyCFunction)(void(*)(void))mmap_mmap_find, METH_FASTCALL, mmap_mmap_find__doc__},

static PyObject *
mmap_mmap_find_impl(mmap_object *self, Py_buffer *view, Py_ssize_t start,
                    Py_ssize_t end);

static PyObject *
mmap_mmap_find(mmap_object *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer view = {NULL, NULL};
    Py_ssize_t start = self->pos;
    Py_ssize_t end = self->size;

    if (!_PyArg_CheckPositional("find", nargs, 1, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &view, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&view, 'C')) {
        _PyArg_BadArgument("find", 1, "contiguous buffer", args[0]);
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (PyFloat_Check(args[1])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        start = ival;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (PyFloat_Check(args[2])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[2]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        end = ival;
    }
skip_optional:
    return_value = mmap_mmap_find_impl(self, &view, start, end);

exit:
    /* Cleanup for view */
    if (view.obj) {
       PyBuffer_Release(&view);
    }

    return return_value;
}

PyDoc_STRVAR(mmap_mmap_rfind__doc__,
"rfind($self, view, start=-1, end=-1, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_RFIND_METHODDEF    \
    {"rfind", (PyCFunction)(void(*)(void))mmap_mmap_rfind, METH_FASTCALL, mmap_mmap_rfind__doc__},

static PyObject *
mmap_mmap_rfind_impl(mmap_object *self, Py_buffer *view, Py_ssize_t start,
                     Py_ssize_t end);

static PyObject *
mmap_mmap_rfind(mmap_object *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer view = {NULL, NULL};
    Py_ssize_t start = self->pos;
    Py_ssize_t end = self->size;

    if (!_PyArg_CheckPositional("rfind", nargs, 1, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &view, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&view, 'C')) {
        _PyArg_BadArgument("rfind", 1, "contiguous buffer", args[0]);
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (PyFloat_Check(args[1])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        start = ival;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (PyFloat_Check(args[2])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[2]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        end = ival;
    }
skip_optional:
    return_value = mmap_mmap_rfind_impl(self, &view, start, end);

exit:
    /* Cleanup for view */
    if (view.obj) {
       PyBuffer_Release(&view);
    }

    return return_value;
}

PyDoc_STRVAR(mmap_mmap_write__doc__,
"write($self, data, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_WRITE_METHODDEF    \
    {"write", (PyCFunction)mmap_mmap_write, METH_O, mmap_mmap_write__doc__},

static PyObject *
mmap_mmap_write_impl(mmap_object *self, Py_buffer *data);

static PyObject *
mmap_mmap_write(mmap_object *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("write", 0, "contiguous buffer", arg);
        goto exit;
    }
    return_value = mmap_mmap_write_impl(self, &data);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(mmap_mmap_write_byte__doc__,
"write_byte($self, value, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_WRITE_BYTE_METHODDEF    \
    {"write_byte", (PyCFunction)mmap_mmap_write_byte, METH_O, mmap_mmap_write_byte__doc__},

static PyObject *
mmap_mmap_write_byte_impl(mmap_object *self, unsigned char value);

static PyObject *
mmap_mmap_write_byte(mmap_object *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    unsigned char value;

    if (PyFloat_Check(arg)) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        long ival = PyLong_AsLong(arg);
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        else if (ival < 0) {
            PyErr_SetString(PyExc_OverflowError,
                            "unsigned byte integer is less than minimum");
            goto exit;
        }
        else if (ival > UCHAR_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "unsigned byte integer is greater than maximum");
            goto exit;
        }
        else {
            value = (unsigned char) ival;
        }
    }
    return_value = mmap_mmap_write_byte_impl(self, value);

exit:
    return return_value;
}

PyDoc_STRVAR(mmap_mmap_size__doc__,
"size($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_SIZE_METHODDEF    \
    {"size", (PyCFunction)mmap_mmap_size, METH_NOARGS, mmap_mmap_size__doc__},

static PyObject *
mmap_mmap_size_impl(mmap_object *self);

static PyObject *
mmap_mmap_size(mmap_object *self, PyObject *Py_UNUSED(ignored))
{
    return mmap_mmap_size_impl(self);
}

PyDoc_STRVAR(mmap_mmap_resize__doc__,
"resize($self, new_size, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_RESIZE_METHODDEF    \
    {"resize", (PyCFunction)mmap_mmap_resize, METH_O, mmap_mmap_resize__doc__},

static PyObject *
mmap_mmap_resize_impl(mmap_object *self, Py_ssize_t new_size);

static PyObject *
mmap_mmap_resize(mmap_object *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_ssize_t new_size;

    if (PyFloat_Check(arg)) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(arg);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        new_size = ival;
    }
    return_value = mmap_mmap_resize_impl(self, new_size);

exit:
    return return_value;
}

PyDoc_STRVAR(mmap_mmap_tell__doc__,
"tell($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_TELL_METHODDEF    \
    {"tell", (PyCFunction)mmap_mmap_tell, METH_NOARGS, mmap_mmap_tell__doc__},

static PyObject *
mmap_mmap_tell_impl(mmap_object *self);

static PyObject *
mmap_mmap_tell(mmap_object *self, PyObject *Py_UNUSED(ignored))
{
    return mmap_mmap_tell_impl(self);
}

PyDoc_STRVAR(mmap_mmap_flush__doc__,
"flush($self, offset=0, size=-1, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_FLUSH_METHODDEF    \
    {"flush", (PyCFunction)(void(*)(void))mmap_mmap_flush, METH_FASTCALL, mmap_mmap_flush__doc__},

static PyObject *
mmap_mmap_flush_impl(mmap_object *self, Py_ssize_t offset, Py_ssize_t size);

static PyObject *
mmap_mmap_flush(mmap_object *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t offset = 0;
    Py_ssize_t size = self->size;

    if (!_PyArg_CheckPositional("flush", nargs, 0, 2)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (PyFloat_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        offset = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (PyFloat_Check(args[1])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        size = ival;
    }
skip_optional:
    return_value = mmap_mmap_flush_impl(self, offset, size);

exit:
    return return_value;
}

PyDoc_STRVAR(mmap_mmap_seek__doc__,
"seek($self, dist, how=0, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_SEEK_METHODDEF    \
    {"seek", (PyCFunction)(void(*)(void))mmap_mmap_seek, METH_FASTCALL, mmap_mmap_seek__doc__},

static PyObject *
mmap_mmap_seek_impl(mmap_object *self, Py_ssize_t dist, int how);

static PyObject *
mmap_mmap_seek(mmap_object *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t dist;
    int how = 0;

    if (!_PyArg_CheckPositional("seek", nargs, 1, 2)) {
        goto exit;
    }
    if (PyFloat_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        dist = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (PyFloat_Check(args[1])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    how = _PyLong_AsInt(args[1]);
    if (how == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = mmap_mmap_seek_impl(self, dist, how);

exit:
    return return_value;
}

PyDoc_STRVAR(mmap_mmap_move__doc__,
"move($self, dest, src, cnt, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_MOVE_METHODDEF    \
    {"move", (PyCFunction)(void(*)(void))mmap_mmap_move, METH_FASTCALL, mmap_mmap_move__doc__},

static PyObject *
mmap_mmap_move_impl(mmap_object *self, Py_ssize_t dest, Py_ssize_t src,
                    Py_ssize_t cnt);

static PyObject *
mmap_mmap_move(mmap_object *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t dest;
    Py_ssize_t src;
    Py_ssize_t cnt;

    if (!_PyArg_CheckPositional("move", nargs, 3, 3)) {
        goto exit;
    }
    if (PyFloat_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        dest = ival;
    }
    if (PyFloat_Check(args[1])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        src = ival;
    }
    if (PyFloat_Check(args[2])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[2]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        cnt = ival;
    }
    return_value = mmap_mmap_move_impl(self, dest, src, cnt);

exit:
    return return_value;
}

PyDoc_STRVAR(mmap_mmap___enter____doc__,
"__enter__($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP___ENTER___METHODDEF    \
    {"__enter__", (PyCFunction)mmap_mmap___enter__, METH_NOARGS, mmap_mmap___enter____doc__},

static PyObject *
mmap_mmap___enter___impl(mmap_object *self);

static PyObject *
mmap_mmap___enter__(mmap_object *self, PyObject *Py_UNUSED(ignored))
{
    return mmap_mmap___enter___impl(self);
}

PyDoc_STRVAR(mmap_mmap___exit____doc__,
"__exit__($self, /, exc_type=None, exc_value=None, traceback=None)\n"
"--\n"
"\n");

#define MMAP_MMAP___EXIT___METHODDEF    \
    {"__exit__", (PyCFunction)(void(*)(void))mmap_mmap___exit__, METH_FASTCALL|METH_KEYWORDS, mmap_mmap___exit____doc__},

static PyObject *
mmap_mmap___exit___impl(mmap_object *self, PyObject *exc_type,
                        PyObject *exc_value, PyObject *traceback);

static PyObject *
mmap_mmap___exit__(mmap_object *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"exc_type", "exc_value", "traceback", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "__exit__", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *exc_type = Py_None;
    PyObject *exc_value = Py_None;
    PyObject *traceback = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        exc_type = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[1]) {
        exc_value = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    traceback = args[2];
skip_optional_pos:
    return_value = mmap_mmap___exit___impl(self, exc_type, exc_value, traceback);

exit:
    return return_value;
}

#if defined(MS_WINDOWS)

PyDoc_STRVAR(mmap_mmap___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)mmap_mmap___sizeof__, METH_NOARGS, mmap_mmap___sizeof____doc__},

static PyObject *
mmap_mmap___sizeof___impl(mmap_object *self);

static PyObject *
mmap_mmap___sizeof__(mmap_object *self, PyObject *Py_UNUSED(ignored))
{
    return mmap_mmap___sizeof___impl(self);
}

#endif /* defined(MS_WINDOWS) */

#if defined(HAVE_MADVISE)

PyDoc_STRVAR(mmap_mmap_madvise__doc__,
"madvise($self, option, start=0, length=-1, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_MADVISE_METHODDEF    \
    {"madvise", (PyCFunction)(void(*)(void))mmap_mmap_madvise, METH_FASTCALL, mmap_mmap_madvise__doc__},

static PyObject *
mmap_mmap_madvise_impl(mmap_object *self, int option, Py_ssize_t start,
                       Py_ssize_t length);

static PyObject *
mmap_mmap_madvise(mmap_object *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int option;
    Py_ssize_t start = 0;
    Py_ssize_t length = self->size;

    if (!_PyArg_CheckPositional("madvise", nargs, 1, 3)) {
        goto exit;
    }
    if (PyFloat_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    option = _PyLong_AsInt(args[0]);
    if (option == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (PyFloat_Check(args[1])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        start = ival;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (PyFloat_Check(args[2])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[2]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        length = ival;
    }
skip_optional:
    return_value = mmap_mmap_madvise_impl(self, option, start, length);

exit:
    return return_value;
}

#endif /* defined(HAVE_MADVISE) */

#if defined(UNIX)

static PyObject *
mmap_mmap_impl(PyTypeObject *type, int fd, Py_ssize_t map_size, int flags,
               int prot, int access, off_t offset);

static PyObject *
mmap_mmap(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fileno", "length", "flags", "prot", "access", "offset", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "mmap", 0};
    PyObject *argsbuf[6];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 2;
    int fd;
    Py_ssize_t map_size;
    int flags = MAP_SHARED;
    int prot = PROT_WRITE | PROT_READ;
    int access = ACCESS_DEFAULT;
    off_t offset = 0;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 2, 6, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (PyFloat_Check(fastargs[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    fd = _PyLong_AsInt(fastargs[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (PyFloat_Check(fastargs[1])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(fastargs[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        map_size = ival;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[2]) {
        if (PyFloat_Check(fastargs[2])) {
            PyErr_SetString(PyExc_TypeError,
                            "integer argument expected, got float" );
            goto exit;
        }
        flags = _PyLong_AsInt(fastargs[2]);
        if (flags == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[3]) {
        if (PyFloat_Check(fastargs[3])) {
            PyErr_SetString(PyExc_TypeError,
                            "integer argument expected, got float" );
            goto exit;
        }
        prot = _PyLong_AsInt(fastargs[3]);
        if (prot == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[4]) {
        if (PyFloat_Check(fastargs[4])) {
            PyErr_SetString(PyExc_TypeError,
                            "integer argument expected, got float" );
            goto exit;
        }
        access = _PyLong_AsInt(fastargs[4]);
        if (access == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!off_t_converter(fastargs[5], &offset)) {
        goto exit;
    }
skip_optional_pos:
    return_value = mmap_mmap_impl(type, fd, map_size, flags, prot, access, offset);

exit:
    return return_value;
}

#endif /* defined(UNIX) */

#if defined(MS_WINDOWS)

static PyObject *
mmap_mmap_impl(PyTypeObject *type, int fileno, Py_ssize_t map_size,
               const char *tagname, int access, long long offset);

static PyObject *
mmap_mmap(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fileno", "length", "tagname", "access", "offset", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "mmap", 0};
    PyObject *argsbuf[5];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 2;
    int fileno;
    Py_ssize_t map_size;
    const char *tagname = NULL;
    int access = ACCESS_DEFAULT;
    long long offset = 0;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 2, 5, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (PyFloat_Check(fastargs[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    fileno = _PyLong_AsInt(fastargs[0]);
    if (fileno == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (PyFloat_Check(fastargs[1])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(fastargs[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        map_size = ival;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[2]) {
        if (fastargs[2] == Py_None) {
            tagname = NULL;
        }
        else if (PyUnicode_Check(fastargs[2])) {
            Py_ssize_t tagname_length;
            tagname = PyUnicode_AsUTF8AndSize(fastargs[2], &tagname_length);
            if (tagname == NULL) {
                goto exit;
            }
            if (strlen(tagname) != (size_t)tagname_length) {
                PyErr_SetString(PyExc_ValueError, "embedded null character");
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("mmap", 3, "str or None", fastargs[2]);
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[3]) {
        if (PyFloat_Check(fastargs[3])) {
            PyErr_SetString(PyExc_TypeError,
                            "integer argument expected, got float" );
            goto exit;
        }
        access = _PyLong_AsInt(fastargs[3]);
        if (access == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (PyFloat_Check(fastargs[4])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    offset = PyLong_AsLongLong(fastargs[4]);
    if (offset == (PY_LONG_LONG)-1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = mmap_mmap_impl(type, fileno, map_size, tagname, access, offset);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#ifndef MMAP_MMAP___SIZEOF___METHODDEF
    #define MMAP_MMAP___SIZEOF___METHODDEF
#endif /* !defined(MMAP_MMAP___SIZEOF___METHODDEF) */

#ifndef MMAP_MMAP_MADVISE_METHODDEF
    #define MMAP_MMAP_MADVISE_METHODDEF
#endif /* !defined(MMAP_MMAP_MADVISE_METHODDEF) */
/*[clinic end generated code: output=3afbcaf82e4b9a14 input=a9049054013a1b77]*/
