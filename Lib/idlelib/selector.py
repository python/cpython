"""Smart SelectThis module provides smart selection functionality for Tkinter text widgets.By analyzing the character at the cursor position and its context, itautomatically selects text units such as words, lines, code blocks, bracketpairs, strings, and more.Functions:    DisableTkDouble1Binding: Disables the default double-click binding of Tkinter text widgets to prevent conflicts with custom selection logic.    TextSelector: Text selection operations, providing methods to select characters, lines, and ranges.    SelectBlock: Selects code blocks based on indentation, supporting multi-line selection.    SmartSelect: An event handler function that triggers smart selection based on the current character (e.g., letters, spaces, quotes, brackets).Usage:    Bind SmartSelect to an event of the text widget (such as <Double-Button-1>)    to enable automatic selection. Suitable for Tkinter applications that    require enhanced text selection experience."""import re

sp = lambda c: eval(c.replace('.',','))
jn = lambda x,y: '%i.%i'%(x,y)
lc = lambda s: jn(s.count('\n')+1,len(s)-s.rfind('\n')-1)

is_code = lambda s: s.split('#')[0].strip()
is_blank = lambda s: not s.strip()
get_indent = lambda s: re.match(r' *', s.rstrip()).end()


def DisableTkDouble1Binding(root):
    tk = root.tk
    tk.eval('bind Text <Double-1> {catch {%W mark set insert sel.first}}')  # See at: "~\tcl\tk8.6\text.tcl"


class TextSelector:
    def __init__(self, text):
        self.text = text

    def select(self, idx1, idx2):
        self.text.tag_remove('sel', '1.0', 'end')
        self.text.tag_add('sel', idx1, idx2)
        self.text.mark_set('insert', idx1)

    def select_chars(self, n=1, n2=0):
        n, n2 = sorted([n, n2])
        self.select('current%+dc' % n, 'current%+dc' % n2)

    def select_from_line(self, n1, n2):
        n2 = 'start%+dc' % n2 if isinstance(n2, int) else n2
        self.select('current linestart+%dc' % n1, 'current line' + n2)

    def select_lines(self, n=1, n2=0):
        n, n2 = sorted([n, n2])
        self.select('current linestart%+dl' % n, 'current linestart%+dl' % n2)


def FindParent(s, c1='(', c2=')'):
    n1 = n2 = 0
    for n, c in enumerate(s):
        n1 += c in c1
        n2 += c in c2
        if n1 and n1 == n2:
            return n


def MatchSpan(r, s, n):
    for m in re.finditer(r, s):
        if m.start() <= n <= m.end():
            return m.span()


def SelectBlock(text, first_line=True):
    patt = re.compile(r'([ \t]*)(.*?)([ \t]*)(#.*)?')

    idx1 = 'current linestart' if first_line else 'current linestart-1l'
    lines = text.get(idx1, 'end').split('\n')
    base_indent = patt.fullmatch(lines[0]).group(1)

    ln = -1
    for i, line in enumerate(lines[1:]):
        indent, code, space, comment = patt.fullmatch(line).groups()
        if code or comment:
            if indent > base_indent:
                ln = i
            else:
                break

    ln = max(1, ln + 2 if first_line else ln + 1)  # at least select one line
    text_sel = TextSelector(text)
    text_sel.select_lines(ln)


