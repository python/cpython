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

def visit(arg, d,l):
	for f in l:
		arg.hackFile(d,f)
		
class DocBuild(build):
	def initialize_options(self):
		build.initialize_options(self)
		self.doc_dir = None
		self.base_dir = None
		self.build_dir = None
		self.download = 1
		self.doc_version = '2.2.1'
		
	def finalize_options(self):
		build.finalize_options(self)
		if self.build_dir is None:
			self.build_dir = self.build_temp
		if self.doc_dir is None:
			self.doc_dir = data_info = self.distribution.data_files[0][1][0]
		self.build_dest = os.path.abspath(os.path.join(self.build_dir,self.doc_dir))
		#print 'DocBuild.finalize_options:\n  build_dest = %s,\n  doc_dir = %s' % (self.build_dest, self.doc_dir)
	
	def spawn(self, *args):
		spawn(args, 1,  self.verbose, self.dry_run)
	
	def downloadDocs(self):
		workdir = os.getcwd()
		self.spawn('curl','-O', 'http://www.python.org/ftp/python/doc/%s/html-%s.tgz' % (self.doc_version,self.doc_version))
		self.mkpath(self.doc_dir)
		os.chdir(self.doc_dir)
		self.spawn('tar', '-xzf', '../html-%s.tgz' % self.doc_version)
		os.chdir(workdir);
		
	def buildDocsFromSouce(self):
		#Totally untested
		spawn(('make','--directory', '../Docs'), 1, self.verbose, self.dry_run)
		copy_tree('../Docs/html', self.doc_dir)
		
	def ensureHtml(self):
		if not os.path.exists(self.doc_dir):
			if self.download:
				self.downloadDocs()
			else:
				self.buildDocsFromSource()
				
	def hackIndex(self):
		ind_html = 'index.html'
		#print 'self.build_dest =', self.build_dest
		hackedIndex = file(os.path.join(self.build_dest, ind_html),'w')
		origIndex = file(os.path.join(self.doc_dir,ind_html))
		r = re.compile('<style type="text/css">.*</style>', re.DOTALL)
		hackedIndex.write(r.sub('<META NAME="AppleTitle" CONTENT="Python Help">',origIndex.read()))
	
	def hackFile(self,d,f):
		origPath = os.path.join(d,f)
		outPath = os.path.join(self.build_dir, d, f)
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
		os.path.walk(self.doc_dir, visit, self)
			
	def makeHelpIndex(self):
		app = '/Developer/Applications/Apple Help Indexing Tool.app'
		self.spawn('open', '-a', app , self.build_dest)
		print "Please wait until Apple Help Indexing Tool finishes before installing"
		
	def run(self):
		self.ensure_finalized()
		self.ensureHtml()
		data_info = self.distribution.data_files[0][1]
		if not os.path.isdir(self.doc_dir):
			raise RuntimeError, \
  			"Can't find source folder for documentation."
		if dep_util.newer(os.path.join(self.doc_dir,'index.html'), os.path.join(self.build_dest,'index.html')):
			self.mkpath(self.build_dest)
			self.hackHtml()
			self.hackIndex()
			self.makeHelpIndex()

class DirInstall(Command):
	def initialize_options(self):
		self.build_dest = None
		self.install_dir = None
			
	def finalize_options(self):	
		build_cmd = self.get_finalized_command('build')
		if self.build_dest == None:
			self.build_dest = build_cmd.build_dest
		if self.install_dir == None:
			self.install_dir = self.distribution.data_files[0][0]
		print self.build_dest, self.install_dir
		
	def run(self):
		self.finalize_options()
		self.ensure_finalized()
		print "Running Installer"
		data_info = self.distribution.data_files[0][1]
		# spawn('pax','-r', '-w', base_dir, 
		print self.__dict__
		self.mkpath(self.install_dir)
		# The Python way
		copy_tree(self.build_dest, self.install_dir)
		# The fast way
		#workdir=os.getcwd()
		#os.chdir(self.build_dest)
		#self.spawn = ('pax', '-r', '-w', '.', self.install_dir)
		#selfspawn(cmd, 1, self.verbose, self.dry_run);
		#os.chdir(workdir)
		print "Installation complete"
		
def mungeVersion(infile, outfile):
	i = file(infile,'r')
	o = file(outfile,'w')
	o.write(re.sub('\$\(VERSION\)',sysconfig.get_config_var('VERSION'),i.read()))
	i.close()
	o.close()
		
def main():
	# turn off warnings when deprecated modules are imported
	import warnings
	warnings.filterwarnings("ignore",category=DeprecationWarning)
	setup(name = 'Python Documentation',
  		version = '%d.%d' % sys.version_info[:2],
  		cmdclass = {'install_data':DirInstall, 'build':DocBuild},
  		# Data to install
  		data_files =   [(sys.prefix+'/Resources/English.lproj/Documentation',['build-html'])]
		)

if __name__ == '__main__':
	main()
