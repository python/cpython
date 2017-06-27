/* Author: Toby Dickenson <htrd90@zepler.org>
 *
 * Copyright (c) 1999 Toby Dickenson
 *
 * Permission to use this software in any way is granted without
 * fee, provided that the copyright notice above appears in all
 * copies. This software is provided "as is" without any warranty.
 */

/* Modified by Guido van Rossum */
/* Beep added by Mark Hammond */
/* Win9X Beep and platform identification added by Uncle Timmy */

/* Example:

   import winsound
   import time

   # Play wav file
   winsound.PlaySound('c:/windows/media/Chord.wav', winsound.SND_FILENAME)

   # Play sound from control panel settings
   winsound.PlaySound('SystemQuestion', winsound.SND_ALIAS)

   # Play wav file from memory
   data=open('c:/windows/media/Chimes.wav',"rb").read()
   winsound.PlaySound(data, winsound.SND_MEMORY)

   # Start playing the first bit of wav file asynchronously
   winsound.PlaySound('c:/windows/media/Chord.wav',
                   winsound.SND_FILENAME|winsound.SND_ASYNC)
   # But dont let it go for too long...
   time.sleep(0.1)
   # ...Before stopping it
   winsound.PlaySound(None, 0)
*/

#include <Python.h>
#include <windows.h>
#include <mmsystem.h>

PyDoc_STRVAR(sound_module_doc,
"PlaySound(sound, flags) - play a sound\n"
"SND_FILENAME - sound is a wav file name\n"
"SND_ALIAS - sound is a registry sound association name\n"
"SND_LOOP - Play the sound repeatedly; must also specify SND_ASYNC\n"
"SND_MEMORY - sound is a memory image of a wav file\n"
"SND_PURGE - stop all instances of the specified sound\n"
"SND_ASYNC - PlaySound returns immediately\n"
"SND_NODEFAULT - Do not play a default beep if the sound can not be found\n"
"SND_NOSTOP - Do not interrupt any sounds currently playing\n"  // Raising RuntimeError if needed
"SND_NOWAIT - Return immediately if the sound driver is busy\n" // Without any errors
"\n"
"Beep(frequency, duration) - Make a beep through the PC speaker.\n"
"MessageBeep(type) - Call Windows MessageBeep.");

/*[clinic input]
module winsound
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=a18401142d97b8d5]*/

#include "clinic/winsound.c.h"

/*[clinic input]
winsound.PlaySound

    sound: object
        The sound to play; a filename, data, or None.
    flags: int
        Flag values, ored together.  See module documentation.

A wrapper around the Windows PlaySound API.
[clinic start generated code]*/

static PyObject *
winsound_PlaySound_impl(PyObject *module, PyObject *sound, int flags)
/*[clinic end generated code: output=49a0fd16a372ebeb input=c63e1f2d848da2f2]*/
{
    int ok;
    wchar_t *wsound;
    Py_buffer view = {NULL, NULL};

    if (sound == Py_None) {
        wsound = NULL;
    } else if (flags & SND_MEMORY) {
        if (flags & SND_ASYNC) {
            /* Sidestep reference counting headache; unfortunately this also
                prevent SND_LOOP from memory. */
            PyErr_SetString(PyExc_RuntimeError,
                            "Cannot play asynchronously from memory");
            return NULL;
        }
        if (PyObject_GetBuffer(sound, &view, PyBUF_SIMPLE) < 0) {
            return NULL;
        }
        wsound = (wchar_t *)view.buf;
    } else {
        if (!PyUnicode_Check(sound)) {
            PyErr_Format(PyExc_TypeError,
                         "'sound' must be str or None, not '%s'",
                         Py_TYPE(sound)->tp_name);
            return NULL;
        }
        wsound = _PyUnicode_AsWideCharString(sound);
        if (wsound == NULL) {
            return NULL;
        }
    }


    Py_BEGIN_ALLOW_THREADS
    ok = PlaySoundW(wsound, NULL, flags);
    Py_END_ALLOW_THREADS
    if (view.obj) {
        PyBuffer_Release(&view);
    } else if (sound != Py_None) {
        PyMem_Free(wsound);
    }
    if (!ok) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to play sound");
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
winsound.Beep

    frequency: int
        Frequency of the sound in hertz.
        Must be in the range 37 through 32,767.
    duration: int
        How long the sound should play, in milliseconds.

A wrapper around the Windows Beep API.
[clinic start generated code]*/

static PyObject *
winsound_Beep_impl(PyObject *module, int frequency, int duration)
/*[clinic end generated code: output=f32382e52ee9b2fb input=40e360cfa00a5cf0]*/
{
    BOOL ok;

    if (frequency < 37 || frequency > 32767) {
        PyErr_SetString(PyExc_ValueError,
                        "frequency must be in 37 thru 32767");
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    ok = Beep(frequency, duration);
    Py_END_ALLOW_THREADS
    if (!ok) {
        PyErr_SetString(PyExc_RuntimeError,"Failed to beep");
        return NULL;
    }

    Py_RETURN_NONE;
}

/*[clinic input]
winsound.MessageBeep

    type: int(c_default="MB_OK") = MB_OK

Call Windows MessageBeep(x).

x defaults to MB_OK.
[clinic start generated code]*/

static PyObject *
winsound_MessageBeep_impl(PyObject *module, int type)
/*[clinic end generated code: output=120875455121121f input=db185f741ae21401]*/
{
    BOOL ok;

    Py_BEGIN_ALLOW_THREADS
    ok = MessageBeep(type);
    Py_END_ALLOW_THREADS

    if (!ok) {
        PyErr_SetExcFromWindowsErr(PyExc_RuntimeError, 0);
        return NULL;
    }

    Py_RETURN_NONE;
}

static struct PyMethodDef sound_methods[] =
{
    WINSOUND_PLAYSOUND_METHODDEF
    WINSOUND_BEEP_METHODDEF
    WINSOUND_MESSAGEBEEP_METHODDEF
    {NULL,  NULL}
};

static void
add_define(PyObject *dict, const char *key, long value)
{
    PyObject *k = PyUnicode_FromString(key);
    PyObject *v = PyLong_FromLong(value);
    if (v && k) {
        PyDict_SetItem(dict, k, v);
    }
    Py_XDECREF(k);
    Py_XDECREF(v);
}

#define ADD_DEFINE(tok) add_define(dict,#tok,tok)


static struct PyModuleDef winsoundmodule = {
    PyModuleDef_HEAD_INIT,
    "winsound",
    sound_module_doc,
    -1,
    sound_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_winsound(void)
{
    PyObject *dict;
    PyObject *module = PyModule_Create(&winsoundmodule);
    if (module == NULL)
        return NULL;
    dict = PyModule_GetDict(module);

    ADD_DEFINE(SND_ASYNC);
    ADD_DEFINE(SND_NODEFAULT);
    ADD_DEFINE(SND_NOSTOP);
    ADD_DEFINE(SND_NOWAIT);
    ADD_DEFINE(SND_ALIAS);
    ADD_DEFINE(SND_FILENAME);
    ADD_DEFINE(SND_MEMORY);
    ADD_DEFINE(SND_PURGE);
    ADD_DEFINE(SND_LOOP);
    ADD_DEFINE(SND_APPLICATION);

    ADD_DEFINE(MB_OK);
    ADD_DEFINE(MB_ICONASTERISK);
    ADD_DEFINE(MB_ICONEXCLAMATION);
    ADD_DEFINE(MB_ICONHAND);
    ADD_DEFINE(MB_ICONQUESTION);
    return module;
}
