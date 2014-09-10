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
 * Mixer interface by Nicholas FitzRoy-Dale <wzdd@lardcave.net>, Dec 2002.
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
#include <sys/soundcard.h>

#if defined(linux)

#ifndef HAVE_STDINT_H
typedef unsigned long uint32_t;
#endif

#elif defined(__FreeBSD__)

# ifndef SNDCTL_DSP_CHANNELS
#  define SNDCTL_DSP_CHANNELS SOUND_PCM_WRITE_CHANNELS
# endif

#endif

typedef struct {
    PyObject_HEAD
    char    *devicename;              /* name of the device file */
    int      fd;                      /* file descriptor */
    int      mode;                    /* file mode (O_RDONLY, etc.) */
    int      icount;                  /* input count */
    int      ocount;                  /* output count */
    uint32_t afmts;                   /* audio formats supported by hardware */
} oss_audio_t;

typedef struct {
    PyObject_HEAD
    int      fd;                      /* The open mixer device */
} oss_mixer_t;


static PyTypeObject OSSAudioType;
static PyTypeObject OSSMixerType;

static PyObject *OSSAudioError;


/* ----------------------------------------------------------------------
 * DSP object initialization/deallocation
 */

static oss_audio_t *
newossobject(PyObject *arg)
{
    oss_audio_t *self;
    int fd, afmts, imode;
    char *devicename = NULL;
    char *mode = NULL;

    /* Two ways to call open():
         open(device, mode) (for consistency with builtin open())
         open(mode)         (for backwards compatibility)
       because the *first* argument is optional, parsing args is
       a wee bit tricky. */
    if (!PyArg_ParseTuple(arg, "s|s:open", &devicename, &mode))
       return NULL;
    if (mode == NULL) {                 /* only one arg supplied */
       mode = devicename;
       devicename = NULL;
    }

    if (strcmp(mode, "r") == 0)
        imode = O_RDONLY;
    else if (strcmp(mode, "w") == 0)
        imode = O_WRONLY;
    else if (strcmp(mode, "rw") == 0)
        imode = O_RDWR;
    else {
        PyErr_SetString(OSSAudioError, "mode must be 'r', 'w', or 'rw'");
        return NULL;
    }

    /* Open the correct device: either the 'device' argument,
       or the AUDIODEV environment variable, or "/dev/dsp". */
    if (devicename == NULL) {              /* called with one arg */
       devicename = getenv("AUDIODEV");
       if (devicename == NULL)             /* $AUDIODEV not set */
          devicename = "/dev/dsp";
    }

    /* Open with O_NONBLOCK to avoid hanging on devices that only allow
       one open at a time.  This does *not* affect later I/O; OSS
       provides a special ioctl() for non-blocking read/write, which is
       exposed via oss_nonblock() below. */
    fd = _Py_open(devicename, imode|O_NONBLOCK);

    if (fd == -1) {
        PyErr_SetFromErrnoWithFilename(PyExc_IOError, devicename);
        return NULL;
    }

    /* And (try to) put it back in blocking mode so we get the
       expected write() semantics. */
    if (fcntl(fd, F_SETFL, 0) == -1) {
        close(fd);
        PyErr_SetFromErrnoWithFilename(PyExc_IOError, devicename);
        return NULL;
    }

    if (ioctl(fd, SNDCTL_DSP_GETFMTS, &afmts) == -1) {
        close(fd);
        PyErr_SetFromErrnoWithFilename(PyExc_IOError, devicename);
        return NULL;
    }
    /* Create and initialize the object */
    if ((self = PyObject_New(oss_audio_t, &OSSAudioType)) == NULL) {
        close(fd);
        return NULL;
    }
    self->devicename = devicename;
    self->fd = fd;
    self->mode = imode;
    self->icount = self->ocount = 0;
    self->afmts  = afmts;
    return self;
}

static void
oss_dealloc(oss_audio_t *self)
{
    /* if already closed, don't reclose it */
    if (self->fd != -1)
        close(self->fd);
    PyObject_Del(self);
}


/* ----------------------------------------------------------------------
 * Mixer object initialization/deallocation
 */

static oss_mixer_t *
newossmixerobject(PyObject *arg)
{
    char *devicename = NULL;
    int fd;
    oss_mixer_t *self;

    if (!PyArg_ParseTuple(arg, "|s", &devicename)) {
        return NULL;
    }

    if (devicename == NULL) {
        devicename = getenv("MIXERDEV");
        if (devicename == NULL)            /* MIXERDEV not set */
            devicename = "/dev/mixer";
    }

    fd = _Py_open(devicename, O_RDWR);
    if (fd == -1) {
        PyErr_SetFromErrnoWithFilename(PyExc_IOError, devicename);
        return NULL;
    }

    if ((self = PyObject_New(oss_mixer_t, &OSSMixerType)) == NULL) {
        close(fd);
        return NULL;
    }

    self->fd = fd;

    return self;
}

static void
oss_mixer_dealloc(oss_mixer_t *self)
{
    /* if already closed, don't reclose it */
    if (self->fd != -1)
        close(self->fd);
    PyObject_Del(self);
}


