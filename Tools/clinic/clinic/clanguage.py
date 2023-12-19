from __future__ import annotations

import functools
import itertools
import sys
import textwrap
from operator import attrgetter
from collections.abc import (
    Iterable,
    Iterator,
    Sequence,
)
from typing import (
    Final,
    Literal,
    TYPE_CHECKING,
)

from . import cpp
from .utils import (
    fail, warn, c_repr,
    Sentinels, TemplateDict, VersionTuple, unspecified,
    _text_accumulator, text_accumulator)
from .function import (
    Function, Class, Module,
    GETTER, SETTER, METHOD_NEW,
)
from .crender_data import CRenderData
from .parameter import Parameter, ParamTuple
from .converters import (
    defining_class_converter,
    self_converter,
    object_converter,
)
from .language import Language
if TYPE_CHECKING:
    from .clinic import Clinic


NO_VARARG = "PY_SSIZE_T_MAX"

sig_end_marker = '--'


def quoted_for_c_string(s: str) -> str:
    for old, new in (
        ('\\', '\\\\'), # must be first!
        ('"', '\\"'),
        ("'", "\\'"),
        ):
        s = s.replace(old, new)
    return s


def wrapped_c_string_literal(
        text: str,
        *,
        width: int = 72,
        suffix: str = '',
        initial_indent: int = 0,
        subsequent_indent: int = 4
) -> str:
    wrapped = textwrap.wrap(text, width=width, replace_whitespace=False,
                            drop_whitespace=False, break_on_hyphens=False)
    separator = '"' + suffix + '\n' + subsequent_indent * ' ' + '"'
    return initial_indent * ' ' + '"' + separator.join(wrapped) + '"'


def rstrip_lines(s: str) -> str:
    text, add, output = _text_accumulator()
    for line in s.split('\n'):
        add(line.rstrip())
        add('\n')
    text.pop()
    return output()


def format_escape(s: str) -> str:
    # double up curly-braces, this string will be used
    # as part of a format_map() template later
    s = s.replace('{', '{{')
    s = s.replace('}', '}}')
    return s


def linear_format(s: str, **kwargs: str) -> str:
    """
    Perform str.format-like substitution, except:
      * The strings substituted must be on lines by
        themselves.  (This line is the "source line".)
      * If the substitution text is empty, the source line
        is removed in the output.
      * If the field is not recognized, the original line
        is passed unmodified through to the output.
      * If the substitution text is not empty:
          * Each line of the substituted text is indented
            by the indent of the source line.
          * A newline will be added to the end.
    """

    add, output = text_accumulator()
    for line in s.split('\n'):
        indent, curly, trailing = line.partition('{')
        if not curly:
            add(line)
            add('\n')
            continue

        name, curly, trailing = trailing.partition('}')
        if not curly or name not in kwargs:
            add(line)
            add('\n')
            continue

        if trailing:
            fail(f"Text found after {{{name}}} block marker! "
                 "It must be on a line by itself.")
        if indent.strip():
            fail(f"Non-whitespace characters found before {{{name}}} block marker! "
                 "It must be on a line by itself.")

        value = kwargs[name]
        if not value:
            continue

        value = textwrap.indent(rstrip_lines(value), indent)
        add(value)
        add('\n')

    return output()[:-1]


def indent_all_lines(s: str, prefix: str) -> str:
    """
    Returns 's', with 'prefix' prepended to all lines.

    If the last line is empty, prefix is not prepended
    to it.  (If s is blank, returns s unchanged.)

    (textwrap.indent only adds to non-blank lines.)
    """
    split = s.split('\n')
    last = split.pop()
    final = []
    for line in split:
        final.append(prefix)
        final.append(line)
        final.append('\n')
    if last:
        final.append(prefix)
        final.append(last)
    return ''.join(final)


def suffix_all_lines(s: str, suffix: str) -> str:
    """
    Returns 's', with 'suffix' appended to all lines.

    If the last line is empty, suffix is not appended
    to it.  (If s is blank, returns s unchanged.)
    """
    split = s.split('\n')
    last = split.pop()
    final = []
    for line in split:
        final.append(line)
        final.append(suffix)
        final.append('\n')
    if last:
        final.append(last)
        final.append(suffix)
    return ''.join(final)


def pprint_words(items: list[str]) -> str:
    if len(items) <= 2:
        return " and ".join(items)
    else:
        return ", ".join(items[:-1]) + " and " + items[-1]


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


def strip_leading_and_trailing_blank_lines(s: str) -> str:
    lines = s.rstrip().split('\n')
    while lines:
        line = lines[0]
        if line.strip():
            break
        del lines[0]
    return '\n'.join(lines)


