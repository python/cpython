from __future__ import annotations
from typing import TYPE_CHECKING, Final, Literal

import libclinic
from libclinic import fail, warn, unspecified, Sentinels
from libclinic.function import (
    Function, Parameter, ParamTuple,
    count_required, group_to_variable_name, permute_optional_groups,
    GETTER, SETTER, METHOD_NEW,
    ACCESSORS, SETTERS)
from libclinic.converter import CConverter
from libclinic.converters import (
    defining_class_converter, object_converter, self_converter)
if TYPE_CHECKING:
    from libclinic.clanguage import CLanguage
    from libclinic.codegen import CodeGen


def declare_parser(
    f: Function,
    *,
    hasformat: bool = False,
    codegen: CodeGen,
) -> str:
    """
    Generates the code template for a static local PyArg_Parser variable,
    with an initializer.  For core code (incl. builtin modules) the
    kwtuple field is also statically initialized.  Otherwise
    it is initialized at runtime.
    """
    limited_capi = codegen.limited_capi
    if hasformat:
        fname = ''
        format_ = '.format = "{format_units}:{name}",'
    else:
        fname = '.fname = "{name}",'
        format_ = ''

    num_keywords = len([
        p for p in f.parameters.values()
        if p.is_positional_or_keyword() or p.is_keyword_only()
    ])

    condition = '#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)'
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

        codegen.add_include('pycore_runtime.h', '_Py_SINGLETON()',
                            condition=condition)
    else:
        # XXX Why do we not statically allocate the tuple
        # for non-builtin modules?
        declarations = """
            #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

            #define NUM_KEYWORDS %d
            static struct {{
                PyGC_Head _this_is_not_used;
                PyObject_VAR_HEAD
                Py_hash_t ob_hash;
                PyObject *ob_item[NUM_KEYWORDS];
            }} _kwtuple = {{
                .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
                .ob_hash = -1,
                .ob_item = {{ {keywords_py} }},
            }};
            #undef NUM_KEYWORDS
            #define KWTUPLE (&_kwtuple.ob_base.ob_base)

            #else  // !Py_BUILD_CORE
            #  define KWTUPLE NULL
            #endif  // !Py_BUILD_CORE
        """ % num_keywords

        codegen.add_include('pycore_gc.h', 'PyGC_Head',
                            condition=condition)
        codegen.add_include('pycore_runtime.h', '_Py_ID()',
                            condition=condition)

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
    {c_basename}({self_type}{self_name}, {parser_parameters})
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
    #if defined({getset_basename}_DOCSTR)
    #   undef {getset_basename}_DOCSTR
    #endif
    #define {getset_basename}_DOCSTR {getset_basename}__doc__
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
    #if !defined({getset_basename}_DOCSTR)
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
    #if !defined({getset_basename}_DOCSTR)
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
GETSETDEF_PROTOTYPE_IFNDEF: Final[str] = libclinic.normalize_snippet("""
    #ifndef {getset_name}_GETSETDEF
        #define {getset_name}_GETSETDEF
    #endif /* !defined({getset_name}_GETSETDEF) */
""")
# The setter is called with NULL to delete the attribute.  Unless @deleter is
# applied to it, deletion is rejected before the implementation is called.
SETTER_PREAMBLE: Final[str] = libclinic.normalize_snippet("""
    if (value == NULL) {{
        PyErr_Format(PyExc_AttributeError,
                     "attribute '{name}' of '%.100s' objects cannot be deleted",
                     Py_TYPE({self_name})->tp_name);
        return -1;
    }}
""", indent=4)


