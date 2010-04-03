"""
gensuitemodule - Generate an AE suite module from an aete/aeut resource

Based on aete.py.

Reading and understanding this code is left as an exercise to the reader.
"""

from warnings import warnpy3k
warnpy3k("In 3.x, the gensuitemodule module is removed.", stacklevel=2)

import MacOS
import EasyDialogs
import os
import string
import sys
import types
import StringIO
import keyword
import macresource
import aetools
import distutils.sysconfig
import OSATerminology
from Carbon.Res import *
import Carbon.Folder
import MacOS
import getopt
import plistlib

_MAC_LIB_FOLDER=os.path.dirname(aetools.__file__)
DEFAULT_STANDARD_PACKAGEFOLDER=os.path.join(_MAC_LIB_FOLDER, 'lib-scriptpackages')
DEFAULT_USER_PACKAGEFOLDER=distutils.sysconfig.get_python_lib()

def usage():
    sys.stderr.write("Usage: %s [opts] application-or-resource-file\n" % sys.argv[0])
    sys.stderr.write("""Options:
--output pkgdir  Pathname of the output package (short: -o)
--resource       Parse resource file in stead of launching application (-r)
--base package   Use another base package in stead of default StdSuites (-b)
--edit old=new   Edit suite names, use empty new to skip a suite (-e)
--creator code   Set creator code for package (-c)
--dump           Dump aete resource to stdout in stead of creating module (-d)
--verbose        Tell us what happens (-v)
""")
    sys.exit(1)

def main():
    if len(sys.argv) > 1:
        SHORTOPTS = "rb:o:e:c:dv"
        LONGOPTS = ("resource", "base=", "output=", "edit=", "creator=", "dump", "verbose")
        try:
            opts, args = getopt.getopt(sys.argv[1:], SHORTOPTS, LONGOPTS)
        except getopt.GetoptError:
            usage()

        process_func = processfile
        basepkgname = 'StdSuites'
        output = None
        edit_modnames = []
        creatorsignature = None
        dump = None
        verbose = None

        for o, a in opts:
            if o in ('-r', '--resource'):
                process_func = processfile_fromresource
            if o in ('-b', '--base'):
                basepkgname = a
            if o in ('-o', '--output'):
                output = a
            if o in ('-e', '--edit'):
                split = a.split('=')
                if len(split) != 2:
                    usage()
                edit_modnames.append(split)
            if o in ('-c', '--creator'):
                if len(a) != 4:
                    sys.stderr.write("creator must be 4-char string\n")
                    sys.exit(1)
                creatorsignature = a
            if o in ('-d', '--dump'):
                dump = sys.stdout
            if o in ('-v', '--verbose'):
                verbose = sys.stderr


        if output and len(args) > 1:
            sys.stderr.write("%s: cannot specify --output with multiple inputs\n" % sys.argv[0])
            sys.exit(1)

        for filename in args:
            process_func(filename, output=output, basepkgname=basepkgname,
                edit_modnames=edit_modnames, creatorsignature=creatorsignature,
                dump=dump, verbose=verbose)
    else:
        main_interactive()

def main_interactive(interact=0, basepkgname='StdSuites'):
    if interact:
        # Ask for save-filename for each module
        edit_modnames = None
    else:
        # Use default filenames for each module
        edit_modnames = []
    appsfolder = Carbon.Folder.FSFindFolder(-32765, 'apps', 0)
    filename = EasyDialogs.AskFileForOpen(
        message='Select scriptable application',
        dialogOptionFlags=0x1056,       # allow selection of .app bundles
        defaultLocation=appsfolder)
    if not filename:
        return
    if not is_scriptable(filename):
        if EasyDialogs.AskYesNoCancel(
                "Warning: application does not seem scriptable",
                yes="Continue", default=2, no="") <= 0:
            return
    try:
        processfile(filename, edit_modnames=edit_modnames, basepkgname=basepkgname,
        verbose=sys.stderr)
    except MacOS.Error, arg:
        print "Error getting terminology:", arg
        print "Retry, manually parsing resources"
        processfile_fromresource(filename, edit_modnames=edit_modnames,
            basepkgname=basepkgname, verbose=sys.stderr)

def is_scriptable(application):
    """Return true if the application is scriptable"""
    if os.path.isdir(application):
        plistfile = os.path.join(application, 'Contents', 'Info.plist')
        if not os.path.exists(plistfile):
            return False
        plist = plistlib.Plist.fromFile(plistfile)
        return plist.get('NSAppleScriptEnabled', False)
    # If it is a file test for an aete/aeut resource.
    currf = CurResFile()
    try:
        refno = macresource.open_pathname(application)
    except MacOS.Error:
        return False
    UseResFile(refno)
    n_terminology = Count1Resources('aete') + Count1Resources('aeut') + \
        Count1Resources('scsz') + Count1Resources('osiz')
    CloseResFile(refno)
    UseResFile(currf)
    return n_terminology > 0

