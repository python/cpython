#!/usr/bin/env python3
#
# Argument Clinic
# Copyright 2012-2013 by Larry Hastings.
# Licensed to the PSF under a contributor agreement.
#

import abc
import ast
import atexit
import clinic
import collections
import contextlib
import functools
import hashlib
import inspect
import io
import itertools
import os
import re
import shlex
import sys
import tempfile
import textwrap

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

version = '1'

_empty = inspect._empty
_void = inspect._void


class Unspecified:
    def __repr__(self):
        return '<Unspecified>'

unspecified = Unspecified()


class Null:
    def __repr__(self):
        return '<Null>'

NULL = Null()


def _text_accumulator():
    text = []
    def output():
        s = ''.join(text)
        text.clear()
        return s
    return text, text.append, output


def text_accumulator():
    """
    Creates a simple text accumulator / joiner.

    Returns a pair of callables:
        append, output
    "append" appends a string to the accumulator.
    "output" returns the contents of the accumulator
       joined together (''.join(accumulator)) and
       empties the accumulator.
    """
    text, append, output = _text_accumulator()
    return append, output


def fail(*args, filename=None, line_number=None):
    joined = " ".join([str(a) for a in args])
    add, output = text_accumulator()
    add("Error")
    if clinic:
        if filename is None:
            filename = clinic.filename
        if clinic.block_parser and (line_number is None):
            line_number = clinic.block_parser.line_number
    if filename is not None:
        add(' in file "' + filename + '"')
    if line_number is not None:
        add(" on line " + str(line_number))
    add(':\n')
    add(joined)
    print(output())
    sys.exit(-1)



def quoted_for_c_string(s):
    for old, new in (
        ('"', '\\"'),
        ("'", "\\'"),
        ):
        s = s.replace(old, new)
    return s

is_legal_c_identifier = re.compile('^[A-Za-z_][A-Za-z0-9_]*$').match

def is_legal_py_identifier(s):
    return all(is_legal_c_identifier(field) for field in s.split('.'))

# added "module", "self", "cls", and "null" just to be safe
# (clinic will generate variables with these names)
c_keywords = set("""
asm auto break case char cls const continue default do double
else enum extern float for goto if inline int long module null
register return self short signed sizeof static struct switch
typedef typeof union unsigned void volatile while
""".strip().split())

def ensure_legal_c_identifier(s):
    # for now, just complain if what we're given isn't legal
    if not is_legal_c_identifier(s):
        fail("Illegal C identifier: {}".format(s))
    # but if we picked a C keyword, pick something else
    if s in c_keywords:
        return s + "_value"
    return s

def rstrip_lines(s):
    text, add, output = _text_accumulator()
    for line in s.split('\n'):
        add(line.rstrip())
        add('\n')
    text.pop()
    return output()

def linear_format(s, **kwargs):
    """
    Perform str.format-like substitution, except:
      * The strings substituted must be on lines by
        themselves.  (This line is the "source line".)
      * If the substitution text is empty, the source line
        is removed in the output.
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

        name, curl, trailing = trailing.partition('}')
        if not curly or name not in kwargs:
            add(line)
            add('\n')
            continue

        if trailing:
            fail("Text found after {" + name + "} block marker!  It must be on a line by itself.")
        if indent.strip():
            fail("Non-whitespace characters found before {" + name + "} block marker!  It must be on a line by itself.")

        value = kwargs[name]
        if not value:
            continue

        value = textwrap.indent(rstrip_lines(value), indent)
        add(value)
        add('\n')

    return output()[:-1]

def version_splitter(s):
    """Splits a version string into a tuple of integers.

    The following ASCII characters are allowed, and employ
    the following conversions:
        a -> -3
        b -> -2
        c -> -1
    (This permits Python-style version strings such as "1.4b3".)
    """
    version = []
    accumulator = []
    def flush():
        if not accumulator:
            raise ValueError('Malformed version string: ' + repr(s))
        version.append(int(''.join(accumulator)))
        accumulator.clear()

    for c in s:
        if c.isdigit():
            accumulator.append(c)
        elif c == '.':
            flush()
        elif c in 'abc':
            flush()
            version.append('abc'.index(c) - 3)
        else:
            raise ValueError('Illegal character ' + repr(c) + ' in version string ' + repr(s))
    flush()
    return tuple(version)

def version_comparitor(version1, version2):
    iterator = itertools.zip_longest(version_splitter(version1), version_splitter(version2), fillvalue=0)
    for i, (a, b) in enumerate(iterator):
        if a < b:
            return -1
        if a > b:
            return 1
    return 0


class CRenderData:
    def __init__(self):

        # The C statements to declare variables.
        # Should be full lines with \n eol characters.
        self.declarations = []

        # The C statements required to initialize the variables before the parse call.
        # Should be full lines with \n eol characters.
        self.initializers = []

        # The entries for the "keywords" array for PyArg_ParseTuple.
        # Should be individual strings representing the names.
        self.keywords = []

        # The "format units" for PyArg_ParseTuple.
        # Should be individual strings that will get
        self.format_units = []

        # The varargs arguments for PyArg_ParseTuple.
        self.parse_arguments = []

        # The parameter declarations for the impl function.
        self.impl_parameters = []

        # The arguments to the impl function at the time it's called.
        self.impl_arguments = []

        # For return converters: the name of the variable that
        # should receive the value returned by the impl.
        self.return_value = "return_value"

        # For return converters: the code to convert the return
        # value from the parse function.  This is also where
        # you should check the _return_value for errors, and
        # "goto exit" if there are any.
        self.return_conversion = []

        # The C statements required to clean up after the impl call.
        self.cleanup = []


class Language(metaclass=abc.ABCMeta):

    start_line = ""
    body_prefix = ""
    stop_line = ""
    checksum_line = ""

    @abc.abstractmethod
    def render(self, block):
        pass

    def validate(self):
        def assert_only_one(field, token='dsl_name'):
            line = getattr(self, field)
            token = '{' + token + '}'
            if len(line.split(token)) != 2:
                fail(self.__class__.__name__ + " " + field + " must contain " + token + " exactly once!")
        assert_only_one('start_line')
        assert_only_one('stop_line')
        assert_only_one('checksum_line')
        assert_only_one('checksum_line', 'checksum')

        if len(self.body_prefix.split('{dsl_name}')) >= 3:
            fail(self.__class__.__name__ + " body_prefix may contain " + token + " once at most!")



class PythonLanguage(Language):

    language      = 'Python'
    start_line    = "#/*[{dsl_name}]"
    body_prefix   = "#"
    stop_line     = "#[{dsl_name}]*/"
    checksum_line = "#/*[{dsl_name} checksum: {checksum}]*/"


def permute_left_option_groups(l):
    """
    Given [1, 2, 3], should yield:
       ()
       (3,)
       (2, 3)
       (1, 2, 3)
    """
    yield tuple()
    accumulator = []
    for group in reversed(l):
        accumulator = list(group) + accumulator
        yield tuple(accumulator)


def permute_right_option_groups(l):
    """
    Given [1, 2, 3], should yield:
      ()
      (1,)
      (1, 2)
      (1, 2, 3)
    """
    yield tuple()
    accumulator = []
    for group in l:
        accumulator.extend(group)
        yield tuple(accumulator)


def permute_optional_groups(left, required, right):
    """
    Generator function that computes the set of acceptable
    argument lists for the provided iterables of
    argument groups.  (Actually it generates a tuple of tuples.)

    Algorithm: prefer left options over right options.

    If required is empty, left must also be empty.
    """
    required = tuple(required)
    result = []

    if not required:
        assert not left

    accumulator = []
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


class CLanguage(Language):

    language      = 'C'
    start_line    = "/*[{dsl_name}]"
    body_prefix   = ""
    stop_line     = "[{dsl_name}]*/"
    checksum_line = "/*[{dsl_name} checksum: {checksum}]*/"

    def render(self, signatures):
        function = None
        for o in signatures:
            if isinstance(o, Function):
                if function:
                    fail("You may specify at most one function per block.\nFound a block containing at least two:\n\t" + repr(function) + " and " + repr(o))
                function = o
        return self.render_function(function)

    def docstring_for_c_string(self, f):
        text, add, output = _text_accumulator()
        # turn docstring into a properly quoted C string
        for line in f.docstring.split('\n'):
            add('"')
            add(quoted_for_c_string(line))
            add('\\n"\n')

        text.pop()
        add('"')
        return ''.join(text)

    impl_prototype_template = "{c_basename}_impl({impl_parameters})"

    @staticmethod
    def template_base(*args):
        flags = '|'.join(f for f in args if f)
        return """
