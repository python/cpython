#!/usr/bin/env python
"""Generate Python documentation in HTML or text for interactive use.

In the Python interpreter, do "from pydoc import help" to provide online
help.  Calling help(thing) on a Python object documents the object.

Or, at the shell command line outside of Python:

Run "pydoc <name>" to show documentation on something.  <name> may be
the name of a function, module, package, or a dotted reference to a
class or function within a module or module in a package.  If the
argument contains a path segment delimiter (e.g. slash on Unix,
backslash on Windows) it is treated as the path to a Python source file.

Run "pydoc -k <keyword>" to search for a keyword in the synopsis lines
of all available modules.

Run "pydoc -p <port>" to start an HTTP server on a given port on the
local machine to generate documentation web pages.

For platforms without a command line, "pydoc -g" starts the HTTP server
and also pops up a little window for controlling it.

Run "pydoc -w <name>" to write out the HTML documentation for a module
to a file named "<name>.html".
"""

__author__ = "Ka-Ping Yee <ping@lfw.org>"
__date__ = "26 February 2001"
__version__ = "$Revision$"
__credits__ = """Guido van Rossum, for an excellent programming language.
Tommy Burnette, the original creator of manpy.
Paul Prescod, for all his work on onlinehelp.
Richard Chamberlain, for the first implementation of textdoc.

Mynd you, møøse bites Kan be pretty nasti..."""

# Note: this module is designed to deploy instantly and run under any
# version of Python from 1.5 and up.  That's why it's a single file and
# some 2.0 features (like string methods) are conspicuously avoided.

import sys, imp, os, stat, re, types, inspect
from repr import Repr
from string import expandtabs, find, join, lower, split, strip, rfind, rstrip

# --------------------------------------------------------- common routines

def synopsis(filename, cache={}):
    """Get the one-line summary out of a module file."""
    mtime = os.stat(filename)[stat.ST_MTIME]
    lastupdate, result = cache.get(filename, (0, None))
    # XXX what if ext is 'rb' type in imp_getsuffixes?
    if lastupdate < mtime:
        file = open(filename)
        line = file.readline()
        while line[:1] == '#' or strip(line) == '':
            line = file.readline()
            if not line: break
        if line[-2:] == '\\\n':
            line = line[:-2] + file.readline()
        line = strip(line)
        if line[:3] == '"""':
            line = line[3:]
            while strip(line) == '':
                line = file.readline()
                if not line: break
            result = strip(split(line, '"""')[0])
        else: result = None
        file.close()
        cache[filename] = (mtime, result)
    return result

def pathdirs():
    """Convert sys.path into a list of absolute, existing, unique paths."""
    dirs = []
    normdirs = []
    for dir in sys.path:
        dir = os.path.abspath(dir or '.')
        normdir = os.path.normcase(dir)
        if normdir not in normdirs and os.path.isdir(dir):
            dirs.append(dir)
            normdirs.append(normdir)
    return dirs

def getdoc(object):
    """Get the doc string or comments for an object."""
    result = inspect.getdoc(object) or inspect.getcomments(object)
    return result and re.sub('^ *\n', '', rstrip(result)) or ''

def classname(object, modname):
    """Get a class name and qualify it with a module name if necessary."""
    name = object.__name__
    if object.__module__ != modname:
        name = object.__module__ + '.' + name
    return name

def isconstant(object):
    """Check if an object is of a type that probably means it's a constant."""
    return type(object) in [
        types.FloatType, types.IntType, types.ListType, types.LongType,
        types.StringType, types.TupleType, types.TypeType,
        hasattr(types, 'UnicodeType') and types.UnicodeType or 0]

def replace(text, *pairs):
    """Do a series of global replacements on a string."""
    for old, new in pairs:
        text = join(split(text, old), new)
    return text

def cram(text, maxlen):
    """Omit part of a string if needed to make it fit in a maximum length."""
    if len(text) > maxlen:
        pre = max(0, (maxlen-3)/2)
        post = max(0, maxlen-3-pre)
        return text[:pre] + '...' + text[len(text)-post:]
    return text

def stripid(text):
    """Remove the hexadecimal id from a Python object representation."""
    # The behaviour of %p is implementation-dependent, so we need an example.
    for pattern in [' at 0x[0-9a-f]{6,}>$', ' at [0-9A-F]{8,}>$']:
        if re.search(pattern, repr(Exception)):
            return re.sub(pattern, '>', text)
    return text

def modulename(path):
    """Return the Python module name for a given path, or None."""
    filename = os.path.basename(path)
    suffixes = map(lambda (suffix, mode, kind): (len(suffix), suffix),
                   imp.get_suffixes())
    suffixes.sort()
    suffixes.reverse() # try longest suffixes first, in case they overlap
    for length, suffix in suffixes:
        if len(filename) > length and filename[-length:] == suffix:
            return filename[:-length]

def allmethods(cl):
    methods = {}
    for key, value in inspect.getmembers(cl, inspect.ismethod):
        methods[key] = 1
    for base in cl.__bases__:
        methods.update(allmethods(base)) # all your base are belong to us
    for key in methods.keys():
        methods[key] = getattr(cl, key)
    return methods

class ErrorDuringImport(Exception):
    """Errors that occurred while trying to import something to document it."""
    def __init__(self, filename, (exc, value, tb)):
        self.filename = filename
        self.exc = exc
        self.value = value
        self.tb = tb

    def __str__(self):
        exc = self.exc
        if type(exc) is types.ClassType:
            exc = exc.__name__
        return 'problem in %s - %s: %s' % (self.filename, exc, self.value)

def importfile(path):
    """Import a Python source file or compiled file given its path."""
    magic = imp.get_magic()
    file = open(path, 'r')
    if file.read(len(magic)) == magic:
        kind = imp.PY_COMPILED
    else:
        kind = imp.PY_SOURCE
    file.close()
    filename = os.path.basename(path)
    name, ext = os.path.splitext(filename)
    file = open(path, 'r')
    try:
        module = imp.load_module(name, file, path, (ext, 'r', kind))
    except:
        raise ErrorDuringImport(path, sys.exc_info())
    file.close()
    return module

def ispackage(path):
    """Guess whether a path refers to a package directory."""
    if os.path.isdir(path):
        for ext in ['.py', '.pyc', '.pyo']:
            if os.path.isfile(os.path.join(path, '__init__' + ext)):
                return 1

# ---------------------------------------------------- formatter base class

class Doc:
    def document(self, object, name=None, *args):
        """Generate documentation for an object."""
        args = (object, name) + args
        if inspect.ismodule(object): return apply(self.docmodule, args)
        if inspect.isclass(object): return apply(self.docclass, args)
        if inspect.isroutine(object): return apply(self.docroutine, args)
        return apply(self.docother, args)

    def fail(self, object, name=None, *args):
        """Raise an exception for unimplemented types."""
        message = "don't know how to document object%s of type %s" % (
            name and ' ' + repr(name), type(object).__name__)
        raise TypeError, message

    docmodule = docclass = docroutine = docother = fail

