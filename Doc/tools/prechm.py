'''
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
'''

import sys
import os
import formatter
import htmllib
import string
import getopt



# moved all the triple_quote up here because my syntax-coloring editor
# sucks a little bit.
usage_mode = '''
Usage: make_chm.py [-c] [-k] [-p] [-v 1.5[.x]] filename
    -c: does not build filename.hhc (Table of Contents)
    -k: does not build filename.hhk (Index)
    -p: does not build filename.hhp (Project File)
    -v 1.5[.x]: makes help for the python 1.5[.x] docs
        (default is python 1.5.2 docs)
'''

# project file (*.hhp) template. there are seven %s
project_template = '''
[OPTIONS]
Compatibility=1.1
Compiled file=%s.chm
Contents file=%s.hhc
Default Window=%s
Default topic=index.html
Display compile progress=No
Full-text search=Yes
Index file=%s.hhk
Language=0x409
Title=Python %s Documentation

[WINDOWS]
%s="Python %s Documentation","%s.hhc","%s.hhk","index.html","index.html",\
,,,,0x63520,220,0x384e,[271,372,740,718],,,,,,,0

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

# Library Doc list of tuples:
# each 'book' : ( Dir, Title, First page, Content page, Index page)
#
supported_libraries = {
    '2.2':  ### Beta!!!  fix for actual release
    [
        ('.', 'Global Module Index', 'modindex.html', None, None),
        ('tut','Tutorial','tut.html','node2.html',None),
        ('lib','Library Reference','lib.html','contents.html','genindex.html'),
        ('ref','Language Reference','ref.html','contents.html','genindex.html'),
        ('mac','Macintosh Reference','mac.html','contents.html','genindex.html'),
        ('ext','Extending and Embedding','ext.html','contents.html',None),
        ('api','Python/C API','api.html','contents.html','genindex.html'),
        ('doc','Documenting Python','doc.html','contents.html',None),
        ('inst','Installing Python Modules', 'inst.html', 'index.html', None),
        ('dist','Distributing Python Modules', 'dist.html', 'index.html', None),
    ],

    '2.1.1':
    [
        ('.', 'Global Module Index', 'modindex.html', None, None),
        ('tut','Tutorial','tut.html','node2.html',None),
        ('lib','Library Reference','lib.html','contents.html','genindex.html'),
        ('ref','Language Reference','ref.html','contents.html','genindex.html'),
        ('mac','Macintosh Reference','mac.html','contents.html','genindex.html'),
        ('ext','Extending and Embedding','ext.html','contents.html',None),
        ('api','Python/C API','api.html','contents.html','genindex.html'),
        ('doc','Documenting Python','doc.html','contents.html',None),
        ('inst','Installing Python Modules', 'inst.html', 'index.html', None),
        ('dist','Distributing Python Modules', 'dist.html', 'index.html', None),
    ],

    '2.0.0':
    [
        ('.', 'Global Module Index', 'modindex.html', None, None),
        ('tut','Tutorial','tut.html','node2.html',None),
        ('lib','Library Reference','lib.html','contents.html','genindex.html'),
        ('ref','Language Reference','ref.html','contents.html','genindex.html'),
        ('mac','Macintosh Reference','mac.html','contents.html','genindex.html'),
        ('ext','Extending and Embedding','ext.html','contents.html',None),
        ('api','Python/C API','api.html','contents.html','genindex.html'),
        ('doc','Documenting Python','doc.html','contents.html',None),
        ('inst','Installing Python Modules', 'inst.html', 'contents.html', None),
        ('dist','Distributing Python Modules', 'dist.html', 'contents.html', None),
    ],

    # <dnagata@creo.com> Apr 17/99: library for 1.5.2 version:
    # <hernan.foffani@iname.com> May 01/99: library for 1.5.2 (04/30/99):
    '1.5.2':
    [
        ('tut','Tutorial','tut.html','node2.html',None),
        ('lib','Library Reference','lib.html','contents.html','genindex.html'),
        ('ref','Language Reference','ref.html','contents.html','genindex.html'),
        ('mac','Macintosh Reference','mac.html','contents.html','genindex.html'),
        ('ext','Extending and Embedding','ext.html','contents.html',None),
        ('api','Python/C API','api.html','contents.html','genindex.html'),
        ('doc','Documenting Python','doc.html','contents.html',None)
    ],

    # library for 1.5.1 version:
    '1.5.1':
    [
        ('tut','Tutorial','tut.html','contents.html',None),
        ('lib','Library Reference','lib.html','contents.html','genindex.html'),
        ('ref','Language Reference','ref-1.html','ref-2.html','ref-11.html'),
        ('ext','Extending and Embedding','ext.html','contents.html',None),
        ('api','Python/C API','api.html','contents.html','genindex.html')
    ],

    # library for 1.5 version:
    '1.5':
    [
        ('tut','Tutorial','tut.html','node1.html',None),
        ('lib','Library Reference','lib.html','node1.html','node268.html'),
        ('ref','Language Reference','ref-1.html','ref-2.html','ref-11.html'),
        ('ext','Extending and Embedding','ext.html','node1.html',None),
        ('api','Python/C API','api.html','node1.html','node48.html')
    ]
}

class AlmostNullWriter(formatter.NullWriter) :
    savedliteral = ''

    def send_flowing_data(self, data) :
        # need the text tag for later
        datastriped = string.strip(data)
        if self.savedliteral == '':
            self.savedliteral = datastriped
        else:
            self.savedliteral = string.strip(self.savedliteral +
                                            ' ' + datastriped)


class HelpHtmlParser(htmllib.HTMLParser) :
    indent = 0  # number of tabs for pritty printing of files
    ft = None   # output file
    path = None # relative path
    proc = 0    # if true I process, if false I skip
                #   (some headers, footers, etc.)

    def begin_group(self) :
        if not self.proc :
            # first level, start processing
            self.proc = 1
        self.indent = self.indent + 1

    def finnish_group(self) :
        self.indent = self.indent - 1
        if self.proc and self.indent == 0 :
            # if processing and back to root, then stop
            self.proc = 0

    def anchor_bgn(self, href, name, type) :
        if self.proc :
            self.formatter.writer.savedliteral = ''
            self.ft.write('<OBJECT type="text/sitemap">\n')
            self.ft.write('\t' * self.indent + \
                '\t<param name="Local" value="' + self.path + \
                '/' + href + '">\n')

    def anchor_end(self) :
        if self.proc :
            self.ft.write('\t' * self.indent + \
                '\t<param name="Name" value="' + \
                self.formatter.writer.savedliteral + '">\n')
            self.ft.write('\t' * self.indent + '\t</OBJECT>\n' )

    def start_dl(self, atr_val) :
        self.begin_group()

    def end_dl(self) :
        self.finnish_group()

    def do_dt(self, atr_val) :
        # no trailing newline on pourpose!
        self.ft.write("\t" * self.indent + "<LI>")


class IdxHlpHtmlParser(HelpHtmlParser) :
    # nothing special here, seems enough with parent class
    pass

class TocHlpHtmlParser(HelpHtmlParser) :

    def start_dl(self, atr_val) :
        self.begin_group()
        self.ft.write('\t' * self.indent + '<UL>\n')

    def end_dl(self) :
        self.finnish_group()
        self.ft.write('</UL>\n')

    def start_ul(self, atr_val) :
        self.begin_group()
        self.ft.write('\t' * self.indent + '<UL>\n')

    def end_ul(self) :
        self.finnish_group()
        self.ft.write('</UL>\n')

    def do_li(self, atr_val) :
        # no trailing newline on pourpose!
        self.ft.write("\t" * self.indent + "<LI>")


def index(path, archivo, output) :
    f = formatter.AbstractFormatter(AlmostNullWriter())
    parser = IdxHlpHtmlParser(f)
    parser.path = path
    parser.ft = output
    fil = path + '/' + archivo
    parser.feed(open(fil).read())
    parser.close()


def content(path, archivo, output) :
    f = formatter.AbstractFormatter(AlmostNullWriter())
    parser = TocHlpHtmlParser(f)
    parser.path = path
    parser.ft = output
    fil = path + '/' + archivo
    parser.feed(open(fil).read())
    parser.close()


def do_index(library, output) :
    output.write('<UL>\n')
    for book in library :
        print '\t', book[2]
        if book[4] :
            index(book[0], book[4], output)
    output.write('</UL>\n')


def do_content(library, version, output) :
    output.write(contents_header % version)
    for book in library :
        print '\t', book[2]
        output.write(object_sitemap % (book[0]+"/"+book[2], book[1]))
        if book[3] :
            content(book[0], book[3], output)
    output.write(contents_footer)


def do_project( library, output, arch, version) :
    output.write( project_template % \
            (arch, arch, arch, arch, version, arch, version, arch, arch) )
    for book in library :
        for page in os.listdir(book[0]) :
            if page[string.rfind(page, '.'):] == '.html' or \
               page[string.rfind(page, '.'):] == '.css':
                output.write(book[0]+ '\\' + page + '\n')


def openfile(file) :
    try :
        p = open(file, "w")
    except IOError, msg :
        print file, ":", msg
        sys.exit(1)
    return p

def usage() :
        print usage_mode
        sys.exit(0)



def do_it(args = None) :
    if not args :
        args = sys.argv[1:]

    if not args :
        usage()

    try :
        optlist, args = getopt.getopt(args, 'ckpv:')
    except getopt.error, msg :
        print msg
        usage()

    if not args or len(args) > 1 :
        usage()
    arch = args[0]

    version = None
    for opt in optlist:
        if opt[0] == '-v':
            version = opt[1]
            break
    if not version:
        usage()

    library = supported_libraries[ version ]

    if not (('-p','') in optlist) :
        f = openfile(arch + '.hhp')
        print "Building Project..."
        do_project(library, f, arch, version)
        if version == '2.0.0':
            for image in os.listdir('icons'):
                f.write('icons'+ '\\' + image + '\n')

        f.close()

    if not (('-c','') in optlist) :
        f = openfile(arch + '.hhc')
        print "Building Table of Content..."
        do_content(library, version, f)
        f.close()

    if not (('-k','') in optlist) :
        f = openfile(arch + '.hhk')
        print "Building Index..."
        do_index(library, f)
        f.close()

if __name__ == '__main__' :
    do_it()