PyDoc_STRVAR({c_basename}__doc__,
{docstring});

#define {methoddef_name}    \\
    {{"{name}", (PyCFunction){c_basename}, {methoddef_flags}, {c_basename}__doc__}},
""".replace('{methoddef_flags}', flags)

    def meth_noargs_template(self, methoddef_flags=""):
        return self.template_base("METH_NOARGS", methoddef_flags) + """
static {impl_return_type}
{impl_prototype};

static PyObject *
{c_basename}({self_type}{self_name}, PyObject *Py_UNUSED(ignored))
{{
    PyObject *return_value = NULL;
    {declarations}
    {initializers}

    {return_value} = {c_basename}_impl({impl_arguments});
    {return_conversion}

{exit_label}
    {cleanup}
    return return_value;
}}

static {impl_return_type}
{impl_prototype}
"""

    def meth_o_template(self, methoddef_flags=""):
        return self.template_base("METH_O", methoddef_flags) + """
static PyObject *
{c_basename}({impl_parameters})
"""

    def meth_o_return_converter_template(self, methoddef_flags=""):
        return self.template_base("METH_O", methoddef_flags) + """
static {impl_return_type}
{impl_prototype};

static PyObject *
{c_basename}({impl_parameters})
{{
    PyObject *return_value = NULL;
    {declarations}
    {initializers}
    _return_value = {c_basename}_impl({impl_arguments});
    {return_conversion}

{exit_label}
    {cleanup}
    return return_value;
}}

static {impl_return_type}
{impl_prototype}
"""

    def option_group_template(self, methoddef_flags=""):
        return self.template_base("METH_VARARGS", methoddef_flags) + """
static {impl_return_type}
{impl_prototype};

static PyObject *
{c_basename}({self_type}{self_name}, PyObject *args)
{{
    PyObject *return_value = NULL;
    {declarations}
    {initializers}

    {option_group_parsing}
    {return_value} = {c_basename}_impl({impl_arguments});
    {return_conversion}

{exit_label}
    {cleanup}
    return return_value;
}}

static {impl_return_type}
{impl_prototype}
"""

    def keywords_template(self, methoddef_flags=""):
        return self.template_base("METH_VARARGS|METH_KEYWORDS", methoddef_flags) + """
static {impl_return_type}
{impl_prototype};

static PyObject *
{c_basename}({self_type}{self_name}, PyObject *args, PyObject *kwargs)
{{
    PyObject *return_value = NULL;
    static char *_keywords[] = {{{keywords}, NULL}};
    {declarations}
    {initializers}

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "{format_units}:{name}", _keywords,
        {parse_arguments}))
        goto exit;
    {return_value} = {c_basename}_impl({impl_arguments});
    {return_conversion}

{exit_label}
    {cleanup}
    return return_value;
}}

static {impl_return_type}
{impl_prototype}
"""

    def positional_only_template(self, methoddef_flags=""):
        return self.template_base("METH_VARARGS", methoddef_flags) + """
static {impl_return_type}
{impl_prototype};

static PyObject *
{c_basename}({self_type}{self_name}, PyObject *args)
{{
    PyObject *return_value = NULL;
    {declarations}
    {initializers}

    if (!PyArg_ParseTuple(args,
        "{format_units}:{name}",
        {parse_arguments}))
        goto exit;
    {return_value} = {c_basename}_impl({impl_arguments});
    {return_conversion}

{exit_label}
    {cleanup}
    return return_value;
}}

static {impl_return_type}
{impl_prototype}
"""

    @staticmethod
    def group_to_variable_name(group):
        adjective = "left_" if group < 0 else "right_"
        return "group_" + adjective + str(abs(group))

    def render_option_group_parsing(self, f, template_dict):
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

        groups = []
        group = None
        left = []
        right = []
        required = []
        last = unspecified

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
            group.append(p)

        count_min = sys.maxsize
        count_max = -1

        add("switch (PyTuple_Size(args)) {{\n")
        for subset in permute_optional_groups(left, required, right):
            count = len(subset)
            count_min = min(count_min, count)
            count_max = max(count_max, count)

            group_ids = {p.group for p in subset}  # eliminate duplicates
            d = {}
            d['count'] = count
            d['name'] = f.name
            d['groups'] = sorted(group_ids)
            d['format_units'] = "".join(p.converter.format_unit for p in subset)

            parse_arguments = []
            for p in subset:
                p.converter.parse_argument(parse_arguments)
            d['parse_arguments'] = ", ".join(parse_arguments)

            group_ids.discard(0)
            lines = [self.group_to_variable_name(g) + " = 1;" for g in group_ids]
            lines = "\n".join(lines)

            s = """
    case {count}:
        if (!PyArg_ParseTuple(args, "{format_units}:{name}", {parse_arguments}))
            return NULL;
        {group_booleans}
        break;
"""[1:]
            s = linear_format(s, group_booleans=lines)
            s = s.format_map(d)
            add(s)

        add("    default:\n")
        s = '        PyErr_SetString(PyExc_TypeError, "{} requires {} to {} arguments");\n'
        add(s.format(f.full_name, count_min, count_max))
        add('        return NULL;\n')
        add("}}")
        template_dict['option_group_parsing'] = output()

    def render_function(self, f):
        if not f:
            return ""

        add, output = text_accumulator()
        data = CRenderData()

        parameters = list(f.parameters.values())
        converters = [p.converter for p in parameters]

        template_dict = {}

        full_name = f.full_name
        template_dict['full_name'] = full_name

        name = full_name.rpartition('.')[2]
        template_dict['name'] = name

        c_basename = f.c_basename or full_name.replace(".", "_")
        template_dict['c_basename'] = c_basename

        methoddef_name = "{}_METHODDEF".format(c_basename.upper())
        template_dict['methoddef_name'] = methoddef_name

        template_dict['docstring'] = self.docstring_for_c_string(f)

        positional = has_option_groups =  False

        if parameters:
            last_group = 0

            for p in parameters:
                c = p.converter

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

            positional = parameters[-1].kind == inspect.Parameter.POSITIONAL_ONLY
            if has_option_groups:
                assert positional

        # now insert our "self" (or whatever) parameters
        # (we deliberately don't call render on self converters)
        stock_self = self_converter('self', f)
        template_dict['self_name'] = stock_self.name
        template_dict['self_type'] = stock_self.type
        data.impl_parameters.insert(0, f.self_converter.type + ("" if f.self_converter.type.endswith('*') else " ") + f.self_converter.name)
        if f.self_converter.type != stock_self.type:
            self_cast = '(' + f.self_converter.type + ')'
        else:
            self_cast = ''
        data.impl_arguments.insert(0, self_cast + stock_self.name)

        f.return_converter.render(f, data)
        template_dict['impl_return_type'] = f.return_converter.type

        template_dict['declarations'] = "\n".join(data.declarations)
        template_dict['initializers'] = "\n\n".join(data.initializers)
        template_dict['keywords'] = '"' + '", "'.join(data.keywords) + '"'
        template_dict['format_units'] = ''.join(data.format_units)
        template_dict['parse_arguments'] = ', '.join(data.parse_arguments)
        template_dict['impl_parameters'] = ", ".join(data.impl_parameters)
        template_dict['impl_arguments'] = ", ".join(data.impl_arguments)
        template_dict['return_conversion'] = "".join(data.return_conversion).rstrip()
        template_dict['cleanup'] = "".join(data.cleanup)
        template_dict['return_value'] = data.return_value

        template_dict['impl_prototype'] = self.impl_prototype_template.format_map(template_dict)

        default_return_converter = (not f.return_converter or
            f.return_converter.type == 'PyObject *')

        if not parameters:
            template = self.meth_noargs_template(f.methoddef_flags)
        elif (len(parameters) == 1 and
              parameters[0].kind == inspect.Parameter.POSITIONAL_ONLY and
              not converters[0].is_optional() and
              isinstance(converters[0], object_converter) and
              converters[0].format_unit == 'O'):
            if default_return_converter:
                template = self.meth_o_template(f.methoddef_flags)
            else:
                # HACK
                # we're using "impl_parameters" for the
                # non-impl function, because that works
                # better for METH_O.  but that means we
                # must supress actually declaring the
                # impl's parameters as variables in the
                # non-impl.  but since it's METH_O, we
                # only have one anyway, so
                # we don't have any problem finding it.
                declarations_copy = list(data.declarations)
                before, pyobject, after = declarations_copy[0].partition('PyObject *')
                assert not before, "hack failed, see comment"
                assert pyobject, "hack failed, see comment"
                assert after and after[0].isalpha(), "hack failed, see comment"
                del declarations_copy[0]
                template_dict['declarations'] = "\n".join(declarations_copy)
                template = self.meth_o_return_converter_template(f.methoddef_flags)
        elif has_option_groups:
            self.render_option_group_parsing(f, template_dict)
            template = self.option_group_template(f.methoddef_flags)
            template = linear_format(template,
                option_group_parsing=template_dict['option_group_parsing'])
        elif positional:
            template = self.positional_only_template(f.methoddef_flags)
        else:
            template = self.keywords_template(f.methoddef_flags)

        template = linear_format(template,
            declarations=template_dict['declarations'],
            return_conversion=template_dict['return_conversion'],
            initializers=template_dict['initializers'],
            cleanup=template_dict['cleanup'],
            )

        # Only generate the "exit:" label
        # if we have any gotos
        need_exit_label = "goto exit;" in template
        template = linear_format(template,
            exit_label="exit:" if need_exit_label else ''
            )

        return template.format_map(template_dict)


@contextlib.contextmanager
def OverrideStdioWith(stdout):
    saved_stdout = sys.stdout
    sys.stdout = stdout
    try:
        yield
    finally:
        assert sys.stdout is stdout
        sys.stdout = saved_stdout


def create_regex(before, after):
    """Create an re object for matching marker lines."""
    pattern = r'^{}(\w+){}$'
    return re.compile(pattern.format(re.escape(before), re.escape(after)))


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

    signatures is either list or None.  If it's a list,
    it may only contain clinic.Module, clinic.Class, and
    clinic.Function objects.  At the moment it should
    contain at most one of each.

    output is either str or None.  If str, it's the output
    from this block, with embedded '\n' characters.

    indent is either str or None.  It's the leading whitespace
    that was found on every line of input.  (If body_prefix is
    not empty, this is the indent *after* removing the
    body_prefix.)

    preindent is either str or None.  It's the whitespace that
    was found in front of every line of input *before* the
    "body_prefix" (see the Language object).  If body_prefix
    is empty, preindent must always be empty too.

    To illustrate indent and preindent: Assume that '_'
    represents whitespace.  If the block processed was in a
    Python file, and looked like this:
      ____#/*[python]
      ____#__for a in range(20):
      ____#____print(a)
      ____#[python]*/
    "preindent" would be "____" and "indent" would be "__".

    """
    def __init__(self, input, dsl_name=None, signatures=None, output=None, indent='', preindent=''):
        assert isinstance(input, str)
        self.input = input
        self.dsl_name = dsl_name
        self.signatures = signatures or []
        self.output = output
        self.indent = indent
        self.preindent = preindent


