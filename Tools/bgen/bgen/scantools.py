"""\

Tools for scanning header files in search of function prototypes.

Often, the function prototypes in header files contain enough information
to automatically generate (or reverse-engineer) interface specifications
from them.  The conventions used are very vendor specific, but once you've
figured out what they are they are often a great help, and it sure beats
manually entering the interface specifications.  (These are needed to generate
the glue used to access the functions from Python.)

In order to make this class useful, almost every component can be overridden.
The defaults are (currently) tuned to scanning Apple Macintosh header files,
although most Mac specific details are contained in header-specific subclasses.
"""

import regex
import regsub
import string
import sys
import os
import fnmatch
from types import *
try:
	import MacOS
except ImportError:
	MacOS = None

from bgenlocations import CREATOR, INCLUDEDIR

Error = "scantools.Error"

class Scanner:

	def __init__(self, input = None, output = None, defsoutput = None):
		self.initsilent()
		self.initblacklists()
		self.initrepairinstructions()
		self.initpaths()
		self.initfiles()
		self.initpatterns()
		self.compilepatterns()
		self.initosspecifics()
		self.initusedtypes()
		if output:
			self.setoutput(output, defsoutput)
		if input:
			self.setinput(input)
	
	def initusedtypes(self):
		self.usedtypes = {}
	
	def typeused(self, type, mode):
		if not self.usedtypes.has_key(type):
			self.usedtypes[type] = {}
		self.usedtypes[type][mode] = None
	
	def reportusedtypes(self):
		types = self.usedtypes.keys()
		types.sort()
		for type in types:
			modes = self.usedtypes[type].keys()
			modes.sort()
			print type, string.join(modes)
			
	def gentypetest(self, file):
		fp = open(file, "w")
		fp.write("types=[\n")
		types = self.usedtypes.keys()
		types.sort()
		for type in types:
			fp.write("\t'%s',\n"%type)
		fp.write("]\n")
		fp.write("""missing=0
for t in types:
	try:
		tt = eval(t)
	except NameError:
		print "** Missing type:", t
		missing = 1
if missing: raise "Missing Types"
""")
		fp.close()

	def initsilent(self):
		self.silent = 0

	def error(self, format, *args):
		if self.silent >= 0:
			print format%args

	def report(self, format, *args):
		if not self.silent:
			print format%args

	def writeinitialdefs(self):
		pass
		
	def initblacklists(self):
		self.blacklistnames = self.makeblacklistnames()
		self.blacklisttypes = ["unknown", "-"] + self.makeblacklisttypes()
		self.greydictnames = self.greylist2dict(self.makegreylist())
		
	def greylist2dict(self, list):
		rv = {}
		for define, namelist in list:
			for name in namelist:
				rv[name] = define
		return rv

	def makeblacklistnames(self):
		return []

	def makeblacklisttypes(self):
		return []
		
	def makegreylist(self):
		return []

	def initrepairinstructions(self):
		self.repairinstructions = self.makerepairinstructions()

	def makerepairinstructions(self):
		"""Parse the repair file into repair instructions.
		
		The file format is simple:
		1) use \ to split a long logical line in multiple physical lines
		2) everything after the first # on a line is ignored (as comment)
		3) empty lines are ignored
		4) remaining lines must have exactly 3 colon-separated fields:
		   functionpattern : argumentspattern : argumentsreplacement
		5) all patterns use shell style pattern matching
		6) an empty functionpattern means the same as *
		7) the other two fields are each comma-separated lists of triples
		8) a triple is a space-separated list of 1-3 words
		9) a triple with less than 3 words is padded at the end with "*" words
		10) when used as a pattern, a triple matches the type, name, and mode
		    of an argument, respectively
		11) when used as a replacement, the words of a triple specify
		    replacements for the corresponding words of the argument,
		    with "*" as a word by itself meaning leave the original word
		    (no other uses of "*" is allowed)
		12) the replacement need not have the same number of triples
		    as the pattern
		"""
		f = self.openrepairfile()
		if not f: return []
		print "Reading repair file", `f.name`, "..."
		list = []
		lineno = 0
		while 1:
			line = f.readline()
			if not line: break
			lineno = lineno + 1
			startlineno = lineno
			while line[-2:] == '\\\n':
				line = line[:-2] + ' ' + f.readline()
				lineno = lineno + 1
			i = string.find(line, '#')
			if i >= 0: line = line[:i]
			words = map(string.strip, string.splitfields(line, ':'))
			if words == ['']: continue
			if len(words) <> 3:
				print "Line", startlineno,
				print ": bad line (not 3 colon-separated fields)"
				print `line`
				continue
			[fpat, pat, rep] = words
			if not fpat: fpat = "*"
			if not pat:
				print "Line", startlineno,
				print "Empty pattern"
				print `line`
				continue
			patparts = map(string.strip, string.splitfields(pat, ','))
			repparts = map(string.strip, string.splitfields(rep, ','))
			patterns = []
			for p in patparts:
				if not p:
					print "Line", startlineno,
					print "Empty pattern part"
					print `line`
					continue
				pattern = string.split(p)
				if len(pattern) > 3:
					print "Line", startlineno,
					print "Pattern part has > 3 words"
					print `line`
					pattern = pattern[:3]
				else:
					while len(pattern) < 3:
						pattern.append("*")
				patterns.append(pattern)
			replacements = []
			for p in repparts:
				if not p:
					print "Line", startlineno,
					print "Empty replacement part"
					print `line`
					continue
				replacement = string.split(p)
				if len(replacement) > 3:
					print "Line", startlineno,
					print "Pattern part has > 3 words"
					print `line`
					replacement = replacement[:3]
				else:
					while len(replacement) < 3:
						replacement.append("*")
				replacements.append(replacement)
			list.append((fpat, patterns, replacements))
		return list
	
	def openrepairfile(self, filename = "REPAIR"):
		try:
			return open(filename, "r")
		except IOError, msg:
			print `filename`, ":", msg
			print "Cannot open repair file -- assume no repair needed"
			return None

	def initfiles(self):
		self.specmine = 0
		self.defsmine = 0
		self.scanmine = 0
		self.specfile = sys.stdout
		self.defsfile = None
		self.scanfile = sys.stdin
		self.lineno = 0
		self.line = ""

	def initpaths(self):
		self.includepath = [':', INCLUDEDIR]

	def initpatterns(self):
