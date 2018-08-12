#!/usr/bin/env python3
from __future__ import (unicode_literals, absolute_import,
                        print_function, division)
import sys

if sys.version_info.major == 2:
    reload(sys)
    sys.setdefaultencoding('utf-8')

try:
    from signal import signal, SIGPIPE, SIG_DFL
    signal(SIGPIPE,SIG_DFL)
except ImportError:  # no SIGPIPE on Windows
    pass

import argparse
import json
import re
import os
from collections import Iterable

try:
    if 'PYTHONPY_CONFIG' in os.environ:
        exec(open(os.environ['PYTHONPY_CONFIG']).read())
    else:
        exec(open('%s/.pythonpy.py' % os.environ['HOME']).read())
except:
    pass
        
try:
    from . import __version__
except (ImportError, ValueError, SystemError):
    __version__ = '???'  # NOQA
__version_info__ = '''Pythonpy %s
Python %s''' % (__version__, sys.version.split(' ')[0])


def import_matches(query, prefix=''):
    matches = set(re.findall(r"(%s[a-zA-Z_][a-zA-Z0-9_]*)\.?" % prefix, query))
    for raw_module_name in matches:
        if re.match('np(\..*)?$', raw_module_name):
            module_name = re.sub('^np', 'numpy', raw_module_name)
        elif re.match('pd(\..*)?$', raw_module_name):
            module_name = re.sub('^pd', 'pandas', raw_module_name)
        else:
            module_name = raw_module_name
        try:
            module = __import__(module_name)
            globals()[raw_module_name] = module
            import_matches(query, prefix='%s.' % module_name)
        except ImportError as e:
            pass


def lazy_imports(*args):
    query = ' '.join([x for x in args if x])
    import_matches(query)


def current_list(input):
    return re.split(r'[^a-zA-Z0-9_\.]', input)


def inspect_source(obj):
    import inspect
    import pydoc
    try:
        pydoc.pager(''.join(inspect.getsourcelines(obj)[0]))
        return None
    except:
        return help(obj)

parser = argparse.ArgumentParser(
            formatter_class=argparse.RawDescriptionHelpFormatter,
            add_help=False)

group = parser.add_argument_group("Options")

parser.add_argument('expression', nargs='?', default='None', help="e.g. py '2 ** 32'")
group.add_argument('-x', dest='lines_of_stdin', action='store_const',
                    const=True, default=False,
                    help='treat each row of stdin as x')
group.add_argument('-l', dest='list_of_stdin', action='store_const',
                    const=True, default=False,
                    help='treat list of stdin as l')
group.add_argument('-j',
                    dest='json_of_stdin', action='store_const',
                    const=True, default=False,
                    help='treat parsed json of stdin as j')
group.add_argument('--csv', dest='csv_delimiter',
                    help='parse stdin as a csv (run with -x or -l)')
group.add_argument('--ji',
                    dest='json_line_input', action='store_const',
                    const=True, default=False,
                    help='preprocess each row of input x with json.loads(x)')
group.add_argument('--jo',
                    dest='json_line_output', action='store_const',
                    const=True, default=False,
                    help='postprocess each row of output x with json.dumps(x)')
group.add_argument('--i',
                    dest='ignore_exceptions', action='store_const',
                    const=True, default=False,
                    help='skip output for rows that trigger exceptions')
group.add_argument('--si', dest='input_delimiter',
                    help='preprocess each row of input x as x.split(delimiter)')
group.add_argument('--so', dest='out_delimiter',
                    help='postprocess each row of output x as delimiter.join(x)')
group.add_argument('-c', dest='pre_cmd', help='run code before expression')
group.add_argument('-C', dest='post_cmd', help='run code after expression')
group.add_argument('-V', '--version', action='version', version=__version_info__, help='version info')
group.add_argument('-h', '--help', action='help', help="show this help message and exit")