class BlockParser:
    """
    Block-oriented parser for Argument Clinic.
    Iterator, yields Block objects.
    """

    def __init__(self, input, language, *, verify=True):
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
        self.start_re = create_regex(before, after)
        self.verify = verify
        self.last_checksum_re = None
        self.last_dsl_name = None
        self.dsl_name = None

    def __iter__(self):
        return self

    def __next__(self):
        if not self.input:
            raise StopIteration

        if self.dsl_name:
            return_value = self.parse_clinic_block(self.dsl_name)
            self.dsl_name = None
            return return_value
        return self.parse_verbatim_block()

    def is_start_line(self, line):
        match = self.start_re.match(line.lstrip())
        return match.group(1) if match else None

    def _line(self):
        self.line_number += 1
        return self.input.pop()

    def parse_verbatim_block(self):
        add, output = text_accumulator()
        self.block_start_line_number = self.line_number

        while self.input:
            line = self._line()
            dsl_name = self.is_start_line(line)
            if dsl_name:
                self.dsl_name = dsl_name
                break
            add(line)

        return Block(output())

    def parse_clinic_block(self, dsl_name):
        input_add, input_output = text_accumulator()
        self.block_start_line_number = self.line_number + 1
        stop_line = self.language.stop_line.format(dsl_name=dsl_name) + '\n'
        body_prefix = self.language.body_prefix.format(dsl_name=dsl_name)

        # consume body of program
        while self.input:
            line = self._line()
            if line == stop_line or self.is_start_line(line):
                break
            if body_prefix:
                line = line.lstrip()
                assert line.startswith(body_prefix)
                line = line[len(body_prefix):]
            input_add(line)

        # consume output and checksum line, if present.
        if self.last_dsl_name == dsl_name:
            checksum_re = self.last_checksum_re
        else:
            before, _, after = self.language.checksum_line.format(dsl_name=dsl_name, checksum='{checksum}').partition('{checksum}')
            assert _ == '{checksum}'
            checksum_re = create_regex(before, after)
            self.last_dsl_name = dsl_name
            self.last_checksum_re = checksum_re

        # scan forward for checksum line
        output_add, output_output = text_accumulator()
        checksum = None
        while self.input:
            line = self._line()
            match = checksum_re.match(line.lstrip())
            checksum = match.group(1) if match else None
            if checksum:
                break
            output_add(line)
            if self.is_start_line(line):
                break

        output = output_output()
        if checksum:
            if self.verify:
                computed = compute_checksum(output)
                if checksum != computed:
                    fail("Checksum mismatch!\nExpected: {}\nComputed: {}".format(checksum, computed))
        else:
            # put back output
            self.input.extend(reversed(output.splitlines(keepends=True)))
            self.line_number -= len(output)
            output = None

        return Block(input_output(), dsl_name, output=output)


class BlockPrinter:

    def __init__(self, language, f=None):
        self.language = language
        self.f = f or io.StringIO()

    def print_block(self, block):
        input = block.input
        output = block.output
        dsl_name = block.dsl_name
        write = self.f.write

        assert not ((dsl_name == None) ^ (output == None)), "you must specify dsl_name and output together, dsl_name " + repr(dsl_name)

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

        output = block.output
        if output:
            write(output)
            if not output.endswith('\n'):
                write('\n')

        write(self.language.checksum_line.format(dsl_name=dsl_name, checksum=compute_checksum(output)))
        write("\n")


# maps strings to Language objects.
# "languages" maps the name of the language ("C", "Python").
# "extensions" maps the file extension ("c", "py").
languages = { 'C': CLanguage, 'Python': PythonLanguage }
extensions = { 'c': CLanguage, 'h': CLanguage, 'py': PythonLanguage }


# maps strings to callables.
# these callables must be of the form:
#   def foo(name, default, *, ...)
# The callable may have any number of keyword-only parameters.
# The callable must return a CConverter object.
# The callable should not call builtins.print.
converters = {}

# maps strings to callables.
# these callables follow the same rules as those for "converters" above.
# note however that they will never be called with keyword-only parameters.
legacy_converters = {}


# maps strings to callables.
# these callables must be of the form:
#   def foo(*, ...)
# The callable may have any number of keyword-only parameters.
# The callable must return a CConverter object.
# The callable should not call builtins.print.
return_converters = {}

class Clinic:
    def __init__(self, language, printer=None, *, verify=True, filename=None):
        # maps strings to Parser objects.
        # (instantiated from the "parsers" global.)
        self.parsers = {}
        self.language = language
        self.printer = printer or BlockPrinter(language)
        self.verify = verify
        self.filename = filename
        self.modules = collections.OrderedDict()
        self.classes = collections.OrderedDict()

        global clinic
        clinic = self

    def parse(self, input):
        printer = self.printer
        self.block_parser = BlockParser(input, self.language, verify=self.verify)
        for block in self.block_parser:
            dsl_name = block.dsl_name
            if dsl_name:
                if dsl_name not in self.parsers:
                    assert dsl_name in parsers, "No parser to handle {!r} block.".format(dsl_name)
                    self.parsers[dsl_name] = parsers[dsl_name](self)
                parser = self.parsers[dsl_name]
                parser.parse(block)
            printer.print_block(block)
        return printer.f.getvalue()

    def _module_and_class(self, fields):
        """
        fields should be an iterable of field names.
        returns a tuple of (module, class).
        the module object could actually be self (a clinic object).
        this function is only ever used to find the parent of where
        a new class/module should go.
        """
        in_classes = False
        parent = module = self
        cls = None
        so_far = []

        for field in fields:
            so_far.append(field)
            if not in_classes:
                child = parent.modules.get(field)
                if child:
                    parent = module = child
                    continue
                in_classes = True
            if not hasattr(parent, 'classes'):
                return module, cls
            child = parent.classes.get(field)
            if not child:
                fail('Parent class or module ' + '.'.join(so_far) + " does not exist.")
            cls = parent = child

        return module, cls


