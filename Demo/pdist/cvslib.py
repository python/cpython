"""Utilities for CVS administration."""

import string
import os
import time
import md5
import fnmatch

if not hasattr(time, 'timezone'):
	time.timezone = 0

class File:

	"""Represent a file's status.

	Instance variables:

	file -- the filename (no slashes), None if uninitialized
	lseen -- true if the data for the local file is up to date
	eseen -- true if the data from the CVS/Entries entry is up to date
	         (this implies that the entry must be written back)
	rseen -- true if the data for the remote file is up to date
	proxy -- RCSProxy instance used to contact the server, or None

	Note that lseen and rseen don't necessary mean that a local
	or remote file *exists* -- they indicate that we've checked it.
	However, eseen means that this instance corresponds to an
	entry in the CVS/Entries file.

	If lseen is true:
	
	lsum -- checksum of the local file, None if no local file
	lctime -- ctime of the local file, None if no local file
	lmtime -- mtime of the local file, None if no local file

	If eseen is true:

	erev -- revision, None if this is a no revision (not '0')
	enew -- true if this is an uncommitted added file
	edeleted -- true if this is an uncommitted removed file
	ectime -- ctime of last local file corresponding to erev
	emtime -- mtime of last local file corresponding to erev
	extra -- 5th string from CVS/Entries file

	If rseen is true:

	rrev -- revision of head, None if non-existent
	rsum -- checksum of that revision, Non if non-existent

	If eseen and rseen are both true:
	
	esum -- checksum of revision erev, None if no revision

	Note
	"""

	def __init__(self, file = None):
		if file and '/' in file:
			raise ValueError, "no slash allowed in file"
		self.file = file
		self.lseen = self.eseen = self.rseen = 0
		self.proxy = None

	def __cmp__(self, other):
		return cmp(self.file, other.file)

	def getlocal(self):
		try:
			self.lmtime, self.lctime = os.stat(self.file)[-2:]
		except os.error:
			self.lmtime = self.lctime = self.lsum = None
		else:
			self.lsum = md5.md5(open(self.file).read()).digest()
		self.lseen = 1

	def getentry(self, line):
		words = string.splitfields(line, '/')
		if self.file and words[1] != self.file:
			raise ValueError, "file name mismatch"
		self.file = words[1]
		self.erev = words[2]
		self.edeleted = 0
		self.enew = 0
		self.ectime = self.emtime = None
		if self.erev[:1] == '-':
			self.edeleted = 1
			self.erev = self.erev[1:]
		if self.erev == '0':
			self.erev = None
			self.enew = 1
		else:
			dates = words[3]
			self.ectime = unctime(dates[:24])
			self.emtime = unctime(dates[25:])
		self.extra = words[4]
		if self.rseen:
			self.getesum()
		self.eseen = 1

	def getremote(self, proxy = None):
		if proxy:
			self.proxy = proxy
		try:
			self.rrev = self.proxy.head(self.file)
		except (os.error, IOError):
			self.rrev = None
		if self.rrev:
			self.rsum = self.proxy.sum(self.file)
		else:
			self.rsum = None
		if self.eseen:
			self.getesum()
		self.rseen = 1

	def getesum(self):
		if self.erev == self.rrev:
			self.esum = self.rsum
		elif self.erev:
			name = (self.file, self.erev)
			self.esum = self.proxy.sum(name)
		else:
			self.esum = None

	def putentry(self):
		"""Return a line suitable for inclusion in CVS/Entries.

		The returned line is terminated by a newline.
		If no entry should be written for this file,
		return "".
		"""
		if not self.eseen:
			return ""

		rev = self.erev or '0'
		if self.edeleted:
			rev = '-' + rev
		if self.enew:
			dates = 'Initial ' + self.file
		else:
			dates = gmctime(self.ectime) + ' ' + \
				gmctime(self.emtime)
		return "/%s/%s/%s/%s/\n" % (
			self.file,
			rev,
			dates,
			self.extra)

	def report(self):
		print '-'*50
		def r(key, repr=repr, self=self):
			try:
				value = repr(getattr(self, key))
			except AttributeError:
				value = "?"
			print "%-15s:" % key, value
		r("file")
		if self.lseen:
			r("lsum", hexify)
			r("lctime", gmctime)
			r("lmtime", gmctime)
		if self.eseen:
			r("erev")
			r("enew")
			r("edeleted")
			r("ectime", gmctime)
			r("emtime", gmctime)
		if self.rseen:
			r("rrev")
			r("rsum", hexify)
			if self.eseen:
				r("esum", hexify)


