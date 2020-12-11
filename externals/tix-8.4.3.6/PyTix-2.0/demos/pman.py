#! /usr/local/bin/python
#
#
# $Id: pman.py,v 1.1 2000/11/05 19:52:02 idiscovery Exp $
#
# An xman like program.  - Sudhir Shenoy, January 1996.
#
# Features:
#
# Can have multiple man pages open at the same time.
#
# Hypertext: Manual page cross references in the Apropos output or a man page
# are highlighted when the mouse moves on top of them. Clicking button 1 over
# the highlighted reference displays the relevant page.
#
# Regexp search in manual page window with wrap around.
#
# Handles MANPATH correctly. If the same man page (e.g. 'make') is in more
# than one directory (/usr/man/man1 and /usr/local/man/man1), precedence is
# decided by which dir appears first in the MANPATH.
#
# BUGS: Doesn't handle the case when the reference is split across two lines.
# This can be fixed by sucking in the whole text from the text widget and then
# doing the search e.g., in class ManWindow but this involves more work.
#
# Page display is slow.
#

import os, regex, regsub, string, sys, Tix

BOLDFONT = '*-Courier-Bold-R-Normal-*-140-*'
ITALICFONT = '*-Courier-Medium-O-Normal-*-140-*'

footer_pat = regex.compile('^     Page [1-9][0-9]*[ \t]+\|^.*Last change:.*[1-9][0-9]*\n')
empty_pat = regex.compile('^[ \t]*\n')
underline_pat = regex.compile('^[ \t]*[Xv!_][Xv!_ \t]*\n')
link_pat = regex.compile('\([A-Za-z0-9._]+\)[ \t]*([ \t]*\([A-Za-z0-9]+\)[ \t]*)')

