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
		print code, self.file
		if code in ('U', 'N'):
			self.get()
		elif code == 'C':
			print "%s: conflict resolution not yet implemented" % \
			      self.file
		elif code == 'D':
			try:
				os.unlink(self.file)
			except os.error:
				pass
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

	def commitcheck(self):
		return self.action() != 'C'

	def put(self, message = ""):
		print "%s: put not yet implemented" % self.file
	
	def get(self):
		data = self.proxy.get(self.file)
		f = open(self.file, 'w')
		f.write(data)
		f.close()
		self.eseen = 1
		self.esum = self.rsum
		self.emtime, self.ectime = os.stat(self.file)[-2:]
		self.erev = self.rrev
		self.enew = 0
		self.edeleted = 0
		# XXX anything else?


class RCVS(CVS):

	FileClass = MyFile

	def __init__(self):
		CVS.__init__(self)

	def checkfiles(self, files):
		if not files:
			def ok(file, self=self):
				e = self.entries[file]
				return e.eseen or e.rseen
			files[:] = filter(ok, self.entries.keys())
			files.sort()
			if not files:
				print "no files to be processed"
				return 1
			else:
				return None
		else:
			sts = None
			for file in files:
				if not self.entries.has_key(file):
					print "%s: nothing known" % file
					sts = 1
			return sts


class rcvs(CommandFrameWork):

	GlobalFlags = 'd:h:p:qv'
	UsageMessage = \
  "usage: rcvs [-d directory] [-h host] [-p port] [-q] [-v] subcommand arg ..."

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
		self.cvs.getlocalfiles()
		self.cvs.getremotefiles(self.proxy)

	def default(self):
		files = []
		if self.cvs.checkfiles(files):
			return 1
		for file in files:
			print self.cvs.entries[file].action(), file

	def do_update(self, opts, files):
		"""update [file] ..."""
		if self.cvs.checkfiles(files):
			return 1
		for file in files:
			if not self.cvs.entries.has_key(file):
				print "%s: not found" % file
			else:
				self.cvs.entries[file].update()
		self.cvs.putentries()

	def do_commit(self, opts, files):
		"""commit [file] ..."""
		if self.cvs.checkfiles(files):
			return 1
		sts = 0
		for file in files:
			if not self.cvs.entries[file].commitcheck():
				sts = 1
		if sts:
			return sts
		message = raw_input("One-liner: ")
		for file in files:
			self.cvs.entries[file].commit(message)
		self.cvs.putentries()


def main():
	rcvs().run()


if __name__ == "__main__":
	main()
