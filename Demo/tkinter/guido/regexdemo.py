"""Basic regular expression demostration facility.

This displays a window with two type-in boxes.  In the top box, you enter a 
regular expression.  In the bottom box, you enter a string.  The first 
match in the string of the regular expression is highlighted with a yellow 
background (or red if the match is empty -- then the character at the match 
is highlighted).  The highlighting is continuously updated.  At the bottom 
are a number of checkboxes which control the regular expression syntax used 
(see the regex_syntax module for descriptions).  When there's no match, or 
when the regular expression is syntactically incorrect, an error message is
displayed.

"""

from Tkinter import *
import regex
import regex_syntax

class RegexDemo:
	
	def __init__(self, master):
		self.master = master
		self.topframe = Frame(self.master)
		self.topframe.pack(fill=X)
		self.promptdisplay = Label(self.topframe, text="Enter a string:")
		self.promptdisplay.pack(side=LEFT)
		self.statusdisplay = Label(self.topframe, text="", anchor=W)
		self.statusdisplay.pack(side=LEFT, fill=X)
		self.regexdisplay = Entry(self.master)
		self.regexdisplay.pack(fill=X)
		self.regexdisplay.focus_set()
		self.labeldisplay = Label(self.master, anchor=W,
			text="Enter a string:")
		self.labeldisplay.pack(fill=X)
		self.labeldisplay.pack(fill=X)
		self.stringdisplay = Text(self.master, width=60, height=4)
		self.stringdisplay.pack(fill=BOTH, expand=1)
		self.stringdisplay.tag_configure("hit", background="yellow")
		self.addoptions()
		self.regexdisplay.bind('<Key>', self.recompile)
		self.stringdisplay.bind('<Key>', self.reevaluate)
		self.compiled = None
		self.recompile()
		btags = self.regexdisplay.bindtags()
		self.regexdisplay.bindtags(btags[1:] + btags[:1])
		btags = self.stringdisplay.bindtags()
		self.stringdisplay.bindtags(btags[1:] + btags[:1])
	
	def addoptions(self):
		self.frames = []
		self.boxes = []
		self.vars = []
		for name in (	'RE_NO_BK_PARENS',
				'RE_NO_BK_VBAR',
				'RE_BK_PLUS_QM',
				'RE_TIGHT_VBAR',
				'RE_NEWLINE_OR',
				'RE_CONTEXT_INDEP_OPS'):
			if len(self.boxes) % 3 == 0:
				frame = Frame(self.master)
				frame.pack(fill=X)
				self.frames.append(frame)
			val = getattr(regex_syntax, name)
			var = IntVar()
			box = Checkbutton(frame,
				variable=var, text=name,
				offvalue=0, onvalue=val,
				command=self.newsyntax)
			box.pack(side=LEFT)
			self.boxes.append(box)
			self.vars.append(var)
	
	def newsyntax(self):
		syntax = 0
		for var in self.vars:
			syntax = syntax | var.get()
		regex.set_syntax(syntax)
		self.recompile()
	
	def recompile(self, event=None):
		try:
			self.compiled = regex.compile(self.regexdisplay.get())
			self.statusdisplay.config(text="")
		except regex.error, msg:
			self.compiled = None
			self.statusdisplay.config(text="regex.error: %s" % str(msg))
		self.reevaluate()
	
	def reevaluate(self, event=None):
		try:
			self.stringdisplay.tag_remove("hit", "1.0", END)
		except TclError:
			pass
		if not self.compiled:
			return
		text = self.stringdisplay.get("1.0", END)
		i = self.compiled.search(text)
		if i < 0:
			self.statusdisplay.config(text="(no match)")
		else:
			self.statusdisplay.config(text="")
			regs = self.compiled.regs
			first, last = regs[0]
			if last == first:
				last = first+1
				self.stringdisplay.tag_configure("hit",
					background="red")
			else:
				self.stringdisplay.tag_configure("hit",
					background="yellow")
			pfirst = "1.0 + %d chars" % first
			plast = "1.0 + %d chars" % last
			self.stringdisplay.tag_add("hit", pfirst, plast)
			self.stringdisplay.yview_pickplace(pfirst)



# Main function, run when invoked as a stand-alone Python program.

def main():
    root = Tk()
    demo = RegexDemo(root)
    root.protocol('WM_DELETE_WINDOW', root.quit)
    root.mainloop()

if __name__ == '__main__':
    main()
