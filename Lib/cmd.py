# A generic class to build line-oriented command interpreters

import string
import sys
import linecache

PROMPT = '(Cmd) '
IDENTCHARS = string.letters + string.digits + '_'

class Cmd:

	def __init__(self):
		self.prompt = PROMPT
		self.identchars = IDENTCHARS
		self.lastcmd = ''

	def cmdloop(self):
		stop = None
		while not stop:
			try:
				line = raw_input(self.prompt)
			except EOFError:
				line = 'EOF'
			stop = self.onecmd(line)

	def onecmd(self, line):
		line = string.strip(line)
		if not line:
			line = self.lastcmd
		else:
			self.lastcmd = line
		i, n = 0, len(line)
		while i < n and line[i] in self.identchars: i = i+1
		cmd, arg = line[:i], string.strip(line[i:])
		if cmd == '':
			return self.default(line)
		else:
			try:
				func = getattr(self, 'do_' + cmd)
			except AttributeError:
				return self.default(line)
			return func(arg)

	def default(self, line):
		print '*** Unknown syntax:', line

	def do_help(self, arg):
		if arg:
			# XXX check arg syntax
			try:
				func = getattr(self, 'help_' + arg)
			except:
				print '*** No help on', `arg`
				return
			func()
		else:
			import newdir
			names = newdir.dir(self.__class__)
			cmds = []
			for name in names:
				if name[:3] == 'do_':
					cmds.append(name[3:])
			print cmds
