#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Check for stylistic and formal issues in .rst and .py
# files included in the documentation.
#
# 01/2009, Georg Brandl

# TODO: - wrong versions in versionadded/changed
#       - wrong markup after versionchanged directive

from __future__ import with_statement

import os
import re
import sys
import getopt
import subprocess
from os.path import join, splitext, abspath, exists
from collections import defaultdict

directives = [
    # standard docutils ones
    'admonition', 'attention', 'caution', 'class', 'compound', 'container',
    'contents', 'csv-table', 'danger', 'date', 'default-role', 'epigraph',
    'error', 'figure', 'footer', 'header', 'highlights', 'hint', 'image',
    'important', 'include', 'line-block', 'list-table', 'meta', 'note',
    'parsed-literal', 'pull-quote', 'raw', 'replace',
    'restructuredtext-test-directive', 'role', 'rubric', 'sectnum', 'sidebar',
    'table', 'target-notes', 'tip', 'title', 'topic', 'unicode', 'warning',
    # Sphinx custom ones
    'acks', 'attribute', 'autoattribute', 'autoclass', 'autodata',
    'autoexception', 'autofunction', 'automethod', 'automodule', 'centered',
    'cfunction', 'class', 'classmethod', 'cmacro', 'cmdoption', 'cmember',
    'code-block', 'confval', 'cssclass', 'ctype', 'currentmodule', 'cvar',
    'data', 'deprecated', 'describe', 'directive', 'doctest', 'envvar', 'event',
    'exception', 'function', 'glossary', 'highlight', 'highlightlang', 'index',
    'literalinclude', 'method', 'module', 'moduleauthor', 'productionlist',
    'program', 'role', 'sectionauthor', 'seealso', 'sourcecode', 'staticmethod',
    'tabularcolumns', 'testcode', 'testoutput', 'testsetup', 'toctree', 'todo',
    'todolist', 'versionadded', 'versionchanged'
]

all_directives = '(' + '|'.join(directives) + ')'
seems_directive_re = re.compile(r'\.\. %s([^a-z:]|:(?!:))' % all_directives)
default_role_re = re.compile(r'(^| )`\w([^`]*?\w)?`($| )')
leaked_markup_re = re.compile(r'[a-z]::[^=]|:[a-z]+:|`|\.\.\s*\w+:')


checkers = {}

checker_props = {'severity': 1, 'falsepositives': False}

def checker(*suffixes, **kwds):
    """Decorator to register a function as a checker."""
    def deco(func):
        for suffix in suffixes:
            checkers.setdefault(suffix, []).append(func)
        for prop in checker_props:
            setattr(func, prop, kwds.get(prop, checker_props[prop]))
        return func
    return deco


@checker('.py', severity=4)
def check_syntax(fn, lines):
    """Check Python examples for valid syntax."""
    code = ''.join(lines)
    if '\r' in code:
        if os.name != 'nt':
            yield 0, '\\r in code file'
        code = code.replace('\r', '')
    try:
        compile(code, fn, 'exec')
    except SyntaxError as err:
        yield err.lineno, 'not compilable: %s' % err


@checker('.rst', severity=2)
def check_suspicious_constructs(fn, lines):
    """Check for suspicious reST constructs."""
    inprod = False
    for lno, line in enumerate(lines):
        if seems_directive_re.match(line):
            yield lno+1, 'comment seems to be intended as a directive'
        if '.. productionlist::' in line:
            inprod = True
        elif not inprod and default_role_re.search(line):
            yield lno+1, 'default role used'
        elif inprod and not line.strip():
            inprod = False


@checker('.py', '.rst')
def check_whitespace(fn, lines):
    """Check for whitespace and line length issues."""
    for lno, line in enumerate(lines):
        if '\r' in line:
            yield lno+1, '\\r in line'
        if '\t' in line:
            yield lno+1, 'OMG TABS!!!1'
        if line[:-1].rstrip(' \t') != line[:-1]:
            yield lno+1, 'trailing whitespace'


@checker('.rst', severity=0)
def check_line_length(fn, lines):
    """Check for line length; this checker is not run by default."""
    for lno, line in enumerate(lines):
        if len(line) > 81:
            # don't complain about tables, links and function signatures
            if line.lstrip()[0] not in '+|' and \
               'http://' not in line and \
               not line.lstrip().startswith(('.. function',
                                             '.. method',
                                             '.. cfunction')):
                yield lno+1, "line too long"


@checker('.html', severity=2, falsepositives=True)
def check_leaked_markup(fn, lines):
    """Check HTML files for leaked reST markup; this only works if
    the HTML files have been built.
    """
    for lno, line in enumerate(lines):
        if leaked_markup_re.search(line):
            yield lno+1, 'possibly leaked markup: %r' % line


def main(argv):
    usage = '''\
Usage: %s [-v] [-f] [-s sev] [-i path]* [path]

Options:  -v       verbose (print all checked file names)
          -f       enable checkers that yield many false positives
          -s sev   only show problems with severity >= sev
          -i path  ignore subdir or file path
''' % argv[0]
    try:
        gopts, args = getopt.getopt(argv[1:], 'vfs:i:')
    except getopt.GetoptError:
        print(usage)
        return 2

    verbose = False
    severity = 1
    ignore = []
    falsepos = False
    for opt, val in gopts:
        if opt == '-v':
            verbose = True
        elif opt == '-f':
            falsepos = True
        elif opt == '-s':
            severity = int(val)
        elif opt == '-i':
            ignore.append(abspath(val))

    if len(args) == 0:
        path = '.'
    elif len(args) == 1:
        path = args[0]
    else:
        print(usage)
        return 2

    if not exists(path):
        print('Error: path %s does not exist' % path)
        return 2

    count = defaultdict(int)

    for root, dirs, files in os.walk(path):
        # ignore subdirs controlled by svn
        if '.svn' in dirs:
            dirs.remove('.svn')

        # ignore subdirs in ignore list
        if abspath(root) in ignore:
            del dirs[:]
            continue

        for fn in files:
            fn = join(root, fn)
            if fn[:2] == './':
                fn = fn[2:]

            # ignore files in ignore list
            if abspath(fn) in ignore:
                continue

            ext = splitext(fn)[1]
            checkerlist = checkers.get(ext, None)
            if not checkerlist:
                continue

            if verbose:
                print('Checking %s...' % fn)

            try:
                with open(fn, 'r') as f:
                    lines = list(f)
            except (IOError, OSError) as err:
                print('%s: cannot open: %s' % (fn, err))
                count[4] += 1
                continue

            for checker in checkerlist:
                if checker.falsepositives and not falsepos:
                    continue
                csev = checker.severity
                if csev >= severity:
                    for lno, msg in checker(fn, lines):
                        print('[%d] %s:%d: %s' % (csev, fn, lno, msg))
                        count[csev] += 1
    if verbose:
        print()
    if not count:
        if severity > 1:
            print('No problems with severity >= %d found.' % severity)
        else:
            print('No problems found.')
    else:
        for severity in sorted(count):
            number = count[severity]
            print('%d problem%s with severity %d found.' %
                  (number, number > 1 and 's' or '', severity))
    return int(bool(count))


if __name__ == '__main__':
    sys.exit(main(sys.argv))
