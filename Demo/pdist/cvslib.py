"""Utilities to read and write CVS admin files (esp. CVS/Entries)"""

import string
import os
import time


class Entry:

	"""Class representing one (parsed) line from CVS/Entries"""
	
	def __init__(self, line):
		words = string.splitfields(line, '/')
		self.file = words[1]
		self.rev = words[2]
		dates = words[3] # ctime, mtime
		if dates[:7] == 'Initial':
			self.ctime = None
			self.mtime = None
			self.new = 1
		else:
			self.ctime = unctime(dates[:24])
			self.mtime = unctime(dates[25:])
			self.new = 0
		self.extra = words[4]
		self.sum = None
	
	def unparse(self):
		if self.new:
			dates = "Initial %s" % self.file
		else:
			dates = gmctime(self.ctime) + ' ' + gmctime(self.mtime)
		return "/%s/%s/%s/%s/\n" % (
			self.file,
			self.rev,
			dates,
			self.extra)
	
	def setsum(self, sum):
		self.sum = sum
	
	def getsum(self):
		return self.sum
	
	def sethexsum(self, hexsum):
		self.setsum(unhexify(hexsum))
	
	def gethexsum(self):
		if self.sum:
			return hexify(self.sum)
		else:
			return None


class CVS:

	"""Class representing the contents of CVS/Entries (and CVS/Sums)"""
	
	def __init__(self):
		self.readentries()
	
	def readentries(self):
		self.entries = {}
		f = self.cvsopen("Entries")
		while 1:
			line = f.readline()
			if not line: break
			e = Entry(line)
			self.entries[e.file] = e
		f.close()
	
	def readsums(self):
		try:
			f = self.cvsopen("Sums")
		except IOError:
			return
		while 1:
			line = f.readline()
			if not line: break
			words = string.split(line)
			[file, rev, hexsum] = words
			e = self.entries[file]
			if e.rev == rev:
				e.sethexsum(hexsum)
		f.close()
	
	def writeentries(self):
		f = self.cvsopen("Entries", 'w')
		for file in self.keys():
			f.write(self.entries[file].unparse())
		f.close()
	
	def writesums(self):
		if self.cvsexists("Sums"):
			f = self.cvsopen("Sums", 'w')
		else:
			f = None
		for file in self.keys():
			e = self.entries[file]
			hexsum = e.gethexsum()
			if hexsum:
				if not f:
					f = self.cvsopen("Sums", 'w')
				f.write("%s %s %s\n" % (file, e.rev, hexsum))
		if f:
			f.close()
	
	def keys(self):
		keys = self.entries.keys()
		keys.sort()
		return keys

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
			os.rename(file, bfile)


hexify_format = '%02x' * 16
def hexify(sum):
	"Return a hex representation of a 16-byte string (e.g. an MD5 digest)"
	return hexify_format % tuple(map(ord, sum))

def unhexify(hexsum):
	"Return the original from a hexified string"
	sum = ''
	for i in range(0, len(hexsum), 2):
		sum = sum + chr(string.atoi(hexsum[i:i+2], 16))
	return sum


unctime_monthmap = {}
def unctime(date):
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
	keys = x.entries.keys()
	keys.sort()
	for file in keys:
		e = x.entries[file]
		print file, e.rev, gmctime(e.ctime), gmctime(e.mtime), e.extra,
		print e.gethexsum()


if __name__ == "__main__":
	test()
