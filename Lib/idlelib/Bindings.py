# This file defines the menu contents and key bindings.  Note that
# there is additional configuration information in the EditorWindow
# class (and subclasses): the menus are created there based on the
# menu_specs (class) variable, and menus not created are silently
# skipped by the code here.  This makes it possible to define the
# Debug menu here, which is only present in the PythonShell window.

import sys
from configHandler import idleConf

menudefs = [
 # underscore prefixes character to underscore
 ('file', [
   ('_New Window', '<<open-new-window>>'),
   ('_Open...', '<<open-window-from-file>>'),
   ('Open _Module...', '<<open-module>>'),
   ('Class _Browser', '<<open-class-browser>>'),
   ('_Path Browser', '<<open-path-browser>>'),
   None,
   ('_Save', '<<save-window>>'),
   ('Save _As...', '<<save-window-as-file>>'),
   ('Save Co_py As...', '<<save-copy-of-window-as-file>>'),
   None,
   ('_Print Window', '<<print-window>>'),
   None,
   ('_Close', '<<close-window>>'),
   ('E_xit', '<<close-all-windows>>'),
  ]),
 ('edit', [
   ('_Undo', '<<undo>>'),
   ('_Redo', '<<redo>>'),
   None,
   ('Cu_t', '<<cut>>'),
   ('_Copy', '<<copy>>'),
   ('_Paste', '<<paste>>'),
   ('Select _All', '<<select-all>>'),
   None,
   ('_Find...', '<<find>>'),
   ('Find A_gain', '<<find-again>>'),
   ('Find _Selection', '<<find-selection>>'),
   ('Find in Files...', '<<find-in-files>>'),
   ('R_eplace...', '<<replace>>'),
   ('Go to _Line', '<<goto-line>>'),
  ]),
('format', [
    ('_Indent Region', '<<indent-region>>'),
    ('_Dedent Region', '<<dedent-region>>'),
    ('Comment _Out Region', '<<comment-region>>'),
    ('U_ncomment Region', '<<uncomment-region>>'),
    ('Tabify Region', '<<tabify-region>>'),
    ('Untabify Region', '<<untabify-region>>'),
    ('Toggle Tabs', '<<toggle-tabs>>'),
    ('New Indent Width', '<<change-indentwidth>>'),
]),
 ('run',[
   ('Python Shell', '<<open-python-shell>>'),
 ]),
 ('shell', [
   ('_View Last Restart', '<<view-restart>>'),
   ('_Restart Shell', '<<restart-shell>>'),
   None,
   ('_Go to File/Line', '<<goto-file-line>>'),
   ('!_Debugger', '<<toggle-debugger>>'),
   ('_Stack Viewer', '<<open-stack-viewer>>'),
   ('!_Auto-open Stack Viewer', '<<toggle-jit-stack-viewer>>' ),
  ]),
 ('options', [
   ('_Configure Idle...', '<<open-config-dialog>>'),
   None,
   ('Revert to _Default Settings', '<<revert-all-settings>>'),
  ]),
 ('help', [
   ('_IDLE Help...', '<<help>>'),
   ('Python _Documentation...', '<<python-docs>>'),
   ('View IDLE _Readme...', '<<view-readme>>'),
   None,
   ('_About IDLE...', '<<about-idle>>'),
  ]),
]

default_keydefs = idleConf.GetCurrentKeySet()

del sys