/* Methods to wrap the OSS ioctls.  The calling convention is pretty
   simple:
     nonblock()        -> ioctl(fd, SNDCTL_DSP_NONBLOCK)
     fmt = setfmt(fmt) -> ioctl(fd, SNDCTL_DSP_SETFMT, &fmt)
     etc.
*/


/* ----------------------------------------------------------------------
 * Helper functions
 */

/* Check if a given file descriptor is valid (i.e. hasn't been closed).
 * If true, return 1. Otherwise, raise ValueError and return 0.
 */
static int _is_fd_valid(int fd)
{
    /* the FD is set to -1 in oss_close()/oss_mixer_close() */
    if (fd >= 0) {
        return 1;
    } else {
        PyErr_SetString(PyExc_ValueError,
                        "Operation on closed OSS device.");
        return 0;
    }
}

/* _do_ioctl_1() is a private helper function used for the OSS ioctls --
   SNDCTL_DSP_{SETFMT,CHANNELS,SPEED} -- that are called from C
   like this:
     ioctl(fd, SNDCTL_DSP_cmd, &arg)

   where arg is the value to set, and on return the driver sets arg to
   the value that was actually set.  Mapping this to Python is obvious:
     arg = dsp.xxx(arg)
*/
static PyObject *
_do_ioctl_1(int fd, PyObject *args, char *fname, int cmd)
{
    char argfmt[33] = "i:";
    int arg;

    assert(strlen(fname) <= 30);
    strncat(argfmt, fname, 30);
    if (!PyArg_ParseTuple(args, argfmt, &arg))
        return NULL;

    if (ioctl(fd, cmd, &arg) == -1)
        return PyErr_SetFromErrno(PyExc_IOError);
    return PyLong_FromLong(arg);
}


/* _do_ioctl_1_internal() is a wrapper for ioctls that take no inputs
   but return an output -- ie. we need to pass a pointer to a local C
   variable so the driver can write its output there, but from Python
   all we see is the return value.  For example,
   SOUND_MIXER_READ_DEVMASK returns a bitmask of available mixer
   devices, but does not use the value of the parameter passed-in in any
   way.
*/
static PyObject *
_do_ioctl_1_internal(int fd, PyObject *args, char *fname, int cmd)
{
    char argfmt[32] = ":";
    int arg = 0;

    assert(strlen(fname) <= 30);
    strncat(argfmt, fname, 30);
    if (!PyArg_ParseTuple(args, argfmt, &arg))
        return NULL;

    if (ioctl(fd, cmd, &arg) == -1)
        return PyErr_SetFromErrno(PyExc_IOError);
    return PyLong_FromLong(arg);
}



/* _do_ioctl_0() is a private helper for the no-argument ioctls:
   SNDCTL_DSP_{SYNC,RESET,POST}. */
static PyObject *
_do_ioctl_0(int fd, PyObject *args, char *fname, int cmd)
{
    char argfmt[32] = ":";
    int rv;

    assert(strlen(fname) <= 30);
    strncat(argfmt, fname, 30);
    if (!PyArg_ParseTuple(args, argfmt))
        return NULL;

    /* According to hannu@opensound.com, all three of the ioctls that
       use this function can block, so release the GIL.  This is
       especially important for SYNC, which can block for several
       seconds. */
    Py_BEGIN_ALLOW_THREADS
    rv = ioctl(fd, cmd, 0);
    Py_END_ALLOW_THREADS

    if (rv == -1)
        return PyErr_SetFromErrno(PyExc_IOError);
    Py_INCREF(Py_None);
    return Py_None;
}


/* ----------------------------------------------------------------------
 * Methods of DSP objects (OSSAudioType)
 */