class CVS:
	
	"""Represent the contents of a CVS admin file (and more).

	Class variables:

	FileClass -- the class to be instantiated for entries
	             (this should be derived from class File above)
	IgnoreList -- shell patterns for local files to be ignored

	Instance variables:

	entries -- a dictionary containing File instances keyed by
	           their file name
	proxy -- an RCSProxy instance, or None
	"""
	
	FileClass = File

	IgnoreList = ['.*', '@*', ',*', '*~', '*.o', '*.a', '*.so', '*.pyc']
	
	def __init__(self):
		self.entries = {}
		self.proxy = None
	
	def setproxy(self, proxy):
		if proxy is self.proxy:
			return
		self.proxy = proxy
		for e in self.entries.values():
			e.rseen = 0
	
	def getentries(self):
		"""Read the contents of CVS/Entries"""
		self.entries = {}
		f = self.cvsopen("Entries")
		while 1:
			line = f.readline()
			if not line: break
			e = self.FileClass()
			e.getentry(line)
			self.entries[e.file] = e
		f.close()
	
	def putentries(self):
		"""Write CVS/Entries back"""
		f = self.cvsopen("Entries", 'w')
		for e in self.values():
			f.write(e.putentry())
		f.close()

	def getlocalfiles(self):
		list = self.entries.keys()
		addlist = os.listdir(os.curdir)
		for name in addlist:
			if name in list:
				continue
			if not self.ignored(name):
				list.append(name)
		list.sort()
		for file in list:
			try:
				e = self.entries[file]
			except KeyError:
				e = self.entries[file] = self.FileClass(file)
			e.getlocal()

	def getremotefiles(self, proxy = None):
		if proxy:
			self.proxy = proxy
		if not self.proxy:
			raise RuntimeError, "no RCS proxy"
		addlist = self.proxy.listfiles()
		for file in addlist:
			try:
				e = self.entries[file]
			except KeyError:
				e = self.entries[file] = self.FileClass(file)
			e.getremote(self.proxy)

	def report(self):
		for e in self.values():
			e.report()
		print '-'*50
	
	def keys(self):
		keys = self.entries.keys()
		keys.sort()
		return keys

	def values(self):
		def value(key, self=self):
			return self.entries[key]
		return map(value, self.keys())

	def items(self):
		def item(key, self=self):
			return (key, self.entries[key])
		return map(item, self.keys())

	def cvsexists(self, file):
		file = os.path.join("CVS", file)
		return os.path.exists(file)
	
	def cvsopen(self, file, mode = 'r'):
		file = os.path.join("CVS", file)
		if 'r' not in mode:
			self.backup(file)
		return open(file, mode)
	
	def backup(self, file):
		if os.path.isfile(file):
			bfile = file + '~'
			try: os.unlink(bfile)
			except os.error: pass
			os.rename(file, bfile)

	def ignored(self, file):
		if os.path.isdir(file): return 1
		for pat in self.IgnoreList:
			if fnmatch.fnmatch(file, pat): return 1
		return 0


# hexify and unhexify are useful to print MD5 checksums in hex format

hexify_format = '%02x' * 16
def hexify(sum):
	"Return a hex representation of a 16-byte string (e.g. an MD5 digest)"
	if sum is None:
		return "None"
	return hexify_format % tuple(map(ord, sum))

def unhexify(hexsum):
	"Return the original from a hexified string"
	if hexsum == "None":
		return None
	sum = ''
	for i in range(0, len(hexsum), 2):
		sum = sum + chr(string.atoi(hexsum[i:i+2], 16))
	return sum


unctime_monthmap = {}
def unctime(date):
	if date == "None": return None
	if not unctime_monthmap:
		months = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun',
			  'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec']
		i = 0
		for m in months:
			i = i+1
			unctime_monthmap[m] = i
	words = string.split(date) # Day Mon DD HH:MM:SS YEAR
	year = string.atoi(words[4])
	month = unctime_monthmap[words[1]]
	day = string.atoi(words[2])
	[hh, mm, ss] = map(string.atoi, string.splitfields(words[3], ':'))
	ss = ss - time.timezone
	return time.mktime((year, month, day, hh, mm, ss, 0, 0, 0))

def gmctime(t):
	if t is None: return "None"
	return time.asctime(time.gmtime(t))

def test_unctime():
	now = int(time.time())
	t = time.gmtime(now)
	at = time.asctime(t)
	print 'GMT', now, at
	print 'timezone', time.timezone
	print 'local', time.ctime(now)
	u = unctime(at)
	print 'unctime()', u
	gu = time.gmtime(u)
	print '->', gu
	print time.asctime(gu)

def test():
	x = CVS()
	x.getentries()
	x.getlocalfiles()
##	x.report()
	import rcsclient
	proxy = rcsclient.openrcsclient()
	x.getremotefiles(proxy)
	x.report()


if __name__ == "__main__":
	test()
