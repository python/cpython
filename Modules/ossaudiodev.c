/*
 * ossaudiodev -- Python interface to the OSS (Open Sound System) API.
 *                This is the standard audio API for Linux and some
 *                flavours of BSD [XXX which ones?]; it is also available
 *                for a wide range of commercial Unices.
 *
 * Originally written by Peter Bosch, March 2000, as linuxaudiodev.
 *
 * Renamed to ossaudiodev and rearranged/revised/hacked up
 * by Greg Ward <gward@python.net>, November 2002.
 *              
 * (c) 2000 Peter Bosch.  All Rights Reserved.
 * (c) 2002 Gregory P. Ward.  All Rights Reserved.
 * (c) 2002 Python Software Foundation.  All Rights Reserved.
 *
 * XXX need a license statement
 *
 * $Id$
 */

#include "Python.h"
#include "structmember.h"

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#define O_RDONLY 00
#define O_WRONLY 01
#endif


#include <sys/ioctl.h>
#if defined(linux)
#include <linux/soundcard.h>

typedef unsigned long uint32_t;

#elif defined(__FreeBSD__)
#include <machine/soundcard.h>

#ifndef SNDCTL_DSP_CHANNELS
#define SNDCTL_DSP_CHANNELS SOUND_PCM_WRITE_CHANNELS
#endif

#endif

typedef struct {
    PyObject_HEAD;
    int		x_fd;		/* The open file */
    int         x_mode;           /* file mode */
    int		x_icount;	/* Input count */
    int		x_ocount;	/* Output count */
    uint32_t	x_afmts;	/* Audio formats supported by hardware*/
} lad_t;

/* XXX several format defined in soundcard.h are not supported,
   including _NE (native endian) options and S32 options
*/

static struct {
    int		a_bps;
    uint32_t	a_fmt;
    char       *a_name;
} audio_types[] = {
    {  8, 	AFMT_MU_LAW, "logarithmic mu-law 8-bit audio" },
    {  8, 	AFMT_A_LAW,  "logarithmic A-law 8-bit audio" },
    {  8,	AFMT_U8,     "linear unsigned 8-bit audio" },
    {  8, 	AFMT_S8,     "linear signed 8-bit audio" },
    { 16, 	AFMT_U16_BE, "linear unsigned 16-bit big-endian audio" },
    { 16, 	AFMT_U16_LE, "linear unsigned 16-bit little-endian audio" },
    { 16, 	AFMT_S16_BE, "linear signed 16-bit big-endian audio" },
    { 16, 	AFMT_S16_LE, "linear signed 16-bit little-endian audio" },
    { 16, 	AFMT_S16_NE, "linear signed 16-bit native-endian audio" },
};

static int n_audio_types = sizeof(audio_types) / sizeof(audio_types[0]);

static PyTypeObject Ladtype;

static PyObject *LinuxAudioError;

static lad_t *
newladobject(PyObject *arg)
{
    lad_t *xp;
    int fd, afmts, imode;
    char *basedev = NULL;
    char *mode = NULL;

    /* Two ways to call linuxaudiodev.open():
         open(device, mode) (for consistency with builtin open())
         open(mode)         (for backwards compatibility)
       because the *first* argument is optional, parsing args is
       a wee bit tricky. */
    if (!PyArg_ParseTuple(arg, "s|s:open", &basedev, &mode))
       return NULL;
    if (mode == NULL) {                 /* only one arg supplied */
       mode = basedev;
       basedev = NULL;
    }

    if (strcmp(mode, "r") == 0)
        imode = O_RDONLY;
    else if (strcmp(mode, "w") == 0)
        imode = O_WRONLY;
    else if (strcmp(mode, "rw") == 0)
        imode = O_RDWR;
    else {
        PyErr_SetString(LinuxAudioError, "mode must be 'r', 'w', or 'rw'");
        return NULL;
    }

    /* Open the correct device.  The base device name comes from the
     * AUDIODEV environment variable first, then /dev/dsp.  The
     * control device tacks "ctl" onto the base device name.
     * 
     * Note that the only difference between /dev/audio and /dev/dsp
     * is that the former uses logarithmic mu-law encoding and the
     * latter uses 8-bit unsigned encoding.
     */

    if (basedev == NULL) {              /* called with one arg */
       basedev = getenv("AUDIODEV");
       if (basedev == NULL)             /* $AUDIODEV not set */
          basedev = "/dev/dsp";
    }

    if ((fd = open(basedev, imode)) == -1) {
        PyErr_SetFromErrnoWithFilename(LinuxAudioError, basedev);
        return NULL;
    }
    if (ioctl(fd, SNDCTL_DSP_GETFMTS, &afmts) == -1) {
        PyErr_SetFromErrnoWithFilename(LinuxAudioError, basedev);
        return NULL;
    }
    /* Create and initialize the object */
    if ((xp = PyObject_New(lad_t, &Ladtype)) == NULL) {
        close(fd);
        return NULL;
    }
    xp->x_fd = fd;
    xp->x_mode = imode;
    xp->x_icount = xp->x_ocount = 0;
    xp->x_afmts  = afmts;
    return xp;
}