def processfile_fromresource(fullname, output=None, basepkgname=None,
        edit_modnames=None, creatorsignature=None, dump=None, verbose=None):
    """Process all resources in a single file"""
    if not is_scriptable(fullname) and verbose:
        print >>verbose, "Warning: app does not seem scriptable: %s" % fullname
    cur = CurResFile()
    if verbose:
        print >>verbose, "Processing", fullname
    rf = macresource.open_pathname(fullname)
    try:
        UseResFile(rf)
        resources = []
        for i in range(Count1Resources('aete')):
            res = Get1IndResource('aete', 1+i)
            resources.append(res)
        for i in range(Count1Resources('aeut')):
            res = Get1IndResource('aeut', 1+i)
            resources.append(res)
        if verbose:
            print >>verbose, "\nLISTING aete+aeut RESOURCES IN", repr(fullname)
        aetelist = []
        for res in resources:
            if verbose:
                print >>verbose, "decoding", res.GetResInfo(), "..."
            data = res.data
            aete = decode(data, verbose)
            aetelist.append((aete, res.GetResInfo()))
    finally:
        if rf != cur:
            CloseResFile(rf)
            UseResFile(cur)
    # switch back (needed for dialogs in Python)
    UseResFile(cur)
    if dump:
        dumpaetelist(aetelist, dump)
    compileaetelist(aetelist, fullname, output=output,
        basepkgname=basepkgname, edit_modnames=edit_modnames,
        creatorsignature=creatorsignature, verbose=verbose)

def processfile(fullname, output=None, basepkgname=None,
        edit_modnames=None, creatorsignature=None, dump=None,
        verbose=None):
    """Ask an application for its terminology and process that"""
    if not is_scriptable(fullname) and verbose:
        print >>verbose, "Warning: app does not seem scriptable: %s" % fullname
    if verbose:
        print >>verbose, "\nASKING FOR aete DICTIONARY IN", repr(fullname)
    try:
        aedescobj, launched = OSATerminology.GetAppTerminology(fullname)
    except MacOS.Error, arg:
        if arg[0] in (-1701, -192): # errAEDescNotFound, resNotFound
            if verbose:
                print >>verbose, "GetAppTerminology failed with errAEDescNotFound/resNotFound, trying manually"
            aedata, sig = getappterminology(fullname, verbose=verbose)
            if not creatorsignature:
                creatorsignature = sig
        else:
            raise
    else:
        if launched:
            if verbose:
                print >>verbose, "Launched", fullname
        raw = aetools.unpack(aedescobj)
        if not raw:
            if verbose:
                print >>verbose, 'Unpack returned empty value:', raw
            return
        if not raw[0].data:
            if verbose:
                print >>verbose, 'Unpack returned value without data:', raw
            return
        aedata = raw[0]
    aete = decode(aedata.data, verbose)
    if dump:
        dumpaetelist([aete], dump)
        return
    compileaete(aete, None, fullname, output=output, basepkgname=basepkgname,
        creatorsignature=creatorsignature, edit_modnames=edit_modnames,
        verbose=verbose)

def getappterminology(fullname, verbose=None):
    """Get application terminology by sending an AppleEvent"""
    # First check that we actually can send AppleEvents
    if not MacOS.WMAvailable():
        raise RuntimeError, "Cannot send AppleEvents, no access to window manager"
    # Next, a workaround for a bug in MacOS 10.2: sending events will hang unless
    # you have created an event loop first.
    import Carbon.Evt
    Carbon.Evt.WaitNextEvent(0,0)
    if os.path.isdir(fullname):
        # Now get the signature of the application, hoping it is a bundle
        pkginfo = os.path.join(fullname, 'Contents', 'PkgInfo')
        if not os.path.exists(pkginfo):
            raise RuntimeError, "No PkgInfo file found"
        tp_cr = open(pkginfo, 'rb').read()
        cr = tp_cr[4:8]
    else:
        # Assume it is a file
        cr, tp = MacOS.GetCreatorAndType(fullname)
    # Let's talk to it and ask for its AETE
    talker = aetools.TalkTo(cr)
    try:
        talker._start()
    except (MacOS.Error, aetools.Error), arg:
        if verbose:
            print >>verbose, 'Warning: start() failed, continuing anyway:', arg
    reply = talker.send("ascr", "gdte")
    #reply2 = talker.send("ascr", "gdut")
    # Now pick the bits out of the return that we need.
    return reply[1]['----'], cr


def compileaetelist(aetelist, fullname, output=None, basepkgname=None,
            edit_modnames=None, creatorsignature=None, verbose=None):
    for aete, resinfo in aetelist:
        compileaete(aete, resinfo, fullname, output=output,
            basepkgname=basepkgname, edit_modnames=edit_modnames,
            creatorsignature=creatorsignature, verbose=verbose)

def dumpaetelist(aetelist, output):
    import pprint
    pprint.pprint(aetelist, output)

def decode(data, verbose=None):
    """Decode a resource into a python data structure"""
    f = StringIO.StringIO(data)
    aete = generic(getaete, f)
    aete = simplify(aete)
    processed = f.tell()
    unprocessed = len(f.read())
    total = f.tell()
    if unprocessed and verbose:
        verbose.write("%d processed + %d unprocessed = %d total\n" %
                         (processed, unprocessed, total))
    return aete

def simplify(item):
    """Recursively replace singleton tuples by their constituent item"""
    if type(item) is types.ListType:
        return map(simplify, item)
    elif type(item) == types.TupleType and len(item) == 2:
        return simplify(item[1])
    else:
        return item


