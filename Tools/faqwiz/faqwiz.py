import sys, string, time, os, stat, regex, cgi, faqconf

from cgi import escape

class FileError:
    def __init__(self, file):
	self.file = file

class InvalidFile(FileError):
    pass

class NoSuchFile(FileError):
    def __init__(self, file, why=None):
	FileError.__init__(self, file)
	self.why = why

def escapeq(s):
    s = escape(s)
    import regsub
    s = regsub.gsub('"', '&quot;', s)
    return s

def interpolate(format, entry={}, kwdict={}, **kw):
    s = format % MDict(kw, entry, kwdict, faqconf.__dict__)
    return s

def emit(format, entry={}, kwdict={}, file=sys.stdout, **kw):
    s = format % MDict(kw, entry, kwdict, faqconf.__dict__)
    file.write(s)

translate_prog = None

def translate(text):
    global translate_prog
    if not translate_prog:
	import regex
	url = '\(http\|ftp\)://[^ \t\r\n]*'
	email = '\<[-a-zA-Z0-9._]+@[-a-zA-Z0-9._]+'
	translate_prog = prog = regex.compile(url + "\|" + email)
    else:
	prog = translate_prog
    i = 0
    list = []
    while 1:
	j = prog.search(text, i)
	if j < 0:
	    break
	list.append(cgi.escape(text[i:j]))
	i = j
	url = prog.group(0)
	while url[-1] in ");:,.?'\"":
	    url = url[:-1]
	url = escape(url)
	if ':' in url:
	    repl = '<A HREF="%s">%s</A>' % (url, url)
	else:
	    repl = '<A HREF="mailto:%s">&lt;%s&gt;</A>' % (url, url)
	list.append(repl)
	i = i + len(url)
    j = len(text)
    list.append(cgi.escape(text[i:j]))
    return string.join(list, '')

emphasize_prog = None

def emphasize(line):
    global emphasize_prog
    import regsub
    if not emphasize_prog:
	import regex
	pat = "\*\([a-zA-Z]+\)\*"
	emphasize_prog = prog = regex.compile(pat)
    else:
	prog = emphasize_prog
    return regsub.gsub(prog, "<I>\\1</I>", line)

def load_cookies():
    if not os.environ.has_key('HTTP_COOKIE'):
	return {}
    raw = os.environ['HTTP_COOKIE']
    words = map(string.strip, string.split(raw, ';'))
    cookies = {}
    for word in words:
	i = string.find(word, '=')
	if i >= 0:
	    key, value = word[:i], word[i+1:]
	    cookies[key] = value
    return cookies

def load_my_cookie():
    cookies = load_cookies()
    try:
	value = cookies[faqconf.COOKIE_NAME]
    except KeyError:
	return {}
    import urllib
    value = urllib.unquote(value)
    words = string.split(value, '/')
    while len(words) < 3:
	words.append('')
    author = string.join(words[:-2], '/')
    email = words[-2]
    password = words[-1]
    return {'author': author,
	    'email': email,
	    'password': password}

class MDict:

    def __init__(self, *d):
	self.__d = d

    def __getitem__(self, key):
	for d in self.__d:
	    try:
		value = d[key]
		if value:
		    return value
	    except KeyError:
		pass
	return ""

class UserInput:

    def __init__(self):
	self.__form = cgi.FieldStorage()

    def __getattr__(self, name):
	if name[0] == '_':
	    raise AttributeError
	try:
	    value = self.__form[name].value
	except (TypeError, KeyError):
	    value = ''
	else:
	    value = string.strip(value)
	setattr(self, name, value)
	return value

    def __getitem__(self, key):
	return getattr(self, key)

class FaqFormatter:

    def __init__(self, entry):
	self.entry = entry

    def show(self, edit=1):
	entry = self.entry
	print "<HR>"
	print "<H2>%s</H2>" % escape(entry.title)
	pre = 0
	for line in string.split(entry.body, '\n'):
	    if not string.strip(line):
		if pre:
		    print '</PRE>'
		    pre = 0
		else:
		    print '<P>'
	    else:
		if line[0] not in string.whitespace:
		    if pre:
			print '</PRE>'
			pre = 0
		else:
		    if not pre:
			print '<PRE>'
			pre = 1
		if '/' in line or '@' in line:
		    line = translate(line)
		elif '<' in line or '&' in line:
		    line = escape(line)
 		if not pre and '*' in line:
 		    line = emphasize(line)
		print line
	if pre:
	    print '</PRE>'
	    pre = 0
	if edit:
	    print '<P>'
	    emit(faqconf.ENTRY_FOOTER, self.entry)
	    if self.entry.last_changed_date:
		emit(faqconf.ENTRY_LOGINFO, self.entry)
	print '<P>'

