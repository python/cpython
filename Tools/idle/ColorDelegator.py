import time
import string
import re
import keyword
from Tkinter import *
from Delegator import Delegator

__debug__ = 0


def any(name, list):
    return "(?P<%s>" % name + string.join(list, "|") + ")"

def make_pat():
    kw = r"\b" + any("KEYWORD", keyword.kwlist) + r"\b"
    comment = any("COMMENT", [r"#[^\n]*"])
    sqstring = r"(\b[rR])?'([^'\\\n]|\\.)*'?"
    dqstring = r'(\b[rR])?"([^"\\\n]|\\.)*"?'
    sq3string = r"(\b[rR])?'''([^'\\]|\\.|'(?!''))*(''')?"
    dq3string = r'(\b[rR])?"""([^"\\]|\\.|"(?!""))*(""")?'
    string = any("STRING", [sq3string, dq3string, sqstring, dqstring])
    return kw + "|" + comment + "|" + string + "|" + any("SYNC", [r"\n"])

prog = re.compile(make_pat(), re.S)
idprog = re.compile(r"\s+(\w+)", re.S)

class ColorDelegator(Delegator):

    def __init__(self):
        Delegator.__init__(self)
        self.prog = prog
	self.idprog = idprog

    def setdelegate(self, delegate):
	if self.delegate is not None:
            self.unbind("<<toggle-auto-coloring>>")
        Delegator.setdelegate(self, delegate)
        if delegate is not None:
            self.config_colors()
            self.bind("<<toggle-auto-coloring>>", self.toggle_colorize_event)
            self.notify_range("1.0", "end")

    def config_colors(self):
        for tag, cnf in self.tagdefs.items():
            if cnf:
                apply(self.tag_configure, (tag,), cnf)

    tagdefs = {
        "COMMENT":    {"foreground": "#dd0000"},
        "KEYWORD":    {"foreground": "#ff7700"},
        "STRING":     {"foreground": "#00aa00"},
        "DEFINITION": {"foreground": "#0000ff"},

        "SYNC":       {}, #{"background": "#ffff00"},
        "TODO":       {}, #{"background": "#cccccc"},
        }

    def insert(self, index, chars, tags=None):
        index = self.index(index)
        self.delegate.insert(index, chars, tags)
        self.notify_range(index, index + "+%dc" % len(chars))

    def delete(self, index1, index2=None):
        index1 = self.index(index1)
        self.delegate.delete(index1, index2)
        self.notify_range(index1)

    after_id = None
    allow_colorizing = 1
    colorizing = 0

    def notify_range(self, index1, index2=None):
        self.tag_add("TODO", index1, index2)
        if self.after_id:
            if __debug__: print "colorizing already scheduled"
            return
        if self.colorizing:
            self.stop_colorizing = 1
	    if __debug__: print "stop colorizing"
        if self.allow_colorizing:
	    if __debug__: print "schedule colorizing"
            self.after_id = self.after(1, self.recolorize)

    def close(self):
        if self.after_id:
            after_id = self.after_id
            self.after_id = None
            if __debug__: print "cancel scheduled recolorizer"
            self.after_cancel(after_id)
        self.allow_colorizing = 0
        self.stop_colorizing = 1

    def toggle_colorize_event(self, event):
        if self.after_id:
            after_id = self.after_id
            self.after_id = None
            if __debug__: print "cancel scheduled recolorizer"
            self.after_cancel(after_id)
	if self.allow_colorizing and self.colorizing:
	    if __debug__: print "stop colorizing"
	    self.stop_colorizing = 1
	self.allow_colorizing = not self.allow_colorizing
	if self.allow_colorizing and not self.colorizing:
	    self.after_id = self.after(1, self.recolorize)
	if __debug__:
	    print "auto colorizing turned", self.allow_colorizing and "on" or "off"
	return "break"

    def recolorize(self):
        self.after_id = None
        if not self.delegate:
            if __debug__: print "no delegate"
            return
	if not self.allow_colorizing:
	    if __debug__: print "auto colorizing is off"
	    return
	if self.colorizing:
	    if __debug__: print "already colorizing"
            return
        try:
	    self.stop_colorizing = 0
            self.colorizing = 1
            if __debug__: print "colorizing..."
            t0 = time.clock()
            self.recolorize_main()
            t1 = time.clock()
            if __debug__: print "%.3f seconds" % (t1-t0)
        finally:
            self.colorizing = 0
        if self.allow_colorizing and self.tag_nextrange("TODO", "1.0"):
	    if __debug__: print "reschedule colorizing"
            self.after_id = self.after(1, self.recolorize)

    def recolorize_main(self):
	next = "1.0"
	was_ok = is_ok = 0
	while 1:
	    item = self.tag_nextrange("TODO", next)
	    if not item:
		break
	    head, tail = item
	    self.tag_remove("SYNC", head, tail)
	    item = self.tag_prevrange("SYNC", head)
	    if item:
		head = item[1]
	    else:
		head = "1.0"

	    chars = ""
	    mark = head
	    is_ok = was_ok = 0
	    while not (was_ok and is_ok):
		next = self.index(mark + " lineend +1c")
		was_ok = "SYNC" in self.tag_names(next + "-1c")
		line = self.get(mark, next)
		##print head, "get", mark, next, "->", `line`
		if not line:
		    return
		for tag in self.tagdefs.keys():
		    self.tag_remove(tag, mark, next)
		chars = chars + line
		m = self.prog.search(chars)
		while m:
		    i, j = m.span()
		    for key, value in m.groupdict().items():
			if value:
			    a, b = m.span(key)
			    self.tag_add(key,
					 head + "+%dc" % a,
					 head + "+%dc" % b)
			    if value in ("def", "class"):
				m1 = self.idprog.match(chars, b)
				if m1:
				    a, b = m1.span(1)
				    self.tag_add("DEFINITION",
						 head + "+%dc" % a,
						 head + "+%dc" % b)
		    m = self.prog.search(chars, j)
		is_ok = "SYNC" in self.tag_names(next + "-1c")
		mark = next
		if is_ok:
		    head = mark
		    chars = ""
		self.update()
		if self.stop_colorizing:
		    if __debug__: print "colorizing stopped"
		    return


def main():
    from Percolator import Percolator
    root = Tk()
    root.wm_protocol("WM_DELETE_WINDOW", root.quit)
    text = Text(background="white")
    text.pack(expand=1, fill="both")
    text.focus_set()
    p = Percolator(text)
    d = ColorDelegator()
    p.insertfilter(d)
    root.mainloop()

if __name__ == "__main__":
    main()
