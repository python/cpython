# The first item of each tuple is the virtual event;
# each of the remaining items is an actual key binding for the event.
# (This conveniently forms an argument list for event_add().)

win_bindings = [
    ("<<beginning-of-line>>", "<Control-a>", "<Home>"),

    ("<<expand-word>>", "<Meta-slash>", "<Alt-slash>"),

    ("<<newline-and-indent>>", "<Key-Return>", "<KP_Enter>"),
    ("<<plain-newline-and-indent>>", "<Control-j>"),

    ("<<interrupt-execution>>", "<Control-c>"),
    ("<<end-of-file>>", "<Control-d>"),

    ("<<dedent-region>>", "<Control-bracketleft>"),
    ("<<indent-region>>", "<Control-bracketright>"),

    ("<<comment-region>>", "<Meta-Key-3>", "<Alt-Key-3>"),
    ("<<uncomment-region>>", "<Meta-Key-4>", "<Alt-Key-4>"),

    ("<<history-previous>>", "<Meta-p>", "<Alt-p>"),
    ("<<history-next>>", "<Meta-n>", "<Alt-n>"),

    ("<<toggle-auto-coloring>>", "<Control-slash>"),

    ("<<close-all-windows>>", "<Control-q>"),
    ("<<open-new-window>>", "<Control-n>"),
    ("<<open-window-from-file>>", "<Control-o>"),
    ("<<save-window>>", "<Control-s>"),
    ("<<save-window-as-file>>", "<Control-w>"),
    ("<<save-copy-of-window-as-file>>", "<Meta-w>"),

    ("<<find>>", "<Control-f>"),
    ("<<find-next>>", "<F3>"),
    ("<<find-same>>", "<Control-F3>"),
    ("<<goto-line>>", "<Alt-g>", "<Meta-g>"),

    ("<<undo>>", "<Control-z>"),
    ("<<redo>>", "<Control-y>"),
    ("<<dump-undo-state>>", "<Control-backslash>"),
]

emacs_bindings = [
    ("<<beginning-of-line>>", "<Control-a>", "<Home>"),
    ("<<center-insert>>", "<Control-l>"),

    ("<<expand-word>>", "<Meta-slash>", "<Alt-slash>"),

    ("<<newline-and-indent>>", "<Key-Return>", "<KP_Enter>"),
    ("<<plain-newline-and-indent>>", "<Control-j>"),

    ("<<interrupt-execution>>", "<Control-c>"),
    ("<<end-of-file>>", "<Control-d>"),

    ("<<dedent-region>>",
     "<Meta-bracketleft>", "<Alt-bracketleft>", "<Control-bracketleft>"),
    ("<<indent-region>>",
     "<Meta-bracketright>", "<Alt-bracketright>", "<Control-bracketright>"),

    ("<<comment-region>>", "<Meta-Key-3>", "<Alt-Key-3>"),
    ("<<uncomment-region>>", "<Meta-Key-4>", "<Alt-Key-4>"),

    ("<<history-previous>>", "<Meta-p>", "<Alt-p>"),
    ("<<history-next>>", "<Meta-n>", "<Alt-n>"),

    ("<<toggle-auto-coloring>>", "<Control-slash>"),

    ("<<close-all-windows>>", "<Control-x><Control-c>"),
    ("<<close-window>>", "<Control-x><Control-0>"),
    ("<<open-new-window>>", "<Control-x><Control-n>"),
    ("<<open-window-from-file>>", "<Control-x><Control-f>"),
    ("<<save-window>>", "<Control-x><Control-s>"),
    ("<<save-window-as-file>>", "<Control-x><Control-w>"),
    ("<<save-copy-of-window-as-file>>", "<Control-x><w>"),

    ("<<find>>", "<Control-u><Control-u><Control-s>"),
    ("<<find-next>>", "<Control-u><Control-s>"),
    ("<<find-same>>", "<Control-s>"),
    ("<<goto-line>>", "<Alt-g>", "<Meta-g>"),

    ("<<undo>>", "<Control-z>"),
    ("<<redo>>", "<Alt-z>", "<Meta-z>"),
    ("<<dump-undo-state>>", "<Control-backslash>"),
]

default_bindings = emacs_bindings

def apply_bindings(text, bindings=default_bindings):
    event_add = text.event_add
    for args in bindings:
        apply(event_add, args)
