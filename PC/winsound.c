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

#include <windows.h>
#include <mmsystem.h>
#include <Python.h>

static char sound_playsound_doc[] =
"PlaySound(sound, flags) - a wrapper around the Windows PlaySound API\n"
"\n"
"The sound argument can be a filename, data, or None.\n"
"For flag values, ored together, see module documentation.\n";

static char sound_beep_doc[] =
"Beep(frequency, duration) - a wrapper around the Windows Beep API\n"
"\n"
"The frequency argument specifies frequency, in hertz, of the sound.\n"
"This parameter must be in the range 37 through 32,767 (0x25 through 0x7FFF).\n"
"The duration argument specifies the number of milli-seconds.\n";

static char sound_module_doc[] =
"PlaySound(sound, flags) - play a sound\n"
"SND_FILENAME - sound is a wav file name\n"
"SND_ALIAS - sound is a control panel sound association name\n"
"SND_LOOP - Play the sound repeatedly; must also specify SND_ASYNC\n"
"SND_MEMORY - sound is a memory image of a wav file\n"
"SND_PURGE - stop all instances of the specified sound\n"
"SND_ASYNC - PlaySound returns immediately\n"
"SND_NODEFAULT - Do not play a default beep if the sound can not be found\n"
"SND_NOSTOP - Do not interrupt any sounds currently playing\n"  // Raising RuntimeError if needed
"SND_NOWAIT - Return immediately if the sound driver is busy\n" // Without any errors
"\n"
"Beep(frequency, duration) - Make a beep through the PC speaker.\n";

PyObject *sound_playsound(PyObject *s, PyObject *args)
{
    const char *sound;
    int flags;
    int length;
    int ok;

    if(!PyArg_ParseTuple(args,"z#i:PlaySound",&sound,&length,&flags))
    {
        return NULL;
    }

    if(flags&SND_ASYNC && flags &SND_MEMORY)
    {
	/* Sidestep reference counting headache; unfortunately this also
	   prevent SND_LOOP from memory. */
        PyErr_SetString(PyExc_RuntimeError,"Cannot play asynchronously from memory");
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    ok = PlaySound(sound,NULL,flags);
    Py_END_ALLOW_THREADS
    if(!ok)
    {
        PyErr_SetString(PyExc_RuntimeError,"Failed to play sound");
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *sound_beep( PyObject *self, PyObject *args )
{
	int freq;
	int dur;
	BOOL ok;

	if (!PyArg_ParseTuple(args, "ii:Beep", &freq,  &dur))
		return NULL;
    Py_BEGIN_ALLOW_THREADS
	ok = Beep(freq,dur);
    Py_END_ALLOW_THREADS
    if(!ok)
    {
        PyErr_SetString(PyExc_RuntimeError,"Failed to beep");
        return NULL;
    }
	Py_INCREF(Py_None);
	return Py_None;
}

static struct PyMethodDef sound_methods[] =
{
    {"PlaySound", sound_playsound, 1, sound_playsound_doc},
    {"Beep",      sound_beep,      1, sound_beep_doc},
    {NULL,  NULL}
};

static void add_define(PyObject *dict, const char *key, long value)
{
    PyObject *k=PyString_FromString(key);
    PyObject *v=PyLong_FromLong(value);
    if(v&&k)
    {
        PyDict_SetItem(dict,k,v);
    }
    Py_XDECREF(k);
    Py_XDECREF(v);
}

#define ADD_DEFINE(tok) add_define(dict,#tok,tok)

DL_EXPORT(void)
initwinsound(void)
{
    PyObject *module=Py_InitModule3("winsound", sound_methods, sound_module_doc);
    PyObject *dict=PyModule_GetDict(module);

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
}
