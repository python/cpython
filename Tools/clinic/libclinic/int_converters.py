from types import NoneType

from libclinic import fail, unspecified, unknown, TypeSet
from libclinic.converter import CConverter, add_legacy_c_converter


class bool_converter(CConverter):
    type = 'int'
    default_type = bool
    format_unit = 'p'
    c_ignored_default = '0'

    def converter_init(self, *, accept: TypeSet = {object}) -> None:
        if accept == {int}:
            self.format_unit = 'i'
        elif accept != {object}:
            fail(f"bool_converter: illegal 'accept' argument {accept!r}")
        if self.default is not unspecified and self.default is not unknown:
            self.default = bool(self.default)
            self.c_default = str(int(self.default))

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 'i':
            return self.format_code("""
                {paramname} = PyLong_AsInt({argname});
                if ({paramname} == -1 && PyErr_Occurred()) {{{{
                    goto exit;
                }}}}
                """,
                argname=argname)
        elif self.format_unit == 'p':
            return self.format_code("""
                {paramname} = PyObject_IsTrue({argname});
                if ({paramname} < 0) {{{{
                    goto exit;
                }}}}
                """,
                argname=argname)
        return super().parse_arg(argname, displayname, limited_capi=limited_capi)


class char_converter(CConverter):
    type = 'char'
    default_type = (bytes, bytearray)
    format_unit = 'c'
    c_ignored_default = "'\0'"

    def converter_init(self) -> None:
        if isinstance(self.default, self.default_type):
            if len(self.default) != 1:
                fail(f"char_converter: illegal default value {self.default!r}")

            self.c_default = repr(bytes(self.default))[1:]
            if self.c_default == '"\'"':
                self.c_default = r"'\''"

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 'c':
            return self.format_code("""
                if (PyBytes_Check({argname}) && PyBytes_GET_SIZE({argname}) == 1) {{{{
                    {paramname} = PyBytes_AS_STRING({argname})[0];
                }}}}
                else if (PyByteArray_Check({argname}) && PyByteArray_GET_SIZE({argname}) == 1) {{{{
                    {paramname} = PyByteArray_AS_STRING({argname})[0];
                }}}}
                else {{{{
                    {bad_argument}
                    goto exit;
                }}}}
                """,
                argname=argname,
                bad_argument=self.bad_argument(displayname, 'a byte string of length 1', limited_capi=limited_capi),
            )
        return super().parse_arg(argname, displayname, limited_capi=limited_capi)


@add_legacy_c_converter('B', bitwise=True)
class unsigned_char_converter(CConverter):
    type = 'unsigned char'
    default_type = int
    format_unit = 'b'
    c_ignored_default = "'\0'"

    def converter_init(self, *, bitwise: bool = False) -> None:
        if bitwise:
            self.format_unit = 'B'

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 'b':
            return self.format_code("""
                {{{{
                    long ival = PyLong_AsLong({argname});
                    if (ival == -1 && PyErr_Occurred()) {{{{
                        goto exit;
                    }}}}
                    else if (ival < 0) {{{{
                        PyErr_SetString(PyExc_OverflowError,
                                        "unsigned byte integer is less than minimum");
                        goto exit;
                    }}}}
                    else if (ival > UCHAR_MAX) {{{{
                        PyErr_SetString(PyExc_OverflowError,
                                        "unsigned byte integer is greater than maximum");
                        goto exit;
                    }}}}
                    else {{{{
                        {paramname} = (unsigned char) ival;
                    }}}}
                }}}}
                """,
                argname=argname)
        elif self.format_unit == 'B':
            return self.format_code("""
                {{{{
                    unsigned long ival = PyLong_AsUnsignedLongMask({argname});
                    if (ival == (unsigned long)-1 && PyErr_Occurred()) {{{{
                        goto exit;
                    }}}}
                    else {{{{
                        {paramname} = (unsigned char) ival;
                    }}}}
                }}}}
                """,
                argname=argname)
        return super().parse_arg(argname, displayname, limited_capi=limited_capi)


class byte_converter(unsigned_char_converter):
    pass


