"""Easy to use dialogs.

Message(msg) -- display a message and an OK button.
AskString(prompt, default) -- ask for a string, display OK and Cancel buttons.
AskPassword(prompt, default) -- like AskString(), but shows text as bullets.
AskYesNoCancel(question, default) -- display a question and Yes, No and Cancel buttons.
GetArgv(optionlist, commandlist) -- fill a sys.argv-like list using a dialog
AskFileForOpen(...) -- Ask the user for an existing file
AskFileForSave(...) -- Ask the user for an output file
AskFolder(...) -- Ask the user to select a folder
bar = Progress(label, maxvalue) -- Display a progress bar
bar.set(value) -- Set value
bar.inc( *amount ) -- increment value by amount (default=1)
bar.label( *newlabel ) -- get or set text label.

More documentation in each function.
This module uses DLOG resources 260 and on.
Based upon STDWIN dialogs with the same names and functions.
"""

from Carbon.Dlg import GetNewDialog, SetDialogItemText, GetDialogItemText, ModalDialog
from Carbon import Qd
from Carbon import QuickDraw
from Carbon import Dialogs
from Carbon import Windows
from Carbon import Dlg,Win,Evt,Events # sdm7g
from Carbon import Ctl
from Carbon import Controls
from Carbon import Menu
from Carbon import AE
import Nav
import MacOS
import string
from Carbon.ControlAccessor import *    # Also import Controls constants
import Carbon.File
import macresource
import os
import sys

__all__ = ['Message', 'AskString', 'AskPassword', 'AskYesNoCancel',
    'GetArgv', 'AskFileForOpen', 'AskFileForSave', 'AskFolder',
    'ProgressBar']

_initialized = 0

def _initialize():
    global _initialized
    if _initialized: return
    macresource.need("DLOG", 260, "dialogs.rsrc", __name__)

def _interact():
    """Make sure the application is in the foreground"""
    AE.AEInteractWithUser(50000000)

def cr2lf(text):
    if '\r' in text:
        text = string.join(string.split(text, '\r'), '\n')
    return text

def lf2cr(text):
    if '\n' in text:
        text = string.join(string.split(text, '\n'), '\r')
    if len(text) > 253:
        text = text[:253] + '\311'
    return text

def Message(msg, id=260, ok=None):
    """Display a MESSAGE string.

    Return when the user clicks the OK button or presses Return.

    The MESSAGE string can be at most 255 characters long.
    """
    _initialize()
    _interact()
    d = GetNewDialog(id, -1)
    if not d:
        print "EasyDialogs: Can't get DLOG resource with id =", id, " (missing resource file?)"
        return
    h = d.GetDialogItemAsControl(2)
    SetDialogItemText(h, lf2cr(msg))
    if ok != None:
        h = d.GetDialogItemAsControl(1)
        h.SetControlTitle(ok)
    d.SetDialogDefaultItem(1)
    d.AutoSizeDialog()
    d.GetDialogWindow().ShowWindow()
    while 1:
        n = ModalDialog(None)
        if n == 1:
            return


def AskString(prompt, default = "", id=261, ok=None, cancel=None):
    """Display a PROMPT string and a text entry field with a DEFAULT string.

    Return the contents of the text entry field when the user clicks the
    OK button or presses Return.
    Return None when the user clicks the Cancel button.

    If omitted, DEFAULT is empty.

    The PROMPT and DEFAULT strings, as well as the return value,
    can be at most 255 characters long.
    """

    _initialize()
    _interact()
    d = GetNewDialog(id, -1)
    if not d:
        print "EasyDialogs: Can't get DLOG resource with id =", id, " (missing resource file?)"
        return
    h = d.GetDialogItemAsControl(3)
    SetDialogItemText(h, lf2cr(prompt))
    h = d.GetDialogItemAsControl(4)
    SetDialogItemText(h, lf2cr(default))
    d.SelectDialogItemText(4, 0, 999)
