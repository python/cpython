import aetools
import Standard_Suite
import Required_Suite
import WWW_Suite
import re
import W
import macfs
import os
import MacPrefs
import MacOS
import string

if hasattr(WWW_Suite, "WWW_Suite"):
	WWW = WWW_Suite.WWW_Suite
else:
	WWW = WWW_Suite.WorldWideWeb_suite_2c__as_defined_in_Spyglass_spec_2e_

class WebBrowser(aetools.TalkTo, 
		Standard_Suite.Standard_Suite, 
		WWW):
	
	def openfile(self, path, activate = 1):
		if activate:
			self.activate()
		self.OpenURL("file:///" + string.join(string.split(path,':'), '/'))

app = W.getapplication()

#SIGNATURE='MSIE' # MS Explorer
SIGNATURE='MOSS' # Netscape

_titlepat = re.compile('<title>\([^<]*\)</title>')

def sucktitle(path):
	f = open(path)
	text = f.read(1024) # assume the title is in the first 1024 bytes
	f.close()
	lowertext = string.lower(text)
	matcher = _titlepat.search(lowertext)
	if matcher:
		return matcher.group(1)
	return path

def verifydocpath(docpath):
	try:
		tut = os.path.join(docpath, "tut")
		lib = os.path.join(docpath, "lib")
		ref = os.path.join(docpath, "ref")
		for path in [tut, lib, ref]:
			if not os.path.exists(path):
				return 0
	except:
		return 0
	return 1


class TwoLineList(W.List):
	
	LDEF_ID = 468
	
	def createlist(self):
		import List
		self._calcbounds()
		self.SetPort()
		rect = self._bounds
		rect = rect[0]+1, rect[1]+1, rect[2]-16, rect[3]-1
		self._list = List.LNew(rect, (0, 0, 1, 0), (0, 28), self.LDEF_ID, self._parentwindow.wid,
					0, 1, 0, 1)
		self.set(self.items)


_resultscounter = 1

class Results:
	
	def __init__(self, hits):
		global _resultscounter
		hits = map(lambda (path, hits): (sucktitle(path), path, hits), hits)
		hits.sort()
		self.hits = hits
		nicehits = map(
				lambda (title, path, hits):
				title + '\r' + string.join(
				map(lambda (c, p): "%s (%d)" % (p, c), hits), ', '), hits)
		nicehits.sort()
		self.w = W.Window((440, 300), "Search results %d" % _resultscounter, minsize = (200, 100))
		self.w.results = TwoLineList((-1, -1, 1, -14), nicehits, self.listhit)
		self.w.open()
		self.w.bind('return', self.listhit)
		self.w.bind('enter', self.listhit)
		_resultscounter = _resultscounter + 1
		self.browser = None
	
	def listhit(self, isdbl = 1):
		if isdbl:
			for i in self.w.results.getselection():
				if self.browser is None:
					self.browser = WebBrowser(SIGNATURE, start = 1)
				self.browser.openfile(self.hits[i][1])

class Status:
	
	def __init__(self):
		self.w = W.Dialog((440, 64), "Searching\xc9")
		self.w.searching = W.TextBox((4, 4, -4, 16), "DevDev:PyPyDoc 1.5.1:ext:parseTupleAndKeywords.html")
		self.w.hits = W.TextBox((4, 24, -4, 16), "Hits: 0")
		self.w.canceltip = W.TextBox((4, 44, -4, 16), "Type cmd-period (.) to cancel.")
		self.w.open()
	
	def set(self, path, hits):
		self.w.searching.set(path)
		self.w.hits.set('Hits: ' + `hits`)
		app.breathe()
	
	def close(self):
		self.w.close()


def match(text, patterns, all):
	hits = []
	hitsappend = hits.append
	stringcount = string.count
	for pat in patterns:
		c = stringcount(text, pat)
		if c > 0:
			hitsappend((c, pat))
		elif all:
			hits[:] = []
			break
	hits.sort()
	hits.reverse()
	return hits

def dosearch(docpath, searchstring, settings):
	(docpath, kind, case, word, tut, lib, ref, ext, api) = settings
	books = [(tut, 'tut'), (lib, 'lib'), (ref, 'ref'), (ext, 'ext'), (api, 'api')]
	if not case:
		searchstring = string.lower(searchstring)
	
	if kind == 1:
		patterns = string.split(searchstring)
		all = 1
	elif kind == 2:
		patterns = string.split(searchstring)
		all = 0
	else:
		patterns = [searchstring]
		all = 0 # not relevant
	
	ospathjoin = os.path.join
	stringlower = string.lower
	status = Status()
	statusset = status.set
	_match = match
	_open = open
	hits = {}
	try:
		MacOS.EnableAppswitch(0)
		try:
			for do, name in books:
				if not do:
					continue
				bookpath = ospathjoin(docpath, name)
				if not os.path.exists(bookpath):
					continue
				files = os.listdir(bookpath)
				for file in files:
					fullpath = ospathjoin(bookpath, file)
					if fullpath[-5:] <> '.html':
						continue
					statusset(fullpath, len(hits))
					f = _open(fullpath)
					text = f.read()
					if not case:
						text = stringlower(text)
					f.close()
					filehits = _match(text, patterns, all)
					if filehits:
						hits[fullpath] = filehits
		finally:
			MacOS.EnableAppswitch(-1)
			status.close()
	except KeyboardInterrupt:
		pass
	hits = hits.items()
	hits.sort()
	return hits


