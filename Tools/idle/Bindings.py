# The first three items of each tupel pertain to the menu bar.  All
# three are None if the item is not to appeat in a menu.  Otherwise,
# the first item is the menu name (in lowercase), the second item is
# the menu item label; the third item is the keyboard shortcut to be
# listed in the menu, if any.  Menu items are added in the order of
# their occurrence in this table.  An item of the form
# ("menu", None, None) can be used to insert a separator in the menu.
#
# The fourth item, if present is the virtual event; each of the
# remaining items is an actual key binding for the event.  (Thus,
# items[3:] conveniently forms an argument list for event_add().)

win_bindings = [
    (None, None, None, "<<beginning-of-line>>", "<Control-a>", "<Home>"),

    (None, None, None, "<<expand-word>>", "<Meta-slash>", "<Alt-slash>"),

    (None, None, None, "<<newline-and-indent>>", "<Key-Return>", "<KP_Enter>"),
    (None, None, None, "<<plain-newline-and-indent>>", "<Control-j>"),

    (None, None, None, "<<interrupt-execution>>", "<Control-c>"),
    (None, None, None, "<<end-of-file>>", "<Control-d>"),

    (None, None, None, "<<dedent-region>>", "<Control-bracketleft>"),
    (None, None, None, "<<indent-region>>", "<Control-bracketright>"),

    (None, None, None, "<<comment-region>>", "<Meta-Key-3>", "<Alt-Key-3>"),
    (None, None, None, "<<uncomment-region>>", "<Meta-Key-4>", "<Alt-Key-4>"),

    (None, None, None, "<<history-previous>>", "<Meta-p>", "<Alt-p>"),
    (None, None, None, "<<history-next>>", "<Meta-n>", "<Alt-n>"),

    (None, None, None, "<<toggle-auto-coloring>>", "<Control-slash>"),

    (None, None, None, "<<close-all-windows>>", "<Control-q>"),
    (None, None, None, "<<open-new-window>>", "<Control-n>"),
    (None, None, None, "<<open-window-from-file>>", "<Control-o>"),
    (None, None, None, "<<save-window>>", "<Control-s>"),
    (None, None, None, "<<save-window-as-file>>", "<Control-w>"),
    (None, None, None, "<<save-copy-of-window-as-file>>", "<Meta-w>"),

    (None, None, None, "<<find>>", "<Control-f>"),
    (None, None, None, "<<find-next>>", "<F3>"),
    (None, None, None, "<<find-same>>", "<Control-F3>"),
    (None, None, None, "<<goto-line>>", "<Alt-g>", "<Meta-g>"),

    (None, None, None, "<<undo>>", "<Control-z>"),
    (None, None, None, "<<redo>>", "<Control-y>"),
    (None, None, None, "<<dump-undo-state>>", "<Control-backslash>"),
]

emacs_bindings = [

    # File menu

    ("file", "New window", "C-x C-n",
     "<<open-new-window>>", "<Control-x><Control-n>"),
    ("file", "Open...", "C-x C-f",
     "<<open-window-from-file>>", "<Control-x><Control-f>"),
    ("file", None, None),

    ("file", "Save", "C-x C-s",
     "<<save-window>>", "<Control-x><Control-s>"),
    ("file", "Save As...", "C-x C-w",
     "<<save-window-as-file>>", "<Control-x><Control-w>"),
    ("file", "Save Copy As...", "C-x w",
     "<<save-copy-of-window-as-file>>", "<Control-x><w>"),
    ("file", None, None),

    ("file", "Close", "C-x C-0",
     "<<close-window>>", "<Control-x><Control-0>"),
    ("file", "Exit", "C-x C-c",
     "<<close-all-windows>>", "<Control-x><Control-c>"),

    # Edit menu

    ("edit", "Undo", "C-z", "<<undo>>", "<Control-z>"),
    ("edit", "Redo", "Alt-z", "<<redo>>", "<Alt-z>", "<Meta-z>"),
    ("edit", None, None),

    ("edit", "Cut", None, "<<Cut>>"),
    ("edit", "Copy", None, "<<Copy>>"),
    ("edit", "Paste", None, "<<Paste>>"),
    ("edit", None, None),

    ("edit", "Find...", "C-s",
     "<<find>>", "<Control-u><Control-u><Control-s>"),
    ("edit", "Find next", "C-u C-s",
     "<<find-next>>", "<Control-u><Control-s>"),
    ("edit", "Find same", "C-s", "<<find-same>>", "<Control-s>"),
    ("edit", "Go to line", "Alt-g", "<<goto-line>>", "<Alt-g>", "<Meta-g>"),
    ("edit", None, None),

    ("edit", "Dedent region", "Ctrl-[", "<<dedent-region>>",
     "<Meta-bracketleft>", "<Alt-bracketleft>", "<Control-bracketleft>"),
    ("edit", "Indent region", "Ctrl-]", "<<indent-region>>",
     "<Meta-bracketright>", "<Alt-bracketright>", "<Control-bracketright>"),

    ("edit", "Comment out region", "Alt-3",
     "<<comment-region>>", "<Meta-Key-3>", "<Alt-Key-3>"),
    ("edit", "Uncomment region", "Alt-4",
     "<<uncomment-region>>", "<Meta-Key-4>", "<Alt-Key-4>"),
    
    # Debug menu
    
    ("debug", "Go to line from traceback", None, "<<goto-traceback-line>>"),
    ("debug", "Open stack viewer", None, "<<open-stack-viewer>>"),
    
    # Help menu
    
    ("help", "Help...", None, "<<help>>"),
    ("help", None, None),
    ("help", "About IDLE...", None, "<<about-idle>>"),

    # Not in any menu

    (None, None, None, "<<beginning-of-line>>", "<Control-a>", "<Home>"),
    (None, None, None, "<<center-insert>>", "<Control-l>"),

    (None, None, None, "<<expand-word>>", "<Meta-slash>", "<Alt-slash>"),

    (None, None, None, "<<newline-and-indent>>", "<Key-Return>", "<KP_Enter>"),
    (None, None, None, "<<plain-newline-and-indent>>", "<Control-j>"),

    (None, None, None, "<<interrupt-execution>>", "<Control-c>"),
    (None, None, None, "<<end-of-file>>", "<Control-d>"),

    (None, None, None, "<<history-previous>>", "<Meta-p>", "<Alt-p>"),
    (None, None, None, "<<history-next>>", "<Meta-n>", "<Alt-n>"),

    (None, None, None, "<<toggle-auto-coloring>>", "<Control-slash>"),

    (None, None, None, "<<dump-undo-state>>", "<Control-backslash>"),
]

default_bindings = emacs_bindings

def apply_bindings(text, bindings=default_bindings):
    event_add = text.event_add
    for args in bindings:
        args = args[3:]
        if args[1:]:
            apply(event_add, args)

def fill_menus(text, dict, bindings=default_bindings):
    # Fill the menus for the given text widget.  The dict argument is
    # a dictionary containing the menus, keyed by their lowercased name.
    # Menus that are absent or None are ignored.
    for args in bindings:
        menu, label, accelerator = args[:3]
        if not menu:
            continue
        menu = dict.get(menu)
        if not menu:
            continue
        if accelerator is None:
            accelerator = ""
        args = args[3:]
        if args:
            def command(text=text, event=args[0]):
                text.event_generate(event)
            menu.add_command(label=label, accelerator=accelerator,
                             command=command)
        elif label or accelerator:
            menu.add_command(label=label, accelerator=accelerator)
        else:
            menu.add_separator()
