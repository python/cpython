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
		'M' -- changed locally, no changes remotely
		'C' -- conflict: changed locally as well as remotely
		       (includes cases where the file has been added
		       or removed locally and remotely)
		"""
		if not self.eseen:
			pass
		return '?'

	def update(self):
		code = self.action()
		print code, self.file
		if code == 'U':
			self.get()
		elif code == 'C':
			print "%s: conflict resolution not yet implemented" % \
			      self.file

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
		self.cvs.report()

	def do_update(self, opts, files):
		"""update [file] ..."""
		if self.cvs.checkfiles(files):
			return 1
		for file in files:
			if not self.cvs.entries.has_key(file):
				print "%s: not found" % file
			else:
				self.cvs.entries[file].update()

	def do_commit(self, opts, files):
		"""commit [file] ..."""
		if self.cvs.checkfiles(files):
			return 1
		sts = 0
		for file in files:
			if not self.entries[file].commitcheck():
				sts = 1
		if sts:
			return sts
		message = raw_input("One-liner: ")
		for file in files:
			self.entries[file].commit(message)


def main():
	rcvs().run()


if __name__ == "__main__":
	main()
