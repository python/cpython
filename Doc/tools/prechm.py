"""
    Makes the necesary files to convert from plain html of
    Python 1.5 and 1.5.x Documentation to
    Microsoft HTML Help format version 1.1
    Doesn't change the html's docs.

    by hernan.foffani@iname.com
    no copyright and no responsabilities.

    modified by Dale Nagata for Python 1.5.2

    Renamed from make_chm.py to prechm.py, and checked into the Python
    project, 19-Apr-2002 by Tim Peters.  Assorted modifications by Tim
    and Fred Drake.  Obtained from Robin Dunn's .chm packaging of the
    Python 2.2 docs, at <http://alldunn.com/python/>.
"""

import sys
import os
from formatter import NullWriter, AbstractFormatter
from htmllib import HTMLParser
import getopt
import cgi

usage_mode = '''
Usage: prechm.py [-c] [-k] [-p] [-v 1.5[.x]] filename
    -c: does not build filename.hhc (Table of Contents)
    -k: does not build filename.hhk (Index)
    -p: does not build filename.hhp (Project File)
    -v 1.5[.x]: makes help for the python 1.5[.x] docs
        (default is python 1.5.2 docs)
'''

# Project file (*.hhp) template.  'arch' is the file basename (like
# the pythlp in pythlp.hhp); 'version' is the doc version number (like
# the 2.2 in Python 2.2).
# The magical numbers in the long line under [WINDOWS] set most of the
# user-visible features (visible buttons, tabs, etc).
# About 0x10384e:  This defines the buttons in the help viewer.  The
# following defns are taken from htmlhelp.h.  Not all possibilities
# actually work, and not all those that work are available from the Help
# Workshop GUI.  In particular, the Zoom/Font button works and is not
# available from the GUI.  The ones we're using are marked with 'x':
#
#    0x000002   Hide/Show   x
#    0x000004   Back        x
#    0x000008   Forward     x
#    0x000010   Stop
#    0x000020   Refresh
#    0x000040   Home        x
#    0x000080   Forward
#    0x000100   Back
#    0x000200   Notes
#    0x000400   Contents
#    0x000800   Locate      x
#    0x001000   Options     x
#    0x002000   Print       x
#    0x004000   Index
#    0x008000   Search
#    0x010000   History
#    0x020000   Favorites
#    0x040000   Jump 1
#    0x080000   Jump 2
#    0x100000   Zoom/Font   x
#    0x200000   TOC Next
#    0x400000   TOC Prev

project_template = '''
[OPTIONS]
Compiled file=%(arch)s.chm
Contents file=%(arch)s.hhc
Default Window=%(arch)s
Default topic=index.html
Display compile progress=No
Full text search stop list file=%(arch)s.stp
Full-text search=Yes
Index file=%(arch)s.hhk
Language=0x409
Title=Python %(version)s Documentation

[WINDOWS]
%(arch)s="Python %(version)s Documentation","%(arch)s.hhc","%(arch)s.hhk",\
"index.html","index.html",,,,,0x63520,220,0x10384e,[0,0,1024,768],,,,,,,0

[FILES]
'''

contents_header = '''\
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<HTML>
<HEAD>
<meta name="GENERATOR" content="Microsoft&reg; HTML Help Workshop 4.1">
<!-- Sitemap 1.0 -->
</HEAD><BODY>
<OBJECT type="text/site properties">
        <param name="Window Styles" value="0x801227">
        <param name="ImageType" value="Folder">
</OBJECT>
<UL>
'''

contents_footer = '''\
</UL></BODY></HTML>
'''

object_sitemap = '''\
<OBJECT type="text/sitemap">
    <param name="Name" value="%s">
    <param name="Local" value="%s">
</OBJECT>
'''

# List of words the full text search facility shouldn't index.  This
# becomes file ARCH.stp.  Note that this list must be pretty small!
# Different versions of the MS docs claim the file has a maximum size of
# 256 or 512 bytes (including \r\n at the end of each line).
# Note that "and", "or", "not" and "near" are operators in the search
# language, so no point indexing them even if we wanted to.
stop_list = '''
a  and  are  as  at
be  but  by
for
if  in  into  is  it
near  no  not
of  on  or
such
that  the  their  then  there  these  they  this  to
was  will  with
'''