static void
lad_dealloc(lad_t *xp)
{
    /* if already closed, don't reclose it */
    if (xp->x_fd != -1)
	close(xp->x_fd);
    PyObject_Del(xp);
}


/* Methods to wrap the OSS ioctls.  The calling convention is pretty
   simple:
     nonblock()        -> ioctl(fd, SNDCTL_DSP_NONBLOCK)
     fmt = setfmt(fmt) -> ioctl(fd, SNDCTL_DSP_SETFMT, &fmt)
     etc.
*/


/* _do_ioctl_1() is a private helper function used for the OSS ioctls --
   SNDCTL_DSP_{SETFMT,CHANNELS,SPEED} -- that that are called from C
   like this:
     ioctl(fd, SNDCTL_DSP_cmd, &arg)

   where arg is the value to set, and on return the driver sets arg to
   the value that was actually set.  Mapping this to Python is obvious:
     arg = dsp.xxx(arg)
*/
static PyObject *
_do_ioctl_1(lad_t *self, PyObject *args, char *fname, int cmd)
{
    char argfmt[13] = "i:";
    int arg;

    assert(strlen(fname) <= 10);
    strcat(argfmt, fname);
    if (!PyArg_ParseTuple(args, argfmt, &arg))
	return NULL;

    if (ioctl(self->x_fd, cmd, &arg) == -1)
        return PyErr_SetFromErrno(LinuxAudioError);
    return PyInt_FromLong(arg);
}

/* _do_ioctl_0() is a private helper for the no-argument ioctls:
   SNDCTL_DSP_{SYNC,RESET,POST}. */
