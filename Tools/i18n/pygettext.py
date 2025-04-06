#! /usr/bin/env python3

"""pygettext -- Python equivalent of xgettext(1)

Many systems (Solaris, Linux, Gnu) provide extensive tools that ease the
internationalization of C programs. Most of these tools are independent of
the programming language and can be used from within Python programs.
Martin von Loewis' work[1] helps considerably in this regard.

pygettext uses Python's standard tokenize module to scan Python source
code, generating .pot files identical to what GNU xgettext[2] generates
for C and C++ code. From there, the standard GNU tools can be used.

A word about marking Python strings as candidates for translation. GNU
xgettext recognizes the following keywords: gettext, dgettext, dcgettext,
and gettext_noop. But those can be a lot of text to include all over your
code. C and C++ have a trick: they use the C preprocessor. Most
internationalized C source includes a #define for gettext() to _() so that
what has to be written in the source is much less. Thus these are both
translatable strings:

    gettext("Translatable String")
    _("Translatable String")

Python of course has no preprocessor so this doesn't work so well.  Thus,
pygettext searches only for _() by default, but see the -k/--keyword flag
below for how to augment this.

 [1] https://www.python.org/workshops/1997-10/proceedings/loewis.html
 [2] https://www.gnu.org/software/gettext/gettext.html

NOTE: pygettext attempts to be option and feature compatible with GNU
xgettext where ever possible. However some options are still missing or are
not fully implemented. Also, xgettext's use of command line switches with
option arguments is broken, and in these cases, pygettext just defines
additional switches.

NOTE: The public interface of pygettext is limited to the command-line
interface only. The internal API is subject to change without notice.

Usage: pygettext [options] inputfile ...

Options:

    -a
    --extract-all
        Deprecated: Not implemented and will be removed in a future version.

    -cTAG
    --add-comments=TAG
        Extract translator comments.  Comments must start with TAG and
        must precede the gettext call.  Multiple -cTAG options are allowed.
        In that case, any comment matching any of the TAGs will be extracted.

    -d name
    --default-domain=name
        Rename the default output file from messages.pot to name.pot.

    -E
    --escape
        Replace non-ASCII characters with octal escape sequences.

    -D
    --docstrings
        Extract module, class, method, and function docstrings.  These do
        not need to be wrapped in _() markers, and in fact cannot be for
        Python to consider them docstrings. (See also the -X option).

    -h
    --help
        Print this help message and exit.

    -k word
    --keyword=word
        Keywords to look for in addition to the default set, which are:
        _, gettext, ngettext, pgettext, npgettext, dgettext, dngettext,
        dpgettext, and dnpgettext.

        You can have multiple -k flags on the command line.

    -K
    --no-default-keywords
        Disable the default set of keywords (see above).  Any keywords
        explicitly added with the -k/--keyword option are still recognized.

    --no-location
        Do not write filename/lineno location comments.

    -n
    --add-location
        Write filename/lineno location comments indicating where each
        extracted string is found in the source.  These lines appear before
        each msgid.  The style of comments is controlled by the -S/--style
        option.  This is the default.

    -o filename
    --output=filename
        Rename the default output file from messages.pot to filename.  If
        filename is `-' then the output is sent to standard out.

    -p dir
    --output-dir=dir
        Output files will be placed in directory dir.

    -S stylename
    --style stylename
        Specify which style to use for location comments.  Two styles are
        supported:

        Solaris  # File: filename, line: line-number
        GNU      #: filename:line

        The style name is case insensitive.  GNU style is the default.

    -v
    --verbose
        Print the names of the files being processed.

    -V
    --version
        Print the version of pygettext and exit.

    -w columns
    --width=columns
        Set width of output to columns.

    -x filename
    --exclude-file=filename
        Specify a file that contains a list of strings that are not be
        extracted from the input files.  Each string to be excluded must
        appear on a line by itself in the file.

    -X filename
    --no-docstrings=filename
        Specify a file that contains a list of files (one per line) that
        should not have their docstrings extracted.  This is only useful in
        conjunction with the -D option above.

If `inputfile' is -, standard input is read.
"""

