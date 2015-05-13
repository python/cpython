/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(winsound_PlaySound__doc__,
"PlaySound($module, sound, flags, /)\n"
"--\n"
"\n"
"A wrapper around the Windows PlaySound API.\n"
"\n"
"  sound\n"
"    The sound to play; a filename, data, or None.\n"
"  flags\n"
"    Flag values, ored together.  See module documentation.");

#define WINSOUND_PLAYSOUND_METHODDEF    \
    {"PlaySound", (PyCFunction)winsound_PlaySound, METH_VARARGS, winsound_PlaySound__doc__},

static PyObject *
winsound_PlaySound_impl(PyModuleDef *module, Py_UNICODE *sound, int flags);

static PyObject *
winsound_PlaySound(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_UNICODE *sound;
    int flags;

    if (!PyArg_ParseTuple(args, "Zi:PlaySound",
        &sound, &flags))
        goto exit;
    return_value = winsound_PlaySound_impl(module, sound, flags);

exit:
    return return_value;
}

PyDoc_STRVAR(winsound_Beep__doc__,
"Beep($module, frequency, duration, /)\n"
"--\n"
"\n"
"A wrapper around the Windows Beep API.\n"
"\n"
"  frequency\n"
"    Frequency of the sound in hertz.\n"
"    Must be in the range 37 through 32,767.\n"
"  duration\n"
"    How long the sound should play, in milliseconds.");

#define WINSOUND_BEEP_METHODDEF    \
    {"Beep", (PyCFunction)winsound_Beep, METH_VARARGS, winsound_Beep__doc__},

static PyObject *
winsound_Beep_impl(PyModuleDef *module, int frequency, int duration);

static PyObject *
winsound_Beep(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int frequency;
    int duration;

    if (!PyArg_ParseTuple(args, "ii:Beep",
        &frequency, &duration))
        goto exit;
    return_value = winsound_Beep_impl(module, frequency, duration);

exit:
    return return_value;
}

PyDoc_STRVAR(winsound_MessageBeep__doc__,
"MessageBeep($module, x=MB_OK, /)\n"
"--\n"
"\n"
"Call Windows MessageBeep(x).\n"
"\n"
"x defaults to MB_OK.");

#define WINSOUND_MESSAGEBEEP_METHODDEF    \
    {"MessageBeep", (PyCFunction)winsound_MessageBeep, METH_VARARGS, winsound_MessageBeep__doc__},

static PyObject *
winsound_MessageBeep_impl(PyModuleDef *module, int x);

static PyObject *
winsound_MessageBeep(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int x = MB_OK;

    if (!PyArg_ParseTuple(args, "|i:MessageBeep",
        &x))
        goto exit;
    return_value = winsound_MessageBeep_impl(module, x);

exit:
    return return_value;
}
/*[clinic end generated code: output=c5b018ac9dc1f500 input=a9049054013a1b77]*/
