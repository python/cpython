"""An attempt at an unweave script.
Jack Jansen, jack@oratrix.com, 13-Dec-00
"""
import re
import sys
import macfs
import os
import macostools

BEGINDEFINITION=re.compile("^<<(?P<name>.*)>>=\s*")
USEDEFINITION=re.compile("^(?P<pre>.*)<<(?P<name>.*)>>(?P<post>[^=].*)")
ENDDEFINITION=re.compile("^@")
GREMLINS=re.compile("[\xa0\xca]")

DEFAULT_CONFIG="""
filepatterns = [
	("^.*\.cp$", ":unweave-src"),
	("^.*\.h$", ":unweave-include"),
]
genlinedirectives = 0
gencomments = 1
"""

class Processor:
	def __init__(self, filename, config={}):
		self.items = {}
		self.filename = filename
		self.fp = open(filename)
		self.lineno = 0
		self.resolving = {}
		self.resolved = {}
		self.pushback = None
		# Options
		if config.has_key("genlinedirectives"):
			self.genlinedirectives = config["genlinedirectives"]
		else:
			self.genlinedirectives = 1
		if config.has_key("gencomments"):
			self.gencomments = config["gencomments"]
		else:
			self.gencomments = 0
		if config.has_key("filepatterns"):
			self.filepatterns = config["filepatterns"]
		else:
			self.filepatterns = []
		self.filepattern_relist = []
		for pat, dummy in self.filepatterns:
			self.filepattern_relist.append(re.compile(pat))
		
	def _readline(self):
		"""Read a line. Allow for pushback"""
		if self.pushback:
			rv = self.pushback
			self.pushback = None
			return rv
		self.lineno = self.lineno + 1
		return self.lineno, self.fp.readline()
		
	def _linedirective(self, lineno):
		"""Return a #line cpp directive for this file position"""
		return '#line %d "%s"\n'%(lineno-3, os.path.split(self.filename)[1])
		
	def _readitem(self):
		"""Read the definition of an item. Insert #line where needed. """
		rv = []
		while 1:
			lineno, line = self._readline()
			if not line:
				break
			if ENDDEFINITION.search(line):
				break
			if BEGINDEFINITION.match(line):
				self.pushback = lineno, line
				break
			mo = USEDEFINITION.match(line)
			if mo:
				pre = mo.group('pre')
				if pre:
