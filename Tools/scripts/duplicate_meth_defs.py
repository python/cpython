#!/usr/bin/env python3
"""Print duplicate method definitions in the same scope."""

import sys
import os
import ast
import re
import argparse
import traceback
from tokenize import open as tokenize_open
from contextlib import contextmanager

class ClassFunctionVisitor(ast.NodeVisitor):
    def __init__(self, duplicates):
        self.duplicates = duplicates
        self.scopes = []

    @contextmanager
    def _visit_new_scope(self, node):
        prev_scope = self.scopes[-1] if len(self.scopes) else None
        new_scope = Scope(node, prev_scope)
        try:
            yield prev_scope, new_scope
        finally:
            self.scopes.append(new_scope)
            self.generic_visit(node)
            self.scopes.pop()

    def visit_FunctionDef(self, node):
        with self._visit_new_scope(node) as (previous, new):
            if previous is not None and previous.type == 'ClassDef':
                new.check_duplicate_meth(self.duplicates)

    def visit_ClassDef(self, node):
        with self._visit_new_scope(node) as (previous, new):
            pass

    visit_AsyncFunctionDef = visit_ClassDef

class Scope:
    def __init__(self, node, previous):
        self.node = node
        self.previous = previous
        self.type = node.__class__.__name__
        self.methods = []
        if previous is None:
            self.id = [node.name]
        else:
            self.id = previous.id + [node.name]

    def check_duplicate_meth(self, duplicates):
        node = self.node
        name = node.name
        # Skip property and generic methods.
        if 'decorator_list' in node._fields and node.decorator_list:
            for decorator in node.decorator_list:
                if (isinstance(decorator, ast.Attribute) and
                        'attr' in decorator._fields and
                        decorator.attr in
                        ('setter', 'getter', 'deleter', 'register')):
                    return
                # A functools single-dispatch method.
                elif (isinstance(decorator, ast.Call) and
                        'func' in decorator._fields and
                        'attr' in decorator.func._fields and
                        decorator.func.attr == 'register'):
                    return

        if name in self.previous.methods:
            duplicates.append((self.id, node.lineno))
        else:
            self.previous.methods.append(name)

def get_duplicates(source, fname):
    duplicates = []
    tree = ast.parse(source, fname)
    ClassFunctionVisitor(duplicates).visit(tree)
    return duplicates

def duplicates_exist(source, fname='<unknown>', dups_to_ignore=[]):
    """Print duplicates and return True if duplicates exist.

    >>> test_basic = '''
    ... class C:
    ...     def foo(self): pass
    ...     def foo(self): pass
    ... '''
    >>> duplicates_exist(test_basic)
    <unknown>:4 C.foo
    True

    >>> test_ignore = '''
    ... class C:
    ...     def foo(self): pass
    ...     def foo(self): pass
    ... '''
    >>> duplicates_exist(test_ignore, dups_to_ignore=['C.foo'])
    False

    >>> test_nested = '''
    ... def foo():
    ...     def bar(): pass
    ...
    ...     class C:
    ...         def foo(self): pass
    ...         def bar(self): pass
    ...         def bar(self): pass
    ...
    ...         class D:
    ...             def foo(self): pass
    ...             def bar(self): pass
    ...             def bar(self): pass
    ...
    ...         def bar(self): pass
    ...
    ...     def bar(): pass
    ... '''
    >>> duplicates_exist(test_nested)
    <unknown>:8 foo.C.bar
    <unknown>:13 foo.C.D.bar
    <unknown>:15 foo.C.bar
    True

    >>> test_generic = '''
    ... class A:
    ...     @functools.singledispatchmethod
    ...     def t(self, arg):
    ...         self.arg = "base"
    ...     @t.register(int)
    ...     def _(self, arg):
    ...         self.arg = "int"
    ...     @t.register(str)
    ...     def _(self, arg):
    ...         self.arg = "str"
    ...
    ... class C:
    ...     @property
    ...     def foo(self): pass
    ...
    ...     @foo.setter
    ...     def foo(self, a, b): pass
    ...
    ...     def bar(self): pass
    ...     def bar(self): pass
    ...
    ...     class D:
    ...         @property
    ...         def foo(self): pass
    ...
    ...         @foo.setter
    ...         def foo(self, a, b): pass
    ...
    ...         def bar(self): pass
    ...         def bar(self): pass
    ...         def foo(self): pass
    ... '''
    >>> duplicates_exist(test_generic)
    <unknown>:21 C.bar
    <unknown>:31 C.D.bar
    <unknown>:32 C.D.foo
    True
    """

    cnt = 0
    duplicates = get_duplicates(source, fname)
    if duplicates:
        for (id, lineno) in duplicates:
            id = '.'.join(id)
            if dups_to_ignore is None or id not in dups_to_ignore:
                print('%s:%d %s' % (fname, lineno, id))
                cnt += 1
    return False if cnt == 0 else True