# Here follows the aete resource decoder.
# It is presented bottom-up instead of top-down because there are  direct
# references to the lower-level part-decoders from the high-level part-decoders.

def getbyte(f, *args):
    c = f.read(1)
    if not c:
        raise EOFError, 'in getbyte' + str(args)
    return ord(c)

def getword(f, *args):
    getalign(f)
    s = f.read(2)
    if len(s) < 2:
        raise EOFError, 'in getword' + str(args)
    return (ord(s[0])<<8) | ord(s[1])

def getlong(f, *args):
    getalign(f)
    s = f.read(4)
    if len(s) < 4:
        raise EOFError, 'in getlong' + str(args)
    return (ord(s[0])<<24) | (ord(s[1])<<16) | (ord(s[2])<<8) | ord(s[3])

def getostype(f, *args):
    getalign(f)
    s = f.read(4)
    if len(s) < 4:
        raise EOFError, 'in getostype' + str(args)
    return s

def getpstr(f, *args):
    c = f.read(1)
    if len(c) < 1:
        raise EOFError, 'in getpstr[1]' + str(args)
    nbytes = ord(c)
    if nbytes == 0: return ''
    s = f.read(nbytes)
    if len(s) < nbytes:
        raise EOFError, 'in getpstr[2]' + str(args)
    return s

def getalign(f):
    if f.tell() & 1:
        c = f.read(1)
        ##if c != '\0':
        ##  print align:', repr(c)

def getlist(f, description, getitem):
    count = getword(f)
    list = []
    for i in range(count):
        list.append(generic(getitem, f))
        getalign(f)
    return list

def alt_generic(what, f, *args):
    print "generic", repr(what), args
    res = vageneric(what, f, args)
    print '->', repr(res)
    return res

def generic(what, f, *args):
    if type(what) == types.FunctionType:
        return apply(what, (f,) + args)
    if type(what) == types.ListType:
        record = []
        for thing in what:
            item = apply(generic, thing[:1] + (f,) + thing[1:])
            record.append((thing[1], item))
        return record
    return "BAD GENERIC ARGS: %r" % (what,)

getdata = [
    (getostype, "type"),
    (getpstr, "description"),
    (getword, "flags")
    ]
getargument = [
    (getpstr, "name"),
    (getostype, "keyword"),
    (getdata, "what")
    ]
getevent = [
    (getpstr, "name"),
    (getpstr, "description"),
    (getostype, "suite code"),
    (getostype, "event code"),
    (getdata, "returns"),
    (getdata, "accepts"),
    (getlist, "optional arguments", getargument)
    ]
getproperty = [
    (getpstr, "name"),
    (getostype, "code"),
    (getdata, "what")
    ]
getelement = [
    (getostype, "type"),
    (getlist, "keyform", getostype)
    ]
getclass = [
    (getpstr, "name"),
    (getostype, "class code"),
    (getpstr, "description"),
    (getlist, "properties", getproperty),
    (getlist, "elements", getelement)
    ]
getcomparison = [
    (getpstr, "operator name"),
    (getostype, "operator ID"),
    (getpstr, "operator comment"),
    ]
getenumerator = [
    (getpstr, "enumerator name"),
    (getostype, "enumerator ID"),
    (getpstr, "enumerator comment")
    ]
getenumeration = [
    (getostype, "enumeration ID"),
    (getlist, "enumerator", getenumerator)
    ]
getsuite = [
    (getpstr, "suite name"),
    (getpstr, "suite description"),
    (getostype, "suite ID"),
    (getword, "suite level"),
    (getword, "suite version"),
    (getlist, "events", getevent),
    (getlist, "classes", getclass),
    (getlist, "comparisons", getcomparison),
    (getlist, "enumerations", getenumeration)
    ]
getaete = [
    (getword, "major/minor version in BCD"),
    (getword, "language code"),
    (getword, "script code"),
    (getlist, "suites", getsuite)
    ]

