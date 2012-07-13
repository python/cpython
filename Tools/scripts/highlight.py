#!/usr/bin/env python3
'''Add syntax highlighting to Python source code'''

__author__ = 'Raymond Hettinger'

import keyword, tokenize, cgi, re, functools
try:
    import builtins
except ImportError:
    import __builtin__ as builtins

#### Analyze Python Source #################################

def is_builtin(s):
    'Return True if s is the name of a builtin'
    return hasattr(builtins, s)

def combine_range(lines, start, end):
    'Join content from a range of lines between start and end'
    (srow, scol), (erow, ecol) = start, end
    if srow == erow:
        return lines[srow-1][scol:ecol], end
    rows = [lines[srow-1][scol:]] + lines[srow: erow-1] + [lines[erow-1][:ecol]]
    return ''.join(rows), end

def analyze_python(source):
    '''Generate and classify chunks of Python for syntax highlighting.
       Yields tuples in the form: (category, categorized_text).
    '''
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
        elif tok_type == tokenize.OP and tok_str[:1] not in '{}[](),.:;@':
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
            text, written = combine_range(lines, written, (srow, scol))
            yield '', text
            text, written = combine_range(lines, written, (erow, ecol))
            yield kind, text
    line_upto_token, written = combine_range(lines, written, (erow, ecol))
    yield '', line_upto_token

#### Raw Output  ###########################################

def raw_highlight(classified_text):
    'Straight text display of text classifications'
    result = []
    for kind, text in classified_text:
        result.append('%15s:  %r\n' % (kind or 'plain', text))
    return ''.join(result)

#### ANSI Output ###########################################

default_ansi = {
    'comment': ('\033[0;31m', '\033[0m'),
    'string': ('\033[0;32m', '\033[0m'),
    'docstring': ('\033[0;32m', '\033[0m'),
    'keyword': ('\033[0;33m', '\033[0m'),
    'builtin': ('\033[0;35m', '\033[0m'),
    'definition': ('\033[0;33m', '\033[0m'),
    'defname': ('\033[0;34m', '\033[0m'),
    'operator': ('\033[0;33m', '\033[0m'),
}

def ansi_highlight(classified_text, colors=default_ansi):
    'Add syntax highlighting to source code using ANSI escape sequences'
    # http://en.wikipedia.org/wiki/ANSI_escape_code
    result = []
    for kind, text in classified_text:
        opener, closer = colors.get(kind, ('', ''))
        result += [opener, text, closer]
    return ''.join(result)

#### HTML Output ###########################################

def html_highlight(classified_text,opener='<pre class="python">\n', closer='</pre>\n'):
    'Convert classified text to an HTML fragment'
    result = [opener]
    for kind, text in classified_text:
        if kind:
            result.append('<span class="%s">' % kind)
        result.append(cgi.escape(text))
        if kind:
            result.append('</span>')
    result.append(closer)
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

def build_html_page(classified_text, title='python',
                    css=default_css, html=default_html):
    'Create a complete HTML page with colorized source code'
    css_str = '\n'.join(['%s %s' % item for item in css.items()])
    result = html_highlight(classified_text)
    title = cgi.escape(title)
    return html.format(title=title, css=css_str, body=result)

#### LaTeX Output ##########################################

default_latex_colors = {
    'comment': 'red',
    'string': 'green',
    'docstring': 'green',
    'keyword': 'orange',
    'builtin': 'purple',
    'definition': 'orange',
    'defname': 'blue',
    'operator': 'brown',
}

default_latex_document = r'''
\documentclass{article}
\usepackage{alltt}
\usepackage{color}
\usepackage[usenames,dvipsnames]{xcolor}
\usepackage[cm]{fullpage}
\begin{document}
\center{\LARGE{%(title)s}}
\begin{alltt}
%(body)s
\end{alltt}
\end{document}
'''

def latex_escape(s):
    'Replace LaTeX special characters with their escaped equivalents'
    # http://en.wikibooks.org/wiki/LaTeX/Basics#Special_Characters
    xlat = {
        '#': r'\#', '$': r'\$', '%': r'\%', '^': r'\textasciicircum{}',
        '&': r'\&', '_': r'\_', '{': r'\{', '}': r'\}', '~': r'\~{}',
        '\\': r'\textbackslash{}',
    }
    return re.sub(r'[\\#$%^&_{}~]', lambda mo: xlat[mo.group()], s)

def latex_highlight(classified_text, title = 'python',
                    colors = default_latex_colors,
                    document = default_latex_document):
    'Create a complete LaTeX document with colorized source code'
    result = []
    for kind, text in classified_text:
        if kind:
            result.append(r'{\color{%s}' % colors[kind])
        result.append(latex_escape(text))
        if kind:
            result.append('}')
    return default_latex_document % dict(title=title, body=''.join(result))


if __name__ == '__main__':
    import sys, argparse, webbrowser, os, textwrap

    parser = argparse.ArgumentParser(
            description = 'Add syntax highlighting to Python source code',
            formatter_class=argparse.RawDescriptionHelpFormatter,
            epilog = textwrap.dedent('''
                examples:

                  # Show syntax highlighted code in the terminal window
                  $ ./highlight.py myfile.py

                  # Colorize myfile.py and display in a browser
                  $ ./highlight.py -b myfile.py

                  # Create an HTML section to embed in an existing webpage
                  ./highlight.py -s myfile.py

                  # Create a complete HTML file
                  $ ./highlight.py -c myfile.py > myfile.html

                  # Create a PDF using LaTeX
                  $ ./highlight.py -l myfile.py | pdflatex

            '''))
    parser.add_argument('sourcefile', metavar = 'SOURCEFILE',
            help = 'file containing Python sourcecode')
    parser.add_argument('-b', '--browser', action = 'store_true',
            help = 'launch a browser to show results')
    parser.add_argument('-c', '--complete', action = 'store_true',
            help = 'build a complete html webpage')
    parser.add_argument('-l', '--latex', action = 'store_true',
            help = 'build a LaTeX document')
    parser.add_argument('-r', '--raw', action = 'store_true',
            help = 'raw parse of categorized text')
    parser.add_argument('-s', '--section', action = 'store_true',
            help = 'show an HTML section rather than a complete webpage')
    args = parser.parse_args()

    if args.section and (args.browser or args.complete):
        parser.error('The -s/--section option is incompatible with '
                     'the -b/--browser or -c/--complete options')

    sourcefile = args.sourcefile
    with open(sourcefile) as f:
        source = f.read()
    classified_text = analyze_python(source)

    if args.raw:
        encoded = raw_highlight(classified_text)
    elif args.complete or args.browser:
        encoded = build_html_page(classified_text, title=sourcefile)
    elif args.section:
        encoded = html_highlight(classified_text)
    elif args.latex:
        encoded = latex_highlight(classified_text, title=sourcefile)
    else:
        encoded = ansi_highlight(classified_text)

    if args.browser:
        htmlfile = os.path.splitext(os.path.basename(sourcefile))[0] + '.html'
        with open(htmlfile, 'w') as f:
            f.write(encoded)
        webbrowser.open('file://' + os.path.abspath(htmlfile))
    else:
        sys.stdout.write(encoded)
