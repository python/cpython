"""Handle exceptions in CGI scripts by formatting tracebacks into nice HTML.

To enable this module, do:

    import cgitb; cgitb.enable()

at the top of your CGI script.  The optional arguments to enable() are:

    display     - if true, tracebacks are displayed in the web browser
    logdir      - if set, tracebacks are written to files in this directory
    context     - number of lines of source code to show for each stack frame

By default, tracebacks are displayed but not saved, and context is 5.

Alternatively, if you have caught an exception and want cgitb to display it
for you, call cgitb.handler().  The optional argument to handler() is a 3-item
tuple (etype, evalue, etb) just like the value of sys.exc_info()."""

__author__ = 'Ka-Ping Yee'
__version__ = '$Revision$'

import sys

def reset():
    """Return a string that resets the CGI and browser to a known state."""
    return '''<!--: spam
Content-Type: text/html

<body bgcolor="#f0f0f8"><font color="#f0f0f8" size="-5"> -->
<body bgcolor="#f0f0f8"><font color="#f0f0f8" size="-5"> --> -->
</font> </font> </font> </script> </object> </blockquote> </pre>
</table> </table> </table> </table> </table> </font> </font> </font>'''

__UNDEF__ = []                          # a special sentinel object
def small(text): return '<small>' + text + '</small>'
def strong(text): return '<strong>' + text + '</strong>'
def grey(text): return '<font color="#909090">' + text + '</font>'

def lookup(name, frame, locals):
    """Find the value for a given name in the given environment."""
    if name in locals:
        return 'local', locals[name]
    if name in frame.f_globals:
        return 'global', frame.f_globals[name]
    return None, __UNDEF__

def scanvars(reader, frame, locals):
    """Scan one logical line of Python and look up values of variables used."""
    import tokenize, keyword
    vars, lasttoken, parent, prefix = [], None, None, ''
    for ttype, token, start, end, line in tokenize.generate_tokens(reader):
        if ttype == tokenize.NEWLINE: break
        if ttype == tokenize.NAME and token not in keyword.kwlist:
            if lasttoken == '.':
                if parent is not __UNDEF__:
                    value = getattr(parent, token, __UNDEF__)
                    vars.append((prefix + token, prefix, value))
            else:
                where, value = lookup(token, frame, locals)
                vars.append((token, where, value))
        elif token == '.':
            prefix += lasttoken + '.'
            parent = value
        else:
            parent, prefix = None, ''
        lasttoken = token
    return vars