def compileaete(aete, resinfo, fname, output=None, basepkgname=None,
        edit_modnames=None, creatorsignature=None, verbose=None):
    """Generate code for a full aete resource. fname passed for doc purposes"""
    [version, language, script, suites] = aete
    major, minor = divmod(version, 256)
    if not creatorsignature:
        creatorsignature, dummy = MacOS.GetCreatorAndType(fname)
    packagename = identify(os.path.splitext(os.path.basename(fname))[0])
    if language:
        packagename = packagename+'_lang%d'%language
    if script:
        packagename = packagename+'_script%d'%script
    if len(packagename) > 27:
        packagename = packagename[:27]
    if output:
        # XXXX Put this in site-packages if it isn't a full pathname?
        if not os.path.exists(output):
            os.mkdir(output)
        pathname = output
    else:
        pathname = EasyDialogs.AskFolder(message='Create and select package folder for %s'%packagename,
            defaultLocation=DEFAULT_USER_PACKAGEFOLDER)
        output = pathname
    if not pathname:
        return
    packagename = os.path.split(os.path.normpath(pathname))[1]
    if not basepkgname:
        basepkgname = EasyDialogs.AskFolder(message='Package folder for base suite (usually StdSuites)',
            defaultLocation=DEFAULT_STANDARD_PACKAGEFOLDER)
    if basepkgname:
        dirname, basepkgname = os.path.split(os.path.normpath(basepkgname))
        if dirname and not dirname in sys.path:
            sys.path.insert(0, dirname)
        basepackage = __import__(basepkgname)
    else:
        basepackage = None
    suitelist = []
    allprecompinfo = []
    allsuites = []
    for suite in suites:
        compiler = SuiteCompiler(suite, basepackage, output, edit_modnames, verbose)
        code, modname, precompinfo = compiler.precompilesuite()
        if not code:
            continue
        allprecompinfo = allprecompinfo + precompinfo
        suiteinfo = suite, pathname, modname
        suitelist.append((code, modname))
        allsuites.append(compiler)
    for compiler in allsuites:
        compiler.compilesuite(major, minor, language, script, fname, allprecompinfo)
    initfilename = os.path.join(output, '__init__.py')
    fp = open(initfilename, 'w')
    MacOS.SetCreatorAndType(initfilename, 'Pyth', 'TEXT')
    fp.write('"""\n')
    fp.write("Package generated from %s\n"%ascii(fname))
    if resinfo:
        fp.write("Resource %s resid %d %s\n"%(ascii(resinfo[1]), resinfo[0], ascii(resinfo[2])))
    fp.write('"""\n')
    fp.write('import aetools\n')
    fp.write('Error = aetools.Error\n')
    suitelist.sort()
    for code, modname in suitelist:
        fp.write("import %s\n" % modname)
    fp.write("\n\n_code_to_module = {\n")
    for code, modname in suitelist:
        fp.write("    '%s' : %s,\n"%(ascii(code), modname))
    fp.write("}\n\n")
    fp.write("\n\n_code_to_fullname = {\n")
    for code, modname in suitelist:
        fp.write("    '%s' : ('%s.%s', '%s'),\n"%(ascii(code), packagename, modname, modname))
    fp.write("}\n\n")
    for code, modname in suitelist:
        fp.write("from %s import *\n"%modname)

    # Generate property dicts and element dicts for all types declared in this module
    fp.write("\ndef getbaseclasses(v):\n")
    fp.write("    if not getattr(v, '_propdict', None):\n")
    fp.write("        v._propdict = {}\n")
    fp.write("        v._elemdict = {}\n")
    fp.write("        for superclassname in getattr(v, '_superclassnames', []):\n")
    fp.write("            superclass = eval(superclassname)\n")
    fp.write("            getbaseclasses(superclass)\n")
    fp.write("            v._propdict.update(getattr(superclass, '_propdict', {}))\n")
    fp.write("            v._elemdict.update(getattr(superclass, '_elemdict', {}))\n")
    fp.write("        v._propdict.update(getattr(v, '_privpropdict', {}))\n")
    fp.write("        v._elemdict.update(getattr(v, '_privelemdict', {}))\n")
    fp.write("\n")
    fp.write("import StdSuites\n")
    allprecompinfo.sort()
    if allprecompinfo:
        fp.write("\n#\n# Set property and element dictionaries now that all classes have been defined\n#\n")
        for codenamemapper in allprecompinfo:
            for k, v in codenamemapper.getall('class'):
                fp.write("getbaseclasses(%s)\n" % v)

    # Generate a code-to-name mapper for all of the types (classes) declared in this module
    application_class = None
    if allprecompinfo:
        fp.write("\n#\n# Indices of types declared in this module\n#\n")
        fp.write("_classdeclarations = {\n")
        for codenamemapper in allprecompinfo:
            for k, v in codenamemapper.getall('class'):
                fp.write("    %r : %s,\n" % (k, v))
                if k == 'capp':
                    application_class = v
        fp.write("}\n")


    if suitelist:
        fp.write("\n\nclass %s(%s_Events"%(packagename, suitelist[0][1]))
        for code, modname in suitelist[1:]:
            fp.write(",\n        %s_Events"%modname)
        fp.write(",\n        aetools.TalkTo):\n")
        fp.write("    _signature = %r\n\n"%(creatorsignature,))
        fp.write("    _moduleName = '%s'\n\n"%packagename)
        if application_class:
            fp.write("    _elemdict = %s._elemdict\n" % application_class)
            fp.write("    _propdict = %s._propdict\n" % application_class)
    fp.close()