# -------------------------------------------- HTML documentation generator

class HTMLRepr(Repr):
    """Class for safely making an HTML representation of a Python object."""
    def __init__(self):
        Repr.__init__(self)
        self.maxlist = self.maxtuple = self.maxdict = 10
        self.maxstring = self.maxother = 100

    def escape(self, text):
        return replace(text, ('&', '&amp;'), ('<', '&lt;'), ('>', '&gt;'))

    def repr(self, object):
        return Repr.repr(self, object)

    def repr1(self, x, level):
        methodname = 'repr_' + join(split(type(x).__name__), '_')
        if hasattr(self, methodname):
            return getattr(self, methodname)(x, level)
        else:
            return self.escape(cram(stripid(repr(x)), self.maxother))

    def repr_string(self, x, level):
        test = cram(x, self.maxstring)
        testrepr = repr(test)
        if '\\' in test and '\\' not in replace(testrepr, (r'\\', '')):
            # Backslashes are only literal in the string and are never
            # needed to make any special characters, so show a raw string.
            return 'r' + testrepr[0] + self.escape(test) + testrepr[0]
        return re.sub(r'((\\[\\abfnrtv\'"]|\\x..|\\u....)+)',
                      r'<font color="#c040c0">\1</font>',
                      self.escape(testrepr))

    def repr_instance(self, x, level):
        try:
            return self.escape(cram(stripid(repr(x)), self.maxstring))
        except:
            return self.escape('<%s instance>' % x.__class__.__name__)

    repr_unicode = repr_string

