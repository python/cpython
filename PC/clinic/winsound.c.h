/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(winsound_PlaySound__doc__,
"PlaySound($module, /, sound, flags)\n"
"--\n"
"\n"
"A wrapper around the Windows PlaySound API.\n"
"\n"
"  sound\n"
"    The sound to play; a filename, data, or None.\n"
"  flags\n"
"    Flag values, ored together.  See module documentation.");

#define WINSOUND_PLAYSOUND_METHODDEF    \
    {"PlaySound", (PyCFunction)winsound_PlaySound, METH_FASTCALL|METH_KEYWORDS, winsound_PlaySound__doc__},

static PyObject *
winsound_PlaySound_impl(PyObject *module, PyObject *sound, int flags);

static PyObject *
winsound_PlaySound(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"sound", "flags", NULL};
    static _PyArg_Parser _parser = {"Oi:PlaySound", _keywords, 0};
    PyObject *sound;
    int flags;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &sound, &flags)) {
        goto exit;
    }
    return_value = winsound_PlaySound_impl(module, sound, flags);

exit:
    return return_value;
}

PyDoc_STRVAR(winsound_Beep__doc__,
"Beep($module, /, frequency, duration)\n"
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
    {"Beep", (PyCFunction)winsound_Beep, METH_FASTCALL|METH_KEYWORDS, winsound_Beep__doc__},

static PyObject *
winsound_Beep_impl(PyObject *module, int frequency, int duration);

static PyObject *
winsound_Beep(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"frequency", "duration", NULL};
    static _PyArg_Parser _parser = {"ii:Beep", _keywords, 0};
    int frequency;
    int duration;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &frequency, &duration)) {
        goto exit;
    }
    return_value = winsound_Beep_impl(module, frequency, duration);

exit:
    return return_value;
}

PyDoc_STRVAR(winsound_MessageBeep__doc__,
"MessageBeep($module, /, type=MB_OK)\n"
"--\n"
"\n"
"Call Windows MessageBeep(x).\n"
"\n"
"x defaults to MB_OK.");

#define WINSOUND_MESSAGEBEEP_METHODDEF    \
    {"MessageBeep", (PyCFunction)winsound_MessageBeep, METH_FASTCALL|METH_KEYWORDS, winsound_MessageBeep__doc__},

static PyObject *
winsound_MessageBeep_impl(PyObject *module, int type);

static PyObject *
winsound_MessageBeep(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"type", NULL};
    static _PyArg_Parser _parser = {"|i:MessageBeep", _keywords, 0};
    int type = MB_OK;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &type)) {
        goto exit;
    }
    return_value = winsound_MessageBeep_impl(module, type);

exit:
    return return_value;
}
/*[clinic end generated code: output=beeee8be95667b7d input=a9049054013a1b77]*/