##					rv.append((lineno, pre+'\n'))
					rv.append((lineno, pre))
			rv.append((lineno, line))
			if mo:
				post = mo.group('post')
				if post and post != '\n':
					rv.append((lineno, post))
		return rv
		
	def _define(self, name, value):
		"""Define an item, or append to an existing definition"""
		if self.items.has_key(name):
			self.items[name] = self.items[name] + value
		else:
			self.items[name] = value
			
	def read(self):
		"""Read the source file and store all definitions"""
		savedcomment = []
		while 1:
			lineno, line = self._readline()
			if not line: break
			mo = BEGINDEFINITION.search(line)
			if mo:
				name = mo.group('name')
				value = self._readitem()
				if self.gencomments:
					defline = [(lineno, '// <%s>=\n'%name)]
					if savedcomment:
						savedcomment = savedcomment + [(lineno, '//\n')] + defline
					else:
						savedcomment = defline
					savedcomment = self._processcomment(savedcomment)
					value = savedcomment + value
					savedcomment = []
				isfilepattern = 0
				for rexp in self.filepattern_relist:
					if rexp.search(name):
						isfilepattern = 1
						break
				if 0 and not isfilepattern:
					value = self._addspace(value)
				self._define(name, value)
			else:
				if self.gencomments:
					# It seems initial blank lines are ignored:-(
					if savedcomment or line.strip():
						savedcomment.append((lineno, '// '+line))
						
	def _processcomment(self, comment):
		# This routine mimicks some artefact of Matthias' code.
		rv = []
		for lineno, line in comment:
			line = line[:-1]
			line = GREMLINS.subn(' ', line)[0]
			if len(line) < 75:
				line = line + (75-len(line))*' '
			line = line + '\n'
			rv.append((lineno, line))
		return rv
		
	def _addspace(self, value, howmany):
		# Yet another routine to mimick yet another artefact
		rv = value[0:1]
		for lineno, line in value[1:]:
			rv.append((lineno, (' '*howmany)+line))
		return rv
		
	def resolve(self):
		"""Resolve all references"""
		for name in self.items.keys():
			self._resolve_one(name)
			
	def _resolve_one(self, name):
		"""Resolve references in one definition, recursively"""
		# First check for unknown macros and recursive calls
		if not self.items.has_key(name):
			print "Undefined macro:", name
			return ['<<%s>>'%name]
		if self.resolving.has_key(name):
			print "Recursive macro:", name
			return ['<<%s>>'%name]
		# Then check that we haven't handled this one before
		if self.resolved.has_key(name):
			return self.items[name]
		# No rest for the wicked: we have work to do.
		self.resolving[name] = 1
		result = []
		lastlineincomplete = 0
		for lineno, line in self.items[name]:
			mo = USEDEFINITION.search(line)
			if mo:
				# We replace the complete line. Is this correct?
				macro = mo.group('name')
				replacement = self._resolve_one(macro)
				if lastlineincomplete:
					replacement = self._addspace(replacement, lastlineincomplete)
				result = result + replacement
			else:
				result.append((lineno, line))
			if line[-1] == '\n':
				lastlineincomplete = 0
			else:
				lastlineincomplete = len(line)
		self.items[name] = result
		self.resolved[name] = 1
		del self.resolving[name]
		return result
		
	def save(self, dir, pattern):
		"""Save macros that match pattern to folder dir"""
		# Compile the pattern, if needed
		if type(pattern) == type(''):
			pattern = re.compile(pattern)
		# If the directory is relative it is relative to the sourcefile
		if not os.path.isabs(dir):
			sourcedir = os.path.split(self.filename)[0]
			dir = os.path.join(sourcedir, dir)
		for name in self.items.keys():
			if pattern.search(name):
				pathname = os.path.join(dir, name)
				data = self._addlinedirectives(self.items[name])
				self._dosave(pathname, data)
				
	def _addlinedirectives(self, data):
		curlineno = -100
		rv = []
		for lineno, line in data:
			curlineno = curlineno + 1
			if self.genlinedirectives and line and line != '\n' and lineno != curlineno:
				rv.append(self._linedirective(lineno))
				curlineno = lineno
			rv.append(line)
		return rv
		
	def _dosave(self, pathname, data):
		"""Save data to pathname, unless it is identical to what is there"""
		if os.path.exists(pathname):
			olddata = open(pathname).readlines()
			if olddata == data:
				return
		macostools.mkdirs(os.path.split(pathname)[0])
		fp = open(pathname, "w").writelines(data)
		
def process(file, config):
	pr = Processor(file, config)
	pr.read()
	pr.resolve()
	for pattern, folder in config['filepatterns']:
		pr.save(folder, pattern)
	
def readconfig():
	"""Read a configuration file, if it doesn't exist create it."""
	configname = sys.argv[0] + '.config'
	if not os.path.exists(configname):
		confstr = DEFAULT_CONFIG
		open(configname, "w").write(confstr)
		print "Created config file", configname
##		print "Please check and adapt (if needed)"
##		sys.exit(0)
	namespace = {}
	execfile(configname, namespace)
	return namespace

def main():
	config = readconfig()
	if len(sys.argv) > 1:
		for file in sys.argv[1:]:
			if file[-3:] == '.nw':
				print "Processing", file
				process(file, config)
			else:
				print "Skipping", file
	else:
		fss, ok = macfs.PromptGetFile("Select .nw source file", "TEXT")
		if not ok:
			sys.exit(0)
		process(fss.as_pathname(), config)
		
if __name__ == "__main__":
	main()