class HTMLDoc(Doc):
    """Formatter class for HTML documentation."""

    # ------------------------------------------- HTML formatting utilities

    _repr_instance = HTMLRepr()
    repr = _repr_instance.repr
    escape = _repr_instance.escape

    def page(self, title, contents):
        """Format an HTML page."""
        return '''
<!doctype html public "-//W3C//DTD HTML 4.0 Transitional//EN">
<html><head><title>Python: %s</title>
<style type="text/css"><!--
TT { font-family: lucida console, lucida typewriter, courier }
--></style></head><body bgcolor="#f0f0f8">
%s
</body></html>''' % (title, contents)

    def heading(self, title, fgcol, bgcol, extras=''):
        """Format a page heading."""
        return '''
<table width="100%%" cellspacing=0 cellpadding=2 border=0>
<tr bgcolor="%s">
<td valign=bottom><small>&nbsp;<br></small
><font color="%s" face="helvetica, arial">&nbsp;<br>%s</font></td
><td align=right valign=bottom
><font color="%s" face="helvetica, arial">%s</font></td></tr></table>
    ''' % (bgcol, fgcol, title, fgcol, extras or '&nbsp;')

    def section(self, title, fgcol, bgcol, contents, width=20,
                prelude='', marginalia=None, gap='&nbsp;&nbsp;'):
        """Format a section with a heading."""
        if marginalia is None:
            marginalia = '&nbsp;' * width
        result = '''
<p><table width="100%%" cellspacing=0 cellpadding=2 border=0>
<tr bgcolor="%s">
<td colspan=3 valign=bottom><small><small>&nbsp;<br></small></small
><font color="%s" face="helvetica, arial">%s</font></td></tr>
    ''' % (bgcol, fgcol, title)
        if prelude:
            result = result + '''
<tr bgcolor="%s"><td rowspan=2>%s</td>
<td colspan=2>%s</td></tr>
<tr><td>%s</td>''' % (bgcol, marginalia, prelude, gap)
        else:
            result = result + '''
<tr><td bgcolor="%s">%s</td><td>%s</td>''' % (bgcol, marginalia, gap)

        return result + '<td width="100%%">%s</td></tr></table>' % contents

    def bigsection(self, title, *args):
        """Format a section with a big heading."""
        title = '<big><strong>%s</strong></big>' % title
        return apply(self.section, (title,) + args)

    def preformat(self, text):
        """Format literal preformatted text."""
        text = self.escape(expandtabs(text))
        return replace(text, ('\n\n', '\n \n'), ('\n\n', '\n \n'),
                             (' ', '&nbsp;'), ('\n', '<br>\n'))

    def multicolumn(self, list, format, cols=4):
        """Format a list of items into a multi-column list."""
        result = ''
        rows = (len(list)+cols-1)/cols
        for col in range(cols):
            result = result + '<td width="%d%%" valign=top>' % (100/cols)
            for i in range(rows*col, rows*col+rows):
                if i < len(list):
                    result = result + format(list[i]) + '<br>\n'
            result = result + '</td>'
        return '<table width="100%%"><tr>%s</tr></table>' % result

    def small(self, text): return '<small>%s</small>' % text
    def grey(self, text): return '<font color="#909090">%s</font>' % text

    def namelink(self, name, *dicts):
        """Make a link for an identifier, given name-to-URL mappings."""
        for dict in dicts:
            if dict.has_key(name):
                return '<a href="%s">%s</a>' % (dict[name], name)
        return name

    def classlink(self, object, modname, *dicts):
        """Make a link for a class."""
        name = object.__name__
        if object.__module__ != modname:
            name = object.__module__ + '.' + name
        for dict in dicts:
            if dict.has_key(object):
                return '<a href="%s">%s</a>' % (dict[object], name)
        return name

    def modulelink(self, object):
        """Make a link for a module."""
        return '<a href="%s.html">%s</a>' % (object.__name__, object.__name__)

    def modpkglink(self, (name, path, ispackage, shadowed)):
        """Make a link for a module or package to display in an index."""
        if shadowed:
            return self.grey(name)
        if path:
            url = '%s.%s.html' % (path, name)
        else:
            url = '%s.html' % name
        if ispackage:
            text = '<strong>%s</strong>&nbsp;(package)' % name
        else:
            text = name
        return '<a href="%s">%s</a>' % (url, text)

    def markup(self, text, escape=None, funcs={}, classes={}, methods={}):
        """Mark up some plain text, given a context of symbols to look for.
        Each context dictionary maps object names to anchor names."""
        escape = escape or self.escape
        results = []
        here = 0
        pattern = re.compile(r'\b((http|ftp)://\S+[\w/]|'
                                r'RFC[- ]?(\d+)|'
                                r'(self\.)?(\w+))\b')
        while 1:
            match = pattern.search(text, here)
            if not match: break
            start, end = match.span()
            results.append(escape(text[here:start]))

            all, scheme, rfc, selfdot, name = match.groups()
            if scheme:
                results.append('<a href="%s">%s</a>' % (all, escape(all)))
            elif rfc:
                url = 'http://www.rfc-editor.org/rfc/rfc%s.txt' % rfc
                results.append('<a href="%s">%s</a>' % (url, escape(all)))
            elif text[end:end+1] == '(':
                results.append(self.namelink(name, methods, funcs, classes))
            elif selfdot:
                results.append('self.<strong>%s</strong>' % name)
            else:
                results.append(self.namelink(name, classes))
            here = end
        results.append(escape(text[here:]))
        return join(results, '')

    # ---------------------------------------------- type-specific routines

    def formattree(self, tree, modname, classes={}, parent=None):
        """Produce HTML for a class tree as given by inspect.getclasstree()."""
        result = ''
        for entry in tree:
            if type(entry) is type(()):
                c, bases = entry
                result = result + '<dt><font face="helvetica, arial"><small>'
                result = result + self.classlink(c, modname, classes)
                if bases and bases != (parent,):
                    parents = []
                    for base in bases:
                        parents.append(self.classlink(base, modname, classes))
                    result = result + '(' + join(parents, ', ') + ')'
                result = result + '\n</small></font></dt>'
            elif type(entry) is type([]):
                result = result + '<dd>\n%s</dd>\n' % self.formattree(
                    entry, modname, classes, c)
        return '<dl>\n%s</dl>\n' % result

    def docmodule(self, object, name=None):
        """Produce HTML documentation for a module object."""
        name = object.__name__ # ignore the passed-in name
        parts = split(name, '.')
        links = []
        for i in range(len(parts)-1):
            links.append(
                '<a href="%s.html"><font color="#ffffff">%s</font></a>' %
                (join(parts[:i+1], '.'), parts[i]))
        linkedname = join(links + parts[-1:], '.')
        head = '<big><big><strong>%s</strong></big></big>' % linkedname
        try:
            path = inspect.getabsfile(object)
            filelink = '<a href="file:%s">%s</a>' % (path, path)
        except TypeError:
            filelink = '(built-in)'
        info = []
        if hasattr(object, '__version__'):
            version = str(object.__version__)
            if version[:11] == '$' + 'Revision: ' and version[-1:] == '$':
                version = strip(version[11:-1])
            info.append('version %s' % self.escape(version))
        if hasattr(object, '__date__'):
            info.append(self.escape(str(object.__date__)))
        if info:
            head = head + ' (%s)' % join(info, ', ')
        result = self.heading(
            head, '#ffffff', '#7799ee', '<a href=".">index</a><br>' + filelink)

        modules = inspect.getmembers(object, inspect.ismodule)

        if 0 and hasattr(object, '__all__'): # disabled for now
            visible = lambda key, all=object.__all__: key in all
        else:
            visible = lambda key: key[:1] != '_'

        classes, cdict = [], {}
        for key, value in inspect.getmembers(object, inspect.isclass):
            if visible(key) and (
                inspect.getmodule(value) or object) is object:
                classes.append((key, value))
                cdict[key] = cdict[value] = '#' + key
        for key, value in classes:
            for base in value.__bases__:
                key, modname = base.__name__, base.__module__
                module = sys.modules.get(modname)
                if modname != name and module and hasattr(module, key):
                    if getattr(module, key) is base:
                        if not cdict.has_key(key):
                            cdict[key] = cdict[base] = modname + '.html#' + key
        funcs, fdict = [], {}
        for key, value in inspect.getmembers(object, inspect.isroutine):
            if visible(key) and (inspect.isbuiltin(value) or
                                 inspect.getmodule(value) is object):
                funcs.append((key, value))
                fdict[key] = '#-' + key
                if inspect.isfunction(value): fdict[value] = fdict[key]
        constants = []
        for key, value in inspect.getmembers(object, isconstant):
            if visible(key):
                constants.append((key, value))

        doc = self.markup(getdoc(object), self.preformat, fdict, cdict)
        doc = doc and '<tt>%s</tt>' % doc
        result = result + '<p>%s</p>\n' % self.small(doc)

        if hasattr(object, '__path__'):
            modpkgs = []
            modnames = []
            for file in os.listdir(object.__path__[0]):
                if file[:1] != '_':
                    path = os.path.join(object.__path__[0], file)
                    modname = modulename(file)
                    if modname and modname not in modnames:
                        modpkgs.append((modname, name, 0, 0))
                        modnames.append(modname)
                    elif ispackage(path):
                        modpkgs.append((file, name, 1, 0))
            modpkgs.sort()
            contents = self.multicolumn(modpkgs, self.modpkglink)
            result = result + self.bigsection(
                'Package Contents', '#ffffff', '#aa55cc', contents)
        elif modules:
            contents = self.multicolumn(
                modules, lambda (key, value), s=self: s.modulelink(value))
            result = result + self.bigsection(
                'Modules', '#fffff', '#aa55cc', contents)

        if classes:
            classlist = map(lambda (key, value): value, classes)
            contents = [self.formattree(
                inspect.getclasstree(classlist, 1), name, cdict)]
            for key, value in classes:
                contents.append(self.document(value, key, fdict, cdict))
            result = result + self.bigsection(
                'Classes', '#ffffff', '#ee77aa', join(contents))
        if funcs:
            contents = []
            for key, value in funcs:
                contents.append(self.document(value, key, fdict, cdict))
            result = result + self.bigsection(
                'Functions', '#ffffff', '#eeaa77', join(contents))
        if constants:
            contents = []
            for key, value in constants:
                contents.append(self.document(value, key))
            result = result + self.bigsection(
                'Constants', '#ffffff', '#55aa55', join(contents, '<br>'))
        if hasattr(object, '__author__'):
            contents = self.markup(str(object.__author__), self.preformat)
            result = result + self.bigsection(
                'Author', '#ffffff', '#7799ee', contents)
        if hasattr(object, '__credits__'):
            contents = self.markup(str(object.__credits__), self.preformat)
            result = result + self.bigsection(
                'Credits', '#ffffff', '#7799ee', contents)

        return result

    def docclass(self, object, name=None, funcs={}, classes={}):
        """Produce HTML documentation for a class object."""
        realname = object.__name__
        name = name or realname
        bases = object.__bases__
        contents = ''

        methods, mdict = [], {}
        for key, value in allmethods(object).items():
            methods.append((key, value))
            mdict[key] = mdict[value] = '#' + name + '-' + key
        methods.sort()
        for key, value in methods:
            contents = contents + self.document(
                value, key, funcs, classes, mdict, object)

        if name == realname:
            title = '<a name="%s">class <strong>%s</strong></a>' % (
                name, realname)
        else:
            title = '<strong>%s</strong> = <a name="%s">class %s</a>' % (
                name, name, realname)
        if bases:
            parents = []
            for base in bases:
                parents.append(
                    self.classlink(base, object.__module__, classes))
            title = title + '(%s)' % join(parents, ', ')
        doc = self.markup(
            getdoc(object), self.preformat, funcs, classes, mdict)
        doc = self.small(doc and '<tt>%s<br>&nbsp;</tt>' % doc or '<tt>&nbsp;</tt>')
        return self.section(title, '#000000', '#ffc8d8', contents, 10, doc)

    def formatvalue(self, object):
        """Format an argument default value as text."""
        return self.small(self.grey('=' + self.repr(object)))

    def docroutine(self, object, name=None,
                   funcs={}, classes={}, methods={}, cl=None):
        """Produce HTML documentation for a function or method object."""
        realname = object.__name__
        name = name or realname
        anchor = (cl and cl.__name__ or '') + '-' + name
        note = ''
        skipdocs = 0
        if inspect.ismethod(object):
            if cl:
                if object.im_class is not cl:
                    base = object.im_class
                    url = '#%s-%s' % (base.__name__, name)
                    basename = base.__name__
                    if base.__module__ != cl.__module__:
                        url = base.__module__ + '.html' + url
                        basename = base.__module__ + '.' + basename
                    note = ' from <a href="%s">%s</a>' % (url, basename)
                    skipdocs = 1
            else:
                note = (object.im_self and
                        ' method of ' + self.repr(object.im_self) or
                        ' unbound %s method' % object.im_class.__name__)
            object = object.im_func

        if name == realname:
            title = '<a name="%s"><strong>%s</strong></a>' % (anchor, realname)
        else:
            if (cl and cl.__dict__.has_key(realname) and
                cl.__dict__[realname] is object):
                reallink = '<a href="#%s">%s</a>' % (
                    cl.__name__ + '-' + realname, realname)
                skipdocs = 1
            else:
                reallink = realname
            title = '<a name="%s"><strong>%s</strong></a> = %s' % (
                anchor, name, reallink)
        if inspect.isbuiltin(object):
            argspec = '(...)'
        else:
            args, varargs, varkw, defaults = inspect.getargspec(object)
            argspec = inspect.formatargspec(
                args, varargs, varkw, defaults, formatvalue=self.formatvalue)
            if realname == '<lambda>':
                decl = '<em>lambda</em>'
                argspec = argspec[1:-1] # remove parentheses

        decl = title + argspec + (note and self.small(self.grey(
            '<font face="helvetica, arial">%s</font>' % note)))

        if skipdocs:
            return '<dl><dt>%s</dl>\n' % decl
        else:
            doc = self.markup(
                getdoc(object), self.preformat, funcs, classes, methods)
            doc = doc and '<tt>%s</tt>' % doc
            return '<dl><dt>%s<dd>%s</dl>\n' % (decl, self.small(doc))

    def docother(self, object, name=None):
        """Produce HTML documentation for a data object."""
        return '<strong>%s</strong> = %s' % (name, self.repr(object))

    def index(self, dir, shadowed=None):
        """Generate an HTML index for a directory of modules."""
        modpkgs = []
        if shadowed is None: shadowed = {}
        seen = {}
        files = os.listdir(dir)

        def found(name, ispackage,
                  modpkgs=modpkgs, shadowed=shadowed, seen=seen):
            if not seen.has_key(name):
                modpkgs.append((name, '', ispackage, shadowed.has_key(name)))
                seen[name] = 1
                shadowed[name] = 1

        # Package spam/__init__.py takes precedence over module spam.py.
        for file in files:
            path = os.path.join(dir, file)
            if ispackage(path): found(file, 1)
        for file in files:
            path = os.path.join(dir, file)
            if file[:1] != '_' and os.path.isfile(path):
                modname = modulename(file)
                if modname: found(modname, 0)

        modpkgs.sort()
        contents = self.multicolumn(modpkgs, self.modpkglink)
        return self.bigsection(dir, '#ffffff', '#ee77aa', contents)

