"""File System Proxy.

Provide an OS-neutral view on a file system, locally or remotely.
The functionality is geared towards implementing some sort of
rdist-like utility between a Mac and a UNIX system.

The module defines three classes:

FSProxyLocal  -- used for local access
FSProxyServer -- used on the server side of remote access
FSProxyClient -- used on the client side of remote access

The remote classes are instantiated with an IP address and an optional
verbosity flag.
"""

import server
import client
import md5
import os
import fnmatch
from stat import *
import time
import fnmatch

if os.name == 'mac':
	import macfs
	maxnamelen = 31
else:
	macfs = None
	maxnamelen = 255

skipnames = (os.curdir, os.pardir)


class FSProxyLocal:
	
	def __init__(self):
		self._dirstack = []
		self._ignore = ['*.pyc'] + self._readignore()
	
	def _close(self):
		while self._dirstack:
			self.back()
	
	def _readignore(self):
		file = self._hide('ignore')
		try:
			f = open(file)
		except IOError:
			file = self._hide('synctree.ignorefiles')
			try:
				f = open(file)
			except IOError:
				return []
		ignore = []
		while 1:
			line = f.readline()
			if not line: break
			if line[-1] == '\n': line = line[:-1]
			ignore.append(line)
		f.close()
		return ignore
	
	def _hidden(self, name):
		if os.name == 'mac':
			return name[0] == '(' and name[-1] == ')'
		else:
			return name[0] == '.'
	
	def _hide(self, name):
		if os.name == 'mac':
			return '(%s)' % name
		else:
			return '.%s' % name
	
	def visible(self, name):
		if len(name) > maxnamelen: return 0
		if name[-1] == '~': return 0
		if name in skipnames: return 0
		if self._hidden(name): return 0
		head, tail = os.path.split(name)
		if head or not tail: return 0
		if macfs:
			if os.path.exists(name) and not os.path.isdir(name):
				try:
					fs = macfs.FSSpec(name)
					c, t = fs.GetCreatorType()
					if t != 'TEXT': return 0
				except macfs.error, msg:
					print "***", name, msg
					return 0
		else:
			if os.path.islink(name): return 0
			if '\0' in open(name, 'rb').read(512): return 0
		for ign in self._ignore:
			if fnmatch.fnmatch(name, ign): return 0
		return 1
	
	def check(self, name):
		if not self.visible(name):
			raise os.error, "protected name %s" % repr(name)
	
	def checkfile(self, name):
		self.check(name)
		if not os.path.isfile(name):
			raise os.error, "not a plain file %s" % repr(name)
	
	def pwd(self):
		return os.getcwd()
	
	def cd(self, name):
		self.check(name)
		save = os.getcwd(), self._ignore
		os.chdir(name)
		self._dirstack.append(save)
		self._ignore = self._ignore + self._readignore()
	
	def back(self):
		if not self._dirstack:
			raise os.error, "empty directory stack"
		dir, ignore = self._dirstack[-1]
		os.chdir(dir)
		del self._dirstack[-1]
		self._ignore = ignore
	
	def _filter(self, files, pat = None):
		if pat:
			def keep(name, pat = pat):
				return fnmatch.fnmatch(name, pat)
			files = filter(keep, files)
		files = filter(self.visible, files)
		files.sort()
		return files
	
	def list(self, pat = None):
		files = os.listdir(os.curdir)
		return self._filter(files, pat)
	
	def listfiles(self, pat = None):
		files = os.listdir(os.curdir)
		files = filter(os.path.isfile, files)
		return self._filter(files, pat)
	
	def listsubdirs(self, pat = None):
		files = os.listdir(os.curdir)
		files = filter(os.path.isdir, files)
		return self._filter(files, pat)
	
	def exists(self, name):
		return self.visible(name) and os.path.exists(name)
	
	def isdir(self, name):
		return self.visible(name) and os.path.isdir(name)
	
	def islink(self, name):
		return self.visible(name) and os.path.islink(name)
	
	def isfile(self, name):
		return self.visible(name) and os.path.isfile(name)
	
	def sum(self, name):
		self.checkfile(name)
		BUFFERSIZE = 1024*8
		f = open(name)
		sum = md5.new()
		while 1:
			buffer = f.read(BUFFERSIZE)
			if not buffer:
				break
			sum.update(buffer)
		return sum.digest()
	
	def size(self, name):
		self.checkfile(name)
		return os.stat(name)[ST_SIZE]
	
	def mtime(self, name):
		self.checkfile(name)
		return time.localtime(os.stat(name)[ST_MTIME])
	
	def stat(self, name):
		self.checkfile(name)
		size = os.stat(name)[ST_SIZE]
		mtime = time.localtime(os.stat(name)[ST_MTIME])
		return size, mtime
	
	def info(self, name):
		sum = self.sum(name)
		size = os.stat(name)[ST_SIZE]
		mtime = time.localtime(os.stat(name)[ST_MTIME])
		return sum, size, mtime
	
	def _list(self, function, list):
		if list is None:
			list = self.listfiles()
		res = []
		for name in list:
			try:
				res.append((name, function(name)))
			except (os.error, IOError):
				res.append((name, None))
		return res
	
	def sumlist(self, list = None):
		return self._list(self.sum, list)
	
	def statlist(self, list = None):
		return self._list(self.stat, list)
	
	def mtimelist(self, list = None):
		return self._list(self.mtime, list)
	
	def sizelist(self, list = None):
		return self._list(self.size, list)
	
	def infolist(self, list = None):
		return self._list(self.info, list)
	
	def _dict(self, function, list):
		if list is None:
			list = self.listfiles()
		dict = {}
		for name in list:
			try:
				dict[name] = function(name)
			except (os.error, IOError):
				pass
		return dict
	
	def sumdict(self, list = None):
		return self.dict(self.sum, list)
	
	def sizedict(self, list = None):
		return self.dict(self.size, list)
	
	def mtimedict(self, list = None):
		return self.dict(self.mtime, list)
	
	def statdict(self, list = None):
		return self.dict(self.stat, list)
	
	def infodict(self, list = None):
		return self._dict(self.info, list)
	
	def read(self, name, offset = 0, length = -1):
		self.checkfile(name)
		f = open(name)
		f.seek(offset)
		if length == 0:
			data = ''
		elif length < 0:
			data = f.read()
		else:
			data = f.read(length)
		f.close()
		return data
	
	def create(self, name):
		self.check(name)
		if os.path.exists(name):
			self.checkfile(name)
			bname = name + '~'
			try:
				os.unlink(bname)
			except os.error:
				pass
			os.rename(name, bname)
		f = open(name, 'w')
		f.close()
	
	def write(self, name, data, offset = 0):
		self.checkfile(name)
		f = open(name, 'r+')
		f.seek(offset)
		f.write(data)
		f.close()
	
	def mkdir(self, name):
		self.check(name)
		os.mkdir(name, 0777)
	
	def rmdir(self, name):
		self.check(name)
		os.rmdir(name)


class FSProxyServer(FSProxyLocal, server.Server):
	
	def __init__(self, address, verbose = server.VERBOSE):
		FSProxyLocal.__init__(self)
		server.Server.__init__(self, address, verbose)
	
	def _close(self):
		server.Server._close(self)
		FSProxyLocal._close(self)
	
	def _serve(self):
		server.Server._serve(self)
		# Retreat into start directory
		while self._dirstack: self.back()


class FSProxyClient(client.Client):
	
	def __init__(self, address, verbose = client.VERBOSE):
		client.Client.__init__(self, address, verbose)


def test():
	import string
	import sys
	if sys.argv[1:]:
		port = string.atoi(sys.argv[1])
	else:
		port = 4127
	proxy = FSProxyServer(('', port))
	proxy._serverloop()


if __name__ == '__main__':
	test()
