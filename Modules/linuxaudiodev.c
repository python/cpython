/* Hey Emacs, this is -*-C-*- 
 ******************************************************************************
 * linuxaudiodev.c -- Linux audio device for python.
 * 
 * Author          : Peter Bosch
 * Created On      : Thu Mar  2 21:10:33 2000
 * Last Modified By: Peter Bosch
 * Last Modified On: Fri Mar 24 11:27:00 2000
 * Status          : Unknown, Use with caution!
 * 
 * Unless other notices are present in any part of this file
 * explicitly claiming copyrights for other people and/or 
 * organizations, the contents of this file is fully copyright 
 * (C) 2000 Peter Bosch, all rights reserved.
 ******************************************************************************
 */

#include "Python.h"
#include "structmember.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <sys/ioctl.h>
#include <linux/soundcard.h>

typedef unsigned long uint32_t;

typedef struct {
  PyObject_HEAD;
  int		x_fd;		/* The open file */
  int		x_icount;	/* Input count */
  int		x_ocount;	/* Output count */
  uint32_t	x_afmts;	/* Supported audio formats */
} lad_t;

static struct {
  int		a_bps;
  uint32_t	a_fmt;
} audio_types[] = {
  {  8, 	AFMT_MU_LAW },
  {  8,		AFMT_U8 },
  {  8, 	AFMT_S8 },
  { 16, 	AFMT_U16_BE },
  { 16, 	AFMT_U16_LE },
  { 16, 	AFMT_S16_BE },
  { 16, 	AFMT_S16_LE },
};


staticforward PyTypeObject Ladtype;

static PyObject *LinuxAudioError;

static lad_t *
newladobject(PyObject *arg)
{
  lad_t *xp;
  int fd, afmts, imode;
  char *mode;
  char *basedev;
  char *ctldev;
  char *opendev;

  /* Check arg for r/w/rw */
  if (!PyArg_ParseTuple(arg, "s", &mode)) return NULL;
  if (strcmp(mode, "r") == 0)
    imode = 0;
  else if (strcmp(mode, "w") == 0)
    imode = 1;
  else {
    PyErr_SetString(LinuxAudioError, "Mode should be one of 'r', or 'w'");
    return NULL;
  }
	
  /* Open the correct device.  The base device name comes from the
   * AUDIODEV environment variable first, then /dev/audio.  The
   * control device tacks "ctl" onto the base device name.
   */
  basedev = getenv("AUDIODEV");
  if (!basedev)
    basedev = "/dev/dsp";

  if ((fd = open(basedev, imode)) < 0) {
    PyErr_SetFromErrnoWithFilename(LinuxAudioError, basedev);
    return NULL;
  }

  if (imode) {
    if (ioctl(fd, SNDCTL_DSP_NONBLOCK, NULL) < 0) {
      PyErr_SetFromErrnoWithFilename(LinuxAudioError, basedev);
      return NULL;
    }
  }

  if (ioctl(fd, SNDCTL_DSP_GETFMTS, &afmts) < 0) {
    PyErr_SetFromErrnoWithFilename(LinuxAudioError, basedev);
    return NULL;
  }

  /* Create and initialize the object */
  if ((xp = PyObject_NEW(lad_t, &Ladtype)) == NULL) {
    close(fd);
    return NULL;
  }
  xp->x_fd     = fd;
  xp->x_icount = xp->x_ocount = 0;
  xp->x_afmts  = afmts;
  return xp;
}

static void
lad_dealloc(lad_t *xp)
{
  close(xp->x_fd);
  PyMem_DEL(xp);
}

static PyObject *
lad_read(lad_t *self, PyObject *args)
{
  int size, count;
  char *cp;
  PyObject *rv;
	
  if (!PyArg_ParseTuple(args, "i", &size)) return NULL;
  rv = PyString_FromStringAndSize(NULL, size);
  if (rv == NULL) return NULL;

  if (!(cp = PyString_AsString(rv))) {
    Py_DECREF(rv);
    return NULL;
  }

  if ((count = read(self->x_fd, cp, size)) < 0) {
    PyErr_SetFromErrno(LinuxAudioError);
    Py_DECREF(rv);
    return NULL;
  }

  self->x_icount += count;
  return rv;
}