# -------------------------------------------- text documentation generator

class TextRepr(Repr):
    """Class for safely making a text representation of a Python object."""
    def __init__(self):
        Repr.__init__(self)
        self.maxlist = self.maxtuple = self.maxdict = 10
        self.maxstring = self.maxother = 100

    def repr1(self, x, level):
        methodname = 'repr_' + join(split(type(x).__name__), '_')
        if hasattr(self, methodname):
            return getattr(self, methodname)(x, level)
        else:
            return cram(stripid(repr(x)), self.maxother)

    def repr_string(self, x, level):
        test = cram(x, self.maxstring)
        testrepr = repr(test)
        if '\\' in test and '\\' not in replace(testrepr, (r'\\', '')):
            # Backslashes are only literal in the string and are never
            # needed to make any special characters, so show a raw string.
            return 'r' + testrepr[0] + test + testrepr[0]
        return testrepr

    def repr_instance(self, x, level):
        try:
            return cram(stripid(repr(x)), self.maxstring)
        except:
            return '<%s instance>' % x.__class__.__name__

class TextDoc(Doc):
    """Formatter class for text documentation."""

    # ------------------------------------------- text formatting utilities

    _repr_instance = TextRepr()
    repr = _repr_instance.repr

    def bold(self, text):
        """Format a string in bold by overstriking."""
        return join(map(lambda ch: ch + '\b' + ch, text), '')

    def indent(self, text, prefix='    '):
        """Indent text by prepending a given prefix to each line."""
        if not text: return ''
        lines = split(text, '\n')
        lines = map(lambda line, prefix=prefix: prefix + line, lines)
        if lines: lines[-1] = rstrip(lines[-1])
        return join(lines, '\n')

    def section(self, title, contents):
        """Format a section with a given heading."""
        return self.bold(title) + '\n' + rstrip(self.indent(contents)) + '\n\n'

    # ---------------------------------------------- type-specific routines

    def formattree(self, tree, modname, parent=None, prefix=''):
        """Render in text a class tree as returned by inspect.getclasstree()."""
        result = ''
        for entry in tree:
            if type(entry) is type(()):
                c, bases = entry
                result = result + prefix + classname(c, modname)
                if bases and bases != (parent,):
                    parents = map(lambda c, m=modname: classname(c, m), bases)
                    result = result + '(%s)' % join(parents, ', ')
                result = result + '\n'
            elif type(entry) is type([]):
                result = result + self.formattree(
                    entry, modname, c, prefix + '    ')
        return result

    def docmodule(self, object, name=None):
        """Produce text documentation for a given module object."""
        name = object.__name__ # ignore the passed-in name
        namesec = name
        lines = split(strip(getdoc(object)), '\n')
        if len(lines) == 1:
            if lines[0]: namesec = namesec + ' - ' + lines[0]
            lines = []
        elif len(lines) >= 2 and not rstrip(lines[1]):
            if lines[0]: namesec = namesec + ' - ' + lines[0]
            lines = lines[2:]
        result = self.section('NAME', namesec)

        try:
            file = inspect.getabsfile(object)
        except TypeError:
            file = '(built-in)'
        result = result + self.section('FILE', file)
        if lines:
            result = result + self.section('DESCRIPTION', join(lines, '\n'))

        classes = []
        for key, value in inspect.getmembers(object, inspect.isclass):
            if (inspect.getmodule(value) or object) is object:
                classes.append((key, value))
        funcs = []
        for key, value in inspect.getmembers(object, inspect.isroutine):
            if inspect.isbuiltin(value) or inspect.getmodule(value) is object:
                funcs.append((key, value))
        constants = []
        for key, value in inspect.getmembers(object, isconstant):
            if key[:1] != '_':
                constants.append((key, value))

        if hasattr(object, '__path__'):
            modpkgs = []
            for file in os.listdir(object.__path__[0]):
                if file[:1] != '_':
                    path = os.path.join(object.__path__[0], file)
                    modname = modulename(file)
                    if modname and modname not in modpkgs:
                        modpkgs.append(modname)
                    elif ispackage(path):
                        modpkgs.append(file + ' (package)')
            modpkgs.sort()
            result = result + self.section(
                'PACKAGE CONTENTS', join(modpkgs, '\n'))

        if classes:
            classlist = map(lambda (key, value): value, classes)
            contents = [self.formattree(
                inspect.getclasstree(classlist, 1), name)]
            for key, value in classes:
                contents.append(self.document(value, key))
            result = result + self.section('CLASSES', join(contents, '\n'))

        if funcs:
            contents = []
            for key, value in funcs:
                contents.append(self.document(value, key))
            result = result + self.section('FUNCTIONS', join(contents, '\n'))

        if constants:
            contents = []
            for key, value in constants:
                contents.append(self.docother(value, key, 70))
            result = result + self.section('CONSTANTS', join(contents, '\n'))

        if hasattr(object, '__version__'):
            version = str(object.__version__)
            if version[:11] == '$' + 'Revision: ' and version[-1:] == '$':
                version = strip(version[11:-1])
            result = result + self.section('VERSION', version)
        if hasattr(object, '__date__'):
            result = result + self.section('DATE', str(object.__date__))
        if hasattr(object, '__author__'):
            result = result + self.section('AUTHOR', str(object.__author__))
        if hasattr(object, '__credits__'):
            result = result + self.section('CREDITS', str(object.__credits__))
        return result

    def docclass(self, object, name=None):
        """Produce text documentation for a given class object."""
        realname = object.__name__
        name = name or realname
        bases = object.__bases__

        if name == realname:
            title = 'class ' + self.bold(realname)
        else:
            title = self.bold(name) + ' = class ' + realname
        if bases:
            def makename(c, m=object.__module__): return classname(c, m)
            parents = map(makename, bases)
            title = title + '(%s)' % join(parents, ', ')

        doc = getdoc(object)
        contents = doc and doc + '\n'
        methods = allmethods(object).items()
        methods.sort()
        for key, value in methods:
            contents = contents + '\n' + self.document(value, key, object)

        if not contents: return title + '\n'
        return title + '\n' + self.indent(rstrip(contents), ' |  ') + '\n'

    def formatvalue(self, object):
        """Format an argument default value as text."""
        return '=' + self.repr(object)

    def docroutine(self, object, name=None, cl=None):
        """Produce text documentation for a function or method object."""
        realname = object.__name__
        name = name or realname
        note = ''
        skipdocs = 0
        if inspect.ismethod(object):
            if cl:
                if object.im_class is not cl:
                    base = object.im_class
                    basename = base.__name__
                    if base.__module__ != cl.__module__:
                        basename = base.__module__ + '.' + basename
                    note = ' from %s' % basename
                    skipdocs = 1
            else:
                if object.im_self:
                    note = ' method of %s' % self.repr(object.im_self)
                else:
                    note = ' unbound %s method' % object.im_class.__name__
            object = object.im_func

        if name == realname:
            title = self.bold(realname)
        else:
            if (cl and cl.__dict__.has_key(realname) and
                cl.__dict__[realname] is object):
                skipdocs = 1
            title = self.bold(name) + ' = ' + realname
        if inspect.isbuiltin(object):
            argspec = '(...)'
        else:
            args, varargs, varkw, defaults = inspect.getargspec(object)
            argspec = inspect.formatargspec(
                args, varargs, varkw, defaults, formatvalue=self.formatvalue)
            if realname == '<lambda>':
                title = 'lambda'
                argspec = argspec[1:-1] # remove parentheses
        decl = title + argspec + note

        if skipdocs:
            return decl + '\n'
        else:
            doc = getdoc(object) or ''
            return decl + '\n' + (doc and rstrip(self.indent(doc)) + '\n')

    def docother(self, object, name=None, maxlen=None):
        """Produce text documentation for a data object."""
        repr = self.repr(object)
        if maxlen:
            line = name + ' = ' + repr
            chop = maxlen - len(line)
            if chop < 0: repr = repr[:chop] + '...'
        line = self.bold(name) + ' = ' + repr
        return line

