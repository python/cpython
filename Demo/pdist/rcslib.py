"""Base local RCS interface module.

Provides simplified interface to RCS controlled files.  Unabashedly
ripped most of this from RCSProxy.py which had in some respects, more
than I needed, and in others, not enough.  I think RCSProxy.py could
be rewritten using this module as a base.
"""

__version__ = "guido's, based on: rcs.py,v 1.2 1995/06/22 21:11:37 bwarsaw Exp"


import string
import os
import tempfile

okchars = string.letters + string.digits + '-_=+.'


class RCSDirectory:

    def __init__(self, directory=None):
	self._dirstack = []
	if directory:
	    self.cd(directory)

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
	namev = self.rcsname(name)
	return namev and (os.path.isfile(namev) or \
			  os.path.isfile(os.path.join('RCS', namev)))

    def isrcs(self, name):
	return name[-2:] == ',v'

    def rcsname(self, name):
	if self.isrcs(name): namev = name
	else: namev = name + ',v'
	if os.path.isfile(namev): return namev
	namev = os.path.join('RCS', namev)
	if os.path.isfile(namev): return namev
	return None

    def realname(self, namev):
	if self.isrcs(namev): name = namev[:-2]
	else: name = namev
	if os.path.isfile(name): return name
	name = os.path.basename(name)
	if os.path.isfile(name): return name
	return None

    def islocked(self, name):
	f = self._open(name, 'rlog -L -R')
	line = f.readline()
	self._closepipe(f)
	if not line: return None
	return not self.realname(name) == self.realname(line)

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
	files = os.listdir(os.curdir)
	files = filter(self.isrcs, files)
	if os.path.isdir('RCS'):
	    files2 = os.listdir('RCS')
	    files2 = filter(self.isrcs, files2)
	    files = files + files2
	files = map(self.realname, files)
	return self._filter(files, pat)

    def listsubdirs(self, pat = None):
	files = os.listdir(os.curdir)
	files = filter(os.path.isdir, files)
	return self._filter(files, pat)

    def isdir(self, name):
	return os.path.isdir(name)

    def _open(self, name, cmd = 'co -p'):
	name, rev = self.checkfile(name)
	namev = self.rcsname(name)
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

    def info(self, name):
	f = self._open(name, 'rlog -h')
	dict = {}
	while 1:
	    line = f.readline()
	    if not line: break
	    if line[0] == '\t':
		# TBD: could be a lock or symbolic name
		# Anything else?
		continue 
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

    def get(self, name, withlock=None, otherflags=""):
	"""a.k.a. co"""
	name, rev = self._unmangle(name)
	if not self.isfile(name):
	    raise os.error, 'not an rcs file %s' % `name`
	if withlock: lockflag = "-l"
	else: lockflag = "-u"
	cmd = 'co %s%s %s %s 2>&1' % (lockflag, rev, otherflags, name)
	sts = os.system(cmd)
	if sts: raise IOError, "bad co exit status %d" % sts

    def put(self, name, message=None, otherflags=""):
	"""a.k.a. ci"""
	if not message: message = "<none>"
	if message and message[-1] != '\n':
	    message = message + '\n'
	name, rev = self._unmangle(name)
	new = not self.isfile(name)
	# by default, check it in -u so we get an unlocked copy back
	# in the current directory
	textfile = None
	try:
	    if new:
		for c in name:
		    if c not in okchars:
			raise ValueError, "bad char in name"
		textfile = tempfile.mktemp()
		f = open(textfile, 'w')
		f.write(message)
		f.close()
		cmd = 'ci -u%s -t%s %s %s 2>&1' % \
		      (rev, textfile, otherflags, name)
	    else:
		cmd = 'ci -u%s -m"%s" %s %s 2>&1' % \
		      (rev, message, otherflags, name)

	    sts = os.system(cmd)
	    if sts: raise IOError, "bad ci exit status %d" % sts
	finally:
	    if textfile: self._remove(textfile)

    def mkdir(self, name):
	os.mkdir(name, 0777)

    def rmdir(self, name):
	os.rmdir(name)