@functools.lru_cache()
def normalize_snippet(
        s: str,
        *,
        indent: int = 0
) -> str:
    """
    Reformats s:
        * removes leading and trailing blank lines
        * ensures that it does not end with a newline
        * dedents so the first nonwhite character on any line is at column "indent"
    """
    s = strip_leading_and_trailing_blank_lines(s)
    s = textwrap.dedent(s)
    if indent:
        s = textwrap.indent(s, ' ' * indent)
    return s


def wrap_declarations(
        text: str,
        length: int = 78
) -> str:
    """
    A simple-minded text wrapper for C function declarations.

    It views a declaration line as looking like this:
        xxxxxxxx(xxxxxxxxx,xxxxxxxxx)
    If called with length=30, it would wrap that line into
        xxxxxxxx(xxxxxxxxx,
                 xxxxxxxxx)
    (If the declaration has zero or one parameters, this
    function won't wrap it.)

    If this doesn't work properly, it's probably better to
    start from scratch with a more sophisticated algorithm,
    rather than try and improve/debug this dumb little function.
    """
    lines = []
    for line in text.split('\n'):
        prefix, _, after_l_paren = line.partition('(')
        if not after_l_paren:
            lines.append(line)
            continue
        in_paren, _, after_r_paren = after_l_paren.partition(')')
        if not _:
            lines.append(line)
            continue
        if ',' not in in_paren:
            lines.append(line)
            continue
        parameters = [x.strip() + ", " for x in in_paren.split(',')]
        prefix += "("
        if len(prefix) < length:
            spaces = " " * len(prefix)
        else:
            spaces = " " * 4

        while parameters:
            line = prefix
            first = True
            while parameters:
                if (not first and
                    (len(line) + len(parameters[0]) > length)):
                    break
                line += parameters.pop(0)
                first = False
            if not parameters:
                line = line.rstrip(", ") + ")" + after_r_paren
            lines.append(line.rstrip())
            prefix = spaces
    return "\n".join(lines)


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
    return normalize_snippet(declarations)


