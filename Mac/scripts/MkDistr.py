#
# Interactively decide what to distribute
#
#
# The exclude file signals files to always exclude,
# The pattern file lines are of the form
# *.c
# This excludes all files ending in .c.
#
# The include file signals files and directories to include.
# Records are of the form
# ('Tools:bgen:AE:AppleEvents.py', 'Lib:MacToolbox:AppleEvents.py')
# This includes the specified file, putting it in the given place, or
# ('Tools:bgen:AE:AppleEvents.py', None)
# This excludes the specified file.
#
from MkDistr_ui import *
import fnmatch
import regex
import os
import sys
import macfs
import macostools

DEBUG=0

SyntaxError='Include/exclude file syntax error'

class Matcher:
	"""Include/exclude database, common code"""
	
	def __init__(self, filename):
		self.filename = filename
		self.rawdata = []
		self.parse(filename)
		self.rawdata.sort()
		self.rebuild()
		self.modified = 0

	def parse(self, dbfile):
		try:
			fp = open(dbfile)
		except IOError:
			return
		data = fp.readlines()
		fp.close()
		for d in data:
			d = d[:-1]
			if not d or d[0] == '#': continue
			pat = self.parseline(d)
			self.rawdata.append(pat)
				
	def save(self):
		fp = open(self.filename, 'w')
		self.savedata(fp, self.rawdata)
		self.modified = 0
			
	def add(self, value):
		if len(value) == 1:
			value = value + ('',)
		self.rawdata.append(value)
		self.rebuild1(value)
		self.modified = 1
		
	def delete(self, value):
		key = value
		for i in range(len(self.rawdata)):
			if self.rawdata[i][0] == key:
				del self.rawdata[i]
				self.unrebuild1(i, key)
				self.modified = 1
				return
		print 'Not found!', key
				
	def getall(self):
		return map(lambda x: x[0], self.rawdata)
	
	def get(self, value):
		for src, dst in self.rawdata:
			if src == value:
				return src, dst
		print 'Not found!', value
				
	def is_modified(self):
		return self.modified
							
class IncMatcher(Matcher):
	"""Include filename database and matching engine"""

	def rebuild(self):
		self.idict = {}
		self.edict = {}
		for v in self.rawdata:
			self.rebuild1(v)
			
	def parseline(self, line):
		try:
			data = eval(line)
		except:
			raise SyntaxError, line
		if type(data) <> type(()) or len(data) not in (1,2):
			raise SyntaxError, line
		if len(data) == 1:
			data = data + ('',)
		return data
		
	def savedata(self, fp, data):
		for d in self.rawdata:
			fp.write(`d`+'\n')
		
	def rebuild1(self, (src, dst)):
		if dst == '':
			dst = src
		if dst == None:
			self.edict[src] = None
		else:
			self.idict[src] = dst
			
	def unrebuild1(self, num, src):
		if self.idict.has_key(src):
			del self.idict[src]
		else:
			del self.edict[src]
	
	def match(self, patharg):
		removed = []
		# First check the include directory
		path = patharg
		while 1:
			if self.idict.has_key(path):
				# We know of this path (or initial piece of path)
				dstpath = self.idict[path]
				# We do want it distributed. Tack on the tail.
				while removed:
					dstpath = os.path.join(dstpath, removed[0])
					removed = removed[1:]
				# Finally, if the resultant string ends in a separator
				# tack on our input filename
				if dstpath[-1] == os.sep:
					dir, file = os.path.split(path)
					dstpath = os.path.join(dstpath, file)
				if DEBUG:
					print 'include', patharg, dstpath
				return dstpath
			path, lastcomp = os.path.split(path)
			if not path:
				break
			removed[0:0] = [lastcomp]
		# Next check the exclude directory
		path = patharg
		while 1:
			if self.edict.has_key(path):
				if DEBUG:
					print 'exclude', patharg, path
				return ''
			path, lastcomp = os.path.split(path)
			if not path:
				break
			removed[0:0] = [lastcomp]
		if DEBUG:
			print 'nomatch', patharg
		return None
			
	def checksourcetree(self):
		rv = []
		for name in self.idict.keys():
			if not os.path.exists(name):
				rv.append(name)
		return rv
				
