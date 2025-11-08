import sys
from collections.abc import Callable
from libclinic.codegen import CRenderData
from libclinic.function import Function
from typing import Any


ReturnConverterType = Callable[..., "CReturnConverter"]


# maps strings to callables.
# these callables must be of the form:
#   def foo(*, ...)
# The callable may have any number of keyword-only parameters.
# The callable must return a CReturnConverter object.
# The callable should not call builtins.print.
ReturnConverterDict = dict[str, ReturnConverterType]
return_converters: ReturnConverterDict = {}


def add_c_return_converter(
    f: ReturnConverterType,
    name: str | None = None
) -> ReturnConverterType:
    if not name:
        name = f.__name__
        if not name.endswith('_return_converter'):
            return f
        name = name.removesuffix('_return_converter')
    return_converters[name] = f
    return f


class CReturnConverterAutoRegister(type):
    def __init__(
        cls: ReturnConverterType,
        name: str,
        bases: tuple[type[object], ...],
        classdict: dict[str, Any]
    ) -> None:
        add_c_return_converter(cls)


class CReturnConverter(metaclass=CReturnConverterAutoRegister):

    # The C type to use for this variable.
    # 'type' should be a Python string specifying the type, e.g. "int".
    # If this is a pointer type, the type string should end with ' *'.
    type = 'PyObject *'

    # The Python default value for this parameter, as a Python value.
    # Or the magic value "unspecified" if there is no default.
    default: object = None

    def __init__(
        self,
        *,
        py_default: str | None = None,
        **kwargs: Any
    ) -> None:
        self.py_default = py_default
        try:
            self.return_converter_init(**kwargs)
        except TypeError as e:
            s = ', '.join(name + '=' + repr(value) for name, value in kwargs.items())
            sys.exit(self.__class__.__name__ + '(' + s + ')\n' + str(e))

    def return_converter_init(self) -> None: ...

    def declare(self, data: CRenderData) -> None:
        line: list[str] = []
        add = line.append
        add(self.type)
        if not self.type.endswith('*'):
            add(' ')
        add(data.converter_retval + ';')
        data.declarations.append(''.join(line))
        data.return_value = data.converter_retval

    def err_occurred_if(
        self,
        expr: str,
        data: CRenderData
    ) -> None:
        line = f'if (({expr}) && PyErr_Occurred()) {{\n    goto exit;\n}}\n'
        data.return_conversion.append(line)

    def err_occurred_if_null_pointer(
        self,
        variable: str,
        data: CRenderData
    ) -> None:
        line = f'if ({variable} == NULL) {{\n    goto exit;\n}}\n'
        data.return_conversion.append(line)

    def render(
        self,
        function: Function,
        data: CRenderData
    ) -> None: ...


add_c_return_converter(CReturnConverter, 'object')


class bool_return_converter(CReturnConverter):
    type = 'int'

    def render(self, function: Function, data: CRenderData) -> None:
        self.declare(data)
        self.err_occurred_if(f"{data.converter_retval} == -1", data)
        data.return_conversion.append(
            f'return_value = PyBool_FromLong((long){data.converter_retval});\n'
        )


class long_return_converter(CReturnConverter):
    type = 'long'
    conversion_fn = 'PyLong_FromLong'
    cast = ''
    unsigned_cast = ''

    def render(self, function: Function, data: CRenderData) -> None:
        self.declare(data)
        self.err_occurred_if(f"{data.converter_retval} == {self.unsigned_cast}-1", data)
        data.return_conversion.append(
            f'return_value = {self.conversion_fn}({self.cast}{data.converter_retval});\n'
        )


class int_return_converter(long_return_converter):
    type = 'int'
    cast = '(long)'


class unsigned_long_return_converter(long_return_converter):
    type = 'unsigned long'
    conversion_fn = 'PyLong_FromUnsignedLong'
    unsigned_cast = '(unsigned long)'


class unsigned_int_return_converter(unsigned_long_return_converter):
    type = 'unsigned int'
    cast = '(unsigned long)'
    unsigned_cast = '(unsigned int)'


class Py_ssize_t_return_converter(long_return_converter):
    type = 'Py_ssize_t'
    conversion_fn = 'PyLong_FromSsize_t'


class size_t_return_converter(long_return_converter):
    type = 'size_t'
    conversion_fn = 'PyLong_FromSize_t'
    unsigned_cast = '(size_t)'


class double_return_converter(CReturnConverter):
    type = 'double'
    cast = ''

    def render(self, function: Function, data: CRenderData) -> None:
        self.declare(data)
        self.err_occurred_if(f"{data.converter_retval} == -1.0", data)
        data.return_conversion.append(
            f'return_value = PyFloat_FromDouble({self.cast}{data.converter_retval});\n'
        )


class float_return_converter(double_return_converter):
    type = 'float'
    cast = '(double)'