# s is a string or None.  If None or empty, return None.  Else tack '.html'
# on to the end, unless it's already there.
def addhtml(s):
    if s:
        if not s.endswith('.html'):
            s += '.html'
    return s

# Convenience class to hold info about "a book" in HTMLHelp terms == a doc
# directory in Python terms.
class Book:
    def __init__(self, directory, title, firstpage,
                 contentpage=None, indexpage=None):
        self.directory   = directory
        self.title       = title
        self.firstpage   = addhtml(firstpage)
        self.contentpage = addhtml(contentpage)
        self.indexpage   = addhtml(indexpage)

# Library Doc list of books:
# each 'book' : (Dir, Title, First page, Content page, Index page)
supported_libraries = {
    '2.4':
    [
        Book('.', 'Main page', 'index'),
        Book('.', 'Global Module Index', 'modindex'),
        Book('whatsnew', "What's New", 'index', 'contents'),
        Book('tut','Tutorial','tut','node2'),
        Book('lib','Library Reference','lib','contents','genindex'),
        Book('ref','Language Reference','ref','contents','genindex'),
        Book('mac','Macintosh Reference','mac','contents','genindex'),
        Book('ext','Extending and Embedding','ext','contents'),
        Book('api','Python/C API','api','contents','genindex'),
        Book('doc','Documenting Python','doc','contents'),
        Book('inst','Installing Python Modules', 'inst', 'index'),
        Book('dist','Distributing Python Modules', 'dist', 'index', 'genindex'),
    ],

    '2.3':
    [
        Book('.', 'Main page', 'index'),
        Book('.', 'Global Module Index', 'modindex'),
        Book('whatsnew', "What's New", 'index', 'contents'),
        Book('tut','Tutorial','tut','node2'),
        Book('lib','Library Reference','lib','contents','genindex'),
        Book('ref','Language Reference','ref','contents','genindex'),
        Book('mac','Macintosh Reference','mac','contents','genindex'),
        Book('ext','Extending and Embedding','ext','contents'),
        Book('api','Python/C API','api','contents','genindex'),
        Book('doc','Documenting Python','doc','contents'),
        Book('inst','Installing Python Modules', 'inst', 'index'),
        Book('dist','Distributing Python Modules', 'dist', 'index'),
    ],

    '2.2':
    [
        Book('.', 'Main page', 'index'),
        Book('.', 'Global Module Index', 'modindex'),
        Book('whatsnew', "What's New", 'index', 'contents'),
        Book('tut','Tutorial','tut','node2'),
        Book('lib','Library Reference','lib','contents','genindex'),
        Book('ref','Language Reference','ref','contents','genindex'),
        Book('mac','Macintosh Reference','mac','contents','genindex'),
        Book('ext','Extending and Embedding','ext','contents'),
        Book('api','Python/C API','api','contents','genindex'),
        Book('doc','Documenting Python','doc','contents'),
        Book('inst','Installing Python Modules', 'inst', 'index'),
        Book('dist','Distributing Python Modules', 'dist', 'index'),
    ],

    '2.1.1':
    [
        Book('.', 'Main page', 'index'),
        Book('.', 'Global Module Index', 'modindex'),
        Book('tut','Tutorial','tut','node2'),
        Book('lib','Library Reference','lib','contents','genindex'),
        Book('ref','Language Reference','ref','contents','genindex'),
        Book('mac','Macintosh Reference','mac','contents','genindex'),
        Book('ext','Extending and Embedding','ext','contents'),
        Book('api','Python/C API','api','contents','genindex'),
        Book('doc','Documenting Python','doc','contents'),
        Book('inst','Installing Python Modules', 'inst', 'index'),
        Book('dist','Distributing Python Modules', 'dist', 'index'),
    ],

    '2.0.0':
    [
        Book('.', 'Global Module Index', 'modindex'),
        Book('tut','Tutorial','tut','node2'),
        Book('lib','Library Reference','lib','contents','genindex'),
        Book('ref','Language Reference','ref','contents','genindex'),
        Book('mac','Macintosh Reference','mac','contents','genindex'),
        Book('ext','Extending and Embedding','ext','contents'),
        Book('api','Python/C API','api','contents','genindex'),
        Book('doc','Documenting Python','doc','contents'),
        Book('inst','Installing Python Modules', 'inst', 'contents'),
        Book('dist','Distributing Python Modules', 'dist', 'contents'),
    ],

    # <dnagata@creo.com> Apr 17/99: library for 1.5.2 version:
    # <hernan.foffani@iname.com> May 01/99: library for 1.5.2 (04/30/99):
    '1.5.2':
    [
        Book('tut','Tutorial','tut','node2'),
        Book('lib','Library Reference','lib','contents','genindex'),
        Book('ref','Language Reference','ref','contents','genindex'),
        Book('mac','Macintosh Reference','mac','contents','genindex'),
        Book('ext','Extending and Embedding','ext','contents'),
        Book('api','Python/C API','api','contents','genindex'),
        Book('doc','Documenting Python','doc','contents')
    ],

    # library for 1.5.1 version:
    '1.5.1':
    [
        Book('tut','Tutorial','tut','contents'),
        Book('lib','Library Reference','lib','contents','genindex'),
        Book('ref','Language Reference','ref-1','ref-2','ref-11'),
        Book('ext','Extending and Embedding','ext','contents'),
        Book('api','Python/C API','api','contents','genindex')
    ],

    # library for 1.5 version:
    '1.5':
    [
        Book('tut','Tutorial','tut','node1'),
        Book('lib','Library Reference','lib','node1','node268'),
        Book('ref','Language Reference','ref-1','ref-2','ref-11'),
        Book('ext','Extending and Embedding','ext','node1'),
        Book('api','Python/C API','api','node1','node48')
    ]
}