try:
    args = parser.parse_args()
    if sum([args.list_of_stdin, args.lines_of_stdin, args.json_of_stdin]) > 1:
        sys.stderr.write('Pythonpy accepts at most one of [-x, -l, -j] flags\n')
        sys.exit(1)

    if args.json_line_input:
        def loads(str_):
            try:
                return json.loads(str_.rstrip())
            except Exception as ex:
                if args.ignore_exceptions:
                    pass
                else:
                    raise ex
        stdin = (loads(x) for x in sys.stdin)
    elif args.csv_delimiter:
        import csv
        stdin = csv.DictReader(sys.stdin, delimiter=args.csv_delimiter)
    elif args.input_delimiter:
        stdin = (x.rstrip().split(args.input_delimiter) for x in sys.stdin)
    else:
        stdin = (x.rstrip() for x in sys.stdin)

    if args.expression:
        args.expression = args.expression.replace("`", "'")
        if args.expression.startswith('?') or args.expression.endswith('?'):
            final_atom = current_list(args.expression.rstrip('?'))[-1]
            first_atom = current_list(args.expression.lstrip('?'))[0]
            if args.expression.startswith('??'):
                import inspect
                args.expression = "inspect_source(%s)" % first_atom
            elif args.expression.endswith('??'):
                import inspect
                args.expression = "inspect_source(%s)" % final_atom
            elif args.expression.startswith('?'):
                args.expression = 'help(%s)' % first_atom
            else:
                args.expression = 'help(%s)' % final_atom
            if args.lines_of_stdin:
                from itertools import islice
                stdin = islice(stdin,1)
    if args.pre_cmd:
        args.pre_cmd = args.pre_cmd.replace("`", "'")
    if args.post_cmd:
        args.post_cmd = args.post_cmd.replace("`", "'")

    lazy_imports(args.expression, args.pre_cmd, args.post_cmd)

    if args.pre_cmd:
        exec(args.pre_cmd)

    def safe_eval(text, x):
        try:
            return eval(text)
        except:
            return None

    if args.lines_of_stdin:
        if args.ignore_exceptions:
            result = (safe_eval(args.expression, x) for x in stdin)
        else:
            result = (eval(args.expression) for x in stdin)
    elif args.list_of_stdin:
        l = list(stdin)
        result = eval(args.expression)
    elif args.json_of_stdin:
        j = json.load(sys.stdin)
        result = eval(args.expression)
    else:
        result = eval(args.expression)

    def format(output):
        if output is None:
            return None
        elif args.json_line_output:
            return json.dumps(output)
        elif args.out_delimiter:
            return args.out_delimiter.join(output)
        elif str(type(output)) == "<class 'collections.OrderedDict'>":
            return dict(output)
        else:
            return output


    is_iterable = (isinstance(result, Iterable) and
                   hasattr(result, '__iter__') and not
                   isinstance(result, str) or
                   isinstance(result, enumerate))
    if isinstance(result, dict):
        for k in result:
            k_formatted = format(k)
            v_formatted = format(result[k])
            print('%s: %s' % (k_formatted, v_formatted))
    elif is_iterable:
        for x in result:
            formatted = format(x)
            if formatted is not None:
                try:
                    print(formatted)
                except UnicodeEncodeError:
                    print(formatted.encode('utf-8'))
    else:
        formatted = format(result)
        if formatted is not None:
            try:
                print(formatted)
            except UnicodeEncodeError:
                print(formatted.encode('utf-8'))

    if args.post_cmd:
        exec(args.post_cmd)
except Exception as ex:
    import traceback
    pyheader = 'pythonpy/__main__.py'
    exprheader = 'File "<string>"'
    foundexpr = False
    lines = traceback.format_exception(*sys.exc_info())
    for line in lines:
        if pyheader in line:
            continue
        sys.stderr.write(line)
        if not foundexpr and line.lstrip().startswith(exprheader) and not isinstance(ex, SyntaxError):
            sys.stderr.write('    {}\n'.format(args.expression))
            foundexpr = True
    
    sys.exit(1)

def main():
    pass