static PyObject *
oss_nonblock(oss_audio_t *self, PyObject *unused)
{
    if (!_is_fd_valid(self->fd))
        return NULL;

    /* Hmmm: it doesn't appear to be possible to return to blocking
       mode once we're in non-blocking mode! */
    if (ioctl(self->fd, SNDCTL_DSP_NONBLOCK, NULL) == -1)
        return PyErr_SetFromErrno(PyExc_IOError);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
oss_setfmt(oss_audio_t *self, PyObject *args)
{
    if (!_is_fd_valid(self->fd))
        return NULL;

    return _do_ioctl_1(self->fd, args, "setfmt", SNDCTL_DSP_SETFMT);
}

static PyObject *
oss_getfmts(oss_audio_t *self, PyObject *unused)
{
    int mask;

    if (!_is_fd_valid(self->fd))
        return NULL;

    if (ioctl(self->fd, SNDCTL_DSP_GETFMTS, &mask) == -1)
        return PyErr_SetFromErrno(PyExc_IOError);
    return PyLong_FromLong(mask);
}

static PyObject *
oss_channels(oss_audio_t *self, PyObject *args)
{
    if (!_is_fd_valid(self->fd))
        return NULL;

    return _do_ioctl_1(self->fd, args, "channels", SNDCTL_DSP_CHANNELS);
}

static PyObject *
oss_speed(oss_audio_t *self, PyObject *args)
{
    if (!_is_fd_valid(self->fd))
        return NULL;

    return _do_ioctl_1(self->fd, args, "speed", SNDCTL_DSP_SPEED);
}

static PyObject *
oss_sync(oss_audio_t *self, PyObject *args)
{
    if (!_is_fd_valid(self->fd))
        return NULL;

    return _do_ioctl_0(self->fd, args, "sync", SNDCTL_DSP_SYNC);
}

static PyObject *
oss_reset(oss_audio_t *self, PyObject *args)
{
    if (!_is_fd_valid(self->fd))
        return NULL;

    return _do_ioctl_0(self->fd, args, "reset", SNDCTL_DSP_RESET);
}

static PyObject *
oss_post(oss_audio_t *self, PyObject *args)
{
    if (!_is_fd_valid(self->fd))
        return NULL;

    return _do_ioctl_0(self->fd, args, "post", SNDCTL_DSP_POST);
}


/* Regular file methods: read(), write(), close(), etc. as well
   as one convenience method, writeall(). */

static PyObject *
oss_read(oss_audio_t *self, PyObject *args)
{
    int size, count;
    char *cp;
    PyObject *rv;

    if (!_is_fd_valid(self->fd))
        return NULL;

    if (!PyArg_ParseTuple(args, "i:read", &size))
        return NULL;
    rv = PyBytes_FromStringAndSize(NULL, size);
    if (rv == NULL)
        return NULL;
    cp = PyBytes_AS_STRING(rv);

    Py_BEGIN_ALLOW_THREADS
    count = read(self->fd, cp, size);
    Py_END_ALLOW_THREADS

    if (count < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        Py_DECREF(rv);
        return NULL;
    }
    self->icount += count;
    _PyBytes_Resize(&rv, count);
    return rv;
}

static PyObject *
oss_write(oss_audio_t *self, PyObject *args)
{
    char *cp;
    int rv, size;

    if (!_is_fd_valid(self->fd))
        return NULL;

    if (!PyArg_ParseTuple(args, "y#:write", &cp, &size)) {
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    rv = write(self->fd, cp, size);
    Py_END_ALLOW_THREADS

    if (rv == -1) {
        return PyErr_SetFromErrno(PyExc_IOError);
    } else {
        self->ocount += rv;
    }
    return PyLong_FromLong(rv);
}

static PyObject *
oss_writeall(oss_audio_t *self, PyObject *args)
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

    if (!_is_fd_valid(self->fd))
        return NULL;

    if (!PyArg_ParseTuple(args, "y#:write", &cp, &size))
        return NULL;

    if (!_PyIsSelectable_fd(self->fd)) {
        PyErr_SetString(PyExc_ValueError,
                        "file descriptor out of range for select");
        return NULL;
    }
    /* use select to wait for audio device to be available */
    FD_ZERO(&write_set_fds);
    FD_SET(self->fd, &write_set_fds);

    while (size > 0) {
        Py_BEGIN_ALLOW_THREADS
        select_rv = select(self->fd+1, NULL, &write_set_fds, NULL, NULL);
        Py_END_ALLOW_THREADS
        assert(select_rv != 0);         /* no timeout, can't expire */
        if (select_rv == -1)
            return PyErr_SetFromErrno(PyExc_IOError);

        Py_BEGIN_ALLOW_THREADS
        rv = write(self->fd, cp, size);
        Py_END_ALLOW_THREADS
        if (rv == -1) {
            if (errno == EAGAIN) {      /* buffer is full, try again */
                errno = 0;
                continue;
            } else                      /* it's a real error */
                return PyErr_SetFromErrno(PyExc_IOError);
        } else {                        /* wrote rv bytes */
            self->ocount += rv;
            size -= rv;
            cp += rv;
        }
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
oss_close(oss_audio_t *self, PyObject *unused)
{
    if (self->fd >= 0) {
        Py_BEGIN_ALLOW_THREADS
        close(self->fd);
        Py_END_ALLOW_THREADS
        self->fd = -1;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
oss_self(PyObject *self, PyObject *unused)
{
    Py_INCREF(self);
    return self;
}

static PyObject *
oss_exit(PyObject *self, PyObject *unused)
{
    _Py_IDENTIFIER(close);

    PyObject *ret = _PyObject_CallMethodId(self, &PyId_close, NULL);
    if (!ret)
        return NULL;
    Py_DECREF(ret);
    Py_RETURN_NONE;
}

static PyObject *
oss_fileno(oss_audio_t *self, PyObject *unused)
{
    if (!_is_fd_valid(self->fd))
        return NULL;

    return PyLong_FromLong(self->fd);
}


/* Convenience methods: these generally wrap a couple of ioctls into one
   common task. */

static PyObject *
oss_setparameters(oss_audio_t *self, PyObject *args)
{
    int wanted_fmt, wanted_channels, wanted_rate, strict=0;
    int fmt, channels, rate;

    if (!_is_fd_valid(self->fd))
        return NULL;

    if (!PyArg_ParseTuple(args, "iii|i:setparameters",
                          &wanted_fmt, &wanted_channels, &wanted_rate,
                          &strict))
        return NULL;

    fmt = wanted_fmt;
    if (ioctl(self->fd, SNDCTL_DSP_SETFMT, &fmt) == -1) {
        return PyErr_SetFromErrno(PyExc_IOError);
    }
    if (strict && fmt != wanted_fmt) {
        return PyErr_Format
            (OSSAudioError,
             "unable to set requested format (wanted %d, got %d)",
             wanted_fmt, fmt);
    }

    channels = wanted_channels;
    if (ioctl(self->fd, SNDCTL_DSP_CHANNELS, &channels) == -1) {
        return PyErr_SetFromErrno(PyExc_IOError);
    }
    if (strict && channels != wanted_channels) {
        return PyErr_Format
            (OSSAudioError,
             "unable to set requested channels (wanted %d, got %d)",
             wanted_channels, channels);
    }

    rate = wanted_rate;
    if (ioctl(self->fd, SNDCTL_DSP_SPEED, &rate) == -1) {
        return PyErr_SetFromErrno(PyExc_IOError);
    }
    if (strict && rate != wanted_rate) {
        return PyErr_Format
            (OSSAudioError,
             "unable to set requested rate (wanted %d, got %d)",
             wanted_rate, rate);
    }

    /* Construct the return value: a (fmt, channels, rate) tuple that
       tells what the audio hardware was actually set to. */
    return Py_BuildValue("(iii)", fmt, channels, rate);
}

static int
_ssize(oss_audio_t *self, int *nchannels, int *ssize)
{
    int fmt;

    fmt = 0;
    if (ioctl(self->fd, SNDCTL_DSP_SETFMT, &fmt) < 0)
        return -errno;

    switch (fmt) {
    case AFMT_MU_LAW:
    case AFMT_A_LAW:
    case AFMT_U8:
    case AFMT_S8:
        *ssize = 1;                     /* 8 bit formats: 1 byte */
        break;
    case AFMT_S16_LE:
    case AFMT_S16_BE:
    case AFMT_U16_LE:
    case AFMT_U16_BE:
        *ssize = 2;                     /* 16 bit formats: 2 byte */
        break;
    case AFMT_MPEG:
    case AFMT_IMA_ADPCM:
    default:
        return -EOPNOTSUPP;
    }
    if (ioctl(self->fd, SNDCTL_DSP_CHANNELS, nchannels) < 0)
        return -errno;
    return 0;
}


/* bufsize returns the size of the hardware audio buffer in number
   of samples */
static PyObject *
oss_bufsize(oss_audio_t *self, PyObject *unused)
{
    audio_buf_info ai;
    int nchannels=0, ssize=0;

    if (!_is_fd_valid(self->fd))
        return NULL;

    if (_ssize(self, &nchannels, &ssize) < 0 || !nchannels || !ssize) {
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }
    if (ioctl(self->fd, SNDCTL_DSP_GETOSPACE, &ai) < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }
    return PyLong_FromLong((ai.fragstotal * ai.fragsize) / (nchannels * ssize));
}

/* obufcount returns the number of samples that are available in the
   hardware for playing */
static PyObject *
oss_obufcount(oss_audio_t *self, PyObject *unused)
{
    audio_buf_info ai;
    int nchannels=0, ssize=0;

    if (!_is_fd_valid(self->fd))
        return NULL;

    if (_ssize(self, &nchannels, &ssize) < 0 || !nchannels || !ssize) {
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }
    if (ioctl(self->fd, SNDCTL_DSP_GETOSPACE, &ai) < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }
    return PyLong_FromLong((ai.fragstotal * ai.fragsize - ai.bytes) /
                          (ssize * nchannels));
}

/* obufcount returns the number of samples that can be played without
   blocking */
static PyObject *
oss_obuffree(oss_audio_t *self, PyObject *unused)
{
    audio_buf_info ai;
    int nchannels=0, ssize=0;

    if (!_is_fd_valid(self->fd))
        return NULL;

    if (_ssize(self, &nchannels, &ssize) < 0 || !nchannels || !ssize) {
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }
    if (ioctl(self->fd, SNDCTL_DSP_GETOSPACE, &ai) < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }
    return PyLong_FromLong(ai.bytes / (ssize * nchannels));
}

static PyObject *
oss_getptr(oss_audio_t *self, PyObject *unused)
{
    count_info info;
    int req;

    if (!_is_fd_valid(self->fd))
        return NULL;

    if (self->mode == O_RDONLY)
        req = SNDCTL_DSP_GETIPTR;
    else
        req = SNDCTL_DSP_GETOPTR;
    if (ioctl(self->fd, req, &info) == -1) {
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }
    return Py_BuildValue("iii", info.bytes, info.blocks, info.ptr);
}


/* ----------------------------------------------------------------------
 * Methods of mixer objects (OSSMixerType)
 */

static PyObject *
oss_mixer_close(oss_mixer_t *self, PyObject *unused)
{
    if (self->fd >= 0) {
        close(self->fd);
        self->fd = -1;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
oss_mixer_fileno(oss_mixer_t *self, PyObject *unused)
{
    if (!_is_fd_valid(self->fd))
        return NULL;

    return PyLong_FromLong(self->fd);
}

/* Simple mixer interface methods */

static PyObject *
oss_mixer_controls(oss_mixer_t *self, PyObject *args)
{
    if (!_is_fd_valid(self->fd))
        return NULL;

    return _do_ioctl_1_internal(self->fd, args, "controls",
        SOUND_MIXER_READ_DEVMASK);
}

static PyObject *
oss_mixer_stereocontrols(oss_mixer_t *self, PyObject *args)
{
    if (!_is_fd_valid(self->fd))
        return NULL;

    return _do_ioctl_1_internal(self->fd, args, "stereocontrols",
        SOUND_MIXER_READ_STEREODEVS);
}

static PyObject *
oss_mixer_reccontrols(oss_mixer_t *self, PyObject *args)
{
    if (!_is_fd_valid(self->fd))
        return NULL;

    return _do_ioctl_1_internal(self->fd, args, "reccontrols",
        SOUND_MIXER_READ_RECMASK);
}

static PyObject *
oss_mixer_get(oss_mixer_t *self, PyObject *args)
{
    int channel, volume;

    if (!_is_fd_valid(self->fd))
        return NULL;

    /* Can't use _do_ioctl_1 because of encoded arg thingy. */
    if (!PyArg_ParseTuple(args, "i:get", &channel))
        return NULL;

    if (channel < 0 || channel > SOUND_MIXER_NRDEVICES) {
        PyErr_SetString(OSSAudioError, "Invalid mixer channel specified.");
        return NULL;
    }

    if (ioctl(self->fd, MIXER_READ(channel), &volume) == -1)
        return PyErr_SetFromErrno(PyExc_IOError);

    return Py_BuildValue("(ii)", volume & 0xff, (volume & 0xff00) >> 8);
}

static PyObject *
oss_mixer_set(oss_mixer_t *self, PyObject *args)
{
    int channel, volume, leftVol, rightVol;

    if (!_is_fd_valid(self->fd))
        return NULL;

    /* Can't use _do_ioctl_1 because of encoded arg thingy. */
    if (!PyArg_ParseTuple(args, "i(ii):set", &channel, &leftVol, &rightVol))
        return NULL;

    if (channel < 0 || channel > SOUND_MIXER_NRDEVICES) {
        PyErr_SetString(OSSAudioError, "Invalid mixer channel specified.");
        return NULL;
    }

    if (leftVol < 0 || rightVol < 0 || leftVol > 100 || rightVol > 100) {
        PyErr_SetString(OSSAudioError, "Volumes must be between 0 and 100.");
        return NULL;
    }

    volume = (rightVol << 8) | leftVol;

    if (ioctl(self->fd, MIXER_WRITE(channel), &volume) == -1)
        return PyErr_SetFromErrno(PyExc_IOError);

    return Py_BuildValue("(ii)", volume & 0xff, (volume & 0xff00) >> 8);
}

static PyObject *
oss_mixer_get_recsrc(oss_mixer_t *self, PyObject *args)
{
    if (!_is_fd_valid(self->fd))
        return NULL;

    return _do_ioctl_1_internal(self->fd, args, "get_recsrc",
        SOUND_MIXER_READ_RECSRC);
}

static PyObject *
oss_mixer_set_recsrc(oss_mixer_t *self, PyObject *args)
{
    if (!_is_fd_valid(self->fd))
        return NULL;

    return _do_ioctl_1(self->fd, args, "set_recsrc",
        SOUND_MIXER_WRITE_RECSRC);
}


/* ----------------------------------------------------------------------
 * Method tables and other bureaucracy
 */

static PyMethodDef oss_methods[] = {
    /* Regular file methods */
    { "read",           (PyCFunction)oss_read, METH_VARARGS },
    { "write",          (PyCFunction)oss_write, METH_VARARGS },
    { "writeall",       (PyCFunction)oss_writeall, METH_VARARGS },
    { "close",          (PyCFunction)oss_close, METH_NOARGS },
    { "fileno",         (PyCFunction)oss_fileno, METH_NOARGS },

    /* Simple ioctl wrappers */
    { "nonblock",       (PyCFunction)oss_nonblock, METH_NOARGS },
    { "setfmt",         (PyCFunction)oss_setfmt, METH_VARARGS },
    { "getfmts",        (PyCFunction)oss_getfmts, METH_NOARGS },
    { "channels",       (PyCFunction)oss_channels, METH_VARARGS },
    { "speed",          (PyCFunction)oss_speed, METH_VARARGS },
    { "sync",           (PyCFunction)oss_sync, METH_VARARGS },
    { "reset",          (PyCFunction)oss_reset, METH_VARARGS },
    { "post",           (PyCFunction)oss_post, METH_VARARGS },

    /* Convenience methods -- wrap a couple of ioctls together */
    { "setparameters",  (PyCFunction)oss_setparameters, METH_VARARGS },
    { "bufsize",        (PyCFunction)oss_bufsize, METH_NOARGS },
    { "obufcount",      (PyCFunction)oss_obufcount, METH_NOARGS },
    { "obuffree",       (PyCFunction)oss_obuffree, METH_NOARGS },
    { "getptr",         (PyCFunction)oss_getptr, METH_NOARGS },

    /* Aliases for backwards compatibility */
    { "flush",          (PyCFunction)oss_sync, METH_VARARGS },

    /* Support for the context management protocol */
    { "__enter__",      oss_self, METH_NOARGS },
    { "__exit__",       oss_exit, METH_VARARGS },

    { NULL,             NULL}           /* sentinel */
};

static PyMethodDef oss_mixer_methods[] = {
    /* Regular file method - OSS mixers are ioctl-only interface */
    { "close",          (PyCFunction)oss_mixer_close, METH_NOARGS },
    { "fileno",         (PyCFunction)oss_mixer_fileno, METH_NOARGS },

    /* Support for the context management protocol */
    { "__enter__",      oss_self, METH_NOARGS },
    { "__exit__",       oss_exit, METH_VARARGS },

    /* Simple ioctl wrappers */
    { "controls",       (PyCFunction)oss_mixer_controls, METH_VARARGS },
    { "stereocontrols", (PyCFunction)oss_mixer_stereocontrols, METH_VARARGS},
    { "reccontrols",    (PyCFunction)oss_mixer_reccontrols, METH_VARARGS},
    { "get",            (PyCFunction)oss_mixer_get, METH_VARARGS },
    { "set",            (PyCFunction)oss_mixer_set, METH_VARARGS },
    { "get_recsrc",     (PyCFunction)oss_mixer_get_recsrc, METH_VARARGS },
    { "set_recsrc",     (PyCFunction)oss_mixer_set_recsrc, METH_VARARGS },

    { NULL,             NULL}
};

static PyObject *
oss_getattro(oss_audio_t *self, PyObject *nameobj)
{
    char *name = "";
    PyObject * rval = NULL;

    if (PyUnicode_Check(nameobj))
        name = _PyUnicode_AsString(nameobj);

    if (strcmp(name, "closed") == 0) {
        rval = (self->fd == -1) ? Py_True : Py_False;
        Py_INCREF(rval);
    }
    else if (strcmp(name, "name") == 0) {
        rval = PyUnicode_FromString(self->devicename);
    }
    else if (strcmp(name, "mode") == 0) {
        /* No need for a "default" in this switch: from newossobject(),
           self->mode can only be one of these three values. */
        switch(self->mode) {
            case O_RDONLY:
                rval = PyUnicode_FromString("r");
                break;
            case O_RDWR:
                rval = PyUnicode_FromString("rw");
                break;
            case O_WRONLY:
                rval = PyUnicode_FromString("w");
                break;
        }
    }
    else {
        rval = PyObject_GenericGetAttr((PyObject *)self, nameobj);
    }
    return rval;
}

static PyTypeObject OSSAudioType = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "ossaudiodev.oss_audio_device", /*tp_name*/
    sizeof(oss_audio_t),        /*tp_size*/
    0,                          /*tp_itemsize*/
    /* methods */
    (destructor)oss_dealloc,    /*tp_dealloc*/
    0,                          /*tp_print*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_reserved*/
    0,                          /*tp_repr*/
    0,                          /*tp_as_number*/
    0,                          /*tp_as_sequence*/
    0,                          /*tp_as_mapping*/
    0,                          /*tp_hash*/
    0,                          /*tp_call*/
    0,                          /*tp_str*/
    (getattrofunc)oss_getattro, /*tp_getattro*/
    0,                          /*tp_setattro*/
    0,                          /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,         /*tp_flags*/
    0,                          /*tp_doc*/
    0,                          /*tp_traverse*/
    0,                          /*tp_clear*/
    0,                          /*tp_richcompare*/
    0,                          /*tp_weaklistoffset*/
    0,                          /*tp_iter*/
    0,                          /*tp_iternext*/
    oss_methods,                /*tp_methods*/
};

static PyTypeObject OSSMixerType = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "ossaudiodev.oss_mixer_device", /*tp_name*/
    sizeof(oss_mixer_t),            /*tp_size*/
    0,                              /*tp_itemsize*/
    /* methods */
    (destructor)oss_mixer_dealloc,  /*tp_dealloc*/
    0,                              /*tp_print*/
    0,                              /*tp_getattr*/
    0,                              /*tp_setattr*/
    0,                              /*tp_reserved*/
    0,                              /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                              /*tp_as_sequence*/
    0,                              /*tp_as_mapping*/
    0,                              /*tp_hash*/
    0,                              /*tp_call*/
    0,                              /*tp_str*/
    0,                              /*tp_getattro*/
    0,                              /*tp_setattro*/
    0,                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,             /*tp_flags*/
    0,                              /*tp_doc*/
    0,                              /*tp_traverse*/
    0,                              /*tp_clear*/
    0,                              /*tp_richcompare*/
    0,                              /*tp_weaklistoffset*/
    0,                              /*tp_iter*/
    0,                              /*tp_iternext*/
    oss_mixer_methods,              /*tp_methods*/
};


static PyObject *
ossopen(PyObject *self, PyObject *args)
{
    return (PyObject *)newossobject(args);
}

static PyObject *
ossopenmixer(PyObject *self, PyObject *args)
{
    return (PyObject *)newossmixerobject(args);
}

static PyMethodDef ossaudiodev_methods[] = {
    { "open", ossopen, METH_VARARGS },
    { "openmixer", ossopenmixer, METH_VARARGS },
    { 0, 0 },
};


#define _EXPORT_INT(mod, name) \
  if (PyModule_AddIntConstant(mod, #name, (long) (name)) == -1) return NULL;


static char *control_labels[] = SOUND_DEVICE_LABELS;
static char *control_names[] = SOUND_DEVICE_NAMES;


static int
build_namelists (PyObject *module)
{
    PyObject *labels;
    PyObject *names;
    PyObject *s;
    int num_controls;
    int i;

    num_controls = Py_ARRAY_LENGTH(control_labels);
    assert(num_controls == Py_ARRAY_LENGTH(control_names));

    labels = PyList_New(num_controls);
    names = PyList_New(num_controls);
    if (labels == NULL || names == NULL)
        goto error2;
    for (i = 0; i < num_controls; i++) {
        s = PyUnicode_FromString(control_labels[i]);
        if (s == NULL)
            goto error2;
        PyList_SET_ITEM(labels, i, s);

        s = PyUnicode_FromString(control_names[i]);
        if (s == NULL)
            goto error2;
        PyList_SET_ITEM(names, i, s);
    }

    if (PyModule_AddObject(module, "control_labels", labels) == -1)
        goto error2;
    if (PyModule_AddObject(module, "control_names", names) == -1)
        goto error1;

    return 0;

error2:
    Py_XDECREF(labels);
error1:
    Py_XDECREF(names);
    return -1;
}


static struct PyModuleDef ossaudiodevmodule = {
        PyModuleDef_HEAD_INIT,
        "ossaudiodev",
        NULL,
        -1,
        ossaudiodev_methods,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC
PyInit_ossaudiodev(void)
{
    PyObject *m;

    if (PyType_Ready(&OSSAudioType) < 0)
        return NULL;

    if (PyType_Ready(&OSSMixerType) < 0)
        return NULL;

    m = PyModule_Create(&ossaudiodevmodule);
    if (m == NULL)
        return NULL;

    OSSAudioError = PyErr_NewException("ossaudiodev.OSSAudioError",
                                       NULL, NULL);
    if (OSSAudioError) {
        /* Each call to PyModule_AddObject decrefs it; compensate: */
        Py_INCREF(OSSAudioError);
        Py_INCREF(OSSAudioError);
        PyModule_AddObject(m, "error", OSSAudioError);
        PyModule_AddObject(m, "OSSAudioError", OSSAudioError);
    }

    /* Build 'control_labels' and 'control_names' lists and add them
       to the module. */
    if (build_namelists(m) == -1)       /* XXX what to do here? */
        return NULL;

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
#ifdef AFMT_AC3
    _EXPORT_INT(m, AFMT_AC3);
#endif
#ifdef AFMT_S16_NE
    _EXPORT_INT(m, AFMT_S16_NE);
#endif
#ifdef AFMT_U16_NE
    _EXPORT_INT(m, AFMT_U16_NE);
#endif
#ifdef AFMT_S32_LE
    _EXPORT_INT(m, AFMT_S32_LE);
#endif
#ifdef AFMT_S32_BE
    _EXPORT_INT(m, AFMT_S32_BE);
#endif
#ifdef AFMT_MPEG
    _EXPORT_INT(m, AFMT_MPEG);
#endif

    /* Expose the sound mixer device numbers. */
    _EXPORT_INT(m, SOUND_MIXER_NRDEVICES);
    _EXPORT_INT(m, SOUND_MIXER_VOLUME);
    _EXPORT_INT(m, SOUND_MIXER_BASS);
    _EXPORT_INT(m, SOUND_MIXER_TREBLE);
    _EXPORT_INT(m, SOUND_MIXER_SYNTH);
    _EXPORT_INT(m, SOUND_MIXER_PCM);
    _EXPORT_INT(m, SOUND_MIXER_SPEAKER);
    _EXPORT_INT(m, SOUND_MIXER_LINE);
    _EXPORT_INT(m, SOUND_MIXER_MIC);
    _EXPORT_INT(m, SOUND_MIXER_CD);
    _EXPORT_INT(m, SOUND_MIXER_IMIX);
    _EXPORT_INT(m, SOUND_MIXER_ALTPCM);
    _EXPORT_INT(m, SOUND_MIXER_RECLEV);
    _EXPORT_INT(m, SOUND_MIXER_IGAIN);
    _EXPORT_INT(m, SOUND_MIXER_OGAIN);
    _EXPORT_INT(m, SOUND_MIXER_LINE1);
    _EXPORT_INT(m, SOUND_MIXER_LINE2);
    _EXPORT_INT(m, SOUND_MIXER_LINE3);
#ifdef SOUND_MIXER_DIGITAL1
    _EXPORT_INT(m, SOUND_MIXER_DIGITAL1);
#endif
#ifdef SOUND_MIXER_DIGITAL2
    _EXPORT_INT(m, SOUND_MIXER_DIGITAL2);
#endif
#ifdef SOUND_MIXER_DIGITAL3
    _EXPORT_INT(m, SOUND_MIXER_DIGITAL3);
#endif
#ifdef SOUND_MIXER_PHONEIN
    _EXPORT_INT(m, SOUND_MIXER_PHONEIN);
#endif
#ifdef SOUND_MIXER_PHONEOUT
    _EXPORT_INT(m, SOUND_MIXER_PHONEOUT);
#endif
#ifdef SOUND_MIXER_VIDEO
    _EXPORT_INT(m, SOUND_MIXER_VIDEO);
#endif
#ifdef SOUND_MIXER_RADIO
    _EXPORT_INT(m, SOUND_MIXER_RADIO);
#endif
#ifdef SOUND_MIXER_MONITOR
    _EXPORT_INT(m, SOUND_MIXER_MONITOR);
#endif

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
#ifdef SNDCTL_DSP_BIND_CHANNEL
    _EXPORT_INT(m, SNDCTL_DSP_BIND_CHANNEL);
#endif
    _EXPORT_INT(m, SNDCTL_DSP_CHANNELS);
    _EXPORT_INT(m, SNDCTL_DSP_GETBLKSIZE);
    _EXPORT_INT(m, SNDCTL_DSP_GETCAPS);
#ifdef SNDCTL_DSP_GETCHANNELMASK
    _EXPORT_INT(m, SNDCTL_DSP_GETCHANNELMASK);
#endif
    _EXPORT_INT(m, SNDCTL_DSP_GETFMTS);
    _EXPORT_INT(m, SNDCTL_DSP_GETIPTR);
    _EXPORT_INT(m, SNDCTL_DSP_GETISPACE);
#ifdef SNDCTL_DSP_GETODELAY
    _EXPORT_INT(m, SNDCTL_DSP_GETODELAY);
#endif
    _EXPORT_INT(m, SNDCTL_DSP_GETOPTR);
    _EXPORT_INT(m, SNDCTL_DSP_GETOSPACE);
#ifdef SNDCTL_DSP_GETSPDIF
    _EXPORT_INT(m, SNDCTL_DSP_GETSPDIF);
#endif
    _EXPORT_INT(m, SNDCTL_DSP_GETTRIGGER);
    _EXPORT_INT(m, SNDCTL_DSP_MAPINBUF);
    _EXPORT_INT(m, SNDCTL_DSP_MAPOUTBUF);
    _EXPORT_INT(m, SNDCTL_DSP_NONBLOCK);
    _EXPORT_INT(m, SNDCTL_DSP_POST);
#ifdef SNDCTL_DSP_PROFILE
    _EXPORT_INT(m, SNDCTL_DSP_PROFILE);
#endif
    _EXPORT_INT(m, SNDCTL_DSP_RESET);
    _EXPORT_INT(m, SNDCTL_DSP_SAMPLESIZE);
    _EXPORT_INT(m, SNDCTL_DSP_SETDUPLEX);
    _EXPORT_INT(m, SNDCTL_DSP_SETFMT);
    _EXPORT_INT(m, SNDCTL_DSP_SETFRAGMENT);
#ifdef SNDCTL_DSP_SETSPDIF
    _EXPORT_INT(m, SNDCTL_DSP_SETSPDIF);
#endif
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
#ifdef SNDCTL_SEQ_GETTIME
    _EXPORT_INT(m, SNDCTL_SEQ_GETTIME);
#endif
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
#ifdef SNDCTL_SYNTH_CONTROL
    _EXPORT_INT(m, SNDCTL_SYNTH_CONTROL);
#endif
#ifdef SNDCTL_SYNTH_ID
    _EXPORT_INT(m, SNDCTL_SYNTH_ID);
#endif
    _EXPORT_INT(m, SNDCTL_SYNTH_INFO);
    _EXPORT_INT(m, SNDCTL_SYNTH_MEMAVL);
#ifdef SNDCTL_SYNTH_REMOVESAMPLE
    _EXPORT_INT(m, SNDCTL_SYNTH_REMOVESAMPLE);
#endif
    _EXPORT_INT(m, SNDCTL_TMR_CONTINUE);
    _EXPORT_INT(m, SNDCTL_TMR_METRONOME);
    _EXPORT_INT(m, SNDCTL_TMR_SELECT);
    _EXPORT_INT(m, SNDCTL_TMR_SOURCE);
    _EXPORT_INT(m, SNDCTL_TMR_START);
    _EXPORT_INT(m, SNDCTL_TMR_STOP);
    _EXPORT_INT(m, SNDCTL_TMR_TEMPO);
    _EXPORT_INT(m, SNDCTL_TMR_TIMEBASE);
    return m;
}