#       d.SetDialogItem(4, 0, 255)
    if ok != None:
        h = d.GetDialogItemAsControl(1)
        h.SetControlTitle(ok)
    if cancel != None:
        h = d.GetDialogItemAsControl(2)
        h.SetControlTitle(cancel)
    d.SetDialogDefaultItem(1)
    d.SetDialogCancelItem(2)
    d.AutoSizeDialog()
    d.GetDialogWindow().ShowWindow()
    while 1:
        n = ModalDialog(None)
        if n == 1:
            h = d.GetDialogItemAsControl(4)
            return cr2lf(GetDialogItemText(h))
        if n == 2: return None

def AskPassword(prompt,  default='', id=264, ok=None, cancel=None):
    """Display a PROMPT string and a text entry field with a DEFAULT string.
    The string is displayed as bullets only.

    Return the contents of the text entry field when the user clicks the
    OK button or presses Return.
    Return None when the user clicks the Cancel button.

    If omitted, DEFAULT is empty.

    The PROMPT and DEFAULT strings, as well as the return value,
    can be at most 255 characters long.
    """
    _initialize()
    _interact()
    d = GetNewDialog(id, -1)
    if not d:
        print "EasyDialogs: Can't get DLOG resource with id =", id, " (missing resource file?)"
        return
    h = d.GetDialogItemAsControl(3)
    SetDialogItemText(h, lf2cr(prompt))
    pwd = d.GetDialogItemAsControl(4)
    bullets = '\245'*len(default)
##      SetControlData(pwd, kControlEditTextPart, kControlEditTextTextTag, bullets)
    SetControlData(pwd, kControlEditTextPart, kControlEditTextPasswordTag, default)
    d.SelectDialogItemText(4, 0, 999)
    Ctl.SetKeyboardFocus(d.GetDialogWindow(), pwd, kControlEditTextPart)
    if ok != None:
        h = d.GetDialogItemAsControl(1)
        h.SetControlTitle(ok)
    if cancel != None:
        h = d.GetDialogItemAsControl(2)
        h.SetControlTitle(cancel)
    d.SetDialogDefaultItem(Dialogs.ok)
    d.SetDialogCancelItem(Dialogs.cancel)
    d.AutoSizeDialog()
    d.GetDialogWindow().ShowWindow()
    while 1:
        n = ModalDialog(None)
        if n == 1:
            h = d.GetDialogItemAsControl(4)
            return cr2lf(GetControlData(pwd, kControlEditTextPart, kControlEditTextPasswordTag))
        if n == 2: return None

def AskYesNoCancel(question, default = 0, yes=None, no=None, cancel=None, id=262):
    """Display a QUESTION string which can be answered with Yes or No.

    Return 1 when the user clicks the Yes button.
    Return 0 when the user clicks the No button.
    Return -1 when the user clicks the Cancel button.

    When the user presses Return, the DEFAULT value is returned.
    If omitted, this is 0 (No).

    The QUESTION string can be at most 255 characters.
    """

    _initialize()
    _interact()
    d = GetNewDialog(id, -1)
    if not d:
        print "EasyDialogs: Can't get DLOG resource with id =", id, " (missing resource file?)"
        return
    # Button assignments:
    # 1 = default (invisible)
    # 2 = Yes
    # 3 = No
    # 4 = Cancel
    # The question string is item 5
    h = d.GetDialogItemAsControl(5)
    SetDialogItemText(h, lf2cr(question))
    if yes != None:
        if yes == '':
            d.HideDialogItem(2)
        else:
            h = d.GetDialogItemAsControl(2)
            h.SetControlTitle(yes)
    if no != None:
        if no == '':
            d.HideDialogItem(3)
        else:
            h = d.GetDialogItemAsControl(3)
            h.SetControlTitle(no)
    if cancel != None:
        if cancel == '':
            d.HideDialogItem(4)
        else:
            h = d.GetDialogItemAsControl(4)
            h.SetControlTitle(cancel)
    d.SetDialogCancelItem(4)
    if default == 1:
        d.SetDialogDefaultItem(2)
    elif default == 0:
        d.SetDialogDefaultItem(3)
    elif default == -1:
        d.SetDialogDefaultItem(4)
    d.AutoSizeDialog()
    d.GetDialogWindow().ShowWindow()
    while 1:
        n = ModalDialog(None)
        if n == 1: return default
        if n == 2: return 1
        if n == 3: return 0
        if n == 4: return -1