static PyObject *
_do_ioctl_0(lad_t *self, PyObject *args, char *fname, int cmd)
{
    char argfmt[12] = ":";

    assert(strlen(fname) <= 10);
    strcat(argfmt, fname);
    if (!PyArg_ParseTuple(args, argfmt))
	return NULL;

    if (ioctl(self->x_fd, cmd, 0) == -1)
        return PyErr_SetFromErrno(LinuxAudioError);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
lad_nonblock(lad_t *self, PyObject *args)
{
    /* Hmmm: it doesn't appear to be possible to return to blocking
       mode once we're in non-blocking mode! */
    if (!PyArg_ParseTuple(args, ":nonblock"))
	return NULL;
    if (ioctl(self->x_fd, SNDCTL_DSP_NONBLOCK, NULL) == -1)
        return PyErr_SetFromErrno(LinuxAudioError);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
lad_setfmt(lad_t *self, PyObject *args)
{
    return _do_ioctl_1(self, args, "setfmt", SNDCTL_DSP_SETFMT);
}

static PyObject *
lad_getfmts(lad_t *self, PyObject *args)
{
    int mask;
    if (!PyArg_ParseTuple(args, ":getfmts"))
	return NULL;
    if (ioctl(self->x_fd, SNDCTL_DSP_GETFMTS, &mask) == -1)
        return PyErr_SetFromErrno(LinuxAudioError);
    return PyInt_FromLong(mask);
}

static PyObject *
lad_channels(lad_t *self, PyObject *args)
{
    return _do_ioctl_1(self, args, "channels", SNDCTL_DSP_CHANNELS);
}

static PyObject *
lad_speed(lad_t *self, PyObject *args)
{
    return _do_ioctl_1(self, args, "speed", SNDCTL_DSP_SPEED);
}

static PyObject *
lad_sync(lad_t *self, PyObject *args)
{
    return _do_ioctl_0(self, args, "sync", SNDCTL_DSP_SYNC);
}
    
static PyObject *
lad_reset(lad_t *self, PyObject *args)
{
    return _do_ioctl_0(self, args, "reset", SNDCTL_DSP_RESET);
}
    
static PyObject *
lad_post(lad_t *self, PyObject *args)
{
    return _do_ioctl_0(self, args, "post", SNDCTL_DSP_POST);
}


/* Regular file methods: read(), write(), close(), etc. as well
   as one convenience method, writeall(). */

static PyObject *
lad_read(lad_t *self, PyObject *args)
{
    int size, count;
    char *cp;
    PyObject *rv;
	
    if (!PyArg_ParseTuple(args, "i:read", &size))
        return NULL;
    rv = PyString_FromStringAndSize(NULL, size);
    if (rv == NULL)
        return NULL;
    cp = PyString_AS_STRING(rv);
    if ((count = read(self->x_fd, cp, size)) < 0) {
        PyErr_SetFromErrno(LinuxAudioError);
        Py_DECREF(rv);
        return NULL;
    }
    self->x_icount += count;
    _PyString_Resize(&rv, count);
    return rv;
}

static PyObject *
lad_write(lad_t *self, PyObject *args)
{
    char *cp;
    int rv, size;

    if (!PyArg_ParseTuple(args, "s#:write", &cp, &size)) {
	return NULL;
    }
    if ((rv = write(self->x_fd, cp, size)) == -1) {
        return PyErr_SetFromErrno(LinuxAudioError);
    } else {
        self->x_ocount += rv;
    }
    return PyInt_FromLong(rv);
}

static PyObject *
lad_writeall(lad_t *self, PyObject *args)
{
    char *cp;
    int rv, size;
    fd_set write_set_fds;
    int select_rv;
    
    /* NB. writeall() is only useful in non-blocking mode: according to
       Guenter Geiger <geiger@xdv.org> on the linux-audio-dev list
       (http://eca.cx/lad/2002/11/0380.html), OSS guarantees that
       write() in blocking mode consumes the whole buffer.  In blocking
       mode, the behaviour of write() and writeall() from Python is
       indistinguishable. */

    if (!PyArg_ParseTuple(args, "s#:write", &cp, &size)) 
        return NULL;

    /* use select to wait for audio device to be available */
    FD_ZERO(&write_set_fds);
    FD_SET(self->x_fd, &write_set_fds);

    while (size > 0) {
        select_rv = select(self->x_fd+1, NULL, &write_set_fds, NULL, NULL);
        assert(select_rv != 0);         /* no timeout, can't expire */
        if (select_rv == -1)
            return PyErr_SetFromErrno(LinuxAudioError);

        rv = write(self->x_fd, cp, size);
        if (rv == -1) {
            if (errno == EAGAIN) {      /* buffer is full, try again */
                errno = 0;
                continue;
            } else                      /* it's a real error */
                return PyErr_SetFromErrno(LinuxAudioError);
        } else {                        /* wrote rv bytes */
            self->x_ocount += rv;
            size -= rv;
            cp += rv;
        }
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
lad_close(lad_t *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":close"))
	return NULL;

    if (self->x_fd >= 0) {
        close(self->x_fd);
        self->x_fd = -1;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
lad_fileno(lad_t *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":fileno")) 
	return NULL;
    return PyInt_FromLong(self->x_fd);
}


/* Convenience methods: these generally wrap a couple of ioctls into one
   common task. */

static PyObject *
lad_setparameters(lad_t *self, PyObject *args)
{
    int rate, ssize, nchannels, n, fmt, emulate=0;

    if (!PyArg_ParseTuple(args, "iiii|i:setparameters",
                          &rate, &ssize, &nchannels, &fmt, &emulate))
        return NULL;
  
    if (rate < 0) {
	PyErr_Format(PyExc_ValueError, "expected rate >= 0, not %d",
		     rate); 
	return NULL;
    }
    if (ssize < 0) {
	PyErr_Format(PyExc_ValueError, "expected sample size >= 0, not %d",
		     ssize);
	return NULL;
    }
    if (nchannels != 1 && nchannels != 2) {
	PyErr_Format(PyExc_ValueError, "nchannels must be 1 or 2, not %d",
		     nchannels);
	return NULL;
    }

    for (n = 0; n < n_audio_types; n++)
        if (fmt == audio_types[n].a_fmt)
            break;
    if (n == n_audio_types) {
	PyErr_Format(PyExc_ValueError, "unknown audio encoding: %d", fmt);
	return NULL;
    }
    if (audio_types[n].a_bps != ssize) {
	PyErr_Format(PyExc_ValueError, 
		     "for %s, expected sample size %d, not %d",
		     audio_types[n].a_name, audio_types[n].a_bps, ssize);
	return NULL;
    }

    if (emulate == 0) {
	if ((self->x_afmts & audio_types[n].a_fmt) == 0) {
	    PyErr_Format(PyExc_ValueError, 
			 "%s format not supported by device",
			 audio_types[n].a_name);
	    return NULL;
	}
    }
    if (ioctl(self->x_fd, SNDCTL_DSP_SETFMT, 
	      &audio_types[n].a_fmt) == -1) {
        PyErr_SetFromErrno(LinuxAudioError);
        return NULL;
    }
    if (ioctl(self->x_fd, SNDCTL_DSP_CHANNELS, &nchannels) == -1) {
        PyErr_SetFromErrno(LinuxAudioError);
        return NULL;
    }
    if (ioctl(self->x_fd, SNDCTL_DSP_SPEED, &rate) == -1) {
        PyErr_SetFromErrno(LinuxAudioError);
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static int
_ssize(lad_t *self, int *nchannels, int *ssize)
{
    int fmt;

    fmt = 0;
    if (ioctl(self->x_fd, SNDCTL_DSP_SETFMT, &fmt) < 0) 
        return -errno;

    switch (fmt) {
    case AFMT_MU_LAW:
    case AFMT_A_LAW:
    case AFMT_U8:
    case AFMT_S8:
        *ssize = sizeof(char);
        break;
    case AFMT_S16_LE:
    case AFMT_S16_BE:
    case AFMT_U16_LE:
    case AFMT_U16_BE:
        *ssize = sizeof(short);
        break;
    case AFMT_MPEG:
    case AFMT_IMA_ADPCM:
    default:
        return -EOPNOTSUPP;
    }
    *nchannels = 0;
    if (ioctl(self->x_fd, SNDCTL_DSP_CHANNELS, nchannels) < 0)
        return -errno;
    return 0;
}


/* bufsize returns the size of the hardware audio buffer in number 
   of samples */
static PyObject *
lad_bufsize(lad_t *self, PyObject *args)
{
    audio_buf_info ai;
    int nchannels, ssize;

    if (!PyArg_ParseTuple(args, ":bufsize")) return NULL;

    if (_ssize(self, &nchannels, &ssize) < 0) {
        PyErr_SetFromErrno(LinuxAudioError);
        return NULL;
    }
    if (ioctl(self->x_fd, SNDCTL_DSP_GETOSPACE, &ai) < 0) {
        PyErr_SetFromErrno(LinuxAudioError);
        return NULL;
    }
    return PyInt_FromLong((ai.fragstotal * ai.fragsize) / (nchannels * ssize));
}

/* obufcount returns the number of samples that are available in the 
   hardware for playing */
static PyObject *
lad_obufcount(lad_t *self, PyObject *args)
{
    audio_buf_info ai;
    int nchannels, ssize;

    if (!PyArg_ParseTuple(args, ":obufcount"))
        return NULL;

    if (_ssize(self, &nchannels, &ssize) < 0) {
        PyErr_SetFromErrno(LinuxAudioError);
        return NULL;
    }
    if (ioctl(self->x_fd, SNDCTL_DSP_GETOSPACE, &ai) < 0) {
        PyErr_SetFromErrno(LinuxAudioError);
        return NULL;
    }
    return PyInt_FromLong((ai.fragstotal * ai.fragsize - ai.bytes) / 
                          (ssize * nchannels));
}

/* obufcount returns the number of samples that can be played without
   blocking */
static PyObject *
lad_obuffree(lad_t *self, PyObject *args)
{
    audio_buf_info ai;
    int nchannels, ssize;

    if (!PyArg_ParseTuple(args, ":obuffree"))
        return NULL;

    if (_ssize(self, &nchannels, &ssize) < 0) {
        PyErr_SetFromErrno(LinuxAudioError);
        return NULL;
    }
    if (ioctl(self->x_fd, SNDCTL_DSP_GETOSPACE, &ai) < 0) {
        PyErr_SetFromErrno(LinuxAudioError);
        return NULL;
    }
    return PyInt_FromLong(ai.bytes / (ssize * nchannels));
}

static PyObject *
lad_getptr(lad_t *self, PyObject *args)
{
    count_info info;
    int req;

    if (!PyArg_ParseTuple(args, ":getptr"))
	return NULL;
    
    if (self->x_mode == O_RDONLY)
	req = SNDCTL_DSP_GETIPTR;
    else
	req = SNDCTL_DSP_GETOPTR;
    if (ioctl(self->x_fd, req, &info) == -1) {
        PyErr_SetFromErrno(LinuxAudioError);
        return NULL;
    }
    return Py_BuildValue("iii", info.bytes, info.blocks, info.ptr);
}

static PyMethodDef lad_methods[] = {
    /* Regular file methods */
    { "read",		(PyCFunction)lad_read, METH_VARARGS },
    { "write",		(PyCFunction)lad_write, METH_VARARGS },
    { "writeall",	(PyCFunction)lad_writeall, METH_VARARGS },
    { "close",		(PyCFunction)lad_close, METH_VARARGS },
    { "fileno",     	(PyCFunction)lad_fileno, METH_VARARGS },

    /* Simple ioctl wrappers */
    { "nonblock",       (PyCFunction)lad_nonblock, METH_VARARGS },
    { "setfmt",		(PyCFunction)lad_setfmt, METH_VARARGS },
    { "getfmts",        (PyCFunction)lad_getfmts, METH_VARARGS },
    { "channels",       (PyCFunction)lad_channels, METH_VARARGS },
    { "speed",          (PyCFunction)lad_speed, METH_VARARGS },
    { "sync", 		(PyCFunction)lad_sync, METH_VARARGS },
    { "reset", 		(PyCFunction)lad_reset, METH_VARARGS },
    { "post", 		(PyCFunction)lad_post, METH_VARARGS },

    /* Convenience methods -- wrap a couple of ioctls together */
    { "setparameters",	(PyCFunction)lad_setparameters, METH_VARARGS },
    { "bufsize",	(PyCFunction)lad_bufsize, METH_VARARGS },
    { "obufcount",	(PyCFunction)lad_obufcount, METH_VARARGS },
    { "obuffree",	(PyCFunction)lad_obuffree, METH_VARARGS },
    { "getptr",         (PyCFunction)lad_getptr, METH_VARARGS },

    /* Aliases for backwards compatibility */
    { "flush",		(PyCFunction)lad_sync, METH_VARARGS },

    { NULL,		NULL}		/* sentinel */
};

static PyObject *
lad_getattr(lad_t *xp, char *name)
{
    return Py_FindMethod(lad_methods, (PyObject *)xp, name);
}

static PyTypeObject Ladtype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,				/*ob_size*/
    "linuxaudiodev.linux_audio_device", /*tp_name*/
    sizeof(lad_t),		/*tp_size*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)lad_dealloc,	/*tp_dealloc*/
    0,				/*tp_print*/
    (getattrfunc)lad_getattr,	/*tp_getattr*/
    0,				/*tp_setattr*/
    0,				/*tp_compare*/
    0,				/*tp_repr*/
};

static PyObject *
ladopen(PyObject *self, PyObject *args)
{
    return (PyObject *)newladobject(args);
}

static PyMethodDef linuxaudiodev_methods[] = {
    { "open", ladopen, METH_VARARGS },
    { 0, 0 },
};


#define _EXPORT_INT(mod, name) \
  if (PyModule_AddIntConstant(mod, #name, (long) (name)) == -1) return;

void
initlinuxaudiodev(void)
{
    PyObject *m;
  
    m = Py_InitModule("linuxaudiodev", linuxaudiodev_methods);

    LinuxAudioError = PyErr_NewException("linuxaudiodev.error", NULL, NULL);
    if (LinuxAudioError)
	PyModule_AddObject(m, "error", LinuxAudioError);

    /* Expose the audio format numbers -- essential! */
    _EXPORT_INT(m, AFMT_QUERY);
    _EXPORT_INT(m, AFMT_MU_LAW);
    _EXPORT_INT(m, AFMT_A_LAW);
    _EXPORT_INT(m, AFMT_IMA_ADPCM);
    _EXPORT_INT(m, AFMT_U8);
    _EXPORT_INT(m, AFMT_S16_LE);
    _EXPORT_INT(m, AFMT_S16_BE);
    _EXPORT_INT(m, AFMT_S8);
    _EXPORT_INT(m, AFMT_U16_LE);
    _EXPORT_INT(m, AFMT_U16_BE);
    _EXPORT_INT(m, AFMT_MPEG);
    _EXPORT_INT(m, AFMT_AC3);
    _EXPORT_INT(m, AFMT_S16_NE);

    /* Expose all the ioctl numbers for masochists who like to do this
       stuff directly. */
    _EXPORT_INT(m, SNDCTL_COPR_HALT);
    _EXPORT_INT(m, SNDCTL_COPR_LOAD);
    _EXPORT_INT(m, SNDCTL_COPR_RCODE);
    _EXPORT_INT(m, SNDCTL_COPR_RCVMSG);
    _EXPORT_INT(m, SNDCTL_COPR_RDATA);
    _EXPORT_INT(m, SNDCTL_COPR_RESET);
    _EXPORT_INT(m, SNDCTL_COPR_RUN);
    _EXPORT_INT(m, SNDCTL_COPR_SENDMSG);
    _EXPORT_INT(m, SNDCTL_COPR_WCODE);
    _EXPORT_INT(m, SNDCTL_COPR_WDATA);
    _EXPORT_INT(m, SNDCTL_DSP_BIND_CHANNEL);
    _EXPORT_INT(m, SNDCTL_DSP_CHANNELS);
    _EXPORT_INT(m, SNDCTL_DSP_GETBLKSIZE);
    _EXPORT_INT(m, SNDCTL_DSP_GETCAPS);
    _EXPORT_INT(m, SNDCTL_DSP_GETCHANNELMASK);
    _EXPORT_INT(m, SNDCTL_DSP_GETFMTS);
    _EXPORT_INT(m, SNDCTL_DSP_GETIPTR);
    _EXPORT_INT(m, SNDCTL_DSP_GETISPACE);
    _EXPORT_INT(m, SNDCTL_DSP_GETODELAY);
    _EXPORT_INT(m, SNDCTL_DSP_GETOPTR);
    _EXPORT_INT(m, SNDCTL_DSP_GETOSPACE);
    _EXPORT_INT(m, SNDCTL_DSP_GETSPDIF);
    _EXPORT_INT(m, SNDCTL_DSP_GETTRIGGER);
    _EXPORT_INT(m, SNDCTL_DSP_MAPINBUF);
    _EXPORT_INT(m, SNDCTL_DSP_MAPOUTBUF);
    _EXPORT_INT(m, SNDCTL_DSP_NONBLOCK);
    _EXPORT_INT(m, SNDCTL_DSP_POST);
    _EXPORT_INT(m, SNDCTL_DSP_PROFILE);
    _EXPORT_INT(m, SNDCTL_DSP_RESET);
    _EXPORT_INT(m, SNDCTL_DSP_SAMPLESIZE);
    _EXPORT_INT(m, SNDCTL_DSP_SETDUPLEX);
    _EXPORT_INT(m, SNDCTL_DSP_SETFMT);
    _EXPORT_INT(m, SNDCTL_DSP_SETFRAGMENT);
    _EXPORT_INT(m, SNDCTL_DSP_SETSPDIF);
    _EXPORT_INT(m, SNDCTL_DSP_SETSYNCRO);
    _EXPORT_INT(m, SNDCTL_DSP_SETTRIGGER);
    _EXPORT_INT(m, SNDCTL_DSP_SPEED);
    _EXPORT_INT(m, SNDCTL_DSP_STEREO);
    _EXPORT_INT(m, SNDCTL_DSP_SUBDIVIDE);
    _EXPORT_INT(m, SNDCTL_DSP_SYNC);
    _EXPORT_INT(m, SNDCTL_FM_4OP_ENABLE);
    _EXPORT_INT(m, SNDCTL_FM_LOAD_INSTR);
    _EXPORT_INT(m, SNDCTL_MIDI_INFO);
    _EXPORT_INT(m, SNDCTL_MIDI_MPUCMD);
    _EXPORT_INT(m, SNDCTL_MIDI_MPUMODE);
    _EXPORT_INT(m, SNDCTL_MIDI_PRETIME);
    _EXPORT_INT(m, SNDCTL_SEQ_CTRLRATE);
    _EXPORT_INT(m, SNDCTL_SEQ_GETINCOUNT);
    _EXPORT_INT(m, SNDCTL_SEQ_GETOUTCOUNT);
    _EXPORT_INT(m, SNDCTL_SEQ_GETTIME);
    _EXPORT_INT(m, SNDCTL_SEQ_NRMIDIS);
    _EXPORT_INT(m, SNDCTL_SEQ_NRSYNTHS);
    _EXPORT_INT(m, SNDCTL_SEQ_OUTOFBAND);
    _EXPORT_INT(m, SNDCTL_SEQ_PANIC);
    _EXPORT_INT(m, SNDCTL_SEQ_PERCMODE);
    _EXPORT_INT(m, SNDCTL_SEQ_RESET);
    _EXPORT_INT(m, SNDCTL_SEQ_RESETSAMPLES);
    _EXPORT_INT(m, SNDCTL_SEQ_SYNC);
    _EXPORT_INT(m, SNDCTL_SEQ_TESTMIDI);
    _EXPORT_INT(m, SNDCTL_SEQ_THRESHOLD);
    _EXPORT_INT(m, SNDCTL_SYNTH_CONTROL);
    _EXPORT_INT(m, SNDCTL_SYNTH_ID);
    _EXPORT_INT(m, SNDCTL_SYNTH_INFO);
    _EXPORT_INT(m, SNDCTL_SYNTH_MEMAVL);
    _EXPORT_INT(m, SNDCTL_SYNTH_REMOVESAMPLE);
    _EXPORT_INT(m, SNDCTL_TMR_CONTINUE);
    _EXPORT_INT(m, SNDCTL_TMR_METRONOME);
    _EXPORT_INT(m, SNDCTL_TMR_SELECT);
    _EXPORT_INT(m, SNDCTL_TMR_SOURCE);
    _EXPORT_INT(m, SNDCTL_TMR_START);
    _EXPORT_INT(m, SNDCTL_TMR_STOP);
    _EXPORT_INT(m, SNDCTL_TMR_TEMPO);
    _EXPORT_INT(m, SNDCTL_TMR_TIMEBASE);
}