import ast
import getopt
import glob
import importlib.machinery
import importlib.util
import os
import sys
import time
import tokenize
from dataclasses import dataclass, field
from io import BytesIO
from operator import itemgetter

__version__ = '1.5'


# The normal pot-file header. msgmerge and Emacs's po-mode work better if it's
# there.
pot_header = '''\
# SOME DESCRIPTIVE TITLE.
# Copyright (C) YEAR ORGANIZATION
# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.
#
msgid ""
msgstr ""
"Project-Id-Version: PACKAGE VERSION\\n"
"POT-Creation-Date: %(time)s\\n"
"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n"
"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n"
"Language-Team: LANGUAGE <LL@li.org>\\n"
"MIME-Version: 1.0\\n"
"Content-Type: text/plain; charset=%(charset)s\\n"
"Content-Transfer-Encoding: %(encoding)s\\n"
"Generated-By: pygettext.py %(version)s\\n"

'''


def usage(code, msg=''):
    print(__doc__, file=sys.stderr)
    if msg:
        print(msg, file=sys.stderr)
    sys.exit(code)


def make_escapes(pass_nonascii):
    global escapes, escape
    if pass_nonascii:
        # Allow non-ascii characters to pass through so that e.g. 'msgid
        # "Höhe"' would not result in 'msgid "H\366he"'.  Otherwise we
        # escape any character outside the 32..126 range.
        escape = escape_ascii
    else:
        escape = escape_nonascii
    escapes = [r"\%03o" % i for i in range(256)]
    for i in range(32, 127):
        escapes[i] = chr(i)
    escapes[ord('\\')] = r'\\'
    escapes[ord('\t')] = r'\t'
    escapes[ord('\r')] = r'\r'
    escapes[ord('\n')] = r'\n'
    escapes[ord('\"')] = r'\"'


def escape_ascii(s, encoding):
    return ''.join(escapes[ord(c)] if ord(c) < 128 else c
                   if c.isprintable() else escape_nonascii(c, encoding)
                   for c in s)


def escape_nonascii(s, encoding):
    return ''.join(escapes[b] for b in s.encode(encoding))


def normalize(s, encoding):
    # This converts the various Python string types into a format that is
    # appropriate for .po files, namely much closer to C style.
    lines = s.split('\n')
    if len(lines) == 1:
        s = '"' + escape(s, encoding) + '"'
    else:
        if not lines[-1]:
            del lines[-1]
            lines[-1] = lines[-1] + '\n'
        for i in range(len(lines)):
            lines[i] = escape(lines[i], encoding)
        lineterm = '\\n"\n"'
        s = '""\n"' + lineterm.join(lines) + '"'
    return s


def containsAny(str, set):
    """Check whether 'str' contains ANY of the chars in 'set'"""
    return 1 in [c in str for c in set]


def getFilesForName(name):
    """Get a list of module files for a filename, a module or package name,
    or a directory.
    """
    if not os.path.exists(name):
        # check for glob chars
        if containsAny(name, "*?[]"):
            files = glob.glob(name)
            list = []
            for file in files:
                list.extend(getFilesForName(file))
            return list

        # try to find module or package
        try:
            spec = importlib.util.find_spec(name)
            name = spec.origin
        except ImportError:
            name = None
        if not name:
            return []

    if os.path.isdir(name):
        # find all python files in directory
        list = []
        # get extension for python source files
        _py_ext = importlib.machinery.SOURCE_SUFFIXES[0]
        for root, dirs, files in os.walk(name):
            # don't recurse into CVS directories
            if 'CVS' in dirs:
                dirs.remove('CVS')
            # add all *.py files to list
            list.extend(
                [os.path.join(root, file) for file in files
                 if os.path.splitext(file)[1] == _py_ext]
                )
        return list
    elif os.path.exists(name):
        # a single file
        return [name]

    return []


