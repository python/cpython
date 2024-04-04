from __future__ import annotations
import itertools
import sys
import textwrap
from typing import TYPE_CHECKING, Literal, Final
from operator import attrgetter
from collections.abc import Iterable

import libclinic
from libclinic import (
    unspecified, fail, warn, Sentinels, VersionTuple)
from libclinic.function import (
    GETTER, SETTER, METHOD_INIT, METHOD_NEW)
from libclinic.crenderdata import CRenderData, TemplateDict
from libclinic.language import Language
from libclinic.function import (
    Module, Class, Function, Parameter,
    permute_optional_groups)
from libclinic.converters import (
    defining_class_converter, object_converter, self_converter)
if TYPE_CHECKING:
    from libclinic.app import Clinic


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
                    converters[0].use_converter()
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
                for parameter in parameters:
                    parameter.converter.use_converter()

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
                for parameter in parameters:
                    parameter.converter.use_converter()

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
