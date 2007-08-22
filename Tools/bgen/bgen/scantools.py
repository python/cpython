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

import re
import sys
import os
import fnmatch
from types import *
try:
    import MacOS
except ImportError:
    MacOS = None

try:
    from bgenlocations import CREATOR, INCLUDEDIR
except ImportError:
    CREATOR = None
    INCLUDEDIR = os.curdir

Error = "scantools.Error"

BEGINHTMLREPORT="""<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<html>
<head>
<style type="text/css">
.unmatched {  }
.commentstripping { color: grey; text-decoration: line-through }
.comment { text-decoration: line-through }
.notcomment { color: black }
.incomplete { color: maroon }
.constant { color: green }
.pyconstant { background-color: yellow }
.blconstant { background-color: yellow; color: red }
.declaration { color: blue }
.pydeclaration { background-color: yellow }
.type { font-style: italic }
.name { font-weight: bold }
.value { font-style: italic }
.arglist { text-decoration: underline }
.blacklisted { background-color: yellow; color: red }
</style>
<title>Bgen scan report</title>
</head>
<body>
<h1>Bgen scan report</h1>
<h2>Legend</h2>
<p>This scan report is intended to help you debug the regular expressions
used by the bgen scanner. It consists of the original ".h" header file(s)
marked up to show you what the regular expressions in the bgen parser matched
for each line. NOTE: comments in the original source files may or may not be
shown.</p>
<p>The typographic conventions of this file are as follows:</p>
<dl>
<dt>comment stripping</dt>
<dd><pre><span class="commentstripping"><span class="notcomment">comment stripping is </span><span class="comment">/* marked up */</span><span class="notcomment"> and the line is repeated if needed</span></span></pre>
<p>If anything here does not appear to happen correctly look at
<tt>comment1_pat</tt> and <tt>comment2_pat</tt>.</p>
</dd>
<dt>constant definitions</dt>
<dd><pre><span class="constant">#define <span class="name">name</span> <span class="value">value</span></pre>
<p>Highlights name and value of the constant. Governed by <tt>sym_pat</tt>.</p>
</dd>
<dt>function declaration</dt>
<dd><pre><span class="declaration"><span class="type">char *</span><span class="name">rindex</span><span class="arglist">(<span class="type">const char *</span><span class="name">s</span>, <span class="type">int </span><span class="name">c</span>)</span>;</span></pre>
<p>Highlights type, name and argument list. <tt>type_pat</tt>,
<tt>name_pat</tt> and <tt>args_pat</tt> are combined into <tt>whole_pat</tt>, which
is what is used here.</p></dd>
</dd>
<dt>incomplete match for function declaration</dt>
<dd><pre><span class="incomplete"><span class="type">char *</span>foo;</span></pre>
<p>The beginning of this looked promising, but it did not match a function declaration.
In other words, it matched <tt>head_pat</tt> but not <tt>whole_pat</tt>. If the next
declaration has also been gobbled up you need to look at <tt>end_pat</tt>.</p>
</dd>
<dt>unrecognized input</dt>
<dd><pre><span class="unmatched">#include "type.h"</span></pre>
<p>If there are function declarations the scanner has missed (i.e. things
are in this class but you want them to be declarations) you need to adapt
<tt>head_pat</tt>.
</dd>
</dl>
<h2>Output</h2>
<pre>
<span class="unmatched">
"""
ENDHTMLREPORT="""</span>
</pre>
</body>
</html>
"""