def parse_file(filename, *, verify=True, output=None, encoding='utf-8'):
    extension = os.path.splitext(filename)[1][1:]
    if not extension:
        fail("Can't extract file type for file " + repr(filename))

    try:
        language = extensions[extension]()
    except KeyError:
        fail("Can't identify file type for file " + repr(filename))

    clinic = Clinic(language, verify=verify, filename=filename)

    with open(filename, 'r', encoding=encoding) as f:
        raw = f.read()

    cooked = clinic.parse(raw)
    if cooked == raw:
        return

    directory = os.path.dirname(filename) or '.'

    with tempfile.TemporaryDirectory(prefix="clinic", dir=directory) as tmpdir:
        bytes = cooked.encode(encoding)
        tmpfilename = os.path.join(tmpdir, os.path.basename(filename))
        with open(tmpfilename, "wb") as f:
            f.write(bytes)
        os.replace(tmpfilename, output or filename)


def compute_checksum(input):
    input = input or ''
    return hashlib.sha1(input.encode('utf-8')).hexdigest()




class PythonParser:
    def __init__(self, clinic):
        pass

    def parse(self, block):
        s = io.StringIO()
        with OverrideStdioWith(s):
            exec(block.input)
        block.output = s.getvalue()


class Module:
    def __init__(self, name, module=None):
        self.name = name
        self.module = self.parent = module

        self.modules = collections.OrderedDict()
        self.classes = collections.OrderedDict()
        self.functions = []

    def __repr__(self):
        return "<clinic.Module " + repr(self.name) + " at " + str(id(self)) + ">"

class Class:
    def __init__(self, name, module=None, cls=None):
        self.name = name
        self.module = module
        self.cls = cls
        self.parent = cls or module

        self.classes = collections.OrderedDict()
        self.functions = []

    def __repr__(self):
        return "<clinic.Class " + repr(self.name) + " at " + str(id(self)) + ">"


DATA, CALLABLE, METHOD, STATIC_METHOD, CLASS_METHOD = range(5)

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

    def __init__(self, parameters=None, *, name,
                 module, cls=None, c_basename=None,
                 full_name=None,
                 return_converter, return_annotation=_empty,
                 docstring=None, kind=CALLABLE, coexist=False):
        self.parameters = parameters or collections.OrderedDict()
        self.return_annotation = return_annotation
        self.name = name
        self.full_name = full_name
        self.module = module
        self.cls = cls
        self.parent = cls or module
        self.c_basename = c_basename
        self.return_converter = return_converter
        self.docstring = docstring or ''
        self.kind = kind
        self.coexist = coexist
        self.self_converter = None

    @property
    def methoddef_flags(self):
        flags = []
        if self.kind == CLASS_METHOD:
            flags.append('METH_CLASS')
        elif self.kind == STATIC_METHOD:
            flags.append('METH_STATIC')
        else:
            assert self.kind == CALLABLE, "unknown kind: " + repr(self.kind)
        if self.coexist:
            flags.append('METH_COEXIST')
        return '|'.join(flags)

    def __repr__(self):
        return '<clinic.Function ' + self.name + '>'


class Parameter:
    """
    Mutable duck type of inspect.Parameter.
    """

    def __init__(self, name, kind, *, default=_empty,
                 function, converter, annotation=_empty,
                 docstring=None, group=0):
        self.name = name
        self.kind = kind
        self.default = default
        self.function = function
        self.converter = converter
        self.annotation = annotation
        self.docstring = docstring or ''
        self.group = group

    def __repr__(self):
        return '<clinic.Parameter ' + self.name + '>'

    def is_keyword_only(self):
        return self.kind == inspect.Parameter.KEYWORD_ONLY

py_special_values = {
    NULL: "None",
}

def py_repr(o):
    special = py_special_values.get(o)
    if special:
        return special
    return repr(o)


c_special_values = {
    NULL: "NULL",
    None: "Py_None",
}

def c_repr(o):
    special = c_special_values.get(o)
    if special:
        return special
    if isinstance(o, str):
        return '"' + quoted_for_c_string(o) + '"'
    return repr(o)

def add_c_converter(f, name=None):
    if not name:
        name = f.__name__
        if not name.endswith('_converter'):
            return f
        name = name[:-len('_converter')]
    converters[name] = f
    return f

def add_default_legacy_c_converter(cls):
    # automatically add converter for default format unit
    # (but without stomping on the existing one if it's already
    # set, in case you subclass)
    if ((cls.format_unit != 'O&') and
        (cls.format_unit not in legacy_converters)):
        legacy_converters[cls.format_unit] = cls
    return cls

def add_legacy_c_converter(format_unit, **kwargs):
    """
    Adds a legacy converter.
    """
    def closure(f):
        if not kwargs:
            added_f = f
        else:
            added_f = functools.partial(f, **kwargs)
        legacy_converters[format_unit] = added_f
        return f
    return closure

class CConverterAutoRegister(type):
    def __init__(cls, name, bases, classdict):
        add_c_converter(cls)
        add_default_legacy_c_converter(cls)

class CConverter(metaclass=CConverterAutoRegister):
    """
    For the init function, self, name, function, and default
    must be keyword-or-positional parameters.  All other
    parameters (including "required" and "doc_default")
    must be keyword-only.
    """

    type = None
    format_unit = 'O&'

    # The Python default value for this parameter, as a Python value.
    # Or "unspecified" if there is no default.
    default = unspecified

    # "default" as it should appear in the documentation, as a string.
    # Or None if there is no default.
    doc_default = None

    # "default" converted into a str for rendering into Python code.
    py_default = None

    # "default" converted into a C value, as a string.
    # Or None if there is no default.
    c_default = None

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
    c_ignored_default = 'NULL'

    # The C converter *function* to be used, if any.
    # (If this is not None, format_unit must be 'O&'.)
    converter = None

    encoding = None
    impl_by_reference = False
    parse_by_reference = True
    length = False

    def __init__(self, name, function, default=unspecified, *, doc_default=None, required=False, annotation=unspecified, **kwargs):
        self.function = function
        self.name = name

        if default is not unspecified:
            self.default = default
            self.py_default = py_repr(default)
            self.doc_default = doc_default if doc_default is not None else self.py_default
            self.c_default = c_repr(default)
        elif doc_default is not None:
            fail(function.fullname + " argument " + name + " specified a 'doc_default' without having a 'default'")
        if annotation != unspecified:
            fail("The 'annotation' parameter is not currently permitted.")
        self.required = required
        self.converter_init(**kwargs)

    def converter_init(self):
        pass

    def is_optional(self):
        return (self.default is not unspecified) and (not self.required)

    def render(self, parameter, data):
        """
        parameter is a clinic.Parameter instance.
        data is a CRenderData instance.
        """
        self.parameter = parameter
        name = ensure_legal_c_identifier(self.name)

        # declarations
        d = self.declaration()
        data.declarations.append(d)

        # initializers
        initializers = self.initialize()
        if initializers:
            data.initializers.append('/* initializers for ' + name + ' */\n' + initializers.rstrip())

        # impl_arguments
        s = ("&" if self.impl_by_reference else "") + name
        data.impl_arguments.append(s)
        if self.length:
            data.impl_arguments.append(self.length_name())

        # keywords
        data.keywords.append(name)

        # format_units
        if self.is_optional() and '|' not in data.format_units:
            data.format_units.append('|')
        if parameter.is_keyword_only() and '$' not in data.format_units:
            data.format_units.append('$')
        data.format_units.append(self.format_unit)

        # parse_arguments
        self.parse_argument(data.parse_arguments)

        # impl_parameters
        data.impl_parameters.append(self.simple_declaration(by_reference=self.impl_by_reference))
        if self.length:
            data.impl_parameters.append("Py_ssize_clean_t " + self.length_name())

        # cleanup
        cleanup = self.cleanup()
        if cleanup:
            data.cleanup.append('/* Cleanup for ' + name + ' */\n' + cleanup.rstrip() + "\n")

    def length_name(self):
        """Computes the name of the associated "length" variable."""
        if not self.length:
            return None
        return ensure_legal_c_identifier(self.name) + "_length"

    # Why is this one broken out separately?
    # For "positional-only" function parsing,
    # which generates a bunch of PyArg_ParseTuple calls.
    def parse_argument(self, list):
        assert not (self.converter and self.encoding)
        if self.format_unit == 'O&':
            assert self.converter
            list.append(self.converter)

        if self.encoding:
            list.append(self.encoding)

        legal_name = ensure_legal_c_identifier(self.name)
        s = ("&" if self.parse_by_reference else "") + legal_name
        list.append(s)

        if self.length:
            list.append("&" + self.length_name())

    #
    # All the functions after here are intended as extension points.
    #

    def simple_declaration(self, by_reference=False):
        """
        Computes the basic declaration of the variable.
        Used in computing the prototype declaration and the
        variable declaration.
        """
        prototype = [self.type]
        if by_reference or not self.type.endswith('*'):
            prototype.append(" ")
        if by_reference:
            prototype.append('*')
        prototype.append(ensure_legal_c_identifier(self.name))
        return "".join(prototype)

    def declaration(self):
        """
        The C statement to declare this variable.
        """
        declaration = [self.simple_declaration()]
        default = self.c_default
        if not default and self.parameter.group:
            default = self.c_ignored_default
        if default:
            declaration.append(" = ")
            declaration.append(default)
        declaration.append(";")
        if self.length:
            declaration.append('\nPy_ssize_clean_t ')
            declaration.append(self.length_name())
            declaration.append(';')
        return "".join(declaration)

    def initialize(self):
        """
        The C statements required to set up this variable before parsing.
        Returns a string containing this code indented at column 0.
        If no initialization is necessary, returns an empty string.
        """
        return ""

    def cleanup(self):
        """
        The C statements required to clean up after this variable.
        Returns a string containing this code indented at column 0.
        If no cleanup is necessary, returns an empty string.
        """
        return ""


