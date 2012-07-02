#!/usr/bin/env python3
'Convert Python source code to HTML with colorized markup'

__all__ = ['colorize', 'build_page', 'default_css', 'default_html']
__author__ = 'Raymond Hettinger'

import keyword, tokenize, cgi, functools

def is_builtin(s):
    'Return True if s is the name of a builtin'
    return s in vars(__builtins__)

def escape_range(lines, start, end):
    'Return escaped content from a range of lines between start and end'
    (srow, scol), (erow, ecol) = start, end
    if srow == erow:
        rows = [lines[srow-1][scol:ecol]]
    else:
        rows = [lines[srow-1][scol:]] + lines[srow: erow-1] + [lines[erow-1][:ecol]]
    return cgi.escape(''.join(rows)), end

def colorize(source):
    'Convert Python source code to an HTML fragment with colorized markup'
    lines = source.splitlines(True)
    lines.append('')
    readline = functools.partial(next, iter(lines), '')
    actions = []
    kind = tok_str = ''
    tok_type = tokenize.COMMENT
    written = (1, 0)
    result = []
    for tok in tokenize.generate_tokens(readline):
        prev_tok_type, prev_tok_str = tok_type, tok_str
        tok_type, tok_str, (srow, scol), (erow, ecol), logical_lineno = tok
        kind, prev_kind = '', kind
        if tok_type == tokenize.COMMENT:
            kind = 'comment'
        elif tok_type == tokenize.OP and tok_str[:1] not in '{}[](),.:;':
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
            line_upto_token, written = escape_range(lines, written, (srow, scol))
            line_thru_token, written = escape_range(lines, written, (erow, ecol))
            result += [line_upto_token, '<span class="%s">' % kind,
                       line_thru_token, '</span>']
        else:
            line_thru_token, written = escape_range(lines, written, (erow, ecol))
            result += [line_thru_token]

    result.insert(0, '<pre class="python">\n')
    result.append('</pre>\n')
    return ''.join(result)

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
<title> %s </title>
<style type="text/css">
%s
</style>
</head>
<body>
%s
</body>
</html>
'''

def build_page(source, title='python', css=default_css, html=default_html):
    'Create a complete HTML page with colorized Python source code'
    css_str = '\n'.join(['%s %s' % item for item in css.items()])
    result = colorize(source)
    title = cgi.escape(title)
    return html % (title, css_str, result)


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
    html = colorize(page) if args.standalone else build_page(page, title=sourcefile)
    if args.browser:
        htmlfile = os.path.splitext(os.path.basename(sourcefile))[0] + '.html'
        with open(htmlfile, 'w') as f:
            f.write(html)
        webbrowser.open('file://' + os.path.abspath(htmlfile))
    else:
        sys.stdout.write(html)