#		self.head_pat = "^extern pascal[ \t]+" # XXX Mac specific!
		self.head_pat = "^EXTERN_API[^_]"
		self.tail_pat = "[;={}]"
#		self.type_pat = "pascal[ \t\n]+\(<type>[a-zA-Z0-9_ \t]*[a-zA-Z0-9_]\)[ \t\n]+"
		self.type_pat = "EXTERN_API" + \
						"[ \t\n]*([ \t\n]*" + \
						"\(<type>[a-zA-Z0-9_* \t]*[a-zA-Z0-9_*]\)" + \
						"[ \t\n]*)[ \t\n]*"
		self.name_pat = "\(<name>[a-zA-Z0-9_]+\)[ \t\n]*"
		self.args_pat = "(\(<args>\([^(;=)]+\|([^(;=)]*)\)*\))"
		self.whole_pat = self.type_pat + self.name_pat + self.args_pat
#		self.sym_pat = "^[ \t]*\(<name>[a-zA-Z0-9_]+\)[ \t]*=" + \
#		               "[ \t]*\(<defn>[-0-9'\"(][^\t\n,;}]*\),?"
		self.sym_pat = "^[ \t]*\(<name>[a-zA-Z0-9_]+\)[ \t]*=" + \
		               "[ \t]*\(<defn>[-0-9_a-zA-Z'\"(][^\t\n,;}]*\),?"
		self.asplit_pat = "^\(<type>.*[^a-zA-Z0-9_]\)\(<name>[a-zA-Z0-9_]+\)$"
		self.comment1_pat = "\(<rest>.*\)//.*"
		# note that the next pattern only removes comments that are wholly within one line
		self.comment2_pat = "\(<rest>.*\)/\*.*\*/"

	def compilepatterns(self):
		for name in dir(self):
			if name[-4:] == "_pat":
				pat = getattr(self, name)
				prog = regex.symcomp(pat)
				setattr(self, name[:-4], prog)

	def initosspecifics(self):
		if MacOS:
			self.filetype = 'TEXT'
			self.filecreator = CREATOR
		else:
			self.filetype = self.filecreator = None

	def setfiletype(self, filename):
		if MacOS and (self.filecreator or self.filetype):
			creator, type = MacOS.GetCreatorAndType(filename)
			if self.filecreator: creator = self.filecreator
			if self.filetype: type = self.filetype
			MacOS.SetCreatorAndType(filename, creator, type)

	def close(self):
		self.closefiles()

	def closefiles(self):
		self.closespec()
		self.closedefs()
		self.closescan()

	def closespec(self):
		tmp = self.specmine and self.specfile
		self.specfile = None
		if tmp: tmp.close()

	def closedefs(self):
		tmp = self.defsmine and self.defsfile
		self.defsfile = None
		if tmp: tmp.close()

	def closescan(self):
		tmp = self.scanmine and self.scanfile
		self.scanfile = None
		if tmp: tmp.close()

	def setoutput(self, spec, defs = None):
		self.closespec()
		self.closedefs()
		if spec:
			if type(spec) == StringType:
				file = self.openoutput(spec)
				mine = 1
			else:
				file = spec
				mine = 0
			self.specfile = file
			self.specmine = mine
		if defs:
			if type(defs) == StringType:
				file = self.openoutput(defs)
				mine = 1
			else:
				file = defs
				mine = 0
			self.defsfile = file
			self.defsmine = mine

	def openoutput(self, filename):
		try:
			file = open(filename, 'w')
		except IOError, arg:
			raise IOError, (filename, arg)
		self.setfiletype(filename)
		return file

	def setinput(self, scan = sys.stdin):
		self.closescan()
		if scan:
			if type(scan) == StringType:
				file = self.openinput(scan)
				mine = 1
			else:
				file = scan
				mine = 0
			self.scanfile = file
			self.scanmine = mine
		self.lineno = 0

	def openinput(self, filename):
		if not os.path.isabs(filename):
			for dir in self.includepath:
				fullname = os.path.join(dir, filename)
				#self.report("trying full name %s", `fullname`)
				try:
					return open(fullname, 'r')
				except IOError:
					pass
		# If not on the path, or absolute, try default open()
		try:
			return open(filename, 'r')
		except IOError, arg:
			raise IOError, (arg, filename)

	def getline(self):
		if not self.scanfile:
			raise Error, "input file not set"
		self.line = self.scanfile.readline()
		if not self.line:
			raise EOFError
		self.lineno = self.lineno + 1
		return self.line

	def scan(self):
		if not self.scanfile:
			self.error("No input file has been specified")
			return
		inputname = self.scanfile.name
		self.report("scanfile = %s", `inputname`)
		if not self.specfile:
			self.report("(No interface specifications will be written)")
		else:
			self.report("specfile = %s", `self.specfile.name`)
			self.specfile.write("# Generated from %s\n\n" % `inputname`)
		if not self.defsfile:
			self.report("(No symbol definitions will be written)")
		else:
			self.report("defsfile = %s", `self.defsfile.name`)
			self.defsfile.write("# Generated from %s\n\n" % `inputname`)
			self.writeinitialdefs()
		self.alreadydone = []
		try:
			while 1:
				try: line = self.getline()
				except EOFError: break
				if self.comment1.match(line) >= 0:
					line = self.comment1.group('rest')
				if self.comment2.match(line) >= 0:
					line = self.comment2.group('rest')
				if self.defsfile and self.sym.match(line) >= 0:
					self.dosymdef()
					continue
				if self.head.match(line) >= 0:
					self.dofuncspec()
					continue
		except EOFError:
			self.error("Uncaught EOF error")
		self.reportusedtypes()

	def dosymdef(self):
		name, defn = self.sym.group('name', 'defn')
		if not name in self.blacklistnames:
			self.defsfile.write("%s = %s\n" % (name, defn))
		else:
			self.defsfile.write("# %s = %s\n" % (name, defn))
		# XXXX No way to handle greylisted names

	def dofuncspec(self):
		raw = self.line
		while self.tail.search(raw) < 0:
			line = self.getline()
			raw = raw + line
		self.processrawspec(raw)

	def processrawspec(self, raw):
		if self.whole.search(raw) < 0:
			self.report("Bad raw spec: %s", `raw`)
			return
		type, name, args = self.whole.group('type', 'name', 'args')
		type = regsub.gsub("\*", " ptr", type)
		type = regsub.gsub("[ \t]+", "_", type)
		if name in self.alreadydone:
			self.report("Name has already been defined: %s", `name`)
			return
		self.report("==> %s %s <==", type, name)
		if self.blacklisted(type, name):
			self.error("*** %s %s blacklisted", type, name)
			return
		returnlist = [(type, name, 'ReturnMode')]
		returnlist = self.repairarglist(name, returnlist)
		[(type, name, returnmode)] = returnlist
		arglist = self.extractarglist(args)
		arglist = self.repairarglist(name, arglist)
		if self.unmanageable(type, name, arglist):
			##for arg in arglist:
			##	self.report("    %s", `arg`)
			self.error("*** %s %s unmanageable", type, name)
			return
		self.alreadydone.append(name)
		self.generate(type, name, arglist)

	def extractarglist(self, args):
		args = string.strip(args)
		if not args or args == "void":
			return []
		parts = map(string.strip, string.splitfields(args, ","))
		arglist = []
		for part in parts:
			arg = self.extractarg(part)
			arglist.append(arg)
		return arglist

	def extractarg(self, part):
		mode = "InMode"
		if self.asplit.match(part) < 0:
			self.error("Indecipherable argument: %s", `part`)
			return ("unknown", part, mode)
		type, name = self.asplit.group('type', 'name')
		type = regsub.gsub("\*", " ptr ", type)
		type = string.strip(type)
		type = regsub.gsub("[ \t]+", "_", type)
		return self.modifyarg(type, name, mode)
	
	def modifyarg(self, type, name, mode):
		if type[:6] == "const_":
			type = type[6:]
		elif type[-4:] == "_ptr":
			type = type[:-4]
			mode = "OutMode"
		if type[-4:] == "_far":
			type = type[:-4]
		return type, name, mode

	def repairarglist(self, functionname, arglist):
		arglist = arglist[:]
		i = 0
		while i < len(arglist):
			for item in self.repairinstructions:
				if len(item) == 2:
					pattern, replacement = item
					functionpat = "*"
				else:
					functionpat, pattern, replacement = item
				if not fnmatch.fnmatchcase(functionname, functionpat):
					continue
				n = len(pattern)
				if i+n > len(arglist): continue
				current = arglist[i:i+n]
				for j in range(n):
					if not self.matcharg(pattern[j], current[j]):
						break
				else: # All items of the pattern match
					new = self.substituteargs(
							pattern, replacement, current)
					if new is not None:
						arglist[i:i+n] = new
						i = i+len(new) # No recursive substitutions
						break
			else: # No patterns match
				i = i+1
		return arglist
	
	def matcharg(self, patarg, arg):
		return len(filter(None, map(fnmatch.fnmatchcase, arg, patarg))) == 3

	def substituteargs(self, pattern, replacement, old):
		new = []
		for k in range(len(replacement)):
			item = replacement[k]
			newitem = [item[0], item[1], item[2]]
			for i in range(3):
				if item[i] == '*':
					newitem[i] = old[k][i]
				elif item[i][:1] == '$':
					index = string.atoi(item[i][1:]) - 1
					newitem[i] = old[index][i]
			new.append(tuple(newitem))
		##self.report("old: %s", `old`)
		##self.report("new: %s", `new`)
		return new

	def generate(self, type, name, arglist):
		self.typeused(type, 'return')
		classname, listname = self.destination(type, name, arglist)
		if not self.specfile: return
		self.specfile.write("f = %s(%s, %s,\n" % (classname, type, `name`))
		for atype, aname, amode in arglist:
			self.typeused(atype, amode)
			self.specfile.write("    (%s, %s, %s),\n" %
			                    (atype, `aname`, amode))
		if self.greydictnames.has_key(name):
			self.specfile.write("    condition=%s,\n"%`self.greydictnames[name]`)
		self.specfile.write(")\n")
		self.specfile.write("%s.append(f)\n\n" % listname)

	def destination(self, type, name, arglist):
		return "FunctionGenerator", "functions"

	def blacklisted(self, type, name):
		if type in self.blacklisttypes:
			##self.report("return type %s is blacklisted", type)
			return 1
		if name in self.blacklistnames:
			##self.report("function name %s is blacklisted", name)
			return 1
		return 0

	def unmanageable(self, type, name, arglist):
		for atype, aname, amode in arglist:
			if atype in self.blacklisttypes:
				self.report("argument type %s is blacklisted", atype)
				return 1
		return 0

