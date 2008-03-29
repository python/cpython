# Build and install an Apple Help Viewer compatible version of the Python
# documentation into the framework.
# Code by Bill Fancher, with some modifications by Jack Jansen.
#
# You must run this as a two-step process
# 1. python setupDocs.py build
# 2. Wait for Apple Help Indexing Tool to finish
# 3. python setupDocs.py install
#
# To do:
# - test whether the docs are available locally before downloading
# - fix buildDocsFromSource
# - Get documentation version from sys.version, fallback to 2.2.1
# - See if we can somehow detect that Apple Help Indexing Tool is finished
# - data_files to setup() doesn't seem the right way to pass the arguments
#
import sys, os, re
from distutils.cmd import Command
from distutils.command.build import build
from distutils.core import setup
from distutils.file_util import copy_file
from distutils.dir_util import copy_tree
from distutils.log import log
from distutils.spawn import spawn
from distutils import sysconfig, dep_util
from distutils.util import change_root
import HelpIndexingTool
import Carbon.File
import time

MAJOR_VERSION='2.4'
MINOR_VERSION='2.4.1'
DESTDIR='/Applications/MacPython-%s/PythonIDE.app/Contents/Resources/English.lproj/PythonDocumentation' % MAJOR_VERSION

class DocBuild(build):
    def initialize_options(self):
        build.initialize_options(self)
        self.build_html = None
        self.build_dest = None
        self.download = 1
        self.doc_version = MINOR_VERSION # Only needed if download is true

    def finalize_options(self):
        build.finalize_options(self)
        if self.build_html is None:
            self.build_html = os.path.join(self.build_base, 'html')
        if self.build_dest is None:
            self.build_dest = os.path.join(self.build_base, 'PythonDocumentation')

    def spawn(self, *args):
        spawn(args, 1,  self.verbose, self.dry_run)

    def downloadDocs(self):
        workdir = os.getcwd()
        # XXX Note: the next strings may change from version to version
        url = 'http://www.python.org/ftp/python/doc/%s/html-%s.tar.bz2' % \
                (self.doc_version,self.doc_version)
        tarfile = 'html-%s.tar.bz2' % self.doc_version
        dirname = 'Python-Docs-%s' % self.doc_version

        if os.path.exists(self.build_html):
            raise RuntimeError, '%s: already exists, please remove and try again' % self.build_html
        os.chdir(self.build_base)
        self.spawn('curl','-O', url)
        self.spawn('tar', '-xjf', tarfile)
        os.rename(dirname, 'html')
        os.chdir(workdir)
