"""Utility routines depending on the finder,
a combination of code by Jack Jansen and erik@letterror.com.

Most events have been captured from
Lasso Capture AE and than translated to python code.

IMPORTANT
Note that the processes() function returns different values
depending on the OS version it is running on. On MacOS 9
the Finder returns the process *names* which can then be
used to find out more about them. On MacOS 8.6 and earlier
the Finder returns a code which does not seem to work.
So bottom line: the processes() stuff does not work on < MacOS9

Mostly written by erik@letterror.com
"""
import Finder
from Carbon import AppleEvents
import aetools
import MacOS
import sys
import Carbon.File
import Carbon.Folder
import aetypes
from types import *

__version__ = '1.1'
Error = 'findertools.Error'

_finder_talker = None

def _getfinder():
    """returns basic (recyclable) Finder AE interface object"""
    global _finder_talker
    if not _finder_talker:
        _finder_talker = Finder.Finder()
    _finder_talker.send_flags = ( _finder_talker.send_flags |
        AppleEvents.kAECanInteract | AppleEvents.kAECanSwitchLayer)
    return _finder_talker

def launch(file):
    """Open a file thru the finder. Specify file by name or fsspec"""
    finder = _getfinder()
    fss = Carbon.File.FSSpec(file)
    return finder.open(fss)

def Print(file):
    """Print a file thru the finder. Specify file by name or fsspec"""
    finder = _getfinder()
    fss = Carbon.File.FSSpec(file)
    return finder._print(fss)

def copy(src, dstdir):
    """Copy a file to a folder"""
    finder = _getfinder()
    if type(src) == type([]):
        src_fss = []
        for s in src:
            src_fss.append(Carbon.File.FSSpec(s))
    else:
        src_fss = Carbon.File.FSSpec(src)
    dst_fss = Carbon.File.FSSpec(dstdir)
    return finder.duplicate(src_fss, to=dst_fss)

def move(src, dstdir):
    """Move a file to a folder"""
    finder = _getfinder()
    if type(src) == type([]):
        src_fss = []
        for s in src:
            src_fss.append(Carbon.File.FSSpec(s))
    else:
        src_fss = Carbon.File.FSSpec(src)
    dst_fss = Carbon.File.FSSpec(dstdir)
    return finder.move(src_fss, to=dst_fss)

def sleep():
    """Put the mac to sleep"""
    finder = _getfinder()
    finder.sleep()

def shutdown():
    """Shut the mac down"""
    finder = _getfinder()
    finder.shut_down()

def restart():
    """Restart the mac"""
    finder = _getfinder()
    finder.restart()


#---------------------------------------------------
#   Additional findertools
#

def reveal(file):
    """Reveal a file in the finder. Specify file by name, fsref or fsspec."""
    finder = _getfinder()
    fsr = Carbon.File.FSRef(file)
    file_alias = fsr.FSNewAliasMinimal()
    return finder.reveal(file_alias)

def select(file):
    """select a file in the finder. Specify file by name, fsref or fsspec."""
    finder = _getfinder()
    fsr = Carbon.File.FSRef(file)
    file_alias = fsr.FSNewAliasMinimal()
    return finder.select(file_alias)

def update(file):
    """Update the display of the specified object(s) to match
    their on-disk representation. Specify file by name, fsref or fsspec."""
    finder = _getfinder()
    fsr = Carbon.File.FSRef(file)
    file_alias = fsr.FSNewAliasMinimal()
    return finder.update(file_alias)


#---------------------------------------------------
#   More findertools
#

def comment(object, comment=None):
    """comment: get or set the Finder-comment of the item, displayed in the 'Get Info' window."""
    object = Carbon.File.FSRef(object)
    object_alias = object.FSNewAliasMonimal()
    if comment == None:
        return _getcomment(object_alias)
    else:
        return _setcomment(object_alias, comment)

def _setcomment(object_alias, comment):
    finder = _getfinder()
    args = {}
    attrs = {}
    aeobj_00 = aetypes.ObjectSpecifier(want=aetypes.Type('cobj'), form="alis", seld=object_alias, fr=None)
    aeobj_01 = aetypes.ObjectSpecifier(want=aetypes.Type('prop'), form="prop", seld=aetypes.Type('comt'), fr=aeobj_00)
    args['----'] = aeobj_01
    args["data"] = comment
    _reply, args, attrs = finder.send("core", "setd", args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)
    if args.has_key('----'):
        return args['----']

