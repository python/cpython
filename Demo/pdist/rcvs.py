#! /usr/local/bin/python

"""Remote CVS -- command line interface"""

from cvslib import CVS, File
import md5
import os
import string
import sys
from cmdfw import CommandFrameWork


class MyFile(File):

	def action(self):
		"""Return a code indicating the update status of this file.

		The possible return values are:
		
		'=' -- everything's fine
		'0' -- file doesn't exist anywhere
		'?' -- exists locally only
		'A' -- new locally
		'R' -- deleted locally
		'U' -- changed remotely, no changes locally
		       (includes new remotely or deleted remotely)
		'M' -- changed locally, no changes remotely
		'C' -- conflict: changed locally as well as remotely
		       (includes cases where the file has been added
		       or removed locally and remotely)
		'D' -- deleted remotely
		'N' -- new remotely
		'r' -- get rid of entry
		'c' -- create entry
		'u' -- update entry
		"""
		if not self.lseen:
			self.getlocal()
		if not self.rseen:
			self.getremote()
		if not self.eseen:
			if not self.lseen:
				if not self.rseen: return '0' # Never heard of
				else:
					return 'N' # New remotely
			else: # self.lseen
				if not self.rseen: return '?' # Local only
				# Local and remote, but no entry
				if self.lsum == self.rsum:
					return 'c' # Restore entry only
				else: return 'C' # Real conflict
		else: # self.eseen
			if not self.lseen:
				if self.eremoved:
					if self.rseen: return 'R' # Removed
					else: return 'r' # Get rid of entry
				else: # not self.eremoved
					if self.rseen:
						print "warning:",
						print self.file,
						print "was lost"
						return 'U'
					else: return 'r' # Get rid of entry
			else: # self.lseen
				if not self.rseen:
					if self.enew: return 'A' # New locally
					else: return 'D' # Deleted remotely
				else: # self.rseen
					if self.enew:
						if self.lsum == self.rsum:
							return 'u'
						else:
							return 'C'
					if self.lsum == self.esum:
						if self.esum == self.rsum:
							return '='
						else:
							return 'U'
					elif self.esum == self.rsum:
						return 'M'
					elif self.lsum == self.rsum:
						return 'u'
					else:
						return 'C'

	def update(self):
		code = self.action()
		if code == '=': return
		print code, self.file
		if code in ('U', 'N'):
			self.get()
		elif code == 'C':
			print "%s: conflict resolution not yet implemented" % \
			      self.file
		elif code == 'D':
			remove(self.file)
			self.eseen = 0
		elif code == 'r':
			self.eseen = 0
		elif code in ('c', 'u'):
			self.eseen = 1
			self.erev = self.rrev
			self.enew = 0
			self.edeleted = 0
			self.esum = self.rsum
			self.emtime, self.ectime = os.stat(self.file)[-2:]
			self.extra = ''

	def commit(self, message = ""):
		code = self.action()
		if code in ('A', 'M'):
			self.put(message)
		elif code == 'R':
			print "%s: committing removes not yet implemented" % \
			      self.file
		elif code == 'C':
			print "%s: conflict resolution not yet implemented" % \
			      self.file

	def diff(self, opts = []):
		self.action()		# To update lseen, rseen
		if self.lsum == self.rsum:
			return
		import tempfile
		flags = ''
		for o, a in opts:
			flags = flags + ' ' + o + a
		flags = flags[1:]
		fn = self.file
		data = self.proxy.get(fn)
		tfn = tempfile.mktemp()
		try:
			tf = open(tfn, 'w')
			tf.write(data)
			tf.close()
			print 'diff %s -r%s %s' % (flags, self.rrev, fn)
			sts = os.system('diff %s %s %s' % (flags, tfn, fn))
			if sts:
				print '='*70
		finally:
			remove(tfn)

	def commitcheck(self):
		return self.action() != 'C'

	def put(self, message = ""):
		print "Checking in", self.file, "..."
		data = open(self.file).read()
		messages = self.proxy.put(self.file, data, message)
		if messages:
			print messages
		self.setentry(self.proxy.head(self.file), self.lsum)
	
	def get(self):
		data = self.proxy.get(self.file)
		f = open(self.file, 'w')
		f.write(data)
		f.close()
		self.setentry(self.rrev, self.rsum)

	def setentry(self, erev, esum):
		self.eseen = 0		# While we're hacking...
		self.esum = esum
		self.emtime, self.ectime = os.stat(self.file)[-2:]
		self.erev = erev
		self.enew = 0
		self.edeleted = 0
		self.eseen = 1		# Done


class RCVS(CVS):

	FileClass = MyFile

	def __init__(self):
		CVS.__init__(self)

	def update(self, files):
		for e in self.whichentries(files, 1):
			e.update()

	def commit(self, files, message = ""):
		list = self.whichentries(files)
		ok = 1
		for e in list:
			if not e.commitcheck():
				ok = 0
		if not ok:
			print "correct above errors first"
			return
		if not message:
			message = raw_input("One-liner: ")
		for e in list:
			e.commit(message)

	def report(self, files):
		for e in self.whichentries(files):
			e.report()

	def diff(self, files, opts):
		for e in self.whichentries(files):
			e.diff(opts)

	def whichentries(self, files, localfilestoo = 0):
		if files:
			list = []
			for file in files:
				if self.entries.has_key(file):
					e = self.entries[file]
				else:
					e = self.FileClass(file)
					self.entries[file] = e
				list.append(e)
		else:
			list = self.entries.values()
			for file in self.proxy.listfiles():
				if self.entries.has_key(file):
					continue
				e = self.FileClass(file)
				self.entries[file] = e
				list.append(e)
			if localfilestoo:
				for file in os.listdir(os.curdir):
					if not self.entries.has_key(file) \
					   and not self.ignored(file):
						e = self.FileClass(file)
						self.entries[file] = e
						list.append(e)
			list.sort()
		if self.proxy:
			for e in list:
				if e.proxy is None:
					e.proxy = self.proxy
		return list


class rcvs(CommandFrameWork):

	GlobalFlags = 'd:h:p:qv'
	UsageMessage = \
"usage: rcvs [-d directory] [-h host] [-p port] [-q] [-v] [subcommand arg ...]"
	PostUsageMessage = \
		"If no subcommand is given, the status of all files is listed"

	def __init__(self):
		"""Constructor."""
		CommandFrameWork.__init__(self)
		self.proxy = None
		self.cvs = RCVS()

	def options(self, opts):
		self.opts = opts

	def ready(self):
		import rcsclient
		self.proxy = rcsclient.openrcsclient(self.opts)
		self.cvs.setproxy(self.proxy)
		self.cvs.getentries()

	def default(self):
		self.cvs.report([])

	def do_update(self, opts, files):
		"""update [file] ..."""
		self.cvs.update(files)
		self.cvs.putentries()
	do_up = do_update

	def do_commit(self, opts, files):
		"""commit [-m message] [file] ..."""
		message = ""
		for o, a in opts:
			if o == '-m': message = a
		self.cvs.commit(files, message)
		self.cvs.putentries()
	do_com = do_commit
	flags_commit = 'm:'

	def do_diff(self, opts, files):
		"""diff [difflags] [file] ..."""
		self.cvs.diff(files, opts)
	do_dif = do_diff
	flags_diff = 'cbitwcefhnlrsD:S:'



def remove(fn):
	try:
		os.unlink(fn)
	except os.error:
		pass


def main():
	rcvs().run()


if __name__ == "__main__":
	main()
