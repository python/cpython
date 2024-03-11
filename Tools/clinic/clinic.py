#!/usr/bin/env python3
#
# Argument Clinic
# Copyright 2012-2013 by Larry Hastings.
# Licensed to the PSF under a contributor agreement.
#
from __future__ import annotations

import abc
import argparse
import ast
import builtins as bltns
import collections
import contextlib
import copy
import dataclasses as dc
import enum
import functools
import inspect
import io
import itertools
import os
import pprint
import re
import shlex
import sys
import textwrap

from collections.abc import (
    Callable,
    Iterable,
    Iterator,
    Sequence,
)
from operator import attrgetter
from types import FunctionType, NoneType
from typing import (
    TYPE_CHECKING,
    Any,
    Final,
    Literal,
    NamedTuple,
    NoReturn,
    Protocol,
    TypeVar,
    cast,
    overload,
)


# Local imports.
import libclinic
import libclinic.cpp
from libclinic import ClinicError


# TODO:
#
# soon:
#
# * allow mixing any two of {positional-only, positional-or-keyword,
#   keyword-only}
#       * dict constructor uses positional-only and keyword-only
#       * max and min use positional only with an optional group
#         and keyword-only
#


# match '#define Py_LIMITED_API'
LIMITED_CAPI_REGEX = re.compile(r'#define +Py_LIMITED_API')


class Sentinels(enum.Enum):
    unspecified = "unspecified"
    unknown = "unknown"

    def __repr__(self) -> str:
        return f"<{self.value.capitalize()}>"


unspecified: Final = Sentinels.unspecified
unknown: Final = Sentinels.unknown


# This one needs to be a distinct class, unlike the other two
class Null:
    def __repr__(self) -> str:
        return '<Null>'


NULL = Null()

TemplateDict = dict[str, str]


@overload
def warn_or_fail(
    *args: object,
    fail: Literal[True],
    filename: str | None = None,
    line_number: int | None = None,
) -> NoReturn: ...

@overload
def warn_or_fail(
    *args: object,
    fail: Literal[False] = False,
    filename: str | None = None,
    line_number: int | None = None,
) -> None: ...

def warn_or_fail(
    *args: object,
    fail: bool = False,
    filename: str | None = None,
    line_number: int | None = None,
) -> None:
    joined = " ".join([str(a) for a in args])
    error = ClinicError(joined, filename=filename, lineno=line_number)
    if fail:
        raise error
    else:
        print(error.report(warn_only=True))


def warn(
    *args: object,
    filename: str | None = None,
    line_number: int | None = None,
) -> None:
    return warn_or_fail(*args, filename=filename, line_number=line_number, fail=False)

def fail(
    *args: object,
    filename: str | None = None,
    line_number: int | None = None,
) -> NoReturn:
    warn_or_fail(*args, filename=filename, line_number=line_number, fail=True)


class CRenderData:
    def __init__(self) -> None:

        # The C statements to declare variables.
        # Should be full lines with \n eol characters.
        self.declarations: list[str] = []

        # The C statements required to initialize the variables before the parse call.
        # Should be full lines with \n eol characters.
        self.initializers: list[str] = []

        # The C statements needed to dynamically modify the values
        # parsed by the parse call, before calling the impl.
        self.modifications: list[str] = []

        # The entries for the "keywords" array for PyArg_ParseTuple.
        # Should be individual strings representing the names.
        self.keywords: list[str] = []

        # The "format units" for PyArg_ParseTuple.
        # Should be individual strings that will get
        self.format_units: list[str] = []

        # The varargs arguments for PyArg_ParseTuple.
        self.parse_arguments: list[str] = []

        # The parameter declarations for the impl function.
        self.impl_parameters: list[str] = []

        # The arguments to the impl function at the time it's called.
        self.impl_arguments: list[str] = []

        # For return converters: the name of the variable that
        # should receive the value returned by the impl.
        self.return_value = "return_value"

        # For return converters: the code to convert the return
        # value from the parse function.  This is also where
        # you should check the _return_value for errors, and
        # "goto exit" if there are any.
        self.return_conversion: list[str] = []
        self.converter_retval = "_return_value"

        # The C statements required to do some operations
        # after the end of parsing but before cleaning up.
        # These operations may be, for example, memory deallocations which
        # can only be done without any error happening during argument parsing.
        self.post_parsing: list[str] = []

        # The C statements required to clean up after the impl call.
        self.cleanup: list[str] = []

        # The C statements to generate critical sections (per-object locking).
        self.lock: list[str] = []
        self.unlock: list[str] = []


class Language(metaclass=abc.ABCMeta):

    start_line = ""
    body_prefix = ""
    stop_line = ""
    checksum_line = ""

    def __init__(self, filename: str) -> None:
        self.filename = filename

    @abc.abstractmethod
    def render(
            self,
            clinic: Clinic,
            signatures: Iterable[Module | Class | Function]
    ) -> str:
        ...

    def parse_line(self, line: str) -> None:
        ...

    def validate(self) -> None:
        def assert_only_one(
                attr: str,
                *additional_fields: str
        ) -> None:
            """
            Ensures that the string found at getattr(self, attr)
            contains exactly one formatter replacement string for
            each valid field.  The list of valid fields is
            ['dsl_name'] extended by additional_fields.

            e.g.
                self.fmt = "{dsl_name} {a} {b}"

                # this passes
                self.assert_only_one('fmt', 'a', 'b')

                # this fails, the format string has a {b} in it
                self.assert_only_one('fmt', 'a')

                # this fails, the format string doesn't have a {c} in it
                self.assert_only_one('fmt', 'a', 'b', 'c')

                # this fails, the format string has two {a}s in it,
                # it must contain exactly one
                self.fmt2 = '{dsl_name} {a} {a}'
                self.assert_only_one('fmt2', 'a')

            """
            fields = ['dsl_name']
            fields.extend(additional_fields)
            line: str = getattr(self, attr)
            fcf = libclinic.FormatCounterFormatter()
            fcf.format(line)
            def local_fail(should_be_there_but_isnt: bool) -> None:
                if should_be_there_but_isnt:
                    fail("{} {} must contain {{{}}} exactly once!".format(
                        self.__class__.__name__, attr, name))
                else:
                    fail("{} {} must not contain {{{}}}!".format(
                        self.__class__.__name__, attr, name))

            for name, count in fcf.counts.items():
                if name in fields:
                    if count > 1:
                        local_fail(True)
                else:
                    local_fail(False)
            for name in fields:
                if fcf.counts.get(name) != 1:
                    local_fail(True)

        assert_only_one('start_line')
        assert_only_one('stop_line')

        field = "arguments" if "{arguments}" in self.checksum_line else "checksum"
        assert_only_one('checksum_line', field)



class PythonLanguage(Language):

    language      = 'Python'
    start_line    = "#/*[{dsl_name} input]"
    body_prefix   = "#"
    stop_line     = "#[{dsl_name} start generated code]*/"
    checksum_line = "#/*[{dsl_name} end generated code: {arguments}]*/"


ParamTuple = tuple["Parameter", ...]


def permute_left_option_groups(
        l: Sequence[Iterable[Parameter]]
) -> Iterator[ParamTuple]:
    """
    Given [(1,), (2,), (3,)], should yield:
       ()
       (3,)
       (2, 3)
       (1, 2, 3)
    """
    yield tuple()
    accumulator: list[Parameter] = []
    for group in reversed(l):
        accumulator = list(group) + accumulator
        yield tuple(accumulator)


def permute_right_option_groups(
        l: Sequence[Iterable[Parameter]]
) -> Iterator[ParamTuple]:
    """
    Given [(1,), (2,), (3,)], should yield:
      ()
      (1,)
      (1, 2)
      (1, 2, 3)
    """
    yield tuple()
    accumulator: list[Parameter] = []
    for group in l:
        accumulator.extend(group)
        yield tuple(accumulator)


def permute_optional_groups(
        left: Sequence[Iterable[Parameter]],
        required: Iterable[Parameter],
        right: Sequence[Iterable[Parameter]]
) -> tuple[ParamTuple, ...]:
    """
    Generator function that computes the set of acceptable
    argument lists for the provided iterables of
    argument groups.  (Actually it generates a tuple of tuples.)

    Algorithm: prefer left options over right options.

    If required is empty, left must also be empty.
    """
    required = tuple(required)
    if not required:
        if left:
            raise ValueError("required is empty but left is not")

    accumulator: list[ParamTuple] = []
    counts = set()
    for r in permute_right_option_groups(right):
        for l in permute_left_option_groups(left):
            t = l + required + r
            if len(t) in counts:
                continue
            counts.add(len(t))
            accumulator.append(t)

    accumulator.sort(key=len)
    return tuple(accumulator)


def declare_parser(
        f: Function,
        *,
        hasformat: bool = False,
        clinic: Clinic,
        limited_capi: bool,
) -> str:
    """
    Generates the code template for a static local PyArg_Parser variable,
    with an initializer.  For core code (incl. builtin modules) the
    kwtuple field is also statically initialized.  Otherwise
    it is initialized at runtime.
    """
    if hasformat:
        fname = ''
        format_ = '.format = "{format_units}:{name}",'
    else:
        fname = '.fname = "{name}",'
        format_ = ''

    num_keywords = len([
        p for p in f.parameters.values()
        if not p.is_positional_only() and not p.is_vararg()
    ])
    if limited_capi:
        declarations = """
            #define KWTUPLE NULL
        """
    elif num_keywords == 0:
        declarations = """
            #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
            #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
            #else
            #  define KWTUPLE NULL
            #endif
        """
    else:
        declarations = """
            #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

            #define NUM_KEYWORDS %d
            static struct {{
                PyGC_Head _this_is_not_used;
                PyObject_VAR_HEAD
                PyObject *ob_item[NUM_KEYWORDS];
            }} _kwtuple = {{
                .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
                .ob_item = {{ {keywords_py} }},
            }};
            #undef NUM_KEYWORDS
            #define KWTUPLE (&_kwtuple.ob_base.ob_base)

            #else  // !Py_BUILD_CORE
            #  define KWTUPLE NULL
            #endif  // !Py_BUILD_CORE
        """ % num_keywords

        condition = '#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)'
        clinic.add_include('pycore_gc.h', 'PyGC_Head', condition=condition)
        clinic.add_include('pycore_runtime.h', '_Py_ID()', condition=condition)

    declarations += """
            static const char * const _keywords[] = {{{keywords_c} NULL}};
            static _PyArg_Parser _parser = {{
                .keywords = _keywords,
                %s
                .kwtuple = KWTUPLE,
            }};
            #undef KWTUPLE
    """ % (format_ or fname)
    return libclinic.normalize_snippet(declarations)