# --------------------------------------------------------- user interfaces

def pager(text):
    """The first time this is called, determine what kind of pager to use."""
    global pager
    pager = getpager()
    pager(text)

def getpager():
    """Decide what method to use for paging through text."""
    if type(sys.stdout) is not types.FileType:
        return plainpager
    if not sys.stdin.isatty() or not sys.stdout.isatty():
        return plainpager
    if os.environ.has_key('PAGER'):
        return lambda a: pipepager(a, os.environ['PAGER'])
    if sys.platform == 'win32':
        return lambda a: tempfilepager(a, 'more <')
    if hasattr(os, 'system') and os.system('less 2>/dev/null') == 0:
        return lambda a: pipepager(a, 'less')

    import tempfile
    filename = tempfile.mktemp()
    open(filename, 'w').close()
    try:
        if hasattr(os, 'system') and os.system('more %s' % filename) == 0:
            return lambda text: pipepager(text, 'more')
        else:
            return ttypager
    finally:
        os.unlink(filename)

def pipepager(text, cmd):
    """Page through text by feeding it to another program."""
    pipe = os.popen(cmd, 'w')
    try:
        pipe.write(text)
        pipe.close()
    except IOError:
        # Ignore broken pipes caused by quitting the pager program.
        pass

def tempfilepager(text, cmd):
    """Page through text by invoking a program on a temporary file."""
    import tempfile
    filename = tempfile.mktemp()
    file = open(filename, 'w')
    file.write(text)
    file.close()
    try:
        os.system(cmd + ' ' + filename)
    finally:
        os.unlink(filename)

