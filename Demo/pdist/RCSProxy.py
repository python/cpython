#! /usr/local/bin/python

"""RCS Proxy.

Provide a simplified interface on RCS files, locally or remotely.
The functionality is geared towards implementing some sort of
remote CVS like utility.  It is modeled after the similar module
FSProxy.

The module defines three classes:

RCSProxyLocal  -- used for local access
RCSProxyServer -- used on the server side of remote access
RCSProxyClient -- used on the client side of remote access

The remote classes are instantiated with an IP address and an optional
verbosity flag.
"""

import server
import client
import md5
import os
import fnmatch
import string
import tempfile


okchars = string.letters + string.digits + '-_=+.'


class RCSProxyLocal:
	
	def __init__(self):
		self._dirstack = []
	
	def _close(self):
		while self._dirstack:
			self.back()
	
	def pwd(self):
		return os.getcwd()
	
	def cd(self, name):
		save = os.getcwd()
		os.chdir(name)
		self._dirstack.append(save)
	
	def back(self):
		if not self._dirstack:
			raise os.error, "empty directory stack"
		dir = self._dirstack[-1]
		os.chdir(dir)
		del self._dirstack[-1]
	
	def _filter(self, files, pat = None):
		if pat:
			def keep(name, pat = pat):
				return fnmatch.fnmatch(name, pat)
			files = filter(keep, files)
		files.sort()
		return files

	def isfile(self, name):
		namev = name + ',v'
		return os.path.isfile(namev) or \
		       os.path.isfile(os.path.join('RCS', namev))
	
	def _unmangle(self, name):
		if type(name) == type(''):
			rev = ''
		else:
			name, rev = name
		return name, rev
	
	def checkfile(self, name):
		name, rev = self._unmangle(name)
		if not self.isfile(name):
			raise os.error, 'not an rcs file %s' % `name`
		for c in rev:
			if c not in okchars:
				raise ValueError, "bad char in rev"
		return name, rev
	
	def listfiles(self, pat = None):
		def isrcs(name): return name[-2:] == ',v'
		def striprcs(name): return name[:-2]
		files = os.listdir(os.curdir)
		files = filter(isrcs, files)
		if os.path.isdir('RCS'):
			files2 = os.listdir('RCS')
			files2 = filter(isrcs, files2)
			files = files + files2
		files = map(striprcs, files)
		return self._filter(files, pat)
	
	def listsubdirs(self, pat = None):
		files = os.listdir(os.curdir)
		files = filter(os.path.isdir, files)
		return self._filter(files, pat)
	
	def isdir(self, name):
		return os.path.isdir(name)

	def _open(self, name, cmd = 'co -p'):
		name, rev = self.checkfile(name)
		namev = name + ',v'
		if rev:
			cmd = cmd + ' -r' + rev
		return os.popen('%s %s' %  (cmd, `namev`))

	def _closepipe(self, f):
		sts = f.close()
		if sts:
			raise IOError, "Exit status %d" % sts
	
	def _remove(self, fn):
		try:
			os.unlink(fn)
		except os.error:
			pass
	
	def sum(self, name):
		f = self._open(name)
		BUFFERSIZE = 1024*8
		sum = md5.new()
		while 1:
			buffer = f.read(BUFFERSIZE)
			if not buffer:
				break
			sum.update(buffer)
		self._closepipe(f)
		return sum.digest()
	
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
		return self.list(self.sum, list)
	
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
	
	def get(self, name):
		f = self._open(name)
		data = f.read()
		self._closepipe(f)
		return data

	def info(self, name):
		f = self._open(name, 'rlog -h')
		dict = {}
		while 1:
			line = f.readline()
			if not line: break
			if line[0] == '\t':
				continue # XXX lock details, later
			i = string.find(line, ':')
			if i > 0:
				key, value = line[:i], string.strip(line[i+1:])
				dict[key] = value
		self._closepipe(f)
		return dict
	
	def head(self, name):
		dict = self.info(name)
		return dict['head']
	
	def log(self, name, flags = ''):
		f = self._open(name, 'rlog %s 2>&1' % flags)
		log = f.read()
		self._closepipe(f)
		return log

	def put(self, fullname, data, message = ""):
		if message and message[-1] != '\n':
			message = message + '\n'
		name, rev = self._unmangle(fullname)
		new = not self.isfile(name)
		if new:
			for c in name:
				if c not in okchars:
					raise ValueError, "bad char in name"
		else:
			self._remove(name)
		f = open(name, 'w')
		f.write(data)
		f.close()
		tf = tempfile.mktemp()
		try:
			if not new:
			    cmd = "rcs -l%s %s >>%s 2>&1" % (rev, name, tf)
			    sts = os.system(cmd)
			    if sts:
				raise IOError, "rcs -l exit status %d" % sts
			cmd = "ci -r%s %s >>%s 2>&1" % (rev, name, tf)
			p = os.popen(cmd, 'w')
			p.write(message)
			sts = p.close()
			if sts:
				raise IOError, "ci exit status %d" % sts
			messages = open(tf).read()
			return messages or None
		finally:
			self._remove(tf)
	
	def mkdir(self, name):
		os.mkdir(name, 0777)
	
	def rmdir(self, name):
		os.rmdir(name)


class RCSProxyServer(RCSProxyLocal, server.Server):
	
	def __init__(self, address, verbose = server.VERBOSE):
		RCSProxyLocal.__init__(self)
		server.Server.__init__(self, address, verbose)
	
	def _close(self):
		server.Server._close(self)
		RCSProxyLocal._close(self)
	
	def _serve(self):
		server.Server._serve(self)
		# Retreat into start directory
		while self._dirstack: self.back()


class RCSProxyClient(client.Client):
	
	def __init__(self, address, verbose = client.VERBOSE):
		client.Client.__init__(self, address, verbose)


def test_server():
	import string
	import sys
	if sys.argv[1:]:
		port = string.atoi(sys.argv[1])
	else:
		port = 4127
	proxy = RCSProxyServer(('', port))
	proxy._serverloop()


def test():
	import sys
	if not sys.argv[1:] or sys.argv[1] and sys.argv[1][0] in '0123456789':
		test_server()
		sys.exit(0)
	proxy = RCSProxyLocal()
	what = sys.argv[1]
	if hasattr(proxy, what):
		attr = getattr(proxy, what)
		if callable(attr):
			print apply(attr, tuple(sys.argv[2:]))
		else:
			print `attr`
	else:
		print "%s: no such attribute" % what
		sys.exit(2)


if __name__ == '__main__':
	test()