class CLanguage(Language):

    body_prefix   = "#"
    language      = 'C'
    start_line    = "/*[{dsl_name} input]"
    body_prefix   = ""
    stop_line     = "[{dsl_name} start generated code]*/"
    checksum_line = "/*[{dsl_name} end generated code: {arguments}]*/"

    NO_VARARG: Final[str] = "PY_SSIZE_T_MAX"

    PARSER_PROTOTYPE_KEYWORD: Final[str] = libclinic.normalize_snippet("""
        static PyObject *
        {c_basename}({self_type}{self_name}, PyObject *args, PyObject *kwargs)
    """)
    PARSER_PROTOTYPE_KEYWORD___INIT__: Final[str] = libclinic.normalize_snippet("""
        static int
        {c_basename}({self_type}{self_name}, PyObject *args, PyObject *kwargs)
    """)
    PARSER_PROTOTYPE_VARARGS: Final[str] = libclinic.normalize_snippet("""
        static PyObject *
        {c_basename}({self_type}{self_name}, PyObject *args)
    """)
    PARSER_PROTOTYPE_FASTCALL: Final[str] = libclinic.normalize_snippet("""
        static PyObject *
        {c_basename}({self_type}{self_name}, PyObject *const *args, Py_ssize_t nargs)
    """)
    PARSER_PROTOTYPE_FASTCALL_KEYWORDS: Final[str] = libclinic.normalize_snippet("""
        static PyObject *
        {c_basename}({self_type}{self_name}, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
    """)
    PARSER_PROTOTYPE_DEF_CLASS: Final[str] = libclinic.normalize_snippet("""
        static PyObject *
        {c_basename}({self_type}{self_name}, PyTypeObject *{defining_class_name}, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
    """)
    PARSER_PROTOTYPE_NOARGS: Final[str] = libclinic.normalize_snippet("""
        static PyObject *
        {c_basename}({self_type}{self_name}, PyObject *Py_UNUSED(ignored))
    """)
    PARSER_PROTOTYPE_GETTER: Final[str] = libclinic.normalize_snippet("""
        static PyObject *
        {c_basename}({self_type}{self_name}, void *Py_UNUSED(context))
    """)
    PARSER_PROTOTYPE_SETTER: Final[str] = libclinic.normalize_snippet("""
        static int
        {c_basename}({self_type}{self_name}, PyObject *value, void *Py_UNUSED(context))
    """)
    METH_O_PROTOTYPE: Final[str] = libclinic.normalize_snippet("""
        static PyObject *
        {c_basename}({impl_parameters})
    """)
    DOCSTRING_PROTOTYPE_VAR: Final[str] = libclinic.normalize_snippet("""
        PyDoc_VAR({c_basename}__doc__);
    """)
    DOCSTRING_PROTOTYPE_STRVAR: Final[str] = libclinic.normalize_snippet("""
        PyDoc_STRVAR({c_basename}__doc__,
        {docstring});
    """)
    GETSET_DOCSTRING_PROTOTYPE_STRVAR: Final[str] = libclinic.normalize_snippet("""
        PyDoc_STRVAR({getset_basename}__doc__,
        {docstring});
        #define {getset_basename}_HAS_DOCSTR
    """)
    IMPL_DEFINITION_PROTOTYPE: Final[str] = libclinic.normalize_snippet("""
        static {impl_return_type}
        {c_basename}_impl({impl_parameters})
    """)
    METHODDEF_PROTOTYPE_DEFINE: Final[str] = libclinic.normalize_snippet(r"""
        #define {methoddef_name}    \
            {{"{name}", {methoddef_cast}{c_basename}{methoddef_cast_end}, {methoddef_flags}, {c_basename}__doc__}},
    """)
    GETTERDEF_PROTOTYPE_DEFINE: Final[str] = libclinic.normalize_snippet(r"""
        #if defined({getset_basename}_HAS_DOCSTR)
        #  define {getset_basename}_DOCSTR {getset_basename}__doc__
        #else
        #  define {getset_basename}_DOCSTR NULL
        #endif
        #if defined({getset_name}_GETSETDEF)
        #  undef {getset_name}_GETSETDEF
        #  define {getset_name}_GETSETDEF {{"{name}", (getter){getset_basename}_get, (setter){getset_basename}_set, {getset_basename}_DOCSTR}},
        #else
        #  define {getset_name}_GETSETDEF {{"{name}", (getter){getset_basename}_get, NULL, {getset_basename}_DOCSTR}},
        #endif
    """)
    SETTERDEF_PROTOTYPE_DEFINE: Final[str] = libclinic.normalize_snippet(r"""
        #if defined({getset_name}_HAS_DOCSTR)
        #  define {getset_basename}_DOCSTR {getset_basename}__doc__
        #else
        #  define {getset_basename}_DOCSTR NULL
        #endif
        #if defined({getset_name}_GETSETDEF)
        #  undef {getset_name}_GETSETDEF
        #  define {getset_name}_GETSETDEF {{"{name}", (getter){getset_basename}_get, (setter){getset_basename}_set, {getset_basename}_DOCSTR}},
        #else
        #  define {getset_name}_GETSETDEF {{"{name}", NULL, (setter){getset_basename}_set, NULL}},
        #endif
    """)
    METHODDEF_PROTOTYPE_IFNDEF: Final[str] = libclinic.normalize_snippet("""
        #ifndef {methoddef_name}
            #define {methoddef_name}
        #endif /* !defined({methoddef_name}) */
    """)
    COMPILER_DEPRECATION_WARNING_PROTOTYPE: Final[str] = r"""
        // Emit compiler warnings when we get to Python {major}.{minor}.
        #if PY_VERSION_HEX >= 0x{major:02x}{minor:02x}00C0
        #  error {message}
        #elif PY_VERSION_HEX >= 0x{major:02x}{minor:02x}00A0
        #  ifdef _MSC_VER
        #    pragma message ({message})
        #  else
        #    warning {message}
        #  endif
        #endif
    """
    DEPRECATION_WARNING_PROTOTYPE: Final[str] = r"""
        if ({condition}) {{{{{errcheck}
            if (PyErr_WarnEx(PyExc_DeprecationWarning,
                    {message}, 1))
            {{{{
                goto exit;
            }}}}
        }}}}
    """

    def __init__(self, filename: str) -> None:
        super().__init__(filename)
        self.cpp = libclinic.cpp.Monitor(filename)

    def parse_line(self, line: str) -> None:
        self.cpp.writeline(line)

    def render(
            self,
            clinic: Clinic,
            signatures: Iterable[Module | Class | Function]
    ) -> str:
        function = None
        for o in signatures:
            if isinstance(o, Function):
                if function:
                    fail("You may specify at most one function per block.\nFound a block containing at least two:\n\t" + repr(function) + " and " + repr(o))
                function = o
        return self.render_function(clinic, function)

    def compiler_deprecated_warning(
            self,
            func: Function,
            parameters: list[Parameter],
    ) -> str | None:
        minversion: VersionTuple | None = None
        for p in parameters:
            for version in p.deprecated_positional, p.deprecated_keyword:
                if version and (not minversion or minversion > version):
                    minversion = version
        if not minversion:
            return None

        # Format the preprocessor warning and error messages.
        assert isinstance(self.cpp.filename, str)
        message = f"Update the clinic input of {func.full_name!r}."
        code = self.COMPILER_DEPRECATION_WARNING_PROTOTYPE.format(
            major=minversion[0],
            minor=minversion[1],
            message=libclinic.c_repr(message),
        )
        return libclinic.normalize_snippet(code)

    def deprecate_positional_use(
            self,
            func: Function,
            params: dict[int, Parameter],
    ) -> str:
        assert len(params) > 0
        first_pos = next(iter(params))
        last_pos = next(reversed(params))

        # Format the deprecation message.
        if len(params) == 1:
            condition = f"nargs == {first_pos+1}"
            amount = f"{first_pos+1} " if first_pos else ""
            pl = "s"
        else:
            condition = f"nargs > {first_pos} && nargs <= {last_pos+1}"
            amount = f"more than {first_pos} " if first_pos else ""
            pl = "s" if first_pos != 1 else ""
        message = (
            f"Passing {amount}positional argument{pl} to "
            f"{func.fulldisplayname}() is deprecated."
        )

        for (major, minor), group in itertools.groupby(
            params.values(), key=attrgetter("deprecated_positional")
        ):
            names = [repr(p.name) for p in group]
            pstr = libclinic.pprint_words(names)
            if len(names) == 1:
                message += (
                    f" Parameter {pstr} will become a keyword-only parameter "
                    f"in Python {major}.{minor}."
                )
            else:
                message += (
                    f" Parameters {pstr} will become keyword-only parameters "
                    f"in Python {major}.{minor}."
                )

        # Append deprecation warning to docstring.
        docstring = textwrap.fill(f"Note: {message}")
        func.docstring += f"\n\n{docstring}\n"
        # Format and return the code block.
        code = self.DEPRECATION_WARNING_PROTOTYPE.format(
            condition=condition,
            errcheck="",
            message=libclinic.wrapped_c_string_literal(message, width=64,
                                                       subsequent_indent=20),
        )
        return libclinic.normalize_snippet(code, indent=4)

    def deprecate_keyword_use(
            self,
            func: Function,
            params: dict[int, Parameter],
            argname_fmt: str | None,
            *,
            fastcall: bool,
            limited_capi: bool,
            clinic: Clinic,
    ) -> str:
        assert len(params) > 0
        last_param = next(reversed(params.values()))

        # Format the deprecation message.
        containscheck = ""
        conditions = []
        for i, p in params.items():
            if p.is_optional():
                if argname_fmt:
                    conditions.append(f"nargs < {i+1} && {argname_fmt % i}")
                elif fastcall:
                    conditions.append(f"nargs < {i+1} && PySequence_Contains(kwnames, &_Py_ID({p.name}))")
                    containscheck = "PySequence_Contains"
                    clinic.add_include('pycore_runtime.h', '_Py_ID()')
                else:
                    conditions.append(f"nargs < {i+1} && PyDict_Contains(kwargs, &_Py_ID({p.name}))")
                    containscheck = "PyDict_Contains"
                    clinic.add_include('pycore_runtime.h', '_Py_ID()')
            else:
                conditions = [f"nargs < {i+1}"]
        condition = ") || (".join(conditions)
        if len(conditions) > 1:
            condition = f"(({condition}))"
        if last_param.is_optional():
            if fastcall:
                if limited_capi:
                    condition = f"kwnames && PyTuple_Size(kwnames) && {condition}"
                else:
                    condition = f"kwnames && PyTuple_GET_SIZE(kwnames) && {condition}"
            else:
                if limited_capi:
                    condition = f"kwargs && PyDict_Size(kwargs) && {condition}"
                else:
                    condition = f"kwargs && PyDict_GET_SIZE(kwargs) && {condition}"
        names = [repr(p.name) for p in params.values()]
        pstr = libclinic.pprint_words(names)
        pl = 's' if len(params) != 1 else ''
        message = (
            f"Passing keyword argument{pl} {pstr} to "
            f"{func.fulldisplayname}() is deprecated."
        )

        for (major, minor), group in itertools.groupby(
            params.values(), key=attrgetter("deprecated_keyword")
        ):
            names = [repr(p.name) for p in group]
            pstr = libclinic.pprint_words(names)
            pl = 's' if len(names) != 1 else ''
            message += (
                f" Parameter{pl} {pstr} will become positional-only "
                f"in Python {major}.{minor}."
            )

        if containscheck:
            errcheck = f"""
            if (PyErr_Occurred()) {{{{ // {containscheck}() above can fail
                goto exit;
            }}}}"""
        else:
            errcheck = ""
        if argname_fmt:
            # Append deprecation warning to docstring.
            docstring = textwrap.fill(f"Note: {message}")
            func.docstring += f"\n\n{docstring}\n"
        # Format and return the code block.
        code = self.DEPRECATION_WARNING_PROTOTYPE.format(
            condition=condition,
            errcheck=errcheck,
            message=libclinic.wrapped_c_string_literal(message, width=64,
                                                       subsequent_indent=20),
        )
        return libclinic.normalize_snippet(code, indent=4)

    def output_templates(
            self,
            f: Function,
            clinic: Clinic
    ) -> dict[str, str]:
        parameters = list(f.parameters.values())
        assert parameters
        first_param = parameters.pop(0)
        assert isinstance(first_param.converter, self_converter)
        requires_defining_class = False
        if parameters and isinstance(parameters[0].converter, defining_class_converter):
            requires_defining_class = True
            del parameters[0]
        converters = [p.converter for p in parameters]

        if f.critical_section:
            clinic.add_include('pycore_critical_section.h', 'Py_BEGIN_CRITICAL_SECTION()')
        has_option_groups = parameters and (parameters[0].group or parameters[-1].group)
        simple_return = (f.return_converter.type == 'PyObject *'
                         and not f.critical_section)
        new_or_init = f.kind.new_or_init

        vararg: int | str = self.NO_VARARG
        pos_only = min_pos = max_pos = min_kw_only = pseudo_args = 0
        for i, p in enumerate(parameters, 1):
            if p.is_keyword_only():
                assert not p.is_positional_only()
                if not p.is_optional():
                    min_kw_only = i - max_pos
            elif p.is_vararg():
                pseudo_args += 1
                vararg = i - 1
            else:
                if vararg == self.NO_VARARG:
                    max_pos = i
                if p.is_positional_only():
                    pos_only = i
                if not p.is_optional():
                    min_pos = i

        meth_o = (len(parameters) == 1 and
              parameters[0].is_positional_only() and
              not converters[0].is_optional() and
              not requires_defining_class and
              not new_or_init)

        # we have to set these things before we're done:
        #
        # docstring_prototype
        # docstring_definition
        # impl_prototype
        # methoddef_define
        # parser_prototype
        # parser_definition
        # impl_definition
        # cpp_if
        # cpp_endif
        # methoddef_ifndef

        return_value_declaration = "PyObject *return_value = NULL;"
        methoddef_define = self.METHODDEF_PROTOTYPE_DEFINE
        if new_or_init and not f.docstring:
            docstring_prototype = docstring_definition = ''
        elif f.kind is GETTER:
            methoddef_define = self.GETTERDEF_PROTOTYPE_DEFINE
            if f.docstring:
                docstring_prototype = ''
                docstring_definition = self.GETSET_DOCSTRING_PROTOTYPE_STRVAR
            else:
                docstring_prototype = docstring_definition = ''
        elif f.kind is SETTER:
            if f.docstring:
                fail("docstrings are only supported for @getter, not @setter")
            return_value_declaration = "int {return_value};"
            methoddef_define = self.SETTERDEF_PROTOTYPE_DEFINE
            docstring_prototype = docstring_definition = ''
        else:
            docstring_prototype = self.DOCSTRING_PROTOTYPE_VAR
            docstring_definition = self.DOCSTRING_PROTOTYPE_STRVAR
        impl_definition = self.IMPL_DEFINITION_PROTOTYPE
        impl_prototype = parser_prototype = parser_definition = None

        # parser_body_fields remembers the fields passed in to the
        # previous call to parser_body. this is used for an awful hack.
        parser_body_fields: tuple[str, ...] = ()
        def parser_body(
                prototype: str,
                *fields: str,
                declarations: str = ''
        ) -> str:
            nonlocal parser_body_fields
            lines = []
            lines.append(prototype)
            parser_body_fields = fields

            preamble = libclinic.normalize_snippet("""
                {{
                    {return_value_declaration}
                    {parser_declarations}
                    {declarations}
                    {initializers}
            """) + "\n"
            finale = libclinic.normalize_snippet("""
                    {modifications}
                    {lock}
                    {return_value} = {c_basename}_impl({impl_arguments});
                    {unlock}
                    {return_conversion}
                    {post_parsing}

                {exit_label}
                    {cleanup}
                    return return_value;
                }}
            """)
            for field in preamble, *fields, finale:
                lines.append(field)
            return libclinic.linear_format("\n".join(lines),
                                           parser_declarations=declarations)

        fastcall = not new_or_init
        limited_capi = clinic.limited_capi
        if limited_capi and (pseudo_args or
                (any(p.is_optional() for p in parameters) and
                 any(p.is_keyword_only() and not p.is_optional() for p in parameters)) or
                any(c.broken_limited_capi for c in converters)):
            warn(f"Function {f.full_name} cannot use limited C API")
            limited_capi = False

        parsearg: str | None
        if not parameters:
            parser_code: list[str] | None
            if f.kind is GETTER:
                flags = "" # This should end up unused
                parser_prototype = self.PARSER_PROTOTYPE_GETTER
                parser_code = []
            elif f.kind is SETTER:
                flags = ""
                parser_prototype = self.PARSER_PROTOTYPE_SETTER
                parser_code = []
            elif not requires_defining_class:
                # no parameters, METH_NOARGS
                flags = "METH_NOARGS"
                parser_prototype = self.PARSER_PROTOTYPE_NOARGS
                parser_code = []
            else:
                assert fastcall

                flags = "METH_METHOD|METH_FASTCALL|METH_KEYWORDS"
                parser_prototype = self.PARSER_PROTOTYPE_DEF_CLASS
                return_error = ('return NULL;' if simple_return
                                else 'goto exit;')
                parser_code = [libclinic.normalize_snippet("""
                    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {{
                        PyErr_SetString(PyExc_TypeError, "{name}() takes no arguments");
                        %s
                    }}
                    """ % return_error, indent=4)]

            if simple_return:
                parser_definition = '\n'.join([
                    parser_prototype,
                    '{{',
                    *parser_code,
                    '    return {c_basename}_impl({impl_arguments});',
                    '}}'])
            else:
                parser_definition = parser_body(parser_prototype, *parser_code)

        elif meth_o:
            flags = "METH_O"

            if (isinstance(converters[0], object_converter) and
                converters[0].format_unit == 'O'):
                meth_o_prototype = self.METH_O_PROTOTYPE

                if simple_return:
                    # maps perfectly to METH_O, doesn't need a return converter.
                    # so we skip making a parse function
                    # and call directly into the impl function.
                    impl_prototype = parser_prototype = parser_definition = ''
                    impl_definition = meth_o_prototype
                else:
                    # SLIGHT HACK
                    # use impl_parameters for the parser here!
                    parser_prototype = meth_o_prototype
                    parser_definition = parser_body(parser_prototype)

            else:
                argname = 'arg'
                if parameters[0].name == argname:
                    argname += '_'
                parser_prototype = libclinic.normalize_snippet("""
                    static PyObject *
                    {c_basename}({self_type}{self_name}, PyObject *%s)
                    """ % argname)

                displayname = parameters[0].get_displayname(0)
                parsearg = converters[0].parse_arg(argname, displayname, limited_capi=limited_capi)
                if parsearg is None:
                    parsearg = """
                        if (!PyArg_Parse(%s, "{format_units}:{name}", {parse_arguments})) {{
                            goto exit;
                        }}
                        """ % argname
                parser_definition = parser_body(parser_prototype,
                                                libclinic.normalize_snippet(parsearg, indent=4))

        elif has_option_groups:
            # positional parameters with option groups
            # (we have to generate lots of PyArg_ParseTuple calls
            #  in a big switch statement)

            flags = "METH_VARARGS"
            parser_prototype = self.PARSER_PROTOTYPE_VARARGS
            parser_definition = parser_body(parser_prototype, '    {option_group_parsing}')

        elif not requires_defining_class and pos_only == len(parameters) - pseudo_args:
            if fastcall:
                # positional-only, but no option groups
                # we only need one call to _PyArg_ParseStack

                flags = "METH_FASTCALL"
                parser_prototype = self.PARSER_PROTOTYPE_FASTCALL
                nargs = 'nargs'
                argname_fmt = 'args[%d]'
            else:
                # positional-only, but no option groups
                # we only need one call to PyArg_ParseTuple

                flags = "METH_VARARGS"
                parser_prototype = self.PARSER_PROTOTYPE_VARARGS
                if limited_capi:
                    nargs = 'PyTuple_Size(args)'
                    argname_fmt = 'PyTuple_GetItem(args, %d)'
                else:
                    nargs = 'PyTuple_GET_SIZE(args)'
                    argname_fmt = 'PyTuple_GET_ITEM(args, %d)'

            left_args = f"{nargs} - {max_pos}"
            max_args = self.NO_VARARG if (vararg != self.NO_VARARG) else max_pos
            if limited_capi:
                parser_code = []
                if nargs != 'nargs':
                    nargs_def = f'Py_ssize_t nargs = {nargs};'
                    parser_code.append(libclinic.normalize_snippet(nargs_def, indent=4))
                    nargs = 'nargs'
                if min_pos == max_args:
                    pl = '' if min_pos == 1 else 's'
                    parser_code.append(libclinic.normalize_snippet(f"""
                        if ({nargs} != {min_pos}) {{{{
                            PyErr_Format(PyExc_TypeError, "{{name}} expected {min_pos} argument{pl}, got %zd", {nargs});
                            goto exit;
                        }}}}
                        """,
                    indent=4))
                else:
                    if min_pos:
                        pl = '' if min_pos == 1 else 's'
                        parser_code.append(libclinic.normalize_snippet(f"""
                            if ({nargs} < {min_pos}) {{{{
                                PyErr_Format(PyExc_TypeError, "{{name}} expected at least {min_pos} argument{pl}, got %zd", {nargs});
                                goto exit;
                            }}}}
                            """,
                            indent=4))
                    if max_args != self.NO_VARARG:
                        pl = '' if max_args == 1 else 's'
                        parser_code.append(libclinic.normalize_snippet(f"""
                            if ({nargs} > {max_args}) {{{{
                                PyErr_Format(PyExc_TypeError, "{{name}} expected at most {max_args} argument{pl}, got %zd", {nargs});
                                goto exit;
                            }}}}
                            """,
                        indent=4))
            else:
                clinic.add_include('pycore_modsupport.h',
                                   '_PyArg_CheckPositional()')
                parser_code = [libclinic.normalize_snippet(f"""
                    if (!_PyArg_CheckPositional("{{name}}", {nargs}, {min_pos}, {max_args})) {{{{
                        goto exit;
                    }}}}
                    """, indent=4)]

            has_optional = False
            for i, p in enumerate(parameters):
                if p.is_vararg():
                    if fastcall:
                        parser_code.append(libclinic.normalize_snippet("""
                            %s = PyTuple_New(%s);
                            if (!%s) {{
                                goto exit;
                            }}
                            for (Py_ssize_t i = 0; i < %s; ++i) {{
                                PyTuple_SET_ITEM(%s, i, Py_NewRef(args[%d + i]));
                            }}
                            """ % (
                                p.converter.parser_name,
                                left_args,
                                p.converter.parser_name,
                                left_args,
                                p.converter.parser_name,
                                max_pos
                            ), indent=4))
                    else:
                        parser_code.append(libclinic.normalize_snippet("""
                            %s = PyTuple_GetSlice(%d, -1);
                            """ % (
                                p.converter.parser_name,
                                max_pos
                            ), indent=4))
                    continue

                displayname = p.get_displayname(i+1)
                argname = argname_fmt % i
                parsearg = p.converter.parse_arg(argname, displayname, limited_capi=limited_capi)
                if parsearg is None:
                    parser_code = None
                    break
                if has_optional or p.is_optional():
                    has_optional = True
                    parser_code.append(libclinic.normalize_snippet("""
                        if (%s < %d) {{
                            goto skip_optional;
                        }}
                        """, indent=4) % (nargs, i + 1))
                parser_code.append(libclinic.normalize_snippet(parsearg, indent=4))

            if parser_code is not None:
                if has_optional:
                    parser_code.append("skip_optional:")
            else:
                if limited_capi:
                    fastcall = False
                if fastcall:
                    clinic.add_include('pycore_modsupport.h',
                                       '_PyArg_ParseStack()')
                    parser_code = [libclinic.normalize_snippet("""
                        if (!_PyArg_ParseStack(args, nargs, "{format_units}:{name}",
                            {parse_arguments})) {{
                            goto exit;
                        }}
                        """, indent=4)]
                else:
                    flags = "METH_VARARGS"
                    parser_prototype = self.PARSER_PROTOTYPE_VARARGS
                    parser_code = [libclinic.normalize_snippet("""
                        if (!PyArg_ParseTuple(args, "{format_units}:{name}",
                            {parse_arguments})) {{
                            goto exit;
                        }}
                        """, indent=4)]
            parser_definition = parser_body(parser_prototype, *parser_code)

        else:
            deprecated_positionals: dict[int, Parameter] = {}
            deprecated_keywords: dict[int, Parameter] = {}
            for i, p in enumerate(parameters):
                if p.deprecated_positional:
                    deprecated_positionals[i] = p
                if p.deprecated_keyword:
                    deprecated_keywords[i] = p

            has_optional_kw = (
                max(pos_only, min_pos) + min_kw_only
                < len(converters) - int(vararg != self.NO_VARARG)
            )

            if limited_capi:
                parser_code = None
                fastcall = False
            else:
                if vararg == self.NO_VARARG:
                    clinic.add_include('pycore_modsupport.h',
                                       '_PyArg_UnpackKeywords()')
                    args_declaration = "_PyArg_UnpackKeywords", "%s, %s, %s" % (
                        min_pos,
                        max_pos,
                        min_kw_only
                    )
                    nargs = "nargs"
                else:
                    clinic.add_include('pycore_modsupport.h',
                                       '_PyArg_UnpackKeywordsWithVararg()')
                    args_declaration = "_PyArg_UnpackKeywordsWithVararg", "%s, %s, %s, %s" % (
                        min_pos,
                        max_pos,
                        min_kw_only,
                        vararg
                    )
                    nargs = f"Py_MIN(nargs, {max_pos})" if max_pos else "0"

                if fastcall:
                    flags = "METH_FASTCALL|METH_KEYWORDS"
                    parser_prototype = self.PARSER_PROTOTYPE_FASTCALL_KEYWORDS
                    argname_fmt = 'args[%d]'
                    declarations = declare_parser(f, clinic=clinic,
                                                  limited_capi=clinic.limited_capi)
                    declarations += "\nPyObject *argsbuf[%s];" % len(converters)
                    if has_optional_kw:
                        declarations += "\nPy_ssize_t noptargs = %s + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - %d;" % (nargs, min_pos + min_kw_only)
                    parser_code = [libclinic.normalize_snippet("""
                        args = %s(args, nargs, NULL, kwnames, &_parser, %s, argsbuf);
                        if (!args) {{
                            goto exit;
                        }}
                        """ % args_declaration, indent=4)]
                else:
                    # positional-or-keyword arguments
                    flags = "METH_VARARGS|METH_KEYWORDS"
                    parser_prototype = self.PARSER_PROTOTYPE_KEYWORD
                    argname_fmt = 'fastargs[%d]'
                    declarations = declare_parser(f, clinic=clinic,
                                                  limited_capi=clinic.limited_capi)
                    declarations += "\nPyObject *argsbuf[%s];" % len(converters)
                    declarations += "\nPyObject * const *fastargs;"
                    declarations += "\nPy_ssize_t nargs = PyTuple_GET_SIZE(args);"
                    if has_optional_kw:
                        declarations += "\nPy_ssize_t noptargs = %s + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - %d;" % (nargs, min_pos + min_kw_only)
                    parser_code = [libclinic.normalize_snippet("""
                        fastargs = %s(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, %s, argsbuf);
                        if (!fastargs) {{
                            goto exit;
                        }}
                        """ % args_declaration, indent=4)]

            if requires_defining_class:
                flags = 'METH_METHOD|' + flags
                parser_prototype = self.PARSER_PROTOTYPE_DEF_CLASS

            if parser_code is not None:
                if deprecated_keywords:
                    code = self.deprecate_keyword_use(f, deprecated_keywords, argname_fmt,
                                                      clinic=clinic,
                                                      fastcall=fastcall,
                                                      limited_capi=limited_capi)
                    parser_code.append(code)

                add_label: str | None = None
                for i, p in enumerate(parameters):
                    if isinstance(p.converter, defining_class_converter):
                        raise ValueError("defining_class should be the first "
                                        "parameter (after self)")
                    displayname = p.get_displayname(i+1)
                    parsearg = p.converter.parse_arg(argname_fmt % i, displayname, limited_capi=limited_capi)
                    if parsearg is None:
                        parser_code = None
                        break
                    if add_label and (i == pos_only or i == max_pos):
                        parser_code.append("%s:" % add_label)
                        add_label = None
                    if not p.is_optional():
                        parser_code.append(libclinic.normalize_snippet(parsearg, indent=4))
                    elif i < pos_only:
                        add_label = 'skip_optional_posonly'
                        parser_code.append(libclinic.normalize_snippet("""
                            if (nargs < %d) {{
                                goto %s;
                            }}
                            """ % (i + 1, add_label), indent=4))
                        if has_optional_kw:
                            parser_code.append(libclinic.normalize_snippet("""
                                noptargs--;
                                """, indent=4))
                        parser_code.append(libclinic.normalize_snippet(parsearg, indent=4))
                    else:
                        if i < max_pos:
                            label = 'skip_optional_pos'
                            first_opt = max(min_pos, pos_only)
                        else:
                            label = 'skip_optional_kwonly'
                            first_opt = max_pos + min_kw_only
                            if vararg != self.NO_VARARG:
                                first_opt += 1
                        if i == first_opt:
                            add_label = label
                            parser_code.append(libclinic.normalize_snippet("""
                                if (!noptargs) {{
                                    goto %s;
                                }}
                                """ % add_label, indent=4))
                        if i + 1 == len(parameters):
                            parser_code.append(libclinic.normalize_snippet(parsearg, indent=4))
                        else:
                            add_label = label
                            parser_code.append(libclinic.normalize_snippet("""
                                if (%s) {{
                                """ % (argname_fmt % i), indent=4))
                            parser_code.append(libclinic.normalize_snippet(parsearg, indent=8))
                            parser_code.append(libclinic.normalize_snippet("""
                                    if (!--noptargs) {{
                                        goto %s;
                                    }}
                                }}
                                """ % add_label, indent=4))

            if parser_code is not None:
                if add_label:
                    parser_code.append("%s:" % add_label)
            else:
                declarations = declare_parser(f, clinic=clinic,
                                              hasformat=True,
                                              limited_capi=limited_capi)
                if limited_capi:
                    # positional-or-keyword arguments
                    assert not fastcall
                    flags = "METH_VARARGS|METH_KEYWORDS"
                    parser_prototype = self.PARSER_PROTOTYPE_KEYWORD
                    parser_code = [libclinic.normalize_snippet("""
                        if (!PyArg_ParseTupleAndKeywords(args, kwargs, "{format_units}:{name}", _keywords,
                            {parse_arguments}))
                            goto exit;
                    """, indent=4)]
                    declarations = "static char *_keywords[] = {{{keywords_c} NULL}};"
                    if deprecated_positionals or deprecated_keywords:
                        declarations += "\nPy_ssize_t nargs = PyTuple_Size(args);"

                elif fastcall:
                    clinic.add_include('pycore_modsupport.h',
                                       '_PyArg_ParseStackAndKeywords()')
                    parser_code = [libclinic.normalize_snippet("""
                        if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser{parse_arguments_comma}
                            {parse_arguments})) {{
                            goto exit;
                        }}
                        """, indent=4)]
                else:
                    clinic.add_include('pycore_modsupport.h',
                                       '_PyArg_ParseTupleAndKeywordsFast()')
                    parser_code = [libclinic.normalize_snippet("""
                        if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
                            {parse_arguments})) {{
                            goto exit;
                        }}
                        """, indent=4)]
                    if deprecated_positionals or deprecated_keywords:
                        declarations += "\nPy_ssize_t nargs = PyTuple_GET_SIZE(args);"
                if deprecated_keywords:
                    code = self.deprecate_keyword_use(f, deprecated_keywords, None,
                                                      clinic=clinic,
                                                      fastcall=fastcall,
                                                      limited_capi=limited_capi)
                    parser_code.append(code)

            if deprecated_positionals:
                code = self.deprecate_positional_use(f, deprecated_positionals)
                # Insert the deprecation code before parameter parsing.
                parser_code.insert(0, code)

            assert parser_prototype is not None
            parser_definition = parser_body(parser_prototype, *parser_code,
                                            declarations=declarations)


        # Copy includes from parameters to Clinic after parse_arg() has been
        # called above.
        for converter in converters:
            for include in converter.includes:
                clinic.add_include(include.filename, include.reason,
                                   condition=include.condition)

        if new_or_init:
            methoddef_define = ''

            if f.kind is METHOD_NEW:
                parser_prototype = self.PARSER_PROTOTYPE_KEYWORD
            else:
                return_value_declaration = "int return_value = -1;"
                parser_prototype = self.PARSER_PROTOTYPE_KEYWORD___INIT__

            fields = list(parser_body_fields)
            parses_positional = 'METH_NOARGS' not in flags
            parses_keywords = 'METH_KEYWORDS' in flags
            if parses_keywords:
                assert parses_positional

            if requires_defining_class:
                raise ValueError("Slot methods cannot access their defining class.")

            if not parses_keywords:
                declarations = '{base_type_ptr}'
                clinic.add_include('pycore_modsupport.h',
                                   '_PyArg_NoKeywords()')
                fields.insert(0, libclinic.normalize_snippet("""
                    if ({self_type_check}!_PyArg_NoKeywords("{name}", kwargs)) {{
                        goto exit;
                    }}
                    """, indent=4))
                if not parses_positional:
                    clinic.add_include('pycore_modsupport.h',
                                       '_PyArg_NoPositional()')
                    fields.insert(0, libclinic.normalize_snippet("""
                        if ({self_type_check}!_PyArg_NoPositional("{name}", args)) {{
                            goto exit;
                        }}
                        """, indent=4))

            parser_definition = parser_body(parser_prototype, *fields,
                                            declarations=declarations)


        methoddef_cast_end = ""
        if flags in ('METH_NOARGS', 'METH_O', 'METH_VARARGS'):
            methoddef_cast = "(PyCFunction)"
        elif f.kind is GETTER:
            methoddef_cast = "" # This should end up unused
        elif limited_capi:
            methoddef_cast = "(PyCFunction)(void(*)(void))"
        else:
            methoddef_cast = "_PyCFunction_CAST("
            methoddef_cast_end = ")"

        if f.methoddef_flags:
            flags += '|' + f.methoddef_flags

        methoddef_define = methoddef_define.replace('{methoddef_flags}', flags)
        methoddef_define = methoddef_define.replace('{methoddef_cast}', methoddef_cast)
        methoddef_define = methoddef_define.replace('{methoddef_cast_end}', methoddef_cast_end)

        methoddef_ifndef = ''
        conditional = self.cpp.condition()
        if not conditional:
            cpp_if = cpp_endif = ''
        else:
            cpp_if = "#if " + conditional
            cpp_endif = "#endif /* " + conditional + " */"

            if methoddef_define and f.full_name not in clinic.ifndef_symbols:
                clinic.ifndef_symbols.add(f.full_name)
                methoddef_ifndef = self.METHODDEF_PROTOTYPE_IFNDEF

        # add ';' to the end of parser_prototype and impl_prototype
        # (they mustn't be None, but they could be an empty string.)
        assert parser_prototype is not None
        if parser_prototype:
            assert not parser_prototype.endswith(';')
            parser_prototype += ';'

        if impl_prototype is None:
            impl_prototype = impl_definition
        if impl_prototype:
            impl_prototype += ";"

        parser_definition = parser_definition.replace("{return_value_declaration}", return_value_declaration)

        compiler_warning = self.compiler_deprecated_warning(f, parameters)
        if compiler_warning:
            parser_definition = compiler_warning + "\n\n" + parser_definition

        d = {
            "docstring_prototype" : docstring_prototype,
            "docstring_definition" : docstring_definition,
            "impl_prototype" : impl_prototype,
            "methoddef_define" : methoddef_define,
            "parser_prototype" : parser_prototype,
            "parser_definition" : parser_definition,
            "impl_definition" : impl_definition,
            "cpp_if" : cpp_if,
            "cpp_endif" : cpp_endif,
            "methoddef_ifndef" : methoddef_ifndef,
        }

        # make sure we didn't forget to assign something,
        # and wrap each non-empty value in \n's
        d2 = {}
        for name, value in d.items():
            assert value is not None, "got a None value for template " + repr(name)
            if value:
                value = '\n' + value + '\n'
            d2[name] = value
        return d2

    @staticmethod
    def group_to_variable_name(group: int) -> str:
        adjective = "left_" if group < 0 else "right_"
        return "group_" + adjective + str(abs(group))

    def render_option_group_parsing(
            self,
            f: Function,
            template_dict: TemplateDict,
            limited_capi: bool,
    ) -> None:
        # positional only, grouped, optional arguments!
        # can be optional on the left or right.
        # here's an example:
        #
        # [ [ [ A1 A2 ] B1 B2 B3 ] C1 C2 ] D1 D2 D3 [ E1 E2 E3 [ F1 F2 F3 ] ]
        #
        # Here group D are required, and all other groups are optional.
        # (Group D's "group" is actually None.)
        # We can figure out which sets of arguments we have based on
        # how many arguments are in the tuple.
        #
        # Note that you need to count up on both sides.  For example,
        # you could have groups C+D, or C+D+E, or C+D+E+F.
        #
        # What if the number of arguments leads us to an ambiguous result?
        # Clinic prefers groups on the left.  So in the above example,
        # five arguments would map to B+C, not C+D.

        out = []
        parameters = list(f.parameters.values())
        if isinstance(parameters[0].converter, self_converter):
            del parameters[0]

        group: list[Parameter] | None = None
        left = []
        right = []
        required: list[Parameter] = []
        last: int | Literal[Sentinels.unspecified] = unspecified

        for p in parameters:
            group_id = p.group
            if group_id != last:
                last = group_id
                group = []
                if group_id < 0:
                    left.append(group)
                elif group_id == 0:
                    group = required
                else:
                    right.append(group)
            assert group is not None
            group.append(p)

        count_min = sys.maxsize
        count_max = -1

        if limited_capi:
            nargs = 'PyTuple_Size(args)'
        else:
            nargs = 'PyTuple_GET_SIZE(args)'
        out.append(f"switch ({nargs}) {{\n")
        for subset in permute_optional_groups(left, required, right):
            count = len(subset)
            count_min = min(count_min, count)
            count_max = max(count_max, count)

            if count == 0:
                out.append("""    case 0:
        break;
""")
                continue

            group_ids = {p.group for p in subset}  # eliminate duplicates
            d: dict[str, str | int] = {}
            d['count'] = count
            d['name'] = f.name
            d['format_units'] = "".join(p.converter.format_unit for p in subset)

            parse_arguments: list[str] = []
            for p in subset:
                p.converter.parse_argument(parse_arguments)
            d['parse_arguments'] = ", ".join(parse_arguments)

            group_ids.discard(0)
            lines = "\n".join([
                self.group_to_variable_name(g) + " = 1;"
                for g in group_ids
            ])

            s = """\
    case {count}:
        if (!PyArg_ParseTuple(args, "{format_units}:{name}", {parse_arguments})) {{
            goto exit;
        }}
        {group_booleans}
        break;
"""
            s = libclinic.linear_format(s, group_booleans=lines)
            s = s.format_map(d)
            out.append(s)

        out.append("    default:\n")
        s = '        PyErr_SetString(PyExc_TypeError, "{} requires {} to {} arguments");\n'
        out.append(s.format(f.full_name, count_min, count_max))
        out.append('        goto exit;\n')
        out.append("}")

        template_dict['option_group_parsing'] = libclinic.format_escape("".join(out))

    def render_function(
            self,
            clinic: Clinic,
            f: Function | None
    ) -> str:
        if f is None or clinic is None:
            return ""

        data = CRenderData()

        assert f.parameters, "We should always have a 'self' at this point!"
        parameters = f.render_parameters
        converters = [p.converter for p in parameters]

        templates = self.output_templates(f, clinic)

        f_self = parameters[0]
        selfless = parameters[1:]
        assert isinstance(f_self.converter, self_converter), "No self parameter in " + repr(f.full_name) + "!"

        if f.critical_section:
            match len(f.target_critical_section):
                case 0:
                    lock = 'Py_BEGIN_CRITICAL_SECTION({self_name});'
                    unlock = 'Py_END_CRITICAL_SECTION();'
                case 1:
                    lock = 'Py_BEGIN_CRITICAL_SECTION({target_critical_section});'
                    unlock = 'Py_END_CRITICAL_SECTION();'
                case _:
                    lock = 'Py_BEGIN_CRITICAL_SECTION2({target_critical_section});'
                    unlock = 'Py_END_CRITICAL_SECTION2();'
            data.lock.append(lock)
            data.unlock.append(unlock)

        last_group = 0
        first_optional = len(selfless)
        positional = selfless and selfless[-1].is_positional_only()
        has_option_groups = False

        # offset i by -1 because first_optional needs to ignore self
        for i, p in enumerate(parameters, -1):
            c = p.converter

            if (i != -1) and (p.default is not unspecified):
                first_optional = min(first_optional, i)

            if p.is_vararg():
                data.cleanup.append(f"Py_XDECREF({c.parser_name});")

            # insert group variable
            group = p.group
            if last_group != group:
                last_group = group
                if group:
                    group_name = self.group_to_variable_name(group)
                    data.impl_arguments.append(group_name)
                    data.declarations.append("int " + group_name + " = 0;")
                    data.impl_parameters.append("int " + group_name)
                    has_option_groups = True

            c.render(p, data)

        if has_option_groups and (not positional):
            fail("You cannot use optional groups ('[' and ']') "
                 "unless all parameters are positional-only ('/').")

        # HACK
        # when we're METH_O, but have a custom return converter,
        # we use "impl_parameters" for the parsing function
        # because that works better.  but that means we must
        # suppress actually declaring the impl's parameters
        # as variables in the parsing function.  but since it's
        # METH_O, we have exactly one anyway, so we know exactly
        # where it is.
        if ("METH_O" in templates['methoddef_define'] and
            '{impl_parameters}' in templates['parser_prototype']):
            data.declarations.pop(0)

        full_name = f.full_name
        template_dict = {'full_name': full_name}
        template_dict['name'] = f.displayname
        if f.kind in {GETTER, SETTER}:
            template_dict['getset_name'] = f.c_basename.upper()
            template_dict['getset_basename'] = f.c_basename
            if f.kind is GETTER:
                template_dict['c_basename'] = f.c_basename + "_get"
            elif f.kind is SETTER:
                template_dict['c_basename'] = f.c_basename + "_set"
                # Implicitly add the setter value parameter.
                data.impl_parameters.append("PyObject *value")
                data.impl_arguments.append("value")
        else:
            template_dict['methoddef_name'] = f.c_basename.upper() + "_METHODDEF"
            template_dict['c_basename'] = f.c_basename

        template_dict['docstring'] = libclinic.docstring_for_c_string(f.docstring)
        template_dict['self_name'] = template_dict['self_type'] = template_dict['self_type_check'] = ''
        template_dict['target_critical_section'] = ', '.join(f.target_critical_section)
        for converter in converters:
            converter.set_template_dict(template_dict)

        if f.kind not in {SETTER, METHOD_INIT}:
            f.return_converter.render(f, data)
        template_dict['impl_return_type'] = f.return_converter.type

        template_dict['declarations'] = libclinic.format_escape("\n".join(data.declarations))
        template_dict['initializers'] = "\n\n".join(data.initializers)
        template_dict['modifications'] = '\n\n'.join(data.modifications)
        template_dict['keywords_c'] = ' '.join('"' + k + '",'
                                               for k in data.keywords)
        keywords = [k for k in data.keywords if k]
        template_dict['keywords_py'] = ' '.join('&_Py_ID(' + k + '),'
                                                for k in keywords)
        template_dict['format_units'] = ''.join(data.format_units)
        template_dict['parse_arguments'] = ', '.join(data.parse_arguments)
        if data.parse_arguments:
            template_dict['parse_arguments_comma'] = ',';
        else:
            template_dict['parse_arguments_comma'] = '';
        template_dict['impl_parameters'] = ", ".join(data.impl_parameters)
        template_dict['impl_arguments'] = ", ".join(data.impl_arguments)

        template_dict['return_conversion'] = libclinic.format_escape("".join(data.return_conversion).rstrip())
        template_dict['post_parsing'] = libclinic.format_escape("".join(data.post_parsing).rstrip())
        template_dict['cleanup'] = libclinic.format_escape("".join(data.cleanup))

        template_dict['return_value'] = data.return_value
        template_dict['lock'] = "\n".join(data.lock)
        template_dict['unlock'] = "\n".join(data.unlock)

        # used by unpack tuple code generator
        unpack_min = first_optional
        unpack_max = len(selfless)
        template_dict['unpack_min'] = str(unpack_min)
        template_dict['unpack_max'] = str(unpack_max)

        if has_option_groups:
            self.render_option_group_parsing(f, template_dict,
                                             limited_capi=clinic.limited_capi)

        # buffers, not destination
        for name, destination in clinic.destination_buffers.items():
            template = templates[name]
            if has_option_groups:
                template = libclinic.linear_format(template,
                        option_group_parsing=template_dict['option_group_parsing'])
            template = libclinic.linear_format(template,
                declarations=template_dict['declarations'],
                return_conversion=template_dict['return_conversion'],
                initializers=template_dict['initializers'],
                modifications=template_dict['modifications'],
                post_parsing=template_dict['post_parsing'],
                cleanup=template_dict['cleanup'],
                lock=template_dict['lock'],
                unlock=template_dict['unlock'],
                )

            # Only generate the "exit:" label
            # if we have any gotos
            label = "exit:" if "goto exit;" in template else ""
            template = libclinic.linear_format(template, exit_label=label)

            s = template.format_map(template_dict)

            # mild hack:
            # reflow long impl declarations
            if name in {"impl_prototype", "impl_definition"}:
                s = libclinic.wrap_declarations(s)

            if clinic.line_prefix:
                s = libclinic.indent_all_lines(s, clinic.line_prefix)
            if clinic.line_suffix:
                s = libclinic.suffix_all_lines(s, clinic.line_suffix)

            destination.append(s)

        return clinic.get_destination('block').dump()


