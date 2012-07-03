#!/usr/bin/env python3
'Add syntax highlighting to Python source code'

__all__ = ['colorize', 'build_page', 'default_css', 'default_html',
           'ansi_colorize', 'default_ansi']
__author__ = 'Raymond Hettinger'

import keyword, tokenize, cgi, functools

def is_builtin(s):
    'Return True if s is the name of a builtin'
    return s in vars(__builtins__)

def combine_range(lines, start, end):
    'Join content from a range of lines between start and end'
    (srow, scol), (erow, ecol) = start, end
    if srow == erow:
        rows = [lines[srow-1][scol:ecol]]
    else:
        rows = [lines[srow-1][scol:]] + lines[srow: erow-1] + [lines[erow-1][:ecol]]
    return ''.join(rows), end

def isolate_tokens(source):
    'Generate chunks of source and identify chunks to be highlighted'
    lines = source.splitlines(True)
    lines.append('')
    readline = functools.partial(next, iter(lines), '')
    kind = tok_str = ''
    tok_type = tokenize.COMMENT
    written = (1, 0)
    for tok in tokenize.generate_tokens(readline):
        prev_tok_type, prev_tok_str = tok_type, tok_str
        tok_type, tok_str, (srow, scol), (erow, ecol), logical_lineno = tok
        kind = ''
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
        line_upto_token, written = combine_range(lines, written, (srow, scol))
        line_thru_token, written = combine_range(lines, written, (erow, ecol))
        yield kind, line_upto_token, line_thru_token

default_ansi = {
    'comment': '\033[0;31m',
    'string': '\033[0;32m',
    'docstring': '\033[0;32m',
    'keyword': '\033[0;33m',
    'builtin': '\033[0;35m',
    'definition': '\033[0;33m',
    'defname': '\033[0;34m',
    'operator': '\033[0;33m',
}

def colorize_ansi(source, colors=default_ansi):
    'Add syntax highlighting to Python source code using ANSI escape sequences'
    # http://en.wikipedia.org/wiki/ANSI_escape_code
    result = []
    for kind, line_upto_token, line_thru_token in isolate_tokens(source):
        if kind:
            result += [line_upto_token, colors[kind], line_thru_token, '\033[0m']
        else:
            result += [line_upto_token, line_thru_token]
    return ''.join(result)

def colorize_html(source):
    'Convert Python source code to an HTML fragment with colorized markup'
    result = ['<pre class="python">\n']
    for kind, line_upto_token, line_thru_token in isolate_tokens(source):
        if kind:
            result += [cgi.escape(line_upto_token),
                       '<span class="%s">' % kind,
                       cgi.escape(line_thru_token),
                       '</span>']
        else:
            result += [cgi.escape(line_upto_token),
                       cgi.escape(line_thru_token)]
    result += ['</pre>\n']
    return ''.join(result)

default_css = {
    '.comment': '{color: crimson;}',
    '.string':  '{color: forestgreen;}',
    '.docstring': '{color: forestgreen; font-style:italic;}',
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
<title> {title} </title>
<style type="text/css">
{css}
</style>
</head>
<body>
{body}
</body>
</html>
'''

def build_page(source, title='python', css=default_css, html=default_html):
    'Create a complete HTML page with colorized Python source code'
    css_str = '\n'.join(['%s %s' % item for item in css.items()])
    result = colorize_html(source)
    title = cgi.escape(title)
    return html.format(title=title, css=css_str, body=result)


if __name__ == '__main__':
    import sys, argparse, webbrowser, os

    parser = argparse.ArgumentParser(
            description = 'Add syntax highlighting to Python source')
    parser.add_argument('sourcefile', metavar = 'SOURCEFILE',
            help = 'File containing Python sourcecode')
    parser.add_argument('-a', '--ansi', action = 'store_true',
            help = 'emit ANSI escape highlighted source')
    parser.add_argument('-b', '--browser', action = 'store_true',
            help = 'launch a browser to show results')
    parser.add_argument('-s', '--section', action = 'store_true',
            help = 'show an HTML section rather than a complete webpage')
    args = parser.parse_args()

    if args.browser and args.section:
        parser.error('The -s/--section option is incompatible with '
                     'the -b/--browser option')
    if args.ansi and (args.browser or args.section):
        parser.error('The -a/--ansi option is incompatible with '
                     'the -b/--browser and -s/--section options')

    sourcefile = args.sourcefile
    with open(sourcefile) as f:
        page = f.read()

    if args.ansi:
        encoded = colorize_ansi(page)
    elif args.section:
        encoded = colorize_html(page)
    else:
        encoded = build_page(page, title=sourcefile)

    if args.browser:
        htmlfile = os.path.splitext(os.path.basename(sourcefile))[0] + '.html'
        with open(htmlfile, 'w') as f:
            f.write(encoded)
        webbrowser.open('file://' + os.path.abspath(htmlfile))
    else:
        sys.stdout.write(encoded)