class bool_converter(CConverter):
    type = 'int'
    format_unit = 'p'
    c_ignored_default = '0'

    def converter_init(self):
        self.default = bool(self.default)
        self.c_default = str(int(self.default))

class char_converter(CConverter):
    type = 'char'
    format_unit = 'c'
    c_ignored_default = "'\0'"

@add_legacy_c_converter('B', bitwise=True)
class byte_converter(CConverter):
    type = 'byte'
    format_unit = 'b'
    c_ignored_default = "'\0'"

    def converter_init(self, *, bitwise=False):
        if bitwise:
            self.format_unit = 'B'

class short_converter(CConverter):
    type = 'short'
    format_unit = 'h'
    c_ignored_default = "0"

class unsigned_short_converter(CConverter):
    type = 'unsigned short'
    format_unit = 'H'
    c_ignored_default = "0"

    def converter_init(self, *, bitwise=False):
        if not bitwise:
            fail("Unsigned shorts must be bitwise (for now).")

@add_legacy_c_converter('C', types='str')
class int_converter(CConverter):
    type = 'int'
    format_unit = 'i'
    c_ignored_default = "0"

    def converter_init(self, *, types='int'):
        if types == 'str':
            self.format_unit = 'C'
        elif types != 'int':
            fail("int_converter: illegal 'types' argument")

class unsigned_int_converter(CConverter):
    type = 'unsigned int'
    format_unit = 'I'
    c_ignored_default = "0"

    def converter_init(self, *, bitwise=False):
        if not bitwise:
            fail("Unsigned ints must be bitwise (for now).")

class long_converter(CConverter):
    type = 'long'
    format_unit = 'l'
    c_ignored_default = "0"

class unsigned_long_converter(CConverter):
    type = 'unsigned long'
    format_unit = 'k'
    c_ignored_default = "0"

    def converter_init(self, *, bitwise=False):
        if not bitwise:
            fail("Unsigned longs must be bitwise (for now).")

class PY_LONG_LONG_converter(CConverter):
    type = 'PY_LONG_LONG'
    format_unit = 'L'
    c_ignored_default = "0"

class unsigned_PY_LONG_LONG_converter(CConverter):
    type = 'unsigned PY_LONG_LONG'
    format_unit = 'K'
    c_ignored_default = "0"

    def converter_init(self, *, bitwise=False):
        if not bitwise:
            fail("Unsigned PY_LONG_LONGs must be bitwise (for now).")

class Py_ssize_t_converter(CConverter):
    type = 'Py_ssize_t'
    format_unit = 'n'
    c_ignored_default = "0"


class float_converter(CConverter):
    type = 'float'
    format_unit = 'f'
    c_ignored_default = "0.0"

class double_converter(CConverter):
    type = 'double'
    format_unit = 'd'
    c_ignored_default = "0.0"


class Py_complex_converter(CConverter):
    type = 'Py_complex'
    format_unit = 'D'
    c_ignored_default = "{0.0, 0.0}"


class object_converter(CConverter):
    type = 'PyObject *'
    format_unit = 'O'

    def converter_init(self, *, type=None):
        if type:
            assert isinstance(type, str)
            assert type.isidentifier()
            try:
                type = eval(type)
                # need more of these!
                type = {
                    str: '&PyUnicode_Type',
                    }[type]
            except NameError:
                type = type
            self.format_unit = 'O!'
            self.encoding = type


@add_legacy_c_converter('s#', length=True)
@add_legacy_c_converter('y', type="bytes")
@add_legacy_c_converter('y#', type="bytes", length=True)
@add_legacy_c_converter('z', nullable=True)
@add_legacy_c_converter('z#', nullable=True, length=True)
class str_converter(CConverter):
    type = 'const char *'
    format_unit = 's'

    def converter_init(self, *, encoding=None, types="str",
        length=False, nullable=False, zeroes=False):

        types = set(types.strip().split())
        bytes_type = set(("bytes",))
        str_type = set(("str",))
        all_3_type = set(("bytearray",)) | bytes_type | str_type
        is_bytes = types == bytes_type
        is_str = types == str_type
        is_all_3 = types == all_3_type

        self.length = bool(length)
        format_unit = None

        if encoding:
            self.encoding = encoding

            if is_str and not (length or zeroes or nullable):
                format_unit = 'es'
            elif is_all_3 and not (length or zeroes or nullable):
                format_unit = 'et'
            elif is_str and length and zeroes and not nullable:
                format_unit = 'es#'
            elif is_all_3 and length and not (nullable or zeroes):
                format_unit = 'et#'

            if format_unit.endswith('#'):
                print("Warning: code using format unit ", repr(format_unit), "probably doesn't work properly.")
                # TODO set pointer to NULL
                # TODO add cleanup for buffer
                pass

        else:
            if zeroes:
                fail("str_converter: illegal combination of arguments (zeroes is only legal with an encoding)")

            if is_bytes and not (nullable or length):
                format_unit = 'y'
            elif is_bytes and length and not nullable:
                format_unit = 'y#'
            elif is_str and not (nullable or length):
                format_unit = 's'
            elif is_str and length and not nullable:
                format_unit = 's#'
            elif is_str and nullable  and not length:
                format_unit = 'z'
            elif is_str and nullable and length:
                format_unit = 'z#'

        if not format_unit:
            fail("str_converter: illegal combination of arguments")
        self.format_unit = format_unit


class PyBytesObject_converter(CConverter):
    type = 'PyBytesObject *'
    format_unit = 'S'

class PyByteArrayObject_converter(CConverter):
    type = 'PyByteArrayObject *'
    format_unit = 'Y'

class unicode_converter(CConverter):
    type = 'PyObject *'
    format_unit = 'U'

@add_legacy_c_converter('u#', length=True)
@add_legacy_c_converter('Z', nullable=True)
@add_legacy_c_converter('Z#', nullable=True, length=True)
class Py_UNICODE_converter(CConverter):
    type = 'Py_UNICODE *'
    format_unit = 'u'

    def converter_init(self, *, nullable=False, length=False):
        format_unit = 'Z' if nullable else 'u'
        if length:
            format_unit += '#'
            self.length = True
        self.format_unit = format_unit