@dc.dataclass(slots=True, repr=False)
class Block:
    r"""
    Represents a single block of text embedded in
    another file.  If dsl_name is None, the block represents
    verbatim text, raw original text from the file, in
    which case "input" will be the only non-false member.
    If dsl_name is not None, the block represents a Clinic
    block.

    input is always str, with embedded \n characters.
    input represents the original text from the file;
    if it's a Clinic block, it is the original text with
    the body_prefix and redundant leading whitespace removed.

    dsl_name is either str or None.  If str, it's the text
    found on the start line of the block between the square
    brackets.

    signatures is a list.
    It may only contain clinic.Module, clinic.Class, and
    clinic.Function objects.  At the moment it should
    contain at most one of each.

    output is either str or None.  If str, it's the output
    from this block, with embedded '\n' characters.

    indent is a str.  It's the leading whitespace
    that was found on every line of input.  (If body_prefix is
    not empty, this is the indent *after* removing the
    body_prefix.)

    "indent" is different from the concept of "preindent"
    (which is not stored as state on Block objects).
    "preindent" is the whitespace that
    was found in front of every line of input *before* the
    "body_prefix" (see the Language object).  If body_prefix
    is empty, preindent must always be empty too.

    To illustrate the difference between "indent" and "preindent":

    Assume that '_' represents whitespace.
    If the block processed was in a Python file, and looked like this:
      ____#/*[python]
      ____#__for a in range(20):
      ____#____print(a)
      ____#[python]*/
    "preindent" would be "____" and "indent" would be "__".

    """
    input: str
    dsl_name: str | None = None
    signatures: list[Module | Class | Function] = dc.field(default_factory=list)
    output: Any = None  # TODO: Very dynamic; probably untypeable in its current form?
    indent: str = ''

    def __repr__(self) -> str:
        dsl_name = self.dsl_name or "text"
        def summarize(s: object) -> str:
            s = repr(s)
            if len(s) > 30:
                return s[:26] + "..." + s[0]
            return s
        parts = (
            repr(dsl_name),
            f"input={summarize(self.input)}",
            f"output={summarize(self.output)}"
        )
        return f"<clinic.Block {' '.join(parts)}>"


