import string
from Tkinter import TclError

###$ event <<newline-and-indent>>
###$ win <Key-Return>
###$ win <KP_Enter>
###$ unix <Key-Return>
###$ unix <KP_Enter>

###$ event <<indent-region>>
###$ win <Control-bracketright>
###$ unix <Alt-bracketright>
###$ unix <Control-bracketright>

###$ event <<dedent-region>>
###$ win <Control-bracketleft>
###$ unix <Alt-bracketleft>
###$ unix <Control-bracketleft>

###$ event <<comment-region>>
###$ win <Alt-Key-3>
###$ unix <Alt-Key-3>

###$ event <<uncomment-region>>
###$ win <Alt-Key-4>
###$ unix <Alt-Key-4>

###$ event <<tabify-region>>
###$ win <Alt-Key-5>
###$ unix <Alt-Key-5>

###$ event <<untabify-region>>
###$ win <Alt-Key-6>
###$ unix <Alt-Key-6>

class AutoIndent:

    menudefs = [
        ('edit', [
            None,
            ('_Indent region', '<<indent-region>>'),
            ('_Dedent region', '<<dedent-region>>'),
            ('Comment _out region', '<<comment-region>>'),
            ('U_ncomment region', '<<uncomment-region>>'),
            ('Tabify region', '<<tabify-region>>'),
            ('Untabify region', '<<untabify-region>>'),
        ]),
    ]

    windows_keydefs = {
        '<<newline-and-indent>>': ['<Key-Return>', '<KP_Enter>'],
        '<<indent-region>>': ['<Control-bracketright>'],
        '<<dedent-region>>': ['<Control-bracketleft>'],
        '<<comment-region>>': ['<Alt-Key-3>'],
        '<<uncomment-region>>': ['<Alt-Key-4>'],
        '<<tabify-region>>': ['<Alt-Key-5>'],
        '<<untabify-region>>': ['<Alt-Key-6>'],
    }

    unix_keydefs = {
        '<<newline-and-indent>>': ['<Key-Return>', '<KP_Enter>'],
        '<<indent-region>>': ['<Alt-bracketright>',
                              '<Meta-bracketright>',
                              '<Control-bracketright>'],
        '<<dedent-region>>': ['<Alt-bracketleft>',
                              '<Meta-bracketleft>',
                              '<Control-bracketleft>'],
        '<<comment-region>>': ['<Alt-Key-3>', '<Meta-Key-3>'],
        '<<uncomment-region>>': ['<Alt-Key-4>', '<Meta-Key-4>'],
        '<<tabify-region>>': ['<Alt-Key-5>', '<Meta-Key-5>'],
        '<<untabify-region>>': ['<Alt-Key-6>', '<Meta-Key-6>'],
    }

    prefertabs = 0
    spaceindent = 4*" "

    def __init__(self, editwin):
        self.text = editwin.text

    def config(self, **options):
        for key, value in options.items():
            if key == 'prefertabs':
                self.prefertabs = value
            elif key == 'spaceindent':
                self.spaceindent = value
            else:
                raise KeyError, "bad option name: %s" % `key`

    def newline_and_indent_event(self, event):
        text = self.text
        try:
            first = text.index("sel.first")
            last = text.index("sel.last")
        except TclError:
            first = last = None
        if first and last:
            text.delete(first, last)
            text.mark_set("insert", first)
        line = text.get("insert linestart", "insert")
        i, n = 0, len(line)
        while i < n and line[i] in " \t":
            i = i+1
        indent = line[:i]
        lastchar = text.get("insert -1c")
        if lastchar == ":":
            if not indent:
                if self.prefertabs:
                    indent = "\t"
                else:
                    indent = self.spaceindent
            elif indent[-1] == "\t":
                indent = indent + "\t"
            else:
                indent = indent + self.spaceindent
        text.insert("insert", "\n" + indent)
        text.see("insert")
        return "break"

    auto_indent = newline_and_indent_event

    def indent_region_event(self, event):
        head, tail, chars, lines = self.get_region()
        for pos in range(len(lines)):
            line = lines[pos]
            if line:
                i, n = 0, len(line)
                while i < n and line[i] in " \t":
                    i = i+1
                line = line[:i] + "    " + line[i:]
                lines[pos] = line
        self.set_region(head, tail, chars, lines)
        return "break"

    def dedent_region_event(self, event):
        head, tail, chars, lines = self.get_region()
        for pos in range(len(lines)):
            line = lines[pos]
            if line:
                i, n = 0, len(line)
                while i < n and line[i] in " \t":
                    i = i+1
                indent, line = line[:i], line[i:]
                if indent:
                    if indent == "\t" or indent[-2:] == "\t\t":
                        indent = indent[:-1] + "    "
                    elif indent[-4:] == "    ":
                        indent = indent[:-4]
                    else:
                        indent = string.expandtabs(indent, 8)
                        indent = indent[:-4]
                    line = indent + line
                lines[pos] = line
        self.set_region(head, tail, chars, lines)
        return "break"

    def comment_region_event(self, event):
        head, tail, chars, lines = self.get_region()
        for pos in range(len(lines)):
            line = lines[pos]
            if not line:
                continue
            lines[pos] = '##' + line
        self.set_region(head, tail, chars, lines)

    def uncomment_region_event(self, event):
        head, tail, chars, lines = self.get_region()
        for pos in range(len(lines)):
            line = lines[pos]
            if not line:
                continue
            if line[:2] == '##':
                line = line[2:]
            elif line[:1] == '#':
                line = line[1:]
            lines[pos] = line
        self.set_region(head, tail, chars, lines)

    def tabify_region_event(self, event):
        head, tail, chars, lines = self.get_region()
        lines = map(tabify, lines)
        self.set_region(head, tail, chars, lines)

    def untabify_region_event(self, event):
        head, tail, chars, lines = self.get_region()
        lines = map(string.expandtabs, lines)
        self.set_region(head, tail, chars, lines)

    def get_region(self):
        text = self.text
        head = text.index("sel.first linestart")
        tail = text.index("sel.last -1c lineend +1c")
        if not (head and tail):
            head = text.index("insert linestart")
            tail = text.index("insert lineend +1c")
        chars = text.get(head, tail)
        lines = string.split(chars, "\n")
        return head, tail, chars, lines

    def set_region(self, head, tail, chars, lines):
        text = self.text
        newchars = string.join(lines, "\n")
        if newchars == chars:
            text.bell()
            return
        text.tag_remove("sel", "1.0", "end")
        text.mark_set("insert", head)
        text.delete(head, tail)
        text.insert(head, newchars)
        text.tag_add("sel", head, "insert")

def tabify(line, tabsize=8):
    spaces = tabsize * ' '
    for i in range(0, len(line), tabsize):
        if line[i:i+tabsize] != spaces:
            break
    else:
        i = len(line)
    return '\t' * (i/tabsize) + line[i:]