# Man Page display widget - borrowed from Guido's demos with minor changes.
class ManPageWidget(Tix.ScrolledText):
    def __init__(self, master=None, cnf={}):
	# Initialize base class
	Tix.ScrolledText.__init__(self, master, cnf)
	self.text['state'] = 'disabled'

	# Define tags for formatting styles
	self.text.tag_config('X', {'underline': 1})
	self.text.tag_config('!', {'font': BOLDFONT})
	self.text.tag_config('_', {'font': ITALICFONT})

	# Set state to idle
	self.fp = None
	self.lineno = 0
	self.tagnum = 0

    # Test whether we are busy parsing a file
    def busy(self):
	return self.fp != None

    # Ensure we're not busy
    def kill(self):
	if self.busy():
	    self._endparser()

    # Parse a file, in the background
    def asyncparsefile(self, fp):
	self._startparser(fp)
	self.tk.createfilehandler(fp, Tix.READABLE, self._filehandler)

    parsefile = asyncparsefile	# Alias

    # I/O handler used by background parsing
    def _filehandler(self, fp, mask):
	nextline = self.fp.readline()
	if not nextline:
	    self._endparser()
	    return
	self._parseline(nextline)

    # Parse a file, now (cannot be aborted)
    def syncparsefile(self, fp):
	from select import select
	def avail(fp=fp, tout=0.0, select=select):
	    return select([fp], [], [], tout)[0]
	height = self.getint(self['height'])
	self._startparser(fp)
	while 1:
	    nextline = fp.readline()
	    if not nextline:
		break
	    self._parseline(nextline)
	self._endparser()

    # Initialize parsing from a particular file -- must not be busy
    def _startparser(self, fp):
	if self.busy():
	    raise RuntimeError, 'startparser: still busy'
	fp.fileno()		# Test for file-ness
	self.fp = fp
	self.lineno = 0
	self.tagnum = 0
	self.ok = 0
	self.empty = 0
	self.buffer = None
	self.text['state'] = 'normal'
	self.text.delete('1.0', 'end')
	self.text['state'] = 'disabled'

    # End parsing -- must be busy, need not be at EOF
    def _endparser(self):
	if not self.busy():
	    raise RuntimeError, 'endparser: not busy'
	if self.buffer:
	    self._parseline('')
	try:
	    self.tk.deletefilehandler(self.fp)
	except Tix.TclError, msg:
	    pass
	self.fp.close()
	self.fp = None
	del self.ok, self.empty, self.buffer

    # Parse a single line
    def _parseline(self, nextline):
	if not self.buffer:
	    # Save this line -- we need one line read-ahead
	    self.buffer = nextline
	    return
	if empty_pat.match(self.buffer) >= 0:
	    # Buffered line was empty -- set a flag
	    self.empty = 1
	    self.buffer = nextline
	    return
	textline = self.buffer
	if underline_pat.match(nextline) >= 0:
	    # Next line is properties for buffered line
	    propline = nextline
	    self.buffer = None
	else:
	    # Next line is read-ahead
	    propline = None
	    self.buffer = nextline
	if not self.ok:
	    # First non blank line after footer must be header
	    # -- skip that too
	    self.ok = 1
	    self.empty = 0
	    return
	if footer_pat.match(textline) >= 0:
	    # Footer -- start skipping until next non-blank line
	    self.ok = 0
	    self.empty = 0
	    return
	self.text['state'] = 'normal'
	if Tix.TkVersion >= 4.0:
	    self.text.mark_set('insert', 'end-1c')
	else:
	    self.text.mark_set('insert', 'end')
	if self.empty:
	    # One or more previous lines were empty
	    # -- insert one blank line in the text
	    self._insert_prop('\n')
	    self.lineno = self.lineno + 1
	    self.empty = 0
	if not propline:
	    # No properties
	    self._insert_prop(textline)
	else:
	    # Search for properties
	    p = ''
	    j = 0
	    for i in range(min(len(propline), len(textline))):
		if propline[i] != p:
		    if j < i:
			self._insert_prop(textline[j:i], p)
			j = i
		    p = propline[i]
	    self._insert_prop(textline[j:])
	startpos = 0
	line = textline[:]
	while 1:
	    pos = link_pat.search(line)
	    if pos < 0:
		break
	    pos = pos + startpos
	    startpos = startpos + link_pat.regs[0][1]
	    tag = self._w + `self.tagnum`
	    self.tagnum = self.tagnum + 1
	    self.text.tag_add(tag, '%d.%d' % (self.lineno + 1, pos),
			      '%d.%d' % (self.lineno + 1, startpos))
	    self.text.tag_bind(tag, '<Any-Enter>',
			       lambda e=None,t=tag,w=self: w._highlight(t, 1))
	    self.text.tag_bind(tag, '<Any-Leave>',
			       lambda e=None,t=tag,w=self: w._highlight(t, 0))
	    self.text.tag_bind(tag, '<1>',
			       lambda e=None,w=self,t=textline[pos:startpos]:
			       w._hyper_link(t))
	    if startpos >= len(textline):
		break
	    line = textline[startpos:]
	self.lineno = self.lineno + 1
	self.text['state'] = 'disabled'

    def _highlight(self, tag, how):
	if how:
	    self.text.tag_config(tag, background="#43ce80", relief=Tix.RAISED)
	else:
	    self.text.tag_config(tag, background="", relief=Tix.FLAT)

    def _hyper_link(self, txt):
	if link_pat.search(txt) < 0:
	    print "Invalid man reference string"
	    return
	pagename = txt[link_pat.regs[1][0]:link_pat.regs[1][1]]
	section = txt[link_pat.regs[2][0]:link_pat.regs[2][1]]
	mandirs = ManDirectories()
	pipe = mandirs.FormattedPipe(section, pagename)
	self.parsefile(pipe)

    # Insert a string at the end, with at most one property (tag)
    def _insert_prop(self, str, prop = ' '):
	here = self.text.index('insert')
	self.text.insert('insert', str)
	if prop != ' ':
	    self.text.tag_add(prop, here, 'insert')
#end class ManPageWidget


