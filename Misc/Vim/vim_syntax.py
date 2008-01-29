from __future__ import with_statement
# XXX(nnorwitz): what versions of python is this file supposed to work with?
# It uses the old print statement not in py3k.

import keyword
import exceptions
import builtins
from string import Template
from sys import subversion

comment_header = '''" Auto-generated Vim syntax file for Python (%s: r%s).
"
" To use: copy or symlink to ~/.vim/syntax/python.vim'''

statement_header = """
if exists("b:current_syntax")
  finish
endif"""

statement_footer = '''
" Uncomment the 'minlines' statement line and comment out the 'maxlines'
" statement line; changes behaviour to look at least 2000 lines previously for
" syntax matches instead of at most 200 lines
syn sync match pythonSync grouphere NONE "):$"
syn sync maxlines=200
"syn sync minlines=2000

let b:current_syntax = "python"'''

looping = ('for', 'while')
conditionals = ('if', 'elif', 'else')
boolean_ops = ('and', 'in', 'is', 'not', 'or')
import_stmts = ('import', 'from')
object_defs = ('def', 'class')

exception_names = sorted(exc for exc in dir(exceptions)
                                if not exc.startswith('__'))

# Need to include functions that start with '__' (e.g., __import__), but
# nothing that comes with modules (e.g., __name__), so just exclude anything in
# the 'exceptions' module since we want to ignore exceptions *and* what any
# module would have
builtin_names = sorted(builtin for builtin in dir(builtins)
                            if builtin not in dir(exceptions))

escapes = (r'+\\[abfnrtv\'"\\]+', r'"\\\o\{1,3}"', r'"\\x\x\{2}"',
            r'"\(\\u\x\{4}\|\\U\x\{8}\)"', r'"\\$"')

todos = ("TODO", "FIXME", "XXX")

# XXX codify?
numbers = (r'"\<0x\x\+[Ll]\=\>"', r'"\<\d\+[LljJ]\=\>"',
            '"\.\d\+\([eE][+-]\=\d\+\)\=[jJ]\=\>"',
            '"\<\d\+\.\([eE][+-]\=\d\+\)\=[jJ]\=\>"',
            '"\<\d\+\.\d\+\([eE][+-]\=\d\+\)\=[jJ]\=\>"')

contained = lambda x: "%s contained" % x

def str_regexes():
    """Generator to yield various combinations of strings regexes"""
    regex_template = Template('matchgroup=Normal ' +
                                'start=+[uU]\=${raw}${sep}+ ' +
                                'end=+${sep}+ ' +
                                '${skip} ' +
                                '${contains}')
    skip_regex = Template(r'skip=+\\\\\|\\${sep}+')
    for raw in ('', '[rR]'):
        for separator in ("'", '"', '"""', "'''"):
            if len(separator) == 1:
                skip = skip_regex.substitute(sep=separator)
            else:
                skip = ''
            contains = 'contains=pythonEscape' if not raw else ''
            yield regex_template.substitute(raw=raw, sep=separator, skip=skip,
                                            contains = contains)

space_errors = (r'excludenl "\S\s\+$"ms=s+1', r'" \+\t"', r'"\t\+ "')

