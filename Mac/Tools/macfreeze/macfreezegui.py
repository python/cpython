"""macfreezegui - The GUI for macfreeze"""
from Carbon import Dlg
import macfs
import EasyDialogs
import sys
import os
import string
from Carbon import Res
import macresource

ID_MAINDIALOG=512

ITEM_SCRIPTNAME=2
ITEM_SCRIPTBROWSE=3
ITEM_GENSOURCE=4
ITEM_GENSOURCE_ITEMS=(7,)
ITEM_SOURCEDIRNAME=6
ITEM_SOURCEDIRBROWSE=7
ITEM_GENRESOURCE=8
ITEM_GENRESOURCE_ITEMS=(11,)
ITEM_RESOURCENAME=10
ITEM_RESOURCEBROWSE=11
ITEM_GENAPPLET=12
ITEM_GENAPPLET_ITEMS=(15,)
ITEM_APPLETNAME=14
ITEM_APPLETBROWSE=15
ITEM_OK=16
ITEM_CANCEL=17
ITEM_DEBUG=19
ITEM_GENINFO=20

RADIO_GROUPING={
        ITEM_GENSOURCE: ITEM_GENSOURCE_ITEMS,
        ITEM_GENRESOURCE: ITEM_GENRESOURCE_ITEMS,
        ITEM_GENAPPLET: ITEM_GENAPPLET_ITEMS,
        ITEM_GENINFO: ()
}

def dialog(script=None):

    # Invent the various names
    if not script:
        fss, ok = macfs.PromptGetFile("Script?", "TEXT")
        if not ok:
            sys.exit(0)
        script = fss.as_pathname()
    basename, ext = os.path.splitext(script)
    if ext:
        appletname = basename
        rsrcname = basename + 'modules.rsrc'
    else:
        appletname = script + '.applet'
        rsrcname = script + 'modules.rsrc'
    dirname, basebase = os.path.split(basename)
    dirname = os.path.join(dirname, 'build.'+basebase)

    # Get the dialog, possibly opening the resource file (if needed)
    macresource.need('DLOG', ID_MAINDIALOG, 'macfreeze.rsrc')
    d = Dlg.GetNewDialog(ID_MAINDIALOG, -1)
    if d == None:
        EasyDialogs.Message("Dialog resource not found or faulty")
        sys.exit(1)

    # Fill the dialog
    d.SetDialogDefaultItem(ITEM_OK)
    d.SetDialogCancelItem(ITEM_CANCEL)

    _dialogsetfile(d, ITEM_SCRIPTNAME, script)
    _dialogsetfile(d, ITEM_SOURCEDIRNAME, dirname)
    _dialogsetfile(d, ITEM_RESOURCENAME, rsrcname)
    _dialogsetfile(d, ITEM_APPLETNAME, appletname)

    gentype = ITEM_GENSOURCE
    _dialogradiogroup(d, ITEM_GENSOURCE)

    # Interact
    d.GetDialogWindow().SetWTitle("Standalone application creation options")
    d.GetDialogWindow().ShowWindow()
    d.DrawDialog()
    while 1:
        item = Dlg.ModalDialog(None)
        if item == ITEM_OK:
            break
        elif item == ITEM_CANCEL:
            sys.exit(0)
        elif item in RADIO_GROUPING.keys():
            gentype = item
            _dialogradiogroup(d, item)
        elif item == ITEM_SCRIPTBROWSE:
            fss, ok = macfs.PromptGetFile("Script?")
            if ok:
                script = fss.as_pathname()
                _dialogsetfile(d, ITEM_SCRIPTNAME, script)
        elif item == ITEM_SOURCEDIRBROWSE:
            fss, ok = macfs.StandardPutFile("Output folder name", os.path.split(dirname)[1])
            if ok:
                dirname = fss.as_pathname()
                _dialogsetfile(d, ITEM_SOURCEDIRNAME, dirname)
        elif item == ITEM_RESOURCEBROWSE:
            fss, ok = macfs.StandardPutFile("Resource output file", os.path.split(rsrcname)[1])
            if ok:
                rsrcname = fss.as_pathname()
                _dialogsetfile(d, ITEM_RESOURCENAME, rsrcname)
        elif item == ITEM_APPLETBROWSE:
            fss, ok = macfs.StandardPutFile("Applet output file", os.path.split(appletname)[1])
            if ok:
                appletname = fss.as_pathname()
                _dialogsetfile(d, ITEM_APPLETNAME, appletname)
        else:
            pass
    tp, h, rect = d.GetDialogItem(ITEM_DEBUG)
    debug = Dlg.GetDialogItemText(h)
    try:
        debug = string.atoi(string.strip(debug))
    except ValueError:
        EasyDialogs.Message("Illegal debug value %r, set to zero."%(debug,))
        debug = 0
    if gentype == ITEM_GENSOURCE:
        return 'source', script, dirname, debug
    elif gentype == ITEM_GENRESOURCE:
        return 'resource', script, rsrcname, debug
    elif gentype == ITEM_GENAPPLET:
        return 'applet', script, appletname, debug
    elif gentype == ITEM_GENINFO:
        return 'info', script, '', debug
    raise 'Error in gentype', gentype

def _dialogradiogroup(d, item):
    for k in RADIO_GROUPING.keys():
        subitems = RADIO_GROUPING[k]
        tp, h, rect = d.GetDialogItem(k)
        if k == item:
            h.as_Control().SetControlValue(1)
            for i2 in subitems:
                d.ShowDialogItem(i2)
        else:
            h.as_Control().SetControlValue(0)
            for i2 in subitems:
                d.HideDialogItem(i2)

def _dialogsetfile(d, item, file):
    if len(file) > 32:
        file = '\311:' + os.path.split(file)[1]
    tp, h, rect = d.GetDialogItem(item)
    Dlg.SetDialogItemText(h, file)

if __name__ == '__main__':
    type, script, file, debug = dialog()
    print type, script, file, 'debug=%d'%debug
    sys.exit(1)
