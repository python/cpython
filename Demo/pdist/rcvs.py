#! /usr/local/bin/python

"Remote CVS -- command line interface"

from cvslib import CVS, Entry
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
def not_ignored(file):
	return not ignored(file)


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
	
	def prepare(self):
		self.localfiles = filter(not_ignored, os.listdir(os.curdir))
		self.localfiles.sort()

		self.entryfiles = self.keys()

		self.remotefiles = self.proxy.listfiles()

		self.rcsfiles = self.entryfiles[:]
		for file in self.remotefiles:
			if file not in self.rcsfiles:
				self.rcsfiles.append(file)
		self.rcsfiles.sort()

		self.localonlyfiles = []
		for file in self.localfiles:
			if file not in self.rcsfiles:
				self.localonlyfiles.append(file)
		self.localonlyfiles.sort()

		self.allfiles = self.rcsfiles + self.localonlyfiles
		self.allfiles.sort()
	
	def preparedetails(self, file):
		entry = file in self.entryfiles
		if entry:
			e = self.entries[file]
		else:
			e = Entry('/%s/0///' % file)
		e.entry = entry
		e.local = file in self.localfiles
		e.remote = file in self.remotefiles
		if e.local:
			e.lsum = sumfile(file)
		else:
			e.lsum = None
		if e.remote:
			e.rrev = self.proxy.head(file)
			if e.rrev == e.rev:
				e.rsum = e.sum
			else:
				e.rsum = self.proxy.sum(file)
		else:
			e.rrev = '0'
			e.rsum = None
		return e
	
	def report(self):
		self.prepare()
		for file in self.allfiles:
			e = self.preparedetails(file)
			if e.lsum == e.sum == e.rsum:
				# All three exist and are equal
				print '=', file
			elif e.lsum == e.sum:
				# Not modified locally, remote update pending
				if e.lsum is None:
					print 'N', file, '(new remote)'
				else:
					print 'U', file
			elif e.sum == e.rsum:
				# No remote update, modified locally
				if e.rsum is None:
					if e.new:
						print 'A', file
					else:
						print '?', file
				else:
					if e.lsum is None:
						print 'LOST', file
					else:
						print 'M', file
			else:
				# Conflict: remote update and locally modified
				if e.lsum == e.rsum:
					# Local and remote match!
					print 'c', file
				else:
					print 'C', file

	def fullcheck(self):
		self.prepare()
		ok = 1
		for file in self.allfiles:
			if file not in self.entryfiles \
			   and file not in self.remotefiles:
				continue
			e = self.preparedetails(file)
			if e.new:
				if e.rrev:
					print "%s: created by someone else"%file
					ok = 0
				continue
			if e.rrev != e.rev:
				print "%s: out of date (%s vs. %s)" % \
				      (file, e.rev, rrev)
				ok = 0
		return ok
	
	def update(self):
		self.prepare()
		for file in self.rcsfiles:
			e = self.preparedetails(file)
			if e.lsum == e.sum == e.rsum:
				print '=', file
				continue
			if e.sum == e.rsum:
				if e.rev != e.rrev:
					print '%s: %s -> %s w/o change' % \
						(file, e.rev, e.rrev)
					e.rev = e.rrev
				if e.lsum != e.sum:
					if e.lsum is None:
						print '%s: file was lost' % \
							(file,)
						self.get(e)
					elif e.new:
						print 'A', file
					else:
						print 'M', file
				continue
			if e.lsum == e.sum:
				if e.rev == e.rrev:
					print '%s: no new revision' % file
				print 'U', file,
				sys.stdout.flush()
				self.get(e)
				print
				continue
			if e.lsum == e.rsum:
				print 'c', file
				e.rev = e.rrev
				e.sum = e.rsum
				e.new = e.sum is None and e.lsum is not None
				if e.sum:
					e.mtime, e.ctime = os.stat(e.file)[-2:]
				self.entries[file] = e
				continue
			print 'C', file, '(not resolved)'
		self.writeentries()
		self.writesums()
	
	def get(self, e):
		data = self.proxy.get(e.file)
		f = open(e.file, 'w')
		f.write(data)
		f.close()
		nsum = md5.new(data).digest()
		e.setsum(nsum)
		e.mtime, e.ctime = os.stat(e.file)[-2:]
		e.new = 0
		e.rev = e.rrev
	
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
			e.new = 0
			# XXX get it?
			mtime, ctime = os.stat(file)[-2:]
			e.mtime = mtime
			e.ctime = ctime
		self.writeentries()
		self.writesums()
	
	def add(self, file):
		if self.entries.has_key(file):
			print "%s: already known" % file
		else:
			self.entries[file] = Entry('/%s/0/Initial %s//\n' %
						   (file, file))


def sumfile(file):
	return md5.new(open(file).read()).digest()


def test():
	import sys
	import getopt
	from rcsclient import openrcsclient

	opts, args = getopt.getopt(sys.argv[1:], 'd:h:p:vq')
	proxy = openrcsclient(opts)
	x = PCVS(proxy)

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

if __name__ == "__main__":
	test()