screenbounds = Qd.GetQDGlobalsScreenBits().bounds
screenbounds = screenbounds[0]+4, screenbounds[1]+4, \
    screenbounds[2]-4, screenbounds[3]-4

kControlProgressBarIndeterminateTag = 'inde'    # from Controls.py


class ProgressBar:
    def __init__(self, title="Working...", maxval=0, label="", id=263):
        self.w = None
        self.d = None
        _initialize()
        self.d = GetNewDialog(id, -1)
        self.w = self.d.GetDialogWindow()
        self.label(label)
        self.title(title)
        self.set(0, maxval)
        self.d.AutoSizeDialog()
        self.w.ShowWindow()
        self.d.DrawDialog()

    def __del__( self ):
        if self.w:
            self.w.BringToFront()
            self.w.HideWindow()
        del self.w
        del self.d

    def title(self, newstr=""):
        """title(text) - Set title of progress window"""
        self.w.BringToFront()
        self.w.SetWTitle(newstr)

    def label( self, *newstr ):
        """label(text) - Set text in progress box"""
        self.w.BringToFront()
        if newstr:
            self._label = lf2cr(newstr[0])
        text_h = self.d.GetDialogItemAsControl(2)
        SetDialogItemText(text_h, self._label)

    def _update(self, value):
        maxval = self.maxval
        if maxval == 0:     # an indeterminate bar
            Ctl.IdleControls(self.w)    # spin the barber pole
        else:               # a determinate bar
            if maxval > 32767:
                value = int(value/(maxval/32767.0))
                maxval = 32767
            maxval = int(maxval)
            value = int(value)
            progbar = self.d.GetDialogItemAsControl(3)
            progbar.SetControlMaximum(maxval)
            progbar.SetControlValue(value)  # set the bar length

        # Test for cancel button
        ready, ev = Evt.WaitNextEvent( Events.mDownMask, 1  )
        if ready :
            what,msg,when,where,mod = ev
            part = Win.FindWindow(where)[0]
            if Dlg.IsDialogEvent(ev):
                ds = Dlg.DialogSelect(ev)
                if ds[0] and ds[1] == self.d and ds[-1] == 1:
                    self.w.HideWindow()
                    self.w = None
                    self.d = None
                    raise KeyboardInterrupt, ev
            else:
                if part == 4:   # inDrag
                    self.w.DragWindow(where, screenbounds)
                else:
                    MacOS.HandleEvent(ev)


    def set(self, value, max=None):
        """set(value) - Set progress bar position"""
        if max != None:
            self.maxval = max
            bar = self.d.GetDialogItemAsControl(3)
            if max <= 0:    # indeterminate bar
                bar.SetControlData(0,kControlProgressBarIndeterminateTag,'\x01')
            else:           # determinate bar
                bar.SetControlData(0,kControlProgressBarIndeterminateTag,'\x00')
        if value < 0:
            value = 0
        elif value > self.maxval:
            value = self.maxval
        self.curval = value
        self._update(value)

    def inc(self, n=1):
        """inc(amt) - Increment progress bar position"""
        self.set(self.curval + n)

ARGV_ID=265
ARGV_ITEM_OK=1
ARGV_ITEM_CANCEL=2
ARGV_OPTION_GROUP=3
ARGV_OPTION_EXPLAIN=4
ARGV_OPTION_VALUE=5
ARGV_OPTION_ADD=6
ARGV_COMMAND_GROUP=7
ARGV_COMMAND_EXPLAIN=8
ARGV_COMMAND_ADD=9
ARGV_ADD_OLDFILE=10
ARGV_ADD_NEWFILE=11
ARGV_ADD_FOLDER=12
ARGV_CMDLINE_GROUP=13
ARGV_CMDLINE_DATA=14