# Key is the function name, value is a dictionary mapping argument positions to the
# type of the argument. The type is one of 'msgid', 'msgid_plural', or 'msgctxt'.
DEFAULTKEYWORDS = {
    '_': {0: 'msgid'},
    'gettext': {0: 'msgid'},
    'ngettext': {0: 'msgid', 1: 'msgid_plural'},
    'pgettext': {0: 'msgctxt', 1: 'msgid'},
    'npgettext': {0: 'msgctxt', 1: 'msgid', 2: 'msgid_plural'},
    'dgettext': {1: 'msgid'},
    'dngettext': {1: 'msgid', 2: 'msgid_plural'},
    'dpgettext': {1: 'msgctxt', 2: 'msgid'},
    'dnpgettext': {1: 'msgctxt', 2: 'msgid', 3: 'msgid_plural'},
}


def parse_spec(spec):
    """Parse a keyword spec string into a dictionary.

    The keyword spec format defines the name of the gettext function and the
    positions of the arguments that correspond to msgid, msgid_plural, and
    msgctxt. The format is as follows:

        name - the name of the gettext function, assumed to
               have a single argument that is the msgid.
        name:pos1 - the name of the gettext function and the position
                    of the msgid argument.
        name:pos1,pos2 - the name of the gettext function and the positions
                         of the msgid and msgid_plural arguments.
        name:pos1,pos2c - the name of the gettext function and the positions
                          of the msgid and msgctxt arguments.
        name:pos1,pos2,pos3c - the name of the gettext function and the
                               positions of the msgid, msgid_plural, and
                               msgctxt arguments.

    As an example, the spec 'foo:1,2,3c' means that the function foo has three
    arguments, the first one is the msgid, the second one is the msgid_plural,
    and the third one is the msgctxt. The positions are 1-based.

    The msgctxt argument can appear in any position, but it can only appear
    once. For example, the keyword specs 'foo:3c,1,2' and 'foo:1,2,3c' are
    equivalent.

    See https://www.gnu.org/software/gettext/manual/gettext.html
    for more information.
    """
    parts = spec.strip().split(':', 1)
    if len(parts) == 1:
        name = parts[0]
        return name, {0: 'msgid'}

    name, args = parts
    if not args:
        raise ValueError(f'Invalid keyword spec {spec!r}: '
                         'missing argument positions')

    result = {}
    for arg in args.split(','):
        arg = arg.strip()
        is_context = False
        if arg.endswith('c'):
            is_context = True
            arg = arg[:-1]

        try:
            pos = int(arg) - 1
        except ValueError as e:
            raise ValueError(f'Invalid keyword spec {spec!r}: '
                             'position is not an integer') from e

        if pos < 0:
            raise ValueError(f'Invalid keyword spec {spec!r}: '
                             'argument positions must be strictly positive')

        if pos in result.values():
            raise ValueError(f'Invalid keyword spec {spec!r}: '
                             'duplicate positions')

        if is_context:
            if 'msgctxt' in result:
                raise ValueError(f'Invalid keyword spec {spec!r}: '
                                 'msgctxt can only appear once')
            result['msgctxt'] = pos
        elif 'msgid' not in result:
            result['msgid'] = pos
        elif 'msgid_plural' not in result:
            result['msgid_plural'] = pos
        else:
            raise ValueError(f'Invalid keyword spec {spec!r}: '
                             'too many positions')

    if 'msgid' not in result and 'msgctxt' in result:
        raise ValueError(f'Invalid keyword spec {spec!r}: '
                         'msgctxt cannot appear without msgid')

    return name, {v: k for k, v in result.items()}


@dataclass(frozen=True)
class Location:
    filename: str
    lineno: int

    def __lt__(self, other):
        return (self.filename, self.lineno) < (other.filename, other.lineno)


@dataclass
class Message:
    msgid: str
    msgid_plural: str | None
    msgctxt: str | None
    locations: set[Location] = field(default_factory=set)
    is_docstring: bool = False
    comments: list[str] = field(default_factory=list)

    def add_location(self, filename, lineno, msgid_plural=None, *,
                     is_docstring=False, comments=None):
        if self.msgid_plural is None:
            self.msgid_plural = msgid_plural
        self.locations.add(Location(filename, lineno))
        self.is_docstring |= is_docstring
        if comments:
            self.comments.extend(comments)


