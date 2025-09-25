from __future__ import annotations
import builtins as bltns
import functools
from typing import Any, TypeVar, Literal, TYPE_CHECKING, cast
from collections.abc import Callable

import libclinic
from libclinic import fail
from libclinic import Sentinels, unspecified, unknown
from libclinic.codegen import CRenderData, Include, TemplateDict
from libclinic.function import Function, Parameter


CConverterClassT = TypeVar("CConverterClassT", bound=type["CConverter"])


type_checks = {
    '&PyLong_Type': ('PyLong_Check', 'int'),
    '&PyTuple_Type': ('PyTuple_Check', 'tuple'),
    '&PyList_Type': ('PyList_Check', 'list'),
    '&PySet_Type': ('PySet_Check', 'set'),
    '&PyFrozenSet_Type': ('PyFrozenSet_Check', 'frozenset'),
    '&PyDict_Type': ('PyDict_Check', 'dict'),
    '&PyUnicode_Type': ('PyUnicode_Check', 'str'),
    '&PyBytes_Type': ('PyBytes_Check', 'bytes'),
    '&PyByteArray_Type': ('PyByteArray_Check', 'bytearray'),
}


def add_c_converter(
        f: CConverterClassT,
        name: str | None = None
) -> CConverterClassT:
    if not name:
        name = f.__name__
        if not name.endswith('_converter'):
            return f
        name = name.removesuffix('_converter')
    converters[name] = f
    return f


def add_default_legacy_c_converter(cls: CConverterClassT) -> CConverterClassT:
    # automatically add converter for default format unit
    # (but without stomping on the existing one if it's already
    # set, in case you subclass)
    if ((cls.format_unit not in ('O&', '')) and
        (cls.format_unit not in legacy_converters)):
        legacy_converters[cls.format_unit] = cls
    return cls


class CConverterAutoRegister(type):
    def __init__(
        cls, name: str, bases: tuple[type[object], ...], classdict: dict[str, Any]
    ) -> None:
        converter_cls = cast(type["CConverter"], cls)
        add_c_converter(converter_cls)
        add_default_legacy_c_converter(converter_cls)