class short_converter(CConverter):
    type = 'short'
    default_type = int
    format_unit = 'h'
    c_ignored_default = "0"

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 'h':
            return self.format_code("""
                {{{{
                    long ival = PyLong_AsLong({argname});
                    if (ival == -1 && PyErr_Occurred()) {{{{
                        goto exit;
                    }}}}
                    else if (ival < SHRT_MIN) {{{{
                        PyErr_SetString(PyExc_OverflowError,
                                        "signed short integer is less than minimum");
                        goto exit;
                    }}}}
                    else if (ival > SHRT_MAX) {{{{
                        PyErr_SetString(PyExc_OverflowError,
                                        "signed short integer is greater than maximum");
                        goto exit;
                    }}}}
                    else {{{{
                        {paramname} = (short) ival;
                    }}}}
                }}}}
                """,
                argname=argname)
        return super().parse_arg(argname, displayname, limited_capi=limited_capi)


class unsigned_short_converter(CConverter):
    type = 'unsigned short'
    default_type = int
    c_ignored_default = "0"

    def converter_init(self, *, bitwise: bool = False) -> None:
        if bitwise:
            self.format_unit = 'H'
        else:
            self.converter = '_PyLong_UnsignedShort_Converter'

    def use_converter(self) -> None:
        if self.converter == '_PyLong_UnsignedShort_Converter':
            self.add_include('pycore_long.h',
                             '_PyLong_UnsignedShort_Converter()')

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 'H':
            return self.format_code("""
                {paramname} = (unsigned short)PyLong_AsUnsignedLongMask({argname});
                if ({paramname} == (unsigned short)-1 && PyErr_Occurred()) {{{{
                    goto exit;
                }}}}
                """,
                argname=argname)
        if not limited_capi:
            return super().parse_arg(argname, displayname, limited_capi=limited_capi)
        # NOTE: Raises OverflowError for negative integer.
        return self.format_code("""
            {{{{
                unsigned long uval = PyLong_AsUnsignedLong({argname});
                if (uval == (unsigned long)-1 && PyErr_Occurred()) {{{{
                    goto exit;
                }}}}
                if (uval > USHRT_MAX) {{{{
                    PyErr_SetString(PyExc_OverflowError,
                                    "Python int too large for C unsigned short");
                    goto exit;
                }}}}
                {paramname} = (unsigned short) uval;
            }}}}
            """,
            argname=argname)


@add_legacy_c_converter('C', accept={str})
class int_converter(CConverter):
    type = 'int'
    default_type = int
    format_unit = 'i'
    c_ignored_default = "0"

    def converter_init(
        self, *, accept: TypeSet = {int}, type: str | None = None
    ) -> None:
        if accept == {str}:
            self.format_unit = 'C'
        elif accept != {int}:
            fail(f"int_converter: illegal 'accept' argument {accept!r}")
        if type is not None:
            self.type = type

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 'i':
            return self.format_code("""
                {paramname} = PyLong_AsInt({argname});
                if ({paramname} == -1 && PyErr_Occurred()) {{{{
                    goto exit;
                }}}}
                """,
                argname=argname)
        elif self.format_unit == 'C':
            return self.format_code("""
                if (!PyUnicode_Check({argname})) {{{{
                    {bad_argument}
                    goto exit;
                }}}}
                if (PyUnicode_GET_LENGTH({argname}) != 1) {{{{
                    {bad_argument}
                    goto exit;
                }}}}
                {paramname} = PyUnicode_READ_CHAR({argname}, 0);
                """,
                argname=argname,
                bad_argument=self.bad_argument(displayname, 'a unicode character', limited_capi=limited_capi),
            )
        return super().parse_arg(argname, displayname, limited_capi=limited_capi)


class unsigned_int_converter(CConverter):
    type = 'unsigned int'
    default_type = int
    c_ignored_default = "0"

    def converter_init(self, *, bitwise: bool = False) -> None:
        if bitwise:
            self.format_unit = 'I'
        else:
            self.converter = '_PyLong_UnsignedInt_Converter'

    def use_converter(self) -> None:
        if self.converter == '_PyLong_UnsignedInt_Converter':
            self.add_include('pycore_long.h',
                             '_PyLong_UnsignedInt_Converter()')

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 'I':
            return self.format_code("""
                {paramname} = (unsigned int)PyLong_AsUnsignedLongMask({argname});
                if ({paramname} == (unsigned int)-1 && PyErr_Occurred()) {{{{
                    goto exit;
                }}}}
                """,
                argname=argname)
        if not limited_capi:
            return super().parse_arg(argname, displayname, limited_capi=limited_capi)
        # NOTE: Raises OverflowError for negative integer.
        return self.format_code("""
            {{{{
                unsigned long uval = PyLong_AsUnsignedLong({argname});
                if (uval == (unsigned long)-1 && PyErr_Occurred()) {{{{
                    goto exit;
                }}}}
                if (uval > UINT_MAX) {{{{
                    PyErr_SetString(PyExc_OverflowError,
                                    "Python int too large for C unsigned int");
                    goto exit;
                }}}}
                {paramname} = (unsigned int) uval;
            }}}}
            """,
            argname=argname)


