"""Define the menu contents, hotkeys, and event bindings.

There is additional configuration information in the EditorWindow class (and
subclasses): the menus are created there based on the menu_specs (class)
variable, and menus not created are silently skipped in the code here.  This
makes it possible, for example, to define a Debug menu which is only present in
the PythonShell window, and a Format menu which is only present in the Editor
windows.

"""
import sys
from idlelib.configHandler import idleConf
from idlelib import macosxSupport

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
   ('Save Cop_y As...', '<<save-copy-of-window-as-file>>'),
   None,
   ('Prin_t Window', '<<print-window>>'),
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
 ('run', [
   ('Python Shell', '<<open-python-shell>>'),
   ]),
 ('shell', [
   ('_View Last Restart', '<<view-restart>>'),
   ('_Restart Shell', '<<restart-shell>>'),
   ]),
 ('debug', [
   ('_Go to File/Line', '<<goto-file-line>>'),
   ('!_Debugger', '<<toggle-debugger>>'),
   ('_Stack Viewer', '<<open-stack-viewer>>'),
   ('!_Auto-open Stack Viewer', '<<toggle-jit-stack-viewer>>'),
   ]),
 ('options', [
   ('_Configure IDLE...', '<<open-config-dialog>>'),
   None,
   ]),
 ('help', [
   ('_About IDLE', '<<about-idle>>'),
   None,
   ('_IDLE Help', '<<help>>'),
   ('Python _Docs', '<<python-docs>>'),
   ]),
]

if macosxSupport.runningAsOSXApp():
    # Running as a proper MacOS application bundle. This block restructures
    # the menus a little to make them conform better to the HIG.

    quitItem = menudefs[0][1][-1]
    closeItem = menudefs[0][1][-2]

    # Remove the last 3 items of the file menu: a separator, close window and
    # quit. Close window will be reinserted just above the save item, where
    # it should be according to the HIG. Quit is in the application menu.
    del menudefs[0][1][-3:]
    menudefs[0][1].insert(6, closeItem)

    # Remove the 'About' entry from the help menu, it is in the application
    # menu
    del menudefs[-1][1][0:2]

    menudefs.insert(0,
            ('application', [
                ('About IDLE', '<<about-idle>>'),
                None,
                ('_Preferences....', '<<open-config-dialog>>'),
            ]))


default_keydefs = idleConf.GetCurrentKeySet()

del sys
