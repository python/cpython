/*
 * SOUND
 * $Id$
 *
 * play sound on a windows platform
 *
 * history:
 * 99-02-02 fl  created
 *
 * Copyright (c) 1999 by Secret Labs AB.
 * Copyright (c) 1999 by Fredrik Lundh.
 *
 * fredrik@pythonware.com
 * http://www.pythonware.com
 *
 * --------------------------------------------------------------------
 * Permission to use, copy, modify, and distribute this software and
 * its associated documentation for any purpose and without fee is
 * hereby granted.  This software is provided as is.
 * --------------------------------------------------------------------
 */

#include "Python.h"

#include "windows.h"
#include "mmsystem.h"

static PyObject*
_play(PyObject* self, PyObject* args)
{
    int result;
    
    /* first argument is a string that should contain the contents of
       a WAV file.  second argument should be non-zero to wait for the
       sound to complete. */

    char *data = NULL;
    int bytes;
    int wait = 0;
    if (!PyArg_ParseTuple(args, "|s#i", &data, &bytes, &wait))
	return NULL;

    result = PlaySound(data, NULL, (SND_MEMORY|SND_NOWAIT|SND_NODEFAULT) |
                       (wait ? 0 : SND_ASYNC));

    return Py_BuildValue("i", result);
}

static PyObject*
_stop(PyObject* self, PyObject* args)
{
    if (!PyArg_NoArgs(args))
	return NULL;

    return Py_BuildValue("i", PlaySound(NULL, NULL, 0));
}

static PyMethodDef _functions[] = {
    {"play", _play, 1},
    {"stop", _stop, 0},
    {NULL, NULL}
};

void
#ifdef WIN32
__declspec(dllexport)
#endif
initwinsound()
{
    Py_InitModule("winsound", _functions);
}