class SuiteCompiler:
    def __init__(self, suite, basepackage, output, edit_modnames, verbose):
        self.suite = suite
        self.basepackage = basepackage
        self.edit_modnames = edit_modnames
        self.output = output
        self.verbose = verbose

        # Set by precompilesuite
        self.pathname = None
        self.modname = None

        # Set by compilesuite
        self.fp = None
        self.basemodule = None
        self.enumsneeded = {}

    def precompilesuite(self):
        """Parse a single suite without generating the output. This step is needed
        so we can resolve recursive references by suites to enums/comps/etc declared
        in other suites"""
        [name, desc, code, level, version, events, classes, comps, enums] = self.suite

        modname = identify(name)
        if len(modname) > 28:
            modname = modname[:27]
        if self.edit_modnames is None:
            self.pathname = EasyDialogs.AskFileForSave(message='Python output file',
                savedFileName=modname+'.py')
        else:
            for old, new in self.edit_modnames:
                if old == modname:
                    modname = new
            if modname:
                self.pathname = os.path.join(self.output, modname + '.py')
            else:
                self.pathname = None
        if not self.pathname:
            return None, None, None

        self.modname = os.path.splitext(os.path.split(self.pathname)[1])[0]

        if self.basepackage and code in self.basepackage._code_to_module:
            # We are an extension of a baseclass (usually an application extending
            # Standard_Suite or so). Import everything from our base module
            basemodule = self.basepackage._code_to_module[code]
        else:
            # We are not an extension.
            basemodule = None

        self.enumsneeded = {}
        for event in events:
            self.findenumsinevent(event)

        objc = ObjectCompiler(None, self.modname, basemodule, interact=(self.edit_modnames is None),
            verbose=self.verbose)
        for cls in classes:
            objc.compileclass(cls)
        for cls in classes:
            objc.fillclasspropsandelems(cls)
        for comp in comps:
            objc.compilecomparison(comp)
        for enum in enums:
            objc.compileenumeration(enum)

        for enum in self.enumsneeded.keys():
            objc.checkforenum(enum)

        objc.dumpindex()

        precompinfo = objc.getprecompinfo(self.modname)

        return code, self.modname, precompinfo

    def compilesuite(self, major, minor, language, script, fname, precompinfo):
        """Generate code for a single suite"""
        [name, desc, code, level, version, events, classes, comps, enums] = self.suite
        # Sort various lists, so re-generated source is easier compared
        def class_sorter(k1, k2):
            """Sort classes by code, and make sure main class sorts before synonyms"""
            # [name, code, desc, properties, elements] = cls
            if k1[1] < k2[1]: return -1
            if k1[1] > k2[1]: return 1
            if not k2[3] or k2[3][0][1] == 'c@#!':
                # This is a synonym, the other one is better
                return -1
            if not k1[3] or k1[3][0][1] == 'c@#!':
                # This is a synonym, the other one is better
                return 1
            return 0

        events.sort()
        classes.sort(class_sorter)
        comps.sort()
        enums.sort()

        self.fp = fp = open(self.pathname, 'w')
        MacOS.SetCreatorAndType(self.pathname, 'Pyth', 'TEXT')

        fp.write('"""Suite %s: %s\n' % (ascii(name), ascii(desc)))
        fp.write("Level %d, version %d\n\n" % (level, version))
        fp.write("Generated from %s\n"%ascii(fname))
        fp.write("AETE/AEUT resource version %d/%d, language %d, script %d\n" % \
            (major, minor, language, script))
        fp.write('"""\n\n')

        fp.write('import aetools\n')
        fp.write('import MacOS\n\n')
        fp.write("_code = %r\n\n"% (code,))
        if self.basepackage and code in self.basepackage._code_to_module:
            # We are an extension of a baseclass (usually an application extending
            # Standard_Suite or so). Import everything from our base module
            fp.write('from %s import *\n'%self.basepackage._code_to_fullname[code][0])
            basemodule = self.basepackage._code_to_module[code]
        elif self.basepackage and code.lower() in self.basepackage._code_to_module:
            # This is needed by CodeWarrior and some others.
            fp.write('from %s import *\n'%self.basepackage._code_to_fullname[code.lower()][0])
            basemodule = self.basepackage._code_to_module[code.lower()]
        else:
            # We are not an extension.
            basemodule = None
        self.basemodule = basemodule
        self.compileclassheader()

        self.enumsneeded = {}
        if events:
            for event in events:
                self.compileevent(event)
        else:
            fp.write("    pass\n\n")

        objc = ObjectCompiler(fp, self.modname, basemodule, precompinfo, interact=(self.edit_modnames is None),
            verbose=self.verbose)
        for cls in classes:
            objc.compileclass(cls)
        for cls in classes:
            objc.fillclasspropsandelems(cls)
        for comp in comps:
            objc.compilecomparison(comp)
        for enum in enums:
            objc.compileenumeration(enum)

        for enum in self.enumsneeded.keys():
            objc.checkforenum(enum)

        objc.dumpindex()

    def compileclassheader(self):
        """Generate class boilerplate"""
        classname = '%s_Events'%self.modname
        if self.basemodule:
            modshortname = string.split(self.basemodule.__name__, '.')[-1]
            baseclassname = '%s_Events'%modshortname
            self.fp.write("class %s(%s):\n\n"%(classname, baseclassname))
        else:
            self.fp.write("class %s:\n\n"%classname)

    def compileevent(self, event):
        """Generate code for a single event"""
        [name, desc, code, subcode, returns, accepts, arguments] = event
        fp = self.fp
        funcname = identify(name)
        #
        # generate name->keyword map
        #
        if arguments:
            fp.write("    _argmap_%s = {\n"%funcname)
            for a in arguments:
                fp.write("        %r : %r,\n"%(identify(a[0]), a[1]))
            fp.write("    }\n\n")

        #
        # Generate function header
        #
        has_arg = (not is_null(accepts))
        opt_arg = (has_arg and is_optional(accepts))

        fp.write("    def %s(self, "%funcname)
        if has_arg:
            if not opt_arg:
                fp.write("_object, ")       # Include direct object, if it has one
            else:
                fp.write("_object=None, ")  # Also include if it is optional
        else:
            fp.write("_no_object=None, ")   # For argument checking
        fp.write("_attributes={}, **_arguments):\n")    # include attribute dict and args
        #
        # Generate doc string (important, since it may be the only
        # available documentation, due to our name-remaping)
        #
        fp.write('        """%s: %s\n'%(ascii(name), ascii(desc)))
        if has_arg:
            fp.write("        Required argument: %s\n"%getdatadoc(accepts))
        elif opt_arg:
            fp.write("        Optional argument: %s\n"%getdatadoc(accepts))
        for arg in arguments:
            fp.write("        Keyword argument %s: %s\n"%(identify(arg[0]),
                    getdatadoc(arg[2])))
        fp.write("        Keyword argument _attributes: AppleEvent attribute dictionary\n")
        if not is_null(returns):
            fp.write("        Returns: %s\n"%getdatadoc(returns))
        fp.write('        """\n')
        #
        # Fiddle the args so everything ends up in 'arguments' dictionary
        #
        fp.write("        _code = %r\n"% (code,))
        fp.write("        _subcode = %r\n\n"% (subcode,))
        #
        # Do keyword name substitution
        #
        if arguments:
            fp.write("        aetools.keysubst(_arguments, self._argmap_%s)\n"%funcname)
        else:
            fp.write("        if _arguments: raise TypeError, 'No optional args expected'\n")
        #
        # Stuff required arg (if there is one) into arguments
        #
        if has_arg:
            fp.write("        _arguments['----'] = _object\n")
        elif opt_arg:
            fp.write("        if _object:\n")
            fp.write("            _arguments['----'] = _object\n")
        else:
            fp.write("        if _no_object is not None: raise TypeError, 'No direct arg expected'\n")
        fp.write("\n")
        #
        # Do enum-name substitution
        #
        for a in arguments:
            if is_enum(a[2]):
                kname = a[1]
                ename = a[2][0]
                if ename != '****':
                    fp.write("        aetools.enumsubst(_arguments, %r, _Enum_%s)\n" %
                        (kname, identify(ename)))
                    self.enumsneeded[ename] = 1
        fp.write("\n")
        #
        # Do the transaction
        #
        fp.write("        _reply, _arguments, _attributes = self.send(_code, _subcode,\n")
        fp.write("                _arguments, _attributes)\n")
        #
        # Error handling
        #
        fp.write("        if _arguments.get('errn', 0):\n")
        fp.write("            raise aetools.Error, aetools.decodeerror(_arguments)\n")
        fp.write("        # XXXX Optionally decode result\n")
        #
        # Decode result
        #
        fp.write("        if '----' in _arguments:\n")
        if is_enum(returns):
            fp.write("            # XXXX Should do enum remapping here...\n")
        fp.write("            return _arguments['----']\n")
        fp.write("\n")

    def findenumsinevent(self, event):
        """Find all enums for a single event"""
        [name, desc, code, subcode, returns, accepts, arguments] = event
        for a in arguments:
            if is_enum(a[2]):
                ename = a[2][0]
                if ename != '****':
                    self.enumsneeded[ename] = 1