class long_converter(CConverter):
    type = 'long'
    default_type = int
    format_unit = 'l'
    c_ignored_default = "0"

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 'l':
            return self.format_code("""
                {paramname} = PyLong_AsLong({argname});
                if ({paramname} == -1 && PyErr_Occurred()) {{{{
                    goto exit;
                }}}}
                """,
                argname=argname)
        return super().parse_arg(argname, displayname, limited_capi=limited_capi)


class unsigned_long_converter(CConverter):
    type = 'unsigned long'
    default_type = int
    c_ignored_default = "0"

    def converter_init(self, *, bitwise: bool = False) -> None:
        if bitwise:
            self.format_unit = 'k'
        else:
            self.converter = '_PyLong_UnsignedLong_Converter'

    def use_converter(self) -> None:
        if self.converter == '_PyLong_UnsignedLong_Converter':
            self.add_include('pycore_long.h',
                             '_PyLong_UnsignedLong_Converter()')

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 'k':
            return self.format_code("""
                if (!PyLong_Check({argname})) {{{{
                    {bad_argument}
                    goto exit;
                }}}}
                {paramname} = PyLong_AsUnsignedLongMask({argname});
                """,
                argname=argname,
                bad_argument=self.bad_argument(displayname, 'int', limited_capi=limited_capi),
            )
        if not limited_capi:
            return super().parse_arg(argname, displayname, limited_capi=limited_capi)
        # NOTE: Raises OverflowError for negative integer.
        return self.format_code("""
            {paramname} = PyLong_AsUnsignedLong({argname});
            if ({paramname} == (unsigned long)-1 && PyErr_Occurred()) {{{{
                goto exit;
            }}}}
            """,
            argname=argname)


class long_long_converter(CConverter):
    type = 'long long'
    default_type = int
    format_unit = 'L'
    c_ignored_default = "0"

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 'L':
            return self.format_code("""
                {paramname} = PyLong_AsLongLong({argname});
                if ({paramname} == -1 && PyErr_Occurred()) {{{{
                    goto exit;
                }}}}
                """,
                argname=argname)
        return super().parse_arg(argname, displayname, limited_capi=limited_capi)


class unsigned_long_long_converter(CConverter):
    type = 'unsigned long long'
    default_type = int
    c_ignored_default = "0"

    def converter_init(self, *, bitwise: bool = False) -> None:
        if bitwise:
            self.format_unit = 'K'
        else:
            self.converter = '_PyLong_UnsignedLongLong_Converter'

    def use_converter(self) -> None:
        if self.converter == '_PyLong_UnsignedLongLong_Converter':
            self.add_include('pycore_long.h',
                             '_PyLong_UnsignedLongLong_Converter()')

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 'K':
            return self.format_code("""
                if (!PyLong_Check({argname})) {{{{
                    {bad_argument}
                    goto exit;
                }}}}
                {paramname} = PyLong_AsUnsignedLongLongMask({argname});
                """,
                argname=argname,
                bad_argument=self.bad_argument(displayname, 'int', limited_capi=limited_capi),
            )
        if not limited_capi:
            return super().parse_arg(argname, displayname, limited_capi=limited_capi)
        # NOTE: Raises OverflowError for negative integer.
        return self.format_code("""
            {paramname} = PyLong_AsUnsignedLongLong({argname});
            if ({paramname} == (unsigned long long)-1 && PyErr_Occurred()) {{{{
                goto exit;
            }}}}
            """,
            argname=argname)


