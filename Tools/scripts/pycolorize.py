#!/usr/bin/env python3
'Convert Python source code to HTML with colorized markup'

__all__ = ['colorize', 'build_page', 'default_css', 'default_html']
__author__  = 'Raymond Hettinger'

import keyword, tokenize, cgi, functools

def insert(s, i, text):
    'Insert text at position i in string s'
    return s[:i] + text + s[i:]

def is_builtin(s):
    'Return True if s is the name of a builtin'
    return s in vars(__builtins__)

def colorize(source):
    'Convert Python source code to an HTML fragment with colorized markup'
    text = cgi.escape(source)
    lines = text.splitlines(True)
    readline = functools.partial(next, iter(lines), '')
    actions = []
    kind = tok_str = ''
    tok_type = tokenize.COMMENT
    for tok in tokenize.generate_tokens(readline):
        prev_tok_type, prev_tok_str = tok_type, tok_str
        tok_type, tok_str, (srow, scol), (erow, ecol), logical_lineno = tok
        kind, prev_kind = '', kind
        if tok_type == tokenize.COMMENT:
            kind = 'comment'
        elif tok_type == tokenize.OP:
            kind = 'operator'
        elif tok_type == tokenize.STRING:
            kind = 'string'
            if prev_tok_type == tokenize.INDENT or scol==0:
                kind = 'docstring'
        elif tok_type == tokenize.NAME:
            if tok_str in ('def', 'class', 'import', 'from'):
                kind = 'definition'
            elif prev_tok_str in ('def', 'class'):
                kind = 'defname'
            elif keyword.iskeyword(tok_str):
                kind = 'keyword'
            elif is_builtin(tok_str) and prev_tok_str != '.':
                kind = 'builtin'
        if kind:
            actions.append(((srow, scol), (erow, ecol), kind))

    for (srow, scol), (erow, ecol), kind in reversed(actions):
        lines[erow-1] = insert(lines[erow-1], ecol, '</span>')
        lines[srow-1] = insert(lines[srow-1], scol, '<span class="%s">' % kind)

    lines.insert(0, '<pre class="python">\n')
    lines.append('</pre>\n')
    return ''.join(lines)

default_css = {
    '.comment': '{color: crimson;}',
    '.string':  '{color: forestgreen;}',
    '.docstring': '{color: forestgreen; font-style:italic}',
    '.keyword': '{color: darkorange;}',
    '.builtin': '{color: purple;}',
    '.definition': '{color: darkorange; font-weight:bold;}',
    '.defname': '{color: blue;}',
    '.operator': '{color: brown;}',
}

default_html = '''\
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN"
          "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
<meta http-equiv="Content-type" content="text/html;charset=UTF-8">
<title> Python Code </title>
<style type="text/css">
%s
</style>
</head>
<body>
%s
</body>
</html>
'''

def build_page(source, html=default_html, css=default_css):
    'Create a complete HTML page with colorized Python source code'
    css_str = '\n'.join(['%s %s' % item for item in css.items()])
    result = colorize(source)
    return html % (css_str, result)


if __name__ == '__main__':
    import sys, argparse, webbrowser, os

    parser = argparse.ArgumentParser(
        description = 'Convert Python source code to colorized HTML')
    parser.add_argument('sourcefile', metavar = 'SOURCEFILE', nargs = 1,
            help = 'File containing Python sourcecode')
    parser.add_argument('-b', '--browser', action = 'store_true',
            help = 'launch a browser to show results')
    parser.add_argument('-s', '--standalone', action = 'store_true',
            help = 'show a standalone snippet rather than a complete webpage')
    args = parser.parse_args()
    if args.browser and args.standalone:
        parser.error('The -s/--standalone option is incompatible with '
                     'the -b/--browser option')

    sourcefile = args.sourcefile[0]
    with open(sourcefile) as f:
        page = f.read()
    html = colorize(page) if args.standalone else build_page(page)
    if args.browser:
        htmlfile = os.path.splitext(os.path.basename(sourcefile))[0] + '.html'
        with open(htmlfile, 'w') as f:
            f.write(html)
        webbrowser.open('file://' + os.path.abspath(htmlfile))
    else:
        sys.stdout.write(html)