class CConverter(metaclass=CConverterAutoRegister):
    """
    For the init function, self, name, function, and default
    must be keyword-or-positional parameters.  All other
    parameters must be keyword-only.
    """

    # The C name to use for this variable.
    name: str

    # The Python name to use for this variable.
    py_name: str

    # The C type to use for this variable.
    # 'type' should be a Python string specifying the type, e.g. "int".
    # If this is a pointer type, the type string should end with ' *'.
    type: str | None = None

    # The Python default value for this parameter, as a Python value.
    # Or the magic value "unspecified" if there is no default.
    # Or the magic value "unknown" if this value is a cannot be evaluated
    # at Argument-Clinic-preprocessing time (but is presumed to be valid
    # at runtime).
    default: object = unspecified

    # If not None, default must be isinstance() of this type.
    # (You can also specify a tuple of types.)
    default_type: bltns.type[object] | tuple[bltns.type[object], ...] | None = None

    # "default" converted into a C value, as a string.
    # Or None if there is no default.
    c_default: str | None = None

    # "default" converted into a Python value, as a string.
    # Or None if there is no default.
    py_default: str | None = None

    # The default value used to initialize the C variable when
    # there is no default, but not specifying a default may
    # result in an "uninitialized variable" warning.  This can
    # easily happen when using option groups--although
    # properly-written code won't actually use the variable,
    # the variable does get passed in to the _impl.  (Ah, if
    # only dataflow analysis could inline the static function!)
    #
    # This value is specified as a string.
    # Every non-abstract subclass should supply a valid value.
    c_ignored_default: str = 'NULL'

    # If true, wrap with Py_UNUSED.
    unused = False

    # The C converter *function* to be used, if any.
    # (If this is not None, format_unit must be 'O&'.)
    converter: str | None = None

    # Should Argument Clinic add a '&' before the name of
    # the variable when passing it into the _impl function?
    impl_by_reference = False

    # Should Argument Clinic add a '&' before the name of
    # the variable when passing it into PyArg_ParseTuple (AndKeywords)?
    parse_by_reference = True

    #############################################################
    #############################################################
    ## You shouldn't need to read anything below this point to ##
    ## write your own converter functions.                     ##
    #############################################################
    #############################################################

    # The "format unit" to specify for this variable when
    # parsing arguments using PyArg_ParseTuple (AndKeywords).
    # Custom converters should always use the default value of 'O&'.
    format_unit = 'O&'

    # What encoding do we want for this variable?  Only used
    # by format units starting with 'e'.
    encoding: str | None = None

    # Should this object be required to be a subclass of a specific type?
    # If not None, should be a string representing a pointer to a
    # PyTypeObject (e.g. "&PyUnicode_Type").
    # Only used by the 'O!' format unit (and the "object" converter).
    subclass_of: str | None = None

    # See also the 'length_name' property.
    # Only used by format units ending with '#'.
    length = False

    # Should we show this parameter in the generated
    # __text_signature__? This is *almost* always True.
    # (It's only False for __new__, __init__, and METH_STATIC functions.)
    show_in_signature = True

    # Overrides the name used in a text signature.
    # The name used for a "self" parameter must be one of
    # self, type, or module; however users can set their own.
    # This lets the self_converter overrule the user-settable
    # name, *just* for the text signature.
    # Only set by self_converter.
    signature_name: str | None = None

    broken_limited_capi: bool = False

    # keep in sync with self_converter.__init__!
    def __init__(self,
             # Positional args:
             name: str,
             py_name: str,
             function: Function,
             default: object = unspecified,
             *,  # Keyword only args:
             c_default: str | None = None,
             py_default: str | None = None,
             annotation: str | Literal[Sentinels.unspecified] = unspecified,
             unused: bool = False,
             **kwargs: Any
    ) -> None:
        self.name = libclinic.ensure_legal_c_identifier(name)
        self.py_name = py_name
        self.unused = unused
        self._includes: list[Include] = []

        if default is not unspecified:
            if (self.default_type
                and default is not unknown
                and not isinstance(default, self.default_type)
            ):
                if isinstance(self.default_type, type):
                    types_str = self.default_type.__name__
                else:
                    names = [cls.__name__ for cls in self.default_type]
                    types_str = ', '.join(names)
                cls_name = self.__class__.__name__
                fail(f"{cls_name}: default value {default!r} for field "
                     f"{name!r} is not of type {types_str!r}")
            self.default = default

        if c_default:
            self.c_default = c_default
        if py_default:
            self.py_default = py_default

        if annotation is not unspecified:
            fail("The 'annotation' parameter is not currently permitted.")

        # Make sure not to set self.function until after converter_init() has been called.
        # This prevents you from caching information
        # about the function in converter_init().
        # (That breaks if we get cloned.)
        self.converter_init(**kwargs)
        self.function = function

    # Add a custom __getattr__ method to improve the error message
    # if somebody tries to access self.function in converter_init().
    #
    # mypy will assume arbitrary access is okay for a class with a __getattr__ method,
    # and that's not what we want,
    # so put it inside an `if not TYPE_CHECKING` block
    if not TYPE_CHECKING:
        def __getattr__(self, attr):
            if attr == "function":
                fail(
                    f"{self.__class__.__name__!r} object has no attribute 'function'.\n"
                    f"Note: accessing self.function inside converter_init is disallowed!"
                )
            return super().__getattr__(attr)
    # this branch is just here for coverage reporting
    else:  # pragma: no cover
        pass

    def converter_init(self) -> None:
        pass

    def is_optional(self) -> bool:
        return (self.default is not unspecified)

    def _render_self(self, parameter: Parameter, data: CRenderData) -> None:
        self.parameter = parameter
        name = self.parser_name

        # impl_arguments
        s = ("&" if self.impl_by_reference else "") + name
        data.impl_arguments.append(s)
        if self.length:
            data.impl_arguments.append(self.length_name)

        # impl_parameters
        data.impl_parameters.append(self.simple_declaration(by_reference=self.impl_by_reference))
        if self.length:
            data.impl_parameters.append(f"Py_ssize_t {self.length_name}")

    def _render_non_self(
            self,
            parameter: Parameter,
            data: CRenderData
    ) -> None:
        self.parameter = parameter
        name = self.name

        # declarations
        d = self.declaration(in_parser=True)
        data.declarations.append(d)

        # initializers
        initializers = self.initialize()
        if initializers:
            data.initializers.append('/* initializers for ' + name + ' */\n' + initializers.rstrip())

        # modifications
        modifications = self.modify()
        if modifications:
            data.modifications.append('/* modifications for ' + name + ' */\n' + modifications.rstrip())

        # keywords
        if parameter.is_variable_length():
            pass
        elif parameter.is_positional_only():
            data.keywords.append('')
        else:
            data.keywords.append(parameter.name)

        # format_units
        if self.is_optional() and '|' not in data.format_units:
            data.format_units.append('|')
        if parameter.is_keyword_only() and '$' not in data.format_units:
            data.format_units.append('$')
        data.format_units.append(self.format_unit)

        # parse_arguments
        self.parse_argument(data.parse_arguments)

        # post_parsing
        if post_parsing := self.post_parsing():
            data.post_parsing.append('/* Post parse cleanup for ' + name + ' */\n' + post_parsing.rstrip() + '\n')

        # cleanup
        cleanup = self.cleanup()
        if cleanup:
            data.cleanup.append('/* Cleanup for ' + name + ' */\n' + cleanup.rstrip() + "\n")

    def render(self, parameter: Parameter, data: CRenderData) -> None:
        """
        parameter is a clinic.Parameter instance.
        data is a CRenderData instance.
        """
        self._render_self(parameter, data)
        self._render_non_self(parameter, data)

    @functools.cached_property
    def length_name(self) -> str:
        """Computes the name of the associated "length" variable."""
        assert self.length is not None
        return self.name + "_length"

    # Why is this one broken out separately?
    # For "positional-only" function parsing,
    # which generates a bunch of PyArg_ParseTuple calls.
    def parse_argument(self, args: list[str]) -> None:
        assert not (self.converter and self.encoding)
        if self.format_unit == 'O&':
            assert self.converter
            args.append(self.converter)

        if self.encoding:
            args.append(libclinic.c_repr(self.encoding))
        elif self.subclass_of:
            args.append(self.subclass_of)

        s = ("&" if self.parse_by_reference else "") + self.parser_name
        args.append(s)

        if self.length:
            args.append(f"&{self.length_name}")

    #
    # All the functions after here are intended as extension points.
    #

    def simple_declaration(
            self,
            by_reference: bool = False,
            *,
            in_parser: bool = False
    ) -> str:
        """
        Computes the basic declaration of the variable.
        Used in computing the prototype declaration and the
        variable declaration.
        """
        assert isinstance(self.type, str)
        prototype = [self.type]
        if by_reference or not self.type.endswith('*'):
            prototype.append(" ")
        if by_reference:
            prototype.append('*')
        if in_parser:
            name = self.parser_name
        else:
            name = self.name
            if self.unused:
                name = f"Py_UNUSED({name})"
        prototype.append(name)
        return "".join(prototype)

    def declaration(self, *, in_parser: bool = False) -> str:
        """
        The C statement to declare this variable.
        """
        declaration = [self.simple_declaration(in_parser=True)]
        default = self.c_default
        if not default and self.parameter.group:
            default = self.c_ignored_default
        if default:
            declaration.append(" = ")
            declaration.append(default)
        declaration.append(";")
        if self.length:
            declaration.append('\n')
            declaration.append(f"Py_ssize_t {self.length_name};")
        return "".join(declaration)

    def initialize(self) -> str:
        """
        The C statements required to set up this variable before parsing.
        Returns a string containing this code indented at column 0.
        If no initialization is necessary, returns an empty string.
        """
        return ""

    def modify(self) -> str:
        """
        The C statements required to modify this variable after parsing.
        Returns a string containing this code indented at column 0.
        If no modification is necessary, returns an empty string.
        """
        return ""

    def post_parsing(self) -> str:
        """
        The C statements required to do some operations after the end of parsing but before cleaning up.
        Return a string containing this code indented at column 0.
        If no operation is necessary, return an empty string.
        """
        return ""

    def cleanup(self) -> str:
        """
        The C statements required to clean up after this variable.
        Returns a string containing this code indented at column 0.
        If no cleanup is necessary, returns an empty string.
        """
        return ""

    def pre_render(self) -> None:
        """
        A second initialization function, like converter_init,
        called just before rendering.
        You are permitted to examine self.function here.
        """
        pass

    def bad_argument(self, displayname: str, expected: str, *, limited_capi: bool, expected_literal: bool = True) -> str:
        assert '"' not in expected
        if limited_capi:
            if expected_literal:
                return (f'PyErr_Format(PyExc_TypeError, '
                        f'"{{{{name}}}}() {displayname} must be {expected}, not %T", '
                        f'{{argname}});')
            else:
                return (f'PyErr_Format(PyExc_TypeError, '
                        f'"{{{{name}}}}() {displayname} must be %s, not %T", '
                        f'"{expected}", {{argname}});')
        else:
            if expected_literal:
                expected = f'"{expected}"'
            self.add_include('pycore_modsupport.h', '_PyArg_BadArgument()')
            return f'_PyArg_BadArgument("{{{{name}}}}", "{displayname}", {expected}, {{argname}});'

    def format_code(self, fmt: str, *,
                    argname: str,
                    bad_argument: str | None = None,
                    bad_argument2: str | None = None,
                    **kwargs: Any) -> str:
        if '{bad_argument}' in fmt:
            if not bad_argument:
                raise TypeError("required 'bad_argument' argument")
            fmt = fmt.replace('{bad_argument}', bad_argument)
        if '{bad_argument2}' in fmt:
            if not bad_argument2:
                raise TypeError("required 'bad_argument2' argument")
            fmt = fmt.replace('{bad_argument2}', bad_argument2)
        return fmt.format(argname=argname, paramname=self.parser_name, **kwargs)

    def use_converter(self) -> None:
        """Method called when self.converter is used to parse an argument."""
        pass

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 'O&':
            self.use_converter()
            return self.format_code("""
                if (!{converter}({argname}, &{paramname})) {{{{
                    goto exit;
                }}}}
                """,
                argname=argname,
                converter=self.converter)
        if self.format_unit == 'O!':
            cast = '(%s)' % self.type if self.type != 'PyObject *' else ''
            if self.subclass_of in type_checks:
                typecheck, typename = type_checks[self.subclass_of]
                return self.format_code("""
                    if (!{typecheck}({argname})) {{{{
                        {bad_argument}
                        goto exit;
                    }}}}
                    {paramname} = {cast}{argname};
                    """,
                    argname=argname,
                    bad_argument=self.bad_argument(displayname, typename, limited_capi=limited_capi),
                    typecheck=typecheck, typename=typename, cast=cast)
            return self.format_code("""
                if (!PyObject_TypeCheck({argname}, {subclass_of})) {{{{
                    {bad_argument}
                    goto exit;
                }}}}
                {paramname} = {cast}{argname};
                """,
                argname=argname,
                bad_argument=self.bad_argument(displayname, '({subclass_of})->tp_name',
                                               expected_literal=False, limited_capi=limited_capi),
                subclass_of=self.subclass_of, cast=cast)
        if self.format_unit == 'O':
            cast = '(%s)' % self.type if self.type != 'PyObject *' else ''
            return self.format_code("""
                {paramname} = {cast}{argname};
                """,
                argname=argname, cast=cast)
        return None

    def set_template_dict(self, template_dict: TemplateDict) -> None:
        pass

    @property
    def parser_name(self) -> str:
        if self.name in libclinic.CLINIC_PREFIXED_ARGS: # bpo-39741
            return libclinic.CLINIC_PREFIX + self.name
        else:
            return self.name

    def add_include(self, name: str, reason: str,
                    *, condition: str | None = None) -> None:
        include = Include(name, reason, condition)
        self._includes.append(include)

    def get_includes(self) -> list[Include]:
        return self._includes


ConverterType = Callable[..., CConverter]
ConverterDict = dict[str, ConverterType]

# maps strings to callables.
# these callables must be of the form:
#   def foo(name, default, *, ...)
# The callable may have any number of keyword-only parameters.
# The callable must return a CConverter object.
# The callable should not call builtins.print.
converters: ConverterDict = {}

# maps strings to callables.
# these callables follow the same rules as those for "converters" above.
# note however that they will never be called with keyword-only parameters.
legacy_converters: ConverterDict = {}


def add_legacy_c_converter(
    format_unit: str,
    **kwargs: Any
) -> Callable[[CConverterClassT], CConverterClassT]:
    def closure(f: CConverterClassT) -> CConverterClassT:
        added_f: Callable[..., CConverter]
        if not kwargs:
            added_f = f
        else:
            added_f = functools.partial(f, **kwargs)
        if format_unit:
            legacy_converters[format_unit] = added_f
        return f
    return closure