##        print "** Please unpack %s" % os.path.join(self.build_base, tarfile)
##        print "** Unpack the files into %s" % self.build_html
##        raise RuntimeError, "You need to unpack the docs manually"

    def buildDocsFromSource(self):
        srcdir = '../../..'
        docdir = os.path.join(srcdir, 'Doc')
        htmldir = os.path.join(docdir, 'html')
        spawn(('make','--directory', docdir, 'html'), 1, self.verbose, self.dry_run)
        self.mkpath(self.build_html)
        copy_tree(htmldir, self.build_html)

    def ensureHtml(self):
        if not os.path.exists(self.build_html):
            if self.download:
                self.downloadDocs()
            else:
                self.buildDocsFromSource()

    def hackIndex(self):
        ind_html = 'index.html'
        #print 'self.build_dest =', self.build_dest
        hackedIndex = file(os.path.join(self.build_dest, ind_html),'w')
        origIndex = file(os.path.join(self.build_html,ind_html))
        r = re.compile('<style type="text/css">.*</style>', re.DOTALL)
        hackedIndex.write(r.sub('<META NAME="AppleTitle" CONTENT="Python Documentation">',origIndex.read()))

    def hackFile(self,d,f):
        origPath = os.path.join(d,f)
        assert(origPath[:len(self.build_html)] == self.build_html)
        outPath = os.path.join(self.build_dest, d[len(self.build_html)+1:], f)
        (name, ext) = os.path.splitext(f)
        if os.path.isdir(origPath):
            self.mkpath(outPath)
        elif ext == '.html':
            if self.verbose: print 'hacking %s to %s' % (origPath,outPath)
            hackedFile = file(outPath, 'w')
            origFile = file(origPath,'r')
            hackedFile.write(self.r.sub('<dl><dt><dd>', origFile.read()))
        else:
            copy_file(origPath, outPath)

    def hackHtml(self):
        self.r = re.compile('<dl><dd>')
        os.path.walk(self.build_html, self.visit, None)

    def visit(self, dummy, dirname, filenames):
        for f in filenames:
            self.hackFile(dirname, f)

    def makeHelpIndex(self):
        app = '/Developer/Applications/Apple Help Indexing Tool.app'
        self.spawn('open', '-a', app , self.build_dest)
        print "Please wait until Apple Help Indexing Tool finishes before installing"

    def makeHelpIndex(self):
        app = HelpIndexingTool.HelpIndexingTool(start=1)
        app.open(Carbon.File.FSSpec(self.build_dest))
        sys.stderr.write("Waiting for Help Indexing Tool to start...")
        while 1:
            # This is bad design in the suite generation code!
            idle = app._get(HelpIndexingTool.Help_Indexing_Tool_Suite._Prop_idleStatus())
            time.sleep(10)
            if not idle: break
            sys.stderr.write(".")
        sys.stderr.write("\n")
        sys.stderr.write("Waiting for Help Indexing Tool to finish...")
        while 1:
            # This is bad design in the suite generation code!
            idle = app._get(HelpIndexingTool.Help_Indexing_Tool_Suite._Prop_idleStatus())
            time.sleep(10)
            if idle: break
            sys.stderr.write(".")
        sys.stderr.write("\n")


    def run(self):
        self.ensure_finalized()
        self.mkpath(self.build_base)
        self.ensureHtml()
        if not os.path.isdir(self.build_html):
            raise RuntimeError, \
            "Can't find source folder for documentation."
        self.mkpath(self.build_dest)
        if dep_util.newer(os.path.join(self.build_html,'index.html'), os.path.join(self.build_dest,'index.html')):
            self.mkpath(self.build_dest)
            self.hackHtml()
            self.hackIndex()
            self.makeHelpIndex()

class AHVDocInstall(Command):
    description = "install Apple Help Viewer html files"
    user_options = [('install-doc=', 'd',
            'directory to install HTML tree'),
             ('root=', None,
             "install everything relative to this alternate root directory"),
            ]

    def initialize_options(self):
        self.build_dest = None
        self.install_doc = None
        self.prefix = None
        self.root = None

    def finalize_options(self):
        self.set_undefined_options('install',
                ('prefix', 'prefix'),
                ('root', 'root'))
#               import pdb ; pdb.set_trace()
        build_cmd = self.get_finalized_command('build')
        if self.build_dest is None:
            build_cmd = self.get_finalized_command('build')
            self.build_dest = build_cmd.build_dest
        if self.install_doc is None:
            self.install_doc = os.path.join(self.prefix, DESTDIR)
        print 'INSTALL', self.build_dest, '->', self.install_doc

    def run(self):
        self.finalize_options()
        self.ensure_finalized()
        print "Running Installer"
        instloc = self.install_doc
        if self.root:
            instloc = change_root(self.root, instloc)
        self.mkpath(instloc)
        copy_tree(self.build_dest, instloc)
        print "Installation complete"

def mungeVersion(infile, outfile):
    i = file(infile,'r')
    o = file(outfile,'w')
    o.write(re.sub('\$\(VERSION\)',sysconfig.get_config_var('VERSION'),i.read()))
    i.close()
    o.close()

def main():
    # turn off warnings when deprecated modules are imported
##      import warnings
##      warnings.filterwarnings("ignore",category=DeprecationWarning)
    setup(name = 'Documentation',
            version = '%d.%d' % sys.version_info[:2],
            cmdclass = {'install_data':AHVDocInstall, 'build':DocBuild},
            data_files = ['dummy'],
            )

if __name__ == '__main__':
    main()