class BlockParser:
    """
    Block-oriented parser for Argument Clinic.
    Iterator, yields Block objects.
    """

    def __init__(
            self,
            input: str,
            language: Language,
            *,
            verify: bool = True
    ) -> None:
        """
        "input" should be a str object
        with embedded \n characters.

        "language" should be a Language object.
        """
        language.validate()

        self.input = collections.deque(reversed(input.splitlines(keepends=True)))
        self.block_start_line_number = self.line_number = 0

        self.language = language
        before, _, after = language.start_line.partition('{dsl_name}')
        assert _ == '{dsl_name}'
        self.find_start_re = libclinic.create_regex(before, after,
                                                    whole_line=False)
        self.start_re = libclinic.create_regex(before, after)
        self.verify = verify
        self.last_checksum_re: re.Pattern[str] | None = None
        self.last_dsl_name: str | None = None
        self.dsl_name: str | None = None
        self.first_block = True

    def __iter__(self) -> BlockParser:
        return self

    def __next__(self) -> Block:
        while True:
            if not self.input:
                raise StopIteration

            if self.dsl_name:
                try:
                    return_value = self.parse_clinic_block(self.dsl_name)
                except ClinicError as exc:
                    exc.filename = self.language.filename
                    exc.lineno = self.line_number
                    raise
                self.dsl_name = None
                self.first_block = False
                return return_value
            block = self.parse_verbatim_block()
            if self.first_block and not block.input:
                continue
            self.first_block = False
            return block


    def is_start_line(self, line: str) -> str | None:
        match = self.start_re.match(line.lstrip())
        return match.group(1) if match else None

    def _line(self, lookahead: bool = False) -> str:
        self.line_number += 1
        line = self.input.pop()
        if not lookahead:
            self.language.parse_line(line)
        return line

    def parse_verbatim_block(self) -> Block:
        lines = []
        self.block_start_line_number = self.line_number

        while self.input:
            line = self._line()
            dsl_name = self.is_start_line(line)
            if dsl_name:
                self.dsl_name = dsl_name
                break
            lines.append(line)

        return Block("".join(lines))

    def parse_clinic_block(self, dsl_name: str) -> Block:
        in_lines = []
        self.block_start_line_number = self.line_number + 1
        stop_line = self.language.stop_line.format(dsl_name=dsl_name)
        body_prefix = self.language.body_prefix.format(dsl_name=dsl_name)

        def is_stop_line(line: str) -> bool:
            # make sure to recognize stop line even if it
            # doesn't end with EOL (it could be the very end of the file)
            if line.startswith(stop_line):
                remainder = line.removeprefix(stop_line)
                if remainder and not remainder.isspace():
                    fail(f"Garbage after stop line: {remainder!r}")
                return True
            else:
                # gh-92256: don't allow incorrectly formatted stop lines
                if line.lstrip().startswith(stop_line):
                    fail(f"Whitespace is not allowed before the stop line: {line!r}")
                return False

        # consume body of program
        while self.input:
            line = self._line()
            if is_stop_line(line) or self.is_start_line(line):
                break
            if body_prefix:
                line = line.lstrip()
                assert line.startswith(body_prefix)
                line = line.removeprefix(body_prefix)
            in_lines.append(line)

        # consume output and checksum line, if present.
        if self.last_dsl_name == dsl_name:
            checksum_re = self.last_checksum_re
        else:
            before, _, after = self.language.checksum_line.format(dsl_name=dsl_name, arguments='{arguments}').partition('{arguments}')
            assert _ == '{arguments}'
            checksum_re = libclinic.create_regex(before, after, word=False)
            self.last_dsl_name = dsl_name
            self.last_checksum_re = checksum_re
        assert checksum_re is not None

        # scan forward for checksum line
        out_lines = []
        arguments = None
        while self.input:
            line = self._line(lookahead=True)
            match = checksum_re.match(line.lstrip())
            arguments = match.group(1) if match else None
            if arguments:
                break
            out_lines.append(line)
            if self.is_start_line(line):
                break

        output: str | None
        output = "".join(out_lines)
        if arguments:
            d = {}
            for field in shlex.split(arguments):
                name, equals, value = field.partition('=')
                if not equals:
                    fail(f"Mangled Argument Clinic marker line: {line!r}")
                d[name.strip()] = value.strip()

            if self.verify:
                if 'input' in d:
                    checksum = d['output']
                else:
                    checksum = d['checksum']

                computed = libclinic.compute_checksum(output, len(checksum))
                if checksum != computed:
                    fail("Checksum mismatch! "
                         f"Expected {checksum!r}, computed {computed!r}. "
                         "Suggested fix: remove all generated code including "
                         "the end marker, or use the '-f' option.")
        else:
            # put back output
            output_lines = output.splitlines(keepends=True)
            self.line_number -= len(output_lines)
            self.input.extend(reversed(output_lines))
            output = None

        return Block("".join(in_lines), dsl_name, output=output)


@dc.dataclass(slots=True, frozen=True)
class Include:
    """
    An include like: #include "pycore_long.h"   // _Py_ID()
    """
    # Example: "pycore_long.h".
    filename: str

    # Example: "_Py_ID()".
    reason: str

    # None means unconditional include.
    # Example: "#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)".
    condition: str | None

    def sort_key(self) -> tuple[str, str]:
        # order: '#if' comes before 'NO_CONDITION'
        return (self.condition or 'NO_CONDITION', self.filename)


@dc.dataclass(slots=True)
class BlockPrinter:
    language: Language
    f: io.StringIO = dc.field(default_factory=io.StringIO)

    # '#include "header.h"   // reason': column of '//' comment
    INCLUDE_COMMENT_COLUMN: Final[int] = 35

    def print_block(
            self,
            block: Block,
            *,
            core_includes: bool = False,
            limited_capi: bool,
            header_includes: dict[str, Include],
    ) -> None:
        input = block.input
        output = block.output
        dsl_name = block.dsl_name
        write = self.f.write

        assert not ((dsl_name is None) ^ (output is None)), "you must specify dsl_name and output together, dsl_name " + repr(dsl_name)

        if not dsl_name:
            write(input)
            return

        write(self.language.start_line.format(dsl_name=dsl_name))
        write("\n")

        body_prefix = self.language.body_prefix.format(dsl_name=dsl_name)
        if not body_prefix:
            write(input)
        else:
            for line in input.split('\n'):
                write(body_prefix)
                write(line)
                write("\n")

        write(self.language.stop_line.format(dsl_name=dsl_name))
        write("\n")

        output = ''
        if core_includes and header_includes:
            # Emit optional "#include" directives for C headers
            output += '\n'

            current_condition: str | None = None
            includes = sorted(header_includes.values(), key=Include.sort_key)
            for include in includes:
                if include.condition != current_condition:
                    if current_condition:
                        output += '#endif\n'
                    current_condition = include.condition
                    if include.condition:
                        output += f'{include.condition}\n'

                if current_condition:
                    line = f'#  include "{include.filename}"'
                else:
                    line = f'#include "{include.filename}"'
                if include.reason:
                    comment = f'// {include.reason}\n'
                    line = line.ljust(self.INCLUDE_COMMENT_COLUMN - 1) + comment
                output += line

            if current_condition:
                output += '#endif\n'

        input = ''.join(block.input)
        output += ''.join(block.output)
        if output:
            if not output.endswith('\n'):
                output += '\n'
            write(output)

        arguments = "output={output} input={input}".format(
            output=libclinic.compute_checksum(output, 16),
            input=libclinic.compute_checksum(input, 16)
        )
        write(self.language.checksum_line.format(dsl_name=dsl_name, arguments=arguments))
        write("\n")

    def write(self, text: str) -> None:
        self.f.write(text)


class BufferSeries:
    """
    Behaves like a "defaultlist".
    When you ask for an index that doesn't exist yet,
    the object grows the list until that item exists.
    So o[n] will always work.

    Supports negative indices for actual items.
    e.g. o[-1] is an element immediately preceding o[0].
    """

    def __init__(self) -> None:
        self._start = 0
        self._array: list[list[str]] = []

    def __getitem__(self, i: int) -> list[str]:
        i -= self._start
        if i < 0:
            self._start += i
            prefix: list[list[str]] = [[] for x in range(-i)]
            self._array = prefix + self._array
            i = 0
        while i >= len(self._array):
            self._array.append([])
        return self._array[i]

    def clear(self) -> None:
        for ta in self._array:
            ta.clear()

    def dump(self) -> str:
        texts = ["".join(ta) for ta in self._array]
        self.clear()
        return "".join(texts)


@dc.dataclass(slots=True, repr=False)
class Destination:
    name: str
    type: str
    clinic: Clinic
    buffers: BufferSeries = dc.field(init=False, default_factory=BufferSeries)
    filename: str = dc.field(init=False)  # set in __post_init__

    args: dc.InitVar[tuple[str, ...]] = ()

    def __post_init__(self, args: tuple[str, ...]) -> None:
        valid_types = ('buffer', 'file', 'suppress')
        if self.type not in valid_types:
            fail(
                f"Invalid destination type {self.type!r} for {self.name}, "
                f"must be {', '.join(valid_types)}"
            )
        extra_arguments = 1 if self.type == "file" else 0
        if len(args) < extra_arguments:
            fail(f"Not enough arguments for destination "
                 f"{self.name!r} new {self.type!r}")
        if len(args) > extra_arguments:
            fail(f"Too many arguments for destination {self.name!r} new {self.type!r}")
        if self.type =='file':
            d = {}
            filename = self.clinic.filename
            d['path'] = filename
            dirname, basename = os.path.split(filename)
            if not dirname:
                dirname = '.'
            d['dirname'] = dirname
            d['basename'] = basename
            d['basename_root'], d['basename_extension'] = os.path.splitext(filename)
            self.filename = args[0].format_map(d)

    def __repr__(self) -> str:
        if self.type == 'file':
            type_repr = f"type='file' file={self.filename!r}"
        else:
            type_repr = f"type={self.type!r}"
        return f"<clinic.Destination {self.name!r} {type_repr}>"

    def clear(self) -> None:
        if self.type != 'buffer':
            fail(f"Can't clear destination {self.name!r}: it's not of type 'buffer'")
        self.buffers.clear()

    def dump(self) -> str:
        return self.buffers.dump()


# "extensions" maps the file extension ("c", "py") to Language classes.
LangDict = dict[str, Callable[[str], Language]]
extensions: LangDict = { name: CLanguage for name in "c cc cpp cxx h hh hpp hxx".split() }
extensions['py'] = PythonLanguage


ClassDict = dict[str, "Class"]
DestinationDict = dict[str, Destination]
ModuleDict = dict[str, "Module"]


class Parser(Protocol):
    def __init__(self, clinic: Clinic) -> None: ...
    def parse(self, block: Block) -> None: ...


