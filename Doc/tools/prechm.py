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
import string
import getopt

usage_mode = '''
Usage: make_chm.py [-c] [-k] [-p] [-v 1.5[.x]] filename
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
"index.html","index.html",,,,,0x63520,220,0x384e,[271,372,740,718],,,,,,,0

[FILES]
'''

contents_header = '''
<OBJECT type="text/site properties">
	<param name="Window Styles" value="0x801227">
	<param name="ImageType" value="Folder">
</OBJECT>
<UL>
<LI> <OBJECT type="text/sitemap">
	<param name="Name" value="Python %s Docs">
	<param name="Local" value="./index.html">
	</OBJECT>
<UL>
'''

contents_footer = '''
</UL></UL>
'''

object_sitemap = '''
    <LI> <OBJECT type="text/sitemap">
        <param name="Local" value="%s">
        <param name="Name" value="%s">
        </OBJECT>
'''


# List of words the full text search facility shouldn't index.  This
# becomes file ARCH.stp.  Note that this list must be pretty small!
# Different versions of the MS docs claim the file has a maximum size of
# 256 or 512 bytes (including \r\n at the end of each line).
# Note that "and", "or", "not" and "near" are operators in the search
# language, so no point indexing them even if we wanted to.
stop_list = '''
a  an  and
is
near
not
of
or
the
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
    '2.2':  ### Beta!!!  fix for actual release
    [
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

    def begin_group(self):
        self.indent += 1
        self.proc = True

    def finish_group(self):
        self.indent -= 1
        # stop processing when back to top level
        self.proc = self.indent > 0

    def anchor_bgn(self, href, name, type):
        if self.proc:
            self.saved_clear()
            self.write('<OBJECT type="text/sitemap">\n')
            self.tab('\t<param name="Local" value="%s/%s">\n' %
                     (self.path, href))

    def anchor_end(self):
        if self.proc:
            self.tab('\t<param name="Name" value="%s">\n' % self.saved_get())
            self.tab('\t</OBJECT>\n')

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
    output.write(contents_header % version)
    for book in library:
        print '\t', book.title, '-', book.firstpage
        output.write(object_sitemap % (book.directory + "/" + book.firstpage,
                                       book.title))
        if book.contentpage:
            content(book.directory, book.contentpage, output)
    output.write(contents_footer)

# Fill in the [FILES] section of the project (.hhp) file.
# 'library' is the list of directory description tuples from
# supported_libraries for the version of the docs getting generated.
def do_project(library, output, arch, version):
    output.write(project_template % locals())
    for book in library:
        directory = book.directory
        path = directory + '\\%s\n'
        for page in os.listdir(directory):
            if page.endswith('.html') or page.endswith('.css'):
                output.write(path % page)

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
