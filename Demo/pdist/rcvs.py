#! /usr/bin/env python

"""Remote CVS -- command line interface"""

# XXX To do:
#
# Bugs:
# - if the remote file is deleted, "rcvs update" will fail
#
# Functionality:
# - cvs rm
# - descend into directories (alraedy done for update)
# - conflict resolution
# - other relevant commands?
# - branches
#
# - Finesses:
# - retain file mode's x bits
# - complain when "nothing known about filename"
# - edit log message the way CVS lets you edit it
# - cvs diff -rREVA -rREVB
# - send mail the way CVS sends it
#
# Performance:
# - cache remote checksums (for every revision ever seen!)
# - translate symbolic revisions to numeric revisions
#
# Reliability:
# - remote locking
#
# Security:
# - Authenticated RPC?


from cvslib import CVS, File
import md5
import os
import string
import sys
from cmdfw import CommandFrameWork


DEF_LOCAL = 1				# Default -l


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

		(and probably others :-)
		"""
		if not self.lseen:
			self.getlocal()
		if not self.rseen:
			self.getremote()
		if not self.eseen:
			if not self.lsum:
				if not self.rsum: return '0' # Never heard of
				else:
					return 'N' # New remotely
			else: # self.lsum
				if not self.rsum: return '?' # Local only
				# Local and remote, but no entry
				if self.lsum == self.rsum:
					return 'c' # Restore entry only
				else: return 'C' # Real conflict
		else: # self.eseen
			if not self.lsum:
				if self.edeleted:
					if self.rsum: return 'R' # Removed
					else: return 'r' # Get rid of entry
				else: # not self.edeleted
					if self.rsum:
						print "warning:",
						print self.file,
						print "was lost"
						return 'U'
					else: return 'r' # Get rid of entry
			else: # self.lsum
				if not self.rsum:
					if self.enew: return 'A' # New locally
					else: return 'D' # Deleted remotely
				else: # self.rsum
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
			return 1
		elif code == 'R':
			print "%s: committing removes not yet implemented" % \
			      self.file
		elif code == 'C':
			print "%s: conflict resolution not yet implemented" % \
			      self.file

	def diff(self, opts = []):
		self.action()		# To update lseen, rseen
		flags = ''
		rev = self.rrev
		# XXX should support two rev options too!
		for o, a in opts:
			if o == '-r':
				rev = a
			else:
				flags = flags + ' ' + o + a
		if rev == self.rrev and self.lsum == self.rsum:
			return
		flags = flags[1:]
		fn = self.file
		data = self.proxy.get((fn, rev))
		sum = md5.new(data).digest()
		if self.lsum == sum:
			return
		import tempfile
		tfn = tempfile.mktemp()
		try:
			tf = open(tfn, 'w')
			tf.write(data)
			tf.close()
			print 'diff %s -r%s %s' % (flags, rev, fn)
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
		if not self.enew:
			self.proxy.lock(self.file)
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

	def log(self, otherflags):
		print self.proxy.log(self.file, otherflags)

	def add(self):
		self.eseen = 0		# While we're hacking...
		self.esum = self.lsum
		self.emtime, self.ectime = 0, 0
		self.erev = ''
		self.enew = 1
		self.edeleted = 0
		self.eseen = 1		# Done
		self.extra = ''

	def setentry(self, erev, esum):
		self.eseen = 0		# While we're hacking...
		self.esum = esum
		self.emtime, self.ectime = os.stat(self.file)[-2:]
		self.erev = erev
		self.enew = 0
		self.edeleted = 0
		self.eseen = 1		# Done
		self.extra = ''


SENDMAIL = "/usr/lib/sendmail -t"
MAILFORM = """To: %s
Subject: CVS changes: %s

...Message from rcvs...

Committed files:
	%s

Log message:
	%s