class Clinic:

    presets_text = """
preset block
everything block
methoddef_ifndef buffer 1
docstring_prototype suppress
parser_prototype suppress
cpp_if suppress
cpp_endif suppress

preset original
everything block
methoddef_ifndef buffer 1
docstring_prototype suppress
parser_prototype suppress
cpp_if suppress
cpp_endif suppress

preset file
everything file
methoddef_ifndef file 1
docstring_prototype suppress
parser_prototype suppress
impl_definition block

preset buffer
everything buffer
methoddef_ifndef buffer 1
impl_definition block
docstring_prototype suppress
impl_prototype suppress
parser_prototype suppress

preset partial-buffer
everything buffer
methoddef_ifndef buffer 1
docstring_prototype block
impl_prototype suppress
methoddef_define block
parser_prototype block
impl_definition block

"""

    def __init__(
            self,
            language: CLanguage,
            printer: BlockPrinter | None = None,
            *,
            filename: str,
            limited_capi: bool,
            verify: bool = True,
    ) -> None:
        # maps strings to Parser objects.
        # (instantiated from the "parsers" global.)
        self.parsers: dict[str, Parser] = {}
        self.language: CLanguage = language
        if printer:
            fail("Custom printers are broken right now")
        self.printer = printer or BlockPrinter(language)
        self.verify = verify
        self.limited_capi = limited_capi
        self.filename = filename
        self.modules: ModuleDict = {}
        self.classes: ClassDict = {}
        self.functions: list[Function] = []
        # dict: include name => Include instance
        self.includes: dict[str, Include] = {}

        self.line_prefix = self.line_suffix = ''

        self.destinations: DestinationDict = {}
        self.add_destination("block", "buffer")
        self.add_destination("suppress", "suppress")
        self.add_destination("buffer", "buffer")
        if filename:
            self.add_destination("file", "file", "{dirname}/clinic/{basename}.h")

        d = self.get_destination_buffer
        self.destination_buffers = {
            'cpp_if': d('file'),
            'docstring_prototype': d('suppress'),
            'docstring_definition': d('file'),
            'methoddef_define': d('file'),
            'impl_prototype': d('file'),
            'parser_prototype': d('suppress'),
            'parser_definition': d('file'),
            'cpp_endif': d('file'),
            'methoddef_ifndef': d('file', 1),
            'impl_definition': d('block'),
        }

        DestBufferType = dict[str, list[str]]
        DestBufferList = list[DestBufferType]

        self.destination_buffers_stack: DestBufferList = []
        self.ifndef_symbols: set[str] = set()

        self.presets: dict[str, dict[Any, Any]] = {}
        preset = None
        for line in self.presets_text.strip().split('\n'):
            line = line.strip()
            if not line:
                continue
            name, value, *options = line.split()
            if name == 'preset':
                self.presets[value] = preset = {}
                continue

            if len(options):
                index = int(options[0])
            else:
                index = 0
            buffer = self.get_destination_buffer(value, index)

            if name == 'everything':
                for name in self.destination_buffers:
                    preset[name] = buffer
                continue

            assert name in self.destination_buffers
            preset[name] = buffer

    def add_include(self, name: str, reason: str,
                    *, condition: str | None = None) -> None:
        try:
            existing = self.includes[name]
        except KeyError:
            pass
        else:
            if existing.condition and not condition:
                # If the previous include has a condition and the new one is
                # unconditional, override the include.
                pass
            else:
                # Already included, do nothing. Only mention a single reason,
                # no need to list all of them.
                return

        self.includes[name] = Include(name, reason, condition)

    def add_destination(
            self,
            name: str,
            type: str,
            *args: str
    ) -> None:
        if name in self.destinations:
            fail(f"Destination already exists: {name!r}")
        self.destinations[name] = Destination(name, type, self, args)

    def get_destination(self, name: str) -> Destination:
        d = self.destinations.get(name)
        if not d:
            fail(f"Destination does not exist: {name!r}")
        return d

    def get_destination_buffer(
            self,
            name: str,
            item: int = 0
    ) -> list[str]:
        d = self.get_destination(name)
        return d.buffers[item]

    def parse(self, input: str) -> str:
        printer = self.printer
        self.block_parser = BlockParser(input, self.language, verify=self.verify)
        for block in self.block_parser:
            dsl_name = block.dsl_name
            if dsl_name:
                if dsl_name not in self.parsers:
                    assert dsl_name in parsers, f"No parser to handle {dsl_name!r} block."
                    self.parsers[dsl_name] = parsers[dsl_name](self)
                parser = self.parsers[dsl_name]
                parser.parse(block)
            printer.print_block(block,
                                limited_capi=self.limited_capi,
                                header_includes=self.includes)

        # these are destinations not buffers
        for name, destination in self.destinations.items():
            if destination.type == 'suppress':
                continue
            output = destination.dump()

            if output:
                block = Block("", dsl_name="clinic", output=output)

                if destination.type == 'buffer':
                    block.input = "dump " + name + "\n"
                    warn("Destination buffer " + repr(name) + " not empty at end of file, emptying.")
                    printer.write("\n")
                    printer.print_block(block,
                                        limited_capi=self.limited_capi,
                                        header_includes=self.includes)
                    continue

                if destination.type == 'file':
                    try:
                        dirname = os.path.dirname(destination.filename)
                        try:
                            os.makedirs(dirname)
                        except FileExistsError:
                            if not os.path.isdir(dirname):
                                fail(f"Can't write to destination "
                                     f"{destination.filename!r}; "
                                     f"can't make directory {dirname!r}!")
                        if self.verify:
                            with open(destination.filename) as f:
                                parser_2 = BlockParser(f.read(), language=self.language)
                                blocks = list(parser_2)
                                if (len(blocks) != 1) or (blocks[0].input != 'preserve\n'):
                                    fail(f"Modified destination file "
                                         f"{destination.filename!r}; not overwriting!")
                    except FileNotFoundError:
                        pass

                    block.input = 'preserve\n'
                    printer_2 = BlockPrinter(self.language)
                    printer_2.print_block(block,
                                          core_includes=True,
                                          limited_capi=self.limited_capi,
                                          header_includes=self.includes)
                    libclinic.write_file(destination.filename,
                                         printer_2.f.getvalue())
                    continue

        return printer.f.getvalue()

    def _module_and_class(
        self, fields: Sequence[str]
    ) -> tuple[Module | Clinic, Class | None]:
        """
        fields should be an iterable of field names.
        returns a tuple of (module, class).
        the module object could actually be self (a clinic object).
        this function is only ever used to find the parent of where
        a new class/module should go.
        """
        parent: Clinic | Module | Class = self
        module: Clinic | Module = self
        cls: Class | None = None

        for idx, field in enumerate(fields):
            if not isinstance(parent, Class):
                if field in parent.modules:
                    parent = module = parent.modules[field]
                    continue
            if field in parent.classes:
                parent = cls = parent.classes[field]
            else:
                fullname = ".".join(fields[idx:])
                fail(f"Parent class or module {fullname!r} does not exist.")

        return module, cls

    def __repr__(self) -> str:
        return "<clinic.Clinic object>"


def parse_file(
        filename: str,
        *,
        limited_capi: bool,
        output: str | None = None,
        verify: bool = True,
) -> None:
    if not output:
        output = filename

    extension = os.path.splitext(filename)[1][1:]
    if not extension:
        raise ClinicError(f"Can't extract file type for file {filename!r}")

    try:
        language = extensions[extension](filename)
    except KeyError:
        raise ClinicError(f"Can't identify file type for file {filename!r}")

    with open(filename, encoding="utf-8") as f:
        raw = f.read()

    # exit quickly if there are no clinic markers in the file
    find_start_re = BlockParser("", language).find_start_re
    if not find_start_re.search(raw):
        return

    if LIMITED_CAPI_REGEX.search(raw):
        limited_capi = True

    assert isinstance(language, CLanguage)
    clinic = Clinic(language,
                    verify=verify,
                    filename=filename,
                    limited_capi=limited_capi)
    cooked = clinic.parse(raw)

    libclinic.write_file(output, cooked)


class PythonParser:
    def __init__(self, clinic: Clinic) -> None:
        pass

    def parse(self, block: Block) -> None:
        with contextlib.redirect_stdout(io.StringIO()) as s:
            exec(block.input)
            block.output = s.getvalue()


@dc.dataclass(repr=False)
class Module:
    name: str
    module: Module | Clinic

    def __post_init__(self) -> None:
        self.parent = self.module
        self.modules: ModuleDict = {}
        self.classes: ClassDict = {}
        self.functions: list[Function] = []

    def __repr__(self) -> str:
        return "<clinic.Module " + repr(self.name) + " at " + str(id(self)) + ">"


@dc.dataclass(repr=False)
class Class:
    name: str
    module: Module | Clinic
    cls: Class | None
    typedef: str
    type_object: str

    def __post_init__(self) -> None:
        self.parent = self.cls or self.module
        self.classes: ClassDict = {}
        self.functions: list[Function] = []

    def __repr__(self) -> str:
        return "<clinic.Class " + repr(self.name) + " at " + str(id(self)) + ">"


unsupported_special_methods: set[str] = set("""

__abs__
__add__
__and__
__call__
__delitem__
__divmod__
__eq__
__float__
__floordiv__
__ge__
__getattr__
__getattribute__
__getitem__
__gt__
__hash__
__iadd__
__iand__
__ifloordiv__
__ilshift__
__imatmul__
__imod__
__imul__
__index__
__int__
__invert__
__ior__
__ipow__
__irshift__
__isub__
__iter__
__itruediv__
__ixor__
__le__
__len__
__lshift__
__lt__
__matmul__
__mod__
__mul__
__neg__
__next__
__or__
__pos__
__pow__
__radd__
__rand__
__rdivmod__
__repr__
__rfloordiv__
__rlshift__
__rmatmul__
__rmod__
__rmul__
__ror__
__rpow__
__rrshift__
__rshift__
__rsub__
__rtruediv__
__rxor__
__setattr__
__setitem__
__str__
__sub__
__truediv__
__xor__

""".strip().split())


class FunctionKind(enum.Enum):
    INVALID         = enum.auto()
    CALLABLE        = enum.auto()
    STATIC_METHOD   = enum.auto()
    CLASS_METHOD    = enum.auto()
    METHOD_INIT     = enum.auto()
    METHOD_NEW      = enum.auto()
    GETTER          = enum.auto()
    SETTER          = enum.auto()

    @functools.cached_property
    def new_or_init(self) -> bool:
        return self in {FunctionKind.METHOD_INIT, FunctionKind.METHOD_NEW}

    def __repr__(self) -> str:
        return f"<clinic.FunctionKind.{self.name}>"


INVALID: Final = FunctionKind.INVALID
CALLABLE: Final = FunctionKind.CALLABLE
STATIC_METHOD: Final = FunctionKind.STATIC_METHOD
CLASS_METHOD: Final = FunctionKind.CLASS_METHOD
METHOD_INIT: Final = FunctionKind.METHOD_INIT
METHOD_NEW: Final = FunctionKind.METHOD_NEW
GETTER: Final = FunctionKind.GETTER
SETTER: Final = FunctionKind.SETTER

ParamDict = dict[str, "Parameter"]
ReturnConverterType = Callable[..., "CReturnConverter"]


@dc.dataclass(repr=False)
class Function:
    """
    Mutable duck type for inspect.Function.

    docstring - a str containing
        * embedded line breaks
        * text outdented to the left margin
        * no trailing whitespace.
        It will always be true that
            (not docstring) or ((not docstring[0].isspace()) and (docstring.rstrip() == docstring))
    """
    parameters: ParamDict = dc.field(default_factory=dict)
    _: dc.KW_ONLY
    name: str
    module: Module | Clinic
    cls: Class | None
    c_basename: str
    full_name: str
    return_converter: CReturnConverter
    kind: FunctionKind
    coexist: bool
    return_annotation: object = inspect.Signature.empty
    docstring: str = ''
    # docstring_only means "don't generate a machine-readable
    # signature, just a normal docstring".  it's True for
    # functions with optional groups because we can't represent
    # those accurately with inspect.Signature in 3.4.
    docstring_only: bool = False
    critical_section: bool = False
    target_critical_section: list[str] = dc.field(default_factory=list)

    def __post_init__(self) -> None:
        self.parent = self.cls or self.module
        self.self_converter: self_converter | None = None
        self.__render_parameters__: list[Parameter] | None = None

    @functools.cached_property
    def displayname(self) -> str:
        """Pretty-printable name."""
        if self.kind.new_or_init:
            assert isinstance(self.cls, Class)
            return self.cls.name
        else:
            return self.name

    @functools.cached_property
    def fulldisplayname(self) -> str:
        parent: Class | Module | Clinic | None
        if self.kind.new_or_init:
            parent = getattr(self.cls, "parent", None)
        else:
            parent = self.parent
        name = self.displayname
        while isinstance(parent, (Module, Class)):
            name = f"{parent.name}.{name}"
            parent = parent.parent
        return name

    @property
    def render_parameters(self) -> list[Parameter]:
        if not self.__render_parameters__:
            l: list[Parameter] = []
            self.__render_parameters__ = l
            for p in self.parameters.values():
                p = p.copy()
                p.converter.pre_render()
                l.append(p)
        return self.__render_parameters__

    @property
    def methoddef_flags(self) -> str | None:
        if self.kind.new_or_init:
            return None
        flags = []
        match self.kind:
            case FunctionKind.CLASS_METHOD:
                flags.append('METH_CLASS')
            case FunctionKind.STATIC_METHOD:
                flags.append('METH_STATIC')
            case _ as kind:
                acceptable_kinds = {FunctionKind.CALLABLE, FunctionKind.GETTER, FunctionKind.SETTER}
                assert kind in acceptable_kinds, f"unknown kind: {kind!r}"
        if self.coexist:
            flags.append('METH_COEXIST')
        return '|'.join(flags)

    def __repr__(self) -> str:
        return f'<clinic.Function {self.name!r}>'

    def copy(self, **overrides: Any) -> Function:
        f = dc.replace(self, **overrides)
        f.parameters = {
            name: value.copy(function=f)
            for name, value in f.parameters.items()
        }
        return f


VersionTuple = tuple[int, int]


@dc.dataclass(repr=False, slots=True)
class Parameter:
    """
    Mutable duck type of inspect.Parameter.
    """
    name: str
    kind: inspect._ParameterKind
    _: dc.KW_ONLY
    default: object = inspect.Parameter.empty
    function: Function
    converter: CConverter
    annotation: object = inspect.Parameter.empty
    docstring: str = ''
    group: int = 0
    # (`None` signifies that there is no deprecation)
    deprecated_positional: VersionTuple | None = None
    deprecated_keyword: VersionTuple | None = None
    right_bracket_count: int = dc.field(init=False, default=0)

    def __repr__(self) -> str:
        return f'<clinic.Parameter {self.name!r}>'

    def is_keyword_only(self) -> bool:
        return self.kind == inspect.Parameter.KEYWORD_ONLY

    def is_positional_only(self) -> bool:
        return self.kind == inspect.Parameter.POSITIONAL_ONLY

    def is_vararg(self) -> bool:
        return self.kind == inspect.Parameter.VAR_POSITIONAL

    def is_optional(self) -> bool:
        return not self.is_vararg() and (self.default is not unspecified)

    def copy(
        self,
        /,
        *,
        converter: CConverter | None = None,
        function: Function | None = None,
        **overrides: Any
    ) -> Parameter:
        function = function or self.function
        if not converter:
            converter = copy.copy(self.converter)
            converter.function = function
        return dc.replace(self, **overrides, function=function, converter=converter)

    def get_displayname(self, i: int) -> str:
        if i == 0:
            return 'argument'
        if not self.is_positional_only():
            return f'argument {self.name!r}'
        else:
            return f'argument {i}'

    def render_docstring(self) -> str:
        lines = [f"  {self.name}"]
        lines.extend(f"    {line}" for line in self.docstring.split("\n"))
        return "\n".join(lines).rstrip()


CConverterClassT = TypeVar("CConverterClassT", bound=type["CConverter"])

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

def add_legacy_c_converter(
        format_unit: str,
        **kwargs: Any
) -> Callable[[CConverterClassT], CConverterClassT]:
    """
    Adds a legacy converter.
    """
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
        self.includes: list[Include] = []

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
        if parameter.is_vararg():
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
        return self.parser_name + "_length"

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
                        f'"{{{{name}}}}() {displayname} must be {expected}, not %.50s", '
                        f'{{argname}} == Py_None ? "None" : Py_TYPE({{argname}})->tp_name);')
            else:
                return (f'PyErr_Format(PyExc_TypeError, '
                        f'"{{{{name}}}}() {displayname} must be %.50s, not %.50s", '
                        f'"{expected}", '
                        f'{{argname}} == Py_None ? "None" : Py_TYPE({{argname}})->tp_name);')
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

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 'O&':
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
        self.includes.append(include)

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

# maps strings to callables.
# these callables must be of the form:
#   def foo(*, ...)
# The callable may have any number of keyword-only parameters.
# The callable must return a CReturnConverter object.
# The callable should not call builtins.print.
ReturnConverterDict = dict[str, ReturnConverterType]
return_converters: ReturnConverterDict = {}

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