class Scanner:

    # Set to 1 in subclass to debug your scanner patterns.
    debug = 0

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
            self.report("%s %s", type, " ".join(modes))

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
        self.silent = 1

    def error(self, format, *args):
        if self.silent >= 0:
            print(format%args)

    def report(self, format, *args):
        if not self.silent:
            print(format%args)

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
        self.inherentpointertypes = self.makeinherentpointertypes()

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
        print("Reading repair file", repr(f.name), "...")
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
            i = line.find('#')
            if i >= 0: line = line[:i]
            words = [s.strip() for s in line.split(':')]
            if words == ['']: continue
            if len(words) <> 3:
                print("Line", startlineno, end=' ')
                print(": bad line (not 3 colon-separated fields)")
                print(repr(line))
                continue
            [fpat, pat, rep] = words
            if not fpat: fpat = "*"
            if not pat:
                print("Line", startlineno, end=' ')
                print("Empty pattern")
                print(repr(line))
                continue
            patparts = [s.strip() for s in pat.split(',')]
            repparts = [s.strip() for s in rep.split(',')]
            patterns = []
            for p in patparts:
                if not p:
                    print("Line", startlineno, end=' ')
                    print("Empty pattern part")
                    print(repr(line))
                    continue
                pattern = p.split()
                if len(pattern) > 3:
                    print("Line", startlineno, end=' ')
                    print("Pattern part has > 3 words")
                    print(repr(line))
                    pattern = pattern[:3]
                else:
                    while len(pattern) < 3:
                        pattern.append("*")
                patterns.append(pattern)
            replacements = []
            for p in repparts:
                if not p:
                    print("Line", startlineno, end=' ')
                    print("Empty replacement part")
                    print(repr(line))
                    continue
                replacement = p.split()
                if len(replacement) > 3:
                    print("Line", startlineno, end=' ')
                    print("Pattern part has > 3 words")
                    print(repr(line))
                    replacement = replacement[:3]
                else:
                    while len(replacement) < 3:
                        replacement.append("*")
                replacements.append(replacement)
            list.append((fpat, patterns, replacements))
        return list

    def makeinherentpointertypes(self):
        return []

    def openrepairfile(self, filename = "REPAIR"):
        try:
            return open(filename, "rU")
        except IOError as msg:
            print(repr(filename), ":", msg)
            print("Cannot open repair file -- assume no repair needed")
            return None

    def initfiles(self):
        self.specmine = 0
        self.defsmine = 0
        self.scanmine = 0
        self.htmlmine = 0
        self.specfile = sys.stdout
        self.defsfile = None
        self.scanfile = sys.stdin
        self.htmlfile = None
        self.lineno = 0
        self.line = ""

    def initpaths(self):
        self.includepath = [os.curdir, INCLUDEDIR]

    def initpatterns(self):
        self.head_pat = r"^EXTERN_API[^_]"
        self.tail_pat = r"[;={}]"
        self.type_pat = r"EXTERN_API" + \
                        r"[ \t\n]*\([ \t\n]*" + \
                        r"(?P<type>[a-zA-Z0-9_* \t]*[a-zA-Z0-9_*])" + \
                        r"[ \t\n]*\)[ \t\n]*"
        self.name_pat = r"(?P<name>[a-zA-Z0-9_]+)[ \t\n]*"
        self.args_pat = r"\((?P<args>([^\(;=\)]+|\([^\(;=\)]*\))*)\)"
        self.whole_pat = self.type_pat + self.name_pat + self.args_pat
        self.sym_pat = r"^[ \t]*(?P<name>[a-zA-Z0-9_]+)[ \t]*=" + \
                       r"[ \t]*(?P<defn>[-0-9_a-zA-Z'\"\(][^\t\n,;}]*),?"
        self.asplit_pat = r"^(?P<type>.*[^a-zA-Z0-9_])(?P<name>[a-zA-Z0-9_]+)(?P<array>\[\])?$"
        self.comment1_pat = r"(?P<rest>.*)//.*"
        # note that the next pattern only removes comments that are wholly within one line
        self.comment2_pat = r"(?P<rest1>.*)/\*.*\*/(?P<rest2>.*)"

    def compilepatterns(self):
        for name in dir(self):
            if name[-4:] == "_pat":
                pat = getattr(self, name)
                prog = re.compile(pat)
                setattr(self, name[:-4], prog)

    def initosspecifics(self):
        if MacOS and CREATOR:
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
        self.closehtml()

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

    def closehtml(self):
        if self.htmlfile: self.htmlfile.write(ENDHTMLREPORT)
        tmp = self.htmlmine and self.htmlfile
        self.htmlfile = None
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

    def sethtmloutput(self, htmlfile):
        self.closehtml()
        if htmlfile:
            if type(htmlfile) == StringType:
                file = self.openoutput(htmlfile)
                mine = 1
            else:
                file = htmlfile
                mine = 0
            self.htmlfile = file
            self.htmlmine = mine
            self.htmlfile.write(BEGINHTMLREPORT)

    def openoutput(self, filename):
        try:
            file = open(filename, 'w')
        except IOError as arg:
            raise IOError(filename, arg)
        self.setfiletype(filename)
        return file

    def setinput(self, scan = sys.stdin):
        if not type(scan) in (TupleType, ListType):
            scan = [scan]
        self.allscaninputs = scan
        self._nextinput()

    def _nextinput(self):
        if not self.allscaninputs:
            return 0
        scan = self.allscaninputs[0]
        self.allscaninputs = self.allscaninputs[1:]
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
        return 1

    def openinput(self, filename):
        if not os.path.isabs(filename):
            for dir in self.includepath:
                fullname = os.path.join(dir, filename)
                #self.report("trying full name %r", fullname)
                try:
                    return open(fullname, 'rU')
                except IOError:
                    pass
        # If not on the path, or absolute, try default open()
        try:
            return open(filename, 'rU')
        except IOError as arg:
            raise IOError(arg, filename)

    def getline(self):
        if not self.scanfile:
            raise Error("input file not set")
        self.line = self.scanfile.readline()
        if not self.line:
            if self._nextinput():
                return self.getline()
            raise EOFError
        self.lineno = self.lineno + 1
        return self.line

    def scan(self):
        if not self.scanfile:
            self.error("No input file has been specified")
            return
        inputname = self.scanfile.name
        self.report("scanfile = %r", inputname)
        if not self.specfile:
            self.report("(No interface specifications will be written)")
        else:
            self.report("specfile = %r", self.specfile.name)
            self.specfile.write("# Generated from %r\n\n" % (inputname,))
        if not self.defsfile:
            self.report("(No symbol definitions will be written)")
        else:
            self.report("defsfile = %r", (self.defsfile.name,))
            self.defsfile.write("# Generated from %r\n\n" % (os.path.split(inputname)[1],))
            self.writeinitialdefs()
        self.alreadydone = []
        try:
            while 1:
                try: line = self.getline()
                except EOFError: break
                if self.debug:
                    self.report("LINE: %r" % (line,))
                match = self.comment1.match(line)
                if match:
                    self.htmlreport(line, klass='commentstripping', ranges=[(
                        match.start('rest'), match.end('rest'), 'notcomment')])
                    line = match.group('rest')
                    if self.debug:
                        self.report("\tafter comment1: %r" % (line,))
                match = self.comment2.match(line)
                while match:
                    if match:
                        self.htmlreport(line, klass='commentstripping', ranges=[
                            (match.start('rest1'), match.end('rest1'), 'notcomment'),
                            (match.start('rest2'), match.end('rest2'), 'notcomment')])
                    line = match.group('rest1')+match.group('rest2')
                    if self.debug:
                        self.report("\tafter comment2: %r" % (line,))
                    match = self.comment2.match(line)
                if self.defsfile:
                    match = self.sym.match(line)
                    if match:
                        if self.debug:
                            self.report("\tmatches sym.")
                        self.dosymdef(match, line)
                        continue
                match = self.head.match(line)
                if match:
                    if self.debug:
                        self.report("\tmatches head.")
                    self.dofuncspec()
                    continue
                self.htmlreport(line, klass='unmatched')
        except EOFError:
            self.error("Uncaught EOF error")
        self.reportusedtypes()

    def dosymdef(self, match, line):
        name, defn = match.group('name', 'defn')
        self.htmlreport(line, klass='constant', ranges=[
            (match.start('name'), match.end('name'), 'name'),
            (match.start('defn'), match.end('defn'), 'value')])
        defn = escape8bit(defn)
        if self.debug:
            self.report("\tsym: name=%r, defn=%r" % (name, defn))
        if not name in self.blacklistnames:
            oline = "%s = %s\n" % (name, defn)
            self.defsfile.write(oline)
            self.htmlreport(oline, klass="pyconstant")
        else:
            self.defsfile.write("# %s = %s\n" % (name, defn))
            self.htmlreport("** no output: name is blacklisted", klass="blconstant")
        # XXXX No way to handle greylisted names

    def dofuncspec(self):
        raw = self.line
        while not self.tail.search(raw):
            line = self.getline()
            if self.debug:
                self.report("* CONTINUATION LINE: %r" % (line,))
            match = self.comment1.match(line)
            if match:
                line = match.group('rest')
                if self.debug:
                    self.report("\tafter comment1: %r" % (line,))
            match = self.comment2.match(line)
            while match:
                line = match.group('rest1')+match.group('rest2')
                if self.debug:
                    self.report("\tafter comment1: %r" % (line,))
                match = self.comment2.match(line)
            raw = raw + line
        if self.debug:
            self.report("* WHOLE LINE: %r" % (raw,))
        self.processrawspec(raw)
        return raw

    def processrawspec(self, raw):
        match = self.whole.search(raw)
        if not match:
            self.report("Bad raw spec: %r", raw)
            if self.debug:
                match = self.type.search(raw)
                if not match:
                    self.report("(Type already doesn't match)")
                    self.htmlreport(raw, klass='incomplete', ranges=[(
                        match.start('type'), match.end('type'), 'type')])
                else:
                    self.report("(but type matched)")
                    self.htmlreport(raw, klass='incomplete')
            return
        type, name, args = match.group('type', 'name', 'args')
        ranges=[
                (match.start('type'), match.end('type'), 'type'),
                (match.start('name'), match.end('name'), 'name'),
                (match.start('args'), match.end('args'), 'arglist')]
        self.htmlreport(raw, klass='declaration', ranges=ranges)
        modifiers = self.getmodifiers(match)
        type = self.pythonizename(type)
        name = self.pythonizename(name)
        if self.checkduplicate(name):
            self.htmlreport("*** no output generated: duplicate name", klass="blacklisted")
            return
        self.report("==> %s %s <==", type, name)
        if self.blacklisted(type, name):
            self.htmlreport("*** no output generated: function name or return type blacklisted", klass="blacklisted")
            self.report("*** %s %s blacklisted", type, name)
            return
        returnlist = [(type, name, 'ReturnMode')]
        returnlist = self.repairarglist(name, returnlist)
        [(type, name, returnmode)] = returnlist
        arglist = self.extractarglist(args)
        arglist = self.repairarglist(name, arglist)
        if self.unmanageable(type, name, arglist):
            self.htmlreport("*** no output generated: some argument blacklisted", klass="blacklisted")
            ##for arg in arglist:
            ##  self.report("    %r", arg)
            self.report("*** %s %s unmanageable", type, name)
            return
        if modifiers:
            self.generate(type, name, arglist, modifiers)
        else:
            self.generate(type, name, arglist)

    def getmodifiers(self, match):
        return []

    def checkduplicate(self, name):
        if name in self.alreadydone:
            self.report("Name has already been defined: %r", name)
            return True
        self.alreadydone.append(name)
        return False

    def pythonizename(self, name):
        name = re.sub("\*", " ptr", name)
        name = name.strip()
        name = re.sub("[ \t]+", "_", name)
        return name

    def extractarglist(self, args):
        args = args.strip()
        if not args or args == "void":
            return []
        parts = [s.strip() for s in args.split(",")]
        arglist = []
        for part in parts:
            arg = self.extractarg(part)
            arglist.append(arg)
        return arglist

    def extractarg(self, part):
        mode = "InMode"
        part = part.strip()
        match = self.asplit.match(part)
        if not match:
            self.error("Indecipherable argument: %r", part)
            return ("unknown", part, mode)
        type, name, array = match.group('type', 'name', 'array')
        if array:
            # array matches an optional [] after the argument name
            type = type + " ptr "
        type = self.pythonizename(type)
        return self.modifyarg(type, name, mode)

    def modifyarg(self, type, name, mode):
        if type[:6] == "const_":
            type = type[6:]
        elif type[-4:] == "_ptr":
            type = type[:-4]
            mode = "OutMode"
        elif type in self.inherentpointertypes:
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
                    index = int(item[i][1:]) - 1
                    newitem[i] = old[index][i]
            new.append(tuple(newitem))
        ##self.report("old: %r", old)
        ##self.report("new: %r", new)
        return new

    def generate(self, tp, name, arglist, modifiers=[]):

        self.typeused(tp, 'return')
        if modifiers:
            classname, listname = self.destination(tp, name, arglist, modifiers)
        else:
            classname, listname = self.destination(tp, name, arglist)
        if not classname or not listname:
            self.htmlreport("*** no output generated: self.destination() returned None", klass="blacklisted")
            return
        if not self.specfile:
            self.htmlreport("*** no output generated: no output file specified", klass="blacklisted")
            return
        self.specfile.write("f = %s(%s, %r,\n" % (classname, tp, name))
        for atype, aname, amode in arglist:
            self.typeused(atype, amode)
            self.specfile.write("    (%s, %r, %s),\n" %
                                (atype, aname, amode))
        if self.greydictnames.has_key(name):
            self.specfile.write("    condition=%r,\n"%(self.greydictnames[name],))
        self.generatemodifiers(classname, name, modifiers)
        self.specfile.write(")\n")
        self.specfile.write("%s.append(f)\n\n" % listname)
        if self.htmlfile:
            oline = "Adding to %s:\n%s(returntype=%s, name=%r" % (listname, classname, tp, name)
            for atype, aname, amode in arglist:
                oline += ",\n    (%s, %r, %s)" % (atype, aname, amode)
            oline += ")\n"
            self.htmlreport(oline, klass="pydeclaration")

    def destination(self, type, name, arglist):
        return "FunctionGenerator", "functions"

    def generatemodifiers(self, classname, name, modifiers):
        pass

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

    def htmlreport(self, line, klass=None, ranges=None):
        if not self.htmlfile: return
        if ranges is None:
            ranges = []
        if klass:
            ranges.insert(0, (0, len(line), klass))
        oline = ''
        i = 0
        for c in line:
            for b, e, name in ranges:
                if b == i:
                    oline += '<span class="%s">' % name
                if e == i:
                    oline += '</span>'
            i += 1

            if c == '<': oline += '&lt;'
            elif c == '>': oline += '&gt;'
            else: oline += c
        for b, e, name in ranges:
            if b >= i:
                oline += '<span class="%s">' % name
            if e >= i:
                oline += '</span>'
        if not line or line[-1] != '\n':
            oline += '\n'
        self.htmlfile.write(oline)

