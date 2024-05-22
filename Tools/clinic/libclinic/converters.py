import builtins as bltns
import functools
import sys
from types import NoneType
from typing import Any

from libclinic import fail, Null, unspecified, unknown
from libclinic.function import (
    Function, Parameter,
    CALLABLE, STATIC_METHOD, CLASS_METHOD, METHOD_INIT, METHOD_NEW,
    GETTER, SETTER)
from libclinic.codegen import CRenderData, TemplateDict
from libclinic.converter import (
    CConverter, legacy_converters, add_legacy_c_converter)


TypeSet = set[bltns.type[object]]


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


class defining_class_converter(CConverter):
    """
    A special-case converter:
    this is the default converter used for the defining class.
    """
    type = 'PyTypeObject *'
    format_unit = ''
    show_in_signature = False

    def converter_init(self, *, type: str | None = None) -> None:
        self.specified_type = type

    def render(self, parameter: Parameter, data: CRenderData) -> None:
        self._render_self(parameter, data)

    def set_template_dict(self, template_dict: TemplateDict) -> None:
        template_dict['defining_class_name'] = self.name


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


class fildes_converter(CConverter):
    type = 'int'
    converter = '_PyLong_FileDescriptor_Converter'

    def use_converter(self) -> None:
        self.add_include('pycore_fileutils.h',
                         '_PyLong_FileDescriptor_Converter()')

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        return self.format_code("""
            {paramname} = PyObject_AsFileDescriptor({argname});
            if ({paramname} < 0) {{{{
                goto exit;
            }}}}
            """,
            argname=argname)


class float_converter(CConverter):
    type = 'float'
    default_type = float
    format_unit = 'f'
    c_ignored_default = "0.0"

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 'f':
            if not limited_capi:
                return self.format_code("""
                    if (PyFloat_CheckExact({argname})) {{{{
                        {paramname} = (float) (PyFloat_AS_DOUBLE({argname}));
                    }}}}
                    else
                    {{{{
                        {paramname} = (float) PyFloat_AsDouble({argname});
                        if ({paramname} == -1.0 && PyErr_Occurred()) {{{{
                            goto exit;
                        }}}}
                    }}}}
                    """,
                    argname=argname)
            else:
                return self.format_code("""
                    {paramname} = (float) PyFloat_AsDouble({argname});
                    if ({paramname} == -1.0 && PyErr_Occurred()) {{{{
                        goto exit;
                    }}}}
                    """,
                    argname=argname)
        return super().parse_arg(argname, displayname, limited_capi=limited_capi)


class double_converter(CConverter):
    type = 'double'
    default_type = float
    format_unit = 'd'
    c_ignored_default = "0.0"

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 'd':
            if not limited_capi:
                return self.format_code("""
                    if (PyFloat_CheckExact({argname})) {{{{
                        {paramname} = PyFloat_AS_DOUBLE({argname});
                    }}}}
                    else
                    {{{{
                        {paramname} = PyFloat_AsDouble({argname});
                        if ({paramname} == -1.0 && PyErr_Occurred()) {{{{
                            goto exit;
                        }}}}
                    }}}}
                    """,
                    argname=argname)
            else:
                return self.format_code("""
                    {paramname} = PyFloat_AsDouble({argname});
                    if ({paramname} == -1.0 && PyErr_Occurred()) {{{{
                        goto exit;
                    }}}}
                    """,
                    argname=argname)
        return super().parse_arg(argname, displayname, limited_capi=limited_capi)


class Py_complex_converter(CConverter):
    type = 'Py_complex'
    default_type = complex
    format_unit = 'D'
    c_ignored_default = "{0.0, 0.0}"

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 'D':
            return self.format_code("""
                {paramname} = PyComplex_AsCComplex({argname});
                if (PyErr_Occurred()) {{{{
                    goto exit;
                }}}}
                """,
                argname=argname)
        return super().parse_arg(argname, displayname, limited_capi=limited_capi)


class object_converter(CConverter):
    type = 'PyObject *'
    format_unit = 'O'

    def converter_init(
            self, *,
            converter: str | None = None,
            type: str | None = None,
            subclass_of: str | None = None
    ) -> None:
        if converter:
            if subclass_of:
                fail("object: Cannot pass in both 'converter' and 'subclass_of'")
            self.format_unit = 'O&'
            self.converter = converter
        elif subclass_of:
            self.format_unit = 'O!'
            self.subclass_of = subclass_of

        if type is not None:
            self.type = type