def SmartSelect(event):
    text = event.widget
    text_sel = TextSelector(text)
    text.tag_remove('hit', '1.0', 'end')

    if 'STRING' in text.tag_names('current'):
        st, ed = text.tag_prevrange('STRING', 'current+1c')  # See at: `idlelib.squeezer.Squeezer.squeeze_current_text_event`
        ed2 = text.index(ed + '-1c')
        cur = text.index('current')
        if cur in (st, ed2):
            return text_sel.select(st, ed)

    cur = text.index('current')  # should not be 'insert', it will cause the cursor position at the beginning of the automatic selection area
    col = sp(cur)[1]
    line = text.get('current linestart', 'current lineend')

    c = text.get('current')  # charset: !"#$%&'()*+,-./:;<=>?@[\]^_`{|}~
    if c == ':':
        if is_code(line[col+1:]):
            text_sel.select_chars()
        else:
            SelectBlock(text)

    elif c == ' ' or c == '\n' and col == 0:
        indent = get_indent(line)
        if col <= indent:

            # | prev line   | current line | action           |
            # | ----------- | ------------ | ---------------- |
            # | blank       | blank        | select line      |
            # | blank       | indent       | select block     |
            # | indent      | blank        | select sub block |
            # | indent      | same indent  | select block     |
            # | less indent | more indent  | select sub block |
            # | more indent | less indent  | select block     |

            prev_line = text.get('current-1l linestart', 'current-1l lineend')
            if not is_blank(prev_line) and (get_indent(prev_line) < indent or is_blank(line)):
                SelectBlock(text, False)
            elif not is_blank(line):
                SelectBlock(text)
            else:
                text_sel.select_lines()
        else:
            p1, p2 = MatchSpan(r' +', line, col)
            text_sel.select_from_line(p1, p2)

    elif re.match(r'\w', c):
        p1, p2 = MatchSpan(r'\w+', line, col)
        text_sel.select_from_line(p1, p2)

        # sometimes will cause performance issues, so return it.
        return

        word = line[p1:p2]
        s = text.get('1.0', 'end')
        for m in re.finditer(r'\b%s\b' % word, s):
            p1, p2 = m.span()
            text.tag_add('hit', lc(s[:p1]), lc(s[:p2]))  # should not be '1.0+nc', it will cause offseting when exist Squeezer

    elif c == '\n':
        text_sel.select_lines()

    elif c == '#':
        if 'COMMENT' in text.tag_names('current') and \
           text.index('current') == text.tag_prevrange('COMMENT', 'current+1c')[0]:
            p1, p2 = MatchSpan(r' *#', line, col)
            if p1 > 0:
                text_sel.select_from_line(p1, 'end')
            else:
                text_sel.select_lines()
        else:
            text_sel.select_chars()

    elif c in '\'"`':  # quote in comment or string
        s = text.get('current+1c', 'end')
        n = s.find(c)
        text_sel.select_chars(n + 2)

    elif c in '([{':
        c1 = c
        c2 = ')]}'['([{'.index(c1)]
        s = text.get('current', 'end')
        n = FindParent(s, c1, c2)
        text_sel.select_chars(n + 1)

    elif c in ')]}':
        c1 = c
        c2 = '([{'[')]}'.index(c1)]
        s = text.get('1.0', 'current+1c')
        n = FindParent(reversed(s), c1, c2)
        text_sel.select_chars(-n, 1)

    elif c == '\\':  # e.g. \n \xhh \uxxxx \Uxxxxxxxx
        c2 = text.get('current+1c')
        n = {'x': 4, 'u': 6, 'U': 10}.get(c2, 2)
        text_sel.select_chars(n)

    elif c == '.':
        s = text.get('current+1c', 'current lineend')
        n = re.match(r'\s*\w*', s).end()
        text_sel.select_chars(n + 1)

    elif c == ',':  # e.g. foo(a, b(c, d=e), f)
        s = text.get('current+1c', 'end')
        n1 = n2 = 0
        for n, c1 in enumerate(s):
            if n1 == n2 and c1 in ',)]}':
                break
            n1 += c1 in '([{'
            n2 += c1 in ')]}'
        text_sel.select_chars(n + 1)

    else:  # charset: !$%&*+,-/;<=>?@^|~
        p1, p2 = MatchSpan(r'[^\w()[\]{}\'" ]+', line, col)  # do not concatenate to brackets, spaces, or quotes
        text_sel.select_from_line(p1, p2)