class Scanner_PreUH3(Scanner):
    """Scanner for Universal Headers before release 3"""
    def initpatterns(self):
        Scanner.initpatterns(self)
        self.head_pat = "^extern pascal[ \t]+" # XXX Mac specific!
        self.type_pat = "pascal[ \t\n]+(?P<type>[a-zA-Z0-9_ \t]*[a-zA-Z0-9_])[ \t\n]+"
        self.whole_pat = self.type_pat + self.name_pat + self.args_pat
        self.sym_pat = "^[ \t]*(?P<name>[a-zA-Z0-9_]+)[ \t]*=" + \
                       "[ \t]*(?P<defn>[-0-9'\"][^\t\n,;}]*),?"

class Scanner_OSX(Scanner):
    """Scanner for modern (post UH3.3) Universal Headers """
    def initpatterns(self):
        Scanner.initpatterns(self)
        self.head_pat = "^EXTERN_API(_C)?"
        self.type_pat = "EXTERN_API(_C)?" + \
                        "[ \t\n]*\([ \t\n]*" + \
                        "(?P<type>[a-zA-Z0-9_* \t]*[a-zA-Z0-9_*])" + \
                        "[ \t\n]*\)[ \t\n]*"
        self.whole_pat = self.type_pat + self.name_pat + self.args_pat
        self.sym_pat = "^[ \t]*(?P<name>[a-zA-Z0-9_]+)[ \t]*=" + \
                       "[ \t]*(?P<defn>[-0-9_a-zA-Z'\"\(][^\t\n,;}]*),?"

_8bit = re.compile(r"[\200-\377]")

def escape8bit(s):
    if _8bit.search(s) is not None:
        out = []
        for c in s:
            o = ord(c)
            if o >= 128:
                out.append("\\" + hex(o)[1:])
            else:
                out.append(c)
        s = "".join(out)
    return s

def test():
    input = "D:Development:THINK C:Mac #includes:Apple #includes:AppleEvents.h"
    output = "@aespecs.py"
    defsoutput = "@aedefs.py"
    s = Scanner(input, output, defsoutput)
    s.scan()

if __name__ == '__main__':
    test()