#
# We define three conventions for buffer types in the 'accept' argument:
#
#  buffer  : any object supporting the buffer interface
#  rwbuffer: any object supporting the buffer interface, but must be writeable
#  robuffer: any object supporting the buffer interface, but must not be writeable
#

class buffer:
    pass
class rwbuffer:
    pass
class robuffer:
    pass


StrConverterKeyType = tuple[frozenset[type[object]], bool, bool]

def str_converter_key(
    types: TypeSet, encoding: bool | str | None, zeroes: bool
) -> StrConverterKeyType:
    return (frozenset(types), bool(encoding), bool(zeroes))

str_converter_argument_map: dict[StrConverterKeyType, str] = {}


class str_converter(CConverter):
    type = 'const char *'
    default_type = (str, Null, NoneType)
    format_unit = 's'

    def converter_init(
            self,
            *,
            accept: TypeSet = {str},
            encoding: str | None = None,
            zeroes: bool = False
    ) -> None:

        key = str_converter_key(accept, encoding, zeroes)
        format_unit = str_converter_argument_map.get(key)
        if not format_unit:
            fail("str_converter: illegal combination of arguments", key)

        self.format_unit = format_unit
        self.length = bool(zeroes)
        if encoding:
            if self.default not in (Null, None, unspecified):
                fail("str_converter: Argument Clinic doesn't support default values for encoded strings")
            self.encoding = encoding
            self.type = 'char *'
            # sorry, clinic can't support preallocated buffers
            # for es# and et#
            self.c_default = "NULL"
        if NoneType in accept and self.c_default == "Py_None":
            self.c_default = "NULL"

    def post_parsing(self) -> str:
        if self.encoding:
            name = self.name
            return f"PyMem_FREE({name});\n"
        else:
            return ""

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 's':
            return self.format_code("""
                if (!PyUnicode_Check({argname})) {{{{
                    {bad_argument}
                    goto exit;
                }}}}
                Py_ssize_t {length_name};
                {paramname} = PyUnicode_AsUTF8AndSize({argname}, &{length_name});
                if ({paramname} == NULL) {{{{
                    goto exit;
                }}}}
                if (strlen({paramname}) != (size_t){length_name}) {{{{
                    PyErr_SetString(PyExc_ValueError, "embedded null character");
                    goto exit;
                }}}}
                """,
                argname=argname,
                bad_argument=self.bad_argument(displayname, 'str', limited_capi=limited_capi),
                length_name=self.length_name)
        if self.format_unit == 'z':
            return self.format_code("""
                if ({argname} == Py_None) {{{{
                    {paramname} = NULL;
                }}}}
                else if (PyUnicode_Check({argname})) {{{{
                    Py_ssize_t {length_name};
                    {paramname} = PyUnicode_AsUTF8AndSize({argname}, &{length_name});
                    if ({paramname} == NULL) {{{{
                        goto exit;
                    }}}}
                    if (strlen({paramname}) != (size_t){length_name}) {{{{
                        PyErr_SetString(PyExc_ValueError, "embedded null character");
                        goto exit;
                    }}}}
                }}}}
                else {{{{
                    {bad_argument}
                    goto exit;
                }}}}
                """,
                argname=argname,
                bad_argument=self.bad_argument(displayname, 'str or None', limited_capi=limited_capi),
                length_name=self.length_name)
        return super().parse_arg(argname, displayname, limited_capi=limited_capi)

#
# This is the fourth or fifth rewrite of registering all the
# string converter format units.  Previous approaches hid
# bugs--generally mismatches between the semantics of the format
# unit and the arguments necessary to represent those semantics
# properly.  Hopefully with this approach we'll get it 100% right.
#
# The r() function (short for "register") both registers the
# mapping from arguments to format unit *and* registers the
# legacy C converter for that format unit.
#
def r(format_unit: str,
      *,
      accept: TypeSet,
      encoding: bool = False,
      zeroes: bool = False
) -> None:
    if not encoding and format_unit != 's':
        # add the legacy c converters here too.
        #
        # note: add_legacy_c_converter can't work for
        #   es, es#, et, or et#
        #   because of their extra encoding argument
        #
        # also don't add the converter for 's' because
        # the metaclass for CConverter adds it for us.
        kwargs: dict[str, Any] = {}
        if accept != {str}:
            kwargs['accept'] = accept
        if zeroes:
            kwargs['zeroes'] = True
        added_f = functools.partial(str_converter, **kwargs)
        legacy_converters[format_unit] = added_f

    d = str_converter_argument_map
    key = str_converter_key(accept, encoding, zeroes)
    if key in d:
        sys.exit("Duplicate keys specified for str_converter_argument_map!")
    d[key] = format_unit