# AlmostNullWriter doesn't print anything; it just arranges to save the
# text sent to send_flowing_data().  This is used to capture the text
# between an anchor begin/end pair, e.g. for TOC entries.

class AlmostNullWriter(NullWriter):

    def __init__(self):
        NullWriter.__init__(self)
        self.saved_clear()

    def send_flowing_data(self, data):
        stripped = data.strip()
        if stripped:    # don't bother to save runs of whitespace
            self.saved.append(stripped)

    # Forget all saved text.
    def saved_clear(self):
        self.saved = []

    # Return all saved text as a string.
    def saved_get(self):
        return ' '.join(self.saved)

class HelpHtmlParser(HTMLParser):

    def __init__(self, formatter, path, output):
        HTMLParser.__init__(self, formatter)
        self.path = path    # relative path
        self.ft = output    # output file
        self.indent = 0     # number of tabs for pretty printing of files
        self.proc = False   # True when actively processing, else False
                            # (headers, footers, etc)
        # XXX This shouldn't need to be a stack -- anchors shouldn't nest.
        # XXX See SF bug <http://www.python.org/sf/546579>.
        self.hrefstack = [] # stack of hrefs from anchor begins

    def begin_group(self):
        self.indent += 1
        self.proc = True

    def finish_group(self):
        self.indent -= 1
        # stop processing when back to top level
        self.proc = self.indent > 0

    def anchor_bgn(self, href, name, type):
        if self.proc:
            # XXX See SF bug <http://www.python.org/sf/546579>.
            # XXX index.html for the 2.2.1 language reference manual contains
            # XXX nested <a></a> tags in the entry for the section on blank
            # XXX lines.  We want to ignore the nested part completely.
            if len(self.hrefstack) == 0:
                self.saved_clear()
                self.hrefstack.append(href)

    def anchor_end(self):
        if self.proc:
            # XXX See XXX above.
            if self.hrefstack:
                title = cgi.escape(self.saved_get(), True)
                path = self.path + '/' + self.hrefstack.pop()
                self.tab(object_sitemap % (title, path))

    def start_dl(self, atr_val):
        self.begin_group()

    def end_dl(self):
        self.finish_group()

    def do_dt(self, atr_val):
        # no trailing newline on purpose!
        self.tab("<LI>")

    # Write text to output file.
    def write(self, text):
        self.ft.write(text)

    # Write text to output file after indenting by self.indent tabs.
    def tab(self, text=''):
        self.write('\t' * self.indent)
        if text:
            self.write(text)

    # Forget all saved text.
    def saved_clear(self):
        self.formatter.writer.saved_clear()

    # Return all saved text as a string.
    def saved_get(self):
        return self.formatter.writer.saved_get()

