import os
import sys
import string
from Tkinter import *
from ScrolledText import ScrolledText
from Dialog import Dialog
import signal

TK_READABLE  = 1
TK_WRITABLE  = 2
TK_EXCEPTION = 4

BUFSIZE = 512

class ShellWindow(ScrolledText):

	def __init__(self, master = None, cnf = {}):
		try:
			shell = cnf['shell']
			del cnf['shell']
		except KeyError:
			try:
				shell = os.environ['SHELL']
			except KeyError:
				shell = '/bin/sh'
			shell = shell + ' -i'
		args = string.split(shell)
		shell = args[0]

		ScrolledText.__init__(self, master, cnf)
		self.pos = '1.0'
		self.bind('<Return>', self.inputhandler)
		self.bind('<Control-c>', self.sigint)
		self.bind('<Control-t>', self.sigterm)
		self.bind('<Control-k>', self.sigkill)
		self.bind('<Control-d>', self.sendeof)

		self.pid, self.fromchild, self.tochild = spawn(shell, args)
		self.tk.createfilehandler(self.fromchild, TK_READABLE,
					  self.outputhandler)

	def outputhandler(self, file, mask):
		data = os.read(file, BUFSIZE)
		if not data:
			self.tk.deletefilehandler(file)
			pid, sts = os.waitpid(self.pid, 0)
			print 'pid', pid, 'status', sts
			self.pid = None
			detail = sts>>8
			cause = sts & 0xff
			if cause == 0:
				msg = "exit status %d" % detail
			else:
				msg = "killed by signal %d" % (cause & 0x7f)
				if cause & 0x80:
					msg = msg + " -- core dumped"
			Dialog(self.master, {
				'text': msg,
				'title': "Exit status",
				'bitmap': 'warning',
				'default': 0,
				'strings': ('OK',),
			})
			return
		self.insert('end', data)
		self.pos = self.index('end')
		self.yview_pickplace('end')

	def inputhandler(self, *args):
		if not self.pid:
			Dialog(self.master, {
				'text': "No active process",
				'title': "No process",
				'bitmap': 'error',
				'default': 0,
				'strings': ('OK',),
			})
			return
		self.insert('end', '\n')
		line = self.get(self.pos, 'end')
		self.pos = self.index('end')
		os.write(self.tochild, line)

	def sendeof(self, *args):
		if not self.pid:
			Dialog(self.master, {
				'text': "No active process",
				'title': "No process",
				'bitmap': 'error',
				'default': 0,
				'strings': ('OK',),
			})
			return
		os.close(self.tochild)

	def sendsig(self, sig):
		if not self.pid:
			Dialog(self.master, {
				'text': "No active process",
				'title': "No process",
				'bitmap': 'error',
				'default': 0,
				'strings': ('OK',),
			})
			return
		os.kill(self.pid, sig)

	def sigint(self, *args):
		self.sendsig(signal.SIGINT)

	def sigquit(self, *args):
		self.sendsig(signal.SIGQUIT)

	def sigterm(self, *args):
		self.sendsig(signal.SIGTERM)

	def sigkill(self, *args):
		self.sendsig(signal.SIGKILL)

MAXFD = 100	# Max number of file descriptors (os.getdtablesize()???)

def spawn(prog, args):
	p2cread, p2cwrite = os.pipe()
	c2pread, c2pwrite = os.pipe()
	pid = os.fork()
	if pid == 0:
		# Child
		os.close(0)
		os.close(1)
		os.close(2)
		if os.dup(p2cread) <> 0:
			sys.stderr.write('popen2: bad read dup\n')
		if os.dup(c2pwrite) <> 1:
			sys.stderr.write('popen2: bad write dup\n')
		if os.dup(c2pwrite) <> 2:
			sys.stderr.write('popen2: bad write dup\n')
		for i in range(3, MAXFD):
			try:
				os.close(i)
			except:
				pass
		try:
			os.execvp(prog, args)
		finally:
			print 'execvp failed'
			os._exit(1)
	os.close(p2cread)
	os.close(c2pwrite)
	return pid, c2pread, p2cwrite

def test():
	shell = string.join(sys.argv[1:])
	cnf = {}
	if shell:
		cnf['shell'] = shell
	root = Tk()
	root.minsize(1, 1)
	w = ShellWindow(root, cnf)
	w.pack({'expand': 1, 'fill': 'both'})
	w.focus_set()
	w.tk.mainloop()

if __name__ == '__main__':
	test()