def _getcomment(object_alias):
    finder = _getfinder()
    args = {}
    attrs = {}
    aeobj_00 = aetypes.ObjectSpecifier(want=aetypes.Type('cobj'), form="alis", seld=object_alias, fr=None)
    aeobj_01 = aetypes.ObjectSpecifier(want=aetypes.Type('prop'), form="prop", seld=aetypes.Type('comt'), fr=aeobj_00)
    args['----'] = aeobj_01
    _reply, args, attrs = finder.send("core", "getd", args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)
    if args.has_key('----'):
        return args['----']


#---------------------------------------------------
#   Get information about current processes in the Finder.

def processes():
    """processes returns a list of all active processes running on this computer and their creators."""
    finder = _getfinder()
    args = {}
    attrs = {}
    processnames = []
    processnumbers = []
    creators = []
    partitions = []
    used = []
    ## get the processnames or else the processnumbers
    args['----'] = aetypes.ObjectSpecifier(want=aetypes.Type('prcs'), form="indx", seld=aetypes.Unknown('abso', "all "), fr=None)
    _reply, args, attrs = finder.send('core', 'getd', args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)
    p = []
    if args.has_key('----'):
        p =  args['----']
        for proc in p:
            if hasattr(proc, 'seld'):
                # it has a real name
                processnames.append(proc.seld)
            elif hasattr(proc, 'type'):
                if proc.type == "psn ":
                    # it has a process number
                    processnumbers.append(proc.data)
    ## get the creators
    args = {}
    attrs = {}
    aeobj_0 = aetypes.ObjectSpecifier(want=aetypes.Type('prcs'), form="indx", seld=aetypes.Unknown('abso', "all "), fr=None)
    args['----'] =  aetypes.ObjectSpecifier(want=aetypes.Type('prop'), form="prop", seld=aetypes.Type('fcrt'), fr=aeobj_0)
    _reply, args, attrs = finder.send('core', 'getd', args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(_arg)
    if args.has_key('----'):
        p =  args['----']
        creators = p[:]
    ## concatenate in one dict
    result = []
    if len(processnames) > len(processnumbers):
        data = processnames
    else:
        data = processnumbers
    for i in range(len(creators)):
        result.append((data[i], creators[i]))
    return result

class _process:
    pass

def isactiveprocess(processname):
    """Check of processname is active. MacOS9"""
    all = processes()
    ok = 0
    for n, c in all:
        if n == processname:
            return 1
    return 0

def processinfo(processname):
    """Return an object with all process properties as attributes for processname. MacOS9"""
    p = _process()

    if processname == "Finder":
        p.partition = None
        p.used = None
    else:
        p.partition = _processproperty(processname, 'appt')
        p.used = _processproperty(processname, 'pusd')
    p.visible = _processproperty(processname, 'pvis')       #Is the process' layer visible?
    p.frontmost = _processproperty(processname, 'pisf') #Is the process the frontmost process?
    p.file = _processproperty(processname, 'file')          #the file from which the process was launched
    p.filetype  = _processproperty(processname, 'asty')     #the OSType of the file type of the process
    p.creatortype = _processproperty(processname, 'fcrt')   #the OSType of the creator of the process (the signature)
    p.accepthighlevel = _processproperty(processname, 'revt')   #Is the process high-level event aware (accepts open application, open document, print document, and quit)?
    p.hasscripting = _processproperty(processname, 'hscr')      #Does the process have a scripting terminology, i.e., can it be scripted?
    return p

def _processproperty(processname, property):
    """return the partition size and memory used for processname"""
    finder = _getfinder()
    args = {}
    attrs = {}
    aeobj_00 = aetypes.ObjectSpecifier(want=aetypes.Type('prcs'), form="name", seld=processname, fr=None)
    aeobj_01 = aetypes.ObjectSpecifier(want=aetypes.Type('prop'), form="prop", seld=aetypes.Type(property), fr=aeobj_00)
    args['----'] = aeobj_01
    _reply, args, attrs = finder.send("core", "getd", args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)
    if args.has_key('----'):
        return args['----']


#---------------------------------------------------
#   Mess around with Finder windows.

def openwindow(object):
    """Open a Finder window for object, Specify object by name or fsspec."""
    finder = _getfinder()
    object = Carbon.File.FSRef(object)
    object_alias = object.FSNewAliasMinimal()
    args = {}
    attrs = {}
    _code = 'aevt'
    _subcode = 'odoc'
    aeobj_0 = aetypes.ObjectSpecifier(want=aetypes.Type('cfol'), form="alis", seld=object_alias, fr=None)
    args['----'] = aeobj_0
    _reply, args, attrs = finder.send(_code, _subcode, args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)

def closewindow(object):
    """Close a Finder window for folder, Specify by path."""
    finder = _getfinder()
    object = Carbon.File.FSRef(object)
    object_alias = object.FSNewAliasMinimal()
    args = {}
    attrs = {}
    _code = 'core'
    _subcode = 'clos'
    aeobj_0 = aetypes.ObjectSpecifier(want=aetypes.Type('cfol'), form="alis", seld=object_alias, fr=None)
    args['----'] = aeobj_0
    _reply, args, attrs = finder.send(_code, _subcode, args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)

def location(object, pos=None):
    """Set the position of a Finder window for folder to pos=(w, h). Specify file by name or fsspec.
    If pos=None, location will return the current position of the object."""
    object = Carbon.File.FSRef(object)
    object_alias = object.FSNewAliasMinimal()
    if not pos:
        return _getlocation(object_alias)
    return _setlocation(object_alias, pos)

def _setlocation(object_alias, (x, y)):
    """_setlocation: Set the location of the icon for the object."""
    finder = _getfinder()
    args = {}
    attrs = {}
    aeobj_00 = aetypes.ObjectSpecifier(want=aetypes.Type('cfol'), form="alis", seld=object_alias, fr=None)
    aeobj_01 = aetypes.ObjectSpecifier(want=aetypes.Type('prop'), form="prop", seld=aetypes.Type('posn'), fr=aeobj_00)
    args['----'] = aeobj_01
    args["data"] = [x, y]
    _reply, args, attrs = finder.send("core", "setd", args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)
    return (x,y)

def _getlocation(object_alias):
    """_getlocation: get the location of the icon for the object."""
    finder = _getfinder()
    args = {}
    attrs = {}
    aeobj_00 = aetypes.ObjectSpecifier(want=aetypes.Type('cfol'), form="alis", seld=object_alias, fr=None)
    aeobj_01 = aetypes.ObjectSpecifier(want=aetypes.Type('prop'), form="prop", seld=aetypes.Type('posn'), fr=aeobj_00)
    args['----'] = aeobj_01
    _reply, args, attrs = finder.send("core", "getd", args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)
    if args.has_key('----'):
        pos = args['----']
        return pos.h, pos.v

def label(object, index=None):
    """label: set or get the label of the item. Specify file by name or fsspec."""
    object = Carbon.File.FSRef(object)
    object_alias = object.FSNewAliasMinimal()
    if index == None:
        return _getlabel(object_alias)
    if index < 0 or index > 7:
        index = 0
    return _setlabel(object_alias, index)

def _getlabel(object_alias):
    """label: Get the label for the object."""
    finder = _getfinder()
    args = {}
    attrs = {}
    aeobj_00 = aetypes.ObjectSpecifier(want=aetypes.Type('cobj'), form="alis", seld=object_alias, fr=None)
    aeobj_01 = aetypes.ObjectSpecifier(want=aetypes.Type('prop'), form="prop", seld=aetypes.Type('labi'), fr=aeobj_00)
    args['----'] = aeobj_01
    _reply, args, attrs = finder.send("core", "getd", args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)
    if args.has_key('----'):
        return args['----']

def _setlabel(object_alias, index):
    """label: Set the label for the object."""
    finder = _getfinder()
    args = {}
    attrs = {}
    _code = 'core'
    _subcode = 'setd'
    aeobj_0 = aetypes.ObjectSpecifier(want=aetypes.Type('prop'),
            form="alis", seld=object_alias, fr=None)
    aeobj_1 = aetypes.ObjectSpecifier(want=aetypes.Type('prop'),
            form="prop", seld=aetypes.Type('labi'), fr=aeobj_0)
    args['----'] = aeobj_1
    args["data"] = index
    _reply, args, attrs = finder.send(_code, _subcode, args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)
    return index

def windowview(folder, view=None):
    """windowview: Set the view of the window for the folder. Specify file by name or fsspec.
    0 = by icon (default)
    1 = by name
    2 = by button
    """
    fsr = Carbon.File.FSRef(folder)
    folder_alias = fsr.FSNewAliasMinimal()
    if view == None:
        return _getwindowview(folder_alias)
    return _setwindowview(folder_alias, view)

def _setwindowview(folder_alias, view=0):
    """set the windowview"""
    attrs = {}
    args = {}
    if view == 1:
        _v = aetypes.Type('pnam')
    elif view == 2:
        _v = aetypes.Type('lgbu')
    else:
        _v = aetypes.Type('iimg')
    finder = _getfinder()
    aeobj_0 = aetypes.ObjectSpecifier(want = aetypes.Type('cfol'),
            form = 'alis', seld = folder_alias, fr=None)
    aeobj_1 = aetypes.ObjectSpecifier(want = aetypes.Type('prop'),
            form = 'prop', seld = aetypes.Type('cwnd'), fr=aeobj_0)
    aeobj_2 = aetypes.ObjectSpecifier(want = aetypes.Type('prop'),
            form = 'prop', seld = aetypes.Type('pvew'), fr=aeobj_1)
    aeobj_3 = aetypes.ObjectSpecifier(want = aetypes.Type('prop'),
            form = 'prop', seld = _v, fr=None)
    _code = 'core'
    _subcode = 'setd'
    args['----'] = aeobj_2
    args['data'] = aeobj_3
    _reply, args, attrs = finder.send(_code, _subcode, args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)
    if args.has_key('----'):
        return args['----']

def _getwindowview(folder_alias):
    """get the windowview"""
    attrs = {}
    args = {}
    finder = _getfinder()
    args = {}
    attrs = {}
    aeobj_00 = aetypes.ObjectSpecifier(want=aetypes.Type('cfol'), form="alis", seld=folder_alias, fr=None)
    aeobj_01 = aetypes.ObjectSpecifier(want=aetypes.Type('prop'), form="prop", seld=aetypes.Type('cwnd'), fr=aeobj_00)
    aeobj_02 = aetypes.ObjectSpecifier(want=aetypes.Type('prop'), form="prop", seld=aetypes.Type('pvew'), fr=aeobj_01)
    args['----'] = aeobj_02
    _reply, args, attrs = finder.send("core", "getd", args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)
    views = {'iimg':0, 'pnam':1, 'lgbu':2}
    if args.has_key('----'):
        return views[args['----'].enum]

def windowsize(folder, size=None):
    """Set the size of a Finder window for folder to size=(w, h), Specify by path.
    If size=None, windowsize will return the current size of the window.
    Specify file by name or fsspec.
    """
    fsr = Carbon.File.FSRef(folder)
    folder_alias = fsr.FSNewAliasMinimal()
    openwindow(fsr)
    if not size:
        return _getwindowsize(folder_alias)
    return _setwindowsize(folder_alias, size)

def _setwindowsize(folder_alias, (w, h)):
    """Set the size of a Finder window for folder to (w, h)"""
    finder = _getfinder()
    args = {}
    attrs = {}
    _code = 'core'
    _subcode = 'setd'
    aevar00 = [w, h]
    aeobj_0 = aetypes.ObjectSpecifier(want=aetypes.Type('cfol'),
            form="alis", seld=folder_alias, fr=None)
    aeobj_1 = aetypes.ObjectSpecifier(want=aetypes.Type('prop'),
            form="prop", seld=aetypes.Type('cwnd'), fr=aeobj_0)
    aeobj_2 = aetypes.ObjectSpecifier(want=aetypes.Type('prop'),
            form="prop", seld=aetypes.Type('ptsz'), fr=aeobj_1)
    args['----'] = aeobj_2
    args["data"] = aevar00
    _reply, args, attrs = finder.send(_code, _subcode, args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)
    return (w, h)

def _getwindowsize(folder_alias):
    """Set the size of a Finder window for folder to (w, h)"""
    finder = _getfinder()
    args = {}
    attrs = {}
    aeobj_0 = aetypes.ObjectSpecifier(want=aetypes.Type('cfol'),
            form="alis", seld=folder_alias, fr=None)
    aeobj_1 = aetypes.ObjectSpecifier(want=aetypes.Type('prop'),
            form="prop", seld=aetypes.Type('cwnd'), fr=aeobj_0)
    aeobj_2 = aetypes.ObjectSpecifier(want=aetypes.Type('prop'),
            form="prop", seld=aetypes.Type('posn'), fr=aeobj_1)
    args['----'] = aeobj_2
    _reply, args, attrs = finder.send('core', 'getd', args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)
    if args.has_key('----'):
        return args['----']

def windowposition(folder, pos=None):
    """Set the position of a Finder window for folder to pos=(w, h)."""
    fsr = Carbon.File.FSRef(folder)
    folder_alias = fsr.FSNewAliasMinimal()
    openwindow(fsr)
    if not pos:
        return _getwindowposition(folder_alias)
    if type(pos) == InstanceType:
        # pos might be a QDPoint object as returned by _getwindowposition
        pos = (pos.h, pos.v)
    return _setwindowposition(folder_alias, pos)

def _setwindowposition(folder_alias, (x, y)):
    """Set the size of a Finder window for folder to (w, h)."""
    finder = _getfinder()
    args = {}
    attrs = {}
    aeobj_0 = aetypes.ObjectSpecifier(want=aetypes.Type('cfol'),
            form="alis", seld=folder_alias, fr=None)
    aeobj_1 = aetypes.ObjectSpecifier(want=aetypes.Type('prop'),
            form="prop", seld=aetypes.Type('cwnd'), fr=aeobj_0)
    aeobj_2 = aetypes.ObjectSpecifier(want=aetypes.Type('prop'),
            form="prop", seld=aetypes.Type('posn'), fr=aeobj_1)
    args['----'] = aeobj_2
    args["data"] = [x, y]
    _reply, args, attrs = finder.send('core', 'setd', args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)
    if args.has_key('----'):
        return args['----']

def _getwindowposition(folder_alias):
    """Get the size of a Finder window for folder, Specify by path."""
    finder = _getfinder()
    args = {}
    attrs = {}
    aeobj_0 = aetypes.ObjectSpecifier(want=aetypes.Type('cfol'),
            form="alis", seld=folder_alias, fr=None)
    aeobj_1 = aetypes.ObjectSpecifier(want=aetypes.Type('prop'),
            form="prop", seld=aetypes.Type('cwnd'), fr=aeobj_0)
    aeobj_2 = aetypes.ObjectSpecifier(want=aetypes.Type('prop'),
            form="prop", seld=aetypes.Type('ptsz'), fr=aeobj_1)
    args['----'] = aeobj_2
    _reply, args, attrs = finder.send('core', 'getd', args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)
    if args.has_key('----'):
        return args['----']

def icon(object, icondata=None):
    """icon sets the icon of object, if no icondata is given,
    icon will return an AE object with binary data for the current icon.
    If left untouched, this data can be used to paste the icon on another file.
    Development opportunity: get and set the data as PICT."""
    fsr = Carbon.File.FSRef(object)
    object_alias = fsr.FSNewAliasMinimal()
    if icondata == None:
        return _geticon(object_alias)
    return _seticon(object_alias, icondata)

def _geticon(object_alias):
    """get the icondata for object. Binary data of some sort."""
    finder = _getfinder()
    args = {}
    attrs = {}
    aeobj_00 = aetypes.ObjectSpecifier(want=aetypes.Type('cobj'),
            form="alis", seld=object_alias, fr=None)
    aeobj_01 = aetypes.ObjectSpecifier(want=aetypes.Type('prop'),
            form="prop", seld=aetypes.Type('iimg'), fr=aeobj_00)
    args['----'] = aeobj_01
    _reply, args, attrs = finder.send("core", "getd", args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)
    if args.has_key('----'):
        return args['----']

def _seticon(object_alias, icondata):
    """set the icondata for object, formatted as produced by _geticon()"""
    finder = _getfinder()
    args = {}
    attrs = {}
    aeobj_00 = aetypes.ObjectSpecifier(want=aetypes.Type('cobj'),
            form="alis", seld=object_alias, fr=None)
    aeobj_01 = aetypes.ObjectSpecifier(want=aetypes.Type('prop'),
            form="prop", seld=aetypes.Type('iimg'), fr=aeobj_00)
    args['----'] = aeobj_01
    args["data"] = icondata
    _reply, args, attrs = finder.send("core", "setd", args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)
    if args.has_key('----'):
        return args['----'].data


#---------------------------------------------------
#   Volumes and servers.

def mountvolume(volume, server=None, username=None, password=None):
    """mount a volume, local or on a server on AppleTalk.
    Note: mounting a ASIP server requires a different operation.
    server is the name of the server where the volume belongs
    username, password belong to a registered user of the volume."""
    finder = _getfinder()
    args = {}
    attrs = {}
    if password:
        args["PASS"] = password
    if username:
        args["USER"] = username
    if server:
        args["SRVR"] = server
    args['----'] = volume
    _reply, args, attrs = finder.send("aevt", "mvol", args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)
    if args.has_key('----'):
        return args['----']

def unmountvolume(volume):
    """unmount a volume that's on the desktop"""
    putaway(volume)

def putaway(object):
    """puth the object away, whereever it came from."""
    finder = _getfinder()
    args = {}
    attrs = {}
    args['----'] = aetypes.ObjectSpecifier(want=aetypes.Type('cdis'), form="name", seld=object, fr=None)
    _reply, args, attrs = talker.send("fndr", "ptwy", args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)
    if args.has_key('----'):
        return args['----']


#---------------------------------------------------
#   Miscellaneous functions
#

def volumelevel(level):
    """set the audio output level, parameter between 0 (silent) and 7 (full blast)"""
    finder = _getfinder()
    args = {}
    attrs = {}
    if level < 0:
        level = 0
    elif level > 7:
        level = 7
    args['----'] = level
    _reply, args, attrs = finder.send("aevt", "stvl", args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)
    if args.has_key('----'):
        return args['----']

def OSversion():
    """return the version of the system software"""
    finder = _getfinder()
    args = {}
    attrs = {}
    aeobj_00 = aetypes.ObjectSpecifier(want=aetypes.Type('prop'), form="prop", seld=aetypes.Type('ver2'), fr=None)
    args['----'] = aeobj_00
    _reply, args, attrs = finder.send("core", "getd", args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)
    if args.has_key('----'):
        return args['----']

def filesharing():
    """return the current status of filesharing and whether it is starting up or not:
        -1  file sharing is off and not starting up
        0   file sharing is off and starting up
        1   file sharing is on"""
    status = -1
    finder = _getfinder()
    # see if it is on
    args = {}
    attrs = {}
    args['----'] = aetypes.ObjectSpecifier(want=aetypes.Type('prop'), form="prop", seld=aetypes.Type('fshr'), fr=None)
    _reply, args, attrs = finder.send("core", "getd", args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)
    if args.has_key('----'):
        if args['----'] == 0:
            status = -1
        else:
            status = 1
    # is it starting up perchance?
    args = {}
    attrs = {}
    args['----'] = aetypes.ObjectSpecifier(want=aetypes.Type('prop'), form="prop", seld=aetypes.Type('fsup'), fr=None)
    _reply, args, attrs = finder.send("core", "getd", args, attrs)
    if args.has_key('errn'):
        raise Error, aetools.decodeerror(args)
    if args.has_key('----'):
        if args['----'] == 1:
            status = 0
    return status

def movetotrash(path):
    """move the object to the trash"""
    fss = Carbon.File.FSSpec(path)
    trashfolder = Carbon.Folder.FSFindFolder(fss.as_tuple()[0], 'trsh', 0)
    move(path, trashfolder)

def emptytrash():
    """empty the trash"""
    finder = _getfinder()
    args = {}
    attrs = {}
    args['----'] = aetypes.ObjectSpecifier(want=aetypes.Type('prop'), form="prop", seld=aetypes.Type('trsh'), fr=None)
    _reply, args, attrs = finder.send("fndr", "empt", args, attrs)
    if args.has_key('errn'):
        raise aetools.Error, aetools.decodeerror(args)


def _test():
    import EasyDialogs
    print 'Original findertools functionality test...'
    print 'Testing launch...'
    pathname = EasyDialogs.AskFileForOpen('File to launch:')
    if pathname:
        result = launch(pathname)
        if result:
            print 'Result: ', result
        print 'Press return-',
        sys.stdin.readline()
    print 'Testing print...'
    pathname = EasyDialogs.AskFileForOpen('File to print:')
    if pathname:
        result = Print(pathname)
        if result:
            print 'Result: ', result
        print 'Press return-',
        sys.stdin.readline()
    print 'Testing copy...'
    pathname = EasyDialogs.AskFileForOpen('File to copy:')
    if pathname:
        destdir = EasyDialogs.AskFolder('Destination:')
        if destdir:
            result = copy(pathname, destdir)
            if result:
                print 'Result:', result
            print 'Press return-',
            sys.stdin.readline()
    print 'Testing move...'
    pathname = EasyDialogs.AskFileForOpen('File to move:')
    if pathname:
        destdir = EasyDialogs.AskFolder('Destination:')
        if destdir:
            result = move(pathname, destdir)
            if result:
                print 'Result:', result
            print 'Press return-',
            sys.stdin.readline()
    print 'Testing sleep...'
    if EasyDialogs.AskYesNoCancel('Sleep?') > 0:
        result = sleep()
        if result:
            print 'Result:', result
        print 'Press return-',
        sys.stdin.readline()
    print 'Testing shutdown...'
    if EasyDialogs.AskYesNoCancel('Shut down?') > 0:
        result = shutdown()
        if result:
            print 'Result:', result
        print 'Press return-',
        sys.stdin.readline()
    print 'Testing restart...'
    if EasyDialogs.AskYesNoCancel('Restart?') > 0:
        result = restart()
        if result:
            print 'Result:', result
        print 'Press return-',
        sys.stdin.readline()

def _test2():
    print '\nmorefindertools version %s\nTests coming up...' %__version__
    import os
    import random

    # miscellaneous
    print '\tfilesharing on?',  filesharing()       # is file sharing on, off, starting up?
    print '\tOS version',       OSversion()     # the version of the system software

    # set the soundvolume in a simple way
    print '\tSystem beep volume'
    for i in range(0, 7):
        volumelevel(i)
        MacOS.SysBeep()

    # Finder's windows, file location, file attributes
    open("@findertoolstest", "w")
    f = "@findertoolstest"
    reveal(f)               # reveal this file in a Finder window
    select(f)               # select this file

    base, file = os.path.split(f)
    closewindow(base)   # close the window this file is in  (opened by reveal)
    openwindow(base)        # open it again
    windowview(base, 1) # set the view by list

    label(f, 2)             # set the label of this file to something orange
    print '\tlabel', label(f)   # get the label of this file

    # the file location only works in a window with icon view!
    print 'Random locations for an icon'
    windowview(base, 0)     # set the view by icon
    windowsize(base, (600, 600))
    for i in range(50):
        location(f, (random.randint(10, 590), random.randint(10, 590)))

    windowsize(base, (200, 400))
    windowview(base, 1)     # set the view by icon

    orgpos = windowposition(base)
    print 'Animated window location'
    for i in range(10):
        pos = (100+i*10, 100+i*10)
        windowposition(base, pos)
        print '\twindow position', pos
    windowposition(base, orgpos)    # park it where it was before

    print 'Put a comment in file', f, ':'
    print '\t', comment(f)      # print the Finder comment this file has
    s = 'This is a comment no one reads!'
    comment(f, s)           # set the Finder comment

def _test3():
    print 'MacOS9 or better specific functions'
    # processes
    pr = processes()        # return a list of tuples with (active_processname, creatorcode)
    print 'Return a list of current active processes:'
    for p in pr:
        print '\t', p

    # get attributes of the first process in the list
    print 'Attributes of the first process in the list:'
    pinfo = processinfo(pr[0][0])
    print '\t', pr[0][0]
    print '\t\tmemory partition', pinfo.partition       # the memory allocated to this process
    print '\t\tmemory used', pinfo.used         # the memory actuall used by this process
    print '\t\tis visible', pinfo.visible           # is the process visible to the user
    print '\t\tis frontmost', pinfo.frontmost       # is the process the front most one?
    print '\t\thas scripting', pinfo.hasscripting       # is the process scriptable?
    print '\t\taccepts high level events',  pinfo.accepthighlevel   # does the process accept high level appleevents?

if __name__ == '__main__':
    _test()
    _test2()
    _test3()