def get_source_comments(source):
    """
    Return a dictionary mapping line numbers to
    comments in the source code.
    """
    comments = {}
    for token in tokenize.tokenize(BytesIO(source).readline):
        if token.type == tokenize.COMMENT:
            # Remove any leading combination of '#' and whitespace
            comment = token.string.lstrip('# \t')
            comments[token.start[0]] = comment
    return comments


class GettextVisitor(ast.NodeVisitor):
    def __init__(self, options):
        super().__init__()
        self.options = options
        self.filename = None
        self.messages = {}
        self.comments = {}

    def visit_file(self, source, filename):
        try:
            module_tree = ast.parse(source)
        except SyntaxError:
            return

        self.filename = filename
        if self.options.comment_tags:
            self.comments = get_source_comments(source)
        self.visit(module_tree)

    def visit_Module(self, node):
        self._extract_docstring(node)
        self.generic_visit(node)

    visit_ClassDef = visit_FunctionDef = visit_AsyncFunctionDef = visit_Module

    def visit_Call(self, node):
        self._extract_message(node)
        self.generic_visit(node)

    def _extract_docstring(self, node):
        if (not self.options.docstrings or
            self.options.nodocstrings.get(self.filename)):
            return

        docstring = ast.get_docstring(node)
        if docstring is not None:
            lineno = node.body[0].lineno  # The first statement is the docstring
            self._add_message(lineno, docstring, is_docstring=True)

    def _extract_message(self, node):
        func_name = self._get_func_name(node)
        spec = self.options.keywords.get(func_name)
        if spec is None:
            return

        max_index = max(spec)
        has_var_positional = any(isinstance(arg, ast.Starred) for
                                 arg in node.args[:max_index+1])
        if has_var_positional:
            print(f'*** {self.filename}:{node.lineno}: Variable positional '
                  f'arguments are not allowed in gettext calls', file=sys.stderr)
            return

        if max_index >= len(node.args):
            print(f'*** {self.filename}:{node.lineno}: Expected at least '
                  f'{max(spec) + 1} positional argument(s) in gettext call, '
                  f'got {len(node.args)}', file=sys.stderr)
            return

        msg_data = {}
        for position, arg_type in spec.items():
            arg = node.args[position]
            if not self._is_string_const(arg):
                print(f'*** {self.filename}:{arg.lineno}: Expected a string '
                      f'constant for argument {position + 1}, '
                      f'got {ast.unparse(arg)}', file=sys.stderr)
                return
            msg_data[arg_type] = arg.value

        lineno = node.lineno
        comments = self._extract_comments(node)
        self._add_message(lineno, **msg_data, comments=comments)

    def _extract_comments(self, node):
        """Extract translator comments.

        Translator comments must precede the gettext call and
        start with one of the comment prefixes defined by
        --add-comments=TAG. See the tests for examples.
        """
        if not self.options.comment_tags:
            return []

        comments = []
        lineno = node.lineno - 1
        # Collect an unbroken sequence of comments starting from
        # the line above the gettext call.
        while lineno >= 1:
            comment = self.comments.get(lineno)
            if comment is None:
                break
            comments.append(comment)
            lineno -= 1

        # Find the first translator comment in the sequence and
        # return all comments starting from that comment.
        comments = comments[::-1]
        first_index = next((i for i, comment in enumerate(comments)
                            if self._is_translator_comment(comment)), None)
        if first_index is None:
            return []
        return comments[first_index:]

    def _is_translator_comment(self, comment):
        return comment.startswith(self.options.comment_tags)

    def _add_message(
            self, lineno, msgid, msgid_plural=None, msgctxt=None, *,
            is_docstring=False, comments=None):
        if msgid in self.options.toexclude:
            return

        if not comments:
            comments = []

        key = self._key_for(msgid, msgctxt)
        message = self.messages.get(key)
        if message:
            message.add_location(
                self.filename,
                lineno,
                msgid_plural,
                is_docstring=is_docstring,
                comments=comments,
            )
        else:
            self.messages[key] = Message(
                msgid=msgid,
                msgid_plural=msgid_plural,
                msgctxt=msgctxt,
                locations={Location(self.filename, lineno)},
                is_docstring=is_docstring,
                comments=comments,
            )

    @staticmethod
    def _key_for(msgid, msgctxt=None):
        if msgctxt is not None:
            return (msgctxt, msgid)
        return msgid

    def _get_func_name(self, node):
        match node.func:
            case ast.Name(id=id):
                return id
            case ast.Attribute(attr=attr):
                return attr
            case _:
                return None

    def _is_string_const(self, node):
        return isinstance(node, ast.Constant) and isinstance(node.value, str)

