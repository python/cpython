"""macgen_bin - Generate application from shared libraries"""

import os
import sys
import string
import types
import macfs
from MACFS import *
import MacOS
from Carbon import Res
import py_resource
import cfmfile
import buildtools


def generate(input, output, module_dict=None, architecture='fat', debug=0):
    # try to remove old file
    try:
        os.remove(output)
    except:
        pass

    if module_dict is None:
        import macmodulefinder
        print "Searching for modules..."
        module_dict, missing = macmodulefinder.process(input, [], [], 1)
        if missing:
            import EasyDialogs
            missing.sort()
            answer = EasyDialogs.AskYesNoCancel("Some modules could not be found; continue anyway?\n(%s)"
                            % string.join(missing, ", "))
            if answer <> 1:
                sys.exit(0)

    applettemplatepath = buildtools.findtemplate()
    corepath = findpythoncore()

    dynamicmodules, dynamicfiles, extraresfiles = findfragments(module_dict, architecture)

    print 'Adding "__main__"'
    buildtools.process(applettemplatepath, input, output, 0)

    outputref = Res.FSpOpenResFile(output, 3)
    try:
        Res.UseResFile(outputref)

        print "Adding Python modules"
        addpythonmodules(module_dict)

        print "Adding PythonCore resources"
        copyres(corepath, outputref, ['cfrg', 'Popt', 'GU\267I'], 1)

        print "Adding resources from shared libraries"
        for ppcpath, cfm68kpath in extraresfiles:
            if os.path.exists(ppcpath):
                copyres(ppcpath, outputref, ['cfrg'], 1)
            elif os.path.exists(cfm68kpath):
                copyres(cfm68kpath, outputref, ['cfrg'], 1)

        print "Fixing sys.path prefs"
        Res.UseResFile(outputref)
        try:
            res = Res.Get1Resource('STR#', 228) # from PythonCore
        except Res.Error: pass
        else:
            res.RemoveResource()
        # setting pref file name to empty string
        res = Res.Get1NamedResource('STR ', "PythonPreferenceFileName")
        res.data = Pstring("")
        res.ChangedResource()
        syspathpref = "$(APPLICATION)"
        res = Res.Resource("\000\001" + Pstring(syspathpref))
        res.AddResource("STR#", 229, "sys.path preference")

        print "Creating 'PYD ' resources"
        for modname, (ppcfrag, cfm68kfrag) in dynamicmodules.items():
            res = Res.Resource(Pstring(ppcfrag) + Pstring(cfm68kfrag))
            id = 0
            while id < 128:
                id = Res.Unique1ID('PYD ')
            res.AddResource('PYD ', id, modname)
    finally:
        Res.CloseResFile(outputref)
    print "Merging code fragments"
    cfmfile.mergecfmfiles([applettemplatepath, corepath] + dynamicfiles.keys(),
                    output, architecture)

    print "done!"


def findfragments(module_dict, architecture):
    dynamicmodules = {}
    dynamicfiles = {}
    extraresfiles = []
    for name, module in module_dict.items():
        if module.gettype() <> 'dynamic':
            continue
        path = resolvealiasfile(module.__file__)
        dir, filename = os.path.split(path)
##              ppcfile, cfm68kfile = makefilenames(filename)
        ppcfile = filename
        cfm68kfile = "dummy.cfm68k.slb"

        # ppc stuff
        ppcpath = os.path.join(dir, ppcfile)
        if architecture <> 'm68k':
            ppcfrag, dynamicfiles = getfragname(ppcpath, dynamicfiles)
        else:
            ppcfrag = "_no_fragment_"

        # 68k stuff
        cfm68kpath = os.path.join(dir, cfm68kfile)
        if architecture <> 'pwpc':
            cfm68kfrag, dynamicfiles = getfragname(cfm68kpath, dynamicfiles)
        else:
            cfm68kfrag = "_no_fragment_"

        dynamicmodules[name] = ppcfrag, cfm68kfrag
        if (ppcpath, cfm68kpath) not in extraresfiles:
            extraresfiles.append((ppcpath, cfm68kpath))
    return dynamicmodules, dynamicfiles, extraresfiles


def getfragname(path, dynamicfiles):
    if not dynamicfiles.has_key(path):
        if os.path.exists(path):
            lib = cfmfile.CfrgResource(path)
            fragname = lib.fragments[0].name
        else:
            print "shared lib not found:", path
            fragname = "_no_fragment_"
        dynamicfiles[path] = fragname
    else:
        fragname = dynamicfiles[path]
    return fragname, dynamicfiles


def addpythonmodules(module_dict):
    # XXX should really use macgen_rsrc.generate(), this does the same, but skips __main__
    items = module_dict.items()
    items.sort()
    for name, module in items:
        mtype = module.gettype()
        if mtype not in ['module', 'package'] or name == "__main__":
            continue
        location = module.__file__

        if location[-4:] == '.pyc':
            # Attempt corresponding .py
            location = location[:-1]
        if location[-3:] != '.py':
            print '*** skipping', location
            continue

        print 'Adding module "%s"' % name
        id, name = py_resource.frompyfile(location, name, preload=0,
                        ispackage=mtype=='package')

def Pstring(str):
    if len(str) > 255:
        raise TypeError, "Str255 must be at most 255 chars long"
    return chr(len(str)) + str

##def makefilenames(name):
##      lname = string.lower(name)
##      pos = string.find(lname, ".ppc.")
##      if pos > 0:
##              return name, name[:pos] + '.CFM68K.' + name[pos+5:]
##      pos = string.find(lname, ".cfm68k.")
##      if pos > 0:
##              return name[:pos] + '.ppc.' + name[pos+8:], name
##      raise ValueError, "can't make ppc/cfm68k filenames"

def copyres(input, output, *args, **kwargs):
    openedin = openedout = 0
    if type(input) == types.StringType:
        input = Res.FSpOpenResFile(input, 1)
        openedin = 1
    if type(output) == types.StringType:
        output = Res.FSpOpenResFile(output, 3)
        openedout = 1
    try:
        apply(buildtools.copyres, (input, output) + args, kwargs)
    finally:
        if openedin:
            Res.CloseResFile(input)
        if openedout:
            Res.CloseResFile(output)

def findpythoncore():
    """find the PythonCore shared library, possibly asking the user if we can't find it"""

    try:
        vRefNum, dirID = macfs.FindFolder(kOnSystemDisk, kSharedLibrariesFolderType, 0)
    except macfs.error:
        extpath = ":"
    else:
        extpath = macfs.FSSpec((vRefNum, dirID, "")).as_pathname()
    version = string.split(sys.version)[0]
    if MacOS.runtimemodel == 'carbon':
        corename = "PythonCoreCarbon " + version
    elif MacOS.runtimemodel == 'ppc':
        corename = "PythonCore " + version
    else:
        raise "Unknown MacOS.runtimemodel", MacOS.runtimemodel
    corepath = os.path.join(extpath, corename)
    if not os.path.exists(corepath):
        corepath = EasyDialogs.AskFileForOpen(message="Please locate PythonCore:",
                typeList=("shlb",))
        if not corepath:
            raise KeyboardInterrupt, "cancelled"
    return resolvealiasfile(corepath)

def resolvealiasfile(path):
    try:
        fss, dummy1, dummy2 = macfs.ResolveAliasFile(path)
    except macfs.error:
        pass
    else:
        path = fss.as_pathname()
    return path