class ManDirectories:
    """Find all man directories (using MANPATH if defined)

    The section names are kept in the list sections.
    Descriptive names are in the dictionary section_names
    The full path name(s) for each section are in the dictionary secpaths."""

    def __init__(self):
	known_names = {'1':'User Commands', '1b':'Commands: BSD',
		       '1c':'Commands: Communications',
		       '1f':'Commands: FMLI', '1m':'Commands: Maintenance',
		       '1s':'Commands: SunOS specific',
		       '2':'System Calls',
		       '3':'Subroutines', '3b':'Routines: BSD',
		       '3c':'Routines: C Library', '3e':'Routines: ELF',
		       '3g':'Routines: General', '3i':'Routines: Wide Char',
		       '3k':'Routines: Kernel VM', '3m':'Routines: Math',
		       '3n':'Routines: Network', '3r':'Routines: Realtime',
		       '3s':'Routines: Std. I/O', '3t':'Routines: Threads',
		       '3x':'Routines: Misc.',
		       '4':'File Formats', '4b':'Files: BSD',
		       '5':'Miscellaneous',
		       '6':'Games',
		       '7':'Devices',
		       '9':'Device Drivers', '9e':'Drivers: Entry Points',
		       '9f':'Drivers: Functions',
		       '9s':'Drivers: Data Structures',
		       'l':'Local',
		       'n':'New'}
	if os.environ.has_key('MANPATH'):
	    manpath = os.environ["MANPATH"]
	if not manpath:
	    manpath = "/usr/share/man"
	manpath = string.splitfields(manpath, ':')
	self.secpaths = {}
	for path in manpath:
	    files = os.listdir(path)
	    for f in files:
		if os.path.isdir(path + '/' + f) and len(f) > 3 and f[:3] == 'man':
		    sec = f[3:]
		    if self.secpaths.has_key(sec):
			temp = self.secpaths[sec] + ':'
		    else:
			temp = ''
		    self.secpaths[sec] = temp + path + '/' + f
	self.sections = self.secpaths.keys()
	self.sections.sort()
	self.section_names = {}
	for s in self.sections:
	    if s in known_names.keys():
		self.section_names[s + ': ' + known_names[s]] = s
	    else:
		self.section_names[s] = s

    def Pages(self, secname):
	if not self.secpaths.has_key(secname):
	    return []
	paths = string.splitfields(self.secpaths[secname], ':')
	wid = len(secname)
	names = []
	for path in paths:
	    files = os.listdir(path)
	    for file in files:
		if file[-(wid + 1):-wid] == '.' and file[-wid:] == secname:
		    file = file[:-(wid + 1)]
		    if file not in names:
			# if duplicate - preceding path takes precedence
			names.append(file)
	names.sort()
	return names

    def FormattedPipe(self, secname, page):
	secname = string.lower(secname)
	if not self.secpaths.has_key(secname):
	    raise ValueError
	file = page + '.' + secname
	paths = string.splitfields(self.secpaths[secname], ':')
	cwd = os.getcwd()
	for path in paths:
	    files = os.listdir(path)
	    if file in files:
		file = path + '/' + file
		os.chdir(path)
		os.chdir('..')
		break
	pipe = os.popen('nroff -man %s | ul -i' % file)
	os.chdir(cwd)
	return pipe
#end class ManDirectories