def iter_modules(paths):
    for f in paths:
        if os.path.isdir(f):
            for path, dirs, filenames in os.walk(f):
                for fname in filenames:
                    if os.path.splitext(fname)[1] == '.py':
                        yield os.path.normpath(os.path.join(path, fname))
        else:
            yield os.path.normpath(f)

def ignored_dups(f):
    """Parse the ignore duplicates file."""

    # Comments or empty lines.
    comments = re.compile(r'^\s*#.*$|^\s*$')

    # Mapping of filename to the list of duplicates to ignore.
    ignored = {}
    if f is None:
        return ignored
    with f:
        for item in (l.rstrip(': \n\t').split(':') for l in f
                if not comments.match(l)):
            path = item[0].strip()
            if not path:
                continue
            path = os.path.normpath(path)
            if len(item) == 1:
                ignored[path] = []
            else:
                dupname = item[1].strip()
                if path in ignored:
                    ignored[path].append(dupname)
                else:
                    ignored[path] = [dupname]
    return ignored

def parse_args(argv):
    parser = argparse.ArgumentParser(description=__doc__.strip())
    parser.add_argument('files', metavar='path', nargs='*',
            help='python module or directory pathname - when a directory, '
            'the python modules in the directory tree are searched '
            'recursively for duplicates.')
    parser.add_argument('-i', '--ignore', metavar='fname', type=open,
            help='ignore duplicates or files listed in <fname>, see'
            ' Tools/scripts/duplicates_ignored.txt for the'
            ' specification of the format of the entries in this file')
    parser.add_argument('-x', '--exclude', metavar='prefixes',
            help="comma separated list of path names prefixes to exclude"
            " (default: '%(default)s')")
    parser.add_argument('-t', '--test', action='store_true',
            help='run the doctests')
    return parser.parse_args(argv)

def _main(args):
    ignored = ignored_dups(args.ignore)
    if args.exclude:
        prefixes = [p.strip() for p in args.exclude.split(',')]
    else:
        prefixes = []

    # Find duplicates.
    rc = 0
    for fname in iter_modules(args.files):
        def excluded(f):
            for p in prefixes:
                if f.startswith(p):
                    return True

        # Skip files whose prefix is excluded or that are configured in the
        # '--ignore' file.
        if excluded(fname):
            continue
        dups_to_ignore = ignored.get(fname)
        if dups_to_ignore == []:
            continue

        try:
            with tokenize_open(fname) as f:
                if duplicates_exist(f.read(), fname, dups_to_ignore):
                    rc = 1
        except (UnicodeDecodeError, SyntaxError) as e:
            print('%s: %s' % (fname, e), file=sys.stderr)
            traceback.print_tb(sys.exc_info()[2])
            rc = 1

    return rc

def main(argv):
    args = parse_args(argv)
    if args.test:
        import doctest
        doctest.testmod()
    else:
        return _main(args)

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