r('es',  encoding=True,              accept={str})
r('es#', encoding=True, zeroes=True, accept={str})
r('et',  encoding=True,              accept={bytes, bytearray, str})
r('et#', encoding=True, zeroes=True, accept={bytes, bytearray, str})
r('s',                               accept={str})
r('s#',                 zeroes=True, accept={robuffer, str})
r('y',                               accept={robuffer})
r('y#',                 zeroes=True, accept={robuffer})
r('z',                               accept={str, NoneType})
r('z#',                 zeroes=True, accept={robuffer, str, NoneType})
del r


class PyBytesObject_converter(CConverter):
    type = 'PyBytesObject *'
    format_unit = 'S'
    # accept = {bytes}

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 'S':
            return self.format_code("""
                if (!PyBytes_Check({argname})) {{{{
                    {bad_argument}
                    goto exit;
                }}}}
                {paramname} = ({type}){argname};
                """,
                argname=argname,
                bad_argument=self.bad_argument(displayname, 'bytes', limited_capi=limited_capi),
                type=self.type)
        return super().parse_arg(argname, displayname, limited_capi=limited_capi)


class PyByteArrayObject_converter(CConverter):
    type = 'PyByteArrayObject *'
    format_unit = 'Y'
    # accept = {bytearray}

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 'Y':
            return self.format_code("""
                if (!PyByteArray_Check({argname})) {{{{
                    {bad_argument}
                    goto exit;
                }}}}
                {paramname} = ({type}){argname};
                """,
                argname=argname,
                bad_argument=self.bad_argument(displayname, 'bytearray', limited_capi=limited_capi),
                type=self.type)
        return super().parse_arg(argname, displayname, limited_capi=limited_capi)


class unicode_converter(CConverter):
    type = 'PyObject *'
    default_type = (str, Null, NoneType)
    format_unit = 'U'

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 'U':
            return self.format_code("""
                if (!PyUnicode_Check({argname})) {{{{
                    {bad_argument}
                    goto exit;
                }}}}
                {paramname} = {argname};
                """,
                argname=argname,
                bad_argument=self.bad_argument(displayname, 'str', limited_capi=limited_capi),
            )
        return super().parse_arg(argname, displayname, limited_capi=limited_capi)


@add_legacy_c_converter('u')
@add_legacy_c_converter('u#', zeroes=True)
@add_legacy_c_converter('Z', accept={str, NoneType})
@add_legacy_c_converter('Z#', accept={str, NoneType}, zeroes=True)
class Py_UNICODE_converter(CConverter):
    type = 'const wchar_t *'
    default_type = (str, Null, NoneType)

    def converter_init(
            self, *,
            accept: TypeSet = {str},
            zeroes: bool = False
    ) -> None:
        format_unit = 'Z' if accept=={str, NoneType} else 'u'
        if zeroes:
            format_unit += '#'
            self.length = True
            self.format_unit = format_unit
        else:
            self.accept = accept
            if accept == {str}:
                self.converter = '_PyUnicode_WideCharString_Converter'
            elif accept == {str, NoneType}:
                self.converter = '_PyUnicode_WideCharString_Opt_Converter'
            else:
                fail(f"Py_UNICODE_converter: illegal 'accept' argument {accept!r}")
        self.c_default = "NULL"

    def cleanup(self) -> str:
        if self.length:
            return ""
        else:
            return f"""PyMem_Free((void *){self.parser_name});\n"""

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if not self.length:
            if self.accept == {str}:
                return self.format_code("""
                    if (!PyUnicode_Check({argname})) {{{{
                        {bad_argument}
                        goto exit;
                    }}}}
                    {paramname} = PyUnicode_AsWideCharString({argname}, NULL);
                    if ({paramname} == NULL) {{{{
                        goto exit;
                    }}}}
                    """,
                    argname=argname,
                    bad_argument=self.bad_argument(displayname, 'str', limited_capi=limited_capi),
                )
            elif self.accept == {str, NoneType}:
                return self.format_code("""
                    if ({argname} == Py_None) {{{{
                        {paramname} = NULL;
                    }}}}
                    else if (PyUnicode_Check({argname})) {{{{
                        {paramname} = PyUnicode_AsWideCharString({argname}, NULL);
                        if ({paramname} == NULL) {{{{
                            goto exit;
                        }}}}
                    }}}}
                    else {{{{
                        {bad_argument}
                        goto exit;
                    }}}}
                    """,
                    argname=argname,
                    bad_argument=self.bad_argument(displayname, 'str or None', limited_capi=limited_capi),
                )
        return super().parse_arg(argname, displayname, limited_capi=limited_capi)