class FaqEntry:

    formatterclass = FaqFormatter

    def __init__(self, fp, file, sec_num):
	import rfc822
	self.file = file
	self.sec, self.num = sec_num
	self.__headers = rfc822.Message(fp)
	self.body = string.strip(fp.read())

    def __getattr__(self, name):
	if name[0] == '_':
	    raise AttributeError
	key = string.join(string.split(name, '_'), '-')
	try:
	    value = self.__headers[key]
	except KeyError:
	    value = ''
	setattr(self, name, value)
	return value

    def __getitem__(self, key):
	return getattr(self, key)

    def show(self, edit=1):
	self.formatterclass(self).show(edit=edit)

    def load_version(self):
	command = interpolate(faqconf.SH_RLOG_H, self)
	p = os.popen(command)
	version = ""
	while 1:
	    line = p.readline()
	    if not line:
		break
	    if line[:5] == 'head:':
		version = string.strip(line[5:])
	p.close()
	self.version = version

class FaqDir:

    entryclass = FaqEntry

    __okprog = regex.compile('^faq\([0-9][0-9]\)\.\([0-9][0-9][0-9]\)\.htp$')

    def __init__(self, dir=os.curdir):
	self.__dir = dir
	self.__files = None

    def __fill(self):
	if self.__files is not None:
	    return
	self.__files = files = []
	okprog = self.__okprog
	for file in os.listdir(self.__dir):
	    if okprog.match(file) >= 0:
		files.append(file)
	files.sort()

    def good(self, file):
	return self.__okprog.match(file) >= 0

    def parse(self, file):
	if not self.good(file):
	    return None
	sec, num = self.__okprog.group(1, 2)
	return string.atoi(sec), string.atoi(num)

    def roulette(self):
	self.__fill()
	import whrandom
	return whrandom.choice(self.__files)

    def list(self):
	# XXX Caller shouldn't modify result
	self.__fill()
	return self.__files

    def open(self, file):
	sec_num = self.parse(file)
	if not sec_num:
	    raise InvalidFile(file)
	try:
	    fp = open(file)
	except IOError, msg:
	    raise NoSuchFile(file, msg)
	try:
	    return self.entryclass(fp, file, sec_num)
	finally:
	    fp.close()

    def show(self, file, edit=1):
	self.open(file).show(edit=edit)

    def new(self, sec):
	XXX