##def _myModalDialog(d):
##      while 1:
##          ready, ev = Evt.WaitNextEvent(0xffff, -1)
##          print 'DBG: WNE', ready, ev
##          if ready :
##              what,msg,when,where,mod = ev
##              part, window = Win.FindWindow(where)
##              if Dlg.IsDialogEvent(ev):
##                  didit, dlgdone, itemdone = Dlg.DialogSelect(ev)
##                  print 'DBG: DialogSelect', didit, dlgdone, itemdone, d
##                  if didit and dlgdone == d:
##                      return itemdone
##              elif window == d.GetDialogWindow():
##                  d.GetDialogWindow().SelectWindow()
##                  if part == 4:   # inDrag
##                          d.DragWindow(where, screenbounds)
##                  else:
##                      MacOS.HandleEvent(ev)
##              else:
##                  MacOS.HandleEvent(ev)
##
def _setmenu(control, items):
    mhandle = control.GetControlData_Handle(Controls.kControlMenuPart,
            Controls.kControlPopupButtonMenuHandleTag)
    menu = Menu.as_Menu(mhandle)
    for item in items:
        if type(item) == type(()):
            label = item[0]
        else:
            label = item
        if label[-1] == '=' or label[-1] == ':':
            label = label[:-1]
        menu.AppendMenu(label)
##          mhandle, mid = menu.getpopupinfo()
##          control.SetControlData_Handle(Controls.kControlMenuPart,
##                  Controls.kControlPopupButtonMenuHandleTag, mhandle)
    control.SetControlMinimum(1)
    control.SetControlMaximum(len(items)+1)

def _selectoption(d, optionlist, idx):
    if idx < 0 or idx >= len(optionlist):
        MacOS.SysBeep()
        return
    option = optionlist[idx]
    if type(option) == type(()):
        if len(option) == 4:
            help = option[2]
        elif len(option) > 1:
            help = option[-1]
        else:
            help = ''
    else:
        help = ''
    h = d.GetDialogItemAsControl(ARGV_OPTION_EXPLAIN)
    if help and len(help) > 250:
        help = help[:250] + '...'
    Dlg.SetDialogItemText(h, help)
    hasvalue = 0
    if type(option) == type(()):
        label = option[0]
    else:
        label = option
    if label[-1] == '=' or label[-1] == ':':
        hasvalue = 1
    h = d.GetDialogItemAsControl(ARGV_OPTION_VALUE)
    Dlg.SetDialogItemText(h, '')
    if hasvalue:
        d.ShowDialogItem(ARGV_OPTION_VALUE)
        d.SelectDialogItemText(ARGV_OPTION_VALUE, 0, 0)
    else:
        d.HideDialogItem(ARGV_OPTION_VALUE)


def GetArgv(optionlist=None, commandlist=None, addoldfile=1, addnewfile=1, addfolder=1, id=ARGV_ID):
    _initialize()
    _interact()
    d = GetNewDialog(id, -1)
    if not d:
        print "EasyDialogs: Can't get DLOG resource with id =", id, " (missing resource file?)"
        return