@add_legacy_c_converter('s*', accept={str, buffer})
@add_legacy_c_converter('z*', accept={str, buffer, NoneType})
@add_legacy_c_converter('w*', accept={rwbuffer})
class Py_buffer_converter(CConverter):
    type = 'Py_buffer'
    format_unit = 'y*'
    impl_by_reference = True
    c_ignored_default = "{NULL, NULL}"

    def converter_init(self, *, accept: TypeSet = {buffer}) -> None:
        if self.default not in (unspecified, None):
            fail("The only legal default value for Py_buffer is None.")

        self.c_default = self.c_ignored_default

        if accept == {str, buffer, NoneType}:
            format_unit = 'z*'
        elif accept == {str, buffer}:
            format_unit = 's*'
        elif accept == {buffer}:
            format_unit = 'y*'
        elif accept == {rwbuffer}:
            format_unit = 'w*'
        else:
            fail("Py_buffer_converter: illegal combination of arguments")

        self.format_unit = format_unit

    def cleanup(self) -> str:
        name = self.name
        return "".join(["if (", name, ".obj) {\n   PyBuffer_Release(&", name, ");\n}\n"])

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        # PyBUF_SIMPLE guarantees that the format units of the buffers are C-contiguous.
        if self.format_unit == 'y*':
            return self.format_code("""
                if (PyObject_GetBuffer({argname}, &{paramname}, PyBUF_SIMPLE) != 0) {{{{
                    goto exit;
                }}}}
                """,
                argname=argname,
                bad_argument=self.bad_argument(displayname, 'contiguous buffer', limited_capi=limited_capi),
            )
        elif self.format_unit == 's*':
            return self.format_code("""
                if (PyUnicode_Check({argname})) {{{{
                    Py_ssize_t len;
                    const char *ptr = PyUnicode_AsUTF8AndSize({argname}, &len);
                    if (ptr == NULL) {{{{
                        goto exit;
                    }}}}
                    if (PyBuffer_FillInfo(&{paramname}, {argname}, (void *)ptr, len, 1, PyBUF_SIMPLE) < 0) {{{{
                        goto exit;
                    }}}}
                }}}}
                else {{{{ /* any bytes-like object */
                    if (PyObject_GetBuffer({argname}, &{paramname}, PyBUF_SIMPLE) != 0) {{{{
                        goto exit;
                    }}}}
                }}}}
                """,
                argname=argname,
                bad_argument=self.bad_argument(displayname, 'contiguous buffer', limited_capi=limited_capi),
            )
        elif self.format_unit == 'w*':
            return self.format_code("""
                if (PyObject_GetBuffer({argname}, &{paramname}, PyBUF_WRITABLE) < 0) {{{{
                    {bad_argument}
                    goto exit;
                }}}}
                """,
                argname=argname,
                bad_argument=self.bad_argument(displayname, 'read-write bytes-like object', limited_capi=limited_capi),
                bad_argument2=self.bad_argument(displayname, 'contiguous buffer', limited_capi=limited_capi),
            )
        return super().parse_arg(argname, displayname, limited_capi=limited_capi)


def correct_name_for_self(
        f: Function
) -> tuple[str, str]:
    if f.kind in {CALLABLE, METHOD_INIT, GETTER, SETTER}:
        if f.cls:
            return "PyObject *", "self"
        return "PyObject *", "module"
    if f.kind is STATIC_METHOD:
        return "void *", "null"
    if f.kind in (CLASS_METHOD, METHOD_NEW):
        return "PyTypeObject *", "type"
    raise AssertionError(f"Unhandled type of function f: {f.kind!r}")


