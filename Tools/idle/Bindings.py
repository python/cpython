# This file defines the menu contents and key bindings.  Note that
# there is additional configuration information in the EditorWindow
# class (and subclasses): the menus are created there based on the
# menu_specs (class) variable, and menus not created are silently
# skipped by the code here.  This makes it possible to define the
# Debug menu here, which is only present in the PythonShell window.

import sys
import string
import re

menudefs = [
 # underscore prefixes character to underscore
 ('file', [
   ('_New window', '<<open-new-window>>'),
   ('_Open...', '<<open-window-from-file>>'),
   ('Open _module...', '<<open-module>>'),
   ('Class _browser...', '<<open-class-browser>>'),
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
   None,
   ('_Find...', '<<find>>'),
   ('Find _next', '<<find-next>>'),
   ('Find _same', '<<find-same>>'),
   ('_Go to line', '<<goto-line>>'),
   None,
   ('_Dedent region', '<<dedent-region>>'),
   ('_Indent region', '<<indent-region>>'),
   ('Comment _out region', '<<comment-region>>'),
   ('U_ncomment region', '<<uncomment-region>>'),
  ]),
 ('debug', [
   ('_Go to line from traceback', '<<goto-traceback-line>>'),
   ('_Open stack viewer', '<<open-stack-viewer>>'),
   ('_Debugger toggle', '<<toggle-debugger>>'),
  ]),
 ('help', [
   ('_Help...', '<<help>>'),
   None,
   ('_About IDLE...', '<<about-idle>>'),
  ]),
]

windows_keydefs = {
 '<<beginning-of-line>>': ['<Control-a>', '<Home>'],
 '<<close-all-windows>>': ['<Control-q>'],
 '<<comment-region>>': ['<Meta-Key-3>', '<Alt-Key-3>'],
 '<<dedent-region>>': ['<Control-bracketleft>'],
 '<<dump-undo-state>>': ['<Control-backslash>'],
 '<<end-of-file>>': ['<Control-d>'],
 '<<expand-word>>': ['<Meta-slash>', '<Alt-slash>'],
 '<<find-next>>': ['<F3>', '<Control-g>'],
 '<<find-same>>': ['<Control-F3>'],
 '<<find>>': ['<Control-f>'],
 '<<goto-line>>': ['<Alt-g>', '<Meta-g>'],
 '<<history-next>>': ['<Meta-n>', '<Alt-n>'],
 '<<history-previous>>': ['<Meta-p>', '<Alt-p>'],
 '<<indent-region>>': ['<Control-bracketright>'],
 '<<interrupt-execution>>': ['<Control-c>'],
 '<<newline-and-indent>>': ['<Key-Return>', '<KP_Enter>'],
 '<<open-new-window>>': ['<Control-n>'],
 '<<open-window-from-file>>': ['<Control-o>'],
 '<<plain-newline-and-indent>>': ['<Control-j>'],
 '<<redo>>': ['<Control-y>'],
 '<<save-copy-of-window-as-file>>': ['<Meta-w>'],
 '<<save-window-as-file>>': ['<Control-w>'],
 '<<save-window>>': ['<Control-s>'],
 '<<toggle-auto-coloring>>': ['<Control-slash>'],
 '<<uncomment-region>>': ['<Meta-Key-4>', '<Alt-Key-4>'],
 '<<undo>>': ['<Control-z>'],
}

emacs_keydefs = {
 '<<Copy>>': ['<Alt-w>'],
 '<<Cut>>': ['<Control-w>'],
 '<<Paste>>': ['<Control-y>'],
 '<<about-idle>>': [],
 '<<beginning-of-line>>': ['<Control-a>', '<Home>'],
 '<<center-insert>>': ['<Control-l>'],
 '<<close-all-windows>>': ['<Control-x><Control-c>'],
 '<<close-window>>': ['<Control-x><Control-0>'],
 '<<comment-region>>': ['<Meta-Key-3>', '<Alt-Key-3>'],
 '<<dedent-region>>': ['<Meta-bracketleft>',
                       '<Alt-bracketleft>',
                       '<Control-bracketleft>'],
 '<<do-nothing>>': ['<Control-x>'],
 '<<dump-undo-state>>': ['<Control-backslash>'],
 '<<end-of-file>>': ['<Control-d>'],
 '<<expand-word>>': ['<Meta-slash>', '<Alt-slash>'],
 '<<find-next>>': ['<Control-u><Control-s>'],
 '<<find-same>>': ['<Control-s>'],
 '<<find>>': ['<Control-u><Control-u><Control-s>'],
 '<<goto-line>>': ['<Alt-g>', '<Meta-g>'],
 '<<goto-traceback-line>>': [],
 '<<help>>': [],
 '<<history-next>>': ['<Meta-n>', '<Alt-n>'],
 '<<history-previous>>': ['<Meta-p>', '<Alt-p>'],
 '<<indent-region>>': ['<Meta-bracketright>',
                       '<Alt-bracketright>',
                       '<Control-bracketright>'],
 '<<interrupt-execution>>': ['<Control-c>'],
 '<<newline-and-indent>>': ['<Key-Return>', '<KP_Enter>'],
 '<<open-class-browser>>': ['<Control-x><Control-b>'],
 '<<open-module>>': ['<Control-x><Control-m>'],
 '<<open-new-window>>': ['<Control-x><Control-n>'],
 '<<open-stack-viewer>>': [],
 '<<open-window-from-file>>': ['<Control-x><Control-f>'],
 '<<plain-newline-and-indent>>': ['<Control-j>'],
 '<<redo>>': ['<Alt-z>', '<Meta-z>'],
 '<<save-copy-of-window-as-file>>': ['<Control-x><w>'],
 '<<save-window-as-file>>': ['<Control-x><Control-w>'],
 '<<save-window>>': ['<Control-x><Control-s>'],
 '<<toggle-auto-coloring>>': ['<Control-slash>'],
 '<<toggle-debugger>>': [],
 '<<uncomment-region>>': ['<Meta-Key-4>', '<Alt-Key-4>'],
 '<<undo>>': ['<Control-z>'],
}

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
}

def getaccelerator(keydefs, event):
    keylist = keydefs.get(event)
    if not keylist:
        return ""
    s = keylist[0]
    if s[:6] == "<Meta-":
        # Prefer Alt over Meta -- they should be the same thing anyway
        alts = "<Alt-" + s[6:]
        if alts in keylist:
            s = alts
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
    default_keydefs = emacs_keydefs

def apply_bindings(text, keydefs=default_keydefs):
    text.keydefs = keydefs
    for event, keylist in keydefs.items():
        if keylist:
            apply(text.event_add, (event,) + tuple(keylist))

def fill_menus(text, menudict, defs=menudefs):
    # Fill the menus for the given text widget.  The menudict argument is
    # a dictionary containing the menus, keyed by their lowercased name.
    # Menus that are absent or None are ignored.
    if hasattr(text, "keydefs"):
        keydefs = text.keydefs
    else:
        keydefs = default_keydefs
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
                accelerator = getaccelerator(keydefs, event)
                def command(text=text, event=event):
                    text.event_generate(event)
                menu.add_command(label=label, underline=underline,
                                 command=command, accelerator=accelerator)