def write_pot_file(messages, options, fp):
    timestamp = time.strftime('%Y-%m-%d %H:%M%z')
    encoding = fp.encoding if fp.encoding else 'UTF-8'
    print(pot_header % {'time': timestamp, 'version': __version__,
                        'charset': encoding,
                        'encoding': '8bit'}, file=fp)

    # Sort locations within each message by filename and lineno
    sorted_keys = [
        (key, sorted(msg.locations))
        for key, msg in messages.items()
    ]
    # Sort messages by locations
    # For example, a message with locations [('test.py', 1), ('test.py', 2)] will
    # appear before a message with locations [('test.py', 1), ('test.py', 3)]
    sorted_keys.sort(key=itemgetter(1))

    for key, locations in sorted_keys:
        msg = messages[key]

        for comment in msg.comments:
            print(f'#. {comment}', file=fp)

        if options.writelocations:
            # location comments are different b/w Solaris and GNU:
            if options.locationstyle == options.SOLARIS:
                for location in locations:
                    print(f'# File: {location.filename}, line: {location.lineno}', file=fp)
            elif options.locationstyle == options.GNU:
                # fit as many locations on one line, as long as the
                # resulting line length doesn't exceed 'options.width'
                locline = '#:'
                for location in locations:
                    s = f' {location.filename}:{location.lineno}'
                    if len(locline) + len(s) <= options.width:
                        locline = locline + s
                    else:
                        print(locline, file=fp)
                        locline = f'#:{s}'
                if len(locline) > 2:
                    print(locline, file=fp)
        if msg.is_docstring:
            # If the entry was gleaned out of a docstring, then add a
            # comment stating so.  This is to aid translators who may wish
            # to skip translating some unimportant docstrings.
            print('#, docstring', file=fp)
        if msg.msgctxt is not None:
            print('msgctxt', normalize(msg.msgctxt, encoding), file=fp)
        print('msgid', normalize(msg.msgid, encoding), file=fp)
        if msg.msgid_plural is not None:
            print('msgid_plural', normalize(msg.msgid_plural, encoding), file=fp)
            print('msgstr[0] ""', file=fp)
            print('msgstr[1] ""\n', file=fp)
        else:
            print('msgstr ""\n', file=fp)


