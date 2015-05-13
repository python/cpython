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
"MessageBeep(x) - Call Windows MessageBeep.");

/*[clinic input]
module winsound
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=a18401142d97b8d5]*/

#include "clinic/winsound.c.h"

/*[clinic input]
winsound.PlaySound

    sound: Py_UNICODE(accept={str, NoneType})
        The sound to play; a filename, data, or None.
    flags: int
        Flag values, ored together.  See module documentation.
    /

A wrapper around the Windows PlaySound API.
[clinic start generated code]*/

static PyObject *
winsound_PlaySound_impl(PyModuleDef *module, Py_UNICODE *sound, int flags)
/*[clinic end generated code: output=614273784bf59e5c input=3411b1b7c1f36d93]*/
{
    int ok;

    if (flags & SND_ASYNC && flags & SND_MEMORY) {
        /* Sidestep reference counting headache; unfortunately this also
            prevent SND_LOOP from memory. */
        PyErr_SetString(PyExc_RuntimeError,
                        "Cannot play asynchronously from memory");
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    ok = PlaySoundW(sound, NULL, flags);
    Py_END_ALLOW_THREADS
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
    /

A wrapper around the Windows Beep API.
[clinic start generated code]*/

static PyObject *
winsound_Beep_impl(PyModuleDef *module, int frequency, int duration)
/*[clinic end generated code: output=c75f282035a872bd input=628a99d2ddf73798]*/
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

    x: int(c_default="MB_OK") = MB_OK
    /

Call Windows MessageBeep(x).

x defaults to MB_OK.
[clinic start generated code]*/

static PyObject *
winsound_MessageBeep_impl(PyModuleDef *module, int x)
/*[clinic end generated code: output=92aa6a822bdc66ad input=a776c8a85c9853f6]*/
{
    MessageBeep(x);
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