#
# We define three string conventions for buffer types in the 'types' argument:
#  'buffer' : any object supporting the buffer interface
#  'rwbuffer': any object supporting the buffer interface, but must be writeable
#  'robuffer': any object supporting the buffer interface, but must not be writeable
#
@add_legacy_c_converter('s*', types='str bytes bytearray buffer')
@add_legacy_c_converter('z*', types='str bytes bytearray buffer', nullable=True)
@add_legacy_c_converter('w*', types='bytearray rwbuffer')
class Py_buffer_converter(CConverter):
    type = 'Py_buffer'
    format_unit = 'y*'
    impl_by_reference = True
    c_ignored_default = "{NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL}"

    def converter_init(self, *, types='bytes bytearray buffer', nullable=False):
        types = set(types.strip().split())
        bytes_type = set(('bytes',))
        bytearray_type = set(('bytearray',))
        buffer_type = set(('buffer',))
        rwbuffer_type = set(('rwbuffer',))
        robuffer_type = set(('robuffer',))
        str_type = set(('str',))
        bytes_bytearray_buffer_type = bytes_type | bytearray_type | buffer_type

        format_unit = None
        if types == (str_type | bytes_bytearray_buffer_type):
            format_unit = 's*' if not nullable else 'z*'
        else:
            if nullable:
                fail('Py_buffer_converter: illegal combination of arguments (nullable=True)')
            elif types == (bytes_bytearray_buffer_type):
                format_unit = 'y*'
            elif types == (bytearray_type | rwuffer_type):
                format_unit = 'w*'
        if not format_unit:
            fail("Py_buffer_converter: illegal combination of arguments")

        self.format_unit = format_unit

    def cleanup(self):
        name = ensure_legal_c_identifier(self.name)
        return "".join(["if (", name, ".buf)\n   PyBuffer_Release(&", name, ");\n"])


class self_converter(CConverter):
    """
    A special-case converter:
    this is the default converter used for "self".
    """
    type = "PyObject *"
    def converter_init(self):
        f = self.function
        if f.kind == CALLABLE:
            if f.cls:
                self.name = "self"
            else:
                self.name = "module"
                self.type = "PyModuleDef *"
        elif f.kind == STATIC_METHOD:
            self.name = "null"
            self.type = "void *"
        elif f.kind == CLASS_METHOD:
            self.name = "cls"
            self.type = "PyTypeObject *"

    def render(self, parameter, data):
        fail("render() should never be called on self_converter instances")



def add_c_return_converter(f, name=None):
    if not name:
        name = f.__name__
        if not name.endswith('_return_converter'):
            return f
        name = name[:-len('_return_converter')]
    return_converters[name] = f
    return f


class CReturnConverterAutoRegister(type):
    def __init__(cls, name, bases, classdict):
        add_c_return_converter(cls)

class CReturnConverter(metaclass=CReturnConverterAutoRegister):

    type = 'PyObject *'
    default = None

    def __init__(self, *, doc_default=None, **kwargs):
        self.doc_default = doc_default
        try:
            self.return_converter_init(**kwargs)
        except TypeError as e:
            s = ', '.join(name + '=' + repr(value) for name, value in kwargs.items())
            sys.exit(self.__class__.__name__ + '(' + s + ')\n' + str(e))

    def return_converter_init(self):
        pass

    def declare(self, data, name="_return_value"):
        line = []
        add = line.append
        add(self.type)
        if not self.type.endswith('*'):
            add(' ')
        add(name + ';')
        data.declarations.append(''.join(line))
        data.return_value = name

    def err_occurred_if(self, expr, data):
        data.return_conversion.append('if (({}) && PyErr_Occurred())\n    goto exit;\n'.format(expr))

    def err_occurred_if_null_pointer(self, variable, data):
        data.return_conversion.append('if ({} == NULL)\n    goto exit;\n'.format(variable))

    def render(self, function, data):
        """
        function is a clinic.Function instance.
        data is a CRenderData instance.
        """
        pass

add_c_return_converter(CReturnConverter, 'object')

class int_return_converter(CReturnConverter):
    type = 'int'

    def render(self, function, data):
        self.declare(data)
        self.err_occurred_if("_return_value == -1", data)
        data.return_conversion.append(
            'return_value = PyLong_FromLong((long)_return_value);\n')


class long_return_converter(CReturnConverter):
    type = 'long'

    def render(self, function, data):
        self.declare(data)
        self.err_occurred_if("_return_value == -1", data)
        data.return_conversion.append(
            'return_value = PyLong_FromLong(_return_value);\n')


class Py_ssize_t_return_converter(CReturnConverter):
    type = 'Py_ssize_t'

    def render(self, function, data):
        self.declare(data)
        self.err_occurred_if("_return_value == -1", data)
        data.return_conversion.append(
            'return_value = PyLong_FromSsize_t(_return_value);\n')


class DecodeFSDefault_return_converter(CReturnConverter):
    type = 'char *'

    def render(self, function, data):
        self.declare(data)
        self.err_occurred_if_null_pointer("_return_value", data)
        data.return_conversion.append(
            'return_value = PyUnicode_DecodeFSDefault(_return_value);\n')


class IndentStack:
    def __init__(self):
        self.indents = []
        self.margin = None

    def _ensure(self):
        if not self.indents:
            fail('IndentStack expected indents, but none are defined.')

    def measure(self, line):
        """
        Returns the length of the line's margin.
        """
        if '\t' in line:
            fail('Tab characters are illegal in the Clinic DSL.')
        stripped = line.lstrip()
        if not len(stripped):
            # we can't tell anything from an empty line
            # so just pretend it's indented like our current indent
            self._ensure()
            return self.indents[-1]
        return len(line) - len(stripped)

    def infer(self, line):
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
    def depth(self):
        """
        Returns how many margins are currently defined.
        """
        return len(self.indents)

    def indent(self, line):
        """
        Indents a line by the currently defined margin.
        """
        return self.margin + line

    def dedent(self, line):
        """
        Dedents a line by the currently defined margin.
        (The inverse of 'indent'.)
        """
        margin = self.margin
        indent = self.indents[-1]
        if not line.startswith(margin):
            fail('Cannot dedent, line does not start with the previous margin:')
        return line[indent:]


