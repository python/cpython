"""Handle exceptions in CGI scripts by formatting tracebacks into nice HTML.

To enable this module, do:

    import cgitb; cgitb.enable()

at the top of your CGI script.  The optional arguments to enable() are:

    display     - if true, tracebacks are displayed in the web browser
    logdir      - if set, tracebacks are written to files in this directory

By default, tracebacks are displayed but not written to files.

Alternatively, if you have caught an exception and want cgitb to display it
for you, call cgitb.handle().  The optional argument to handle() is a 3-item
tuple (etype, evalue, etb) just like the value of sys.exc_info()."""

__author__ = 'Ka-Ping Yee'
__version__ = '$Revision$'

def reset():
    """Return a string that resets the CGI and browser to a known state."""
    return '''<!--: spam
Content-Type: text/html

<body bgcolor="#f0f0ff"><font color="#f0f0ff" size="-5"> -->
<body bgcolor="#f0f0ff"><font color="#f0f0ff" size="-5"> --> -->
</font> </font> </font> </script> </object> </blockquote> </pre>
</table> </table> </table> </table> </table> </font> </font> </font>'''

def html(etype, evalue, etb, context=5):
    """Return a nice HTML document describing the traceback."""
    import sys, os, types, time, traceback
    import keyword, tokenize, linecache, inspect, pydoc

    if type(etype) is types.ClassType:
        etype = etype.__name__
    pyver = 'Python ' + sys.version.split()[0] + ': ' + sys.executable
    date = time.ctime(time.time())
    head = '<body bgcolor="#f0f0ff">' + pydoc.html.heading(
        '<big><big><strong>%s</strong></big></big>' % str(etype),
        '#ffffff', '#aa55cc', pyver + '<br>' + date) + '''
<p>A problem occurred in a Python script.
Here is the sequence of function calls leading up to
the error, with the most recent (innermost) call last.'''

    indent = '<tt><small>' + '&nbsp;' * 5 + '</small>&nbsp;</tt>'
    frames = []
    records = inspect.getinnerframes(etb, context)
    for frame, file, lnum, func, lines, index in records:
        file = file and os.path.abspath(file) or '?'
        link = '<a href="file:%s">%s</a>' % (file, pydoc.html.escape(file))
        args, varargs, varkw, locals = inspect.getargvalues(frame)
        if func == '?':
            call = ''
        else:
            def eqrepr(value): return '=' + pydoc.html.repr(value)
            call = 'in <strong>%s</strong>' % func + inspect.formatargvalues(
                    args, varargs, varkw, locals, formatvalue=eqrepr)

        names = []
        def tokeneater(type, token, start, end, line):
            if type == tokenize.NAME and token not in keyword.kwlist:
                if token not in names: names.append(token)
            if type == tokenize.NEWLINE: raise IndexError
        def linereader(lnum=[lnum]):
            line = linecache.getline(file, lnum[0])
            lnum[0] += 1
            return line

        try:
            tokenize.tokenize(linereader, tokeneater)
        except IndexError: pass
        lvals = []
        for name in names:
            if name in frame.f_code.co_varnames:
                if locals.has_key(name):
                    value = pydoc.html.repr(locals[name])
                else:
                    value = '<em>undefined</em>'
                name = '<strong>%s</strong>' % name
            else:
                if frame.f_globals.has_key(name):
                    value = pydoc.html.repr(frame.f_globals[name])
                else:
                    value = '<em>undefined</em>'
                name = '<em>global</em> <strong>%s</strong>' % name
            lvals.append('%s&nbsp;= %s' % (name, value))
        if lvals:
            lvals = indent + '''
<small><font color="#909090">%s</font></small><br>''' % (', '.join(lvals))
        else:
            lvals = ''

        level = '''
<table width="100%%" bgcolor="#d8bbff" cellspacing=0 cellpadding=2 border=0>
<tr><td>%s %s</td></tr></table>\n''' % (link, call)
        excerpt = []
        if index is not None:
            i = lnum - index
            for line in lines:
                num = '<small><font color="#909090">%s</font></small>' % (
                    '&nbsp;' * (5-len(str(i))) + str(i))
                line = '<tt>%s&nbsp;%s</tt>' % (num, pydoc.html.preformat(line))
                if i == lnum:
                    line = '''
<table width="100%%" bgcolor="#ffccee" cellspacing=0 cellpadding=0 border=0>
<tr><td>%s</td></tr></table>\n''' % line
                excerpt.append('\n' + line)
                if i == lnum:
                    excerpt.append(lvals)
                i = i + 1
        frames.append('<p>' + level + '\n'.join(excerpt))

    exception = ['<p><strong>%s</strong>: %s' % (str(etype), str(evalue))]
    if type(evalue) is types.InstanceType:
        for name in dir(evalue):
            value = pydoc.html.repr(getattr(evalue, name))
            exception.append('\n<br>%s%s&nbsp;=\n%s' % (indent, name, value))

    import traceback
    plaintrace = ''.join(traceback.format_exception(etype, evalue, etb))

    return head + ''.join(frames) + ''.join(exception) + '''


<!-- The above is a description of an error that occurred in a Python program.
     It is formatted for display in a Web browser because it appears that
     we are running in a CGI environment.  In case you are viewing this
     message outside of a Web browser, here is the original error traceback:

%s
-->
''' % plaintrace

class Hook:
    def __init__(self, display=1, logdir=None):
        self.display = display          # send tracebacks to browser if true
        self.logdir = logdir            # log tracebacks to files if not None

    def __call__(self, etype, evalue, etb):
        """This hook can replace sys.excepthook (for Python 2.1 or higher)."""
        self.handle((etype, evalue, etb))

    def handle(self, info=None):
        import sys, os
        info = info or sys.exc_info()
        text = 0
        print reset()

        try:
            doc = html(*info)
        except:                         # just in case something goes wrong
            import traceback
            doc = ''.join(traceback.format_exception(*info))
            text = 1

        if self.display:
            if text:
                doc = doc.replace('&', '&amp;').replace('<', '&lt;')
                print '<pre>', doc, '</pre>'
            else:
                print doc
        else:
            print '<p>A problem occurred in a Python script.'

        if self.logdir is not None:
            import tempfile
            name = tempfile.mktemp(['.html', '.txt'][text])
            path = os.path.join(self.logdir, os.path.basename(name))
            try:
                file = open(path, 'w')
                file.write(doc)
                file.close()
                print '<p>%s contains the description of this error.' % path
            except:
                print '<p>Tried to write to %s, but failed.' % path

handler = Hook().handle
def enable(display=1, logdir=None):
    import sys
    sys.excepthook = Hook(display, logdir)