class PyDocSearch:
	
	def __init__(self):
		prefs = MacPrefs.GetPrefs(W.getapplication().preffilepath)
		try:
			(docpath, kind, case, word, tut, lib, ref, ext, api) = prefs.docsearchengine
		except:
			(docpath, kind, case, word, tut, lib, ref, ext, api) = prefs.docsearchengine = \
				("", 0, 0, 0, 1, 1, 0, 0, 0)
		
		if docpath and not verifydocpath(docpath):
			docpath = ""
		
		self.w = W.Window((400, 200), "Search the Python Documentation")
		self.w.searchtext = W.EditText((10, 10, -100, 20), callback = self.checkbuttons)
		self.w.searchbutton = W.Button((-90, 12, 80, 16), "Search", self.search)
		buttons = []
		
		gutter = 10
		width = 130
		bookstart = width + 2 * gutter
		self.w.phraseradio = W.RadioButton((10, 38, width, 16), "As a phrase", buttons)
		self.w.allwordsradio = W.RadioButton((10, 58, width, 16), "All words", buttons)
		self.w.anywordsradio = W.RadioButton((10, 78, width, 16), "Any word", buttons)
		self.w.casesens = W.CheckBox((10, 98, width, 16), "Case sensitive")
		self.w.wholewords = W.CheckBox((10, 118, width, 16), "Whole words")
		self.w.tutorial = W.CheckBox((bookstart, 38, -10, 16), "Tutorial")
		self.w.library = W.CheckBox((bookstart, 58, -10, 16), "Library reference")
		self.w.langueref = W.CheckBox((bookstart, 78, -10, 16), "Lanuage reference manual")
		self.w.extending = W.CheckBox((bookstart, 98, -10, 16), "Extending & embedding")
		self.w.api = W.CheckBox((bookstart, 118, -10, 16), "C/C++ API")
		
		self.w.setdocfolderbutton = W.Button((10, -30, 80, 16), "Set doc folder", self.setdocpath)
		
		if docpath:
			self.w.setdefaultbutton(self.w.searchbutton)
		else:
			self.w.setdefaultbutton(self.w.setdocfolderbutton)
		
		self.docpath = docpath
		if not docpath:
			docpath = "(please select the Python html documentation folder)"
		self.w.docfolder = W.TextBox((100, -28, -10, 16), docpath)
		
		[self.w.phraseradio, self.w.allwordsradio, self.w.anywordsradio][kind].set(1)
		
		self.w.casesens.set(case)
		self.w.wholewords.set(word)
		self.w.tutorial.set(tut)
		self.w.library.set(lib)
		self.w.langueref.set(ref)
		self.w.extending.set(ext)
		self.w.api.set(api)
		
		self.w.open()
		self.w.wholewords.enable(0)
		self.w.bind('<close>', self.close)
		self.w.searchbutton.enable(0)
	
	def search(self):
		hits = dosearch(self.docpath, self.w.searchtext.get(), self.getsettings())
		if hits:
			Results(hits)
		elif hasattr(MacOS, 'SysBeep'):
			MacOS.SysBeep(0)
		#import PyBrowser
		#PyBrowser.Browser(hits)
	
	def setdocpath(self):
		fss, ok = macfs.GetDirectory()
		if ok:
			docpath = fss.as_pathname()
			if not verifydocpath(docpath):
				W.Message("This does not seem to be a Python documentation folder...")
			else:
				self.docpath = docpath
				self.w.docfolder.set(docpath)
				self.w.setdefaultbutton(self.w.searchbutton)
	
	def close(self):
		prefs = MacPrefs.GetPrefs(W.getapplication().preffilepath)
		prefs.docsearchengine = self.getsettings()
	
	def getsettings(self):
		radiobuttons = [self.w.phraseradio, self.w.allwordsradio, self.w.anywordsradio]
		for i in range(3):
			if radiobuttons[i].get():
				kind = i
				break
		docpath = self.docpath
		case = self.w.casesens.get()
		word = self.w.wholewords.get()
		tut = self.w.tutorial.get()
		lib = self.w.library.get()
		ref = self.w.langueref.get()
		ext = self.w.extending.get()
		api = self.w.api.get()
		return (docpath, kind, case, word, tut, lib, ref, ext, api)
	
	def checkbuttons(self):
		self.w.searchbutton.enable(not not self.w.searchtext.get())