class IdxHlpHtmlParser(HelpHtmlParser):
    # nothing special here, seems enough with parent class
    pass

class TocHlpHtmlParser(HelpHtmlParser):

    def start_dl(self, atr_val):
        self.begin_group()
        self.tab('<UL>\n')

    def end_dl(self):
        self.finish_group()
        self.tab('</UL>\n')

    def start_ul(self, atr_val):
        self.begin_group()
        self.tab('<UL>\n')

    def end_ul(self):
        self.finish_group()
        self.tab('</UL>\n')

    def do_li(self, atr_val):
        # no trailing newline on purpose!
        self.tab("<LI>")

def index(path, indexpage, output):
    parser = IdxHlpHtmlParser(AbstractFormatter(AlmostNullWriter()),
                              path, output)
    f = open(path + '/' + indexpage)
    parser.feed(f.read())
    parser.close()
    f.close()

def content(path, contentpage, output):
    parser = TocHlpHtmlParser(AbstractFormatter(AlmostNullWriter()),
                              path, output)
    f = open(path + '/' + contentpage)
    parser.feed(f.read())
    parser.close()
    f.close()

def do_index(library, output):
    output.write('<UL>\n')
    for book in library:
        print '\t', book.title, '-', book.indexpage
        if book.indexpage:
            index(book.directory, book.indexpage, output)
    output.write('</UL>\n')

def do_content(library, version, output):
    output.write(contents_header)
    for book in library:
        print '\t', book.title, '-', book.firstpage
        path = book.directory + "/" + book.firstpage
        output.write('<LI>')
        output.write(object_sitemap % (book.title, path))
        if book.contentpage:
            content(book.directory, book.contentpage, output)
    output.write(contents_footer)

# Fill in the [FILES] section of the project (.hhp) file.
# 'library' is the list of directory description tuples from
# supported_libraries for the version of the docs getting generated.
def do_project(library, output, arch, version):
    output.write(project_template % locals())
    pathseen = {}
    for book in library:
        directory = book.directory
        path = directory + '\\%s\n'
        for page in os.listdir(directory):
            if page.endswith('.html') or page.endswith('.css'):
                fullpath = path % page
                if fullpath not in pathseen:
                    output.write(fullpath)
                    pathseen[fullpath] = True

def openfile(file):
    try:
        p = open(file, "w")
    except IOError, msg:
        print file, ":", msg
        sys.exit(1)
    return p

def usage():
    print usage_mode
    sys.exit(0)

def do_it(args = None):
    if not args:
        args = sys.argv[1:]

    if not args:
        usage()

    try:
        optlist, args = getopt.getopt(args, 'ckpv:')
    except getopt.error, msg:
        print msg
        usage()

    if not args or len(args) > 1:
        usage()
    arch = args[0]

    version = None
    for opt in optlist:
        if opt[0] == '-v':
            version = opt[1]
            break
    if not version:
        usage()

    library = supported_libraries[version]

    if not (('-p','') in optlist):
        fname = arch + '.stp'
        f = openfile(fname)
        print "Building stoplist", fname, "..."
        words = stop_list.split()
        words.sort()
        for word in words:
            print >> f, word
        f.close()

        f = openfile(arch + '.hhp')
        print "Building Project..."
        do_project(library, f, arch, version)
        if version == '2.0.0':
            for image in os.listdir('icons'):
                f.write('icons'+ '\\' + image + '\n')

        f.close()

    if not (('-c','') in optlist):
        f = openfile(arch + '.hhc')
        print "Building Table of Content..."
        do_content(library, version, f)
        f.close()

    if not (('-k','') in optlist):
        f = openfile(arch + '.hhk')
        print "Building Index..."
        do_index(library, f)
        f.close()

if __name__ == '__main__':
    do_it()
