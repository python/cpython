/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(unicodedata_UCD_decimal__doc__,
"decimal($self, unichr, default=None, /)\n"
"--\n"
"\n"
"Converts a Unicode character into its equivalent decimal value.\n"
"\n"
"Returns the decimal value assigned to the Unicode character unichr\n"
"as integer. If no such value is defined, default is returned, or, if\n"
"not given, ValueError is raised.");

#define UNICODEDATA_UCD_DECIMAL_METHODDEF    \
    {"decimal", (PyCFunction)unicodedata_UCD_decimal, METH_VARARGS, unicodedata_UCD_decimal__doc__},

static PyObject *
unicodedata_UCD_decimal_impl(PreviousDBVersion *self,
                             PyUnicodeObject *unichr,
                             PyObject *default_value);

static PyObject *
unicodedata_UCD_decimal(PreviousDBVersion *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyUnicodeObject *unichr;
    PyObject *default_value = NULL;

    if (!PyArg_ParseTuple(args,
        "O!|O:decimal",
        &PyUnicode_Type, &unichr, &default_value))
        goto exit;
    return_value = unicodedata_UCD_decimal_impl(self, unichr, default_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=33b488251c4fd143 input=a9049054013a1b77]*/