class DSLParser:
    def __init__(self, clinic):
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

    def reset(self):
        self.function = None
        self.state = self.state_dsl_start
        self.parameter_indent = None
        self.keyword_only = False
        self.group = 0
        self.parameter_state = self.ps_start
        self.indent = IndentStack()
        self.kind = CALLABLE
        self.coexist = False

    def directive_version(self, required):
        global version
        if version_comparitor(version, required) < 0:
            fail("Insufficient Clinic version!\n  Version: " + version + "\n  Required: " + required)

    def directive_module(self, name):
        fields = name.split('.')
        new = fields.pop()
        module, cls = self.clinic._module_and_class(fields)
        if cls:
            fail("Can't nest a module inside a class!")
        m = Module(name, module)
        module.modules[name] = m
        self.block.signatures.append(m)

    def directive_class(self, name):
        fields = name.split('.')
        in_classes = False
        parent = self
        name = fields.pop()
        so_far = []
        module, cls = self.clinic._module_and_class(fields)

        c = Class(name, module, cls)
        if cls:
            cls.classes[name] = c
        else:
            module.classes[name] = c
        self.block.signatures.append(c)

    def at_classmethod(self):
        assert self.kind is CALLABLE
        self.kind = CLASS_METHOD

    def at_staticmethod(self):
        assert self.kind is CALLABLE
        self.kind = STATIC_METHOD

    def at_coexist(self):
        assert self.coexist == False
        self.coexist = True


    def parse(self, block):
        self.reset()
        self.block = block
        block_start = self.clinic.block_parser.line_number
        lines = block.input.split('\n')
        for line_number, line in enumerate(lines, self.clinic.block_parser.block_start_line_number):
            if '\t' in line:
                fail('Tab characters are illegal in the Clinic DSL.\n\t' + repr(line), line_number=block_start)
            self.state(line)

        self.next(self.state_terminal)
        self.state(None)

        block.output = self.clinic.language.render(block.signatures)

    @staticmethod
    def ignore_line(line):
        # ignore comment-only lines
        if line.lstrip().startswith('#'):
            return True

        # Ignore empty lines too
        # (but not in docstring sections!)
        if not line.strip():
            return True

        return False

    @staticmethod
    def calculate_indent(line):
        return len(line) - len(line.strip())

    def next(self, state, line=None):
        # real_print(self.state.__name__, "->", state.__name__, ", line=", line)
        self.state = state
        if line is not None:
            self.state(line)

    def state_dsl_start(self, line):
        # self.block = self.ClinicOutputBlock(self)
        if self.ignore_line(line):
            return
        self.next(self.state_modulename_name, line)

    def state_modulename_name(self, line):
        # looking for declaration, which establishes the leftmost column
        # line should be
        #     modulename.fnname [as c_basename] [-> return annotation]
        # square brackets denote optional syntax.
        #
        # (but we might find a directive first!)
        #
        # this line is permitted to start with whitespace.
        # we'll call this number of spaces F (for "function").

        if not line.strip():
            return

        self.indent.infer(line)

        # is it a directive?
        fields = shlex.split(line)
        directive_name = fields[0]
        directive = self.directives.get(directive_name, None)
        if directive:
            directive(*fields[1:])
            return

        line, _, returns = line.partition('->')

        full_name, _, c_basename = line.partition(' as ')
        full_name = full_name.strip()
        c_basename = c_basename.strip() or None

        if not is_legal_py_identifier(full_name):
            fail("Illegal function name: {}".format(full_name))
        if c_basename and not is_legal_c_identifier(c_basename):
            fail("Illegal C basename: {}".format(c_basename))

        if not returns:
            return_converter = CReturnConverter()
        else:
            ast_input = "def x() -> {}: pass".format(returns)
            module = None
            try:
                module = ast.parse(ast_input)
            except SyntaxError:
                pass
            if not module:
                fail("Badly-formed annotation for " + full_name + ": " + returns)
            try:
                name, legacy, kwargs = self.parse_converter(module.body[0].returns)
                assert not legacy
                if name not in return_converters:
                    fail("Error: No available return converter called " + repr(name))
                return_converter = return_converters[name](**kwargs)
            except ValueError:
                fail("Badly-formed annotation for " + full_name + ": " + returns)

        fields = [x.strip() for x in full_name.split('.')]
        function_name = fields.pop()
        module, cls = self.clinic._module_and_class(fields)

        if not module:
            fail("Undefined module used in declaration of " + repr(full_name.strip()) + ".")
        self.function = Function(name=function_name, full_name=full_name, module=module, cls=cls, c_basename=c_basename,
                                 return_converter=return_converter, kind=self.kind, coexist=self.coexist)
        self.block.signatures.append(self.function)
        self.next(self.state_parameters_start)

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
    ##############
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
    # separate boolean state variables.)  The states are:
    #
    #  [ [ a, b, ] c, ] d, e, f, [ g, h, [ i ] ] /   <- line
    # 01   2          3          4           5   6   <- state transitions
    #
    # 0: ps_start.  before we've seen anything.  legal transitions are to 1 or 3.
    # 1: ps_left_square_before.  left square brackets before required parameters.
    # 2: ps_group_before.  in a group, before required parameters.
    # 3: ps_required.  required parameters.  (renumber left groups!)
    # 4: ps_group_after.  in a group, after required parameters.
    # 5: ps_right_square_after.  right square brackets after required parameters.
    # 6: ps_seen_slash.  seen slash.
    ps_start, ps_left_square_before, ps_group_before, ps_required, \
    ps_group_after, ps_right_square_after, ps_seen_slash = range(7)

    def state_parameters_start(self, line):
        if self.ignore_line(line):
            return

        # if this line is not indented, we have no parameters
        if not self.indent.infer(line):
            return self.next(self.state_function_docstring, line)

        return self.next(self.state_parameter, line)


    def to_required(self):
        """
        Transition to the "required" parameter state.
        """
        if self.parameter_state != self.ps_required:
            self.parameter_state = self.ps_required
            for p in self.function.parameters.values():
                p.group = -p.group

    def state_parameter(self, line):
        if self.ignore_line(line):
            return

        assert self.indent.depth == 2
        indent = self.indent.infer(line)
        if indent == -1:
            # we outdented, must be to definition column
            return self.next(self.state_function_docstring, line)

        if indent == 1:
            # we indented, must be to new parameter docstring column
            return self.next(self.state_parameter_docstring_start, line)

        line = line.lstrip()

        if line in ('*', '/', '[', ']'):
            self.parse_special_symbol(line)
            return

        if self.parameter_state in (self.ps_start, self.ps_required):
            self.to_required()
        elif self.parameter_state == self.ps_left_square_before:
            self.parameter_state = self.ps_group_before
        elif self.parameter_state == self.ps_group_before:
            if not self.group:
                self.to_required()
        elif self.parameter_state == self.ps_group_after:
            pass
        else:
            fail("Function " + self.function.name + " has an unsupported group configuration. (Unexpected state " + str(self.parameter_state) + ")")

        ast_input = "def x({}): pass".format(line)
        module = None
        try:
            module = ast.parse(ast_input)
        except SyntaxError:
            pass
        if not module:
            fail("Function " + self.function.name + " has an invalid parameter declaration:\n\t" + line)

        function_args = module.body[0].args
        parameter = function_args.args[0]

        if function_args.defaults:
            expr = function_args.defaults[0]
            # mild hack: explicitly support NULL as a default value
            if isinstance(expr, ast.Name) and expr.id == 'NULL':
                value = NULL
            else:
                value = ast.literal_eval(expr)
        else:
            value = unspecified

        parameter_name = parameter.arg
        name, legacy, kwargs = self.parse_converter(parameter.annotation)
        dict = legacy_converters if legacy else converters
        legacy_str = "legacy " if legacy else ""
        if name not in dict:
            fail('{} is not a valid {}converter'.format(name, legacy_str))
        converter = dict[name](parameter_name, self.function, value, **kwargs)

        # special case: if it's the self converter,
        # don't actually add it to the parameter list
        if isinstance(converter, self_converter):
            if self.function.parameters or (self.parameter_state != self.ps_required):
                fail("The 'self' parameter, if specified, must be the very first thing in the parameter block.")
            if self.function.self_converter:
                fail("You can't specify the 'self' parameter more than once.")
            self.function.self_converter = converter
            self.parameter_state = self.ps_start
            return

        kind = inspect.Parameter.KEYWORD_ONLY if self.keyword_only else inspect.Parameter.POSITIONAL_OR_KEYWORD
        p = Parameter(parameter_name, kind, function=self.function, converter=converter, default=value, group=self.group)
        self.function.parameters[parameter_name] = p

    def parse_converter(self, annotation):
        if isinstance(annotation, ast.Str):
            return annotation.s, True, {}

        if isinstance(annotation, ast.Name):
            return annotation.id, False, {}

        assert isinstance(annotation, ast.Call)

        name = annotation.func.id
        kwargs = {node.arg: ast.literal_eval(node.value) for node in annotation.keywords}
        return name, False, kwargs

    def parse_special_symbol(self, symbol):
        if self.parameter_state == self.ps_seen_slash:
            fail("Function " + self.function.name + " specifies " + symbol + " after /, which is unsupported.")

        if symbol == '*':
            if self.keyword_only:
                fail("Function " + self.function.name + " uses '*' more than once.")
            self.keyword_only = True
        elif symbol == '[':
            if self.parameter_state in (self.ps_start, self.ps_left_square_before):
                self.parameter_state = self.ps_left_square_before
            elif self.parameter_state in (self.ps_required, self.ps_group_after):
                self.parameter_state = self.ps_group_after
            else:
                fail("Function " + self.function.name + " has an unsupported group configuration. (Unexpected state " + str(self.parameter_state) + ")")
            self.group += 1
        elif symbol == ']':
            if not self.group:
                fail("Function " + self.function.name + " has a ] without a matching [.")
            if not any(p.group == self.group for p in self.function.parameters.values()):
                fail("Function " + self.function.name + " has an empty group.\nAll groups must contain at least one parameter.")
            self.group -= 1
            if self.parameter_state in (self.ps_left_square_before, self.ps_group_before):
                self.parameter_state = self.ps_group_before
            elif self.parameter_state in (self.ps_group_after, self.ps_right_square_after):
                self.parameter_state = self.ps_right_square_after
            else:
                fail("Function " + self.function.name + " has an unsupported group configuration. (Unexpected state " + str(self.parameter_state) + ")")
        elif symbol == '/':
            # ps_required is allowed here, that allows positional-only without option groups
            # to work (and have default values!)
            if (self.parameter_state not in (self.ps_required, self.ps_right_square_after, self.ps_group_before)) or self.group:
                fail("Function " + self.function.name + " has an unsupported group configuration. (Unexpected state " + str(self.parameter_state) + ")")
            if self.keyword_only:
                fail("Function " + self.function.name + " mixes keyword-only and positional-only parameters, which is unsupported.")
            self.parameter_state = self.ps_seen_slash
            # fixup preceeding parameters
            for p in self.function.parameters.values():
                if p.kind != inspect.Parameter.POSITIONAL_OR_KEYWORD:
                    fail("Function " + self.function.name + " mixes keyword-only and positional-only parameters, which is unsupported.")
                p.kind = inspect.Parameter.POSITIONAL_ONLY

    def state_parameter_docstring_start(self, line):
        self.parameter_docstring_indent = len(self.indent.margin)
        assert self.indent.depth == 3
        return self.next(self.state_parameter_docstring, line)

    # every line of the docstring must start with at least F spaces,
    # where F > P.
    # these F spaces will be stripped.
    def state_parameter_docstring(self, line):
        stripped = line.strip()
        if stripped.startswith('#'):
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

        assert self.function.parameters
        last_parameter = next(reversed(list(self.function.parameters.values())))

        new_docstring = last_parameter.docstring

        if new_docstring:
            new_docstring += '\n'
        if stripped:
            new_docstring += self.indent.dedent(line)

        last_parameter.docstring = new_docstring

    # the final stanza of the DSL is the docstring.
    def state_function_docstring(self, line):
        if not self.function.self_converter:
            self.function.self_converter = self_converter("self", self.function)

        if self.group:
            fail("Function " + self.function.name + " has a ] without a matching [.")

        stripped = line.strip()
        if stripped.startswith('#'):
            return

        new_docstring = self.function.docstring
        if new_docstring:
            new_docstring += "\n"
        if stripped:
            line = self.indent.dedent(line).rstrip()
        else:
            line = ''
        new_docstring += line
        self.function.docstring = new_docstring

    def format_docstring(self):
        f = self.function

        add, output = text_accumulator()
        parameters = list(f.parameters.values())

        ##
        ## docstring first line
        ##

        add(f.name)
        add('(')

        # populate "right_bracket_count" field for every parameter
        if parameters:
            # for now, the only way Clinic supports positional-only parameters
            # is if all of them are positional-only.
            positional_only_parameters = [p.kind == inspect.Parameter.POSITIONAL_ONLY for p in parameters]
            if parameters[0].kind == inspect.Parameter.POSITIONAL_ONLY:
                assert all(positional_only_parameters)
                for p in parameters:
                    p.right_bracket_count = abs(p.group)
            else:
                # don't put any right brackets around non-positional-only parameters, ever.
                for p in parameters:
                    p.right_bracket_count = 0

        right_bracket_count = 0

        def fix_right_bracket_count(desired):
            nonlocal right_bracket_count
            s = ''
            while right_bracket_count < desired:
                s += '['
                right_bracket_count += 1
            while right_bracket_count > desired:
                s += ']'
                right_bracket_count -= 1
            return s

        added_star = False
        add_comma = False

        for p in parameters:
            assert p.name

            if p.is_keyword_only() and not added_star:
                added_star = True
                if add_comma:
                    add(', ')
                add('*')

            a = [p.name]
            if p.converter.is_optional():
                a.append('=')
                value = p.converter.default
                a.append(p.converter.doc_default)
            s = fix_right_bracket_count(p.right_bracket_count)
            s += "".join(a)
            if add_comma:
                add(', ')
            add(s)
            add_comma = True

        add(fix_right_bracket_count(0))
        add(')')

        # if f.return_converter.doc_default:
        #     add(' -> ')
        #     add(f.return_converter.doc_default)

        docstring_first_line = output()

        # now fix up the places where the brackets look wrong
        docstring_first_line = docstring_first_line.replace(', ]', ',] ')

        # okay.  now we're officially building the "parameters" section.
        # create substitution text for {parameters}
        spacer_line = False
        for p in parameters:
            if not p.docstring.strip():
                continue
            if spacer_line:
                add('\n')
            else:
                spacer_line = True
            add("  ")
            add(p.name)
            add('\n')
            add(textwrap.indent(rstrip_lines(p.docstring.rstrip()), "    "))
        parameters = output()
        if parameters:
            parameters += '\n'

        ##
        ## docstring body
        ##

        docstring = f.docstring.rstrip()
        lines = [line.rstrip() for line in docstring.split('\n')]

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

        if len(lines) >= 2:
            if lines[1]:
                fail("Docstring for " + f.full_name + " does not have a summary line!\n" +
                    "Every non-blank function docstring must start with\n" +
                    "a single line summary followed by an empty line.")
        elif len(lines) == 1:
            # the docstring is only one line right now--the summary line.
            # add an empty line after the summary line so we have space
            # between it and the {parameters} we're about to add.
            lines.append('')

        parameters_marker_count = len(docstring.split('{parameters}')) - 1
        if parameters_marker_count > 1:
            fail('You may not specify {parameters} more than once in a docstring!')

        if not parameters_marker_count:
            # insert after summary line
            lines.insert(2, '{parameters}')

        # insert at front of docstring
        lines.insert(0, docstring_first_line)

        docstring = "\n".join(lines)

        add(docstring)
        docstring = output()

        docstring = linear_format(docstring, parameters=parameters)
        docstring = docstring.rstrip()

        return docstring

    def state_terminal(self, line):
        """
        Called when processing the block is done.
        """
        assert not line

        if not self.function:
            return

        if self.keyword_only:
            values = self.function.parameters.values()
            if not values:
                no_parameter_after_star = True
            else:
                last_parameter = next(reversed(list(values)))
                no_parameter_after_star = last_parameter.kind != inspect.Parameter.KEYWORD_ONLY
            if no_parameter_after_star:
                fail("Function " + self.function.name + " specifies '*' without any parameters afterwards.")

        # remove trailing whitespace from all parameter docstrings
        for name, value in self.function.parameters.items():
            if not value:
                continue
            value.docstring = value.docstring.rstrip()

        self.function.docstring = self.format_docstring()