static PyObject *
lad_write(lad_t *self, PyObject *args)
{
  char *cp;
  int rv, size;
	
  if (!PyArg_ParseTuple(args, "s#", &cp, &size)) return NULL;

  while (size > 0) {
    if ((rv = write(self->x_fd, cp, size)) < 0) {
      PyErr_SetFromErrno(LinuxAudioError);
      return NULL;
    }
    self->x_ocount += rv;
    size           -= rv;
    cp             += rv;
  }
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
lad_close(lad_t *self, PyObject *args)
{
  if (!PyArg_ParseTuple(args, "")) return NULL;
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
  if (!PyArg_ParseTuple(args, "")) return NULL;
  return PyInt_FromLong(self->x_fd);
}

static PyObject *
lad_setparameters(lad_t *self, PyObject *args)
{
  int rate, ssize, nchannels, stereo, n, fmt;

  if (!PyArg_ParseTuple(args, "iiii", &rate, &ssize, &nchannels, &fmt))
    return NULL;
  
  if (rate < 0 || ssize < 0 || (nchannels != 1 && nchannels != 2)) {
    PyErr_SetFromErrno(LinuxAudioError);
    return NULL;
  }

  if (ioctl(self->x_fd, SOUND_PCM_WRITE_RATE, &rate) < 0) {
    PyErr_SetFromErrno(LinuxAudioError);
    return NULL;
  }

  if (ioctl(self->x_fd, SNDCTL_DSP_SAMPLESIZE, &ssize) < 0) {
    PyErr_SetFromErrno(LinuxAudioError);
    return NULL;
  }

  stereo = (nchannels == 1)? 0: (nchannels == 2)? 1: -1;
  if (ioctl(self->x_fd, SNDCTL_DSP_STEREO, &stereo) < 0) {
    PyErr_SetFromErrno(LinuxAudioError);
    return NULL;
  }

  for (n = 0; n != sizeof(audio_types) / sizeof(audio_types[0]); n++)
    if (fmt == audio_types[n].a_fmt)
      break;

  if (n == sizeof(audio_types) / sizeof(audio_types[0]) ||
      audio_types[n].a_bps != ssize ||
      (self->x_afmts & audio_types[n].a_fmt) == 0) {
    PyErr_SetFromErrno(LinuxAudioError);
    return NULL;
  }

  if (ioctl(self->x_fd, SNDCTL_DSP_SETFMT, &audio_types[n].a_fmt) < 0) {
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

  if (!PyArg_ParseTuple(args, "")) return NULL;

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

  if (!PyArg_ParseTuple(args, "")) return NULL;

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

  if (!PyArg_ParseTuple(args, "")) return NULL;

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

/* Flush the device */
static PyObject *
lad_flush(lad_t *self, PyObject *args)
{
  if (!PyArg_ParseTuple(args, "")) return NULL;

  if (ioctl(self->x_fd, SNDCTL_DSP_SYNC, NULL) < 0) {
    PyErr_SetFromErrno(LinuxAudioError);
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}

static PyMethodDef lad_methods[] = {
  { "read",		(PyCFunction)lad_read, 1 },
  { "write",		(PyCFunction)lad_write, 1 },
  { "setparameters",	(PyCFunction)lad_setparameters, 1 },
  { "bufsize",		(PyCFunction)lad_bufsize, 1 },
  { "obufcount",	(PyCFunction)lad_obufcount, 1 },
  { "obuffree",		(PyCFunction)lad_obuffree, 1 },
  { "flush",		(PyCFunction)lad_flush, 1 },
  { "close",		(PyCFunction)lad_close, 1 },
  { "fileno",     	(PyCFunction)lad_fileno, 1 },
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
  "linux_audio_device",		/*tp_name*/
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
  { "open", ladopen, 1 },
  { 0, 0 },
};

static int
ins(PyObject *d, char *symbol, long value)
{
  PyObject* v = PyInt_FromLong(value);
  if (!v || PyDict_SetItemString(d, symbol, v) < 0)
    return -1;                   /* triggers fatal error */

  Py_DECREF(v);
  return 0;
}
void
initlinuxaudiodev()
{
  PyObject *m, *d, *x;
  
  m = Py_InitModule("linuxaudiodev", linuxaudiodev_methods);
  d = PyModule_GetDict(m);

  LinuxAudioError = PyErr_NewException("linuxaudiodev.error", NULL, NULL);
  if (LinuxAudioError)
    PyDict_SetItemString(d, "error", LinuxAudioError);

  x = PyInt_FromLong((long) AFMT_MU_LAW);
  if (x == NULL || PyDict_SetItemString(d, "AFMT_MU_LAW", x) < 0)
    goto error;
  Py_DECREF(x);

  x = PyInt_FromLong((long) AFMT_U8);
  if (x == NULL || PyDict_SetItemString(d, "AFMT_U8", x) < 0)
    goto error;
  Py_DECREF(x);

  x = PyInt_FromLong((long) AFMT_S8);
  if (x == NULL || PyDict_SetItemString(d, "AFMT_S8", x) < 0)
    goto error;
  Py_DECREF(x);

  x = PyInt_FromLong((long) AFMT_U16_BE);
  if (x == NULL || PyDict_SetItemString(d, "AFMT_U16_BE", x) < 0)
    goto error;
  Py_DECREF(x);

  x = PyInt_FromLong((long) AFMT_U16_LE);
  if (x == NULL || PyDict_SetItemString(d, "AFMT_U16_LE", x) < 0)
    goto error;
  Py_DECREF(x);

  x = PyInt_FromLong((long) AFMT_S16_BE);
  if (x == NULL || PyDict_SetItemString(d, "AFMT_S16_BE", x) < 0)
    goto error;
  Py_DECREF(x);

  x = PyInt_FromLong((long) AFMT_S16_LE);
  if (x == NULL || PyDict_SetItemString(d, "AFMT_S16_LE", x) < 0)
    goto error;
  Py_DECREF(x);

  /* Check for errors */
  if (PyErr_Occurred()) {
  error:
    Py_FatalError("can't initialize module linuxaudiodev");
  }
}
