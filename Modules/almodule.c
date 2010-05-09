
#define OLD_INTERFACE           /* define for pre-Irix 6 interface */

#include "Python.h"
#include "stringobject.h"
#include <audio.h>
#include <stdarg.h>

#ifndef AL_NO_ELEM
#ifndef OLD_INTERFACE
#define OLD_INTERFACE
#endif /* OLD_INTERFACE */
#endif /* AL_NO_ELEM */

static PyObject *ErrorObject;

/* ----------------------------------------------------- */

/* Declarations for objects of type port */

typedef struct {
    PyObject_HEAD
    /* XXXX Add your own stuff here */
    ALport port;
} alpobject;

static PyTypeObject Alptype;



/* ---------------------------------------------------------------- */

/* Declarations for objects of type config */

typedef struct {
    PyObject_HEAD
    /* XXXX Add your own stuff here */
    ALconfig config;
} alcobject;

static PyTypeObject Alctype;


static void
ErrorHandler(long code, const char *fmt, ...)
{
    va_list args;
    char buf[128];

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
    PyErr_SetString(ErrorObject, buf);
}

#ifdef AL_NO_ELEM               /* IRIX 6 */

static PyObject *
param2python(int resource, int param, ALvalue value, ALparamInfo *pinfo)
{
    ALparamInfo info;

    if (pinfo == NULL) {
        pinfo = &info;
        if (alGetParamInfo(resource, param, &info) < 0)
            return NULL;
    }
    switch (pinfo->elementType) {
    case AL_PTR_ELEM:
        /* XXXX don't know how to handle this */
    case AL_NO_ELEM:
        Py_INCREF(Py_None);
        return Py_None;
    case AL_INT32_ELEM:
    case AL_RESOURCE_ELEM:
    case AL_ENUM_ELEM:
        return PyInt_FromLong((long) value.i);
    case AL_INT64_ELEM:
        return PyLong_FromLongLong(value.ll);
    case AL_FIXED_ELEM:
        return PyFloat_FromDouble(alFixedToDouble(value.ll));
    case AL_CHAR_ELEM:
        if (value.ptr == NULL) {
            Py_INCREF(Py_None);
            return Py_None;
        }
        return PyString_FromString((char *) value.ptr);
    default:
        PyErr_SetString(ErrorObject, "unknown element type");
        return NULL;
    }
}

static int
python2elem(PyObject *item, void *ptr, int elementType)
{
    switch (elementType) {
    case AL_INT32_ELEM:
    case AL_RESOURCE_ELEM:
    case AL_ENUM_ELEM:
        if (!PyInt_Check(item)) {
            PyErr_BadArgument();
            return -1;
        }
        *((int *) ptr) = PyInt_AsLong(item);
        break;
    case AL_INT64_ELEM:
        if (PyInt_Check(item))
            *((long long *) ptr) = PyInt_AsLong(item);
        else if (PyLong_Check(item))
            *((long long *) ptr) = PyLong_AsLongLong(item);
        else {
            PyErr_BadArgument();
            return -1;
        }
        break;
    case AL_FIXED_ELEM:
        if (PyInt_Check(item))
            *((long long *) ptr) = alDoubleToFixed((double) PyInt_AsLong(item));
        else if (PyFloat_Check(item))
            *((long long *) ptr) = alDoubleToFixed(PyFloat_AsDouble(item));
        else {
            PyErr_BadArgument();
            return -1;
        }
        break;
    default:
        PyErr_SetString(ErrorObject, "unknown element type");
        return -1;
    }
    return 0;
}

static int
python2param(int resource, ALpv *param, PyObject *value, ALparamInfo *pinfo)
{
    ALparamInfo info;
    int i, stepsize;
    PyObject *item;

    if (pinfo == NULL) {
        pinfo = &info;
        if (alGetParamInfo(resource, param->param, &info) < 0)
            return -1;
    }
    switch (pinfo->valueType) {
    case AL_STRING_VAL:
        if (pinfo->elementType != AL_CHAR_ELEM) {
            PyErr_SetString(ErrorObject, "unknown element type");
            return -1;
        }
        if (!PyString_Check(value)) {
            PyErr_BadArgument();
            return -1;
        }
        param->value.ptr = PyString_AS_STRING(value);
        param->sizeIn = PyString_GET_SIZE(value)+1; /*account for NUL*/
        break;
    case AL_SET_VAL:
    case AL_VECTOR_VAL:
        if (!PyList_Check(value) && !PyTuple_Check(value)) {
            PyErr_BadArgument();
            return -1;
        }
        switch (pinfo->elementType) {
        case AL_INT32_ELEM:
        case AL_RESOURCE_ELEM:
        case AL_ENUM_ELEM:
            param->sizeIn = PySequence_Size(value);
            param->value.ptr = PyMem_NEW(int, param->sizeIn);
            stepsize = sizeof(int);
            break;
        case AL_INT64_ELEM:
        case AL_FIXED_ELEM:
            param->sizeIn = PySequence_Size(value);
            param->value.ptr = PyMem_NEW(long long, param->sizeIn);
            stepsize = sizeof(long long);
            break;
        }
        for (i = 0; i < param->sizeIn; i++) {
            item = PySequence_GetItem(value, i);
            if (python2elem(item, (void *) ((char *) param->value.ptr + i*stepsize), pinfo->elementType) < 0) {
                PyMem_DEL(param->value.ptr);
                return -1;
            }
        }
        break;
    case AL_SCALAR_VAL:
        switch (pinfo->elementType) {
        case AL_INT32_ELEM:
        case AL_RESOURCE_ELEM:
        case AL_ENUM_ELEM:
            return python2elem(value, (void *) &param->value.i,
                               pinfo->elementType);
        case AL_INT64_ELEM:
        case AL_FIXED_ELEM:
            return python2elem(value, (void *) &param->value.ll,
                               pinfo->elementType);
        default:
            PyErr_SetString(ErrorObject, "unknown element type");
            return -1;
        }
    }
    return 0;
}

static int
python2params(int resource1, int resource2, PyObject *list, ALpv **pvsp, ALparamInfo **pinfop)
{
    PyObject *item;
    ALpv *pvs;
    ALparamInfo *pinfo;
    int npvs, i;

    npvs = PyList_Size(list);
    pvs = PyMem_NEW(ALpv, npvs);
    pinfo = PyMem_NEW(ALparamInfo, npvs);
    for (i = 0; i < npvs; i++) {
        item = PyList_GetItem(list, i);
        if (!PyArg_ParseTuple(item, "iO", &pvs[i].param, &item))
            goto error;
        if (alGetParamInfo(resource1, pvs[i].param, &pinfo[i]) < 0 &&
            alGetParamInfo(resource2, pvs[i].param, &pinfo[i]) < 0)
            goto error;
        if (python2param(resource1, &pvs[i], item, &pinfo[i]) < 0)
            goto error;
    }

    *pvsp = pvs;
    *pinfop = pinfo;
    return npvs;

  error:
    /* XXXX we should clean up everything */
    if (pvs)
        PyMem_DEL(pvs);
    if (pinfo)
        PyMem_DEL(pinfo);
    return -1;
}

/* -------------------------------------------------------- */