#
# This class stores the code<->name translations for a single module. It is used
# to keep the information while we're compiling the module, but we also keep these objects
# around so if one suite refers to, say, an enum in another suite we know where to
# find it. Finally, if we really can't find a code, the user can add modules by
# hand.
#
class CodeNameMapper:

    def __init__(self, interact=1, verbose=None):
        self.code2name = {
            "property" : {},
            "class" : {},
            "enum" : {},
            "comparison" : {},
        }
        self.name2code =  {
            "property" : {},
            "class" : {},
            "enum" : {},
            "comparison" : {},
        }
        self.modulename = None
        self.star_imported = 0
        self.can_interact = interact
        self.verbose = verbose

    def addnamecode(self, type, name, code):
        self.name2code[type][name] = code
        if code not in self.code2name[type]:
            self.code2name[type][code] = name

    def hasname(self, name):
        for dict in self.name2code.values():
            if name in dict:
                return True
        return False

    def hascode(self, type, code):
        return code in self.code2name[type]

    def findcodename(self, type, code):
        if not self.hascode(type, code):
            return None, None, None
        name = self.code2name[type][code]
        if self.modulename and not self.star_imported:
            qualname = '%s.%s'%(self.modulename, name)
        else:
            qualname = name
        return name, qualname, self.modulename

    def getall(self, type):
        return self.code2name[type].items()

    def addmodule(self, module, name, star_imported):
        self.modulename = name
        self.star_imported = star_imported
        for code, name in module._propdeclarations.items():
            self.addnamecode('property', name, code)
        for code, name in module._classdeclarations.items():
            self.addnamecode('class', name, code)
        for code in module._enumdeclarations.keys():
            self.addnamecode('enum', '_Enum_'+identify(code), code)
        for code, name in module._compdeclarations.items():
            self.addnamecode('comparison', name, code)

    def prepareforexport(self, name=None):
        if not self.modulename:
            self.modulename = name
        return self

