#!/usr/bin/python

import tokenize
import string

TABSONLY = 'TABSONLY'
SPACESONLY = 'SPACESONLY'
MIXED = 'MIXED'

class PyText:
	def __init__(self, fnm, optdict):
		self.optdict = optdict
		self.fnm = fnm
		self.txt = open(self.fnm, 'r').readlines()
		self.indents = [(0, 0, )]
		self.lnndx = 0
		self.indentndx = 0
	def getline(self):
		if self.lnndx < len(self.txt):
			txt = self.txt[self.lnndx]
			self.lnndx = self.lnndx + 1
		else:
			txt = ''
		return txt
	def tokeneater(self, type, token, start, end, line):
		if type == tokenize.INDENT:
			(lvl, s) = self.indents[-1]
			self.indents[-1] = (lvl, s, start[0]-1)
			self.indents.append((lvl+1, start[0]-1,))
		elif type == tokenize.DEDENT:
			(lvl, s) = self.indents[-1]
			self.indents[-1] = (lvl, s, start[0]-1)
			self.indents.append((lvl-1, start[0]-1,))
		elif type == tokenize.ENDMARKER:
			(lvl, s) = self.indents[-1]
			self.indents[-1] = (lvl, s, len(self.txt))
	def split(self, ln):
		content = string.lstrip(ln)
		if not content:
			return ('', '\n')
		lead = ln[:len(ln) - len(content)]
		lead = string.expandtabs(lead)
		return (lead, content)
		
	def process(self):
		style = self.optdict.get('style', TABSONLY)
		indent = string.atoi(self.optdict.get('indent', '4'))
		tabsz = string.atoi(self.optdict.get('tabs', '8'))
		print 'file %s -> style %s, tabsize %d, indent %d' % (self.fnm, style, tabsz, indent)
		tokenize.tokenize(self.getline, self.tokeneater)
		#import pprint
		#pprint.pprint(self.indents)
		new = []
		for (lvl, s, e) in self.indents:
			if s >= len(self.txt):
				break
			if s == e:
				continue
			oldlead, content = self.split(self.txt[s])
			#print "oldlead", len(oldlead), `oldlead`
			if style == TABSONLY:
				newlead = '\t'*lvl
			elif style == SPACESONLY:
				newlead = ' '*(indent*lvl)
			else:
				sz = indent*lvl
				t,spcs = divmod(sz, tabsz)
				newlead = '\t'*t + ' '*spcs
			new.append(newlead + content)
			for ln in self.txt[s+1:e]:
				lead, content = self.split(ln)
				#print "lead:", len(lead)
				new.append(newlead + lead[len(oldlead):] + content)
		self.save(new)
		#print "---", self.fnm
		#for ln in new:
		#    print ln,
		#print
		
	def save(self, txt):
		bakname = os.path.splitext(self.fnm)[0]+'.bak'
		print "backing up", self.fnm, "to", bakname
		#print os.getcwd()
		try:
			os.rename(self.fnm, bakname)
		except os.error:
			os.remove(bakname)
			os.rename(self.fnm, bakname)
		open(self.fnm, 'w').writelines(txt)
		
def test():
	tc = PyText('test1.py')
	tc.process()
	tc = PyText('test1.py')
	tc.process(style=TABSONLY)
	tc = PyText('test1.py')
	tc.process(style=MIXED, indent=4, tabs=8)
	tc = PyText('test1.py')
	tc.process(style=MIXED, indent=2, tabs=8)
	
def cleanfile(fnm, d):
	if os.path.isdir(fnm) and not os.path.islink(fnm):
		names = os.listdir(fnm)
		for name in names:
			fullnm = os.path.join(fnm, name)
			if (os.path.isdir(fullnm) and not os.path.islink(fullnm)) or \
			    os.path.normcase(fullnm[-3:]) == ".py":
				cleanfile(fullnm, d)
		return
	tc = PyText(fnm, d)
	tc.process()
	
usage="""\
%s [options] [path...]
 options
  -T : reformat to TABS ONLY
  -S : reformat to SPACES ONLY ( -i option is important)
  -M : reformat to MIXED SPACES / TABS ( -t and -i options important)
  -t<n> : tab is worth <n> characters
  -i<n> : indents should be <n> characters 
  -h : print this text
 path is file or directory
"""
if __name__ == '__main__':
	import sys, getopt, os
	opts, args = getopt.getopt(sys.argv[1:], "TSMht:i:")
	d = {}
	print `opts`
	for opt in opts:
		if opt[0] == '-T':
			d['style'] = TABSONLY
		elif opt[0] == '-S':
			d['style'] = SPACESONLY
		elif opt[0] == '-M':
			d['style'] = MIXED
		elif opt[0] == '-t':
			d['tabs'] = opt[1]
		elif opt[0] == '-i':
			d['indent'] = opt[1]
		elif opt[0] == '-h':
			print usage % sys.argv[0]
			sys.exit(0)
	if not args:
		print usage % sys.argv[0]
	for arg in args:
		cleanfile(arg, d)
		
		
		
		
		
		
		
		
		
		
		
		
		
		