class FaqWizard:

    def __init__(self):
	self.ui = UserInput()
	self.dir = FaqDir()

    def go(self):
	print "Content-type: text/html"
	req = self.ui.req or "home"
	mname = 'do_%s' % req
	try:
	    meth = getattr(self, mname)
	except AttributeError:
	    self.error("Bad request %s" % `req`)
	else:
	    try:
		meth()
	    except InvalidFile, exc:
		self.error("Invalid entry file name %s" % exc.file)
	    except NoSuchFile, exc:
		self.error("No entry with file name %s" % exc.file)
	self.epilogue()

    def error(self, message, **kw):
	self.prologue(faqconf.T_ERROR)
	apply(emit, (message,), kw)

    def prologue(self, title, entry=None, **kw):
	emit(faqconf.PROLOGUE, entry, kwdict=kw, title=escape(title))

    def epilogue(self):
	emit(faqconf.EPILOGUE)

    def do_home(self):
	self.prologue(faqconf.T_HOME)
	emit(faqconf.HOME)

    def do_search(self):
	query = self.ui.query
	if not query:
	    self.error("No query string")
	    return
	self.prologue(faqconf.T_SEARCH)
	if self.ui.casefold == "no":
	    p = regex.compile(query)
	else:
	    p = regex.compile(query, regex.casefold)
	hits = []
	for file in self.dir.list():
	    try:
		entry = self.dir.open(file)
	    except FileError:
		constants
	    if p.search(entry.title) >= 0 or p.search(entry.body) >= 0:
		hits.append(file)
	if not hits:
	    emit(faqconf.NO_HITS, count=0)
	elif len(hits) <= faqconf.MAXHITS:
	    if len(hits) == 1:
		emit(faqconf.ONE_HIT, count=1)
	    else:
		emit(faqconf.FEW_HITS, count=len(hits))
	    self.format_all(hits)
	else:
	    emit(faqconf.MANY_HITS, count=len(hits))
	    self.format_index(hits)

    def do_all(self):
	self.prologue(faqconf.T_ALL)
	files = self.dir.list()
	self.last_changed(files)
	self.format_all(files)

    def do_compat(self):
	files = self.dir.list()
	emit(faqconf.COMPAT)
	self.last_changed(files)
	self.format_all(files, edit=0)
	sys.exit(0)

    def last_changed(self, files):
	latest = 0
	for file in files:
	    try:
		st = os.stat(file)
	    except os.error:
		continue
	    mtime = st[stat.ST_MTIME]
	    if mtime > latest:
		latest = mtime
	print time.strftime(faqconf.LAST_CHANGED,
			    time.localtime(time.time()))

    def format_all(self, files, edit=1):
	for file in files:
	    self.dir.show(file, edit=edit)

    def do_index(self):
	self.prologue(faqconf.T_INDEX)
	self.format_index(self.dir.list())

    def format_index(self, files):
	sec = 0
	for file in files:
	    try:
		entry = self.dir.open(file)
	    except NoSuchFile:
		continue
	    if entry.sec != sec:
		if sec:
		    emit(faqconf.INDEX_ENDSECTION, sec=sec)
		sec = entry.sec
		emit(faqconf.INDEX_SECTION,
			    sec=sec,
			    title=faqconf.SECTION_TITLES[sec])
	    emit(faqconf.INDEX_ENTRY, entry)
	if sec:
	    emit(faqconf.INDEX_ENDSECTION, sec=sec)

    def do_recent(self):
	if not self.ui.days:
	    days = 1
	else:
	    days = string.atof(self.ui.days)
	now = time.time()
	try:
	    cutoff = now - days * 24 * 3600
	except OverflowError:
	    cutoff = 0
	list = []
	for file in self.dir.list():
	    try:
		st = os.stat(file)
	    except os.error:
		continue
	    mtime = st[stat.ST_MTIME]
	    if mtime >= cutoff:
		list.append((mtime, file))
	list.sort()
	list.reverse()
	self.prologue(faqconf.T_RECENT)
	if days <= 1:
	    period = "%.2g hours" % (days*24)
	else:
	    period = "%.6g days" % days
	if not list:
	    emit(faqconf.NO_RECENT, period=period)
	elif len(list) == 1:
	    emit(faqconf.ONE_RECENT, period=period)
	else:
	    emit(faqconf.SOME_RECENT, period=period, count=len(list))
	self.format_all(map(lambda (mtime, file): file, list))
	emit(faqconf.TAIL_RECENT)

    def do_roulette(self):
	self.prologue(faqconf.T_ROULETTE)
	file = self.dir.roulette()
	self.dir.show(file)

    def do_help(self):
	self.prologue(faqconf.T_HELP)
	emit(faqconf.HELP)

    def do_show(self):
	entry = self.dir.open(self.ui.file)
	self.prologue("Python FAQ Entry")
	entry.show()

    def do_add(self):
	self.prologue(T_ADD)
	self.error("Not yet implemented")

    def do_delete(self):
	self.prologue(T_DELETE)
	self.error("Not yet implemented")

    def do_log(self):
	entry = self.dir.open(self.ui.file)
	self.prologue(faqconf.T_LOG, entry)
	emit(faqconf.LOG, entry)
	self.rlog(interpolate(faqconf.SH_RLOG, entry), entry)

    def rlog(self, command, entry=None):
	output = os.popen(command).read()
	sys.stdout.write("<PRE>")
	athead = 0
	lines = string.split(output, "\n")
	while lines and not lines[-1]:
	    del lines[-1]
	if lines:
	    line = lines[-1]
	    if line[:1] == '=' and len(line) >= 40 and \
	       line == line[0]*len(line):
		del lines[-1]
	for line in lines:
	    if entry and athead and line[:9] == 'revision ':
		rev = string.strip(line[9:])
		if rev != "1.1":
		    emit(faqconf.DIFFLINK, entry, rev=rev, line=line)
		else:
		    print line
		athead = 0
	    else:
		athead = 0
		if line[:1] == '-' and len(line) >= 20 and \
		   line == len(line) * line[0]:
		    athead = 1
		    sys.stdout.write("<HR>")
		else:
		    print line
	print "</PRE>"

    def do_diff(self):
	entry = self.dir.open(self.ui.file)
	rev = self.ui.rev
	r = regex.compile(
	    "^\([1-9][0-9]?[0-9]?\)\.\([1-9][0-9]?[0-9]?[0-9]?\)$")
	if r.match(rev) < 0:
	    self.error("Invalid revision number: %s" % `rev`)
	[major, minor] = map(string.atoi, r.group(1, 2))
	if minor == 1:
	    self.error("No previous revision")
	    return
	prev = "%d.%d" % (major, minor-1)
	self.prologue(faqconf.T_DIFF, entry)
	self.shell(interpolate(faqconf.SH_RDIFF, entry, rev=rev, prev=prev))

    def shell(self, command):
	output = os.popen(command).read()
	sys.stdout.write("<PRE>")
	print escape(output)
	print "</PRE>"

    def do_new(self):
	editor = FaqEditor(self.ui, self.dir.new(self.file))
	self.prologue(faqconf.T_NEW)
	self.error("Not yet implemented")

    def do_edit(self):
	entry = self.dir.open(self.ui.file)
	entry.load_version()
	self.prologue(faqconf.T_EDIT)
	emit(faqconf.EDITHEAD)
	emit(faqconf.EDITFORM1, entry, editversion=entry.version)
	emit(faqconf.EDITFORM2, entry, load_my_cookie(), log=self.ui.log)
	emit(faqconf.EDITFORM3)
	entry.show(edit=0)

    def do_review(self):
	entry = self.dir.open(self.ui.file)
	entry.load_version()
	# Check that the FAQ entry number didn't change
	if string.split(self.ui.title)[:1] != string.split(entry.title)[:1]:
	    self.error("Don't change the FAQ entry number please.")
	    return
	# Check that the edited version is the current version
	if entry.version != self.ui.editversion:
	    self.error("Version conflict.")
	    emit(faqconf.VERSIONCONFLICT, entry, self.ui)
	    return
	commit_ok = ((not faqconf.PASSWORD
		      or self.ui.password == faqconf.PASSWORD) 
		     and self.ui.author
		     and '@' in self.ui.email
		     and self.ui.log)
	if self.ui.commit:
	    if not commit_ok:
		self.cantcommit()
	    else:
		self.commit()
	    return
	self.prologue(faqconf.T_REVIEW)
	emit(faqconf.REVIEWHEAD)
	entry.body = self.ui.body
	entry.title = self.ui.title
	entry.show(edit=0)
	emit(faqconf.EDITFORM1, entry, self.ui)
	if commit_ok:
	    emit(faqconf.COMMIT)
	else:
	    emit(faqconf.NOCOMMIT)
	emit(faqconf.EDITFORM2, entry, load_my_cookie(), log=self.ui.log)
	emit(faqconf.EDITFORM3)

    def cantcommit(self):
	self.prologue(faqconf.T_CANTCOMMIT)
	print faqconf.CANTCOMMIT_HEAD
	if not self.ui.passwd:
	    emit(faqconf.NEED_PASSWD)
	if not self.ui.log:
	    emit(faqconf.NEED_LOG)
	if not self.ui.author:
	    emit(faqconf.NEED_AUTHOR)
	if not self.ui.email:
	    emit(faqconf.NEED_EMAIL)
	print faqconf.CANTCOMMIT_TAIL

    def commit(self):
	file = self.ui.file
	entry = self.dir.open(file)
	# Chech that there were any changes
	if self.ui.body == entry.body and self.ui.title == entry.title:
	    self.error("No changes.")
	    return
	# XXX Should lock here
	try:
	    os.unlink(file)
	except os.error:
	    pass
	try:
	    f = open(file, "w")
	except IOError, why:
	    self.error(faqconf.CANTWRITE, file=file, why=why)
	    return
	date = time.ctime(time.time())
	emit(faqconf.FILEHEADER, self.ui, os.environ, date=date, file=f)
	f.write("\n")
	f.write(self.ui.body)
	f.write("\n")
	f.close()

	import tempfile
	tfn = tempfile.mktemp()
	f = open(tfn, "w")
	emit(faqconf.LOGHEADER, self.ui, os.environ, date=date, file=f)
	f.close()

	command = interpolate(
	    faqconf.SH_LOCK + "\n" + faqconf.SH_CHECKIN,
	    file=file, tfn=tfn)

	p = os.popen(command)
	output = p.read()
	sts = p.close()
	# XXX Should unlock here
	if not sts:
	    self.prologue(faqconf.T_COMMITTED)
	    emit(faqconf.COMMITTED)
	else:
	    self.error(faqconf.T_COMMITFAILED)
	    emit(faqconf.COMMITFAILED, sts=sts)
	print "<PRE>%s</PRE>" % cgi.escape(output)

	try:
	    os.unlink(tfn)
	except os.error:
	    pass

	entry = self.dir.open(file)
	entry.show()

wiz = FaqWizard()
wiz.go()

BOOTSTRAP = """\
#! /usr/local/bin/python
FAQDIR = "/usr/people/guido/python/FAQ"

# This bootstrap script should be placed in your cgi-bin directory.
# You only need to edit the first two lines (above): Change
# /usr/local/bin/python to where your Python interpreter lives (you
# can't use /usr/bin/env here!); change FAQDIR to where your FAQ
# lives.  The faqwiz.py and faqconf.py files should live there, too.

import posix
t1 = posix.times()
import os, sys, time, operator
os.chdir(FAQDIR)
sys.path.insert(0, FAQDIR)
try:
    import faqwiz
except SystemExit, n:
    sys.exit(n)
except:
    t, v, tb = sys.exc_type, sys.exc_value, sys.exc_traceback
    print
    import cgi
    cgi.print_exception(t, v, tb)
t2 = posix.times()
fmt = "<BR>(times: user %.3g, sys %.3g, ch-user %.3g, ch-sys %.3g, real %.3g)"
print fmt % tuple(map(operator.sub, t2, t1))
"""