def main():
    try:
        opts, args = getopt.getopt(
            sys.argv[1:],
            'ac::d:DEhk:Kno:p:S:Vvw:x:X:',
            ['extract-all', 'add-comments=?', 'default-domain=', 'escape',
             'help', 'keyword=', 'no-default-keywords',
             'add-location', 'no-location', 'output=', 'output-dir=',
             'style=', 'verbose', 'version', 'width=', 'exclude-file=',
             'docstrings', 'no-docstrings',
             ])
    except getopt.error as msg:
        usage(1, msg)

    # for holding option values
    class Options:
        # constants
        GNU = 1
        SOLARIS = 2
        # defaults
        extractall = 0 # FIXME: currently this option has no effect at all.
        escape = 0
        keywords = []
        outpath = ''
        outfile = 'messages.pot'
        writelocations = 1
        locationstyle = GNU
        verbose = 0
        width = 78
        excludefilename = ''
        docstrings = 0
        nodocstrings = {}
        comment_tags = set()

    options = Options()
    locations = {'gnu' : options.GNU,
                 'solaris' : options.SOLARIS,
                 }
    no_default_keywords = False
    # parse options
    for opt, arg in opts:
        if opt in ('-h', '--help'):
            usage(0)
        elif opt in ('-a', '--extract-all'):
            print("DepreciationWarning: -a/--extract-all is not implemented and will be removed in a future version",
                  file=sys.stderr)
            options.extractall = 1
        elif opt in ('-c', '--add-comments'):
            options.comment_tags.add(arg)
        elif opt in ('-d', '--default-domain'):
            options.outfile = arg + '.pot'
        elif opt in ('-E', '--escape'):
            options.escape = 1
        elif opt in ('-D', '--docstrings'):
            options.docstrings = 1
        elif opt in ('-k', '--keyword'):
            options.keywords.append(arg)
        elif opt in ('-K', '--no-default-keywords'):
            no_default_keywords = True
        elif opt in ('-n', '--add-location'):
            options.writelocations = 1
        elif opt in ('--no-location',):
            options.writelocations = 0
        elif opt in ('-S', '--style'):
            options.locationstyle = locations.get(arg.lower())
            if options.locationstyle is None:
                usage(1, f'Invalid value for --style: {arg}')
        elif opt in ('-o', '--output'):
            options.outfile = arg
        elif opt in ('-p', '--output-dir'):
            options.outpath = arg
        elif opt in ('-v', '--verbose'):
            options.verbose = 1
        elif opt in ('-V', '--version'):
            print(f'pygettext.py (xgettext for Python) {__version__}')
            sys.exit(0)
        elif opt in ('-w', '--width'):
            try:
                options.width = int(arg)
            except ValueError:
                usage(1, f'--width argument must be an integer: {arg}')
        elif opt in ('-x', '--exclude-file'):
            options.excludefilename = arg
        elif opt in ('-X', '--no-docstrings'):
            fp = open(arg)
            try:
                while 1:
                    line = fp.readline()
                    if not line:
                        break
                    options.nodocstrings[line[:-1]] = 1
            finally:
                fp.close()

    options.comment_tags = tuple(options.comment_tags)

    # calculate escapes
    make_escapes(not options.escape)

    # calculate all keywords
    try:
        custom_keywords = dict(parse_spec(spec) for spec in options.keywords)
    except ValueError as e:
        print(e, file=sys.stderr)
        sys.exit(1)
    options.keywords = {}
    if not no_default_keywords:
        options.keywords |= DEFAULTKEYWORDS
    # custom keywords override default keywords
    options.keywords |= custom_keywords

    # initialize list of strings to exclude
    if options.excludefilename:
        try:
            with open(options.excludefilename) as fp:
                options.toexclude = fp.readlines()
        except IOError:
            print(f"Can't read --exclude-file: {options.excludefilename}",
                  file=sys.stderr)
            sys.exit(1)
    else:
        options.toexclude = []

    # resolve args to module lists
    expanded = []
    for arg in args:
        if arg == '-':
            expanded.append(arg)
        else:
            expanded.extend(getFilesForName(arg))
    args = expanded

    # slurp through all the files
    visitor = GettextVisitor(options)
    for filename in args:
        if filename == '-':
            if options.verbose:
                print('Reading standard input')
            source = sys.stdin.buffer.read()
        else:
            if options.verbose:
                print(f'Working on {filename}')
            with open(filename, 'rb') as fp:
                source = fp.read()

        visitor.visit_file(source, filename)

    # write the output
    if options.outfile == '-':
        fp = sys.stdout
        closep = 0
    else:
        if options.outpath:
            options.outfile = os.path.join(options.outpath, options.outfile)
        fp = open(options.outfile, 'w')
        closep = 1
    try:
        write_pot_file(visitor.messages, options, fp)
    finally:
        if closep:
            fp.close()


if __name__ == '__main__':
    main()
