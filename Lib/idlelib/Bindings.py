# This file defines the menu contents and key bindings.  Note that
# there is additional configuration information in the EditorWindow
# class (and subclasses): the menus are created there based on the
# menu_specs (class) variable, and menus not created are silently
# skipped by the code here.  This makes it possible to define the
# Debug menu here, which is only present in the PythonShell window.

# changes by dscherer@cmu.edu:
#   - Python shell moved to 'Run' menu
#   - "Help" renamed to "IDLE Help" to distinguish from Python help.
#     The distinction between the environment and the language is dim
#     or nonexistent in a novice's mind.
#   - Silly advice added

import sys
import string
from keydefs import *

menudefs = [
 # underscore prefixes character to underscore
 ('file', [
   ('_New window', '<<open-new-window>>'),
   ('_Open...', '<<open-window-from-file>>'),
   ('Open _module...', '<<open-module>>'),
   ('Class _browser', '<<open-class-browser>>'),
   ('_Path browser', '<<open-path-browser>>'),
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
 ('run',[
   ('Python shell', '<<open-python-shell>>'),
 ]),
 ('debug', [
   ('_Go to file/line', '<<goto-file-line>>'),
   ('_Stack viewer', '<<open-stack-viewer>>'),
   ('!_Debugger', '<<toggle-debugger>>'),
   ('!_Auto-open stack viewer', '<<toggle-jit-stack-viewer>>' ),
  ]),
# ('settings', [
#   ('_Configure Idle...', '<<open-config-dialog>>'),
#   None,
#   ('Revert to _Default Settings', '<<revert-all-settings>>'),
#  ]),
 ('help', [
   ('_IDLE Help...', '<<help>>'),
   ('Python _Documentation...', '<<python-docs>>'),
   ('_Advice...', '<<good-advice>>'),
   ('View IDLE _Readme...', '<<view-readme>>'),
   None,
   ('_About IDLE...', '<<about-idle>>'),
  ]),
]

if sys.platform == 'win32':
    default_keydefs = windows_keydefs
else:
    default_keydefs = unix_keydefs

del sys
