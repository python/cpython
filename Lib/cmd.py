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
			names = dir(self.__class__)
			cmds_doc = []
			cmds_undoc = []
			help = {}
			for name in names:
				if name[:5] == 'help_':
					help[name[5:]]=1
			for name in names:
				if name[:3] == 'do_':
					cmd=name[3:]
					if help.has_key(cmd):
						cmds_doc.append(cmd)
						del help[cmd]
					else:
						cmds_undoc.append(cmd)
			print 
			self.print_topics("Documented commands (type help " \
		                          "<topic>):",cmds_doc, 15, 80)
			self.print_topics("Miscellaneous help topics:",
		                          help.keys(), 15, 80)
			self.print_topics("Undocumented commands:", 
		                          cmds_undoc, 15, 80)

	def print_topics(self, header, cmds, cmdlen, maxcol):
		if cmds:
			print header;
			print "="*len(header)
			(cmds_per_line,junk)=divmod(maxcol,cmdlen)
			col=cmds_per_line
			for cmd in cmds:
				if col==0: print
				print (("%-"+`cmdlen`+"s") % cmd),
				col = (col+1) % cmds_per_line
			print "\n"