class ParseArgsCodeGen:
    func: Function
    codegen: CodeGen
    limited_capi: bool = False

    # Function parameters
    parameters: list[Parameter]
    self_parameter_converter: self_converter
    converters: list[CConverter]

    # Is 'defining_class' used for the first parameter?
    requires_defining_class: bool

    # Use METH_FASTCALL calling convention?
    fastcall: bool

    # Declaration of the return variable (ex: "int return_value;")
    return_value_declaration: str

    # Calling convention (ex: "METH_NOARGS")
    flags: str

    # Variables declarations
    declarations: str

    pos_only: int = 0
    min_pos: int = 0
    max_pos: int = 0
    min_kw_only: int = 0
    varpos: Parameter | None = None
    var_keyword: Parameter | None = None

    docstring_prototype: str
    docstring_definition: str
    impl_prototype: str | None
    impl_definition: str
    methoddef_define: str
    parser_prototype: str
    parser_definition: str
    cpp_if: str
    cpp_endif: str
    methoddef_ifndef: str

    parser_body_fields: tuple[str, ...]

    def __init__(self, func: Function, codegen: CodeGen) -> None:
        self.func = func
        self.codegen = codegen

        self.parameters = list(self.func.parameters.values())
        self_parameter = self.parameters.pop(0)
        if not isinstance(self_parameter.converter, self_converter):
            raise ValueError("the first parameter must use self_converter")
        self.self_parameter_converter = self_parameter.converter

        self.requires_defining_class = False
        if self.parameters and isinstance(self.parameters[0].converter, defining_class_converter):
            self.requires_defining_class = True
            del self.parameters[0]

        for i, p in enumerate(self.parameters):
            if p.is_vararg():
                self.varpos = p
                del self.parameters[i]
                break

        for i, p in enumerate(self.parameters):
            if p.is_var_keyword():
                self.var_keyword = p
                del self.parameters[i]
                break

        self.converters = [p.converter for p in self.parameters]

        if self.func.critical_section:
            self.codegen.add_include('pycore_critical_section.h',
                                     'Py_BEGIN_CRITICAL_SECTION()')

        # Use fastcall if not disabled, except if in a __new__ or
        # __init__ method, or if there is a **kwargs parameter.
        if self.func.disable_fastcall:
            self.fastcall = False
        elif self.var_keyword is not None:
            self.fastcall = False
        else:
            self.fastcall = not self.is_new_or_init()

        self.pos_only = 0
        self.min_pos = 0
        self.max_pos = 0
        self.min_kw_only = 0
        for i, p in enumerate(self.parameters, 1):
            if p.is_keyword_only():
                assert not p.is_positional_only()
                if not p.is_optional():
                    self.min_kw_only = i - self.max_pos
            else:
                self.max_pos = i
                if p.is_positional_only():
                    self.pos_only = i
                if not p.is_optional():
                    self.min_pos = i

    def is_new_or_init(self) -> bool:
        return self.func.kind.new_or_init

    def has_option_groups(self) -> bool:
        return (bool(self.parameters
                and (self.parameters[0].group or self.parameters[-1].group)))

    def use_meth_o(self) -> bool:
        return (len(self.parameters) == 1
                and self.parameters[0].is_positional_only()
                and not self.has_option_groups()
                and not self.converters[0].is_optional()
                and not self.varpos
                and not self.requires_defining_class
                and not self.is_new_or_init())

    def use_simple_return(self) -> bool:
        return (self.func.return_converter.type == 'PyObject *'
                and not self.func.critical_section)

    def use_pyobject_self(self) -> bool:
        return self.self_parameter_converter.use_pyobject_self(self.func)

    def select_prototypes(self) -> None:
        self.docstring_prototype = ''
        self.docstring_definition = ''
        self.methoddef_define = METHODDEF_PROTOTYPE_DEFINE
        self.return_value_declaration = "PyObject *{parser_retval} = NULL;"

        if self.is_new_or_init() and not self.func.docstring:
            pass
        elif self.func.kind is GETTER:
            self.methoddef_define = GETTERDEF_PROTOTYPE_DEFINE
            if self.func.docstring:
                self.docstring_definition = GETSET_DOCSTRING_PROTOTYPE_STRVAR
        elif self.func.kind in SETTERS:
            if self.func.docstring:
                fail("docstrings are only supported for @getter, not @setter",
                     line_number=self.func.line_number)
            self.return_value_declaration = "int {parser_retval};"
            self.methoddef_define = SETTERDEF_PROTOTYPE_DEFINE
        else:
            self.docstring_prototype = DOCSTRING_PROTOTYPE_VAR
            self.docstring_definition = DOCSTRING_PROTOTYPE_STRVAR

    def init_limited_capi(self) -> None:
        self.limited_capi = self.codegen.limited_capi
        if self.limited_capi and (
                (self.varpos and self.pos_only < len(self.parameters)) or
                (any(p.is_optional() for p in self.parameters) and
                 any(p.is_keyword_only() and not p.is_optional() for p in self.parameters)) or
                any(c.broken_limited_capi for c in self.converters)):
            warn(f"Function {self.func.full_name} cannot use limited C API")
            self.limited_capi = False

    def parser_body(
        self,
        *fields: str,
        declarations: str = ''
    ) -> None:
        lines = [self.parser_prototype]
        self.parser_body_fields = fields

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
                return {parser_retval};
            }}
        """)
        for field in preamble, *fields, finale:
            lines.append(field)
        code = libclinic.linear_format("\n".join(lines),
                                       parser_declarations=self.declarations)
        self.parser_definition = code

    def parse_no_args(self) -> None:
        parser_code: list[str] | None
        simple_return = self.use_simple_return()
        if self.func.kind is GETTER:
            self.parser_prototype = PARSER_PROTOTYPE_GETTER
            parser_code = []
        elif self.func.kind in SETTERS:
            self.parser_prototype = PARSER_PROTOTYPE_SETTER
            if self.func.kind is SETTER:
                parser_code = [SETTER_PREAMBLE]
            else:
                parser_code = []
        elif not self.requires_defining_class:
            # no self.parameters, METH_NOARGS
            self.flags = "METH_NOARGS"
            self.parser_prototype = PARSER_PROTOTYPE_NOARGS
            parser_code = []
        else:
            assert self.fastcall

            self.flags = "METH_METHOD|METH_FASTCALL|METH_KEYWORDS"
            self.parser_prototype = PARSER_PROTOTYPE_DEF_CLASS
            return_error = ('return NULL;' if simple_return
                            else 'goto exit;')
            parser_code = [libclinic.normalize_snippet("""
                if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {{
                    PyErr_SetString(PyExc_TypeError, "{name}() takes no arguments");
                    %s
                }}
                """ % return_error, indent=4)]

        if simple_return:
            self.parser_definition = '\n'.join([
                self.parser_prototype,
                '{{',
                *parser_code,
                '    return {c_basename}_impl({impl_arguments});',
                '}}'])
        else:
            self.parser_body(*parser_code)

    def parse_one_arg(self) -> None:
        self.flags = "METH_O"

        if (isinstance(self.converters[0], object_converter) and
            self.converters[0].format_unit == 'O'):
            meth_o_prototype = METH_O_PROTOTYPE

            if self.use_simple_return() and self.use_pyobject_self():
                # maps perfectly to METH_O, doesn't need a return converter.
                # so we skip making a parse function
                # and call directly into the impl function.
                self.impl_prototype = ''
                self.impl_definition = meth_o_prototype
            else:
                # SLIGHT HACK
                # use impl_parameters for the parser here!
                self.parser_prototype = meth_o_prototype
                self.parser_body()

        else:
            argname = 'arg'
            if self.parameters[0].name == argname:
                argname += '_'
            self.parser_prototype = libclinic.normalize_snippet("""
                static PyObject *
                {c_basename}({self_type}{self_name}, PyObject *%s)
                """ % argname)

            displayname = self.parameters[0].get_displayname(0)
            parsearg: str | None
            parsearg = self.converters[0].parse_arg(argname, displayname,
                                                    limited_capi=self.limited_capi)
            if parsearg is None:
                self.converters[0].use_converter()
                parsearg = """
                    if (!PyArg_Parse(%s, "{format_units}:{name}", {parse_arguments})) {{
                        goto exit;
                    }}
                    """ % argname

            parser_code = libclinic.normalize_snippet(parsearg, indent=4)
            self.parser_body(parser_code)

    def option_group_subsets(self) -> dict[int, ParamTuple]:
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
        #
        # A nested group can only be omitted together with the group
        # containing it, but groups on the same level, like G and H in
        #
        # [ G1 G2 ] [ H1 ] I1 I2
        #
        # can be omitted independently of each other.

        # Groups are collected into chains of nested groups.  A group which
        # is not nested in the preceding one starts a new chain.
        group: list[Parameter] | None = None
        left: list[list[list[Parameter]]] = []
        right: list[list[list[Parameter]]] = []
        required: list[Parameter] = []
        last: int | Literal[Sentinels.unspecified] = unspecified
        last_depth = 0

        for p in self.parameters:
            group_id = p.group
            if group_id != last:
                last = group_id
                group = []
                if group_id == 0:
                    group = required
                else:
                    chains = left if group_id < 0 else right
                    nested = ((p.group_depth < last_depth) if group_id < 0
                              else (p.group_depth > last_depth))
                    if chains and nested:
                        chains[-1].append(group)
                    else:
                        chains.append([group])
                last_depth = p.group_depth
            assert group is not None
            group.append(p)

        # Map the number of arguments to the subset which accepts it.
        subsets: dict[int, ParamTuple] = {}
        for subset in permute_optional_groups(left, required, right):
            for count in range(count_required(subset), len(subset) + 1):
                if count in subsets:
                    fail(f"Function {self.func.full_name!r} has an ambiguous "
                         f"group configuration: a call with {count} "
                         f"argument(s) can be parsed in more than one way.",
                         line_number=self.func.line_number)
                subsets[count] = subset
        return subsets

    def parse_option_groups(self) -> None:
        # positional parameters with option groups
        # (which groups are passed is determined by the number of arguments)

        nargs, argname_fmt = self.select_positional_convention()
        # With the stack convention "nargs" is already a parameter.
        size_expr = None if self.fastcall else nargs

        subsets = self.option_group_subsets()
        parser_code = self.render_option_groups(subsets, argname_fmt, size_expr)
        if parser_code is None:
            # The code of some converter cannot be inlined, so the
            # arguments of every combination of groups are parsed at once.
            parser_code = self.render_option_groups_parse_tuple(subsets)
        self.parser_body(*parser_code)

    def render_option_groups(
        self,
        subsets: dict[int, ParamTuple],
        argname_fmt: str,
        size_expr: str | None,
    ) -> list[str] | None:
        """Return the code which parses the arguments of all groups.

        Every parameter is parsed once, in a block guarded by the number of
        arguments.  Return None if the code of some converter cannot be
        inlined.
        """
        # Split the parameters into the groups, in the order in which
        # they are passed.
        groups: list[tuple[int, list[Parameter]]] = []
        for p in self.parameters:
            if not groups or groups[-1][0] != p.group:
                groups.append((p.group, []))
            groups[-1][1].append(p)

        indices, shifting = self.render_option_group_indices(subsets, groups)

        parser_code = []
        if size_expr is not None:
            parser_code.append(libclinic.normalize_snippet(
                f"Py_ssize_t nargs = {size_expr};", indent=4))
        if shifting:
            parser_code.append(libclinic.normalize_snippet(
                "Py_ssize_t offset = 0;", indent=4))
        parser_code.append(self.render_option_group_check(subsets))

        has_optional = False
        for group_id, params in groups:
            body = []
            for p in params:
                index_expr = indices[p.name]
                if index_expr.isdigit():
                    displayname = p.get_displayname(int(index_expr) + 1)
                else:
                    # The position of the argument differs between the calls.
                    displayname = p.get_displayname(0)
                parsearg = p.converter.parse_arg(argname_fmt % index_expr,
                                                 displayname,
                                                 limited_capi=self.limited_capi)
                if parsearg is None:
                    return None
                if not group_id and (has_optional or p.is_optional()):
                    # Such parameter is omitted on its own, and nothing
                    # can follow it, so the rest is simply skipped.
                    has_optional = True
                    body.append(libclinic.normalize_snippet("""
                        if (nargs <= %s) {{
                            goto skip_optional;
                        }}
                        """, indent=4) % index_expr)
                body.append(libclinic.normalize_snippet(parsearg, indent=4))

            if not group_id:
                parser_code.extend(body)
                continue

            if group_id in shifting:
                # The arguments of this group shift the following ones.
                body.append(libclinic.normalize_snippet(
                    f"offset += {len(params)};", indent=4))
            body.append(libclinic.normalize_snippet(
                f"{group_to_variable_name(group_id)} = 1;", indent=4))
            condition = self.render_option_group_condition(subsets, group_id)
            parser_code.append(libclinic.normalize_snippet(
                f"if ({condition}) {{{{", indent=4))
            parser_code.extend(libclinic.normalize_snippet(line, indent=8)
                               for line in body)
            parser_code.append(libclinic.normalize_snippet("}}", indent=4))

        if has_optional:
            parser_code.append("skip_optional:")
        return parser_code

    @staticmethod
    def render_option_group_indices(
        subsets: dict[int, ParamTuple],
        groups: list[tuple[int, list[Parameter]]],
    ) -> tuple[dict[str, str], set[int]]:
        """Return the index of the argument of every parameter, as C code,
        and the groups which have to add their size to "offset".

        A group which is not passed shifts the following arguments.  Their
        index can still be fixed if it is counted from the end; otherwise it
        is counted at run time, in "offset".
        """
        forward: dict[str, set[int]] = {}
        backward: dict[str, set[int]] = {}
        for count, subset in subsets.items():
            for i, p in enumerate(subset[:count]):
                forward.setdefault(p.name, set()).add(i)
                backward.setdefault(p.name, set()).add(count - i)
        # A parameter with a default value can be omitted on its own, so
        # its index from the end is not fixed.
        for group_id, params in groups:
            if not group_id:
                for p in params:
                    if p.is_optional():
                        backward.pop(p.name)

        def counted_at_run_time(p: Parameter) -> bool:
            return (len(forward[p.name]) != 1
                    and len(backward.get(p.name, ())) != 1)

        # A group adds its size to "offset" if it shifts an argument
        # which is counted at run time.
        shifting: set[int] = set()
        seen: list[int] = []
        for group_id, params in groups:
            if any(counted_at_run_time(p) for p in params):
                shifting.update(g for g in seen if g)
            seen.append(group_id)

        indices = {}
        # The number of the parameters before the current one, and how many
        # of them are counted in "offset".
        pos = 0
        in_offset = 0
        for group_id, params in groups:
            for i, p in enumerate(params):
                if not counted_at_run_time(p):
                    if len(forward[p.name]) == 1:
                        indices[p.name] = str(min(forward[p.name]))
                    else:
                        indices[p.name] = f"nargs - {min(backward[p.name])}"
                else:
                    index = pos + i - in_offset
                    indices[p.name] = ("offset" if index == 0
                                       else f"offset + {index}")
            pos += len(params)
            if group_id in shifting:
                in_offset += len(params)
        return indices, shifting

    @staticmethod
    def render_option_group_condition(
        subsets: dict[int, ParamTuple],
        group_id: int,
    ) -> str:
        """Return the condition on which the group is passed."""
        counts = sorted(count for count, subset in subsets.items()
                        if any(p.group == group_id for p in subset))
        if counts == list(range(counts[0], max(subsets) + 1)):
            return f"nargs >= {counts[0]}"
        return " || ".join(f"nargs == {count}" for count in counts)

    def render_option_group_check(
        self,
        subsets: dict[int, ParamTuple],
    ) -> str:
        """Return the code which rejects an unsupported number of arguments."""
        error = ('PyErr_SetString(PyExc_TypeError, "%s requires %d to %d '
                 'arguments");' % (self.func.full_name, min(subsets),
                                   max(subsets)))
        counts = sorted(subsets)
        if counts == list(range(counts[0], counts[-1] + 1)):
            tests = [f"nargs > {counts[-1]}"]
            if counts[0]:
                tests.insert(0, f"nargs < {counts[0]}")
            lines = [
                f"if ({' || '.join(tests)}) {{{{",
                f"    {error}",
                "    goto exit;",
                "}}",
            ]
        else:
            # Some numbers of arguments in between are not supported.
            lines = ["switch (nargs) {{"]
            lines += [f"    case {count}:" for count in counts]
            lines += [
                "        break;",
                "    default:",
                f"        {error}",
                "        goto exit;",
                "}}",
            ]
        return libclinic.normalize_snippet("\n".join(lines), indent=4)

    def render_option_groups_parse_tuple(
        self,
        subsets: dict[int, ParamTuple],
    ) -> list[str]:
        """Return the code which parses the arguments of every group at once."""
        parse_call = self.render_parse_all_arguments()
        # The convention can have changed, so ask for it again.
        size_expr, _ = self.select_positional_convention()
        parser_code = [libclinic.normalize_snippet(f"switch ({size_expr}) {{{{",
                                                   indent=4)]
        for count, subset in sorted(subsets.items()):
            parser_code.append(f"        case {count}:")
            if count != len(subset):
                # The omitted parameters are parsed by the following case.
                continue
            if count:
                format_units = [p.converter.format_unit for p in subset]
                n_required = count_required(subset)
                if n_required < count:
                    format_units.insert(n_required, '|')
                parse_arguments: list[str] = []
                for p in subset:
                    p.converter.parse_argument(parse_arguments)
                parser_code.append(libclinic.normalize_snippet("""
                    if (!%s"%s:%s", %s)) {{
                        goto exit;
                    }}
                    """, indent=12) % (parse_call, "".join(format_units),
                                       self.func.name,
                                       ", ".join(parse_arguments)))
            # A set would eliminate duplicates too, but the iteration
            # order of small negative integers depends on the platform.
            group_ids = dict.fromkeys(p.group for p in subset)
            group_ids.pop(0, None)
            for group_id in group_ids:
                parser_code.append(
                    f"            {group_to_variable_name(group_id)} = 1;")
            parser_code.append("            break;")
        parser_code.append("        default:")
        parser_code.append(
            '            PyErr_SetString(PyExc_TypeError, "%s requires %d to %d arguments");'
            % (self.func.full_name, min(subsets), max(subsets)))
        parser_code.append("            goto exit;")
        parser_code.append("    }}")
        return parser_code

    def _parse_vararg(self) -> str:
        assert self.varpos is not None
        c = self.varpos.converter
        assert isinstance(c, libclinic.converters.VarPosCConverter)
        return c.parse_vararg(pos_only=self.pos_only,
                              min_pos=self.min_pos,
                              max_pos=self.max_pos,
                              fastcall=self.fastcall,
                              limited_capi=self.limited_capi)

    def _parse_kwarg(self) -> str:
        assert self.var_keyword is not None
        c = self.var_keyword.converter
        assert isinstance(c, libclinic.converters.VarKeywordCConverter)
        return c.parse_var_keyword()

    def select_positional_convention(self) -> tuple[str, str]:
        """Select the calling convention of a positional-only function.

        Set the method flags and the prototype, and return the expression of
        the number of arguments and the format of an argument expression.
        """
        if self.fastcall:
            self.flags = "METH_FASTCALL"
            self.parser_prototype = PARSER_PROTOTYPE_FASTCALL
            return 'nargs', 'args[%s]'

        self.flags = "METH_VARARGS"
        self.parser_prototype = PARSER_PROTOTYPE_VARARGS
        if self.limited_capi:
            return 'PyTuple_Size(args)', 'PyTuple_GetItem(args, %s)'
        return 'PyTuple_GET_SIZE(args)', 'PyTuple_GET_ITEM(args, %s)'

    def render_parse_all_arguments(self) -> str:
        """Return the beginning of the call which parses all arguments.

        Fall back to the tuple convention if the stack one cannot be used.
        """
        for p in self.parameters:
            p.converter.use_converter()
        if self.limited_capi:
            # _PyArg_ParseStack() is not part of the limited C API.
            self.fastcall = False
        if self.fastcall:
            self.codegen.add_include('pycore_modsupport.h',
                                     '_PyArg_ParseStack()')
            return '_PyArg_ParseStack(args, nargs, '
        self.flags = "METH_VARARGS"
        self.parser_prototype = PARSER_PROTOTYPE_VARARGS
        return 'PyArg_ParseTuple(args, '

    def parse_pos_only(self) -> None:
        # positional-only, but no option groups: we only need one call
        # to _PyArg_ParseStack or PyArg_ParseTuple
        nargs, argname_fmt = self.select_positional_convention()

        parser_code = []
        max_args = NO_VARARG if self.varpos else self.max_pos
        if self.limited_capi:
            if nargs != 'nargs':
                nargs_def = f'Py_ssize_t nargs = {nargs};'
                parser_code.append(libclinic.normalize_snippet(nargs_def, indent=4))
                nargs = 'nargs'
            if self.min_pos == max_args:
                pl = '' if self.min_pos == 1 else 's'
                parser_code.append(libclinic.normalize_snippet(f"""
                    if ({nargs} != {self.min_pos}) {{{{
                        PyErr_Format(PyExc_TypeError, "{{name}} expected {self.min_pos} argument{pl}, got %zd", {nargs});
                        goto exit;
                    }}}}
                    """,
                indent=4))
            else:
                if self.min_pos:
                    pl = '' if self.min_pos == 1 else 's'
                    parser_code.append(libclinic.normalize_snippet(f"""
                        if ({nargs} < {self.min_pos}) {{{{
                            PyErr_Format(PyExc_TypeError, "{{name}} expected at least {self.min_pos} argument{pl}, got %zd", {nargs});
                            goto exit;
                        }}}}
                        """,
                        indent=4))
                if max_args != NO_VARARG:
                    pl = '' if max_args == 1 else 's'
                    parser_code.append(libclinic.normalize_snippet(f"""
                        if ({nargs} > {max_args}) {{{{
                            PyErr_Format(PyExc_TypeError, "{{name}} expected at most {max_args} argument{pl}, got %zd", {nargs});
                            goto exit;
                        }}}}
                        """,
                    indent=4))
        elif self.min_pos or max_args != NO_VARARG:
            self.codegen.add_include('pycore_modsupport.h',
                                     '_PyArg_CheckPositional()')
            parser_code.append(libclinic.normalize_snippet(f"""
                if (!_PyArg_CheckPositional("{{name}}", {nargs}, {self.min_pos}, {max_args})) {{{{
                    goto exit;
                }}}}
                """, indent=4))

        has_optional = False
        use_parser_code = True
        for i, p in enumerate(self.parameters):
            displayname = p.get_displayname(i+1)
            argname = argname_fmt % i
            parsearg: str | None
            parsearg = p.converter.parse_arg(argname, displayname, limited_capi=self.limited_capi)
            if parsearg is None:
                if self.varpos:
                    raise ValueError(
                        f"Using converter {p.converter} is not supported "
                        f"in function with var-positional parameter")
                use_parser_code = False
                parser_code = []
                break
            if has_optional or p.is_optional():
                has_optional = True
                parser_code.append(libclinic.normalize_snippet("""
                    if (%s < %d) {{
                        goto skip_optional;
                    }}
                    """, indent=4) % (nargs, i + 1))
            parser_code.append(libclinic.normalize_snippet(parsearg, indent=4))

        if use_parser_code:
            if has_optional:
                parser_code.append("skip_optional:")
            if self.varpos:
                parser_code.append(libclinic.normalize_snippet(self._parse_vararg(), indent=4))
            elif self.var_keyword:
                parser_code.append(libclinic.normalize_snippet(self._parse_kwarg(), indent=4))
        else:
            parse_call = self.render_parse_all_arguments()
            parser_code = [libclinic.normalize_snippet("""
                if (!%s"{format_units}:{name}",
                    {parse_arguments})) {{
                    goto exit;
                }}
                """, indent=4) % parse_call]
        self.parser_body(*parser_code)

    def parse_var_keyword(self) -> None:
        self.flags = "METH_VARARGS|METH_KEYWORDS"
        self.parser_prototype = PARSER_PROTOTYPE_KEYWORD
        nargs = 'PyTuple_GET_SIZE(args)'

        parser_code = []
        max_args = NO_VARARG if self.varpos else self.max_pos
        if self.varpos is None and self.min_pos == self.max_pos == 0:
            self.codegen.add_include('pycore_modsupport.h',
                                     '_PyArg_NoPositional()')
            parser_code.append(libclinic.normalize_snippet("""
                if (!_PyArg_NoPositional("{name}", args)) {{
                    goto exit;
                }}
                """, indent=4))
        elif self.min_pos or max_args != NO_VARARG:
            self.codegen.add_include('pycore_modsupport.h',
                                     '_PyArg_CheckPositional()')
            parser_code.append(libclinic.normalize_snippet(f"""
                if (!_PyArg_CheckPositional("{{name}}", {nargs}, {self.min_pos}, {max_args})) {{{{
                    goto exit;
                }}}}
                """, indent=4))

        has_optional = False
        for i, p in enumerate(self.parameters):
            parse_arg = p.converter.parse_arg(
                f'PyTuple_GET_ITEM(args, {i})',
                p.get_displayname(i+1),
                limited_capi=self.limited_capi,
            )
            assert parse_arg is not None
            if has_optional or p.is_optional():
                has_optional = True
                parser_code.append(libclinic.normalize_snippet("""
                    if (%s < %d) {{
                        goto skip_optional;
                    }}
                    """, indent=4) % (nargs, i + 1))
            parser_code.append(libclinic.normalize_snippet(parse_arg, indent=4))
        if has_optional:
            parser_code.append("skip_optional:")

        if self.varpos:
            parser_code.append(libclinic.normalize_snippet(self._parse_vararg(), indent=4))
        if self.var_keyword:
            parser_code.append(libclinic.normalize_snippet(self._parse_kwarg(), indent=4))
        self.parser_body(*parser_code)

    def parse_general(self, clang: CLanguage) -> None:
        parsearg: str | None
        deprecated_positionals: dict[int, Parameter] = {}
        deprecated_keywords: dict[int, Parameter] = {}
        for i, p in enumerate(self.parameters):
            if p.deprecated_positional:
                deprecated_positionals[i] = p
            if p.deprecated_keyword:
                deprecated_keywords[i] = p

        has_optional_kw = (
            max(self.pos_only, self.min_pos) + self.min_kw_only
            < len(self.converters)
        )

        use_parser_code = True
        if self.limited_capi:
            parser_code = []
            use_parser_code = False
            self.fastcall = False
        else:
            self.codegen.add_include('pycore_modsupport.h',
                                     '_PyArg_UnpackKeywords()')
            if not self.varpos:
                nargs = "nargs"
            else:
                nargs = f"Py_MIN(nargs, {self.max_pos})" if self.max_pos else "0"

            if self.fastcall:
                self.flags = "METH_FASTCALL|METH_KEYWORDS"
                self.parser_prototype = PARSER_PROTOTYPE_FASTCALL_KEYWORDS
                self.declarations = declare_parser(self.func, codegen=self.codegen)
                self.declarations += "\nPyObject *argsbuf[%s];" % (len(self.converters) or 1)
                if self.varpos:
                    self.declarations += "\nPyObject * const *fastargs;"
                    argsname = 'fastargs'
                    argname_fmt = 'fastargs[%d]'
                else:
                    argsname = 'args'
                    argname_fmt = 'args[%d]'
                if has_optional_kw:
                    self.declarations += "\nPy_ssize_t noptargs = %s + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - %d;" % (nargs, self.min_pos + self.min_kw_only)
                unpack_args = 'args, nargs, NULL, kwnames'
            else:
                # positional-or-keyword arguments
                self.flags = "METH_VARARGS|METH_KEYWORDS"
                self.parser_prototype = PARSER_PROTOTYPE_KEYWORD
                argsname = 'fastargs'
                argname_fmt = 'fastargs[%d]'
                self.declarations = declare_parser(self.func, codegen=self.codegen)
                self.declarations += "\nPyObject *argsbuf[%s];" % (len(self.converters) or 1)
                self.declarations += "\nPyObject * const *fastargs;"
                self.declarations += "\nPy_ssize_t nargs = PyTuple_GET_SIZE(args);"
                if has_optional_kw:
                    self.declarations += "\nPy_ssize_t noptargs = %s + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - %d;" % (nargs, self.min_pos + self.min_kw_only)
                unpack_args = '_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL'
            parser_code = [libclinic.normalize_snippet(f"""
                {argsname} = _PyArg_UnpackKeywords({unpack_args}, &_parser,
                        /*minpos*/ {self.min_pos}, /*maxpos*/ {self.max_pos}, /*minkw*/ {self.min_kw_only}, /*varpos*/ {1 if self.varpos else 0}, argsbuf);
                if (!{argsname}) {{{{
                    goto exit;
                }}}}
                """, indent=4)]

        if self.requires_defining_class:
            self.flags = 'METH_METHOD|' + self.flags
            self.parser_prototype = PARSER_PROTOTYPE_DEF_CLASS

        if use_parser_code:
            if deprecated_keywords:
                code = clang.deprecate_keyword_use(self.func, deprecated_keywords,
                                                   argname_fmt,
                                                   codegen=self.codegen,
                                                   fastcall=self.fastcall)
                parser_code.append(code)

            add_label: str | None = None
            for i, p in enumerate(self.parameters):
                if isinstance(p.converter, defining_class_converter):
                    raise ValueError("defining_class should be the first "
                                    "parameter (after clang)")
                displayname = p.get_displayname(i+1)
                parsearg = p.converter.parse_arg(argname_fmt % i, displayname, limited_capi=self.limited_capi)
                if parsearg is None:
                    parser_code = []
                    use_parser_code = False
                    break
                if add_label and (i == self.pos_only or i == self.max_pos):
                    parser_code.append("%s:" % add_label)
                    add_label = None
                if not p.is_optional():
                    parser_code.append(libclinic.normalize_snippet(parsearg, indent=4))
                elif i < self.pos_only:
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
                    if i < self.max_pos:
                        label = 'skip_optional_pos'
                        first_opt = max(self.min_pos, self.pos_only)
                    else:
                        label = 'skip_optional_kwonly'
                        first_opt = self.max_pos + self.min_kw_only
                    if i == first_opt:
                        add_label = label
                        parser_code.append(libclinic.normalize_snippet("""
                            if (!noptargs) {{
                                goto %s;
                            }}
                            """ % add_label, indent=4))
                    if i + 1 == len(self.parameters):
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

        if use_parser_code:
            if add_label:
                parser_code.append("%s:" % add_label)
            if self.varpos:
                parser_code.append(libclinic.normalize_snippet(self._parse_vararg(), indent=4))
        else:
            for parameter in self.parameters:
                parameter.converter.use_converter()

            self.declarations = declare_parser(self.func, codegen=self.codegen,
                                               hasformat=True)
            if self.limited_capi:
                # positional-or-keyword arguments
                assert not self.fastcall
                self.flags = "METH_VARARGS|METH_KEYWORDS"
                self.parser_prototype = PARSER_PROTOTYPE_KEYWORD
                parser_code = [libclinic.normalize_snippet("""
                    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "{format_units}:{name}", _keywords,
                        {parse_arguments}))
                        goto exit;
                """, indent=4)]
                self.declarations = "static char *_keywords[] = {{{keywords_c} NULL}};"
                if deprecated_positionals or deprecated_keywords:
                    self.declarations += "\nPy_ssize_t nargs = PyTuple_Size(args);"

            elif self.fastcall:
                self.codegen.add_include('pycore_modsupport.h',
                                         '_PyArg_ParseStackAndKeywords()')
                parser_code = [libclinic.normalize_snippet("""
                    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser{parse_arguments_comma}
                        {parse_arguments})) {{
                        goto exit;
                    }}
                    """, indent=4)]
            else:
                self.codegen.add_include('pycore_modsupport.h',
                                         '_PyArg_ParseTupleAndKeywordsFast()')
                parser_code = [libclinic.normalize_snippet("""
                    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
                        {parse_arguments})) {{
                        goto exit;
                    }}
                    """, indent=4)]
                if deprecated_positionals or deprecated_keywords:
                    self.declarations += "\nPy_ssize_t nargs = PyTuple_GET_SIZE(args);"
            if deprecated_keywords:
                code = clang.deprecate_keyword_use(self.func, deprecated_keywords,
                                                   codegen=self.codegen,
                                                   fastcall=self.fastcall)
                parser_code.append(code)

        if deprecated_positionals:
            code = clang.deprecate_positional_use(self.func, deprecated_positionals)
            # Insert the deprecation code before parameter parsing.
            parser_code.insert(0, code)

        assert self.parser_prototype is not None
        self.parser_body(*parser_code, declarations=self.declarations)

    def copy_includes(self) -> None:
        # Copy includes from parameters to Clinic after parse_arg()
        # has been called above.
        converters = self.converters
        if self.varpos:
            converters = converters + [self.varpos.converter]
        for converter in converters:
            for include in converter.get_includes():
                self.codegen.add_include(
                    include.filename,
                    include.reason,
                    condition=include.condition)

    def handle_new_or_init(self) -> None:
        self.methoddef_define = ''

        if self.func.kind is METHOD_NEW:
            self.parser_prototype = PARSER_PROTOTYPE_KEYWORD
        else:
            self.return_value_declaration = "int {parser_retval} = -1;"
            self.parser_prototype = PARSER_PROTOTYPE_KEYWORD___INIT__

        fields: list[str] = list(self.parser_body_fields)
        parses_positional = 'METH_NOARGS' not in self.flags
        parses_keywords = 'METH_KEYWORDS' in self.flags
        if parses_keywords:
            assert parses_positional

        if self.requires_defining_class:
            raise ValueError("Slot methods cannot access their defining class.")

        if not parses_keywords:
            self.declarations = '{base_type_ptr}'
            self.codegen.add_include('pycore_modsupport.h',
                                     '_PyArg_NoKeywords()')
            fields.insert(0, libclinic.normalize_snippet("""
                if ({self_type_check}!_PyArg_NoKeywords("{name}", kwargs)) {{
                    goto exit;
                }}
                """, indent=4))
            if not parses_positional:
                self.codegen.add_include('pycore_modsupport.h',
                                         '_PyArg_NoPositional()')
                fields.insert(0, libclinic.normalize_snippet("""
                    if ({self_type_check}!_PyArg_NoPositional("{name}", args)) {{
                        goto exit;
                    }}
                    """, indent=4))

        self.parser_body(*fields, declarations=self.declarations)

    def process_methoddef(self, clang: CLanguage) -> None:
        methoddef_cast_end = ""
        if self.flags in ('METH_NOARGS', 'METH_O', 'METH_VARARGS'):
            methoddef_cast = "(PyCFunction)"
        elif self.func.kind is GETTER:
            methoddef_cast = "" # This should end up unused
        elif self.limited_capi:
            methoddef_cast = "(PyCFunction)(void(*)(void))"
        else:
            methoddef_cast = "_PyCFunction_CAST("
            methoddef_cast_end = ")"

        if self.func.methoddef_flags:
            self.flags += '|' + self.func.methoddef_flags

        self.methoddef_define = self.methoddef_define.replace('{methoddef_flags}', self.flags)
        self.methoddef_define = self.methoddef_define.replace('{methoddef_cast}', methoddef_cast)
        self.methoddef_define = self.methoddef_define.replace('{methoddef_cast_end}', methoddef_cast_end)

        self.methoddef_ifndef = ''
        conditional = clang.cpp.condition()
        if not conditional:
            self.cpp_if = self.cpp_endif = ''
        else:
            self.cpp_if = "#if " + conditional
            self.cpp_endif = "#endif /* " + conditional + " */"

            if self.methoddef_define and self.codegen.add_ifndef_symbol(self.func.full_name):
                if self.func.kind in ACCESSORS:
                    self.methoddef_ifndef = GETSETDEF_PROTOTYPE_IFNDEF
                else:
                    self.methoddef_ifndef = METHODDEF_PROTOTYPE_IFNDEF

    def finalize(self, clang: CLanguage) -> None:
        # add ';' to the end of self.parser_prototype and self.impl_prototype
        # (they mustn't be None, but they could be an empty string.)
        assert self.parser_prototype is not None
        if self.parser_prototype:
            assert not self.parser_prototype.endswith(';')
            self.parser_prototype += ';'

        if self.impl_prototype is None:
            self.impl_prototype = self.impl_definition
        if self.impl_prototype:
            self.impl_prototype += ";"

        self.parser_definition = self.parser_definition.replace("{return_value_declaration}", self.return_value_declaration)

        compiler_warning = clang.compiler_deprecated_warning(self.func, self.parameters)
        if compiler_warning:
            self.parser_definition = compiler_warning + "\n\n" + self.parser_definition

    def create_template_dict(self) -> dict[str, str]:
        d = {
            "docstring_prototype" : self.docstring_prototype,
            "docstring_definition" : self.docstring_definition,
            "impl_prototype" : self.impl_prototype,
            "methoddef_define" : self.methoddef_define,
            "parser_prototype" : self.parser_prototype,
            "parser_definition" : self.parser_definition,
            "impl_definition" : self.impl_definition,
            "cpp_if" : self.cpp_if,
            "cpp_endif" : self.cpp_endif,
            "methoddef_ifndef" : self.methoddef_ifndef,
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

    def parse_args(self, clang: CLanguage) -> dict[str, str]:
        self.select_prototypes()
        self.init_limited_capi()

        self.flags = ""
        self.declarations = ""
        self.parser_prototype = ""
        self.parser_definition = ""
        self.impl_prototype = None
        self.impl_definition = IMPL_DEFINITION_PROTOTYPE

        # parser_body_fields remembers the fields passed in to the
        # previous call to parser_body. this is used for an awful hack.
        self.parser_body_fields: tuple[str, ...] = ()

        if not self.parameters and not self.varpos and not self.var_keyword:
            self.parse_no_args()
        elif self.use_meth_o():
            self.parse_one_arg()
        elif self.has_option_groups():
            self.parse_option_groups()
        elif self.var_keyword is not None:
            self.parse_var_keyword()
        elif (not self.requires_defining_class
              and self.pos_only == len(self.parameters)):
            self.parse_pos_only()
        else:
            self.parse_general(clang)

        self.copy_includes()
        if self.is_new_or_init():
            self.handle_new_or_init()
        self.process_methoddef(clang)
        self.finalize(clang)

        return self.create_template_dict()