#       h = d.GetDialogItemAsControl(3)
#       SetDialogItemText(h, lf2cr(prompt))
#       h = d.GetDialogItemAsControl(4)
#       SetDialogItemText(h, lf2cr(default))
#       d.SelectDialogItemText(4, 0, 999)
#       d.SetDialogItem(4, 0, 255)
    if optionlist:
        _setmenu(d.GetDialogItemAsControl(ARGV_OPTION_GROUP), optionlist)
        _selectoption(d, optionlist, 0)
    else:
        d.GetDialogItemAsControl(ARGV_OPTION_GROUP).DeactivateControl()
    if commandlist:
        _setmenu(d.GetDialogItemAsControl(ARGV_COMMAND_GROUP), commandlist)
        if type(commandlist[0]) == type(()) and len(commandlist[0]) > 1:
            help = commandlist[0][-1]
            h = d.GetDialogItemAsControl(ARGV_COMMAND_EXPLAIN)
            Dlg.SetDialogItemText(h, help)
    else:
        d.GetDialogItemAsControl(ARGV_COMMAND_GROUP).DeactivateControl()
    if not addoldfile:
        d.GetDialogItemAsControl(ARGV_ADD_OLDFILE).DeactivateControl()
    if not addnewfile:
        d.GetDialogItemAsControl(ARGV_ADD_NEWFILE).DeactivateControl()
    if not addfolder:
        d.GetDialogItemAsControl(ARGV_ADD_FOLDER).DeactivateControl()
    d.SetDialogDefaultItem(ARGV_ITEM_OK)
    d.SetDialogCancelItem(ARGV_ITEM_CANCEL)
    d.GetDialogWindow().ShowWindow()
    d.DrawDialog()
    if hasattr(MacOS, 'SchedParams'):
        appsw = MacOS.SchedParams(1, 0)
    try:
        while 1:
            stringstoadd = []
            n = ModalDialog(None)
            if n == ARGV_ITEM_OK:
                break
            elif n == ARGV_ITEM_CANCEL:
                raise SystemExit
            elif n == ARGV_OPTION_GROUP:
                idx = d.GetDialogItemAsControl(ARGV_OPTION_GROUP).GetControlValue()-1
                _selectoption(d, optionlist, idx)
            elif n == ARGV_OPTION_VALUE:
                pass
            elif n == ARGV_OPTION_ADD:
                idx = d.GetDialogItemAsControl(ARGV_OPTION_GROUP).GetControlValue()-1
                if 0 <= idx < len(optionlist):
                    option = optionlist[idx]
                    if type(option) == type(()):
                        option = option[0]
                    if option[-1] == '=' or option[-1] == ':':
                        option = option[:-1]
                        h = d.GetDialogItemAsControl(ARGV_OPTION_VALUE)
                        value = Dlg.GetDialogItemText(h)
                    else:
                        value = ''
                    if len(option) == 1:
                        stringtoadd = '-' + option
                    else:
                        stringtoadd = '--' + option
                    stringstoadd = [stringtoadd]
                    if value:
                        stringstoadd.append(value)
                else:
                    MacOS.SysBeep()
            elif n == ARGV_COMMAND_GROUP:
                idx = d.GetDialogItemAsControl(ARGV_COMMAND_GROUP).GetControlValue()-1
                if 0 <= idx < len(commandlist) and type(commandlist[idx]) == type(()) and \
                        len(commandlist[idx]) > 1:
                    help = commandlist[idx][-1]
                    h = d.GetDialogItemAsControl(ARGV_COMMAND_EXPLAIN)
                    Dlg.SetDialogItemText(h, help)
            elif n == ARGV_COMMAND_ADD:
                idx = d.GetDialogItemAsControl(ARGV_COMMAND_GROUP).GetControlValue()-1
                if 0 <= idx < len(commandlist):
                    command = commandlist[idx]
                    if type(command) == type(()):
                        command = command[0]
                    stringstoadd = [command]
                else:
                    MacOS.SysBeep()
            elif n == ARGV_ADD_OLDFILE:
                pathname = AskFileForOpen()
                if pathname:
                    stringstoadd = [pathname]
            elif n == ARGV_ADD_NEWFILE:
                pathname = AskFileForSave()
                if pathname:
                    stringstoadd = [pathname]
            elif n == ARGV_ADD_FOLDER:
                pathname = AskFolder()
                if pathname:
                    stringstoadd = [pathname]
            elif n == ARGV_CMDLINE_DATA:
                pass # Nothing to do
            else:
                raise RuntimeError, "Unknown dialog item %d"%n

            for stringtoadd in stringstoadd:
                if '"' in stringtoadd or "'" in stringtoadd or " " in stringtoadd:
                    stringtoadd = repr(stringtoadd)
                h = d.GetDialogItemAsControl(ARGV_CMDLINE_DATA)
                oldstr = GetDialogItemText(h)
                if oldstr and oldstr[-1] != ' ':
                    oldstr = oldstr + ' '
                oldstr = oldstr + stringtoadd
                if oldstr[-1] != ' ':
                    oldstr = oldstr + ' '
                SetDialogItemText(h, oldstr)
                d.SelectDialogItemText(ARGV_CMDLINE_DATA, 0x7fff, 0x7fff)
        h = d.GetDialogItemAsControl(ARGV_CMDLINE_DATA)
        oldstr = GetDialogItemText(h)
        tmplist = string.split(oldstr)
        newlist = []
        while tmplist:
            item = tmplist[0]
            del tmplist[0]
            if item[0] == '"':
                while item[-1] != '"':
                    if not tmplist:
                        raise RuntimeError, "Unterminated quoted argument"
                    item = item + ' ' + tmplist[0]
                    del tmplist[0]
                item = item[1:-1]
            if item[0] == "'":
                while item[-1] != "'":
                    if not tmplist:
                        raise RuntimeError, "Unterminated quoted argument"
                    item = item + ' ' + tmplist[0]
                    del tmplist[0]
                item = item[1:-1]
            newlist.append(item)
        return newlist
    finally:
        if hasattr(MacOS, 'SchedParams'):
            MacOS.SchedParams(*appsw)
        del d

