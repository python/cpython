/* CD module -- interface to Mark Callow's and Roger Chickering's */
 /* CD Audio Library (CD). */

#include <sys/types.h>
#include <cdaudio.h>
#include "Python.h"

#define NCALLBACKS      8

typedef struct {
    PyObject_HEAD
    CDPLAYER *ob_cdplayer;
} cdplayerobject;

static PyObject *CdError;               /* exception cd.error */

static PyObject *
CD_allowremoval(cdplayerobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":allowremoval"))
        return NULL;

    CDallowremoval(self->ob_cdplayer);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
CD_preventremoval(cdplayerobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":preventremoval"))
        return NULL;

    CDpreventremoval(self->ob_cdplayer);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
CD_bestreadsize(cdplayerobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":bestreadsize"))
        return NULL;

    return PyInt_FromLong((long) CDbestreadsize(self->ob_cdplayer));
}

static PyObject *
CD_close(cdplayerobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":close"))
        return NULL;

    if (!CDclose(self->ob_cdplayer)) {
        PyErr_SetFromErrno(CdError); /* XXX - ??? */
        return NULL;
    }
    self->ob_cdplayer = NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
CD_eject(cdplayerobject *self, PyObject *args)
{
    CDSTATUS status;

    if (!PyArg_ParseTuple(args, ":eject"))
        return NULL;

    if (!CDeject(self->ob_cdplayer)) {
        if (CDgetstatus(self->ob_cdplayer, &status) &&
            status.state == CD_NODISC)
            PyErr_SetString(CdError, "no disc in player");
        else
            PyErr_SetString(CdError, "eject failed");
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
CD_getstatus(cdplayerobject *self, PyObject *args)
{
    CDSTATUS status;

    if (!PyArg_ParseTuple(args, ":getstatus"))
        return NULL;

    if (!CDgetstatus(self->ob_cdplayer, &status)) {
        PyErr_SetFromErrno(CdError); /* XXX - ??? */
        return NULL;
    }

    return Py_BuildValue("(ii(iii)(iii)(iii)iiii)", status.state,
                   status.track, status.min, status.sec, status.frame,
                   status.abs_min, status.abs_sec, status.abs_frame,
                   status.total_min, status.total_sec, status.total_frame,
                   status.first, status.last, status.scsi_audio,
                   status.cur_block);
}

static PyObject *
CD_gettrackinfo(cdplayerobject *self, PyObject *args)
{
    int track;
    CDTRACKINFO info;
    CDSTATUS status;

    if (!PyArg_ParseTuple(args, "i:gettrackinfo", &track))
        return NULL;

    if (!CDgettrackinfo(self->ob_cdplayer, track, &info)) {
        if (CDgetstatus(self->ob_cdplayer, &status) &&
            status.state == CD_NODISC)
            PyErr_SetString(CdError, "no disc in player");
        else
            PyErr_SetString(CdError, "gettrackinfo failed");
        return NULL;
    }

    return Py_BuildValue("((iii)(iii))",
                   info.start_min, info.start_sec, info.start_frame,
                   info.total_min, info.total_sec, info.total_frame);
}

static PyObject *
CD_msftoblock(cdplayerobject *self, PyObject *args)
{
    int min, sec, frame;

    if (!PyArg_ParseTuple(args, "iii:msftoblock", &min, &sec, &frame))
        return NULL;

    return PyInt_FromLong((long) CDmsftoblock(self->ob_cdplayer,
                                            min, sec, frame));
}

static PyObject *
CD_play(cdplayerobject *self, PyObject *args)
{
    int start, play;
    CDSTATUS status;

    if (!PyArg_ParseTuple(args, "ii:play", &start, &play))
        return NULL;

    if (!CDplay(self->ob_cdplayer, start, play)) {
        if (CDgetstatus(self->ob_cdplayer, &status) &&
            status.state == CD_NODISC)
            PyErr_SetString(CdError, "no disc in player");
        else
            PyErr_SetString(CdError, "play failed");
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
CD_playabs(cdplayerobject *self, PyObject *args)
{
    int min, sec, frame, play;
    CDSTATUS status;

    if (!PyArg_ParseTuple(args, "iiii:playabs", &min, &sec, &frame, &play))
        return NULL;

    if (!CDplayabs(self->ob_cdplayer, min, sec, frame, play)) {
        if (CDgetstatus(self->ob_cdplayer, &status) &&
            status.state == CD_NODISC)
            PyErr_SetString(CdError, "no disc in player");
        else
            PyErr_SetString(CdError, "playabs failed");
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
CD_playtrack(cdplayerobject *self, PyObject *args)
{
    int start, play;
    CDSTATUS status;

    if (!PyArg_ParseTuple(args, "ii:playtrack", &start, &play))
        return NULL;

    if (!CDplaytrack(self->ob_cdplayer, start, play)) {
        if (CDgetstatus(self->ob_cdplayer, &status) &&
            status.state == CD_NODISC)
            PyErr_SetString(CdError, "no disc in player");
        else
            PyErr_SetString(CdError, "playtrack failed");
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
CD_playtrackabs(cdplayerobject *self, PyObject *args)
{
    int track, min, sec, frame, play;
    CDSTATUS status;

    if (!PyArg_ParseTuple(args, "iiiii:playtrackabs", &track, &min, &sec,
                          &frame, &play))
        return NULL;

    if (!CDplaytrackabs(self->ob_cdplayer, track, min, sec, frame, play)) {
        if (CDgetstatus(self->ob_cdplayer, &status) &&
            status.state == CD_NODISC)
            PyErr_SetString(CdError, "no disc in player");
        else
            PyErr_SetString(CdError, "playtrackabs failed");
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
CD_readda(cdplayerobject *self, PyObject *args)
{
    int numframes, n;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "i:readda", &numframes))
        return NULL;

    result = PyString_FromStringAndSize(NULL, numframes * sizeof(CDFRAME));
    if (result == NULL)
        return NULL;

    n = CDreadda(self->ob_cdplayer,
                   (CDFRAME *) PyString_AsString(result), numframes);
    if (n == -1) {
        Py_DECREF(result);
        PyErr_SetFromErrno(CdError);
        return NULL;
    }
    if (n < numframes)
        _PyString_Resize(&result, n * sizeof(CDFRAME));

    return result;
}

static PyObject *
CD_seek(cdplayerobject *self, PyObject *args)
{
    int min, sec, frame;
    long PyTryBlock;

    if (!PyArg_ParseTuple(args, "iii:seek", &min, &sec, &frame))
        return NULL;

    PyTryBlock = CDseek(self->ob_cdplayer, min, sec, frame);
    if (PyTryBlock == -1) {
        PyErr_SetFromErrno(CdError);
        return NULL;
    }

    return PyInt_FromLong(PyTryBlock);
}

static PyObject *
CD_seektrack(cdplayerobject *self, PyObject *args)
{
    int track;
    long PyTryBlock;

    if (!PyArg_ParseTuple(args, "i:seektrack", &track))
        return NULL;

    PyTryBlock = CDseektrack(self->ob_cdplayer, track);
    if (PyTryBlock == -1) {
        PyErr_SetFromErrno(CdError);
        return NULL;
    }

    return PyInt_FromLong(PyTryBlock);
}

static PyObject *
CD_seekblock(cdplayerobject *self, PyObject *args)
{
    unsigned long PyTryBlock;

    if (!PyArg_ParseTuple(args, "l:seekblock", &PyTryBlock))
        return NULL;

    PyTryBlock = CDseekblock(self->ob_cdplayer, PyTryBlock);
    if (PyTryBlock == (unsigned long) -1) {
        PyErr_SetFromErrno(CdError);
        return NULL;
    }

    return PyInt_FromLong(PyTryBlock);
}

static PyObject *
CD_stop(cdplayerobject *self, PyObject *args)
{
    CDSTATUS status;

    if (!PyArg_ParseTuple(args, ":stop"))
        return NULL;

    if (!CDstop(self->ob_cdplayer)) {
        if (CDgetstatus(self->ob_cdplayer, &status) &&
            status.state == CD_NODISC)
            PyErr_SetString(CdError, "no disc in player");
        else
            PyErr_SetString(CdError, "stop failed");
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
CD_togglepause(cdplayerobject *self, PyObject *args)
{
    CDSTATUS status;

    if (!PyArg_ParseTuple(args, ":togglepause"))
        return NULL;

    if (!CDtogglepause(self->ob_cdplayer)) {
        if (CDgetstatus(self->ob_cdplayer, &status) &&
            status.state == CD_NODISC)
            PyErr_SetString(CdError, "no disc in player");
        else
            PyErr_SetString(CdError, "togglepause failed");
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef cdplayer_methods[] = {
    {"allowremoval",            (PyCFunction)CD_allowremoval,   METH_VARARGS},
    {"bestreadsize",            (PyCFunction)CD_bestreadsize,   METH_VARARGS},
    {"close",                   (PyCFunction)CD_close,          METH_VARARGS},
    {"eject",                   (PyCFunction)CD_eject,          METH_VARARGS},
    {"getstatus",               (PyCFunction)CD_getstatus,              METH_VARARGS},
    {"gettrackinfo",            (PyCFunction)CD_gettrackinfo,   METH_VARARGS},
    {"msftoblock",              (PyCFunction)CD_msftoblock,             METH_VARARGS},
    {"play",                    (PyCFunction)CD_play,           METH_VARARGS},
    {"playabs",                 (PyCFunction)CD_playabs,                METH_VARARGS},
    {"playtrack",               (PyCFunction)CD_playtrack,              METH_VARARGS},
    {"playtrackabs",            (PyCFunction)CD_playtrackabs,   METH_VARARGS},
    {"preventremoval",          (PyCFunction)CD_preventremoval, METH_VARARGS},
    {"readda",                  (PyCFunction)CD_readda,         METH_VARARGS},
    {"seek",                    (PyCFunction)CD_seek,           METH_VARARGS},
    {"seekblock",               (PyCFunction)CD_seekblock,              METH_VARARGS},
    {"seektrack",               (PyCFunction)CD_seektrack,              METH_VARARGS},
    {"stop",                    (PyCFunction)CD_stop,           METH_VARARGS},
    {"togglepause",             (PyCFunction)CD_togglepause,    METH_VARARGS},
    {NULL,                      NULL}           /* sentinel */
};

static void
cdplayer_dealloc(cdplayerobject *self)
{
    if (self->ob_cdplayer != NULL)
        CDclose(self->ob_cdplayer);
    PyObject_Del(self);
}

static PyObject *
cdplayer_getattr(cdplayerobject *self, char *name)
{
    if (self->ob_cdplayer == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "no player active");
        return NULL;
    }
    return Py_FindMethod(cdplayer_methods, (PyObject *)self, name);
}

PyTypeObject CdPlayertype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                          /*ob_size*/
    "cd.cdplayer",      /*tp_name*/
    sizeof(cdplayerobject),     /*tp_size*/
    0,                          /*tp_itemsize*/
    /* methods */
    (destructor)cdplayer_dealloc, /*tp_dealloc*/
    0,                          /*tp_print*/
    (getattrfunc)cdplayer_getattr, /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_compare*/
    0,                          /*tp_repr*/
};

static PyObject *
newcdplayerobject(CDPLAYER *cdp)
{
    cdplayerobject *p;

    p = PyObject_New(cdplayerobject, &CdPlayertype);
    if (p == NULL)
        return NULL;
    p->ob_cdplayer = cdp;
    return (PyObject *) p;
}

static PyObject *
CD_open(PyObject *self, PyObject *args)
{
    char *dev, *direction;
    CDPLAYER *cdp;

    /*
     * Variable number of args.
     * First defaults to "None", second defaults to "r".
     */
    dev = NULL;
    direction = "r";
    if (!PyArg_ParseTuple(args, "|zs:open", &dev, &direction))
        return NULL;

    cdp = CDopen(dev, direction);
    if (cdp == NULL) {
        PyErr_SetFromErrno(CdError);
        return NULL;
    }

    return newcdplayerobject(cdp);
}

typedef struct {
    PyObject_HEAD
    CDPARSER *ob_cdparser;
    struct {
        PyObject *ob_cdcallback;
        PyObject *ob_cdcallbackarg;
    } ob_cdcallbacks[NCALLBACKS];
} cdparserobject;

static void
CD_callback(void *arg, CDDATATYPES type, void *data)
{
    PyObject *result, *args, *v = NULL;
    char *p;
    int i;
    cdparserobject *self;

    self = (cdparserobject *) arg;
    args = PyTuple_New(3);
    if (args == NULL)
        return;
    Py_INCREF(self->ob_cdcallbacks[type].ob_cdcallbackarg);
    PyTuple_SetItem(args, 0, self->ob_cdcallbacks[type].ob_cdcallbackarg);
    PyTuple_SetItem(args, 1, PyInt_FromLong((long) type));
    switch (type) {
    case cd_audio:
        v = PyString_FromStringAndSize(data, CDDA_DATASIZE);
        break;
    case cd_pnum:
    case cd_index:
        v = PyInt_FromLong(((CDPROGNUM *) data)->value);
        break;
    case cd_ptime:
    case cd_atime:
#define ptr ((struct cdtimecode *) data)
        v = Py_BuildValue("(iii)",
                    ptr->mhi * 10 + ptr->mlo,
                    ptr->shi * 10 + ptr->slo,
                    ptr->fhi * 10 + ptr->flo);
#undef ptr
        break;
    case cd_catalog:
        v = PyString_FromStringAndSize(NULL, 13);
        p = PyString_AsString(v);
        for (i = 0; i < 13; i++)
            *p++ = ((char *) data)[i] + '0';
        break;
    case cd_ident:
#define ptr ((struct cdident *) data)
        v = PyString_FromStringAndSize(NULL, 12);
        p = PyString_AsString(v);
        CDsbtoa(p, ptr->country, 2);
        p += 2;
        CDsbtoa(p, ptr->owner, 3);
        p += 3;
        *p++ = ptr->year[0] + '0';
        *p++ = ptr->year[1] + '0';
        *p++ = ptr->serial[0] + '0';
        *p++ = ptr->serial[1] + '0';
        *p++ = ptr->serial[2] + '0';
        *p++ = ptr->serial[3] + '0';
        *p++ = ptr->serial[4] + '0';
#undef ptr
        break;
    case cd_control:
        v = PyInt_FromLong((long) *((unchar *) data));
        break;
    }
    PyTuple_SetItem(args, 2, v);
    if (PyErr_Occurred()) {
        Py_DECREF(args);
        return;
    }

    result = PyEval_CallObject(self->ob_cdcallbacks[type].ob_cdcallback,
                               args);
    Py_DECREF(args);
    Py_XDECREF(result);
}

static PyObject *
CD_deleteparser(cdparserobject *self, PyObject *args)
{
    int i;

    if (!PyArg_ParseTuple(args, ":deleteparser"))
        return NULL;

    CDdeleteparser(self->ob_cdparser);
    self->ob_cdparser = NULL;

    /* no sense in keeping the callbacks, so remove them */
    for (i = 0; i < NCALLBACKS; i++) {
        Py_CLEAR(self->ob_cdcallbacks[i].ob_cdcallback);
        Py_CLEAR(self->ob_cdcallbacks[i].ob_cdcallbackarg);
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
CD_parseframe(cdparserobject *self, PyObject *args)
{
    char *cdfp;
    int length;
    CDFRAME *p;

    if (!PyArg_ParseTuple(args, "s#:parseframe", &cdfp, &length))
        return NULL;

    if (length % sizeof(CDFRAME) != 0) {
        PyErr_SetString(PyExc_TypeError, "bad length");
        return NULL;
    }

    p = (CDFRAME *) cdfp;
    while (length > 0) {
        CDparseframe(self->ob_cdparser, p);
        length -= sizeof(CDFRAME);
        p++;
        if (PyErr_Occurred())
            return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
CD_removecallback(cdparserobject *self, PyObject *args)
{
    int type;

    if (!PyArg_ParseTuple(args, "i:removecallback", &type))
        return NULL;

    if (type < 0 || type >= NCALLBACKS) {
        PyErr_SetString(PyExc_TypeError, "bad type");
        return NULL;
    }

    CDremovecallback(self->ob_cdparser, (CDDATATYPES) type);

    Py_CLEAR(self->ob_cdcallbacks[type].ob_cdcallback);

    Py_CLEAR(self->ob_cdcallbacks[type].ob_cdcallbackarg);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
CD_resetparser(cdparserobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":resetparser"))
        return NULL;

    CDresetparser(self->ob_cdparser);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
CD_addcallback(cdparserobject *self, PyObject *args)
{
    int type;
    PyObject *func, *funcarg;

    /* XXX - more work here */
    if (!PyArg_ParseTuple(args, "iOO:addcallback", &type, &func, &funcarg))
        return NULL;

    if (type < 0 || type >= NCALLBACKS) {
        PyErr_SetString(PyExc_TypeError, "argument out of range");
        return NULL;
    }

#ifdef CDsetcallback
    CDaddcallback(self->ob_cdparser, (CDDATATYPES) type, CD_callback,
                  (void *) self);
#else
    CDsetcallback(self->ob_cdparser, (CDDATATYPES) type, CD_callback,
                  (void *) self);
#endif
    Py_INCREF(func);
    Py_XSETREF(self->ob_cdcallbacks[type].ob_cdcallback, func);
    Py_INCREF(funcarg);
    Py_XSETREF(self->ob_cdcallbacks[type].ob_cdcallbackarg, funcarg);

/*
    if (type == cd_audio) {
        sigfpe_[_UNDERFL].repls = _ZERO;
        handle_sigfpes(_ON, _EN_UNDERFL, NULL,
                                _ABORT_ON_ERROR, NULL);
    }
*/

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef cdparser_methods[] = {
    {"addcallback",             (PyCFunction)CD_addcallback,    METH_VARARGS},
    {"deleteparser",            (PyCFunction)CD_deleteparser,   METH_VARARGS},
    {"parseframe",              (PyCFunction)CD_parseframe,     METH_VARARGS},
    {"removecallback",          (PyCFunction)CD_removecallback, METH_VARARGS},
    {"resetparser",             (PyCFunction)CD_resetparser,    METH_VARARGS},
                                            /* backward compatibility */
    {"setcallback",             (PyCFunction)CD_addcallback,    METH_VARARGS},
    {NULL,                      NULL}           /* sentinel */
};

static void
cdparser_dealloc(cdparserobject *self)
{
    int i;

    for (i = 0; i < NCALLBACKS; i++) {
        Py_CLEAR(self->ob_cdcallbacks[i].ob_cdcallback);
        Py_CLEAR(self->ob_cdcallbacks[i].ob_cdcallbackarg);
    }
    CDdeleteparser(self->ob_cdparser);
    PyObject_Del(self);
}

static PyObject *
cdparser_getattr(cdparserobject *self, char *name)
{
    if (self->ob_cdparser == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "no parser active");
        return NULL;
    }

    return Py_FindMethod(cdparser_methods, (PyObject *)self, name);
}

PyTypeObject CdParsertype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                          /*ob_size*/
    "cd.cdparser",              /*tp_name*/
    sizeof(cdparserobject),     /*tp_size*/
    0,                          /*tp_itemsize*/
    /* methods */
    (destructor)cdparser_dealloc, /*tp_dealloc*/
    0,                          /*tp_print*/
    (getattrfunc)cdparser_getattr, /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_compare*/
    0,                          /*tp_repr*/
};

static PyObject *
newcdparserobject(CDPARSER *cdp)
{
    cdparserobject *p;
    int i;

    p = PyObject_New(cdparserobject, &CdParsertype);
    if (p == NULL)
        return NULL;
    p->ob_cdparser = cdp;
    for (i = 0; i < NCALLBACKS; i++) {
        p->ob_cdcallbacks[i].ob_cdcallback = NULL;
        p->ob_cdcallbacks[i].ob_cdcallbackarg = NULL;
    }
    return (PyObject *) p;
}

static PyObject *
CD_createparser(PyObject *self, PyObject *args)
{
    CDPARSER *cdp;

    if (!PyArg_ParseTuple(args, ":createparser"))
        return NULL;
    cdp = CDcreateparser();
    if (cdp == NULL) {
        PyErr_SetString(CdError, "createparser failed");
        return NULL;
    }

    return newcdparserobject(cdp);
}

static PyObject *
CD_msftoframe(PyObject *self, PyObject *args)
{
    int min, sec, frame;

    if (!PyArg_ParseTuple(args, "iii:msftoframe", &min, &sec, &frame))
        return NULL;

    return PyInt_FromLong((long) CDmsftoframe(min, sec, frame));
}

static PyMethodDef CD_methods[] = {
    {"open",                    (PyCFunction)CD_open,           METH_VARARGS},
    {"createparser",            (PyCFunction)CD_createparser,   METH_VARARGS},
    {"msftoframe",              (PyCFunction)CD_msftoframe,     METH_VARARGS},
    {NULL,              NULL}   /* Sentinel */
};

void
initcd(void)
{
    PyObject *m, *d;

    if (PyErr_WarnPy3k("the cd module has been removed in "
                       "Python 3.0", 2) < 0)
        return;

    m = Py_InitModule("cd", CD_methods);
    if (m == NULL)
        return;
    d = PyModule_GetDict(m);

    CdError = PyErr_NewException("cd.error", NULL, NULL);
    PyDict_SetItemString(d, "error", CdError);

    /* Identifiers for the different types of callbacks from the parser */
    PyDict_SetItemString(d, "audio", PyInt_FromLong((long) cd_audio));
    PyDict_SetItemString(d, "pnum", PyInt_FromLong((long) cd_pnum));
    PyDict_SetItemString(d, "index", PyInt_FromLong((long) cd_index));
    PyDict_SetItemString(d, "ptime", PyInt_FromLong((long) cd_ptime));
    PyDict_SetItemString(d, "atime", PyInt_FromLong((long) cd_atime));
    PyDict_SetItemString(d, "catalog", PyInt_FromLong((long) cd_catalog));
    PyDict_SetItemString(d, "ident", PyInt_FromLong((long) cd_ident));
    PyDict_SetItemString(d, "control", PyInt_FromLong((long) cd_control));

    /* Block size information for digital audio data */
    PyDict_SetItemString(d, "DATASIZE",
                       PyInt_FromLong((long) CDDA_DATASIZE));
    PyDict_SetItemString(d, "BLOCKSIZE",
                       PyInt_FromLong((long) CDDA_BLOCKSIZE));

    /* Possible states for the cd player */
    PyDict_SetItemString(d, "ERROR", PyInt_FromLong((long) CD_ERROR));
    PyDict_SetItemString(d, "NODISC", PyInt_FromLong((long) CD_NODISC));
    PyDict_SetItemString(d, "READY", PyInt_FromLong((long) CD_READY));
    PyDict_SetItemString(d, "PLAYING", PyInt_FromLong((long) CD_PLAYING));
    PyDict_SetItemString(d, "PAUSED", PyInt_FromLong((long) CD_PAUSED));
    PyDict_SetItemString(d, "STILL", PyInt_FromLong((long) CD_STILL));
#ifdef CD_CDROM                 /* only newer versions of the library */
    PyDict_SetItemString(d, "CDROM", PyInt_FromLong((long) CD_CDROM));
#endif
}
