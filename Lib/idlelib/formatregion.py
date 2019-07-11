"""Format a selected region of text.

Several formatting options are available:
indent, deindent, comment, uncomment, tabify, and untabify.
"""
import re
from tkinter.simpledialog import askinteger


# Temporary copy from editor.py; importing it would cause an import cycle.
# TODO: Move the definition to a common location that both modules can import.
_line_indent_re = re.compile(r'[ \t]*')
def get_line_indent(line, tabwidth):
    """Return a line's indentation as (# chars, effective # of spaces).

    The effective # of spaces is the length after properly "expanding"
    the tabs into spaces, as done by str.expandtabs(tabwidth).
    """
    m = _line_indent_re.match(line)
    return m.end(), len(m.group().expandtabs(tabwidth))


class FormatRegion:
    "Format selected text."

    def __init__(self, editwin):
        self.editwin = editwin

    def get_region(self):
        """Return line information about the selected text region.

        If text is selected, the first and last indices will be
        for the selection.  If there is no text selected, the
        indices will be the current cursor location.

        Return a tuple containing (first index, last index,
            string representation of text, list of text lines).
        """
        text = self.editwin.text
        first, last = self.editwin.get_selection_indices()
        if first and last:
            head = text.index(first + " linestart")
            tail = text.index(last + "-1c lineend +1c")
        else:
            head = text.index("insert linestart")
            tail = text.index("insert lineend +1c")
        chars = text.get(head, tail)
        lines = chars.split("\n")
        return head, tail, chars, lines

    def set_region(self, head, tail, chars, lines):
        """Replace the text between the given indices.

        Args:
            head: Starting index of text to replace.
            tail: Ending index of text to replace.
            chars: Expected to be string of current text
                between head and tail.
            lines: List of new lines to insert between head
                and tail.
        """
        text = self.editwin.text
        newchars = "\n".join(lines)
        if newchars == chars:
            text.bell()
            return
        text.tag_remove("sel", "1.0", "end")
        text.mark_set("insert", head)
        text.undo_block_start()
        text.delete(head, tail)
        text.insert(head, newchars)
        text.undo_block_stop()
        text.tag_add("sel", head, "insert")

    def indent_region_event(self, event=None):
        "Indent region by indentwidth spaces."
        head, tail, chars, lines = self.get_region()
        for pos in range(len(lines)):
            line = lines[pos]
            if line:
                raw, effective = get_line_indent(line, self.editwin.tabwidth)
                effective = effective + self.editwin.indentwidth
                lines[pos] = self.editwin._make_blanks(effective) + line[raw:]
        self.set_region(head, tail, chars, lines)
        return "break"

    def dedent_region_event(self, event=None):
        "Dedent region by indentwidth spaces."
        head, tail, chars, lines = self.get_region()
        for pos in range(len(lines)):
            line = lines[pos]
            if line:
                raw, effective = get_line_indent(line, self.editwin.tabwidth)
                effective = max(effective - self.editwin.indentwidth, 0)
                lines[pos] = self.editwin._make_blanks(effective) + line[raw:]
        self.set_region(head, tail, chars, lines)
        return "break"

    def comment_region_event(self, event=None):
        """Comment out each line in region.

        ## is appended to the beginning of each line to comment it out.
        """
        head, tail, chars, lines = self.get_region()
        for pos in range(len(lines) - 1):
            line = lines[pos]
            lines[pos] = '##' + line
        self.set_region(head, tail, chars, lines)
        return "break"

    def uncomment_region_event(self, event=None):
        """Uncomment each line in region.

        Remove ## or # in the first positions of a line.  If the comment
        is not in the beginning position, this command will have no effect.
        """
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
        return "break"

    def tabify_region_event(self, event=None):
        "Convert leading spaces to tabs for each line in selected region."
        head, tail, chars, lines = self.get_region()
        tabwidth = self._asktabwidth()
        if tabwidth is None:
            return
        for pos in range(len(lines)):
            line = lines[pos]
            if line:
                raw, effective = get_line_indent(line, tabwidth)
                ntabs, nspaces = divmod(effective, tabwidth)
                lines[pos] = '\t' * ntabs + ' ' * nspaces + line[raw:]
        self.set_region(head, tail, chars, lines)
        return "break"

    def untabify_region_event(self, event=None):
        "Expand tabs to spaces for each line in region."
        head, tail, chars, lines = self.get_region()
        tabwidth = self._asktabwidth()
        if tabwidth is None:
            return
        for pos in range(len(lines)):
            lines[pos] = lines[pos].expandtabs(tabwidth)
        self.set_region(head, tail, chars, lines)
        return "break"

    def _asktabwidth(self):
        "Return value for tab width."
        return askinteger(
            "Tab width",
            "Columns per tab? (2-16)",
            parent=self.editwin.text,
            initialvalue=self.editwin.indentwidth,
            minvalue=2,
            maxvalue=16)


if __name__ == "__main__":
    from unittest import main
    main('idlelib.idle_test.test_formatregion', verbosity=2, exit=False)