class Scanner_PreUH3(Scanner):
	"""Scanner for Universal Headers before release 3"""
	def initpatterns(self):
		Scanner.initpatterns(self)
		self.head_pat = "^extern pascal[ \t]+" # XXX Mac specific!
		self.tail_pat = "[;={}]"
		self.type_pat = "pascal[ \t\n]+\(<type>[a-zA-Z0-9_ \t]*[a-zA-Z0-9_]\)[ \t\n]+"
		self.name_pat = "\(<name>[a-zA-Z0-9_]+\)[ \t\n]*"
		self.args_pat = "(\(<args>\([^(;=)]+\|([^(;=)]*)\)*\))"
		self.whole_pat = self.type_pat + self.name_pat + self.args_pat
		self.sym_pat = "^[ \t]*\(<name>[a-zA-Z0-9_]+\)[ \t]*=" + \
		               "[ \t]*\(<defn>[-0-9'\"][^\t\n,;}]*\),?"
		self.asplit_pat = "^\(<type>.*[^a-zA-Z0-9_]\)\(<name>[a-zA-Z0-9_]+\)$"

	
def test():
	input = "D:Development:THINK C:Mac #includes:Apple #includes:AppleEvents.h"
	output = "@aespecs.py"
	defsoutput = "@aedefs.py"
	s = Scanner(input, output, defsoutput)
	s.scan()

if __name__ == '__main__':
	test()