static PyObject *
SetConfig(alcobject *self, PyObject *args, int (*func)(ALconfig, int))
{
    int par;

    if (!PyArg_ParseTuple(args, "i:SetConfig", &par))
        return NULL;

    if ((*func)(self->config, par) == -1)
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
GetConfig(alcobject *self, PyObject *args, int (*func)(ALconfig))
{
    int par;

    if (!PyArg_ParseTuple(args, ":GetConfig"))
        return NULL;

    if ((par = (*func)(self->config)) == -1)
        return NULL;

    return PyInt_FromLong((long) par);
}

PyDoc_STRVAR(alc_SetWidth__doc__,
"alSetWidth: set the wordsize for integer audio data.");

static PyObject *
alc_SetWidth(alcobject *self, PyObject *args)
{
    return SetConfig(self, args, alSetWidth);
}


PyDoc_STRVAR(alc_GetWidth__doc__,
"alGetWidth: get the wordsize for integer audio data.");

static PyObject *
alc_GetWidth(alcobject *self, PyObject *args)
{
    return GetConfig(self, args, alGetWidth);
}


PyDoc_STRVAR(alc_SetSampFmt__doc__,
"alSetSampFmt: set the sample format setting in an audio ALconfig "
"structure.");

static PyObject *
alc_SetSampFmt(alcobject *self, PyObject *args)
{
    return SetConfig(self, args, alSetSampFmt);
}


PyDoc_STRVAR(alc_GetSampFmt__doc__,
"alGetSampFmt: get the sample format setting in an audio ALconfig "
"structure.");

static PyObject *
alc_GetSampFmt(alcobject *self, PyObject *args)
{
    return GetConfig(self, args, alGetSampFmt);
}


PyDoc_STRVAR(alc_SetChannels__doc__,
"alSetChannels: set the channel settings in an audio ALconfig.");

static PyObject *
alc_SetChannels(alcobject *self, PyObject *args)
{
    return SetConfig(self, args, alSetChannels);
}


PyDoc_STRVAR(alc_GetChannels__doc__,
"alGetChannels: get the channel settings in an audio ALconfig.");

static PyObject *
alc_GetChannels(alcobject *self, PyObject *args)
{
    return GetConfig(self, args, alGetChannels);
}


PyDoc_STRVAR(alc_SetFloatMax__doc__,
"alSetFloatMax: set the maximum value of floating point sample data.");

static PyObject *
alc_SetFloatMax(alcobject *self, PyObject *args)
{
    double maximum_value;

    if (!PyArg_ParseTuple(args, "d:SetFloatMax", &maximum_value))
        return NULL;
    if (alSetFloatMax(self->config, maximum_value) < 0)
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}


PyDoc_STRVAR(alc_GetFloatMax__doc__,
"alGetFloatMax: get the maximum value of floating point sample data.");

static PyObject *
alc_GetFloatMax(alcobject *self, PyObject *args)
{
    double maximum_value;

    if (!PyArg_ParseTuple(args, ":GetFloatMax"))
        return NULL;
    if ((maximum_value = alGetFloatMax(self->config)) == 0)
        return NULL;
    return PyFloat_FromDouble(maximum_value);
}


PyDoc_STRVAR(alc_SetDevice__doc__,
"alSetDevice: set the device setting in an audio ALconfig structure.");

static PyObject *
alc_SetDevice(alcobject *self, PyObject *args)
{
    return SetConfig(self, args, alSetDevice);
}


PyDoc_STRVAR(alc_GetDevice__doc__,
"alGetDevice: get the device setting in an audio ALconfig structure.");

static PyObject *
alc_GetDevice(alcobject *self, PyObject *args)
{
    return GetConfig(self, args, alGetDevice);
}


PyDoc_STRVAR(alc_SetQueueSize__doc__,
"alSetQueueSize: set audio port buffer size.");

static PyObject *
alc_SetQueueSize(alcobject *self, PyObject *args)
{
    return SetConfig(self, args, alSetQueueSize);
}


PyDoc_STRVAR(alc_GetQueueSize__doc__,
"alGetQueueSize: get audio port buffer size.");

static PyObject *
alc_GetQueueSize(alcobject *self, PyObject *args)
{
    return GetConfig(self, args, alGetQueueSize);
}

#endif /* AL_NO_ELEM */

static PyObject *
setconfig(alcobject *self, PyObject *args, int (*func)(ALconfig, long))
{
    long par;

    if (!PyArg_ParseTuple(args, "l:SetConfig", &par))
        return NULL;

    if ((*func)(self->config, par) == -1)
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
getconfig(alcobject *self, PyObject *args, long (*func)(ALconfig))
{
    long par;

    if (!PyArg_ParseTuple(args, ":GetConfig"))
        return NULL;

    if ((par = (*func)(self->config)) == -1)
        return NULL;

    return PyInt_FromLong((long) par);
}

static PyObject *
alc_setqueuesize (alcobject *self, PyObject *args)
{
    return setconfig(self, args, ALsetqueuesize);
}

static PyObject *
alc_getqueuesize (alcobject *self, PyObject *args)
{
    return getconfig(self, args, ALgetqueuesize);
}

static PyObject *
alc_setwidth (alcobject *self, PyObject *args)
{
    return setconfig(self, args, ALsetwidth);
}

static PyObject *
alc_getwidth (alcobject *self, PyObject *args)
{
    return getconfig(self, args, ALgetwidth);
}

static PyObject *
alc_getchannels (alcobject *self, PyObject *args)
{
    return getconfig(self, args, ALgetchannels);
}

static PyObject *
alc_setchannels (alcobject *self, PyObject *args)
{
    return setconfig(self, args, ALsetchannels);
}

#ifdef AL_405

static PyObject *
alc_getsampfmt (alcobject *self, PyObject *args)
{
    return getconfig(self, args, ALgetsampfmt);
}

static PyObject *
alc_setsampfmt (alcobject *self, PyObject *args)
{
    return setconfig(self, args, ALsetsampfmt);
}

static PyObject *
alc_getfloatmax(alcobject *self, PyObject *args)
{
    double arg;

    if (!PyArg_ParseTuple(args, ":GetFloatMax"))
        return 0;
    if ((arg = ALgetfloatmax(self->config)) == 0)
        return NULL;
    return PyFloat_FromDouble(arg);
}

static PyObject *
alc_setfloatmax(alcobject *self, PyObject *args)
{
    double arg;

    if (!PyArg_ParseTuple(args, "d:SetFloatMax", &arg))
        return 0;
    if (ALsetfloatmax(self->config, arg) == -1)
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}
#endif /* AL_405 */

static struct PyMethodDef alc_methods[] = {
#ifdef AL_NO_ELEM               /* IRIX 6 */
    {"SetWidth",        (PyCFunction)alc_SetWidth,      METH_VARARGS,   alc_SetWidth__doc__},
    {"GetWidth",        (PyCFunction)alc_GetWidth,      METH_VARARGS,   alc_GetWidth__doc__},
    {"SetSampFmt",      (PyCFunction)alc_SetSampFmt,    METH_VARARGS,   alc_SetSampFmt__doc__},
    {"GetSampFmt",      (PyCFunction)alc_GetSampFmt,    METH_VARARGS,   alc_GetSampFmt__doc__},
    {"SetChannels",     (PyCFunction)alc_SetChannels,   METH_VARARGS,   alc_SetChannels__doc__},
    {"GetChannels",     (PyCFunction)alc_GetChannels,   METH_VARARGS,   alc_GetChannels__doc__},
    {"SetFloatMax",     (PyCFunction)alc_SetFloatMax,   METH_VARARGS,   alc_SetFloatMax__doc__},
    {"GetFloatMax",     (PyCFunction)alc_GetFloatMax,   METH_VARARGS,   alc_GetFloatMax__doc__},
    {"SetDevice",       (PyCFunction)alc_SetDevice,     METH_VARARGS,   alc_SetDevice__doc__},
    {"GetDevice",       (PyCFunction)alc_GetDevice,     METH_VARARGS,   alc_GetDevice__doc__},
    {"SetQueueSize",            (PyCFunction)alc_SetQueueSize,  METH_VARARGS,   alc_SetQueueSize__doc__},
    {"GetQueueSize",            (PyCFunction)alc_GetQueueSize,  METH_VARARGS,   alc_GetQueueSize__doc__},
#endif /* AL_NO_ELEM */
    {"getqueuesize",            (PyCFunction)alc_getqueuesize,  METH_VARARGS},
    {"setqueuesize",            (PyCFunction)alc_setqueuesize,  METH_VARARGS},
    {"getwidth",                (PyCFunction)alc_getwidth,      METH_VARARGS},
    {"setwidth",                (PyCFunction)alc_setwidth,      METH_VARARGS},
    {"getchannels",             (PyCFunction)alc_getchannels,   METH_VARARGS},
    {"setchannels",             (PyCFunction)alc_setchannels,   METH_VARARGS},
#ifdef AL_405
    {"getsampfmt",              (PyCFunction)alc_getsampfmt,    METH_VARARGS},
    {"setsampfmt",              (PyCFunction)alc_setsampfmt,    METH_VARARGS},
    {"getfloatmax",             (PyCFunction)alc_getfloatmax,   METH_VARARGS},
    {"setfloatmax",             (PyCFunction)alc_setfloatmax,   METH_VARARGS},
#endif /* AL_405 */

    {NULL,              NULL}           /* sentinel */
};

/* ---------- */


static PyObject *
newalcobject(ALconfig config)
{
    alcobject *self;

    self = PyObject_New(alcobject, &Alctype);
    if (self == NULL)
        return NULL;
    /* XXXX Add your own initializers here */
    self->config = config;
    return (PyObject *) self;
}


static void
alc_dealloc(alcobject *self)
{
    /* XXXX Add your own cleanup code here */
#ifdef AL_NO_ELEM               /* IRIX 6 */
    (void) alFreeConfig(self->config);          /* ignore errors */
#else
    (void) ALfreeconfig(self->config);          /* ignore errors */
#endif
    PyObject_Del(self);
}

static PyObject *
alc_getattr(alcobject *self, char *name)
{
    /* XXXX Add your own getattr code here */
    return Py_FindMethod(alc_methods, (PyObject *)self, name);
}

PyDoc_STRVAR(Alctype__doc__, "");

static PyTypeObject Alctype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                                  /*ob_size*/
    "al.config",                        /*tp_name*/
    sizeof(alcobject),                  /*tp_basicsize*/
    0,                                  /*tp_itemsize*/
    /* methods */
    (destructor)alc_dealloc,            /*tp_dealloc*/
    (printfunc)0,               /*tp_print*/
    (getattrfunc)alc_getattr,           /*tp_getattr*/
    (setattrfunc)0,     /*tp_setattr*/
    (cmpfunc)0,                 /*tp_compare*/
    (reprfunc)0,                /*tp_repr*/
    0,                          /*tp_as_number*/
    0,                  /*tp_as_sequence*/
    0,                  /*tp_as_mapping*/
    (hashfunc)0,                /*tp_hash*/
    (ternaryfunc)0,             /*tp_call*/
    (reprfunc)0,                /*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    Alctype__doc__ /* Documentation string */
};

/* End of code for config objects */
/* ---------------------------------------------------------------- */

#ifdef AL_NO_ELEM               /* IRIX 6 */

PyDoc_STRVAR(alp_SetConfig__doc__,
"alSetConfig: set the ALconfig of an audio ALport.");

static PyObject *
alp_SetConfig(alpobject *self, PyObject *args)
{
    alcobject *config;
    if (!PyArg_ParseTuple(args, "O!:SetConfig", &Alctype, &config))
        return NULL;
    if (alSetConfig(self->port, config->config) < 0)
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}


PyDoc_STRVAR(alp_GetConfig__doc__,
"alGetConfig: get the ALconfig of an audio ALport.");

static PyObject *
alp_GetConfig(alpobject *self, PyObject *args)
{
    ALconfig config;
    if (!PyArg_ParseTuple(args, ":GetConfig"))
        return NULL;
    if ((config = alGetConfig(self->port)) == NULL)
        return NULL;
    return newalcobject(config);
}


PyDoc_STRVAR(alp_GetResource__doc__,
"alGetResource: get the resource associated with an audio port.");

static PyObject *
alp_GetResource(alpobject *self, PyObject *args)
{
    int resource;

    if (!PyArg_ParseTuple(args, ":GetResource"))
        return NULL;
    if ((resource = alGetResource(self->port)) == 0)
        return NULL;
    return PyInt_FromLong((long) resource);
}


PyDoc_STRVAR(alp_GetFD__doc__,
"alGetFD: get the file descriptor for an audio port.");

static PyObject *
alp_GetFD(alpobject *self, PyObject *args)
{
    int fd;

    if (!PyArg_ParseTuple(args, ":GetFD"))
        return NULL;

    if ((fd = alGetFD(self->port)) < 0)
        return NULL;

    return PyInt_FromLong((long) fd);
}


PyDoc_STRVAR(alp_GetFilled__doc__,
"alGetFilled: return the number of filled sample frames in "
"an audio port.");

static PyObject *
alp_GetFilled(alpobject *self, PyObject *args)
{
    int filled;

    if (!PyArg_ParseTuple(args, ":GetFilled"))
        return NULL;
    if ((filled = alGetFilled(self->port)) < 0)
        return NULL;
    return PyInt_FromLong((long) filled);
}


PyDoc_STRVAR(alp_GetFillable__doc__,
"alGetFillable: report the number of unfilled sample frames "
"in an audio port.");

static PyObject *
alp_GetFillable(alpobject *self, PyObject *args)
{
    int fillable;

    if (!PyArg_ParseTuple(args, ":GetFillable"))
        return NULL;
    if ((fillable = alGetFillable(self->port)) < 0)
        return NULL;
    return PyInt_FromLong((long) fillable);
}


PyDoc_STRVAR(alp_ReadFrames__doc__,
"alReadFrames: read sample frames from an audio port.");

static PyObject *
alp_ReadFrames(alpobject *self, PyObject *args)
{
    int framecount;
    PyObject *v;
    int size;
    int ch;
    ALconfig c;

    if (!PyArg_ParseTuple(args, "i:ReadFrames", &framecount))
        return NULL;
    if (framecount < 0) {
        PyErr_SetString(ErrorObject, "negative framecount");
        return NULL;
    }
    c = alGetConfig(self->port);
    switch (alGetSampFmt(c)) {
    case AL_SAMPFMT_TWOSCOMP:
        switch (alGetWidth(c)) {
        case AL_SAMPLE_8:
            size = 1;
            break;
        case AL_SAMPLE_16:
            size = 2;
            break;
        case AL_SAMPLE_24:
            size = 4;
            break;
        default:
            PyErr_SetString(ErrorObject, "can't determine width");
            alFreeConfig(c);
            return NULL;
        }
        break;
    case AL_SAMPFMT_FLOAT:
        size = 4;
        break;
    case AL_SAMPFMT_DOUBLE:
        size = 8;
        break;
    default:
        PyErr_SetString(ErrorObject, "can't determine format");
        alFreeConfig(c);
        return NULL;
    }
    ch = alGetChannels(c);
    alFreeConfig(c);
    if (ch < 0) {
        PyErr_SetString(ErrorObject, "can't determine # of channels");
        return NULL;
    }
    size *= ch;
    v = PyString_FromStringAndSize((char *) NULL, size * framecount);
    if (v == NULL)
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    alReadFrames(self->port, (void *) PyString_AS_STRING(v), framecount);
    Py_END_ALLOW_THREADS

    return v;
}


PyDoc_STRVAR(alp_DiscardFrames__doc__,
"alDiscardFrames: discard audio from an audio port.");

static PyObject *
alp_DiscardFrames(alpobject *self, PyObject *args)
{
    int framecount;

    if (!PyArg_ParseTuple(args, "i:DiscardFrames", &framecount))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    framecount = alDiscardFrames(self->port, framecount);
    Py_END_ALLOW_THREADS

    if (framecount < 0)
        return NULL;

    return PyInt_FromLong((long) framecount);
}


PyDoc_STRVAR(alp_ZeroFrames__doc__,
"alZeroFrames: write zero-valued sample frames to an audio port.");

static PyObject *
alp_ZeroFrames(alpobject *self, PyObject *args)
{
    int framecount;

    if (!PyArg_ParseTuple(args, "i:ZeroFrames", &framecount))
        return NULL;

    if (framecount < 0) {
        PyErr_SetString(ErrorObject, "negative framecount");
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    alZeroFrames(self->port, framecount);
    Py_END_ALLOW_THREADS

    Py_INCREF(Py_None);
    return Py_None;
}


PyDoc_STRVAR(alp_SetFillPoint__doc__,
"alSetFillPoint: set low- or high-water mark for an audio port.");

static PyObject *
alp_SetFillPoint(alpobject *self, PyObject *args)
{
    int fillpoint;

    if (!PyArg_ParseTuple(args, "i:SetFillPoint", &fillpoint))
        return NULL;

    if (alSetFillPoint(self->port, fillpoint) < 0)
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}


PyDoc_STRVAR(alp_GetFillPoint__doc__,
"alGetFillPoint: get low- or high-water mark for an audio port.");

static PyObject *
alp_GetFillPoint(alpobject *self, PyObject *args)
{
    int fillpoint;

    if (!PyArg_ParseTuple(args, ":GetFillPoint"))
        return NULL;

    if ((fillpoint = alGetFillPoint(self->port)) < 0)
        return NULL;

    return PyInt_FromLong((long) fillpoint);
}


PyDoc_STRVAR(alp_GetFrameNumber__doc__,
"alGetFrameNumber: get the absolute sample frame number "
"associated with a port.");

static PyObject *
alp_GetFrameNumber(alpobject *self, PyObject *args)
{
    stamp_t fnum;

    if (!PyArg_ParseTuple(args, ":GetFrameNumber"))
        return NULL;

    if (alGetFrameNumber(self->port, &fnum) < 0)
        return NULL;

    return PyLong_FromLongLong((long long) fnum);
}


PyDoc_STRVAR(alp_GetFrameTime__doc__,
"alGetFrameTime: get the time at which a sample frame came "
"in or will go out.");

static PyObject *
alp_GetFrameTime(alpobject *self, PyObject *args)
{
    stamp_t fnum, time;
    PyObject *ret, *v0, *v1;

    if (!PyArg_ParseTuple(args, ":GetFrameTime"))
        return NULL;
    if (alGetFrameTime(self->port, &fnum, &time) < 0)
        return NULL;
    v0 = PyLong_FromLongLong((long long) fnum);
    v1 = PyLong_FromLongLong((long long) time);
    if (PyErr_Occurred()) {
        Py_XDECREF(v0);
        Py_XDECREF(v1);
        return NULL;
    }
    ret = PyTuple_Pack(2, v0, v1);
    Py_DECREF(v0);
    Py_DECREF(v1);
    return ret;
}


PyDoc_STRVAR(alp_WriteFrames__doc__,
"alWriteFrames: write sample frames to an audio port.");

static PyObject *
alp_WriteFrames(alpobject *self, PyObject *args)
{
    char *samples;
    int length;
    int size, ch;
    ALconfig c;

    if (!PyArg_ParseTuple(args, "s#:WriteFrames", &samples, &length))
        return NULL;
    c = alGetConfig(self->port);
    switch (alGetSampFmt(c)) {
    case AL_SAMPFMT_TWOSCOMP:
        switch (alGetWidth(c)) {
        case AL_SAMPLE_8:
            size = 1;
            break;
        case AL_SAMPLE_16:
            size = 2;
            break;
        case AL_SAMPLE_24:
            size = 4;
            break;
        default:
            PyErr_SetString(ErrorObject, "can't determine width");
            alFreeConfig(c);
            return NULL;
        }
        break;
    case AL_SAMPFMT_FLOAT:
        size = 4;
        break;
    case AL_SAMPFMT_DOUBLE:
        size = 8;
        break;
    default:
        PyErr_SetString(ErrorObject, "can't determine format");
        alFreeConfig(c);
        return NULL;
    }
    ch = alGetChannels(c);
    alFreeConfig(c);
    if (ch < 0) {
        PyErr_SetString(ErrorObject, "can't determine # of channels");
        return NULL;
    }
    size *= ch;
    if (length % size != 0) {
        PyErr_SetString(ErrorObject,
                        "buffer length not whole number of frames");
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    alWriteFrames(self->port, (void *) samples, length / size);
    Py_END_ALLOW_THREADS

    Py_INCREF(Py_None);
    return Py_None;
}


PyDoc_STRVAR(alp_ClosePort__doc__, "alClosePort: close an audio port.");

static PyObject *
alp_ClosePort(alpobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":ClosePort"))
        return NULL;
    if (alClosePort(self->port) < 0)
        return NULL;
    self->port = NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

#endif /* AL_NO_ELEM */

#ifdef OLD_INTERFACE
static PyObject *
alp_closeport(alpobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":ClosePort"))
        return NULL;
    if (ALcloseport(self->port) < 0)
        return NULL;
    self->port = NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
alp_getfd(alpobject *self, PyObject *args)
{
    int fd;

    if (!PyArg_ParseTuple(args, ":GetFD"))
        return NULL;
    if ((fd = ALgetfd(self-> port)) == -1)
        return NULL;
    return PyInt_FromLong(fd);
}

static PyObject *
alp_getfilled(alpobject *self, PyObject *args)
{
    long count;

    if (!PyArg_ParseTuple(args, ":GetFilled"))
        return NULL;
    if ((count = ALgetfilled(self-> port)) == -1)
        return NULL;
    return PyInt_FromLong(count);
}

static PyObject *
alp_getfillable(alpobject *self, PyObject *args)
{
    long count;

    if (!PyArg_ParseTuple(args, ":GetFillable"))
        return NULL;
    if ((count = ALgetfillable(self-> port)) == -1)
        return NULL;
    return PyInt_FromLong (count);
}

static PyObject *
alp_readsamps(alpobject *self, PyObject *args)
{
    long count;
    PyObject *v;
    ALconfig c;
    int width;
    int ret;

    if (!PyArg_ParseTuple(args, "l:readsamps", &count))
        return NULL;

    if (count <= 0) {
        PyErr_SetString(ErrorObject, "al.readsamps : arg <= 0");
        return NULL;
    }

    c = ALgetconfig(self->port);
#ifdef AL_405
    width = ALgetsampfmt(c);
    if (width == AL_SAMPFMT_FLOAT)
        width = sizeof(float);
    else if (width == AL_SAMPFMT_DOUBLE)
        width = sizeof(double);
    else
        width = ALgetwidth(c);
#else
    width = ALgetwidth(c);
#endif /* AL_405 */
    ALfreeconfig(c);
    v = PyString_FromStringAndSize((char *)NULL, width * count);
    if (v == NULL)
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    ret = ALreadsamps(self->port, (void *) PyString_AsString(v), count);
    Py_END_ALLOW_THREADS
    if (ret == -1) {
        Py_DECREF(v);
        return NULL;
    }

    return (v);
}

static PyObject *
alp_writesamps(alpobject *self, PyObject *args)
{
    char *buf;
    int size, width;
    ALconfig c;
    int ret;

    if (!PyArg_ParseTuple(args, "s#:writesamps", &buf, &size))
        return NULL;

    c = ALgetconfig(self->port);
#ifdef AL_405
    width = ALgetsampfmt(c);
    if (width == AL_SAMPFMT_FLOAT)
        width = sizeof(float);
    else if (width == AL_SAMPFMT_DOUBLE)
        width = sizeof(double);
    else
        width = ALgetwidth(c);
#else
    width = ALgetwidth(c);
#endif /* AL_405 */
    ALfreeconfig(c);
    Py_BEGIN_ALLOW_THREADS
    ret = ALwritesamps (self->port, (void *) buf, (long) size / width);
    Py_END_ALLOW_THREADS
    if (ret == -1)
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
alp_getfillpoint(alpobject *self, PyObject *args)
{
    long count;

    if (!PyArg_ParseTuple(args, ":GetFillPoint"))
        return NULL;
    if ((count = ALgetfillpoint(self->port)) == -1)
        return NULL;
    return PyInt_FromLong(count);
}

static PyObject *
alp_setfillpoint(alpobject *self, PyObject *args)
{
    long count;

    if (!PyArg_ParseTuple(args, "l:SetFillPoint", &count))
        return NULL;
    if (ALsetfillpoint(self->port, count) == -1)
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
alp_setconfig(alpobject *self, PyObject *args)
{
    alcobject *config;

    if (!PyArg_ParseTuple(args, "O!:SetConfig", &Alctype, &config))
        return NULL;
    if (ALsetconfig(self->port, config->config) == -1)
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
alp_getconfig(alpobject *self, PyObject *args)
{
    ALconfig config;

    if (!PyArg_ParseTuple(args, ":GetConfig"))
        return NULL;
    config = ALgetconfig(self->port);
    if (config == NULL)
        return NULL;
    return newalcobject(config);
}

#ifdef AL_405
static PyObject *
alp_getstatus(alpobject *self, PyObject *args)
{
    PyObject *list, *v;
    long *PVbuffer;
    long length;
    int i;

    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &list))
        return NULL;
    length = PyList_Size(list);
    PVbuffer = PyMem_NEW(long, length);
    if (PVbuffer == NULL)
        return PyErr_NoMemory();
    for (i = 0; i < length; i++) {
        v = PyList_GetItem(list, i);
        if (!PyInt_Check(v)) {
            PyMem_DEL(PVbuffer);
            PyErr_BadArgument();
            return NULL;
        }
        PVbuffer[i] = PyInt_AsLong(v);
    }

    if (ALgetstatus(self->port, PVbuffer, length) == -1)
        return NULL;

    for (i = 0; i < length; i++)
        PyList_SetItem(list, i, PyInt_FromLong(PVbuffer[i]));

    PyMem_DEL(PVbuffer);

    Py_INCREF(Py_None);
    return Py_None;
}
#endif /* AL_405 */

#endif /* OLD_INTERFACE */

static struct PyMethodDef alp_methods[] = {
#ifdef AL_NO_ELEM               /* IRIX 6 */
    {"SetConfig",       (PyCFunction)alp_SetConfig,     METH_VARARGS,   alp_SetConfig__doc__},
    {"GetConfig",       (PyCFunction)alp_GetConfig,     METH_VARARGS,   alp_GetConfig__doc__},
    {"GetResource",     (PyCFunction)alp_GetResource,   METH_VARARGS,   alp_GetResource__doc__},
    {"GetFD",           (PyCFunction)alp_GetFD, METH_VARARGS,   alp_GetFD__doc__},
    {"GetFilled",       (PyCFunction)alp_GetFilled,     METH_VARARGS,   alp_GetFilled__doc__},
    {"GetFillable",     (PyCFunction)alp_GetFillable,   METH_VARARGS,   alp_GetFillable__doc__},
    {"ReadFrames",      (PyCFunction)alp_ReadFrames,    METH_VARARGS,   alp_ReadFrames__doc__},
    {"DiscardFrames",           (PyCFunction)alp_DiscardFrames, METH_VARARGS,   alp_DiscardFrames__doc__},
    {"ZeroFrames",      (PyCFunction)alp_ZeroFrames,    METH_VARARGS,   alp_ZeroFrames__doc__},
    {"SetFillPoint",            (PyCFunction)alp_SetFillPoint,  METH_VARARGS,   alp_SetFillPoint__doc__},
    {"GetFillPoint",            (PyCFunction)alp_GetFillPoint,  METH_VARARGS,   alp_GetFillPoint__doc__},
    {"GetFrameNumber",          (PyCFunction)alp_GetFrameNumber,        METH_VARARGS,   alp_GetFrameNumber__doc__},
    {"GetFrameTime",            (PyCFunction)alp_GetFrameTime,  METH_VARARGS,   alp_GetFrameTime__doc__},
    {"WriteFrames",     (PyCFunction)alp_WriteFrames,   METH_VARARGS,   alp_WriteFrames__doc__},
    {"ClosePort",       (PyCFunction)alp_ClosePort,     METH_VARARGS,   alp_ClosePort__doc__},
#endif /* AL_NO_ELEM */
#ifdef OLD_INTERFACE
    {"closeport",               (PyCFunction)alp_closeport,     METH_VARARGS},
    {"getfd",                   (PyCFunction)alp_getfd, METH_VARARGS},
    {"fileno",                  (PyCFunction)alp_getfd, METH_VARARGS},
    {"getfilled",               (PyCFunction)alp_getfilled,     METH_VARARGS},
    {"getfillable",             (PyCFunction)alp_getfillable,   METH_VARARGS},
    {"readsamps",               (PyCFunction)alp_readsamps,     METH_VARARGS},
    {"writesamps",              (PyCFunction)alp_writesamps,    METH_VARARGS},
    {"setfillpoint",            (PyCFunction)alp_setfillpoint,  METH_VARARGS},
    {"getfillpoint",            (PyCFunction)alp_getfillpoint,  METH_VARARGS},
    {"setconfig",               (PyCFunction)alp_setconfig,     METH_VARARGS},
    {"getconfig",               (PyCFunction)alp_getconfig,     METH_VARARGS},
#ifdef AL_405
    {"getstatus",               (PyCFunction)alp_getstatus,     METH_VARARGS},
#endif /* AL_405 */
#endif /* OLD_INTERFACE */

    {NULL,              NULL}           /* sentinel */
};

/* ---------- */


static PyObject *
newalpobject(ALport port)
{
    alpobject *self;

    self = PyObject_New(alpobject, &Alptype);
    if (self == NULL)
        return NULL;
    /* XXXX Add your own initializers here */
    self->port = port;
    return (PyObject *) self;
}


static void
alp_dealloc(alpobject *self)
{
    /* XXXX Add your own cleanup code here */
    if (self->port) {
#ifdef AL_NO_ELEM               /* IRIX 6 */
        alClosePort(self->port);
#else
        ALcloseport(self->port);
#endif
    }
    PyObject_Del(self);
}

static PyObject *
alp_getattr(alpobject *self, char *name)
{
    /* XXXX Add your own getattr code here */
    if (self->port == NULL) {
        PyErr_SetString(ErrorObject, "port already closed");
        return NULL;
    }
    return Py_FindMethod(alp_methods, (PyObject *)self, name);
}

PyDoc_STRVAR(Alptype__doc__, "");

static PyTypeObject Alptype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                                  /*ob_size*/
    "al.port",                          /*tp_name*/
    sizeof(alpobject),                  /*tp_basicsize*/
    0,                                  /*tp_itemsize*/
    /* methods */
    (destructor)alp_dealloc,            /*tp_dealloc*/
    (printfunc)0,               /*tp_print*/
    (getattrfunc)alp_getattr,           /*tp_getattr*/
    (setattrfunc)0,     /*tp_setattr*/
    (cmpfunc)0,                 /*tp_compare*/
    (reprfunc)0,                /*tp_repr*/
    0,                          /*tp_as_number*/
    0,                  /*tp_as_sequence*/
    0,                  /*tp_as_mapping*/
    (hashfunc)0,                /*tp_hash*/
    (ternaryfunc)0,             /*tp_call*/
    (reprfunc)0,                /*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    Alptype__doc__ /* Documentation string */
};

/* End of code for port objects */
/* -------------------------------------------------------- */


#ifdef AL_NO_ELEM               /* IRIX 6 */

PyDoc_STRVAR(al_NewConfig__doc__,
"alNewConfig: create and initialize an audio ALconfig structure.");

static PyObject *
al_NewConfig(PyObject *self, PyObject *args)
{
    ALconfig config;

    if (!PyArg_ParseTuple(args, ":NewConfig"))
        return NULL;
    if ((config = alNewConfig()) == NULL)
        return NULL;
    return newalcobject(config);
}

PyDoc_STRVAR(al_OpenPort__doc__,
"alOpenPort: open an audio port.");

static PyObject *
al_OpenPort(PyObject *self, PyObject *args)
{
    ALport port;
    char *name, *dir;
    alcobject *config = NULL;

    if (!PyArg_ParseTuple(args, "ss|O!:OpenPort", &name, &dir, &Alctype, &config))
        return NULL;
    if ((port = alOpenPort(name, dir, config ? config->config : NULL)) == NULL)
        return NULL;
    return newalpobject(port);
}

PyDoc_STRVAR(al_Connect__doc__,
"alConnect: connect two audio I/O resources.");

static PyObject *
al_Connect(PyObject *self, PyObject *args)
{
    int source, dest, nprops = 0, id, i;
    ALpv *props = NULL;
    ALparamInfo *propinfo = NULL;
    PyObject *propobj = NULL;

    if (!PyArg_ParseTuple(args, "ii|O!:Connect", &source, &dest, &PyList_Type, &propobj))
        return NULL;
    if (propobj != NULL) {
        nprops = python2params(source, dest, propobj, &props, &propinfo);
        if (nprops < 0)
            return NULL;
    }

    id = alConnect(source, dest, props, nprops);

    if (props) {
        for (i = 0; i < nprops; i++) {
            switch (propinfo[i].valueType) {
            case AL_SET_VAL:
            case AL_VECTOR_VAL:
                PyMem_DEL(props[i].value.ptr);
                break;
            }
        }
        PyMem_DEL(props);
        PyMem_DEL(propinfo);
    }

    if (id < 0)
        return NULL;
    return PyInt_FromLong((long) id);
}

PyDoc_STRVAR(al_Disconnect__doc__,
"alDisconnect: delete a connection between two audio I/O resources.");

static PyObject *
al_Disconnect(PyObject *self, PyObject *args)
{
    int res;

    if (!PyArg_ParseTuple(args, "i:Disconnect", &res))
        return NULL;
    if (alDisconnect(res) < 0)
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(al_GetParams__doc__,
"alGetParams: get the values of audio resource parameters.");

static PyObject *
al_GetParams(PyObject *self, PyObject *args)
{
    int resource;
    PyObject *pvslist, *item = NULL, *v = NULL;
    ALpv *pvs;
    int i, j, npvs;
    ALparamInfo *pinfo;

    if (!PyArg_ParseTuple(args, "iO!:GetParams", &resource, &PyList_Type, &pvslist))
        return NULL;
    npvs = PyList_Size(pvslist);
    pvs = PyMem_NEW(ALpv, npvs);
    pinfo = PyMem_NEW(ALparamInfo, npvs);
    for (i = 0; i < npvs; i++) {
        item = PyList_GetItem(pvslist, i);
        if (!PyInt_Check(item)) {
            item = NULL;
            PyErr_SetString(ErrorObject, "list of integers expected");
            goto error;
        }
        pvs[i].param = (int) PyInt_AsLong(item);
        item = NULL;            /* not needed anymore */
        if (alGetParamInfo(resource, pvs[i].param, &pinfo[i]) < 0)
            goto error;
        switch (pinfo[i].valueType) {
        case AL_NO_VAL:
            break;
        case AL_MATRIX_VAL:
            pinfo[i].maxElems *= pinfo[i].maxElems2;
            /* fall through */
        case AL_STRING_VAL:
        case AL_SET_VAL:
        case AL_VECTOR_VAL:
            switch (pinfo[i].elementType) {
            case AL_INT32_ELEM:
            case AL_RESOURCE_ELEM:
            case AL_ENUM_ELEM:
                pvs[i].value.ptr = PyMem_NEW(int, pinfo[i].maxElems);
                pvs[i].sizeIn = pinfo[i].maxElems;
                break;
            case AL_INT64_ELEM:
            case AL_FIXED_ELEM:
                pvs[i].value.ptr = PyMem_NEW(long long, pinfo[i].maxElems);
                pvs[i].sizeIn = pinfo[i].maxElems;
                break;
            case AL_CHAR_ELEM:
                pvs[i].value.ptr = PyMem_NEW(char, 32);
                pvs[i].sizeIn = 32;
                break;
            case AL_NO_ELEM:
            case AL_PTR_ELEM:
            default:
                PyErr_SetString(ErrorObject, "internal error");
                goto error;
            }
            break;
        case AL_SCALAR_VAL:
            break;
        default:
            PyErr_SetString(ErrorObject, "internal error");
            goto error;
        }
        if (pinfo[i].valueType == AL_MATRIX_VAL) {
            pinfo[i].maxElems /= pinfo[i].maxElems2;
            pvs[i].sizeIn /= pinfo[i].maxElems2;
            pvs[i].size2In = pinfo[i].maxElems2;
        }
    }
    if (alGetParams(resource, pvs, npvs) < 0)
        goto error;
    if (!(v = PyList_New(npvs)))
        goto error;
    for (i = 0; i < npvs; i++) {
        if (pvs[i].sizeOut < 0) {
            char buf[32];
            PyOS_snprintf(buf, sizeof(buf),
                          "problem with param %d", i);
            PyErr_SetString(ErrorObject, buf);
            goto error;
        }
        switch (pinfo[i].valueType) {
        case AL_NO_VAL:
            item = Py_None;
            Py_INCREF(item);
            break;
        case AL_STRING_VAL:
            item = PyString_FromString(pvs[i].value.ptr);
            PyMem_DEL(pvs[i].value.ptr);
            break;
        case AL_MATRIX_VAL:
            /* XXXX this is not right */
            pvs[i].sizeOut *= pvs[i].size2Out;
            /* fall through */
        case AL_SET_VAL:
        case AL_VECTOR_VAL:
            item = PyList_New(pvs[i].sizeOut);
            for (j = 0; j < pvs[i].sizeOut; j++) {
                switch (pinfo[i].elementType) {
                case AL_INT32_ELEM:
                case AL_RESOURCE_ELEM:
                case AL_ENUM_ELEM:
                    PyList_SetItem(item, j, PyInt_FromLong((long) ((int *) pvs[i].value.ptr)[j]));
                    break;
                case AL_INT64_ELEM:
                    PyList_SetItem(item, j, PyLong_FromLongLong(((long long *) pvs[i].value.ptr)[j]));
                    break;
                case AL_FIXED_ELEM:
                    PyList_SetItem(item, j, PyFloat_FromDouble(alFixedToDouble(((long long *) pvs[i].value.ptr)[j])));
                    break;
                default:
                    PyErr_SetString(ErrorObject, "internal error");
                    goto error;
                }
            }
            PyMem_DEL(pvs[i].value.ptr);
            break;
        case AL_SCALAR_VAL:
            item = param2python(resource, pvs[i].param, pvs[i].value, &pinfo[i]);
            break;
        }
        if (PyErr_Occurred() ||
            PyList_SetItem(v, i, Py_BuildValue("(iO)", pvs[i].param,
                                               item)) < 0 ||
            PyErr_Occurred())
            goto error;
        Py_DECREF(item);
    }
    PyMem_DEL(pvs);
    PyMem_DEL(pinfo);
    return v;

  error:
    Py_XDECREF(v);
    Py_XDECREF(item);
    if (pvs)
        PyMem_DEL(pvs);
    if (pinfo)
        PyMem_DEL(pinfo);
    return NULL;
}

PyDoc_STRVAR(al_SetParams__doc__,
"alSetParams: set the values of audio resource parameters.");

static PyObject *
al_SetParams(PyObject *self, PyObject *args)
{
    int resource;
    PyObject *pvslist;
    ALpv *pvs;
    ALparamInfo *pinfo;
    int npvs, i;

    if (!PyArg_ParseTuple(args, "iO!:SetParams", &resource, &PyList_Type, &pvslist))
        return NULL;
    npvs = python2params(resource, -1, pvslist, &pvs, &pinfo);
    if (npvs < 0)
        return NULL;

    if (alSetParams(resource, pvs, npvs) < 0)
        goto error;

    /* cleanup */
    for (i = 0; i < npvs; i++) {
        switch (pinfo[i].valueType) {
        case AL_SET_VAL:
        case AL_VECTOR_VAL:
            PyMem_DEL(pvs[i].value.ptr);
            break;
        }
    }
    PyMem_DEL(pvs);
    PyMem_DEL(pinfo);

    Py_INCREF(Py_None);
    return Py_None;

  error:
    /* XXXX we should clean up everything */
    if (pvs)
        PyMem_DEL(pvs);
    if (pinfo)
        PyMem_DEL(pinfo);
    return NULL;
}

PyDoc_STRVAR(al_QueryValues__doc__,
"alQueryValues: get the set of possible values for a parameter.");

static PyObject *
al_QueryValues(PyObject *self, PyObject *args)
{
    int resource, param;
    ALvalue *return_set = NULL;
    int setsize = 32, qualsize = 0, nvals, i;
    ALpv *quals = NULL;
    ALparamInfo pinfo;
    ALparamInfo *qualinfo = NULL;
    PyObject *qualobj = NULL;
    PyObject *res = NULL, *item;

    if (!PyArg_ParseTuple(args, "ii|O!:QueryValues", &resource, &param,
                          &PyList_Type, &qualobj))
        return NULL;
    if (qualobj != NULL) {
        qualsize = python2params(resource, param, qualobj, &quals, &qualinfo);
        if (qualsize < 0)
            return NULL;
    }
    setsize = 32;
    return_set = PyMem_NEW(ALvalue, setsize);
    if (return_set == NULL) {
        PyErr_NoMemory();
        goto cleanup;
    }

  retry:
    nvals = alQueryValues(resource, param, return_set, setsize, quals, qualsize);
    if (nvals < 0)
        goto cleanup;
    if (nvals > setsize) {
        ALvalue *old_return_set = return_set;
        setsize = nvals;
        PyMem_RESIZE(return_set, ALvalue, setsize);
        if (return_set == NULL) {
            return_set = old_return_set;
            PyErr_NoMemory();
            goto cleanup;
        }
        goto retry;
    }

    if (alGetParamInfo(resource, param, &pinfo) < 0)
        goto cleanup;

    res = PyList_New(nvals);
    if (res == NULL)
        goto cleanup;
    for (i = 0; i < nvals; i++) {
        item = param2python(resource, param, return_set[i], &pinfo);
        if (item == NULL ||
            PyList_SetItem(res, i, item) < 0) {
            Py_DECREF(res);
            res = NULL;
            goto cleanup;
        }
    }

  cleanup:
    if (return_set)
        PyMem_DEL(return_set);
    if (quals) {
        for (i = 0; i < qualsize; i++) {
            switch (qualinfo[i].valueType) {
            case AL_SET_VAL:
            case AL_VECTOR_VAL:
                PyMem_DEL(quals[i].value.ptr);
                break;
            }
        }
        PyMem_DEL(quals);
        PyMem_DEL(qualinfo);
    }

    return res;
}

PyDoc_STRVAR(al_GetParamInfo__doc__,
"alGetParamInfo: get information about a parameter on "
"a particular audio resource.");

static PyObject *
al_GetParamInfo(PyObject *self, PyObject *args)
{
    int res, param;
    ALparamInfo pinfo;
    PyObject *v, *item;

    if (!PyArg_ParseTuple(args, "ii:GetParamInfo", &res, &param))
        return NULL;
    if (alGetParamInfo(res, param, &pinfo) < 0)
        return NULL;
    v = PyDict_New();
    if (!v) return NULL;

    item = PyInt_FromLong((long) pinfo.resource);
    PyDict_SetItemString(v, "resource", item);
    Py_DECREF(item);

    item = PyInt_FromLong((long) pinfo.param);
    PyDict_SetItemString(v, "param", item);
    Py_DECREF(item);

    item = PyInt_FromLong((long) pinfo.valueType);
    PyDict_SetItemString(v, "valueType", item);
    Py_DECREF(item);

    if (pinfo.valueType != AL_NO_VAL && pinfo.valueType != AL_SCALAR_VAL) {
        /* multiple values */
        item = PyInt_FromLong((long) pinfo.maxElems);
        PyDict_SetItemString(v, "maxElems", item);
        Py_DECREF(item);

        if (pinfo.valueType == AL_MATRIX_VAL) {
            /* 2 dimensional */
            item = PyInt_FromLong((long) pinfo.maxElems2);
            PyDict_SetItemString(v, "maxElems2", item);
            Py_DECREF(item);
        }
    }

    item = PyInt_FromLong((long) pinfo.elementType);
    PyDict_SetItemString(v, "elementType", item);
    Py_DECREF(item);

    item = PyString_FromString(pinfo.name);
    PyDict_SetItemString(v, "name", item);
    Py_DECREF(item);

    item = param2python(res, param, pinfo.initial, &pinfo);
    PyDict_SetItemString(v, "initial", item);
    Py_DECREF(item);

    if (pinfo.elementType != AL_ENUM_ELEM &&
        pinfo.elementType != AL_RESOURCE_ELEM &&
        pinfo.elementType != AL_CHAR_ELEM) {
        /* range param */
        item = param2python(res, param, pinfo.min, &pinfo);
        PyDict_SetItemString(v, "min", item);
        Py_DECREF(item);

        item = param2python(res, param, pinfo.max, &pinfo);
        PyDict_SetItemString(v, "max", item);
        Py_DECREF(item);

        item = param2python(res, param, pinfo.minDelta, &pinfo);
        PyDict_SetItemString(v, "minDelta", item);
        Py_DECREF(item);

        item = param2python(res, param, pinfo.maxDelta, &pinfo);
        PyDict_SetItemString(v, "maxDelta", item);
        Py_DECREF(item);

        item = PyInt_FromLong((long) pinfo.specialVals);
        PyDict_SetItemString(v, "specialVals", item);
        Py_DECREF(item);
    }

    return v;
}

PyDoc_STRVAR(al_GetResourceByName__doc__,
"alGetResourceByName: find an audio resource by name.");

static PyObject *
al_GetResourceByName(PyObject *self, PyObject *args)
{
    int res, start_res, type;
    char *name;

    if (!PyArg_ParseTuple(args, "isi:GetResourceByName", &start_res, &name, &type))
        return NULL;
    if ((res = alGetResourceByName(start_res, name, type)) == 0)
        return NULL;
    return PyInt_FromLong((long) res);
}

PyDoc_STRVAR(al_IsSubtype__doc__,
"alIsSubtype: indicate if one resource type is a subtype of another.");

static PyObject *
al_IsSubtype(PyObject *self, PyObject *args)
{
    int type, subtype;

    if (!PyArg_ParseTuple(args, "ii:IsSubtype", &type, &subtype))
        return NULL;
    return PyInt_FromLong((long) alIsSubtype(type, subtype));
}

PyDoc_STRVAR(al_SetErrorHandler__doc__, "");

static PyObject *
al_SetErrorHandler(PyObject *self, PyObject *args)
{

    if (!PyArg_ParseTuple(args, ":SetErrorHandler"))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

#endif /* AL_NO_ELEM */

#ifdef OLD_INTERFACE

static PyObject *
al_openport(PyObject *self, PyObject *args)
{
    char *name, *dir;
    ALport port;
    alcobject *config = NULL;

    if (!PyArg_ParseTuple(args, "ss|O!:OpenPort", &name, &dir, &Alctype, &config))
        return NULL;
    if ((port = ALopenport(name, dir, config ? config->config : NULL)) == NULL)
        return NULL;
    return newalpobject(port);
}

static PyObject *
al_newconfig(PyObject *self, PyObject *args)
{
    ALconfig config;

    if (!PyArg_ParseTuple(args, ":NewConfig"))
        return NULL;
    if ((config = ALnewconfig ()) == NULL)
        return NULL;
    return newalcobject(config);
}

static PyObject *
al_queryparams(PyObject *self, PyObject *args)
{
    long device;
    long length;
    long *PVbuffer;
    long PVdummy[2];
    PyObject *v = NULL;
    int i;

    if (!PyArg_ParseTuple(args, "l:queryparams", &device))
        return NULL;
    if ((length = ALqueryparams(device, PVdummy, 2L)) == -1)
        return NULL;
    if ((PVbuffer = PyMem_NEW(long, length)) == NULL)
        return PyErr_NoMemory();
    if (ALqueryparams(device, PVbuffer, length) >= 0 &&
        (v = PyList_New((int)length)) != NULL) {
        for (i = 0; i < length; i++)
            PyList_SetItem(v, i, PyInt_FromLong(PVbuffer[i]));
    }
    PyMem_DEL(PVbuffer);
    return v;
}

static PyObject *
doParams(PyObject *args, int (*func)(long, long *, long), int modified)
{
    long device;
    PyObject *list, *v;
    long *PVbuffer;
    long length;
    int i;

    if (!PyArg_ParseTuple(args, "lO!", &device, &PyList_Type, &list))
        return NULL;
    length = PyList_Size(list);
    PVbuffer = PyMem_NEW(long, length);
    if (PVbuffer == NULL)
        return PyErr_NoMemory();
    for (i = 0; i < length; i++) {
        v = PyList_GetItem(list, i);
        if (!PyInt_Check(v)) {
            PyMem_DEL(PVbuffer);
            PyErr_BadArgument();
            return NULL;
        }
        PVbuffer[i] = PyInt_AsLong(v);
    }

    if ((*func)(device, PVbuffer, length) == -1) {
        PyMem_DEL(PVbuffer);
        return NULL;
    }

    if (modified) {
        for (i = 0; i < length; i++)
            PyList_SetItem(list, i, PyInt_FromLong(PVbuffer[i]));
    }

    PyMem_DEL(PVbuffer);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
al_getparams(PyObject *self, PyObject *args)
{
    return doParams(args, ALgetparams, 1);
}

static PyObject *
al_setparams(PyObject *self, PyObject *args)
{
    return doParams(args, ALsetparams, 0);
}

static PyObject *
al_getname(PyObject *self, PyObject *args)
{
    long device, descriptor;
    char *name;

    if (!PyArg_ParseTuple(args, "ll:getname", &device, &descriptor))
        return NULL;
    if ((name = ALgetname(device, descriptor)) == NULL)
        return NULL;
    return PyString_FromString(name);
}

static PyObject *
al_getdefault(PyObject *self, PyObject *args)
{
    long device, descriptor, value;

    if (!PyArg_ParseTuple(args, "ll:getdefault", &device, &descriptor))
        return NULL;
    if ((value = ALgetdefault(device, descriptor)) == -1)
        return NULL;
    return PyLong_FromLong(value);
}

static PyObject *
al_getminmax(PyObject *self, PyObject *args)
{
    long device, descriptor, min, max;

    if (!PyArg_ParseTuple(args, "ll:getminmax", &device, &descriptor))
        return NULL;
    min = -1;
    max = -1;
    if (ALgetminmax(device, descriptor, &min, &max) == -1)
        return NULL;
    return Py_BuildValue("ll", min, max);
}

#endif /* OLD_INTERFACE */

/* List of methods defined in the module */

static struct PyMethodDef al_methods[] = {
#ifdef AL_NO_ELEM               /* IRIX 6 */
    {"NewConfig",       (PyCFunction)al_NewConfig,      METH_VARARGS,   al_NewConfig__doc__},
    {"OpenPort",        (PyCFunction)al_OpenPort,       METH_VARARGS,   al_OpenPort__doc__},
    {"Connect",         (PyCFunction)al_Connect,        METH_VARARGS,   al_Connect__doc__},
    {"Disconnect",      (PyCFunction)al_Disconnect,     METH_VARARGS,   al_Disconnect__doc__},
    {"GetParams",       (PyCFunction)al_GetParams,      METH_VARARGS,   al_GetParams__doc__},
    {"SetParams",       (PyCFunction)al_SetParams,      METH_VARARGS,   al_SetParams__doc__},
    {"QueryValues",     (PyCFunction)al_QueryValues,    METH_VARARGS,   al_QueryValues__doc__},
    {"GetParamInfo",            (PyCFunction)al_GetParamInfo,   METH_VARARGS,   al_GetParamInfo__doc__},
    {"GetResourceByName",       (PyCFunction)al_GetResourceByName,      METH_VARARGS,   al_GetResourceByName__doc__},
    {"IsSubtype",       (PyCFunction)al_IsSubtype,      METH_VARARGS,   al_IsSubtype__doc__},
#if 0
    /* this one not supported */
    {"SetErrorHandler",         (PyCFunction)al_SetErrorHandler,        METH_VARARGS,   al_SetErrorHandler__doc__},
#endif
#endif /* AL_NO_ELEM */
#ifdef OLD_INTERFACE
    {"openport",                (PyCFunction)al_openport,       METH_VARARGS},
    {"newconfig",               (PyCFunction)al_newconfig,      METH_VARARGS},
    {"queryparams",             (PyCFunction)al_queryparams,    METH_VARARGS},
    {"getparams",               (PyCFunction)al_getparams,      METH_VARARGS},
    {"setparams",               (PyCFunction)al_setparams,      METH_VARARGS},
    {"getname",                 (PyCFunction)al_getname,        METH_VARARGS},
    {"getdefault",              (PyCFunction)al_getdefault,     METH_VARARGS},
    {"getminmax",               (PyCFunction)al_getminmax,      METH_VARARGS},
#endif /* OLD_INTERFACE */

    {NULL,       (PyCFunction)NULL, 0, NULL}            /* sentinel */
};


/* Initialization function for the module (*must* be called inital) */

PyDoc_STRVAR(al_module_documentation, "");

void
inital(void)
{
    PyObject *m, *d, *x;

    if (PyErr_WarnPy3k("the al module has been removed in "
                       "Python 3.0", 2) < 0)
        return;

    /* Create the module and add the functions */
    m = Py_InitModule4("al", al_methods,
        al_module_documentation,
        (PyObject*)NULL,PYTHON_API_VERSION);
    if (m == NULL)
        return;

    /* Add some symbolic constants to the module */
    d = PyModule_GetDict(m);
    ErrorObject = PyErr_NewException("al.error", NULL, NULL);
    PyDict_SetItemString(d, "error", ErrorObject);

    /* XXXX Add constants here */
#ifdef AL_4CHANNEL
    x =  PyInt_FromLong((long) AL_4CHANNEL);
    if (x == NULL || PyDict_SetItemString(d, "FOURCHANNEL", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_ADAT_IF_TYPE
    x =  PyInt_FromLong((long) AL_ADAT_IF_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "ADAT_IF_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_ADAT_MCLK_TYPE
    x =  PyInt_FromLong((long) AL_ADAT_MCLK_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "ADAT_MCLK_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_AES_IF_TYPE
    x =  PyInt_FromLong((long) AL_AES_IF_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "AES_IF_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_AES_MCLK_TYPE
    x =  PyInt_FromLong((long) AL_AES_MCLK_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "AES_MCLK_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_ANALOG_IF_TYPE
    x =  PyInt_FromLong((long) AL_ANALOG_IF_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "ANALOG_IF_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_ASSOCIATE
    x =  PyInt_FromLong((long) AL_ASSOCIATE);
    if (x == NULL || PyDict_SetItemString(d, "ASSOCIATE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_BUFFER_NULL
    x =  PyInt_FromLong((long) AL_BAD_BUFFER_NULL);
    if (x == NULL || PyDict_SetItemString(d, "BAD_BUFFER_NULL", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_BUFFERLENGTH
    x =  PyInt_FromLong((long) AL_BAD_BUFFERLENGTH);
    if (x == NULL || PyDict_SetItemString(d, "BAD_BUFFERLENGTH", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_BUFFERLENGTH_NEG
    x =  PyInt_FromLong((long) AL_BAD_BUFFERLENGTH_NEG);
    if (x == NULL || PyDict_SetItemString(d, "BAD_BUFFERLENGTH_NEG", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_BUFFERLENGTH_ODD
    x =  PyInt_FromLong((long) AL_BAD_BUFFERLENGTH_ODD);
    if (x == NULL || PyDict_SetItemString(d, "BAD_BUFFERLENGTH_ODD", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_CHANNELS
    x =  PyInt_FromLong((long) AL_BAD_CHANNELS);
    if (x == NULL || PyDict_SetItemString(d, "BAD_CHANNELS", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_CONFIG
    x =  PyInt_FromLong((long) AL_BAD_CONFIG);
    if (x == NULL || PyDict_SetItemString(d, "BAD_CONFIG", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_COUNT_NEG
    x =  PyInt_FromLong((long) AL_BAD_COUNT_NEG);
    if (x == NULL || PyDict_SetItemString(d, "BAD_COUNT_NEG", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_DEVICE
    x =  PyInt_FromLong((long) AL_BAD_DEVICE);
    if (x == NULL || PyDict_SetItemString(d, "BAD_DEVICE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_DEVICE_ACCESS
    x =  PyInt_FromLong((long) AL_BAD_DEVICE_ACCESS);
    if (x == NULL || PyDict_SetItemString(d, "BAD_DEVICE_ACCESS", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_DIRECTION
    x =  PyInt_FromLong((long) AL_BAD_DIRECTION);
    if (x == NULL || PyDict_SetItemString(d, "BAD_DIRECTION", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_FILLPOINT
    x =  PyInt_FromLong((long) AL_BAD_FILLPOINT);
    if (x == NULL || PyDict_SetItemString(d, "BAD_FILLPOINT", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_FLOATMAX
    x =  PyInt_FromLong((long) AL_BAD_FLOATMAX);
    if (x == NULL || PyDict_SetItemString(d, "BAD_FLOATMAX", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_ILLEGAL_STATE
    x =  PyInt_FromLong((long) AL_BAD_ILLEGAL_STATE);
    if (x == NULL || PyDict_SetItemString(d, "BAD_ILLEGAL_STATE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_NO_PORTS
    x =  PyInt_FromLong((long) AL_BAD_NO_PORTS);
    if (x == NULL || PyDict_SetItemString(d, "BAD_NO_PORTS", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_NOT_FOUND
    x =  PyInt_FromLong((long) AL_BAD_NOT_FOUND);
    if (x == NULL || PyDict_SetItemString(d, "BAD_NOT_FOUND", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_NOT_IMPLEMENTED
    x =  PyInt_FromLong((long) AL_BAD_NOT_IMPLEMENTED);
    if (x == NULL || PyDict_SetItemString(d, "BAD_NOT_IMPLEMENTED", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_OUT_OF_MEM
    x =  PyInt_FromLong((long) AL_BAD_OUT_OF_MEM);
    if (x == NULL || PyDict_SetItemString(d, "BAD_OUT_OF_MEM", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_PARAM
    x =  PyInt_FromLong((long) AL_BAD_PARAM);
    if (x == NULL || PyDict_SetItemString(d, "BAD_PARAM", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_PERMISSIONS
    x =  PyInt_FromLong((long) AL_BAD_PERMISSIONS);
    if (x == NULL || PyDict_SetItemString(d, "BAD_PERMISSIONS", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_PORT
    x =  PyInt_FromLong((long) AL_BAD_PORT);
    if (x == NULL || PyDict_SetItemString(d, "BAD_PORT", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_PORTSTYLE
    x =  PyInt_FromLong((long) AL_BAD_PORTSTYLE);
    if (x == NULL || PyDict_SetItemString(d, "BAD_PORTSTYLE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_PVBUFFER
    x =  PyInt_FromLong((long) AL_BAD_PVBUFFER);
    if (x == NULL || PyDict_SetItemString(d, "BAD_PVBUFFER", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_QSIZE
    x =  PyInt_FromLong((long) AL_BAD_QSIZE);
    if (x == NULL || PyDict_SetItemString(d, "BAD_QSIZE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_RATE
    x =  PyInt_FromLong((long) AL_BAD_RATE);
    if (x == NULL || PyDict_SetItemString(d, "BAD_RATE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_RESOURCE
    x =  PyInt_FromLong((long) AL_BAD_RESOURCE);
    if (x == NULL || PyDict_SetItemString(d, "BAD_RESOURCE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_SAMPFMT
    x =  PyInt_FromLong((long) AL_BAD_SAMPFMT);
    if (x == NULL || PyDict_SetItemString(d, "BAD_SAMPFMT", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_TRANSFER_SIZE
    x =  PyInt_FromLong((long) AL_BAD_TRANSFER_SIZE);
    if (x == NULL || PyDict_SetItemString(d, "BAD_TRANSFER_SIZE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_BAD_WIDTH
    x =  PyInt_FromLong((long) AL_BAD_WIDTH);
    if (x == NULL || PyDict_SetItemString(d, "BAD_WIDTH", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_CHANNEL_MODE
    x =  PyInt_FromLong((long) AL_CHANNEL_MODE);
    if (x == NULL || PyDict_SetItemString(d, "CHANNEL_MODE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_CHANNELS
    x =  PyInt_FromLong((long) AL_CHANNELS);
    if (x == NULL || PyDict_SetItemString(d, "CHANNELS", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_CHAR_ELEM
    x =  PyInt_FromLong((long) AL_CHAR_ELEM);
    if (x == NULL || PyDict_SetItemString(d, "CHAR_ELEM", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_CLOCK_GEN
    x =  PyInt_FromLong((long) AL_CLOCK_GEN);
    if (x == NULL || PyDict_SetItemString(d, "CLOCK_GEN", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_CLOCKGEN_TYPE
    x =  PyInt_FromLong((long) AL_CLOCKGEN_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "CLOCKGEN_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_CONNECT
    x =  PyInt_FromLong((long) AL_CONNECT);
    if (x == NULL || PyDict_SetItemString(d, "CONNECT", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_CONNECTION_TYPE
    x =  PyInt_FromLong((long) AL_CONNECTION_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "CONNECTION_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_CONNECTIONS
    x =  PyInt_FromLong((long) AL_CONNECTIONS);
    if (x == NULL || PyDict_SetItemString(d, "CONNECTIONS", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_CRYSTAL_MCLK_TYPE
    x =  PyInt_FromLong((long) AL_CRYSTAL_MCLK_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "CRYSTAL_MCLK_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_DEFAULT_DEVICE
    x =  PyInt_FromLong((long) AL_DEFAULT_DEVICE);
    if (x == NULL || PyDict_SetItemString(d, "DEFAULT_DEVICE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_DEFAULT_INPUT
    x =  PyInt_FromLong((long) AL_DEFAULT_INPUT);
    if (x == NULL || PyDict_SetItemString(d, "DEFAULT_INPUT", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_DEFAULT_OUTPUT
    x =  PyInt_FromLong((long) AL_DEFAULT_OUTPUT);
    if (x == NULL || PyDict_SetItemString(d, "DEFAULT_OUTPUT", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_DEST
    x =  PyInt_FromLong((long) AL_DEST);
    if (x == NULL || PyDict_SetItemString(d, "DEST", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_DEVICE_TYPE
    x =  PyInt_FromLong((long) AL_DEVICE_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "DEVICE_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_DEVICES
    x =  PyInt_FromLong((long) AL_DEVICES);
    if (x == NULL || PyDict_SetItemString(d, "DEVICES", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_DIGITAL_IF_TYPE
    x =  PyInt_FromLong((long) AL_DIGITAL_IF_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "DIGITAL_IF_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_DIGITAL_INPUT_RATE
    x =  PyInt_FromLong((long) AL_DIGITAL_INPUT_RATE);
    if (x == NULL || PyDict_SetItemString(d, "DIGITAL_INPUT_RATE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_DISCONNECT
    x =  PyInt_FromLong((long) AL_DISCONNECT);
    if (x == NULL || PyDict_SetItemString(d, "DISCONNECT", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_ENUM_ELEM
    x =  PyInt_FromLong((long) AL_ENUM_ELEM);
    if (x == NULL || PyDict_SetItemString(d, "ENUM_ELEM", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_ENUM_VALUE
    x =  PyInt_FromLong((long) AL_ENUM_VALUE);
    if (x == NULL || PyDict_SetItemString(d, "ENUM_VALUE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_ERROR_INPUT_OVERFLOW
    x =  PyInt_FromLong((long) AL_ERROR_INPUT_OVERFLOW);
    if (x == NULL || PyDict_SetItemString(d, "ERROR_INPUT_OVERFLOW", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_ERROR_LENGTH
    x =  PyInt_FromLong((long) AL_ERROR_LENGTH);
    if (x == NULL || PyDict_SetItemString(d, "ERROR_LENGTH", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_ERROR_LOCATION_LSP
    x =  PyInt_FromLong((long) AL_ERROR_LOCATION_LSP);
    if (x == NULL || PyDict_SetItemString(d, "ERROR_LOCATION_LSP", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_ERROR_LOCATION_MSP
    x =  PyInt_FromLong((long) AL_ERROR_LOCATION_MSP);
    if (x == NULL || PyDict_SetItemString(d, "ERROR_LOCATION_MSP", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_ERROR_NUMBER
    x =  PyInt_FromLong((long) AL_ERROR_NUMBER);
    if (x == NULL || PyDict_SetItemString(d, "ERROR_NUMBER", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_ERROR_OUTPUT_UNDERFLOW
    x =  PyInt_FromLong((long) AL_ERROR_OUTPUT_UNDERFLOW);
    if (x == NULL || PyDict_SetItemString(d, "ERROR_OUTPUT_UNDERFLOW", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_ERROR_TYPE
    x =  PyInt_FromLong((long) AL_ERROR_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "ERROR_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_FIXED_ELEM
    x =  PyInt_FromLong((long) AL_FIXED_ELEM);
    if (x == NULL || PyDict_SetItemString(d, "FIXED_ELEM", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_FIXED_MCLK_TYPE
    x =  PyInt_FromLong((long) AL_FIXED_MCLK_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "FIXED_MCLK_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_GAIN
    x =  PyInt_FromLong((long) AL_GAIN);
    if (x == NULL || PyDict_SetItemString(d, "GAIN", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_GAIN_REF
    x =  PyInt_FromLong((long) AL_GAIN_REF);
    if (x == NULL || PyDict_SetItemString(d, "GAIN_REF", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_HRB_TYPE
    x =  PyInt_FromLong((long) AL_HRB_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "HRB_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_INPUT_COUNT
    x =  PyInt_FromLong((long) AL_INPUT_COUNT);
    if (x == NULL || PyDict_SetItemString(d, "INPUT_COUNT", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_INPUT_DEVICE_TYPE
    x =  PyInt_FromLong((long) AL_INPUT_DEVICE_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "INPUT_DEVICE_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_INPUT_DIGITAL
    x =  PyInt_FromLong((long) AL_INPUT_DIGITAL);
    if (x == NULL || PyDict_SetItemString(d, "INPUT_DIGITAL", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_INPUT_HRB_TYPE
    x =  PyInt_FromLong((long) AL_INPUT_HRB_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "INPUT_HRB_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_INPUT_LINE
    x =  PyInt_FromLong((long) AL_INPUT_LINE);
    if (x == NULL || PyDict_SetItemString(d, "INPUT_LINE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_INPUT_MIC
    x =  PyInt_FromLong((long) AL_INPUT_MIC);
    if (x == NULL || PyDict_SetItemString(d, "INPUT_MIC", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_INPUT_PORT_TYPE
    x =  PyInt_FromLong((long) AL_INPUT_PORT_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "INPUT_PORT_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_INPUT_RATE
    x =  PyInt_FromLong((long) AL_INPUT_RATE);
    if (x == NULL || PyDict_SetItemString(d, "INPUT_RATE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_INPUT_SOURCE
    x =  PyInt_FromLong((long) AL_INPUT_SOURCE);
    if (x == NULL || PyDict_SetItemString(d, "INPUT_SOURCE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_INT32_ELEM
    x =  PyInt_FromLong((long) AL_INT32_ELEM);
    if (x == NULL || PyDict_SetItemString(d, "INT32_ELEM", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_INT64_ELEM
    x =  PyInt_FromLong((long) AL_INT64_ELEM);
    if (x == NULL || PyDict_SetItemString(d, "INT64_ELEM", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_INTERFACE
    x =  PyInt_FromLong((long) AL_INTERFACE);
    if (x == NULL || PyDict_SetItemString(d, "INTERFACE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_INTERFACE_TYPE
    x =  PyInt_FromLong((long) AL_INTERFACE_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "INTERFACE_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_INVALID_PARAM
    x =  PyInt_FromLong((long) AL_INVALID_PARAM);
    if (x == NULL || PyDict_SetItemString(d, "INVALID_PARAM", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_INVALID_VALUE
    x =  PyInt_FromLong((long) AL_INVALID_VALUE);
    if (x == NULL || PyDict_SetItemString(d, "INVALID_VALUE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_JITTER
    x =  PyInt_FromLong((long) AL_JITTER);
    if (x == NULL || PyDict_SetItemString(d, "JITTER", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_LABEL
    x =  PyInt_FromLong((long) AL_LABEL);
    if (x == NULL || PyDict_SetItemString(d, "LABEL", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_LEFT_INPUT_ATTEN
    x =  PyInt_FromLong((long) AL_LEFT_INPUT_ATTEN);
    if (x == NULL || PyDict_SetItemString(d, "LEFT_INPUT_ATTEN", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_LEFT_MONITOR_ATTEN
    x =  PyInt_FromLong((long) AL_LEFT_MONITOR_ATTEN);
    if (x == NULL || PyDict_SetItemString(d, "LEFT_MONITOR_ATTEN", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_LEFT_SPEAKER_GAIN
    x =  PyInt_FromLong((long) AL_LEFT_SPEAKER_GAIN);
    if (x == NULL || PyDict_SetItemString(d, "LEFT_SPEAKER_GAIN", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_LEFT1_INPUT_ATTEN
    x =  PyInt_FromLong((long) AL_LEFT1_INPUT_ATTEN);
    if (x == NULL || PyDict_SetItemString(d, "LEFT1_INPUT_ATTEN", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_LEFT2_INPUT_ATTEN
    x =  PyInt_FromLong((long) AL_LEFT2_INPUT_ATTEN);
    if (x == NULL || PyDict_SetItemString(d, "LEFT2_INPUT_ATTEN", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_LINE_IF_TYPE
    x =  PyInt_FromLong((long) AL_LINE_IF_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "LINE_IF_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_LOCKED
    x =  PyInt_FromLong((long) AL_LOCKED);
    if (x == NULL || PyDict_SetItemString(d, "LOCKED", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_MASTER_CLOCK
    x =  PyInt_FromLong((long) AL_MASTER_CLOCK);
    if (x == NULL || PyDict_SetItemString(d, "MASTER_CLOCK", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_MATRIX_VAL
    x =  PyInt_FromLong((long) AL_MATRIX_VAL);
    if (x == NULL || PyDict_SetItemString(d, "MATRIX_VAL", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_MAX_ERROR
    x =  PyInt_FromLong((long) AL_MAX_ERROR);
    if (x == NULL || PyDict_SetItemString(d, "MAX_ERROR", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_MAX_EVENT_PARAM
    x =  PyInt_FromLong((long) AL_MAX_EVENT_PARAM);
    if (x == NULL || PyDict_SetItemString(d, "MAX_EVENT_PARAM", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_MAX_PBUFSIZE
    x =  PyInt_FromLong((long) AL_MAX_PBUFSIZE);
    if (x == NULL || PyDict_SetItemString(d, "MAX_PBUFSIZE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_MAX_PORTS
    x =  PyInt_FromLong((long) AL_MAX_PORTS);
    if (x == NULL || PyDict_SetItemString(d, "MAX_PORTS", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_MAX_RESOURCE_ID
    x =  PyInt_FromLong((long) AL_MAX_RESOURCE_ID);
    if (x == NULL || PyDict_SetItemString(d, "MAX_RESOURCE_ID", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_MAX_SETSIZE
    x =  PyInt_FromLong((long) AL_MAX_SETSIZE);
    if (x == NULL || PyDict_SetItemString(d, "MAX_SETSIZE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_MAX_STRLEN
    x =  PyInt_FromLong((long) AL_MAX_STRLEN);
    if (x == NULL || PyDict_SetItemString(d, "MAX_STRLEN", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_MCLK_TYPE
    x =  PyInt_FromLong((long) AL_MCLK_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "MCLK_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_MIC_IF_TYPE
    x =  PyInt_FromLong((long) AL_MIC_IF_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "MIC_IF_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_MONITOR_CTL
    x =  PyInt_FromLong((long) AL_MONITOR_CTL);
    if (x == NULL || PyDict_SetItemString(d, "MONITOR_CTL", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_MONITOR_OFF
    x =  PyInt_FromLong((long) AL_MONITOR_OFF);
    if (x == NULL || PyDict_SetItemString(d, "MONITOR_OFF", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_MONITOR_ON
    x =  PyInt_FromLong((long) AL_MONITOR_ON);
    if (x == NULL || PyDict_SetItemString(d, "MONITOR_ON", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_MONO
    x =  PyInt_FromLong((long) AL_MONO);
    if (x == NULL || PyDict_SetItemString(d, "MONO", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_MUTE
    x =  PyInt_FromLong((long) AL_MUTE);
    if (x == NULL || PyDict_SetItemString(d, "MUTE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_NAME
    x =  PyInt_FromLong((long) AL_NAME);
    if (x == NULL || PyDict_SetItemString(d, "NAME", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_NEG_INFINITY
    x =  PyInt_FromLong((long) AL_NEG_INFINITY);
    if (x == NULL || PyDict_SetItemString(d, "NEG_INFINITY", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_NEG_INFINITY_BIT
    x =  PyInt_FromLong((long) AL_NEG_INFINITY_BIT);
    if (x == NULL || PyDict_SetItemString(d, "NEG_INFINITY_BIT", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_NO_CHANGE
    x =  PyInt_FromLong((long) AL_NO_CHANGE);
    if (x == NULL || PyDict_SetItemString(d, "NO_CHANGE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_NO_CHANGE_BIT
    x =  PyInt_FromLong((long) AL_NO_CHANGE_BIT);
    if (x == NULL || PyDict_SetItemString(d, "NO_CHANGE_BIT", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_NO_ELEM
    x =  PyInt_FromLong((long) AL_NO_ELEM);
    if (x == NULL || PyDict_SetItemString(d, "NO_ELEM", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_NO_ERRORS
    x =  PyInt_FromLong((long) AL_NO_ERRORS);
    if (x == NULL || PyDict_SetItemString(d, "NO_ERRORS", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_NO_OP
    x =  PyInt_FromLong((long) AL_NO_OP);
    if (x == NULL || PyDict_SetItemString(d, "NO_OP", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_NO_VAL
    x =  PyInt_FromLong((long) AL_NO_VAL);
    if (x == NULL || PyDict_SetItemString(d, "NO_VAL", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_NULL_INTERFACE
    x =  PyInt_FromLong((long) AL_NULL_INTERFACE);
    if (x == NULL || PyDict_SetItemString(d, "NULL_INTERFACE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_NULL_RESOURCE
    x =  PyInt_FromLong((long) AL_NULL_RESOURCE);
    if (x == NULL || PyDict_SetItemString(d, "NULL_RESOURCE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_OPTICAL_IF_TYPE
    x =  PyInt_FromLong((long) AL_OPTICAL_IF_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "OPTICAL_IF_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_OUTPUT_COUNT
    x =  PyInt_FromLong((long) AL_OUTPUT_COUNT);
    if (x == NULL || PyDict_SetItemString(d, "OUTPUT_COUNT", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_OUTPUT_DEVICE_TYPE
    x =  PyInt_FromLong((long) AL_OUTPUT_DEVICE_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "OUTPUT_DEVICE_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_OUTPUT_HRB_TYPE
    x =  PyInt_FromLong((long) AL_OUTPUT_HRB_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "OUTPUT_HRB_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_OUTPUT_PORT_TYPE
    x =  PyInt_FromLong((long) AL_OUTPUT_PORT_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "OUTPUT_PORT_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_OUTPUT_RATE
    x =  PyInt_FromLong((long) AL_OUTPUT_RATE);
    if (x == NULL || PyDict_SetItemString(d, "OUTPUT_RATE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_PARAM_BIT
    x =  PyInt_FromLong((long) AL_PARAM_BIT);
    if (x == NULL || PyDict_SetItemString(d, "PARAM_BIT", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_PARAMS
    x =  PyInt_FromLong((long) AL_PARAMS);
    if (x == NULL || PyDict_SetItemString(d, "PARAMS", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_PORT_COUNT
    x =  PyInt_FromLong((long) AL_PORT_COUNT);
    if (x == NULL || PyDict_SetItemString(d, "PORT_COUNT", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_PORT_TYPE
    x =  PyInt_FromLong((long) AL_PORT_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "PORT_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_PORTS
    x =  PyInt_FromLong((long) AL_PORTS);
    if (x == NULL || PyDict_SetItemString(d, "PORTS", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_PORTSTYLE_DIRECT
    x =  PyInt_FromLong((long) AL_PORTSTYLE_DIRECT);
    if (x == NULL || PyDict_SetItemString(d, "PORTSTYLE_DIRECT", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_PORTSTYLE_SERIAL
    x =  PyInt_FromLong((long) AL_PORTSTYLE_SERIAL);
    if (x == NULL || PyDict_SetItemString(d, "PORTSTYLE_SERIAL", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_PRINT_ERRORS
    x =  PyInt_FromLong((long) AL_PRINT_ERRORS);
    if (x == NULL || PyDict_SetItemString(d, "PRINT_ERRORS", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_PTR_ELEM
    x =  PyInt_FromLong((long) AL_PTR_ELEM);
    if (x == NULL || PyDict_SetItemString(d, "PTR_ELEM", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RANGE_VALUE
    x =  PyInt_FromLong((long) AL_RANGE_VALUE);
    if (x == NULL || PyDict_SetItemString(d, "RANGE_VALUE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RATE
    x =  PyInt_FromLong((long) AL_RATE);
    if (x == NULL || PyDict_SetItemString(d, "RATE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RATE_11025
    x =  PyInt_FromLong((long) AL_RATE_11025);
    if (x == NULL || PyDict_SetItemString(d, "RATE_11025", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RATE_16000
    x =  PyInt_FromLong((long) AL_RATE_16000);
    if (x == NULL || PyDict_SetItemString(d, "RATE_16000", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RATE_22050
    x =  PyInt_FromLong((long) AL_RATE_22050);
    if (x == NULL || PyDict_SetItemString(d, "RATE_22050", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RATE_32000
    x =  PyInt_FromLong((long) AL_RATE_32000);
    if (x == NULL || PyDict_SetItemString(d, "RATE_32000", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RATE_44100
    x =  PyInt_FromLong((long) AL_RATE_44100);
    if (x == NULL || PyDict_SetItemString(d, "RATE_44100", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RATE_48000
    x =  PyInt_FromLong((long) AL_RATE_48000);
    if (x == NULL || PyDict_SetItemString(d, "RATE_48000", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RATE_8000
    x =  PyInt_FromLong((long) AL_RATE_8000);
    if (x == NULL || PyDict_SetItemString(d, "RATE_8000", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RATE_AES_1
    x =  PyInt_FromLong((long) AL_RATE_AES_1);
    if (x == NULL || PyDict_SetItemString(d, "RATE_AES_1", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RATE_AES_1s
    x =  PyInt_FromLong((long) AL_RATE_AES_1s);
    if (x == NULL || PyDict_SetItemString(d, "RATE_AES_1s", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RATE_AES_2
    x =  PyInt_FromLong((long) AL_RATE_AES_2);
    if (x == NULL || PyDict_SetItemString(d, "RATE_AES_2", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RATE_AES_3
    x =  PyInt_FromLong((long) AL_RATE_AES_3);
    if (x == NULL || PyDict_SetItemString(d, "RATE_AES_3", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RATE_AES_4
    x =  PyInt_FromLong((long) AL_RATE_AES_4);
    if (x == NULL || PyDict_SetItemString(d, "RATE_AES_4", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RATE_AES_6
    x =  PyInt_FromLong((long) AL_RATE_AES_6);
    if (x == NULL || PyDict_SetItemString(d, "RATE_AES_6", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RATE_FRACTION_D
    x =  PyInt_FromLong((long) AL_RATE_FRACTION_D);
    if (x == NULL || PyDict_SetItemString(d, "RATE_FRACTION_D", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RATE_FRACTION_N
    x =  PyInt_FromLong((long) AL_RATE_FRACTION_N);
    if (x == NULL || PyDict_SetItemString(d, "RATE_FRACTION_N", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RATE_INPUTRATE
    x =  PyInt_FromLong((long) AL_RATE_INPUTRATE);
    if (x == NULL || PyDict_SetItemString(d, "RATE_INPUTRATE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RATE_NO_DIGITAL_INPUT
    x =  PyInt_FromLong((long) AL_RATE_NO_DIGITAL_INPUT);
    if (x == NULL || PyDict_SetItemString(d, "RATE_NO_DIGITAL_INPUT", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RATE_UNACQUIRED
    x =  PyInt_FromLong((long) AL_RATE_UNACQUIRED);
    if (x == NULL || PyDict_SetItemString(d, "RATE_UNACQUIRED", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RATE_UNDEFINED
    x =  PyInt_FromLong((long) AL_RATE_UNDEFINED);
    if (x == NULL || PyDict_SetItemString(d, "RATE_UNDEFINED", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_REF_0DBV
    x =  PyInt_FromLong((long) AL_REF_0DBV);
    if (x == NULL || PyDict_SetItemString(d, "REF_0DBV", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_REF_NONE
    x =  PyInt_FromLong((long) AL_REF_NONE);
    if (x == NULL || PyDict_SetItemString(d, "REF_NONE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RESERVED1_TYPE
    x =  PyInt_FromLong((long) AL_RESERVED1_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "RESERVED1_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RESERVED2_TYPE
    x =  PyInt_FromLong((long) AL_RESERVED2_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "RESERVED2_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RESERVED3_TYPE
    x =  PyInt_FromLong((long) AL_RESERVED3_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "RESERVED3_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RESERVED4_TYPE
    x =  PyInt_FromLong((long) AL_RESERVED4_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "RESERVED4_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RESOURCE
    x =  PyInt_FromLong((long) AL_RESOURCE);
    if (x == NULL || PyDict_SetItemString(d, "RESOURCE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RESOURCE_ELEM
    x =  PyInt_FromLong((long) AL_RESOURCE_ELEM);
    if (x == NULL || PyDict_SetItemString(d, "RESOURCE_ELEM", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RESOURCE_TYPE
    x =  PyInt_FromLong((long) AL_RESOURCE_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "RESOURCE_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RIGHT_INPUT_ATTEN
    x =  PyInt_FromLong((long) AL_RIGHT_INPUT_ATTEN);
    if (x == NULL || PyDict_SetItemString(d, "RIGHT_INPUT_ATTEN", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RIGHT_MONITOR_ATTEN
    x =  PyInt_FromLong((long) AL_RIGHT_MONITOR_ATTEN);
    if (x == NULL || PyDict_SetItemString(d, "RIGHT_MONITOR_ATTEN", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RIGHT_SPEAKER_GAIN
    x =  PyInt_FromLong((long) AL_RIGHT_SPEAKER_GAIN);
    if (x == NULL || PyDict_SetItemString(d, "RIGHT_SPEAKER_GAIN", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RIGHT1_INPUT_ATTEN
    x =  PyInt_FromLong((long) AL_RIGHT1_INPUT_ATTEN);
    if (x == NULL || PyDict_SetItemString(d, "RIGHT1_INPUT_ATTEN", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_RIGHT2_INPUT_ATTEN
    x =  PyInt_FromLong((long) AL_RIGHT2_INPUT_ATTEN);
    if (x == NULL || PyDict_SetItemString(d, "RIGHT2_INPUT_ATTEN", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_SAMPFMT_DOUBLE
    x =  PyInt_FromLong((long) AL_SAMPFMT_DOUBLE);
    if (x == NULL || PyDict_SetItemString(d, "SAMPFMT_DOUBLE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_SAMPFMT_FLOAT
    x =  PyInt_FromLong((long) AL_SAMPFMT_FLOAT);
    if (x == NULL || PyDict_SetItemString(d, "SAMPFMT_FLOAT", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_SAMPFMT_TWOSCOMP
    x =  PyInt_FromLong((long) AL_SAMPFMT_TWOSCOMP);
    if (x == NULL || PyDict_SetItemString(d, "SAMPFMT_TWOSCOMP", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_SAMPLE_16
    x =  PyInt_FromLong((long) AL_SAMPLE_16);
    if (x == NULL || PyDict_SetItemString(d, "SAMPLE_16", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_SAMPLE_24
    x =  PyInt_FromLong((long) AL_SAMPLE_24);
    if (x == NULL || PyDict_SetItemString(d, "SAMPLE_24", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_SAMPLE_8
    x =  PyInt_FromLong((long) AL_SAMPLE_8);
    if (x == NULL || PyDict_SetItemString(d, "SAMPLE_8", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_SCALAR_VAL
    x =  PyInt_FromLong((long) AL_SCALAR_VAL);
    if (x == NULL || PyDict_SetItemString(d, "SCALAR_VAL", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_SET_VAL
    x =  PyInt_FromLong((long) AL_SET_VAL);
    if (x == NULL || PyDict_SetItemString(d, "SET_VAL", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_SHORT_NAME
    x =  PyInt_FromLong((long) AL_SHORT_NAME);
    if (x == NULL || PyDict_SetItemString(d, "SHORT_NAME", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_SMPTE272M_IF_TYPE
    x =  PyInt_FromLong((long) AL_SMPTE272M_IF_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "SMPTE272M_IF_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_SOURCE
    x =  PyInt_FromLong((long) AL_SOURCE);
    if (x == NULL || PyDict_SetItemString(d, "SOURCE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_SPEAKER_IF_TYPE
    x =  PyInt_FromLong((long) AL_SPEAKER_IF_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "SPEAKER_IF_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_SPEAKER_MUTE_CTL
    x =  PyInt_FromLong((long) AL_SPEAKER_MUTE_CTL);
    if (x == NULL || PyDict_SetItemString(d, "SPEAKER_MUTE_CTL", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_SPEAKER_MUTE_OFF
    x =  PyInt_FromLong((long) AL_SPEAKER_MUTE_OFF);
    if (x == NULL || PyDict_SetItemString(d, "SPEAKER_MUTE_OFF", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_SPEAKER_MUTE_ON
    x =  PyInt_FromLong((long) AL_SPEAKER_MUTE_ON);
    if (x == NULL || PyDict_SetItemString(d, "SPEAKER_MUTE_ON", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_SPEAKER_PLUS_LINE_IF_TYPE
    x =  PyInt_FromLong((long) AL_SPEAKER_PLUS_LINE_IF_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "SPEAKER_PLUS_LINE_IF_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_STEREO
    x =  PyInt_FromLong((long) AL_STEREO);
    if (x == NULL || PyDict_SetItemString(d, "STEREO", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_STRING_VAL
    x =  PyInt_FromLong((long) AL_STRING_VAL);
    if (x == NULL || PyDict_SetItemString(d, "STRING_VAL", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_SUBSYSTEM
    x =  PyInt_FromLong((long) AL_SUBSYSTEM);
    if (x == NULL || PyDict_SetItemString(d, "SUBSYSTEM", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_SUBSYSTEM_TYPE
    x =  PyInt_FromLong((long) AL_SUBSYSTEM_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "SUBSYSTEM_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_SYNC_INPUT_TO_AES
    x =  PyInt_FromLong((long) AL_SYNC_INPUT_TO_AES);
    if (x == NULL || PyDict_SetItemString(d, "SYNC_INPUT_TO_AES", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_SYNC_OUTPUT_TO_AES
    x =  PyInt_FromLong((long) AL_SYNC_OUTPUT_TO_AES);
    if (x == NULL || PyDict_SetItemString(d, "SYNC_OUTPUT_TO_AES", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_SYSTEM
    x =  PyInt_FromLong((long) AL_SYSTEM);
    if (x == NULL || PyDict_SetItemString(d, "SYSTEM", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_SYSTEM_TYPE
    x =  PyInt_FromLong((long) AL_SYSTEM_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "SYSTEM_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_TEST_IF_TYPE
    x =  PyInt_FromLong((long) AL_TEST_IF_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "TEST_IF_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_TYPE
    x =  PyInt_FromLong((long) AL_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_TYPE_BIT
    x =  PyInt_FromLong((long) AL_TYPE_BIT);
    if (x == NULL || PyDict_SetItemString(d, "TYPE_BIT", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_UNUSED_COUNT
    x =  PyInt_FromLong((long) AL_UNUSED_COUNT);
    if (x == NULL || PyDict_SetItemString(d, "UNUSED_COUNT", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_UNUSED_PORTS
    x =  PyInt_FromLong((long) AL_UNUSED_PORTS);
    if (x == NULL || PyDict_SetItemString(d, "UNUSED_PORTS", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_VARIABLE_MCLK_TYPE
    x =  PyInt_FromLong((long) AL_VARIABLE_MCLK_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "VARIABLE_MCLK_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_VECTOR_VAL
    x =  PyInt_FromLong((long) AL_VECTOR_VAL);
    if (x == NULL || PyDict_SetItemString(d, "VECTOR_VAL", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_VIDEO_MCLK_TYPE
    x =  PyInt_FromLong((long) AL_VIDEO_MCLK_TYPE);
    if (x == NULL || PyDict_SetItemString(d, "VIDEO_MCLK_TYPE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif
#ifdef AL_WORDSIZE
    x =  PyInt_FromLong((long) AL_WORDSIZE);
    if (x == NULL || PyDict_SetItemString(d, "WORDSIZE", x) < 0)
        goto error;
    Py_DECREF(x);
#endif

#ifdef AL_NO_ELEM               /* IRIX 6 */
    (void) alSetErrorHandler(ErrorHandler);
#endif /* AL_NO_ELEM */
#ifdef OLD_INTERFACE
    (void) ALseterrorhandler(ErrorHandler);
#endif /* OLD_INTERFACE */

  error:
    return;
}