def html((etype, evalue, etb), context=5):
    """Return a nice HTML document describing a given traceback."""
    import os, types, time, traceback, linecache, inspect, pydoc

    if type(etype) is types.ClassType:
        etype = etype.__name__
    pyver = 'Python ' + sys.version.split()[0] + ': ' + sys.executable
    date = time.ctime(time.time())
    head = '<body bgcolor="#f0f0f8">' + pydoc.html.heading(
        '<big><big><strong>%s</strong></big></big>' % str(etype),
        '#ffffff', '#6622aa', pyver + '<br>' + date) + '''
<p>A problem occurred in a Python script.  Here is the sequence of
function calls leading up to the error, in the order they occurred.'''

    indent = '<tt>' + small('&nbsp;' * 5) + '&nbsp;</tt>'
    frames = []
    records = inspect.getinnerframes(etb, context)
    for frame, file, lnum, func, lines, index in records:
        file = file and os.path.abspath(file) or '?'
        link = '<a href="file://%s">%s</a>' % (file, pydoc.html.escape(file))
        args, varargs, varkw, locals = inspect.getargvalues(frame)
        call = ''
        if func != '?':
            call = 'in ' + strong(func) + \
                inspect.formatargvalues(args, varargs, varkw, locals,
                    formatvalue=lambda value: '=' + pydoc.html.repr(value))

        highlight = {}
        def reader(lnum=[lnum]):
            highlight[lnum[0]] = 1
            try: return linecache.getline(file, lnum[0])
            finally: lnum[0] += 1
        vars = scanvars(reader, frame, locals)

        rows = ['<tr><td bgcolor="#d8bbff">%s%s %s</td></tr>' %
                ('<big>&nbsp;</big>', link, call)]
        if index is not None:
            i = lnum - index
            for line in lines:
                num = small('&nbsp;' * (5-len(str(i))) + str(i)) + '&nbsp;'
                line = '<tt>%s%s</tt>' % (num, pydoc.html.preformat(line))
                if i in highlight:
                    rows.append('<tr><td bgcolor="#ffccee">%s</td></tr>' % line)
                else:
                    rows.append('<tr><td>%s</td></tr>' % grey(line))
                i += 1

        done, dump = {}, []
        for name, where, value in vars:
            if name in done: continue
            done[name] = 1
            if value is not __UNDEF__:
                if where == 'global': name = '<em>global</em> ' + strong(name)
                elif where == 'local': name = strong(name)
                else: name = where + strong(name.split('.')[-1])
                dump.append('%s&nbsp;= %s' % (name, pydoc.html.repr(value)))
            else:
                dump.append(name + ' <em>undefined</em>')

        rows.append('<tr><td>%s</td></tr>' % small(grey(', '.join(dump))))
        frames.append('''<p>
<table width="100%%" cellspacing=0 cellpadding=0 border=0>
%s</table>''' % '\n'.join(rows))

    exception = ['<p>%s: %s' % (strong(str(etype)), str(evalue))]
    if type(evalue) is types.InstanceType:
        for name in dir(evalue):
            value = pydoc.html.repr(getattr(evalue, name))
            exception.append('\n<br>%s%s&nbsp;=\n%s' % (indent, name, value))

    import traceback
    return head + ''.join(frames) + ''.join(exception) + '''


<!-- The above is a description of an error in a Python program, formatted
     for a Web browser because the 'cgitb' module was enabled.  In case you
     are not reading this in a Web browser, here is the original traceback:

%s
-->
''' % ''.join(traceback.format_exception(etype, evalue, etb))

class Hook:
    """A hook to replace sys.excepthook that shows tracebacks in HTML."""

    def __init__(self, display=1, logdir=None, context=5, file=None):
        self.display = display          # send tracebacks to browser if true
        self.logdir = logdir            # log tracebacks to files if not None
        self.context = context          # number of source code lines per frame
        self.file = file or sys.stdout  # place to send the output

    def __call__(self, etype, evalue, etb):
        self.handle((etype, evalue, etb))

    def handle(self, info=None):
        info = info or sys.exc_info()
        self.file.write(reset())

        try:
            text, doc = 0, html(info, self.context)
        except:                         # just in case something goes wrong
            import traceback
            text, doc = 1, ''.join(traceback.format_exception(*info))

        if self.display:
            if text:
                doc = doc.replace('&', '&amp;').replace('<', '&lt;')
                self.file.write('<pre>' + doc + '</pre>\n')
            else:
                self.file.write(doc + '\n')
        else:
            self.file.write('<p>A problem occurred in a Python script.\n')

        if self.logdir is not None:
            import os, tempfile
            name = tempfile.mktemp(['.html', '.txt'][text])
            path = os.path.join(self.logdir, os.path.basename(name))
            try:
                file = open(path, 'w')
                file.write(doc)
                file.close()
                msg = '<p> %s contains the description of this error.' % path
            except:
                msg = '<p> Tried to save traceback to %s, but failed.' % path
            self.file.write(msg + '\n')
        try:
            self.file.flush()
        except: pass

handler = Hook().handle
def enable(display=1, logdir=None, context=5):
    """Install an exception handler that formats tracebacks as HTML.

    The optional argument 'display' can be set to 0 to suppress sending the
    traceback to the browser, and 'logdir' can be set to a directory to cause
    tracebacks to be written to files there."""
    sys.excepthook = Hook(display, logdir, context)