def plain(text):
    """Remove boldface formatting from text."""
    return re.sub('.\b', '', text)

def ttypager(text):
    """Page through text on a text terminal."""
    lines = split(plain(text), '\n')
    try:
        import tty
        fd = sys.stdin.fileno()
        old = tty.tcgetattr(fd)
        tty.setcbreak(fd)
        getchar = lambda: sys.stdin.read(1)
    except (ImportError, AttributeError):
        tty = None
        getchar = lambda: sys.stdin.readline()[:-1][:1]

    try:
        r = inc = os.environ.get('LINES', 25) - 1
        sys.stdout.write(join(lines[:inc], '\n') + '\n')
        while lines[r:]:
            sys.stdout.write('-- more --')
            sys.stdout.flush()
            c = getchar()

            if c in ['q', 'Q']:
                sys.stdout.write('\r          \r')
                break
            elif c in ['\r', '\n']:
                sys.stdout.write('\r          \r' + lines[r] + '\n')
                r = r + 1
                continue
            if c in ['b', 'B', '\x1b']:
                r = r - inc - inc
                if r < 0: r = 0
            sys.stdout.write('\n' + join(lines[r:r+inc], '\n') + '\n')
            r = r + inc

    finally:
        if tty:
            tty.tcsetattr(fd, tty.TCSAFLUSH, old)

def plainpager(text):
    """Simply print unformatted text.  This is the ultimate fallback."""
    sys.stdout.write(plain(text))

def describe(thing):
    """Produce a short description of the given thing."""
    if inspect.ismodule(thing):
        if thing.__name__ in sys.builtin_module_names:
            return 'built-in module ' + thing.__name__
        if hasattr(thing, '__path__'):
            return 'package ' + thing.__name__
        else:
            return 'module ' + thing.__name__
    if inspect.isbuiltin(thing):
        return 'built-in function ' + thing.__name__
    if inspect.isclass(thing):
        return 'class ' + thing.__name__
    if inspect.isfunction(thing):
        return 'function ' + thing.__name__
    if inspect.ismethod(thing):
        return 'method ' + thing.__name__
    if type(thing) is types.InstanceType:
        return 'instance of ' + thing.__class__.__name__
    return type(thing).__name__

def freshimport(name, cache={}):
    """Import a module, reloading it if the source file has changed."""
    topmodule = __import__(name)
    module = None
    for component in split(name, '.'):
        if module == None:
            module = topmodule
            path = split(name, '.')[0]
        else:
            module = getattr(module, component)
            path = path + '.' + component
        if hasattr(module, '__file__'):
            file = module.__file__
            if os.path.exists(file):
                info = (file, os.path.getmtime(file), os.path.getsize(file))
            if cache.has_key(path) and cache[path] != info:
                module = reload(module)
            file = module.__file__
            if os.path.exists(file):
                info = (file, os.path.getmtime(file), os.path.getsize(file))
            cache[path] = info
    return module

def locate(path):
    """Locate an object by name (or dotted path), importing as necessary."""
    if not path: # special case: imp.find_module('') strangely succeeds
        return None
    if type(path) is not types.StringType:
        return path
    parts = split(path, '.')
    n = len(parts)
    while n > 0:
        path = join(parts[:n], '.')
        try:
            module = freshimport(path)
        except:
            # Did the error occur before or after the module was found?
            (exc, value, tb) = info = sys.exc_info()
            if sys.modules.has_key(path):
                # An error occured while executing the imported module.
                raise ErrorDuringImport(sys.modules[path].__file__, info)
            elif exc is SyntaxError:
                # A SyntaxError occurred before we could execute the module.
                raise ErrorDuringImport(value.filename, info)
            elif exc is ImportError and \
                 split(lower(str(value)))[:2] == ['no', 'module']:
                # The module was not found.
                n = n - 1
                continue
            else:
                # Some other error occurred before executing the module.
                raise DocImportError(filename, sys.exc_info())
        try:
            x = module
            for p in parts[n:]:
                x = getattr(x, p)
            return x
        except AttributeError:
            n = n - 1
            continue
    if hasattr(__builtins__, path):
        return getattr(__builtins__, path)
    return None

# --------------------------------------- interactive interpreter interface

text = TextDoc()
html = HTMLDoc()

def doc(thing):
    """Display documentation on an object (for interactive use)."""
    if type(thing) is type(""):
        try:
            object = locate(thing)
        except ErrorDuringImport, value:
            print value
            return
        if object:
            thing = object
        else:
            print 'no Python documentation found for %s' % repr(thing)
            return

    desc = describe(thing)
    module = inspect.getmodule(thing)
    if module and module is not thing:
        desc = desc + ' in module ' + module.__name__
    pager('Help on %s:\n\n' % desc + text.document(thing))

def writedoc(key):
    """Write HTML documentation to a file in the current directory."""
    try:
        object = locate(key)
    except ErrorDuringImport, value:
        print value
    else:
        if object:
            page = html.page('Python: ' + describe(object),
                             html.document(object, object.__name__))
            file = open(key + '.html', 'w')
            file.write(page)
            file.close()
            print 'wrote', key + '.html'
        else:
            print 'no Python documentation found for %s' % repr(key)

def writedocs(dir, pkgpath='', done={}):
    """Write out HTML documentation for all modules in a directory tree."""
    for file in os.listdir(dir):
        path = os.path.join(dir, file)
        if ispackage(path):
            writedocs(path, pkgpath + file + '.')
        elif os.path.isfile(path):
            modname = modulename(path)
            if modname:
                modname = pkgpath + modname
                if not done.has_key(modname):
                    done[modname] = 1
                    writedoc(modname)

class Helper:
    def __repr__(self):
        return '''To get help on a Python object, call help(object).
To get help on a module or package, either import it before calling
help(module) or call help('modulename').'''

    def __call__(self, *args):
        if args:
            doc(args[0])
        else:
            print repr(self)

help = Helper()

def man(key):
    """Display documentation on an object in a form similar to man(1)."""
    object = locate(key)
    if object:
        title = 'Python Library Documentation: ' + describe(object)
        lastdot = rfind(key, '.')
        if lastdot > 0: title = title + ' in ' + key[:lastdot]
        pager('\n' + title + '\n\n' + text.document(object, key))
        found = 1
    else:
        print 'no Python documentation found for %s' % repr(key)

class Scanner:
    """A generic tree iterator."""
    def __init__(self, roots, children, recurse):
        self.roots = roots[:]
        self.state = []
        self.children = children
        self.recurse = recurse

    def next(self):
        if not self.state:
            if not self.roots:
                return None
            root = self.roots.pop(0)
            self.state = [(root, self.children(root))]
        node, children = self.state[-1]
        if not children:
            self.state.pop()
            return self.next()
        child = children.pop(0)
        if self.recurse(child):
            self.state.append((child, self.children(child)))
        return child

