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
   # But don't let it go for too long...
   time.sleep(0.1)
   # ...Before stopping it
   winsound.PlaySound(None, 0)
*/

// Need limited C API version 3.13 for Py_mod_gil
#include "pyconfig.h"  // Py_GIL_DISABLED
#ifndef Py_GIL_DISABLED
#  define Py_LIMITED_API 0x030d0000
#endif

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
"SND_APPLICATION - sound is an application-specific alias in the registry."
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
    } else if (PyBytes_Check(sound)) {
        PyObject *type_name = PyType_GetQualName(Py_TYPE(sound));
        if (type_name != NULL) {
            PyErr_Format(PyExc_TypeError,
                         "'sound' must be str, os.PathLike, or None, not %S",
                         type_name);
            Py_DECREF(type_name);
        }
        return NULL;
    } else {
        PyObject *obj = PyOS_FSPath(sound);
        // Either <obj> is unicode/bytes/NULL, or a helpful message
        // has been surfaced to the user about how they gave a non-path.
        if (obj == NULL) return NULL;
        if (PyBytes_Check(obj)) {
            PyErr_Format(PyExc_TypeError,
                         "'sound' must resolve to str, not bytes");
            Py_DECREF(obj);
            return NULL;
        }
        wsound = PyUnicode_AsWideCharString(obj, NULL);
        Py_DECREF(obj);
        if (wsound == NULL) return NULL;
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

#define ADD_DEFINE(CONST) do {                                  \
    if (PyModule_AddIntConstant(module, #CONST, CONST) < 0) {   \
        return -1;                                              \
    }                                                           \
} while (0)

static int
exec_module(PyObject *module)
{
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

#undef ADD_DEFINE

    return 0;
}

static PyModuleDef_Slot sound_slots[] = {
    {Py_mod_exec, exec_module},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef winsoundmodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "winsound",
    .m_doc = sound_module_doc,
    .m_methods = sound_methods,
    .m_slots = sound_slots,
};

PyMODINIT_FUNC
PyInit_winsound(void)
{
    return PyModuleDef_Init(&winsoundmodule);
}