class byte_converter(unsigned_char_converter): pass

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
            self.add_include('pycore_abstract.h', '_PyNumber_Index()')
        elif accept == {int, NoneType}:
            self.converter = '_Py_convert_optional_to_ssize_t'
            self.add_include('pycore_abstract.h',
                             '_Py_convert_optional_to_ssize_t()')
        else:
            fail(f"Py_ssize_t_converter: illegal 'accept' argument {accept!r}")

    def parse_arg(self, argname: str, displayname: str, *, limited_capi: bool) -> str | None:
        if self.format_unit == 'n':
            if limited_capi:
                PyNumber_Index = 'PyNumber_Index'
            else:
                PyNumber_Index = '_PyNumber_Index'
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

    def converter_init(self, *, accept: TypeSet = {int, NoneType}) -> None:
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

    def converter_init(self, *, accept: TypeSet = {int, NoneType}) -> None:
        self.add_include('pycore_fileutils.h',
                         '_PyLong_FileDescriptor_Converter()')

    def _parse_arg(self, argname: str, displayname: str) -> str | None:
        return self.format_code("""
            {paramname} = PyObject_AsFileDescriptor({argname});
            if ({paramname} == -1) {{{{
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

class buffer: pass
class rwbuffer: pass
class robuffer: pass

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

    def render(
            self,
            function: Function,
            data: CRenderData
    ) -> None:
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

    def render(
            self,
            function: Function,
            data: CRenderData
    ) -> None:
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

    def render(
            self,
            function: Function,
            data: CRenderData
    ) -> None:
        self.declare(data)
        self.err_occurred_if(f"{data.converter_retval} == -1.0", data)
        data.return_conversion.append(
            f'return_value = PyFloat_FromDouble({self.cast}{data.converter_retval});\n'
        )


class float_return_converter(double_return_converter):
    type = 'float'
    cast = '(double)'


def eval_ast_expr(
        node: ast.expr,
        globals: dict[str, Any],
        *,
        filename: str = '-'
) -> Any:
    """
    Takes an ast.Expr node.  Compiles it into a function object,
    then calls the function object with 0 arguments.
    Returns the result of that function call.

    globals represents the globals dict the expression
    should see.  (There's no equivalent for "locals" here.)
    """

    if isinstance(node, ast.Expr):
        node = node.value

    expr = ast.Expression(node)
    co = compile(expr, filename, 'eval')
    fn = FunctionType(co, globals)
    return fn()


class IndentStack:
    def __init__(self) -> None:
        self.indents: list[int] = []
        self.margin: str | None = None

    def _ensure(self) -> None:
        if not self.indents:
            fail('IndentStack expected indents, but none are defined.')

    def measure(self, line: str) -> int:
        """
        Returns the length of the line's margin.
        """
        if '\t' in line:
            fail('Tab characters are illegal in the Argument Clinic DSL.')
        stripped = line.lstrip()
        if not len(stripped):
            # we can't tell anything from an empty line
            # so just pretend it's indented like our current indent
            self._ensure()
            return self.indents[-1]
        return len(line) - len(stripped)

    def infer(self, line: str) -> int:
        """
        Infer what is now the current margin based on this line.
        Returns:
            1 if we have indented (or this is the first margin)
            0 if the margin has not changed
           -N if we have dedented N times
        """
        indent = self.measure(line)
        margin = ' ' * indent
        if not self.indents:
            self.indents.append(indent)
            self.margin = margin
            return 1
        current = self.indents[-1]
        if indent == current:
            return 0
        if indent > current:
            self.indents.append(indent)
            self.margin = margin
            return 1
        # indent < current
        if indent not in self.indents:
            fail("Illegal outdent.")
        outdent_count = 0
        while indent != current:
            self.indents.pop()
            current = self.indents[-1]
            outdent_count -= 1
        self.margin = margin
        return outdent_count

    @property
    def depth(self) -> int:
        """
        Returns how many margins are currently defined.
        """
        return len(self.indents)

    def dedent(self, line: str) -> str:
        """
        Dedents a line by the currently defined margin.
        """
        assert self.margin is not None, "Cannot call .dedent() before calling .infer()"
        margin = self.margin
        indent = self.indents[-1]
        if not line.startswith(margin):
            fail('Cannot dedent; line does not start with the previous margin.')
        return line[indent:]


StateKeeper = Callable[[str], None]
ConverterArgs = dict[str, Any]

class ParamState(enum.IntEnum):
    """Parameter parsing state.

     [ [ a, b, ] c, ] d, e, f=3, [ g, h, [ i ] ]   <- line
    01   2          3       4    5           6     <- state transitions
    """
    # Before we've seen anything.
    # Legal transitions: to LEFT_SQUARE_BEFORE or REQUIRED
    START = 0

    # Left square backets before required params.
    LEFT_SQUARE_BEFORE = 1

    # In a group, before required params.
    GROUP_BEFORE = 2

    # Required params, positional-or-keyword or positional-only (we
    # don't know yet). Renumber left groups!
    REQUIRED = 3

    # Positional-or-keyword or positional-only params that now must have
    # default values.
    OPTIONAL = 4

    # In a group, after required params.
    GROUP_AFTER = 5

    # Right square brackets after required params.
    RIGHT_SQUARE_AFTER = 6


class FunctionNames(NamedTuple):
    full_name: str
    c_basename: str


class DSLParser:
    function: Function | None
    state: StateKeeper
    keyword_only: bool
    positional_only: bool
    deprecated_positional: VersionTuple | None
    deprecated_keyword: VersionTuple | None
    group: int
    parameter_state: ParamState
    indent: IndentStack
    kind: FunctionKind
    coexist: bool
    forced_text_signature: str | None
    parameter_continuation: str
    preserve_output: bool
    critical_section: bool
    target_critical_section: list[str]
    from_version_re = re.compile(r'([*/]) +\[from +(.+)\]')

    def __init__(self, clinic: Clinic) -> None:
        self.clinic = clinic

        self.directives = {}
        for name in dir(self):
            # functions that start with directive_ are added to directives
            _, s, key = name.partition("directive_")
            if s:
                self.directives[key] = getattr(self, name)

            # functions that start with at_ are too, with an @ in front
            _, s, key = name.partition("at_")
            if s:
                self.directives['@' + key] = getattr(self, name)

        self.reset()

    def reset(self) -> None:
        self.function = None
        self.state = self.state_dsl_start
        self.keyword_only = False
        self.positional_only = False
        self.deprecated_positional = None
        self.deprecated_keyword = None
        self.group = 0
        self.parameter_state: ParamState = ParamState.START
        self.indent = IndentStack()
        self.kind = CALLABLE
        self.coexist = False
        self.forced_text_signature = None
        self.parameter_continuation = ''
        self.preserve_output = False
        self.critical_section = False
        self.target_critical_section = []

    def directive_module(self, name: str) -> None:
        fields = name.split('.')[:-1]
        module, cls = self.clinic._module_and_class(fields)
        if cls:
            fail("Can't nest a module inside a class!")

        if name in module.modules:
            fail(f"Already defined module {name!r}!")

        m = Module(name, module)
        module.modules[name] = m
        self.block.signatures.append(m)

    def directive_class(
            self,
            name: str,
            typedef: str,
            type_object: str
    ) -> None:
        fields = name.split('.')
        name = fields.pop()
        module, cls = self.clinic._module_and_class(fields)

        parent = cls or module
        if name in parent.classes:
            fail(f"Already defined class {name!r}!")

        c = Class(name, module, cls, typedef, type_object)
        parent.classes[name] = c
        self.block.signatures.append(c)

    def directive_set(self, name: str, value: str) -> None:
        if name not in ("line_prefix", "line_suffix"):
            fail(f"unknown variable {name!r}")

        value = value.format_map({
            'block comment start': '/*',
            'block comment end': '*/',
            })

        self.clinic.__dict__[name] = value

    def directive_destination(
            self,
            name: str,
            command: str,
            *args: str
    ) -> None:
        match command:
            case "new":
                self.clinic.add_destination(name, *args)
            case "clear":
                self.clinic.get_destination(name).clear()
            case _:
                fail(f"unknown destination command {command!r}")


    def directive_output(
            self,
            command_or_name: str,
            destination: str = ''
    ) -> None:
        fd = self.clinic.destination_buffers

        if command_or_name == "preset":
            preset = self.clinic.presets.get(destination)
            if not preset:
                fail(f"Unknown preset {destination!r}!")
            fd.update(preset)
            return

        if command_or_name == "push":
            self.clinic.destination_buffers_stack.append(fd.copy())
            return

        if command_or_name == "pop":
            if not self.clinic.destination_buffers_stack:
                fail("Can't 'output pop', stack is empty!")
            previous_fd = self.clinic.destination_buffers_stack.pop()
            fd.update(previous_fd)
            return

        # secret command for debugging!
        if command_or_name == "print":
            self.block.output.append(pprint.pformat(fd))
            self.block.output.append('\n')
            return

        d = self.clinic.get_destination_buffer(destination)

        if command_or_name == "everything":
            for name in list(fd):
                fd[name] = d
            return

        if command_or_name not in fd:
            allowed = ["preset", "push", "pop", "print", "everything"]
            allowed.extend(fd)
            fail(f"Invalid command or destination name {command_or_name!r}. "
                 "Must be one of:\n -",
                 "\n - ".join([repr(word) for word in allowed]))
        fd[command_or_name] = d

    def directive_dump(self, name: str) -> None:
        self.block.output.append(self.clinic.get_destination(name).dump())

    def directive_printout(self, *args: str) -> None:
        self.block.output.append(' '.join(args))
        self.block.output.append('\n')

    def directive_preserve(self) -> None:
        if self.preserve_output:
            fail("Can't have 'preserve' twice in one block!")
        self.preserve_output = True

    def at_classmethod(self) -> None:
        if self.kind is not CALLABLE:
            fail("Can't set @classmethod, function is not a normal callable")
        self.kind = CLASS_METHOD

    def at_critical_section(self, *args: str) -> None:
        if len(args) > 2:
            fail("Up to 2 critical section variables are supported")
        self.target_critical_section.extend(args)
        self.critical_section = True

    def at_getter(self) -> None:
        match self.kind:
            case FunctionKind.GETTER:
                fail("Cannot apply @getter twice to the same function!")
            case FunctionKind.SETTER:
                fail("Cannot apply both @getter and @setter to the same function!")
            case _:
                self.kind = FunctionKind.GETTER

    def at_setter(self) -> None:
        match self.kind:
            case FunctionKind.SETTER:
                fail("Cannot apply @setter twice to the same function!")
            case FunctionKind.GETTER:
                fail("Cannot apply both @getter and @setter to the same function!")
            case _:
                self.kind = FunctionKind.SETTER

    def at_staticmethod(self) -> None:
        if self.kind is not CALLABLE:
            fail("Can't set @staticmethod, function is not a normal callable")
        self.kind = STATIC_METHOD

    def at_coexist(self) -> None:
        if self.coexist:
            fail("Called @coexist twice!")
        self.coexist = True

    def at_text_signature(self, text_signature: str) -> None:
        if self.forced_text_signature:
            fail("Called @text_signature twice!")
        self.forced_text_signature = text_signature

    def parse(self, block: Block) -> None:
        self.reset()
        self.block = block
        self.saved_output = self.block.output
        block.output = []
        block_start = self.clinic.block_parser.line_number
        lines = block.input.split('\n')
        for line_number, line in enumerate(lines, self.clinic.block_parser.block_start_line_number):
            if '\t' in line:
                fail(f'Tab characters are illegal in the Clinic DSL: {line!r}',
                     line_number=block_start)
            try:
                self.state(line)
            except ClinicError as exc:
                exc.lineno = line_number
                exc.filename = self.clinic.filename
                raise

        self.do_post_block_processing_cleanup(line_number)
        block.output.extend(self.clinic.language.render(self.clinic, block.signatures))

        if self.preserve_output:
            if block.output:
                fail("'preserve' only works for blocks that don't produce any output!",
                     line_number=line_number)
            block.output = self.saved_output

    def in_docstring(self) -> bool:
        """Return true if we are processing a docstring."""
        return self.state in {
            self.state_parameter_docstring,
            self.state_function_docstring,
        }

    def valid_line(self, line: str) -> bool:
        # ignore comment-only lines
        if line.lstrip().startswith('#'):
            return False

        # Ignore empty lines too
        # (but not in docstring sections!)
        if not self.in_docstring() and not line.strip():
            return False

        return True

    def next(
            self,
            state: StateKeeper,
            line: str | None = None
    ) -> None:
        self.state = state
        if line is not None:
            self.state(line)

    def state_dsl_start(self, line: str) -> None:
        if not self.valid_line(line):
            return

        # is it a directive?
        fields = shlex.split(line)
        directive_name = fields[0]
        directive = self.directives.get(directive_name, None)
        if directive:
            try:
                directive(*fields[1:])
            except TypeError as e:
                fail(str(e))
            return

        self.next(self.state_modulename_name, line)

    def parse_function_names(self, line: str) -> FunctionNames:
        left, as_, right = line.partition(' as ')
        full_name = left.strip()
        c_basename = right.strip()
        if as_ and not c_basename:
            fail("No C basename provided after 'as' keyword")
        if not c_basename:
            fields = full_name.split(".")
            if fields[-1] == '__new__':
                fields.pop()
            c_basename = "_".join(fields)
        if not libclinic.is_legal_py_identifier(full_name):
            fail(f"Illegal function name: {full_name!r}")
        if not libclinic.is_legal_c_identifier(c_basename):
            fail(f"Illegal C basename: {c_basename!r}")
        names = FunctionNames(full_name=full_name, c_basename=c_basename)
        self.normalize_function_kind(names.full_name)
        return names

    def normalize_function_kind(self, fullname: str) -> None:
        # Fetch the method name and possibly class.
        fields = fullname.split('.')
        name = fields.pop()
        _, cls = self.clinic._module_and_class(fields)

        # Check special method requirements.
        if name in unsupported_special_methods:
            fail(f"{name!r} is a special method and cannot be converted to Argument Clinic!")
        if name == '__init__' and (self.kind is not CALLABLE or not cls):
            fail(f"{name!r} must be a normal method; got '{self.kind}'!")
        if name == '__new__' and (self.kind is not CLASS_METHOD or not cls):
            fail("'__new__' must be a class method!")
        if self.kind in {GETTER, SETTER} and not cls:
            fail("@getter and @setter must be methods")

        # Normalise self.kind.
        if name == '__new__':
            self.kind = METHOD_NEW
        elif name == '__init__':
            self.kind = METHOD_INIT

    def resolve_return_converter(
        self, full_name: str, forced_converter: str
    ) -> CReturnConverter:
        if forced_converter:
            if self.kind in {GETTER, SETTER}:
                fail(f"@{self.kind.name.lower()} method cannot define a return type")
            if self.kind is METHOD_INIT:
                fail("__init__ methods cannot define a return type")
            ast_input = f"def x() -> {forced_converter}: pass"
            try:
                module_node = ast.parse(ast_input)
            except SyntaxError:
                fail(f"Badly formed annotation for {full_name!r}: {forced_converter!r}")
            function_node = module_node.body[0]
            assert isinstance(function_node, ast.FunctionDef)
            try:
                name, legacy, kwargs = self.parse_converter(function_node.returns)
                if legacy:
                    fail(f"Legacy converter {name!r} not allowed as a return converter")
                if name not in return_converters:
                    fail(f"No available return converter called {name!r}")
                return return_converters[name](**kwargs)
            except ValueError:
                fail(f"Badly formed annotation for {full_name!r}: {forced_converter!r}")

        if self.kind in {METHOD_INIT, SETTER}:
            return int_return_converter()
        return CReturnConverter()

    def parse_cloned_function(self, names: FunctionNames, existing: str) -> None:
        full_name, c_basename = names
        fields = [x.strip() for x in existing.split('.')]
        function_name = fields.pop()
        module, cls = self.clinic._module_and_class(fields)
        parent = cls or module

        for existing_function in parent.functions:
            if existing_function.name == function_name:
                break
        else:
            print(f"{cls=}, {module=}, {existing=}", file=sys.stderr)
            print(f"{(cls or module).functions=}", file=sys.stderr)
            fail(f"Couldn't find existing function {existing!r}!")

        fields = [x.strip() for x in full_name.split('.')]
        function_name = fields.pop()
        module, cls = self.clinic._module_and_class(fields)

        overrides: dict[str, Any] = {
            "name": function_name,
            "full_name": full_name,
            "module": module,
            "cls": cls,
            "c_basename": c_basename,
            "docstring": "",
        }
        if not (existing_function.kind is self.kind and
                existing_function.coexist == self.coexist):
            # Allow __new__ or __init__ methods.
            if existing_function.kind.new_or_init:
                overrides["kind"] = self.kind
                # Future enhancement: allow custom return converters
                overrides["return_converter"] = CReturnConverter()
            else:
                fail("'kind' of function and cloned function don't match! "
                     "(@classmethod/@staticmethod/@coexist)")
        function = existing_function.copy(**overrides)
        self.function = function
        self.block.signatures.append(function)
        (cls or module).functions.append(function)
        self.next(self.state_function_docstring)

    def state_modulename_name(self, line: str) -> None:
        # looking for declaration, which establishes the leftmost column
        # line should be
        #     modulename.fnname [as c_basename] [-> return annotation]
        # square brackets denote optional syntax.
        #
        # alternatively:
        #     modulename.fnname [as c_basename] = modulename.existing_fn_name
        # clones the parameters and return converter from that
        # function.  you can't modify them.  you must enter a
        # new docstring.
        #
        # (but we might find a directive first!)
        #
        # this line is permitted to start with whitespace.
        # we'll call this number of spaces F (for "function").

        assert self.valid_line(line)
        self.indent.infer(line)

        # are we cloning?
        before, equals, existing = line.rpartition('=')
        if equals:
            existing = existing.strip()
            if libclinic.is_legal_py_identifier(existing):
                # we're cloning!
                names = self.parse_function_names(before)
                return self.parse_cloned_function(names, existing)

        line, _, returns = line.partition('->')
        returns = returns.strip()
        full_name, c_basename = self.parse_function_names(line)
        return_converter = self.resolve_return_converter(full_name, returns)

        fields = [x.strip() for x in full_name.split('.')]
        function_name = fields.pop()
        module, cls = self.clinic._module_and_class(fields)

        func = Function(
            name=function_name,
            full_name=full_name,
            module=module,
            cls=cls,
            c_basename=c_basename,
            return_converter=return_converter,
            kind=self.kind,
            coexist=self.coexist,
            critical_section=self.critical_section,
            target_critical_section=self.target_critical_section
        )
        self.add_function(func)

        self.next(self.state_parameters_start)

    def add_function(self, func: Function) -> None:
        # Insert a self converter automatically.
        tp, name = correct_name_for_self(func)
        if func.cls and tp == "PyObject *":
            func.self_converter = self_converter(name, name, func,
                                                 type=func.cls.typedef)
        else:
            func.self_converter = self_converter(name, name, func)
        func.parameters[name] = Parameter(
            name,
            inspect.Parameter.POSITIONAL_ONLY,
            function=func,
            converter=func.self_converter
        )

        self.block.signatures.append(func)
        self.function = func
        (func.cls or func.module).functions.append(func)

    # Now entering the parameters section.  The rules, formally stated:
    #
    #   * All lines must be indented with spaces only.
    #   * The first line must be a parameter declaration.
    #   * The first line must be indented.
    #       * This first line establishes the indent for parameters.
    #       * We'll call this number of spaces P (for "parameter").
    #   * Thenceforth:
    #       * Lines indented with P spaces specify a parameter.
    #       * Lines indented with > P spaces are docstrings for the previous
    #         parameter.
    #           * We'll call this number of spaces D (for "docstring").
    #           * All subsequent lines indented with >= D spaces are stored as
    #             part of the per-parameter docstring.
    #           * All lines will have the first D spaces of the indent stripped
    #             before they are stored.
    #           * It's illegal to have a line starting with a number of spaces X
    #             such that P < X < D.
    #       * A line with < P spaces is the first line of the function
    #         docstring, which ends processing for parameters and per-parameter
    #         docstrings.
    #           * The first line of the function docstring must be at the same
    #             indent as the function declaration.
    #       * It's illegal to have any line in the parameters section starting
    #         with X spaces such that F < X < P.  (As before, F is the indent
    #         of the function declaration.)
    #
    # Also, currently Argument Clinic places the following restrictions on groups:
    #   * Each group must contain at least one parameter.
    #   * Each group may contain at most one group, which must be the furthest
    #     thing in the group from the required parameters.  (The nested group
    #     must be the first in the group when it's before the required
    #     parameters, and the last thing in the group when after the required
    #     parameters.)
    #   * There may be at most one (top-level) group to the left or right of
    #     the required parameters.
    #   * You must specify a slash, and it must be after all parameters.
    #     (In other words: either all parameters are positional-only,
    #      or none are.)
    #
    #  Said another way:
    #   * Each group must contain at least one parameter.
    #   * All left square brackets before the required parameters must be
    #     consecutive.  (You can't have a left square bracket followed
    #     by a parameter, then another left square bracket.  You can't
    #     have a left square bracket, a parameter, a right square bracket,
    #     and then a left square bracket.)
    #   * All right square brackets after the required parameters must be
    #     consecutive.
    #
    # These rules are enforced with a single state variable:
    # "parameter_state".  (Previously the code was a miasma of ifs and
    # separate boolean state variables.)  The states are defined in the
    # ParamState class.

    def state_parameters_start(self, line: str) -> None:
        if not self.valid_line(line):
            return

        # if this line is not indented, we have no parameters
        if not self.indent.infer(line):
            return self.next(self.state_function_docstring, line)

        assert self.function is not None
        if self.function.kind in {GETTER, SETTER}:
            getset = self.function.kind.name.lower()
            fail(f"@{getset} methods cannot define parameters")

        self.parameter_continuation = ''
        return self.next(self.state_parameter, line)


    def to_required(self) -> None:
        """
        Transition to the "required" parameter state.
        """
        if self.parameter_state is not ParamState.REQUIRED:
            self.parameter_state = ParamState.REQUIRED
            assert self.function is not None
            for p in self.function.parameters.values():
                p.group = -p.group

    def state_parameter(self, line: str) -> None:
        assert isinstance(self.function, Function)

        if not self.valid_line(line):
            return

        if self.parameter_continuation:
            line = self.parameter_continuation + ' ' + line.lstrip()
            self.parameter_continuation = ''

        assert self.indent.depth == 2
        indent = self.indent.infer(line)
        if indent == -1:
            # we outdented, must be to definition column
            return self.next(self.state_function_docstring, line)

        if indent == 1:
            # we indented, must be to new parameter docstring column
            return self.next(self.state_parameter_docstring_start, line)

        line = line.rstrip()
        if line.endswith('\\'):
            self.parameter_continuation = line[:-1]
            return

        line = line.lstrip()
        version: VersionTuple | None = None
        match = self.from_version_re.fullmatch(line)
        if match:
            line = match[1]
            version = self.parse_version(match[2])

        func = self.function
        match line:
            case '*':
                self.parse_star(func, version)
            case '[':
                self.parse_opening_square_bracket(func)
            case ']':
                self.parse_closing_square_bracket(func)
            case '/':
                self.parse_slash(func, version)
            case param:
                self.parse_parameter(param)

    def parse_parameter(self, line: str) -> None:
        assert self.function is not None

        match self.parameter_state:
            case ParamState.START | ParamState.REQUIRED:
                self.to_required()
            case ParamState.LEFT_SQUARE_BEFORE:
                self.parameter_state = ParamState.GROUP_BEFORE
            case ParamState.GROUP_BEFORE:
                if not self.group:
                    self.to_required()
            case ParamState.GROUP_AFTER | ParamState.OPTIONAL:
                pass
            case st:
                fail(f"Function {self.function.name} has an unsupported group configuration. (Unexpected state {st}.a)")

        # handle "as" for  parameters too
        c_name = None
        name, have_as_token, trailing = line.partition(' as ')
        if have_as_token:
            name = name.strip()
            if ' ' not in name:
                fields = trailing.strip().split(' ')
                if not fields:
                    fail("Invalid 'as' clause!")
                c_name = fields[0]
                if c_name.endswith(':'):
                    name += ':'
                    c_name = c_name[:-1]
                fields[0] = name
                line = ' '.join(fields)

        default: str | None
        base, equals, default = line.rpartition('=')
        if not equals:
            base = default
            default = None

        module = None
        try:
            ast_input = f"def x({base}): pass"
            module = ast.parse(ast_input)
        except SyntaxError:
            try:
                # the last = was probably inside a function call, like
                #   c: int(accept={str})
                # so assume there was no actual default value.
                default = None
                ast_input = f"def x({line}): pass"
                module = ast.parse(ast_input)
            except SyntaxError:
                pass
        if not module:
            fail(f"Function {self.function.name!r} has an invalid parameter declaration:\n\t",
                 repr(line))

        function = module.body[0]
        assert isinstance(function, ast.FunctionDef)
        function_args = function.args

        if len(function_args.args) > 1:
            fail(f"Function {self.function.name!r} has an "
                 f"invalid parameter declaration (comma?): {line!r}")
        if function_args.defaults or function_args.kw_defaults:
            fail(f"Function {self.function.name!r} has an "
                 f"invalid parameter declaration (default value?): {line!r}")
        if function_args.kwarg:
            fail(f"Function {self.function.name!r} has an "
                 f"invalid parameter declaration (**kwargs?): {line!r}")

        if function_args.vararg:
            if any(p.is_vararg() for p in self.function.parameters.values()):
                fail("Too many var args")
            is_vararg = True
            parameter = function_args.vararg
        else:
            is_vararg = False
            parameter = function_args.args[0]

        parameter_name = parameter.arg
        name, legacy, kwargs = self.parse_converter(parameter.annotation)

        value: object
        if not default:
            if self.parameter_state is ParamState.OPTIONAL:
                fail(f"Can't have a parameter without a default ({parameter_name!r}) "
                      "after a parameter with a default!")
            if is_vararg:
                value = NULL
                kwargs.setdefault('c_default', "NULL")
            else:
                value = unspecified
            if 'py_default' in kwargs:
                fail("You can't specify py_default without specifying a default value!")
        else:
            if is_vararg:
                fail("Vararg can't take a default value!")

            if self.parameter_state is ParamState.REQUIRED:
                self.parameter_state = ParamState.OPTIONAL
            default = default.strip()
            bad = False
            ast_input = f"x = {default}"
            try:
                module = ast.parse(ast_input)

                if 'c_default' not in kwargs:
                    # we can only represent very simple data values in C.
                    # detect whether default is okay, via a denylist
                    # of disallowed ast nodes.
                    class DetectBadNodes(ast.NodeVisitor):
                        bad = False
                        def bad_node(self, node: ast.AST) -> None:
                            self.bad = True

                        # inline function call
                        visit_Call = bad_node
                        # inline if statement ("x = 3 if y else z")
                        visit_IfExp = bad_node

                        # comprehensions and generator expressions
                        visit_ListComp = visit_SetComp = bad_node
                        visit_DictComp = visit_GeneratorExp = bad_node

                        # literals for advanced types
                        visit_Dict = visit_Set = bad_node
                        visit_List = visit_Tuple = bad_node

                        # "starred": "a = [1, 2, 3]; *a"
                        visit_Starred = bad_node

                    denylist = DetectBadNodes()
                    denylist.visit(module)
                    bad = denylist.bad
                else:
                    # if they specify a c_default, we can be more lenient about the default value.
                    # but at least make an attempt at ensuring it's a valid expression.
                    try:
                        value = eval(default)
                    except NameError:
                        pass # probably a named constant
                    except Exception as e:
                        fail("Malformed expression given as default value "
                             f"{default!r} caused {e!r}")
                    else:
                        if value is unspecified:
                            fail("'unspecified' is not a legal default value!")
                if bad:
                    fail(f"Unsupported expression as default value: {default!r}")

                assignment = module.body[0]
                assert isinstance(assignment, ast.Assign)
                expr = assignment.value
                # mild hack: explicitly support NULL as a default value
                c_default: str | None
                if isinstance(expr, ast.Name) and expr.id == 'NULL':
                    value = NULL
                    py_default = '<unrepresentable>'
                    c_default = "NULL"
                elif (isinstance(expr, ast.BinOp) or
                    (isinstance(expr, ast.UnaryOp) and
                     not (isinstance(expr.operand, ast.Constant) and
                          type(expr.operand.value) in {int, float, complex})
                    )):
                    c_default = kwargs.get("c_default")
                    if not (isinstance(c_default, str) and c_default):
                        fail(f"When you specify an expression ({default!r}) "
                             f"as your default value, "
                             f"you MUST specify a valid c_default.",
                             ast.dump(expr))
                    py_default = default
                    value = unknown
                elif isinstance(expr, ast.Attribute):
                    a = []
                    n: ast.expr | ast.Attribute = expr
                    while isinstance(n, ast.Attribute):
                        a.append(n.attr)
                        n = n.value
                    if not isinstance(n, ast.Name):
                        fail(f"Unsupported default value {default!r} "
                             "(looked like a Python constant)")
                    a.append(n.id)
                    py_default = ".".join(reversed(a))

                    c_default = kwargs.get("c_default")
                    if not (isinstance(c_default, str) and c_default):
                        fail(f"When you specify a named constant ({py_default!r}) "
                             "as your default value, "
                             "you MUST specify a valid c_default.")

                    try:
                        value = eval(py_default)
                    except NameError:
                        value = unknown
                else:
                    value = ast.literal_eval(expr)
                    py_default = repr(value)
                    if isinstance(value, (bool, NoneType)):
                        c_default = "Py_" + py_default
                    elif isinstance(value, str):
                        c_default = libclinic.c_repr(value)
                    else:
                        c_default = py_default

            except SyntaxError as e:
                fail(f"Syntax error: {e.text!r}")
            except (ValueError, AttributeError):
                value = unknown
                c_default = kwargs.get("c_default")
                py_default = default
                if not (isinstance(c_default, str) and c_default):
                    fail("When you specify a named constant "
                         f"({py_default!r}) as your default value, "
                         "you MUST specify a valid c_default.")

            kwargs.setdefault('c_default', c_default)
            kwargs.setdefault('py_default', py_default)

        dict = legacy_converters if legacy else converters
        legacy_str = "legacy " if legacy else ""
        if name not in dict:
            fail(f'{name!r} is not a valid {legacy_str}converter')
        # if you use a c_name for the parameter, we just give that name to the converter
        # but the parameter object gets the python name
        converter = dict[name](c_name or parameter_name, parameter_name, self.function, value, **kwargs)

        kind: inspect._ParameterKind
        if is_vararg:
            kind = inspect.Parameter.VAR_POSITIONAL
        elif self.keyword_only:
            kind = inspect.Parameter.KEYWORD_ONLY
        else:
            kind = inspect.Parameter.POSITIONAL_OR_KEYWORD

        if isinstance(converter, self_converter):
            if len(self.function.parameters) == 1:
                if self.parameter_state is not ParamState.REQUIRED:
                    fail("A 'self' parameter cannot be marked optional.")
                if value is not unspecified:
                    fail("A 'self' parameter cannot have a default value.")
                if self.group:
                    fail("A 'self' parameter cannot be in an optional group.")
                kind = inspect.Parameter.POSITIONAL_ONLY
                self.parameter_state = ParamState.START
                self.function.parameters.clear()
            else:
                fail("A 'self' parameter, if specified, must be the "
                     "very first thing in the parameter block.")

        if isinstance(converter, defining_class_converter):
            _lp = len(self.function.parameters)
            if _lp == 1:
                if self.parameter_state is not ParamState.REQUIRED:
                    fail("A 'defining_class' parameter cannot be marked optional.")
                if value is not unspecified:
                    fail("A 'defining_class' parameter cannot have a default value.")
                if self.group:
                    fail("A 'defining_class' parameter cannot be in an optional group.")
            else:
                fail("A 'defining_class' parameter, if specified, must either "
                     "be the first thing in the parameter block, or come just "
                     "after 'self'.")


        p = Parameter(parameter_name, kind, function=self.function,
                      converter=converter, default=value, group=self.group,
                      deprecated_positional=self.deprecated_positional)

        names = [k.name for k in self.function.parameters.values()]
        if parameter_name in names[1:]:
            fail(f"You can't have two parameters named {parameter_name!r}!")
        elif names and parameter_name == names[0] and c_name is None:
            fail(f"Parameter {parameter_name!r} requires a custom C name")

        key = f"{parameter_name}_as_{c_name}" if c_name else parameter_name
        self.function.parameters[key] = p

    @staticmethod
    def parse_converter(
            annotation: ast.expr | None
    ) -> tuple[str, bool, ConverterArgs]:
        match annotation:
            case ast.Constant(value=str() as value):
                return value, True, {}
            case ast.Name(name):
                return name, False, {}
            case ast.Call(func=ast.Name(name)):
                symbols = globals()
                kwargs: ConverterArgs = {}
                for node in annotation.keywords:
                    if not isinstance(node.arg, str):
                        fail("Cannot use a kwarg splat in a function-call annotation")
                    kwargs[node.arg] = eval_ast_expr(node.value, symbols)
                return name, False, kwargs
            case _:
                fail(
                    "Annotations must be either a name, a function call, or a string."
                )

    def parse_version(self, thenceforth: str) -> VersionTuple:
        """Parse Python version in `[from ...]` marker."""
        assert isinstance(self.function, Function)

        try:
            major, minor = thenceforth.split(".")
            return int(major), int(minor)
        except ValueError:
            fail(
                f"Function {self.function.name!r}: expected format '[from major.minor]' "
                f"where 'major' and 'minor' are integers; got {thenceforth!r}"
            )

    def parse_star(self, function: Function, version: VersionTuple | None) -> None:
        """Parse keyword-only parameter marker '*'.

        The 'version' parameter signifies the future version from which
        the marker will take effect (None means it is already in effect).
        """
        if version is None:
            if self.keyword_only:
                fail(f"Function {function.name!r} uses '*' more than once.")
            self.check_previous_star()
            self.check_remaining_star()
            self.keyword_only = True
        else:
            if self.keyword_only:
                fail(f"Function {function.name!r}: '* [from ...]' must precede '*'")
            if self.deprecated_positional:
                if self.deprecated_positional == version:
                    fail(f"Function {function.name!r} uses '* [from "
                         f"{version[0]}.{version[1]}]' more than once.")
                if self.deprecated_positional < version:
                    fail(f"Function {function.name!r}: '* [from "
                         f"{version[0]}.{version[1]}]' must precede '* [from "
                         f"{self.deprecated_positional[0]}.{self.deprecated_positional[1]}]'")
        self.deprecated_positional = version

    def parse_opening_square_bracket(self, function: Function) -> None:
        """Parse opening parameter group symbol '['."""
        match self.parameter_state:
            case ParamState.START | ParamState.LEFT_SQUARE_BEFORE:
                self.parameter_state = ParamState.LEFT_SQUARE_BEFORE
            case ParamState.REQUIRED | ParamState.GROUP_AFTER:
                self.parameter_state = ParamState.GROUP_AFTER
            case st:
                fail(f"Function {function.name!r} "
                     f"has an unsupported group configuration. "
                     f"(Unexpected state {st}.b)")
        self.group += 1
        function.docstring_only = True

    def parse_closing_square_bracket(self, function: Function) -> None:
        """Parse closing parameter group symbol ']'."""
        if not self.group:
            fail(f"Function {function.name!r} has a ']' without a matching '['.")
        if not any(p.group == self.group for p in function.parameters.values()):
            fail(f"Function {function.name!r} has an empty group. "
                 "All groups must contain at least one parameter.")
        self.group -= 1
        match self.parameter_state:
            case ParamState.LEFT_SQUARE_BEFORE | ParamState.GROUP_BEFORE:
                self.parameter_state = ParamState.GROUP_BEFORE
            case ParamState.GROUP_AFTER | ParamState.RIGHT_SQUARE_AFTER:
                self.parameter_state = ParamState.RIGHT_SQUARE_AFTER
            case st:
                fail(f"Function {function.name!r} "
                     f"has an unsupported group configuration. "
                     f"(Unexpected state {st}.c)")

    def parse_slash(self, function: Function, version: VersionTuple | None) -> None:
        """Parse positional-only parameter marker '/'.

        The 'version' parameter signifies the future version from which
        the marker will take effect (None means it is already in effect).
        """
        if version is None:
            if self.deprecated_keyword:
                fail(f"Function {function.name!r}: '/' must precede '/ [from ...]'")
            if self.deprecated_positional:
                fail(f"Function {function.name!r}: '/' must precede '* [from ...]'")
            if self.keyword_only:
                fail(f"Function {function.name!r}: '/' must precede '*'")
            if self.positional_only:
                fail(f"Function {function.name!r} uses '/' more than once.")
        else:
            if self.deprecated_keyword:
                if self.deprecated_keyword == version:
                    fail(f"Function {function.name!r} uses '/ [from "
                         f"{version[0]}.{version[1]}]' more than once.")
                if self.deprecated_keyword > version:
                    fail(f"Function {function.name!r}: '/ [from "
                         f"{version[0]}.{version[1]}]' must precede '/ [from "
                         f"{self.deprecated_keyword[0]}.{self.deprecated_keyword[1]}]'")
            if self.deprecated_positional:
                fail(f"Function {function.name!r}: '/ [from ...]' must precede '* [from ...]'")
            if self.keyword_only:
                fail(f"Function {function.name!r}: '/ [from ...]' must precede '*'")
        self.positional_only = True
        self.deprecated_keyword = version
        if version is not None:
            found = False
            for p in reversed(function.parameters.values()):
                found = p.kind is inspect.Parameter.POSITIONAL_OR_KEYWORD
                break
            if not found:
                fail(f"Function {function.name!r} specifies '/ [from ...]' "
                     f"without preceding parameters.")
        # REQUIRED and OPTIONAL are allowed here, that allows positional-only
        # without option groups to work (and have default values!)
        allowed = {
            ParamState.REQUIRED,
            ParamState.OPTIONAL,
            ParamState.RIGHT_SQUARE_AFTER,
            ParamState.GROUP_BEFORE,
        }
        if (self.parameter_state not in allowed) or self.group:
            fail(f"Function {function.name!r} has an unsupported group configuration. "
                 f"(Unexpected state {self.parameter_state}.d)")
        # fixup preceding parameters
        for p in function.parameters.values():
            if p.kind is inspect.Parameter.POSITIONAL_OR_KEYWORD:
                if version is None:
                    p.kind = inspect.Parameter.POSITIONAL_ONLY
                elif p.deprecated_keyword is None:
                    p.deprecated_keyword = version

    def state_parameter_docstring_start(self, line: str) -> None:
        assert self.indent.margin is not None, "self.margin.infer() has not yet been called to set the margin"
        self.parameter_docstring_indent = len(self.indent.margin)
        assert self.indent.depth == 3
        return self.next(self.state_parameter_docstring, line)

    def docstring_append(self, obj: Function | Parameter, line: str) -> None:
        """Add a rstripped line to the current docstring."""
        # gh-80282: We filter out non-ASCII characters from the docstring,
        # since historically, some compilers may balk on non-ASCII input.
        # If you're using Argument Clinic in an external project,
        # you may not need to support the same array of platforms as CPython,
        # so you may be able to remove this restriction.
        matches = re.finditer(r'[^\x00-\x7F]', line)
        if offending := ", ".join([repr(m[0]) for m in matches]):
            warn("Non-ascii characters are not allowed in docstrings:",
                 offending)

        docstring = obj.docstring
        if docstring:
            docstring += "\n"
        if stripped := line.rstrip():
            docstring += self.indent.dedent(stripped)
        obj.docstring = docstring

    # every line of the docstring must start with at least F spaces,
    # where F > P.
    # these F spaces will be stripped.
    def state_parameter_docstring(self, line: str) -> None:
        if not self.valid_line(line):
            return

        indent = self.indent.measure(line)
        if indent < self.parameter_docstring_indent:
            self.indent.infer(line)
            assert self.indent.depth < 3
            if self.indent.depth == 2:
                # back to a parameter
                return self.next(self.state_parameter, line)
            assert self.indent.depth == 1
            return self.next(self.state_function_docstring, line)

        assert self.function and self.function.parameters
        last_param = next(reversed(self.function.parameters.values()))
        self.docstring_append(last_param, line)

    # the final stanza of the DSL is the docstring.
    def state_function_docstring(self, line: str) -> None:
        assert self.function is not None

        if self.group:
            fail(f"Function {self.function.name!r} has a ']' without a matching '['.")

        if not self.valid_line(line):
            return

        self.docstring_append(self.function, line)

    def format_docstring_signature(
        self, f: Function, parameters: list[Parameter]
    ) -> str:
        lines = []
        lines.append(f.displayname)
        if self.forced_text_signature:
            lines.append(self.forced_text_signature)
        elif f.kind in {GETTER, SETTER}:
            # @getter and @setter do not need signatures like a method or a function.
            return ''
        else:
            lines.append('(')

            # populate "right_bracket_count" field for every parameter
            assert parameters, "We should always have a self parameter. " + repr(f)
            assert isinstance(parameters[0].converter, self_converter)
            # self is always positional-only.
            assert parameters[0].is_positional_only()
            assert parameters[0].right_bracket_count == 0
            positional_only = True
            for p in parameters[1:]:
                if not p.is_positional_only():
                    positional_only = False
                else:
                    assert positional_only
                if positional_only:
                    p.right_bracket_count = abs(p.group)
                else:
                    # don't put any right brackets around non-positional-only parameters, ever.
                    p.right_bracket_count = 0

            right_bracket_count = 0

            def fix_right_bracket_count(desired: int) -> str:
                nonlocal right_bracket_count
                s = ''
                while right_bracket_count < desired:
                    s += '['
                    right_bracket_count += 1
                while right_bracket_count > desired:
                    s += ']'
                    right_bracket_count -= 1
                return s

            need_slash = False
            added_slash = False
            need_a_trailing_slash = False

            # we only need a trailing slash:
            #   * if this is not a "docstring_only" signature
            #   * and if the last *shown* parameter is
            #     positional only
            if not f.docstring_only:
                for p in reversed(parameters):
                    if not p.converter.show_in_signature:
                        continue
                    if p.is_positional_only():
                        need_a_trailing_slash = True
                    break


            added_star = False

            first_parameter = True
            last_p = parameters[-1]
            line_length = len(''.join(lines))
            indent = " " * line_length
            def add_parameter(text: str) -> None:
                nonlocal line_length
                nonlocal first_parameter
                if first_parameter:
                    s = text
                    first_parameter = False
                else:
                    s = ' ' + text
                    if line_length + len(s) >= 72:
                        lines.extend(["\n", indent])
                        line_length = len(indent)
                        s = text
                line_length += len(s)
                lines.append(s)

            for p in parameters:
                if not p.converter.show_in_signature:
                    continue
                assert p.name

                is_self = isinstance(p.converter, self_converter)
                if is_self and f.docstring_only:
                    # this isn't a real machine-parsable signature,
                    # so let's not print the "self" parameter
                    continue

                if p.is_positional_only():
                    need_slash = not f.docstring_only
                elif need_slash and not (added_slash or p.is_positional_only()):
                    added_slash = True
                    add_parameter('/,')

                if p.is_keyword_only() and not added_star:
                    added_star = True
                    add_parameter('*,')

                p_lines = [fix_right_bracket_count(p.right_bracket_count)]

                if isinstance(p.converter, self_converter):
                    # annotate first parameter as being a "self".
                    #
                    # if inspect.Signature gets this function,
                    # and it's already bound, the self parameter
                    # will be stripped off.
                    #
                    # if it's not bound, it should be marked
                    # as positional-only.
                    #
                    # note: we don't print "self" for __init__,
                    # because this isn't actually the signature
                    # for __init__.  (it can't be, __init__ doesn't
                    # have a docstring.)  if this is an __init__
                    # (or __new__), then this signature is for
                    # calling the class to construct a new instance.
                    p_lines.append('$')

                if p.is_vararg():
                    p_lines.append("*")

                name = p.converter.signature_name or p.name
                p_lines.append(name)

                if not p.is_vararg() and p.converter.is_optional():
                    p_lines.append('=')
                    value = p.converter.py_default
                    if not value:
                        value = repr(p.converter.default)
                    p_lines.append(value)

                if (p != last_p) or need_a_trailing_slash:
                    p_lines.append(',')

                p_output = "".join(p_lines)
                add_parameter(p_output)

            lines.append(fix_right_bracket_count(0))
            if need_a_trailing_slash:
                add_parameter('/')
            lines.append(')')

        # PEP 8 says:
        #
        #     The Python standard library will not use function annotations
        #     as that would result in a premature commitment to a particular
        #     annotation style. Instead, the annotations are left for users
        #     to discover and experiment with useful annotation styles.
        #
        # therefore this is commented out:
        #
        # if f.return_converter.py_default:
        #     lines.append(' -> ')
        #     lines.append(f.return_converter.py_default)

        if not f.docstring_only:
            lines.append("\n" + libclinic.SIG_END_MARKER + "\n")

        signature_line = "".join(lines)

        # now fix up the places where the brackets look wrong
        return signature_line.replace(', ]', ',] ')

    @staticmethod
    def format_docstring_parameters(params: list[Parameter]) -> str:
        """Create substitution text for {parameters}"""
        return "".join(p.render_docstring() + "\n" for p in params if p.docstring)

    def format_docstring(self) -> str:
        assert self.function is not None
        f = self.function
        # For the following special cases, it does not make sense to render a docstring.
        if f.kind in {METHOD_INIT, METHOD_NEW, GETTER, SETTER} and not f.docstring:
            return f.docstring

        # Enforce the summary line!
        # The first line of a docstring should be a summary of the function.
        # It should fit on one line (80 columns? 79 maybe?) and be a paragraph
        # by itself.
        #
        # Argument Clinic enforces the following rule:
        #  * either the docstring is empty,
        #  * or it must have a summary line.
        #
        # Guido said Clinic should enforce this:
        # http://mail.python.org/pipermail/python-dev/2013-June/127110.html

        lines = f.docstring.split('\n')
        if len(lines) >= 2:
            if lines[1]:
                fail(f"Docstring for {f.full_name!r} does not have a summary line!\n"
                     "Every non-blank function docstring must start with "
                     "a single line summary followed by an empty line.")
        elif len(lines) == 1:
            # the docstring is only one line right now--the summary line.
            # add an empty line after the summary line so we have space
            # between it and the {parameters} we're about to add.
            lines.append('')

        parameters_marker_count = len(f.docstring.split('{parameters}')) - 1
        if parameters_marker_count > 1:
            fail('You may not specify {parameters} more than once in a docstring!')

        # insert signature at front and params after the summary line
        if not parameters_marker_count:
            lines.insert(2, '{parameters}')
        lines.insert(0, '{signature}')

        # finalize docstring
        params = f.render_parameters
        parameters = self.format_docstring_parameters(params)
        signature = self.format_docstring_signature(f, params)
        docstring = "\n".join(lines)
        return libclinic.linear_format(docstring,
                                       signature=signature,
                                       parameters=parameters).rstrip()

    def check_remaining_star(self, lineno: int | None = None) -> None:
        assert isinstance(self.function, Function)

        if self.keyword_only:
            symbol = '*'
        elif self.deprecated_positional:
            symbol = '* [from ...]'
        else:
            return

        for p in reversed(self.function.parameters.values()):
            if self.keyword_only:
                if p.kind == inspect.Parameter.KEYWORD_ONLY:
                    return
            elif self.deprecated_positional:
                if p.deprecated_positional == self.deprecated_positional:
                    return
            break

        fail(f"Function {self.function.name!r} specifies {symbol!r} "
             f"without following parameters.", line_number=lineno)

    def check_previous_star(self, lineno: int | None = None) -> None:
        assert isinstance(self.function, Function)

        for p in self.function.parameters.values():
            if p.kind == inspect.Parameter.VAR_POSITIONAL:
                fail(f"Function {self.function.name!r} uses '*' more than once.")


    def do_post_block_processing_cleanup(self, lineno: int) -> None:
        """
        Called when processing the block is done.
        """
        if not self.function:
            return

        self.check_remaining_star(lineno)
        try:
            self.function.docstring = self.format_docstring()
        except ClinicError as exc:
            exc.lineno = lineno
            exc.filename = self.clinic.filename
            raise




# maps strings to callables.
# the callable should return an object
# that implements the clinic parser
# interface (__init__ and parse).
#
# example parsers:
#   "clinic", handles the Clinic DSL
#   "python", handles running Python code
#
parsers: dict[str, Callable[[Clinic], Parser]] = {
    'clinic': DSLParser,
    'python': PythonParser,
}


def create_cli() -> argparse.ArgumentParser:
    cmdline = argparse.ArgumentParser(
        prog="clinic.py",
        description="""Preprocessor for CPython C files.

The purpose of the Argument Clinic is automating all the boilerplate involved
with writing argument parsing code for builtins and providing introspection
signatures ("docstrings") for CPython builtins.

For more information see https://devguide.python.org/development-tools/clinic/""")
    cmdline.add_argument("-f", "--force", action='store_true',
                         help="force output regeneration")
    cmdline.add_argument("-o", "--output", type=str,
                         help="redirect file output to OUTPUT")
    cmdline.add_argument("-v", "--verbose", action='store_true',
                         help="enable verbose mode")
    cmdline.add_argument("--converters", action='store_true',
                         help=("print a list of all supported converters "
                               "and return converters"))
    cmdline.add_argument("--make", action='store_true',
                         help="walk --srcdir to run over all relevant files")
    cmdline.add_argument("--srcdir", type=str, default=os.curdir,
                         help="the directory tree to walk in --make mode")
    cmdline.add_argument("--exclude", type=str, action="append",
                         help=("a file to exclude in --make mode; "
                               "can be given multiple times"))
    cmdline.add_argument("--limited", dest="limited_capi", action='store_true',
                         help="use the Limited C API")
    cmdline.add_argument("filename", metavar="FILE", type=str, nargs="*",
                         help="the list of files to process")
    return cmdline


def run_clinic(parser: argparse.ArgumentParser, ns: argparse.Namespace) -> None:
    if ns.converters:
        if ns.filename:
            parser.error(
                "can't specify --converters and a filename at the same time"
            )
        converters: list[tuple[str, str]] = []
        return_converters: list[tuple[str, str]] = []
        ignored = set("""
            add_c_converter
            add_c_return_converter
            add_default_legacy_c_converter
            add_legacy_c_converter
            """.strip().split())
        module = globals()
        for name in module:
            for suffix, ids in (
                ("_return_converter", return_converters),
                ("_converter", converters),
            ):
                if name in ignored:
                    continue
                if name.endswith(suffix):
                    ids.append((name, name.removesuffix(suffix)))
                    break
        print()

        print("Legacy converters:")
        legacy = sorted(legacy_converters)
        print('    ' + ' '.join(c for c in legacy if c[0].isupper()))
        print('    ' + ' '.join(c for c in legacy if c[0].islower()))
        print()

        for title, attribute, ids in (
            ("Converters", 'converter_init', converters),
            ("Return converters", 'return_converter_init', return_converters),
        ):
            print(title + ":")
            longest = -1
            for name, short_name in ids:
                longest = max(longest, len(short_name))
            for name, short_name in sorted(ids, key=lambda x: x[1].lower()):
                cls = module[name]
                callable = getattr(cls, attribute, None)
                if not callable:
                    continue
                signature = inspect.signature(callable)
                parameters = []
                for parameter_name, parameter in signature.parameters.items():
                    if parameter.kind == inspect.Parameter.KEYWORD_ONLY:
                        if parameter.default != inspect.Parameter.empty:
                            s = f'{parameter_name}={parameter.default!r}'
                        else:
                            s = parameter_name
                        parameters.append(s)
                print('    {}({})'.format(short_name, ', '.join(parameters)))
            print()
        print("All converters also accept (c_default=None, py_default=None, annotation=None).")
        print("All return converters also accept (py_default=None).")
        return

    if ns.make:
        if ns.output or ns.filename:
            parser.error("can't use -o or filenames with --make")
        if not ns.srcdir:
            parser.error("--srcdir must not be empty with --make")
        if ns.exclude:
            excludes = [os.path.join(ns.srcdir, f) for f in ns.exclude]
            excludes = [os.path.normpath(f) for f in excludes]
        else:
            excludes = []
        for root, dirs, files in os.walk(ns.srcdir):
            for rcs_dir in ('.svn', '.git', '.hg', 'build', 'externals'):
                if rcs_dir in dirs:
                    dirs.remove(rcs_dir)
            for filename in files:
                # handle .c, .cpp and .h files
                if not filename.endswith(('.c', '.cpp', '.h')):
                    continue
                path = os.path.join(root, filename)
                path = os.path.normpath(path)
                if path in excludes:
                    continue
                if ns.verbose:
                    print(path)
                parse_file(path,
                           verify=not ns.force, limited_capi=ns.limited_capi)
        return

    if not ns.filename:
        parser.error("no input files")

    if ns.output and len(ns.filename) > 1:
        parser.error("can't use -o with multiple filenames")

    for filename in ns.filename:
        if ns.verbose:
            print(filename)
        parse_file(filename, output=ns.output,
                   verify=not ns.force, limited_capi=ns.limited_capi)


def main(argv: list[str] | None = None) -> NoReturn:
    parser = create_cli()
    args = parser.parse_args(argv)
    try:
        run_clinic(parser, args)
    except ClinicError as exc:
        sys.stderr.write(exc.report())
        sys.exit(1)
    else:
        sys.exit(0)


if __name__ == "__main__":
    main()