class ModuleScanner(Scanner):
    """An interruptible scanner that searches module synopses."""
    def __init__(self):
        roots = map(lambda dir: (dir, ''), pathdirs())
        Scanner.__init__(self, roots, self.submodules, self.ispackage)

    def submodules(self, (dir, package)):
        children = []
        for file in os.listdir(dir):
            path = os.path.join(dir, file)
            if ispackage(path):
                children.append((path, package + (package and '.') + file))
            else:
                children.append((path, package))
        children.sort()
        return children

    def ispackage(self, (dir, package)):
        return ispackage(dir)

    def run(self, key, callback, completer=None):
        self.quit = 0
        seen = {}

        for modname in sys.builtin_module_names:
            if modname != '__main__':
                seen[modname] = 1
                desc = split(freshimport(modname).__doc__ or '', '\n')[0]
                if find(lower(modname + ' - ' + desc), lower(key)) >= 0:
                    callback(None, modname, desc)

        while not self.quit:
            node = self.next()
            if not node: break
            path, package = node
            modname = modulename(path)
            if os.path.isfile(path) and modname:
                modname = package + (package and '.') + modname
                if not seen.has_key(modname):
                    seen[modname] = 1
                    desc = synopsis(path) or ''
                    if find(lower(modname + ' - ' + desc), lower(key)) >= 0:
                        callback(path, modname, desc)
        if completer: completer()

def apropos(key):
    """Print all the one-line module summaries that contain a substring."""
    def callback(path, modname, desc):
        if modname[-9:] == '.__init__':
            modname = modname[:-9] + ' (package)'
        print modname, '-', desc or '(no description)'
    ModuleScanner().run(key, callback)

# --------------------------------------------------- web browser interface

def serve(port, callback=None):
    import BaseHTTPServer, mimetools, select

    # Patch up mimetools.Message so it doesn't break if rfc822 is reloaded.
    class Message(mimetools.Message):
        def __init__(self, fp, seekable=1):
            Message = self.__class__
            Message.__bases__[0].__bases__[0].__init__(self, fp, seekable)
            self.encodingheader = self.getheader('content-transfer-encoding')
            self.typeheader = self.getheader('content-type')
            self.parsetype()
            self.parseplist()

    class DocHandler(BaseHTTPServer.BaseHTTPRequestHandler):
        def send_document(self, title, contents):
            try:
                self.send_response(200)
                self.send_header('Content-Type', 'text/html')
                self.end_headers()
                self.wfile.write(html.page(title, contents))
            except IOError: pass

        def do_GET(self):
            path = self.path
            if path[-5:] == '.html': path = path[:-5]
            if path[:1] == '/': path = path[1:]
            if path and path != '.':
                try:
                    obj = locate(path)
                except ErrorDuringImport, value:
                    self.send_document(path, html.escape(str(value)))
                    return
                if obj:
                    self.send_document(describe(obj), html.document(obj, path))
                else:
                    self.send_document(path,
'no Python documentation found for %s' % repr(path))
            else:
                heading = html.heading(
'<big><big><strong>Python: Index of Modules</strong></big></big>',
'#ffffff', '#7799ee')
                def bltinlink(name):
                    return '<a href="%s.html">%s</a>' % (name, name)
                names = filter(lambda x: x != '__main__',
                               sys.builtin_module_names)
                contents = html.multicolumn(names, bltinlink)
                indices = ['<p>' + html.bigsection(
                    'Built-in Modules', '#ffffff', '#ee77aa', contents)]

                seen = {}
                for dir in pathdirs():
                    indices.append(html.index(dir, seen))
                contents = heading + join(indices) + '''<p align=right>
<small><small><font color="#909090" face="helvetica, arial"><strong>
pydoc</strong> by Ka-Ping Yee &lt;ping@lfw.org&gt;</font></small></small>'''
                self.send_document('Index of Modules', contents)

        def log_message(self, *args): pass

    class DocServer(BaseHTTPServer.HTTPServer):
        def __init__(self, port, callback):
            host = (sys.platform == 'mac') and '127.0.0.1' or 'localhost'
            self.address = ('', port)
            self.url = 'http://%s:%d/' % (host, port)
            self.callback = callback
            self.base.__init__(self, self.address, self.handler)

        def serve_until_quit(self):
            import select
            self.quit = 0
            while not self.quit:
                rd, wr, ex = select.select([self.socket.fileno()], [], [], 1)
                if rd: self.handle_request()

        def server_activate(self):
            self.base.server_activate(self)
            if self.callback: self.callback(self)

    DocServer.base = BaseHTTPServer.HTTPServer
    DocServer.handler = DocHandler
    DocHandler.MessageClass = Message
    try:
        DocServer(port, callback).serve_until_quit()
    except (KeyboardInterrupt, select.error):
        pass
    print 'server stopped'

# ----------------------------------------------------- graphical interface