class ExcMatcher(Matcher):
	"""Exclude pattern database and matching engine"""

	def rebuild(self):
		self.relist = []
		for v in self.rawdata:
			self.rebuild1(v)
		
	def parseline(self, data):
		return (data, None)

	def savedata(self, fp, data):
		for d in self.rawdata:
			fp.write(d[0]+'\n')		
		
	def rebuild1(self, (src, dst)):
		pat = fnmatch.translate(src)
		if DEBUG:
			print 'PATTERN', `src`, 'REGEX', `pat`
		self.relist.append(regex.compile(pat))
		
	def unrebuild1(self, num, src):
		del self.relist[num]
	
	def match(self, path):
		comps = os.path.split(path)
		file = comps[-1]
		for pat in self.relist:
			if pat and pat.match(file) == len(file):
				if DEBUG:
					print 'excmatch', file, pat
				return 1
		return 0		
		 
		
class Main:
	"""The main program glueing it all together"""
	
	def __init__(self):
		InitUI()
		fss, ok = macfs.GetDirectory('Source directory:')
		if not ok:
			sys.exit(0)
		os.chdir(fss.as_pathname())
		if not os.path.isdir(':Mac:Distributions'):
			os.mkdir(':Mac:Distributions')
		typedist = GetType()
		self.inc = IncMatcher(':Mac:Distributions:%s.include'%typedist)
		self.exc = ExcMatcher(':Mac:Distributions:%s.exclude'%typedist)
		self.ui = MkDistrUI(self)
		self.ui.mainloop()
		
	def check(self):
		return self.checkdir(':', 1)
		
	def checkdir(self, path, istop):
		if DEBUG:
			print 'checkdir', path
		files = os.listdir(path)
		rv = []
		todo = []
		for f in files:
			if DEBUG:
				print 'checkfile', f
			if self.exc.match(f):
				if DEBUG:
					print 'exclude match', f
				continue
			fullname = os.path.join(path, f)
			if DEBUG:
				print 'checkpath', fullname
			matchvalue = self.inc.match(fullname)
			if matchvalue == None:
				if os.path.isdir(fullname):
					if DEBUG:
						print 'do dir', fullname
					todo.append(fullname)
				else:
					if DEBUG:
						print 'include', fullname
					rv.append(fullname)
			elif DEBUG:
				print 'badmatch', matchvalue
		for d in todo:
			if len(rv) > 500:
				if istop:
					rv.append('... and more ...')
				return rv
			rv = rv + self.checkdir(d, 0)
		return rv
		
	def run(self, destprefix):
		missing = self.inc.checksourcetree()
		if missing:
			print '==== Missing source files ===='
			for i in missing:
				print i
			print '==== Fix and retry ===='
			return
		if not self.rundir(':', destprefix, 0):
			return
		self.rundir(':', destprefix, 1)

	def rundir(self, path, destprefix, doit):
		files = os.listdir(path)
		todo = []
		rv = 1
		for f in files:
			if self.exc.match(f):
				continue
			fullname = os.path.join(path, f)
			if os.path.isdir(fullname):
				todo.append(fullname)
			else:
				dest = self.inc.match(fullname)
				if dest == None:
					print 'Not yet resolved:', fullname
					rv = 0
				if dest:
					if doit:
						print 'COPY ', fullname
						print '  -> ', os.path.join(destprefix, dest)
						try:
							macostools.copy(fullname, os.path.join(destprefix, dest), 1)
						except: #DBG
							print 'cwd', os.path.getcwd() #DBG
							print 'fsspec', macfs.FSSpec(fullname) #DBG
							sys.exit(1)
		for d in todo:
			if not self.rundir(d, destprefix, doit):
				rv = 0
		return rv
		
	def save(self):
		self.inc.save()
		self.exc.save()
		
	def is_modified(self):
		return self.inc.is_modified() or self.exc.is_modified()

if __name__ == '__main__':
	Main()
	