def _process_Nav_args(dftflags, **args):
    import aepack
    import Carbon.AE
    import Carbon.File
    for k in args.keys():
        if args[k] is None:
            del args[k]
    # Set some defaults, and modify some arguments
    if not args.has_key('dialogOptionFlags'):
        args['dialogOptionFlags'] = dftflags
    if args.has_key('defaultLocation') and \
            not isinstance(args['defaultLocation'], Carbon.AE.AEDesc):
        defaultLocation = args['defaultLocation']
        if isinstance(defaultLocation, (Carbon.File.FSSpec, Carbon.File.FSRef)):
            args['defaultLocation'] = aepack.pack(defaultLocation)
        else:
            defaultLocation = Carbon.File.FSRef(defaultLocation)
            args['defaultLocation'] = aepack.pack(defaultLocation)
    if args.has_key('typeList') and not isinstance(args['typeList'], Carbon.Res.ResourceType):
        typeList = args['typeList'][:]
        # Workaround for OSX typeless files:
        if 'TEXT' in typeList and not '\0\0\0\0' in typeList:
            typeList = typeList + ('\0\0\0\0',)
        data = 'Pyth' + struct.pack("hh", 0, len(typeList))
        for type in typeList:
            data = data+type
        args['typeList'] = Carbon.Res.Handle(data)
    tpwanted = str
    if args.has_key('wanted'):
        tpwanted = args['wanted']
        del args['wanted']
    return args, tpwanted

def _dummy_Nav_eventproc(msg, data):
    pass

_default_Nav_eventproc = _dummy_Nav_eventproc

def SetDefaultEventProc(proc):
    global _default_Nav_eventproc
    rv = _default_Nav_eventproc
    if proc is None:
        proc = _dummy_Nav_eventproc
    _default_Nav_eventproc = proc
    return rv

def AskFileForOpen(
        message=None,
        typeList=None,
        # From here on the order is not documented
        version=None,
        defaultLocation=None,
        dialogOptionFlags=None,
        location=None,
        clientName=None,
        windowTitle=None,
        actionButtonLabel=None,
        cancelButtonLabel=None,
        preferenceKey=None,
        popupExtension=None,
        eventProc=_dummy_Nav_eventproc,
        previewProc=None,
        filterProc=None,
        wanted=None,
        multiple=None):
    """Display a dialog asking the user for a file to open.

    wanted is the return type wanted: FSSpec, FSRef, unicode or string (default)
    the other arguments can be looked up in Apple's Navigation Services documentation"""

    default_flags = 0x56 # Or 0xe4?
    args, tpwanted = _process_Nav_args(default_flags, version=version,
        defaultLocation=defaultLocation, dialogOptionFlags=dialogOptionFlags,
        location=location,clientName=clientName,windowTitle=windowTitle,
        actionButtonLabel=actionButtonLabel,cancelButtonLabel=cancelButtonLabel,
        message=message,preferenceKey=preferenceKey,
        popupExtension=popupExtension,eventProc=eventProc,previewProc=previewProc,
        filterProc=filterProc,typeList=typeList,wanted=wanted,multiple=multiple)
    _interact()
    try:
        rr = Nav.NavChooseFile(args)
        good = 1
    except Nav.error, arg:
        if arg[0] != -128: # userCancelledErr
            raise Nav.error, arg
        return None
    if not rr.validRecord or not rr.selection:
        return None
    if issubclass(tpwanted, Carbon.File.FSRef):
        return tpwanted(rr.selection_fsr[0])
    if issubclass(tpwanted, Carbon.File.FSSpec):
        return tpwanted(rr.selection[0])
    if issubclass(tpwanted, str):
        return tpwanted(rr.selection_fsr[0].as_pathname())
    if issubclass(tpwanted, unicode):
        return tpwanted(rr.selection_fsr[0].as_pathname(), 'utf8')
    raise TypeError, "Unknown value for argument 'wanted': %s" % repr(tpwanted)