# maps strings to callables.
# the callable should return an object
# that implements the clinic parser
# interface (__init__ and parse).
#
# example parsers:
#   "clinic", handles the Clinic DSL
#   "python", handles running Python code
#
parsers = {'clinic' : DSLParser, 'python': PythonParser}


clinic = None


def main(argv):
    import sys

    if sys.version_info.major < 3 or sys.version_info.minor < 3:
        sys.exit("Error: clinic.py requires Python 3.3 or greater.")

    import argparse
    cmdline = argparse.ArgumentParser()
    cmdline.add_argument("-f", "--force", action='store_true')
    cmdline.add_argument("-o", "--output", type=str)
    cmdline.add_argument("--converters", action='store_true')
    cmdline.add_argument("--make", action='store_true')
    cmdline.add_argument("filename", type=str, nargs="*")
    ns = cmdline.parse_args(argv)

    if ns.converters:
        if ns.filename:
            print("Usage error: can't specify --converters and a filename at the same time.")
            print()
            cmdline.print_usage()
            sys.exit(-1)
        converters = []
        return_converters = []
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
                    ids.append((name, name[:-len(suffix)]))
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
                            s = '{}={!r}'.format(parameter_name, parameter.default)
                        else:
                            s = parameter_name
                        parameters.append(s)
                print('    {}({})'.format(short_name, ', '.join(parameters)))
                # add_comma = False
                # for parameter_name, parameter in signature.parameters.items():
                #     if parameter.kind == inspect.Parameter.KEYWORD_ONLY:
                #         if add_comma:
                #             parameters.append(', ')
                #         else:
                #             add_comma = True
                #         s = parameter_name
                #         if parameter.default != inspect.Parameter.empty:
                #             s += '=' + repr(parameter.default)
                #         parameters.append(s)
                # parameters.append(')')

                # print("   ", short_name + "".join(parameters))
            print()
        print("All converters also accept (doc_default=None, required=False).")
        print("All return converters also accept (doc_default=None).")
        sys.exit(0)

    if ns.make:
        if ns.output or ns.filename:
            print("Usage error: can't use -o or filenames with --make.")
            print()
            cmdline.print_usage()
            sys.exit(-1)
        for root, dirs, files in os.walk('.'):
            for rcs_dir in ('.svn', '.git', '.hg'):
                if rcs_dir in dirs:
                    dirs.remove(rcs_dir)
            for filename in files:
                if not filename.endswith('.c'):
                    continue
                path = os.path.join(root, filename)
                parse_file(path, verify=not ns.force)
        return

    if not ns.filename:
        cmdline.print_usage()
        sys.exit(-1)

    if ns.output and len(ns.filename) > 1:
        print("Usage error: can't use -o with multiple filenames.")
        print()
        cmdline.print_usage()
        sys.exit(-1)

    for filename in ns.filename:
        parse_file(filename, output=ns.output, verify=not ns.force)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
