# This file defines the menu contents and key bindings.  Note that
# there is additional configuration information in the EditorWindow
# class (and subclasses): the menus are created there based on the
# menu_specs (class) variable, and menus not created are silently
# skipped by the code here.  This makes it possible to define the
# Debug menu here, which is only present in the PythonShell window.

import sys
import string
import re
from keydefs import *

menudefs = [
 # underscore prefixes character to underscore
 ('file', [
   ('_New window', '<<open-new-window>>'),
   ('_Open...', '<<open-window-from-file>>'),
   ('Open _module...', '<<open-module>>'),
   ('Class _browser', '<<open-class-browser>>'),
   ('Python shell', '<<open-python-shell>>'),
   None,
   ('_Save', '<<save-window>>'),
   ('Save _As...', '<<save-window-as-file>>'),
   ('Save Co_py As...', '<<save-copy-of-window-as-file>>'),
   None,
   ('_Close', '<<close-window>>'),
   ('E_xit', '<<close-all-windows>>'),
  ]),
 ('edit', [
   ('_Undo', '<<undo>>'),
   ('_Redo', '<<redo>>'),
   None,
   ('Cu_t', '<<Cut>>'),
   ('_Copy', '<<Copy>>'),
   ('_Paste', '<<Paste>>'),
   ('Select _All', '<<select-all>>'),
  ]),
 ('script', [
   ('Run module', '<<run-module>>'),
   ('Run script', '<<run-script>>'),
   ('New shell', '<<new-shell>>'),
  ]),
 ('debug', [
   ('_Go to file/line', '<<goto-file-line>>'),
   ('_Open stack viewer', '<<open-stack-viewer>>'),
   ('_Debugger toggle', '<<toggle-debugger>>'),
  ]),
 ('help', [
   ('_Help...', '<<help>>'),
   None,
   ('_About IDLE...', '<<about-idle>>'),
  ]),
]

def prepstr(s):
    # Helper to extract the underscore from a string,
    # e.g. prepstr("Co_py") returns (2, "Copy").
    i = string.find(s, '_')
    if i >= 0:
        s = s[:i] + s[i+1:]
    return i, s

keynames = {
 'bracketleft': '[',
 'bracketright': ']',
 'slash': '/',
}

def get_accelerator(keydefs, event):
    keylist = keydefs.get(event)
    if not keylist:
        return ""
    s = keylist[0]
    s = re.sub(r"-[a-z]\b", lambda m: string.upper(m.group()), s)
    s = re.sub(r"\b\w+\b", lambda m: keynames.get(m.group(), m.group()), s)
    s = re.sub("Key-", "", s)
    s = re.sub("Control-", "Ctrl-", s)
    s = re.sub("-", "+", s)
    s = re.sub("><", " ", s)
    s = re.sub("<", "", s)
    s = re.sub(">", "", s)
    return s

if sys.platform == 'win32':
    default_keydefs = windows_keydefs
else:
    default_keydefs = unix_keydefs

def apply_bindings(text, keydefs=default_keydefs):
    text.keydefs = keydefs
    for event, keylist in keydefs.items():
        if keylist:
            apply(text.event_add, (event,) + tuple(keylist))

def fill_menus(text, menudict, defs=menudefs, keydefs=default_keydefs):
    # Fill the menus for the given text widget.  The menudict argument is
    # a dictionary containing the menus, keyed by their lowercased name.
    # Menus that are absent or None are ignored.
    for mname, itemlist in defs:
        menu = menudict.get(mname)
        if not menu:
            continue
        for item in itemlist:
            if not item:
                menu.add_separator()
            else:
                label, event = item
                underline, label = prepstr(label)
                accelerator = get_accelerator(keydefs, event)
                def command(text=text, event=event):
                    text.event_generate(event)
                menu.add_command(label=label, underline=underline,
                                 command=command, accelerator=accelerator)