def gui():
    """Graphical interface (starts web server and pops up a control window)."""
    class GUI:
        def __init__(self, window, port=7464):
            self.window = window
            self.server = None
            self.scanner = None

            import Tkinter
            self.server_frm = Tkinter.Frame(window)
            self.title_lbl = Tkinter.Label(self.server_frm,
                text='Starting server...\n ')
            self.open_btn = Tkinter.Button(self.server_frm,
                text='open browser', command=self.open, state='disabled')
            self.quit_btn = Tkinter.Button(self.server_frm,
                text='quit serving', command=self.quit, state='disabled')

            self.search_frm = Tkinter.Frame(window)
            self.search_lbl = Tkinter.Label(self.search_frm, text='Search for')
            self.search_ent = Tkinter.Entry(self.search_frm)
            self.search_ent.bind('<Return>', self.search)
            self.stop_btn = Tkinter.Button(self.search_frm,
                text='stop', pady=0, command=self.stop, state='disabled')
            if sys.platform == 'win32':
                # Trying to hide and show this button crashes under Windows.
                self.stop_btn.pack(side='right')

            self.window.title('pydoc')
            self.window.protocol('WM_DELETE_WINDOW', self.quit)
            self.title_lbl.pack(side='top', fill='x')
            self.open_btn.pack(side='left', fill='x', expand=1)
            self.quit_btn.pack(side='right', fill='x', expand=1)
            self.server_frm.pack(side='top', fill='x')

            self.search_lbl.pack(side='left')
            self.search_ent.pack(side='right', fill='x', expand=1)
            self.search_frm.pack(side='top', fill='x')
            self.search_ent.focus_set()

            font = ('helvetica', sys.platform == 'win32' and 8 or 10)
            self.result_lst = Tkinter.Listbox(window, font=font, height=6)
            self.result_lst.bind('<Button-1>', self.select)
            self.result_lst.bind('<Double-Button-1>', self.goto)
            self.result_scr = Tkinter.Scrollbar(window,
                orient='vertical', command=self.result_lst.yview)
            self.result_lst.config(yscrollcommand=self.result_scr.set)

            self.result_frm = Tkinter.Frame(window)
            self.goto_btn = Tkinter.Button(self.result_frm,
                text='go to selected', command=self.goto)
            self.hide_btn = Tkinter.Button(self.result_frm,
                text='hide results', command=self.hide)
            self.goto_btn.pack(side='left', fill='x', expand=1)
            self.hide_btn.pack(side='right', fill='x', expand=1)

            self.window.update()
            self.minwidth = self.window.winfo_width()
            self.minheight = self.window.winfo_height()
            self.bigminheight = (self.server_frm.winfo_reqheight() +
                                 self.search_frm.winfo_reqheight() +
                                 self.result_lst.winfo_reqheight() +
                                 self.result_frm.winfo_reqheight())
            self.bigwidth, self.bigheight = self.minwidth, self.bigminheight
            self.expanded = 0
            self.window.wm_geometry('%dx%d' % (self.minwidth, self.minheight))
            self.window.wm_minsize(self.minwidth, self.minheight)

            import threading
            threading.Thread(target=serve, args=(port, self.ready)).start()

        def ready(self, server):
            self.server = server
            self.title_lbl.config(
                text='Python documentation server at\n' + server.url)
            self.open_btn.config(state='normal')
            self.quit_btn.config(state='normal')

        def open(self, event=None, url=None):
            url = url or self.server.url
            try:
                import webbrowser
                webbrowser.open(url)
            except ImportError: # pre-webbrowser.py compatibility
                if sys.platform == 'win32':
                    os.system('start "%s"' % url)
                elif sys.platform == 'mac':
                    try:
                        import ic
                        ic.launchurl(url)
                    except ImportError: pass
                else:
                    rc = os.system('netscape -remote "openURL(%s)" &' % url)
                    if rc: os.system('netscape "%s" &' % url)

        def quit(self, event=None):
            if self.server:
                self.server.quit = 1
            self.window.quit()

        def search(self, event=None):
            key = self.search_ent.get()
            self.stop_btn.pack(side='right')
            self.stop_btn.config(state='normal')
            self.search_lbl.config(text='Searching for "%s"...' % key)
            self.search_ent.forget()
            self.search_lbl.pack(side='left')
            self.result_lst.delete(0, 'end')
            self.goto_btn.config(state='disabled')
            self.expand()

            import threading
            if self.scanner:
                self.scanner.quit = 1
            self.scanner = ModuleScanner()
            threading.Thread(target=self.scanner.run,
                             args=(key, self.update, self.done)).start()

        def update(self, path, modname, desc):
            if modname[-9:] == '.__init__':
                modname = modname[:-9] + ' (package)'
            self.result_lst.insert('end',
                modname + ' - ' + (desc or '(no description)'))

        def stop(self, event=None):
            if self.scanner:
                self.scanner.quit = 1
                self.scanner = None

        def done(self):
            self.scanner = None
            self.search_lbl.config(text='Search for')
            self.search_lbl.pack(side='left')
            self.search_ent.pack(side='right', fill='x', expand=1)
            if sys.platform != 'win32': self.stop_btn.forget()
            self.stop_btn.config(state='disabled')

        def select(self, event=None):
            self.goto_btn.config(state='normal')

        def goto(self, event=None):
            selection = self.result_lst.curselection()
            if selection:
                modname = split(self.result_lst.get(selection[0]))[0]
                self.open(url=self.server.url + modname + '.html')

        def collapse(self):
            if not self.expanded: return
            self.result_frm.forget()
            self.result_scr.forget()
            self.result_lst.forget()
            self.bigwidth = self.window.winfo_width()
            self.bigheight = self.window.winfo_height()
            self.window.wm_geometry('%dx%d' % (self.minwidth, self.minheight))
            self.window.wm_minsize(self.minwidth, self.minheight)
            self.expanded = 0

        def expand(self):
            if self.expanded: return
            self.result_frm.pack(side='bottom', fill='x')
            self.result_scr.pack(side='right', fill='y')
            self.result_lst.pack(side='top', fill='both', expand=1)
            self.window.wm_geometry('%dx%d' % (self.bigwidth, self.bigheight))
            self.window.wm_minsize(self.minwidth, self.bigminheight)
            self.expanded = 1

        def hide(self, event=None):
            self.stop()
            self.collapse()

    import Tkinter
    try:
        gui = GUI(Tkinter.Tk())
        Tkinter.mainloop()
    except KeyboardInterrupt:
        pass

# -------------------------------------------------- command-line interface

def ispath(x):
    return type(x) is types.StringType and find(x, os.sep) >= 0

def cli():
    """Command-line interface (looks at sys.argv to decide what to do)."""
    import getopt
    class BadUsage: pass

    # Scripts don't get the current directory in their path by default.
    sys.path.insert(0, '.')

    try:
        opts, args = getopt.getopt(sys.argv[1:], 'gk:p:w')
        writing = 0

        for opt, val in opts:
            if opt == '-g':
                gui()
                return
            if opt == '-k':
                apropos(val)
                return
            if opt == '-p':
                try:
                    port = int(val)
                except ValueError:
                    raise BadUsage
                def ready(server):
                    print 'server ready at %s' % server.url
                serve(port, ready)
                return
            if opt == '-w':
                writing = 1

        if not args: raise BadUsage
        for arg in args:
            try:
                if ispath(arg) and os.path.isfile(arg):
                    arg = importfile(arg)
                if writing:
                    if ispath(arg) and os.path.isdir(arg):
                        writedocs(arg)
                    else:
                        writedoc(arg)
                else:
                    man(arg)
            except ErrorDuringImport, value:
                print value

    except (getopt.error, BadUsage):
        cmd = sys.argv[0]
        print """pydoc - the Python documentation tool

%s <name> ...
    Show text documentation on something.  <name> may be the name of a
    function, module, or package, or a dotted reference to a class or
    function within a module or module in a package.  If <name> contains
    a '%s', it is used as the path to a Python source file to document.

%s -k <keyword>
    Search for a keyword in the synopsis lines of all available modules.

%s -p <port>
    Start an HTTP server on the given port on the local machine.

%s -g
    Pop up a graphical interface for serving and finding documentation.

%s -w <name> ...
    Write out the HTML documentation for a module to a file in the current
    directory.  If <name> contains a '%s', it is treated as a filename.
""" % (cmd, os.sep, cmd, cmd, cmd, cmd, os.sep)

if __name__ == '__main__': cli()