class ManPageWindow:
    def __init__(self, pipe):
	self.top = Tix.Toplevel()
	frame = Tix.Frame(self.top)
	frame2 = Tix.Frame(frame)
	self.search_str = Tix.StringVar()
	self.case_sensitive = Tix.StringVar()
	btn = Tix.Button(frame2, text='Regexp Search:', command=self.Search)
	entry = Tix.Entry(frame2, relief=Tix.SUNKEN)
	entry['textvariable'] = self.search_str
	entry.bind('<Return>', self.Search)
	casesense = Tix.Checkbutton(frame2, text='Case Sensitive',
				    relief=Tix.FLAT,
				    variable=self.case_sensitive)
	btn.pack(side=Tix.LEFT, expand=0)
	entry.pack(side=Tix.LEFT, expand=1, fill=Tix.X)
	casesense.pack(side=Tix.RIGHT, expand=0)
	self.man = ManPageWidget(frame)
	btn = Tix.Button(frame, text='Close', command=self.Quit)
	frame2.pack(side=Tix.TOP, expand=0, fill=Tix.X)
	self.man.pack(side=Tix.TOP, expand=1, fill=Tix.BOTH)
	btn.pack(side=Tix.BOTTOM, expand=0, fill=Tix.X)
	frame.pack(expand=1, fill=Tix.BOTH)
	self.man.parsefile(pipe)

    def Search(self, event=None):
	str = self.search_str.get()
	if not str:
	    self.top.bell()
	    print "No search string ?"
	    return
	try:
	    if self.case_sensitive.get() == '1':
		pat = regex.compile(str, regex.casefold)
	    else:
		pat = regex.compile(str)
	except regex.error, msg:
	    self.top.bell()
	    print "regex error"
	    return
	pos = self.man.text.index('insert')
	lineno = string.atoi(pos[:string.find(pos, '.')])
	endpos = self.man.text.index('end')
	endlineno = string.atoi(endpos[:string.find(endpos, '.')])
	wraplineno = lineno
	found = 0
	while 1:
	    lineno = lineno + 1
	    if lineno > endlineno:
		if wraplineno <= 0:
		    break
		endlineno = wraplineno
		lineno = 0
		wraplineno = 0
	    line = self.man.text.get('%d.0 linestart' % lineno,
				     '%d.0 lineend' % lineno)
	    i = pat.search(line)
	    if i >= 0:
		found = 1
		n = max(1, len(pat.group(0)))
		try:
		    self.man.text.tag_remove('sel', 'sel.first', 'sel.last')
		except Tix.TclError:
		    pass
		self.man.text.tag_add('sel', '%d.%d' % (lineno, i),
				      '%d.%d' % (lineno, i+n))
		self.man.text.mark_set('insert', '%d.%d' % (lineno, i))
		self.man.text.yview_pickplace('insert')
		break

	if not found:
	    self.frame.bell()

    def Quit(self):
	del self.search_str
	del self.case_sensitive
       	self.top.destroy()
#end class ManPageWindow

class AproposWindow:
    def __init__(self):
	self.top = Tix.Toplevel()
	frame = Tix.Frame(self.top)
	frame2 = Tix.Frame(frame)
	self.apropos_str = Tix.StringVar()
	btn = Tix.Button(frame2, text='Apropos:', command=self.Apropos)
	entry = Tix.Entry(frame2, relief=Tix.SUNKEN, width=20)
	entry['textvariable'] = self.apropos_str
	entry.bind('<Return>', self.Apropos)
	btn.pack(side=Tix.LEFT, expand=0)
	entry.pack(side=Tix.RIGHT, expand=1, fill=Tix.X)
	frame2.pack(side=Tix.TOP, expand=0, fill=Tix.X)
	self.stext = Tix.ScrolledText(frame)
	self.stext.text.tag_config('!', font=BOLDFONT)
	btn = Tix.Button(frame, text='Close', command=self.Quit)
	self.stext.pack(side=Tix.TOP, expand=1, fill=Tix.BOTH)
	btn.pack(side=Tix.BOTTOM, expand=0, fill=Tix.X)
	frame.pack(expand=1, fill=Tix.BOTH)

    def Apropos(self, event=None):
	str = self.apropos_str.get()
	if not str:
	    self.top.bell()
	    print "No string ?"
	    return
	pipe = os.popen('apropos ' + str, 'r')
	self.stext.text.delete('1.0', Tix.END)
	tabs = regex.compile('\011+')
	num = 1
	while 1:
	    line = pipe.readline()
	    if not line:
		break
	    line = regsub.gsub(tabs, '\011', line)
	    fields = string.splitfields(line, '\011')
	    if len(fields) == 1:
		line = line[string.find(line, ' ') + 1:]
		line = regsub.gsub('^ *', '', line)
		fields = ['???', line]
	    if len(fields) == 2:
		tmp = string.splitfields(fields[1], '-')
		fields = fields[0:1] + tmp
	    num = num + 1
	    self.stext.text.insert('insert', fields[0]+'\t', '!')
	    self.stext.text.insert('insert', fields[1], `num`)
	    self.stext.text.tag_bind(`num`, '<Any-Enter>',
				     lambda e=None,t=`num`,w=self:
				     w._highlight(t, 1))
	    self.stext.text.tag_bind(`num`, '<Any-Leave>',
				     lambda e=None,t=`num`,w=self:
				     w._highlight(t, 0))
	    self.stext.text.tag_bind(`num`, '<1>',
				     lambda e=None,w=self,t=fields[1]:
				     w._hyper_link(t))
	    self.stext.text.insert('insert', fields[2])

    def _highlight(self, tag, how):
	if how:
	    self.stext.text.tag_config(tag, background="#43ce80",
				       relief=Tix.RAISED)
	else:
	    self.stext.text.tag_config(tag, background="", relief=Tix.FLAT)

    def _hyper_link(self, txt):
	if link_pat.search(txt) < 0:
	    print "Invalid man reference string"
	    return
	pagename = txt[link_pat.regs[1][0]:link_pat.regs[1][1]]
	section = txt[link_pat.regs[2][0]:link_pat.regs[2][1]]
	mandirs = ManDirectories()
	pipe = mandirs.FormattedPipe(section, pagename)
	disp = ManPageWindow(pipe)

    def Quit(self):
	del self.apropos_str
	self.top.destroy()

