import string

class AutoIndent:

    def __init__(self, text, prefertabs=0, spaceindent=4*" "):
        self.text = text
        self.prefertabs = prefertabs
        self.spaceindent = spaceindent
        text.bind("<<newline-and-indent>>", self.autoindent)
        text.bind("<<indent-region>>", self.indentregion)
        text.bind("<<dedent-region>>", self.dedentregion)
        text.bind("<<comment-region>>", self.commentregion)
        text.bind("<<uncomment-region>>", self.uncommentregion)

    def config(self, **options):
        for key, value in options.items():
            if key == 'prefertabs':
                self.prefertabs = value
            elif key == 'spaceindent':
                self.spaceindent = value
            else:
                raise KeyError, "bad option name: %s" % `key`

    def autoindent(self, event):
        text = self.text
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

    def indentregion(self, event):
        head, tail, chars, lines = self.getregion()
        for pos in range(len(lines)):
            line = lines[pos]
            if line:
                i, n = 0, len(line)
                while i < n and line[i] in " \t":
                    i = i+1
                line = line[:i] + "    " + line[i:]
                lines[pos] = line
        self.setregion(head, tail, chars, lines)
        return "break"

    def dedentregion(self, event):
        head, tail, chars, lines = self.getregion()
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
        self.setregion(head, tail, chars, lines)
        return "break"

    def commentregion(self, event):
        head, tail, chars, lines = self.getregion()
        for pos in range(len(lines)):
            line = lines[pos]
            if not line:
                continue
            lines[pos] = '##' + line
        self.setregion(head, tail, chars, lines)

    def uncommentregion(self, event):
        head, tail, chars, lines = self.getregion()
        for pos in range(len(lines)):
            line = lines[pos]
            if not line:
                continue
            if line[:2] == '##':
                line = line[2:]
            elif line[:1] == '#':
                line = line[1:]
            lines[pos] = line
        self.setregion(head, tail, chars, lines)

    def getregion(self):
        text = self.text
        head = text.index("sel.first linestart")
        tail = text.index("sel.last -1c lineend +1c")
        if not (head and tail):
            head = text.index("insert linestart")
            tail = text.index("insert lineend +1c")
        chars = text.get(head, tail)
        lines = string.split(chars, "\n")
        return head, tail, chars, lines

    def setregion(self, head, tail, chars, lines):
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