class self_converter(CConverter):
    """
    A special-case converter:
    this is the default converter used for "self".
    """
    type: str | None = None
    format_unit = ''

    def converter_init(self, *, type: str | None = None) -> None:
        self.specified_type = type

    def pre_render(self) -> None:
        f = self.function
        default_type, default_name = correct_name_for_self(f)
        self.signature_name = default_name
        self.type = self.specified_type or self.type or default_type

        kind = self.function.kind

        if kind is STATIC_METHOD or kind.new_or_init:
            self.show_in_signature = False

    # tp_new (METHOD_NEW) functions are of type newfunc:
    #     typedef PyObject *(*newfunc)(PyTypeObject *, PyObject *, PyObject *);
    #
    # tp_init (METHOD_INIT) functions are of type initproc:
    #     typedef int (*initproc)(PyObject *, PyObject *, PyObject *);
    #
    # All other functions generated by Argument Clinic are stored in
    # PyMethodDef structures, in the ml_meth slot, which is of type PyCFunction:
    #     typedef PyObject *(*PyCFunction)(PyObject *, PyObject *);
    # However!  We habitually cast these functions to PyCFunction,
    # since functions that accept keyword arguments don't fit this signature
    # but are stored there anyway.  So strict type equality isn't important
    # for these functions.
    #
    # So:
    #
    # * The name of the first parameter to the impl and the parsing function will always
    #   be self.name.
    #
    # * The type of the first parameter to the impl will always be of self.type.
    #
    # * If the function is neither tp_new (METHOD_NEW) nor tp_init (METHOD_INIT):
    #   * The type of the first parameter to the parsing function is also self.type.
    #     This means that if you step into the parsing function, your "self" parameter
    #     is of the correct type, which may make debugging more pleasant.
    #
    # * Else if the function is tp_new (METHOD_NEW):
    #   * The type of the first parameter to the parsing function is "PyTypeObject *",
    #     so the type signature of the function call is an exact match.
    #   * If self.type != "PyTypeObject *", we cast the first parameter to self.type
    #     in the impl call.
    #
    # * Else if the function is tp_init (METHOD_INIT):
    #   * The type of the first parameter to the parsing function is "PyObject *",
    #     so the type signature of the function call is an exact match.
    #   * If self.type != "PyObject *", we cast the first parameter to self.type
    #     in the impl call.

    @property
    def parser_type(self) -> str:
        assert self.type is not None
        if self.function.kind in {METHOD_INIT, METHOD_NEW, STATIC_METHOD, CLASS_METHOD}:
            tp, _ = correct_name_for_self(self.function)
            return tp
        return self.type

    def render(self, parameter: Parameter, data: CRenderData) -> None:
        """
        parameter is a clinic.Parameter instance.
        data is a CRenderData instance.
        """
        if self.function.kind is STATIC_METHOD:
            return

        self._render_self(parameter, data)

        if self.type != self.parser_type:
            # insert cast to impl_argument[0], aka self.
            # we know we're in the first slot in all the CRenderData lists,
            # because we render parameters in order, and self is always first.
            assert len(data.impl_arguments) == 1
            assert data.impl_arguments[0] == self.name
            assert self.type is not None
            data.impl_arguments[0] = '(' + self.type + ")" + data.impl_arguments[0]

    def set_template_dict(self, template_dict: TemplateDict) -> None:
        template_dict['self_name'] = self.name
        template_dict['self_type'] = self.parser_type
        kind = self.function.kind
        cls = self.function.cls

        if kind.new_or_init and cls and cls.typedef:
            if kind is METHOD_NEW:
                type_check = (
                    '({0} == base_tp || {0}->tp_init == base_tp->tp_init)'
                 ).format(self.name)
            else:
                type_check = ('(Py_IS_TYPE({0}, base_tp) ||\n        '
                              ' Py_TYPE({0})->tp_new == base_tp->tp_new)'
                             ).format(self.name)

            line = f'{type_check} &&\n        '
            template_dict['self_type_check'] = line

            type_object = cls.type_object
            type_ptr = f'PyTypeObject *base_tp = {type_object};'
            template_dict['base_type_ptr'] = type_ptr