def AskFileForSave(
        message=None,
        savedFileName=None,
        # From here on the order is not documented
        version=None,
        defaultLocation=None,
        dialogOptionFlags=None,
        location=None,
        clientName=None,
        windowTitle=None,
        actionButtonLabel=None,
        cancelButtonLabel=None,
        preferenceKey=None,
        popupExtension=None,
        eventProc=_dummy_Nav_eventproc,
        fileType=None,
        fileCreator=None,
        wanted=None,
        multiple=None):
    """Display a dialog asking the user for a filename to save to.

    wanted is the return type wanted: FSSpec, FSRef, unicode or string (default)
    the other arguments can be looked up in Apple's Navigation Services documentation"""


    default_flags = 0x07
    args, tpwanted = _process_Nav_args(default_flags, version=version,
        defaultLocation=defaultLocation, dialogOptionFlags=dialogOptionFlags,
        location=location,clientName=clientName,windowTitle=windowTitle,
        actionButtonLabel=actionButtonLabel,cancelButtonLabel=cancelButtonLabel,
        savedFileName=savedFileName,message=message,preferenceKey=preferenceKey,
        popupExtension=popupExtension,eventProc=eventProc,fileType=fileType,
        fileCreator=fileCreator,wanted=wanted,multiple=multiple)
    _interact()
    try:
        rr = Nav.NavPutFile(args)
        good = 1
    except Nav.error, arg:
        if arg[0] != -128: # userCancelledErr
            raise Nav.error, arg
        return None
    if not rr.validRecord or not rr.selection:
        return None
    if issubclass(tpwanted, Carbon.File.FSRef):
        raise TypeError, "Cannot pass wanted=FSRef to AskFileForSave"
    if issubclass(tpwanted, Carbon.File.FSSpec):
        return tpwanted(rr.selection[0])
    if issubclass(tpwanted, (str, unicode)):
        if sys.platform == 'mac':
            fullpath = rr.selection[0].as_pathname()
        else:
            # This is gross, and probably incorrect too
            vrefnum, dirid, name = rr.selection[0].as_tuple()
            pardir_fss = Carbon.File.FSSpec((vrefnum, dirid, ''))
            pardir_fsr = Carbon.File.FSRef(pardir_fss)
            pardir_path = pardir_fsr.FSRefMakePath()  # This is utf-8
            name_utf8 = unicode(name, 'macroman').encode('utf8')
            fullpath = os.path.join(pardir_path, name_utf8)
        if issubclass(tpwanted, unicode):
            return unicode(fullpath, 'utf8')
        return tpwanted(fullpath)
    raise TypeError, "Unknown value for argument 'wanted': %s" % repr(tpwanted)

