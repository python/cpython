/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(make_heaptype_with_member__doc__,
"make_heaptype_with_member($module, /, extra_base_size=0, basicsize=0,\n"
"                          member_offset=0, add_relative_flag=False, *,\n"
"                          member_name=\'memb\', member_flags=0,\n"
"                          member_type=-1)\n"
"--\n"
"\n");

#define MAKE_HEAPTYPE_WITH_MEMBER_METHODDEF    \
    {"make_heaptype_with_member", (PyCFunction)(void(*)(void))make_heaptype_with_member, METH_VARARGS|METH_KEYWORDS, make_heaptype_with_member__doc__},

static PyObject *
make_heaptype_with_member_impl(PyObject *module, int extra_base_size,
                               int basicsize, int member_offset,
                               int add_relative_flag,
                               const char *member_name, int member_flags,
                               int member_type);

static PyObject *
make_heaptype_with_member(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"extra_base_size", "basicsize", "member_offset", "add_relative_flag", "member_name", "member_flags", "member_type", NULL};
    int extra_base_size = 0;
    int basicsize = 0;
    int member_offset = 0;
    int add_relative_flag = 0;
    const char *member_name = "memb";
    int member_flags = 0;
    int member_type = Py_T_BYTE;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|iiip$sii:make_heaptype_with_member", _keywords,
        &extra_base_size, &basicsize, &member_offset, &add_relative_flag, &member_name, &member_flags, &member_type))
        goto exit;
    return_value = make_heaptype_with_member_impl(module, extra_base_size, basicsize, member_offset, add_relative_flag, member_name, member_flags, member_type);

exit:
    return return_value;
}
/*[clinic end generated code: output=01933185947faecc input=a9049054013a1b77]*/