class ObjectCompiler:
    def __init__(self, fp, modname, basesuite, othernamemappers=None, interact=1,
            verbose=None):
        self.fp = fp
        self.verbose = verbose
        self.basesuite = basesuite
        self.can_interact = interact
        self.modulename = modname
        self.namemappers = [CodeNameMapper(self.can_interact, self.verbose)]
        if othernamemappers:
            self.othernamemappers = othernamemappers[:]
        else:
            self.othernamemappers = []
        if basesuite:
            basemapper = CodeNameMapper(self.can_interact, self.verbose)
            basemapper.addmodule(basesuite, '', 1)
            self.namemappers.append(basemapper)

    def getprecompinfo(self, modname):
        list = []
        for mapper in self.namemappers:
            emapper = mapper.prepareforexport(modname)
            if emapper:
                list.append(emapper)
        return list

    def findcodename(self, type, code):
        while 1:
            # First try: check whether we already know about this code.
            for mapper in self.namemappers:
                if mapper.hascode(type, code):
                    return mapper.findcodename(type, code)
            # Second try: maybe one of the other modules knows about it.
            for mapper in self.othernamemappers:
                if mapper.hascode(type, code):
                    self.othernamemappers.remove(mapper)
                    self.namemappers.append(mapper)
                    if self.fp:
                        self.fp.write("import %s\n"%mapper.modulename)
                    break
            else:
                # If all this has failed we ask the user for a guess on where it could
                # be and retry.
                if self.fp:
                    m = self.askdefinitionmodule(type, code)
                else:
                    m = None
                if not m: return None, None, None
                mapper = CodeNameMapper(self.can_interact, self.verbose)
                mapper.addmodule(m, m.__name__, 0)
                self.namemappers.append(mapper)

    def hasname(self, name):
        for mapper in self.othernamemappers:
            if mapper.hasname(name) and mapper.modulename != self.modulename:
                if self.verbose:
                    print >>self.verbose, "Duplicate Python identifier:", name, self.modulename, mapper.modulename
                return True
        return False

    def askdefinitionmodule(self, type, code):
        if not self.can_interact:
            if self.verbose:
                print >>self.verbose, "** No definition for %s '%s' found" % (type, code)
            return None
        path = EasyDialogs.AskFileForSave(message='Where is %s %s declared?'%(type, code))
        if not path: return
        path, file = os.path.split(path)
        modname = os.path.splitext(file)[0]
        if not path in sys.path:
            sys.path.insert(0, path)
        m = __import__(modname)
        self.fp.write("import %s\n"%modname)
        return m

    def compileclass(self, cls):
        [name, code, desc, properties, elements] = cls
        pname = identify(name)
        if self.namemappers[0].hascode('class', code):
            # plural forms and such
            othername, dummy, dummy = self.namemappers[0].findcodename('class', code)
            if self.fp:
                self.fp.write("\n%s = %s\n"%(pname, othername))
        else:
            if self.fp:
                self.fp.write('\nclass %s(aetools.ComponentItem):\n' % pname)
                self.fp.write('    """%s - %s """\n' % (ascii(name), ascii(desc)))
                self.fp.write('    want = %r\n' % (code,))
        self.namemappers[0].addnamecode('class', pname, code)
        is_application_class = (code == 'capp')
        properties.sort()
        for prop in properties:
            self.compileproperty(prop, is_application_class)
        elements.sort()
        for elem in elements:
            self.compileelement(elem)

    def compileproperty(self, prop, is_application_class=False):
        [name, code, what] = prop
        if code == 'c@#!':
            # Something silly with plurals. Skip it.
            return
        pname = identify(name)
        if self.namemappers[0].hascode('property', code):
            # plural forms and such
            othername, dummy, dummy = self.namemappers[0].findcodename('property', code)
            if pname == othername:
                return
            if self.fp:
                self.fp.write("\n_Prop_%s = _Prop_%s\n"%(pname, othername))
        else:
            if self.fp:
                self.fp.write("class _Prop_%s(aetools.NProperty):\n" % pname)
                self.fp.write('    """%s - %s """\n' % (ascii(name), ascii(what[1])))
                self.fp.write("    which = %r\n" % (code,))
                self.fp.write("    want = %r\n" % (what[0],))
        self.namemappers[0].addnamecode('property', pname, code)
        if is_application_class and self.fp:
            self.fp.write("%s = _Prop_%s()\n" % (pname, pname))

    def compileelement(self, elem):
        [code, keyform] = elem
        if self.fp:
            self.fp.write("#        element %r as %s\n" % (code, keyform))

    def fillclasspropsandelems(self, cls):
        [name, code, desc, properties, elements] = cls
        cname = identify(name)
        if self.namemappers[0].hascode('class', code) and \
                self.namemappers[0].findcodename('class', code)[0] != cname:
            # This is an other name (plural or so) for something else. Skip.
            if self.fp and (elements or len(properties) > 1 or (len(properties) == 1 and
                properties[0][1] != 'c@#!')):
                if self.verbose:
                    print >>self.verbose, '** Skip multiple %s of %s (code %r)' % (cname, self.namemappers[0].findcodename('class', code)[0], code)
                raise RuntimeError, "About to skip non-empty class"
            return
        plist = []
        elist = []
        superclasses = []
        for prop in properties:
            [pname, pcode, what] = prop
            if pcode == "c@#^":
                superclasses.append(what)
            if pcode == 'c@#!':
                continue
            pname = identify(pname)
            plist.append(pname)

        superclassnames = []
        for superclass in superclasses:
            superId, superDesc, dummy = superclass
            superclassname, fullyqualifiedname, module = self.findcodename("class", superId)
            # I don't think this is correct:
            if superclassname == cname:
                pass # superclassnames.append(fullyqualifiedname)
            else:
                superclassnames.append(superclassname)

        if self.fp:
            self.fp.write("%s._superclassnames = %r\n"%(cname, superclassnames))

        for elem in elements:
            [ecode, keyform] = elem
            if ecode == 'c@#!':
                continue
            name, ename, module = self.findcodename('class', ecode)
            if not name:
                if self.fp:
                    self.fp.write("# XXXX %s element %r not found!!\n"%(cname, ecode))
            else:
                elist.append((name, ename))

        plist.sort()
        elist.sort()

        if self.fp:
            self.fp.write("%s._privpropdict = {\n"%cname)
            for n in plist:
                self.fp.write("    '%s' : _Prop_%s,\n"%(n, n))
            self.fp.write("}\n")
            self.fp.write("%s._privelemdict = {\n"%cname)
            for n, fulln in elist:
                self.fp.write("    '%s' : %s,\n"%(n, fulln))
            self.fp.write("}\n")

    def compilecomparison(self, comp):
        [name, code, comment] = comp
        iname = identify(name)
        self.namemappers[0].addnamecode('comparison', iname, code)
        if self.fp:
            self.fp.write("class %s(aetools.NComparison):\n" % iname)
            self.fp.write('    """%s - %s """\n' % (ascii(name), ascii(comment)))

    def compileenumeration(self, enum):
        [code, items] = enum
        name = "_Enum_%s" % identify(code)
        if self.fp:
            self.fp.write("%s = {\n" % name)
            for item in items:
                self.compileenumerator(item)
            self.fp.write("}\n\n")
        self.namemappers[0].addnamecode('enum', name, code)
        return code

    def compileenumerator(self, item):
        [name, code, desc] = item
        self.fp.write("    %r : %r,\t# %s\n" % (identify(name), code, ascii(desc)))

    def checkforenum(self, enum):
        """This enum code is used by an event. Make sure it's available"""
        name, fullname, module = self.findcodename('enum', enum)
        if not name:
            if self.fp:
                self.fp.write("_Enum_%s = None # XXXX enum %s not found!!\n"%(identify(enum), ascii(enum)))
            return
        if module:
            if self.fp:
                self.fp.write("from %s import %s\n"%(module, name))

    def dumpindex(self):
        if not self.fp:
            return
        self.fp.write("\n#\n# Indices of types declared in this module\n#\n")

        self.fp.write("_classdeclarations = {\n")
        classlist = self.namemappers[0].getall('class')
        classlist.sort()
        for k, v in classlist:
            self.fp.write("    %r : %s,\n" % (k, v))
        self.fp.write("}\n")

        self.fp.write("\n_propdeclarations = {\n")
        proplist = self.namemappers[0].getall('property')
        proplist.sort()
        for k, v in proplist:
            self.fp.write("    %r : _Prop_%s,\n" % (k, v))
        self.fp.write("}\n")

        self.fp.write("\n_compdeclarations = {\n")
        complist = self.namemappers[0].getall('comparison')
        complist.sort()
        for k, v in complist:
            self.fp.write("    %r : %s,\n" % (k, v))
        self.fp.write("}\n")

        self.fp.write("\n_enumdeclarations = {\n")
        enumlist = self.namemappers[0].getall('enum')
        enumlist.sort()
        for k, v in enumlist:
            self.fp.write("    %r : %s,\n" % (k, v))
        self.fp.write("}\n")