class PManWindow:
    def __init__(self, master=None):
	self.mandirs = ManDirectories()
	self.frame = Tix.Frame(master)
	self.section = Tix.StringVar()
	combo = Tix.ComboBox(self.frame, label='Section: ', dropdown=1,
			     editable=0, variable=self.section,
			     command=self.UpdatePageList)
	pagelist = Tix.ScrolledListBox(self.frame, scrollbar='auto')
	self.listbox = pagelist.listbox
	self.listbox.bind('<Double-1>', self.ShowPage)
	temp = self.mandirs.section_names.keys()
	temp.sort()
	for s in temp:
	    combo.insert(Tix.END, s)
	box = Tix.ButtonBox(self.frame, orientation=Tix.HORIZONTAL)
	box.add('show', text='Show Page ...', underline=0, width=13,
		command=self.ShowPage)
	box.add('aprop', text='Apropos ...', underline=0, width=13,
		command=self.Apropos)
	box.add('quit', text='Quit', underline=0, width=13,
		command=self.Quit)
	combo.pack(side=Tix.TOP, expand=0, fill=Tix.X)
	pagelist.pack(side=Tix.TOP, expand=1, fill=Tix.BOTH)
	box.pack(side=Tix.BOTTOM, expand=0, fill=Tix.X)
	self.frame.pack(expand=1, fill=Tix.BOTH)

    def UpdatePageList(self, event=None):
	secname = self.section.get()
	if not self.mandirs.section_names.has_key(secname):
	    return
	secname = self.mandirs.section_names[secname]
	pages = self.mandirs.Pages(secname)
	self.listbox.delete(0, Tix.END)
	for page in pages:
	    self.listbox.insert(Tix.END, page)

    def ShowPage(self, event=None):
	secname = self.section.get()
	secname = self.mandirs.section_names[secname]
	idx = self.listbox.curselection()
	pagename = self.listbox.get(idx)
	pipe = self.mandirs.FormattedPipe(secname, pagename)
	page_display = ManPageWindow(pipe)

    def Apropos(self):
	apropos_disp = AproposWindow()

    def Quit(self):
	sys.exit()
#end class PManWindow

def main():
    root = Tix.Tk()
    root.minsize(10, 10)
    win = PManWindow(root)
    root.mainloop()

if __name__ == '__main__':
    main()