class CLanguage(Language):

    body_prefix   = "#"
    language      = 'C'
    start_line    = "/*[{dsl_name} input]"
    body_prefix   = ""
    stop_line     = "[{dsl_name} start generated code]*/"
    checksum_line = "/*[{dsl_name} end generated code: {arguments}]*/"

    PARSER_PROTOTYPE_KEYWORD: Final[str] = normalize_snippet("""
        static PyObject *
        {c_basename}({self_type}{self_name}, PyObject *args, PyObject *kwargs)
    """)
    PARSER_PROTOTYPE_KEYWORD___INIT__: Final[str] = normalize_snippet("""
        static int
        {c_basename}({self_type}{self_name}, PyObject *args, PyObject *kwargs)
    """)
    PARSER_PROTOTYPE_VARARGS: Final[str] = normalize_snippet("""
        static PyObject *
        {c_basename}({self_type}{self_name}, PyObject *args)
    """)
    PARSER_PROTOTYPE_FASTCALL: Final[str] = normalize_snippet("""
        static PyObject *
        {c_basename}({self_type}{self_name}, PyObject *const *args, Py_ssize_t nargs)
    """)
    PARSER_PROTOTYPE_FASTCALL_KEYWORDS: Final[str] = normalize_snippet("""
        static PyObject *
        {c_basename}({self_type}{self_name}, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
    """)
    PARSER_PROTOTYPE_DEF_CLASS: Final[str] = normalize_snippet("""
        static PyObject *
        {c_basename}({self_type}{self_name}, PyTypeObject *{defining_class_name}, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
    """)
    PARSER_PROTOTYPE_NOARGS: Final[str] = normalize_snippet("""
        static PyObject *
        {c_basename}({self_type}{self_name}, PyObject *Py_UNUSED(ignored))
    """)
    PARSER_PROTOTYPE_GETTER: Final[str] = normalize_snippet("""
        static PyObject *
        {c_basename}({self_type}{self_name}, void *Py_UNUSED(context))
    """)
    PARSER_PROTOTYPE_SETTER: Final[str] = normalize_snippet("""
        static int
        {c_basename}({self_type}{self_name}, PyObject *value, void *Py_UNUSED(context))
    """)
    METH_O_PROTOTYPE: Final[str] = normalize_snippet("""
        static PyObject *
        {c_basename}({impl_parameters})
    """)
    DOCSTRING_PROTOTYPE_VAR: Final[str] = normalize_snippet("""
        PyDoc_VAR({c_basename}__doc__);
    """)
    DOCSTRING_PROTOTYPE_STRVAR: Final[str] = normalize_snippet("""
        PyDoc_STRVAR({c_basename}__doc__,
        {docstring});
    """)
    IMPL_DEFINITION_PROTOTYPE: Final[str] = normalize_snippet("""
        static {impl_return_type}
        {c_basename}_impl({impl_parameters})
    """)
    METHODDEF_PROTOTYPE_DEFINE: Final[str] = normalize_snippet(r"""
        #define {methoddef_name}    \
            {{"{name}", {methoddef_cast}{c_basename}{methoddef_cast_end}, {methoddef_flags}, {c_basename}__doc__}},
    """)
    GETTERDEF_PROTOTYPE_DEFINE: Final[str] = normalize_snippet(r"""
        #if defined({getset_name}_GETSETDEF)
        #  undef {getset_name}_GETSETDEF
        #  define {getset_name}_GETSETDEF {{"{name}", (getter){getset_basename}_get, (setter){getset_basename}_set, NULL}},
        #else
        #  define {getset_name}_GETSETDEF {{"{name}", (getter){getset_basename}_get, NULL, NULL}},
        #endif
    """)
    SETTERDEF_PROTOTYPE_DEFINE: Final[str] = normalize_snippet(r"""
        #if defined({getset_name}_GETSETDEF)
        #  undef {getset_name}_GETSETDEF
        #  define {getset_name}_GETSETDEF {{"{name}", (getter){getset_basename}_get, (setter){getset_basename}_set, NULL}},
        #else
        #  define {getset_name}_GETSETDEF {{"{name}", NULL, (setter){getset_basename}_set, NULL}},
        #endif
    """)
    METHODDEF_PROTOTYPE_IFNDEF: Final[str] = normalize_snippet("""
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
        self.cpp = cpp.Monitor(filename)
        self.cpp.fail = fail  # type: ignore[method-assign]

    def parse_line(self, line: str) -> None:
        self.cpp.writeline(line)

    def render(
            self,
            clinic: Clinic | None,
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
            message=c_repr(message),
        )
        return normalize_snippet(code)

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
            pstr = pprint_words(names)
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
            message=wrapped_c_string_literal(message, width=64,
                                             subsequent_indent=20),
        )
        return normalize_snippet(code, indent=4)

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
        pstr = pprint_words(names)
        pl = 's' if len(params) != 1 else ''
        message = (
            f"Passing keyword argument{pl} {pstr} to "
            f"{func.fulldisplayname}() is deprecated."
        )

        for (major, minor), group in itertools.groupby(
            params.values(), key=attrgetter("deprecated_keyword")
        ):
            names = [repr(p.name) for p in group]
            pstr = pprint_words(names)
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
            message=wrapped_c_string_literal(message, width=64,
                                             subsequent_indent=20),
        )
        return normalize_snippet(code, indent=4)

    def docstring_for_c_string(
            self,
            f: Function
    ) -> str:
        text, add, output = _text_accumulator()
        # turn docstring into a properly quoted C string
        for line in f.docstring.split('\n'):
            add('"')
            add(quoted_for_c_string(line))
            add('\\n"\n')

        if text[-2] == sig_end_marker:
            # If we only have a signature, add the blank line that the
            # __text_signature__ getter expects to be there.
            add('"\\n"')
        else:
            text.pop()
            add('"')
        return ''.join(text)

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

        # Copy includes from parameters to Clinic
        for converter in converters:
            include = converter.include
            if include:
                clinic.add_include(include.filename, include.reason,
                                   condition=include.condition)
        if f.critical_section:
            clinic.add_include('pycore_critical_section.h', 'Py_BEGIN_CRITICAL_SECTION()')
        has_option_groups = parameters and (parameters[0].group or parameters[-1].group)
        simple_return = (f.return_converter.type == 'PyObject *'
                         and not f.critical_section)
        new_or_init = f.kind.new_or_init

        vararg: int | str = NO_VARARG
        pos_only = min_pos = max_pos = min_kw_only = pseudo_args = 0
        for i, p in enumerate(parameters, 1):
            if p.is_keyword_only():
                assert not p.is_positional_only()
                if not p.is_optional():
                    min_kw_only = i - max_pos
            elif p.is_vararg():
                if vararg != NO_VARARG:
                    fail("Too many var args")
                pseudo_args += 1
                vararg = i - 1
            else:
                if vararg == NO_VARARG:
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
            docstring_prototype = docstring_definition = ''
        elif f.kind is SETTER:
            return_value_declaration = "int {return_value};"
            methoddef_define = self.SETTERDEF_PROTOTYPE_DEFINE
            docstring_prototype = docstring_prototype = docstring_definition = ''
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
            add, output = text_accumulator()
            add(prototype)
            parser_body_fields = fields

            preamble = normalize_snippet("""
                {{
                    {return_value_declaration}
                    {parser_declarations}
                    {declarations}
                    {initializers}
            """) + "\n"
            finale = normalize_snippet("""
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
                add('\n')
                add(field)
            return linear_format(output(), parser_declarations=declarations)

        fastcall = not new_or_init
        limited_capi = clinic.limited_capi
        if limited_capi and (pseudo_args or
                (any(p.is_optional() for p in parameters) and
                 any(p.is_keyword_only() and not p.is_optional() for p in parameters)) or
                any(c.broken_limited_capi for c in converters)):
            warn(f"Function {f.full_name} cannot use limited C API")
            limited_capi = False

        parsearg: str | None
        if f.kind in {GETTER, SETTER} and parameters:
            fail(f"@{f.kind.name.lower()} method cannot define parameters")

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
                parser_code = [normalize_snippet("""
                    if (nargs) {{
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
                parser_prototype = normalize_snippet("""
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
                                                normalize_snippet(parsearg, indent=4))

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
            max_args = NO_VARARG if (vararg != NO_VARARG) else max_pos
            if limited_capi:
                parser_code = []
                if nargs != 'nargs':
                    parser_code.append(normalize_snippet(f'Py_ssize_t nargs = {nargs};', indent=4))
                    nargs = 'nargs'
                if min_pos == max_args:
                    pl = '' if min_pos == 1 else 's'
                    parser_code.append(normalize_snippet(f"""
                        if ({nargs} != {min_pos}) {{{{
                            PyErr_Format(PyExc_TypeError, "{{name}} expected {min_pos} argument{pl}, got %zd", {nargs});
                            goto exit;
                        }}}}
                        """,
                    indent=4))
                else:
                    if min_pos:
                        pl = '' if min_pos == 1 else 's'
                        parser_code.append(normalize_snippet(f"""
                            if ({nargs} < {min_pos}) {{{{
                                PyErr_Format(PyExc_TypeError, "{{name}} expected at least {min_pos} argument{pl}, got %zd", {nargs});
                                goto exit;
                            }}}}
                            """,
                            indent=4))
                    if max_args != NO_VARARG:
                        pl = '' if max_args == 1 else 's'
                        parser_code.append(normalize_snippet(f"""
                            if ({nargs} > {max_args}) {{{{
                                PyErr_Format(PyExc_TypeError, "{{name}} expected at most {max_args} argument{pl}, got %zd", {nargs});
                                goto exit;
                            }}}}
                            """,
                        indent=4))
            else:
                clinic.add_include('pycore_modsupport.h',
                                   '_PyArg_CheckPositional()')
                parser_code = [normalize_snippet(f"""
                    if (!_PyArg_CheckPositional("{{name}}", {nargs}, {min_pos}, {max_args})) {{{{
                        goto exit;
                    }}}}
                    """, indent=4)]

            has_optional = False
            for i, p in enumerate(parameters):
                if p.is_vararg():
                    if fastcall:
                        parser_code.append(normalize_snippet("""
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
                        parser_code.append(normalize_snippet("""
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
                    parser_code.append(normalize_snippet("""
                        if (%s < %d) {{
                            goto skip_optional;
                        }}
                        """, indent=4) % (nargs, i + 1))
                parser_code.append(normalize_snippet(parsearg, indent=4))

            if parser_code is not None:
                if has_optional:
                    parser_code.append("skip_optional:")
            else:
                if limited_capi:
                    fastcall = False
                if fastcall:
                    clinic.add_include('pycore_modsupport.h',
                                       '_PyArg_ParseStack()')
                    parser_code = [normalize_snippet("""
                        if (!_PyArg_ParseStack(args, nargs, "{format_units}:{name}",
                            {parse_arguments})) {{
                            goto exit;
                        }}
                        """, indent=4)]
                else:
                    flags = "METH_VARARGS"
                    parser_prototype = self.PARSER_PROTOTYPE_VARARGS
                    parser_code = [normalize_snippet("""
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

            has_optional_kw = (max(pos_only, min_pos) + min_kw_only < len(converters) - int(vararg != NO_VARARG))

            if limited_capi:
                parser_code = None
                fastcall = False
            else:
                if vararg == NO_VARARG:
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
                    parser_code = [normalize_snippet("""
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
                    parser_code = [normalize_snippet("""
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
                        parser_code.append(normalize_snippet(parsearg, indent=4))
                    elif i < pos_only:
                        add_label = 'skip_optional_posonly'
                        parser_code.append(normalize_snippet("""
                            if (nargs < %d) {{
                                goto %s;
                            }}
                            """ % (i + 1, add_label), indent=4))
                        if has_optional_kw:
                            parser_code.append(normalize_snippet("""
                                noptargs--;
                                """, indent=4))
                        parser_code.append(normalize_snippet(parsearg, indent=4))
                    else:
                        if i < max_pos:
                            label = 'skip_optional_pos'
                            first_opt = max(min_pos, pos_only)
                        else:
                            label = 'skip_optional_kwonly'
                            first_opt = max_pos + min_kw_only
                            if vararg != NO_VARARG:
                                first_opt += 1
                        if i == first_opt:
                            add_label = label
                            parser_code.append(normalize_snippet("""
                                if (!noptargs) {{
                                    goto %s;
                                }}
                                """ % add_label, indent=4))
                        if i + 1 == len(parameters):
                            parser_code.append(normalize_snippet(parsearg, indent=4))
                        else:
                            add_label = label
                            parser_code.append(normalize_snippet("""
                                if (%s) {{
                                """ % (argname_fmt % i), indent=4))
                            parser_code.append(normalize_snippet(parsearg, indent=8))
                            parser_code.append(normalize_snippet("""
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
                    parser_code = [normalize_snippet("""
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
                    parser_code = [normalize_snippet("""
                        if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser{parse_arguments_comma}
                            {parse_arguments})) {{
                            goto exit;
                        }}
                        """, indent=4)]
                else:
                    clinic.add_include('pycore_modsupport.h',
                                       '_PyArg_ParseTupleAndKeywordsFast()')
                    parser_code = [normalize_snippet("""
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
                fields.insert(0, normalize_snippet("""
                    if ({self_type_check}!_PyArg_NoKeywords("{name}", kwargs)) {{
                        goto exit;
                    }}
                    """, indent=4))
                if not parses_positional:
                    clinic.add_include('pycore_modsupport.h',
                                       '_PyArg_NoPositional()')
                    fields.insert(0, normalize_snippet("""
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

        add, output = text_accumulator()
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
        add(f"switch ({nargs}) {{\n")
        for subset in permute_optional_groups(left, required, right):
            count = len(subset)
            count_min = min(count_min, count)
            count_max = max(count_max, count)

            if count == 0:
                add("""    case 0:
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
            s = linear_format(s, group_booleans=lines)
            s = s.format_map(d)
            add(s)

        add("    default:\n")
        s = '        PyErr_SetString(PyExc_TypeError, "{} requires {} to {} arguments");\n'
        add(s.format(f.full_name, count_min, count_max))
        add('        goto exit;\n')
        add("}")
        template_dict['option_group_parsing'] = format_escape(output())

    def render_function(
            self,
            clinic: Clinic | None,
            f: Function | None
    ) -> str:
        if f is None or clinic is None:
            return ""

        add, output = text_accumulator()
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

        template_dict['docstring'] = self.docstring_for_c_string(f)

        template_dict['self_name'] = template_dict['self_type'] = template_dict['self_type_check'] = ''
        template_dict['target_critical_section'] = ', '.join(f.target_critical_section)
        for converter in converters:
            converter.set_template_dict(template_dict)

        f.return_converter.render(f, data)
        if f.kind is SETTER:
            # All setters return an int.
            template_dict['impl_return_type'] = 'int'
        else:
            template_dict['impl_return_type'] = f.return_converter.type

        template_dict['declarations'] = format_escape("\n".join(data.declarations))
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
        template_dict['return_conversion'] = format_escape("".join(data.return_conversion).rstrip())
        template_dict['post_parsing'] = format_escape("".join(data.post_parsing).rstrip())
        template_dict['cleanup'] = format_escape("".join(data.cleanup))
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
                template = linear_format(template,
                        option_group_parsing=template_dict['option_group_parsing'])
            template = linear_format(template,
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
            need_exit_label = "goto exit;" in template
            template = linear_format(template,
                exit_label="exit:" if need_exit_label else ''
                )

            s = template.format_map(template_dict)

            # mild hack:
            # reflow long impl declarations
            if name in {"impl_prototype", "impl_definition"}:
                s = wrap_declarations(s)

            if clinic.line_prefix:
                s = indent_all_lines(s, clinic.line_prefix)
            if clinic.line_suffix:
                s = suffix_all_lines(s, clinic.line_suffix)

            destination.append(s)

        return clinic.get_destination('block').dump()
