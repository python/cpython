#! /usr/local/bin/python

"""Remote CVS -- command line interface"""

from cvslib import CVS, File
import md5
import os
import string
import sys
from cmdfw import CommandFrameWork


class MyFile(File):

	def update(self):
		print self.file, '...'
		if self.lsum == self.esum == self.rsum:
			print '=', self.file
			return
		if self.lsum and not self.erev and not self.rrev:
			print '?', self.file
			return
		print 'X', self.file
		


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
		self.cvs.report()

	def do_update(self, opts, files):
		self.cvs.checkfiles(files)
		if not files:
			print "no files"
			return
		for file in files:
			if not self.cvs.entries.has_key(file):
				print "%s: not found" % file
			else:
				self.cvs.entries[file].update()


def main():
	rcvs().run()


if __name__ == "__main__":
	main()
