from cvslib import CVS, Entry
import RCSProxy
import client
import md5
import os
import string
import sys
import time
import fnmatch


ignored_patterns = ['*.pyc', '.*', '*~', '@*']
def ignored(file):
	if os.path.isdir(file): return 1
	for pat in ignored_patterns:
		if fnmatch.fnmatch(file, pat): return 1
	return 0


class PCVS(CVS):
	
	def __init__(self, proxy):
		CVS.__init__(self)
		self.proxy = proxy
		self.readsums()
		self.calcsums()
	
	def calcsums(self):
		for file in self.keys():
			e = self.entries[file]
			if not e.new and e.sum is None:
				sum = self.proxy.sum((file, e.rev))
				e.setsum(sum)
	
	def fullcheck(self):
		ok = 1
		for file in self.keys():
			e = self.entries[file]
			if e.new:
				if self.proxy.isfile(file):
					print "%s: created by someone else!"
					ok = 0
				continue
			rrev = self.proxy.head(file)
			if rrev != e.rev:
				print "%s: out of date (%s vs. %s)" % \
				      (file, e.rev, rrev)
				ok = 0
		return ok
	
	def update(self):
		for file in self.keys():
			e = self.entries[file]
			if e.new:
				print 'A', file
				continue
			rrev = self.proxy.head(file)
			lsum = sumfile(file)
			if rrev == e.rev:
				if lsum == e.sum:
					print '=', file
				else:
					print 'M', file
				continue
			if e.sum != lsum:
				print "%s: conflict -- not updated" % file
				continue
			print "%s: getting ..." % file
			data = self.proxy.get(file)
			f = open(file, 'w')
			f.write(data)
			f.close()
			nsum = md5.new(data).digest()
			e.setsum(nsum)
			e.rev = rrev
			print 'U', file
		self.writeentries()
		self.writesums()
	
	def commit(self):
		if not self.fullcheck():
			print "correct above errors first"
			return
		needed = []
		for file in self.keys():
			e = self.entries[file]
			if e.new:
				needed.append(file)
				continue
			lsum = sumfile(file)
			if lsum != e.sum:
				needed.append(file)
				continue
		if not needed:
			print "no changes need committing"
			return
		message = raw_input("One-liner: ")
		for file in needed:
			print "%s: putting ..." % file
			e = self.entries[file]
			data = open(file).read()
			self.proxy.put(file, data, message)
			e.rev = self.proxy.head(file)
			e.setsum(self.proxy.sum(file))
			# XXX get it?
			mtime, ctime = os.stat(file)[-2:]
			e.mtime = mtime
			e.ctime = ctime
		self.writeentries()
		self.writesums()
	
	def report(self):
		keys = self.keys()
		files = os.listdir(os.curdir)
		allfiles = files
		for file in keys:
			if file not in allfiles:
				allfiles.append(file)
		allfiles.sort()
		for file in allfiles:
			if file not in keys:
				if not ignored(file):
					print '?', file
				continue
			if file not in files:
				print file, ': lost'
				continue
			e = self.entries[file]
			if not os.path.exists(file):
				print "%s: lost" % file
				continue
			if e.new:
				print 'A', file
				continue
			lsum = sumfile(file)
			rrev = self.proxy.head(file)
			if rrev == e.rev:
				if lsum == e.sum:
					print '=', file
				else:
					print 'M', file
			else:
				if lsum == e.sum:
					print 'U', file
				else:
					print 'C', file
	
	def add(self, file):
		if self.entries.has_key(file):
			print "%s: already known"
		else:
			self.entries[file] = Entry('/%s/0/Initial %s//\n' %
						   (file, file))


def sumfile(file):
	return md5.new(open(file).read()).digest()


def test():
	proxy = RCSProxy.RCSProxyClient(('voorn.cwi.nl', 4127))
	proxy.cd('/ufs/guido/voorn/python-RCS/Demo/pdist')
	x = PCVS(proxy)
	args = sys.argv[1:]
	if args:
		cmd = args[0]
		files = args[1:]
		if cmd == 'add':
			if not files:
				print "add needs at least one file argument"
			else:
				for file in files:
					x.add(file)
				x.writeentries()
		elif cmd in ('update', 'up'):
			if files:
				print "updates wants no file arguments"
			else:
				x.update()
		elif cmd in ('commit', 'com'):
			if files:
				print "commit wants no file arguments"
			else:
				x.commit()
		else:
			print "Unknown command", cmd
	else:
		x.report()
		if sys.argv[1:]: x.writesums()

if __name__ == "__main__":
	test()