"""


class RCVS(CVS):

	FileClass = MyFile

	def __init__(self):
		CVS.__init__(self)

	def update(self, files):
		for e in self.whichentries(files, 1):
			e.update()

	def commit(self, files, message = ""):
		list = self.whichentries(files)
		if not list: return
		ok = 1
		for e in list:
			if not e.commitcheck():
				ok = 0
		if not ok:
			print "correct above errors first"
			return
		if not message:
			message = raw_input("One-liner: ")
		committed = []
		for e in list:
			if e.commit(message):
				committed.append(e.file)
		self.mailinfo(committed, message)

	def mailinfo(self, files, message = ""):
		towhom = "sjoerd@cwi.nl, jack@cwi.nl" # XXX
		mailtext = MAILFORM % (towhom, string.join(files),
					string.join(files), message)
		print '-'*70
		print mailtext
		print '-'*70
		ok = raw_input("OK to mail to %s? " % towhom)
		if string.lower(string.strip(ok)) in ('y', 'ye', 'yes'):
			p = os.popen(SENDMAIL, "w")
			p.write(mailtext)
			sts = p.close()
			if sts:
				print "Sendmail exit status %s" % str(sts)
			else:
				print "Mail sent."
		else:
			print "No mail sent."

	def report(self, files):
		for e in self.whichentries(files):
			e.report()

	def diff(self, files, opts):
		for e in self.whichentries(files):
			e.diff(opts)

	def add(self, files):
		if not files:
			raise RuntimeError, "'cvs add' needs at least one file"
		list = []
		for e in self.whichentries(files, 1):
			e.add()

	def rm(self, files):
		if not files:
			raise RuntimeError, "'cvs rm' needs at least one file"
		raise RuntimeError, "'cvs rm' not yet imlemented"

	def log(self, files, opts):
		flags = ''
		for o, a in opts:
			flags = flags + ' ' + o + a
		for e in self.whichentries(files):
			e.log(flags)

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

	GlobalFlags = 'd:h:p:qvL'
	UsageMessage = \
"usage: rcvs [-d directory] [-h host] [-p port] [-q] [-v] [subcommand arg ...]"
	PostUsageMessage = \
		"If no subcommand is given, the status of all files is listed"

	def __init__(self):
		"""Constructor."""
		CommandFrameWork.__init__(self)
		self.proxy = None
		self.cvs = RCVS()
		
	def close(self):
		if self.proxy:
			self.proxy._close()
		self.proxy = None

	def recurse(self):
		self.close()
		names = os.listdir(os.curdir)
		for name in names:
			if name == os.curdir or name == os.pardir:
				continue
			if name == "CVS":
				continue
			if not os.path.isdir(name):
				continue
			if os.path.islink(name):
				continue
			print "--- entering subdirectory", name, "---"
			os.chdir(name)
			try:
				if os.path.isdir("CVS"):
					self.__class__().run()
				else:
					self.recurse()
			finally:
				os.chdir(os.pardir)
				print "--- left subdirectory", name, "---"

	def options(self, opts):
		self.opts = opts

	def ready(self):
		import rcsclient
		self.proxy = rcsclient.openrcsclient(self.opts)
		self.cvs.setproxy(self.proxy)
		self.cvs.getentries()

	def default(self):
		self.cvs.report([])

	def do_report(self, opts, files):
		self.cvs.report(files)

	def do_update(self, opts, files):
		"""update [-l] [-R] [file] ..."""
		local = DEF_LOCAL
		for o, a in opts:
			if o == '-l': local = 1
			if o == '-R': local = 0
		self.cvs.update(files)
		self.cvs.putentries()
		if not local and not files:
			self.recurse()
	flags_update = '-lR'
	do_up = do_update
	flags_up = flags_update

	def do_commit(self, opts, files):
		"""commit [-m message] [file] ..."""
		message = ""
		for o, a in opts:
			if o == '-m': message = a
		self.cvs.commit(files, message)
		self.cvs.putentries()
	flags_commit = 'm:'
	do_com = do_commit
	flags_com = flags_commit

	def do_diff(self, opts, files):
		"""diff [difflags] [file] ..."""
		self.cvs.diff(files, opts)
	flags_diff = 'cbitwcefhnlr:sD:S:'
	do_dif = do_diff
	flags_dif = flags_diff

	def do_add(self, opts, files):
		"""add file ..."""
		if not files:
			print "'rcvs add' requires at least one file"
			return
		self.cvs.add(files)
		self.cvs.putentries()

	def do_remove(self, opts, files):
		"""remove file ..."""
		if not files:
			print "'rcvs remove' requires at least one file"
			return
		self.cvs.remove(files)
		self.cvs.putentries()
	do_rm = do_remove

	def do_log(self, opts, files):
		"""log [rlog-options] [file] ..."""
		self.cvs.log(files, opts)
	flags_log = 'bhLNRtd:s:V:r:'


def remove(fn):
	try:
		os.unlink(fn)
	except os.error:
		pass


def main():
	r = rcvs()
	try:
		r.run()
	finally:
		r.close()


if __name__ == "__main__":
	main()