statements = (
                ('',
                    # XXX Might need to change pythonStatement since have
                    # specific Repeat, Conditional, Operator, etc. for 'while',
                    # etc.
                    [("Statement", "pythonStatement", "keyword",
                        (kw for kw in keyword.kwlist
                            if kw not in (looping + conditionals + boolean_ops +
                                        import_stmts + object_defs))
                      ),
                     ("Statement", "pythonStatement", "keyword",
                         (' '.join(object_defs) +
                             ' nextgroup=pythonFunction skipwhite')),
                     ("Function","pythonFunction", "match",
                         contained('"[a-zA-Z_][a-zA-Z0-9_]*"')),
                     ("Repeat", "pythonRepeat", "keyword", looping),
                     ("Conditional", "pythonConditional", "keyword",
                         conditionals),
                     ("Operator", "pythonOperator", "keyword", boolean_ops),
                     ("PreCondit", "pythonPreCondit", "keyword", import_stmts),
                     ("Comment", "pythonComment", "match",
                         '"#.*$" contains=pythonTodo'),
                     ("Todo", "pythonTodo", "keyword",
                         contained(' '.join(todos))),
                     ("String", "pythonString", "region", str_regexes()),
                     ("Special", "pythonEscape", "match",
                         (contained(esc) for esc in escapes
                             if not '$' in esc)),
                     ("Special", "pythonEscape", "match", r'"\\$"'),
                    ]
                ),
                ("python_highlight_numbers",
                    [("Number", "pythonNumber", "match", numbers)]
                ),
                ("python_highlight_builtins",
                    [("Function", "pythonBuiltin", "keyword", builtin_names)]
                ),
                ("python_highlight_exceptions",
                    [("Exception", "pythonException", "keyword",
                        exception_names)]
                ),
                ("python_highlight_space_errors",
                    [("Error", "pythonSpaceError", "match",
                        ("display " + err for err in space_errors))]
                )
             )

def syn_prefix(type_, kind):
    return 'syn %s %s    ' % (type_, kind)

def fill_stmt(iterable, fill_len):
    """Yield a string that fills at most fill_len characters with strings
    returned by 'iterable' and separated by a space"""
    # Deal with trailing char to handle ' '.join() calculation
    fill_len += 1
    overflow = None
    it = iter(iterable)
    while True:
        buffer_ = []
        total_len = 0
        if overflow:
            buffer_.append(overflow)
            total_len += len(overflow) + 1
            overflow = None
        while total_len < fill_len:
            try:
                new_item = next(it)
                buffer_.append(new_item)
                total_len += len(new_item) + 1
            except StopIteration:
                if buffer_:
                    break
                if overflow:
                    yield overflow
                return
        if total_len > fill_len:
            overflow = buffer_.pop()
            total_len -= len(overflow) - 1
        ret = ' '.join(buffer_)
        assert len(ret) <= fill_len
        yield ret

FILL = 80

def main(file_path):
    with open(file_path, 'w') as FILE:
        # Comment for file
        print>>FILE, comment_header % subversion[1:]
        print>>FILE, ''
        # Statements at start of file
        print>>FILE, statement_header
        print>>FILE, ''
        # Generate case for python_highlight_all
        print>>FILE, 'if exists("python_highlight_all")'
        for statement_var, statement_parts in statements:
            if statement_var:
                print>>FILE, '  let %s = 1' % statement_var
        else:
            print>>FILE, 'endif'
            print>>FILE, ''
        # Generate Python groups
        for statement_var, statement_parts in statements:
            if statement_var:
                print>>FILE, 'if exists("%s")' % statement_var
                indent = '  '
            else:
                indent = ''
            for colour_group, group, type_, arguments in statement_parts:
                if not isinstance(arguments, basestring):
                    prefix = syn_prefix(type_, group)
                    if type_ == 'keyword':
                        stmt_iter = fill_stmt(arguments,
                                            FILL - len(prefix) - len(indent))
                        try:
                            while True:
                                print>>FILE, indent + prefix + next(stmt_iter)
                        except StopIteration:
                            print>>FILE, ''
                    else:
                        for argument in arguments:
                            print>>FILE, indent + prefix + argument
                        else:
                            print>>FILE, ''

                else:
                    print>>FILE, indent + syn_prefix(type_, group) + arguments
                    print>>FILE, ''
            else:
                if statement_var:
                    print>>FILE, 'endif'
                    print>>FILE, ''
            print>>FILE, ''
        # Associating Python group with Vim colour group
        for statement_var, statement_parts in statements:
            if statement_var:
                print>>FILE, '  if exists("%s")' % statement_var
                indent = '    '
            else:
                indent = '  '
            for colour_group, group, type_, arguments in statement_parts:
                print>>FILE, (indent + "hi def link %s %s" %
                                (group, colour_group))
            else:
                if statement_var:
                    print>>FILE, '  endif'
                print>>FILE, ''
        # Statements at the end of the file
        print>>FILE, statement_footer

if __name__ == '__main__':
    main("python.vim")