def AskFolder(
        message=None,
        # From here on the order is not documented
        version=None,
        defaultLocation=None,
        dialogOptionFlags=None,
        location=None,
        clientName=None,
        windowTitle=None,
        actionButtonLabel=None,
        cancelButtonLabel=None,
        preferenceKey=None,
        popupExtension=None,
        eventProc=_dummy_Nav_eventproc,
        filterProc=None,
        wanted=None,
        multiple=None):
    """Display a dialog asking the user for select a folder.

    wanted is the return type wanted: FSSpec, FSRef, unicode or string (default)
    the other arguments can be looked up in Apple's Navigation Services documentation"""

    default_flags = 0x17
    args, tpwanted = _process_Nav_args(default_flags, version=version,
        defaultLocation=defaultLocation, dialogOptionFlags=dialogOptionFlags,
        location=location,clientName=clientName,windowTitle=windowTitle,
        actionButtonLabel=actionButtonLabel,cancelButtonLabel=cancelButtonLabel,
        message=message,preferenceKey=preferenceKey,
        popupExtension=popupExtension,eventProc=eventProc,filterProc=filterProc,
        wanted=wanted,multiple=multiple)
    _interact()
    try:
        rr = Nav.NavChooseFolder(args)
        good = 1
    except Nav.error, arg:
        if arg[0] != -128: # userCancelledErr
            raise Nav.error, arg
        return None
    if not rr.validRecord or not rr.selection:
        return None
    if issubclass(tpwanted, Carbon.File.FSRef):
        return tpwanted(rr.selection_fsr[0])
    if issubclass(tpwanted, Carbon.File.FSSpec):
        return tpwanted(rr.selection[0])
    if issubclass(tpwanted, str):
        return tpwanted(rr.selection_fsr[0].as_pathname())
    if issubclass(tpwanted, unicode):
        return tpwanted(rr.selection_fsr[0].as_pathname(), 'utf8')
    raise TypeError, "Unknown value for argument 'wanted': %s" % repr(tpwanted)


def test():
    import time

    Message("Testing EasyDialogs.")
    optionlist = (('v', 'Verbose'), ('verbose', 'Verbose as long option'),
                ('flags=', 'Valued option'), ('f:', 'Short valued option'))
    commandlist = (('start', 'Start something'), ('stop', 'Stop something'))
    argv = GetArgv(optionlist=optionlist, commandlist=commandlist, addoldfile=0)
    Message("Command line: %s"%' '.join(argv))
    for i in range(len(argv)):
        print 'arg[%d] = %r' % (i, argv[i])
    ok = AskYesNoCancel("Do you want to proceed?")
    ok = AskYesNoCancel("Do you want to identify?", yes="Identify", no="No")
    if ok > 0:
        s = AskString("Enter your first name", "Joe")
        s2 = AskPassword("Okay %s, tell us your nickname"%s, s, cancel="None")
        if not s2:
            Message("%s has no secret nickname"%s)
        else:
            Message("Hello everybody!!\nThe secret nickname of %s is %s!!!"%(s, s2))
    else:
        s = 'Anonymous'
    rv = AskFileForOpen(message="Gimme a file, %s"%s, wanted=Carbon.File.FSSpec)
    Message("rv: %s"%rv)
    rv = AskFileForSave(wanted=Carbon.File.FSRef, savedFileName="%s.txt"%s)
    Message("rv.as_pathname: %s"%rv.as_pathname())
    rv = AskFolder()
    Message("Folder name: %s"%rv)
    text = ( "Working Hard...", "Hardly Working..." ,
            "So far, so good!", "Keep on truckin'" )
    bar = ProgressBar("Progress, progress...", 0, label="Ramping up...")
    try:
        if hasattr(MacOS, 'SchedParams'):
            appsw = MacOS.SchedParams(1, 0)
        for i in xrange(20):
            bar.inc()
            time.sleep(0.05)
        bar.set(0,100)
        for i in xrange(100):
            bar.set(i)
            time.sleep(0.05)
            if i % 10 == 0:
                bar.label(text[(i/10) % 4])
        bar.label("Done.")
        time.sleep(1.0)     # give'em a chance to see "Done."
    finally:
        del bar
        if hasattr(MacOS, 'SchedParams'):
            MacOS.SchedParams(*appsw)

if __name__ == '__main__':
    try:
        test()
    except KeyboardInterrupt:
        Message("Operation Canceled.")
