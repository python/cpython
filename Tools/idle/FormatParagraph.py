# Extension to format a paragraph

import string
import re

class FormatParagraph:

    menudefs = [
        ('edit', [
            ('Format Paragraph', '<<format-paragraph>>'),
         ])
    ]

    keydefs = {
        '<<format-paragraph>>': ['<Alt-q>'],
    }
    
    unix_keydefs = {
        '<<format-paragraph>>': ['<Meta-q>'],
    } 

    def __init__(self, editwin):
        self.editwin = editwin

    def format_paragraph_event(self, event):
        text = self.editwin.text
        try:
            first = text.index("sel.first")
            last = text.index("sel.last")
        except TclError:
            first = last = None
        if first and last:
            data = text.get(first, last)
        else:
            first, last, data = find_paragraph(text, text.index("insert"))
        newdata = reformat_paragraph(data)
        text.tag_remove("sel", "1.0", "end")
        if newdata != data:
            text.mark_set("insert", first)
            text.delete(first, last)
            text.insert(first, newdata)
        else:
            text.mark_set("insert", last)
        text.see("insert")

def find_paragraph(text, mark):
    lineno, col = map(int, string.split(mark, "."))
    line = text.get("%d.0" % lineno, "%d.0 lineend" % lineno)
    while text.compare("%d.0" % lineno, "<", "end") and is_all_white(line):
        lineno = lineno + 1
        line = text.get("%d.0" % lineno, "%d.0 lineend" % lineno)
    first_lineno = lineno
    while not is_all_white(line):
        lineno = lineno + 1
        line = text.get("%d.0" % lineno, "%d.0 lineend" % lineno)
    last = "%d.0" % lineno
    # Search back to beginning of paragraph
    lineno = first_lineno - 1
    line = text.get("%d.0" % lineno, "%d.0 lineend" % lineno)
    while lineno > 0 and not is_all_white(line):
        lineno = lineno - 1
        line = text.get("%d.0" % lineno, "%d.0 lineend" % lineno)
    first = "%d.0" % (lineno+1)
    return first, last, text.get(first, last)

def reformat_paragraph(data, limit=70):
    lines = string.split(data, "\n")
    i = 0
    n = len(lines)
    while i < n and is_all_white(lines[i]):
        i = i+1
    if i >= n:
        return data
    indent1 = get_indent(lines[i])
    if i+1 < n and not is_all_white(lines[i+1]):
        indent2 = get_indent(lines[i+1])
    else:
        indent2 = indent1
    new = lines[:i]
    partial = indent1
    while i < n and not is_all_white(lines[i]):
        # XXX Should take double space after period (etc.) into account
        words = re.split("(\s+)", lines[i])
        for j in range(0, len(words), 2):
            word = words[j]
            if not word:
                continue # Can happen when line ends in whitespace
            if len(string.expandtabs(partial + word)) > limit and \
               partial != indent1:
                new.append(string.rstrip(partial))
                partial = indent2
            partial = partial + word + " "
            if j+1 < len(words) and words[j+1] != " ":
                partial = partial + " "
        i = i+1
    new.append(string.rstrip(partial))
    # XXX Should reformat remaining paragraphs as well
    new.extend(lines[i:])
    return string.join(new, "\n")

def is_all_white(line):
    return re.match(r"^\s*$", line) is not None

def get_indent(line):
    return re.match(r"^(\s*)", line).group()