def compiledata(data):
    [type, description, flags] = data
    return "%r -- %r %s" % (type, description, compiledataflags(flags))

def is_null(data):
    return data[0] == 'null'

def is_optional(data):
    return (data[2] & 0x8000)

def is_enum(data):
    return (data[2] & 0x2000)

def getdatadoc(data):
    [type, descr, flags] = data
    if descr:
        return ascii(descr)
    if type == '****':
        return 'anything'
    if type == 'obj ':
        return 'an AE object reference'
    return "undocumented, typecode %r"%(type,)

dataflagdict = {15: "optional", 14: "list", 13: "enum", 12: "mutable"}
def compiledataflags(flags):
    bits = []
    for i in range(16):
        if flags & (1<<i):
            if i in dataflagdict.keys():
                bits.append(dataflagdict[i])
            else:
                bits.append(repr(i))
    return '[%s]' % string.join(bits)

def ascii(str):
    """Return a string with all non-ascii characters hex-encoded"""
    if type(str) != type(''):
        return map(ascii, str)
    rv = ''
    for c in str:
        if c in ('\t', '\n', '\r') or ' ' <= c < chr(0x7f):
            rv = rv + c
        else:
            rv = rv + '\\' + 'x%02.2x' % ord(c)
    return rv

def identify(str):
    """Turn any string into an identifier:
    - replace space by _
    - replace other illegal chars by _xx_ (hex code)
    - append _ if the result is a python keyword
    """
    if not str:
        return "empty_ae_name_"
    rv = ''
    ok = string.ascii_letters + '_'
    ok2 = ok + string.digits
    for c in str:
        if c in ok:
            rv = rv + c
        elif c == ' ':
            rv = rv + '_'
        else:
            rv = rv + '_%02.2x_'%ord(c)
        ok = ok2
    if keyword.iskeyword(rv):
        rv = rv + '_'
    return rv

# Call the main program

if __name__ == '__main__':
    main()
    sys.exit(1)