class Py_ssize_t_converter(CConverter):
    type = 'Py_ssize_t'
    c_ignored_default = "0"

    def converter_init(self, *, accept: TypeSet = {int}) -> None:
        if accept == {int}:
            self.format_unit = 'n'
            self.default_type = int
        elif accept == {int, NoneType}:
            self.converter = '_Py_convert_optional_to_ssize_t'
        else:
            fail(f"Py_ssize_t_converter: illegal 'accept' argument {accept!r}")

    def use_converter(self) -> None:
        if self.converter == '_Py_convert_optional_to_ssize_t':
            self.add_include('pycore_abstract.h',
                             '_Py_convert_optional_to_ssize_t()')

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 'n':
            if limited_capi:
                PyNumber_Index = 'PyNumber_Index'
            else:
                PyNumber_Index = '_PyNumber_Index'
                self.add_include('pycore_abstract.h', '_PyNumber_Index()')
            return self.format_code("""
                {{{{
                    Py_ssize_t ival = -1;
                    PyObject *iobj = {PyNumber_Index}({argname});
                    if (iobj != NULL) {{{{
                        ival = PyLong_AsSsize_t(iobj);
                        Py_DECREF(iobj);
                    }}}}
                    if (ival == -1 && PyErr_Occurred()) {{{{
                        goto exit;
                    }}}}
                    {paramname} = ival;
                }}}}
                """,
                argname=argname,
                PyNumber_Index=PyNumber_Index)
        if not limited_capi:
            return super().parse_arg(argname, displayname, limited_capi=limited_capi)
        return self.format_code("""
            if ({argname} != Py_None) {{{{
                if (PyIndex_Check({argname})) {{{{
                    {paramname} = PyNumber_AsSsize_t({argname}, PyExc_OverflowError);
                    if ({paramname} == -1 && PyErr_Occurred()) {{{{
                        goto exit;
                    }}}}
                }}}}
                else {{{{
                    {bad_argument}
                    goto exit;
                }}}}
            }}}}
            """,
            argname=argname,
            bad_argument=self.bad_argument(displayname, 'integer or None', limited_capi=limited_capi),
        )


class slice_index_converter(CConverter):
    type = 'Py_ssize_t'

    def converter_init(self, *, accept: TypeSet = {int, NoneType}) -> None:
        if accept == {int}:
            self.converter = '_PyEval_SliceIndexNotNone'
            self.nullable = False
        elif accept == {int, NoneType}:
            self.converter = '_PyEval_SliceIndex'
            self.nullable = True
        else:
            fail(f"slice_index_converter: illegal 'accept' argument {accept!r}")

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if not limited_capi:
            return super().parse_arg(argname, displayname, limited_capi=limited_capi)
        if self.nullable:
            return self.format_code("""
                if (!Py_IsNone({argname})) {{{{
                    if (PyIndex_Check({argname})) {{{{
                        {paramname} = PyNumber_AsSsize_t({argname}, NULL);
                        if ({paramname} == -1 && PyErr_Occurred()) {{{{
                            return 0;
                        }}}}
                    }}}}
                    else {{{{
                        PyErr_SetString(PyExc_TypeError,
                                        "slice indices must be integers or "
                                        "None or have an __index__ method");
                        goto exit;
                    }}}}
                }}}}
                """,
                argname=argname)
        else:
            return self.format_code("""
                if (PyIndex_Check({argname})) {{{{
                    {paramname} = PyNumber_AsSsize_t({argname}, NULL);
                    if ({paramname} == -1 && PyErr_Occurred()) {{{{
                        goto exit;
                    }}}}
                }}}}
                else {{{{
                    PyErr_SetString(PyExc_TypeError,
                                    "slice indices must be integers or "
                                    "have an __index__ method");
                    goto exit;
                }}}}
                """,
                argname=argname)

class size_t_converter(CConverter):
    type = 'size_t'
    converter = '_PyLong_Size_t_Converter'
    c_ignored_default = "0"

    def use_converter(self) -> None:
        self.add_include('pycore_long.h',
                         '_PyLong_Size_t_Converter()')

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 'n':
            return self.format_code("""
                {paramname} = PyNumber_AsSsize_t({argname}, PyExc_OverflowError);
                if ({paramname} == -1 && PyErr_Occurred()) {{{{
                    goto exit;
                }}}}
                """,
                argname=argname)
        if not limited_capi:
            return super().parse_arg(argname, displayname, limited_capi=limited_capi)
        # NOTE: Raises OverflowError for negative integer.
        return self.format_code("""
            {paramname} = PyLong_AsSize_t({argname});
            if ({paramname} == (size_t)-1 && PyErr_Occurred()) {{{{
                goto exit;
            }}}}
            """,
            argname=argname)
