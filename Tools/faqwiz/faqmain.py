"""Interactive FAQ project.

Note that this is not an executable script; it's an importable module.
The actual CGI script can be kept minimal; it's appended at the end of
this file as a string constant.

XXX TO DO

- next/prev/index links in do_show?
- should have files containing section headers
- customize rcs command pathnames
- recognize urls and email addresses and turn them into <A> tags
- use cookies to keep Name/email the same
- explanation of editing somewhere
- various embellishments, GIFs, crosslinks, hints, etc.
- create new sections
- rearrange entries
- delete entries
- send email on changes
- optional staging of entries until reviewed?
- freeze entries
- username/password for editors
- Change references to other Q's and whole sections
- Browse should display menu of 7 sections & let you pick
  (or frontpage should have the option to browse a section or all)
- support adding annotations, too
- make it more generic (so you can create your own FAQ)

"""

NAMEPAT = "faq??.???.htp"
NAMEREG = "^faq\([0-9][0-9]\)\.\([0-9][0-9][0-9]\)\.htp$"

class FAQServer:

    def __init__(self):
	pass

    def main(self):
	self.form = cgi.FieldStorage()
	req = self.req or 'frontpage'
	try:
	    method = getattr(self, 'do_%s' % req)
	except AttributeError:
	    print "Unrecognized request type", req
	else:
	    method()
	    self.epilogue()

    KEYS = ['req', 'query', 'name', 'text', 'commit', 'title',
	    'author', 'email', 'log', 'section', 'number', 'add',
	    'version']

    def __getattr__(self, key):
	if key not in self.KEYS:
	    raise AttributeError
	try:
	    form = self.form
	    try:
		item = form[key]
	    except TypeError, msg:
		raise KeyError, msg, sys.exc_traceback
	except KeyError:
	    return ''
	value = self.form[key].value
	value = string.strip(value)
	setattr(self, key, value)
	return value

    def do_frontpage(self):
	self.prologue("Python FAQ (alpha) Front Page")
	print """
	<UL>
	<LI><A HREF="faq.py?req=index">FAQ index</A>
	<LI><A HREF="faq.py?req=all">The whole FAQ</A>
	<LI><A HREF="faq.py?req=roulette">FAQ roulette</A>
	<LI><A HREF="faq.py?req=recent">Recently changed FAQ entries</A>
	<LI><A HREF="faq.py?req=add">Add a new FAQ entry</A>
	<LI><A HREF="faq.py?req=delete">Delete a FAQ entry</A>
	</UL>

	<H2>Search the FAQ</H2>

	<FORM ACTION="faq.py?req=query">
	<INPUT TYPE=text NAME=query>
	<INPUT TYPE=submit VALUE="Search">
	<INPUT TYPE=hidden NAME=req VALUE=query>
	</FORM>

	Disclaimer: these pages are intended to be edited by anyone.
	Please exercise discretion when editing, don't be rude, etc.
	"""

    def do_index(self):
	self.prologue("Python FAQ Index")
	names = os.listdir(os.curdir)
	names.sort()
	section = None
	for name in names:
	    headers, text = self.read(name)
	    if headers:
		title = headers['title']
		i = string.find(title, '.')
		nsec = title[:i]
		if nsec != section:
		    if section:
			print """
			<P>
			<LI><A HREF="faq.py?req=add&amp;section=%s"
			>Add new entry</A> (at this point)
			</UL>
			""" % section
		    section = nsec
		    print "<H2>Section %s</H2>" % section
		    print "<UL>"
		print '<LI><A HREF="faq.py?req=show&name=%s">%s</A>' % (
		    name, cgi.escape(title))
	if section:
	    print """
	    <P>
	    <LI><A HREF="faq.py?req=add&amp;section=%s">Add new entry</A>
	    (at this point)
	    </UL>
	    """ % section
	else:
	    print "No FAQ entries?!?!"

    def do_show(self):
	self.prologue("Python FAQ Entry")
	print "<HR>"
	name = self.name
	headers, text = self.read(name)
	if not headers:
	    self.error("Invalid file name", name)
	    return
	self.show(name, headers['title'], text)

    def do_all(self):
	self.prologue("The Whole Python FAQ")
	print "<HR>"
	names = os.listdir(os.curdir)
	names.sort()
	section = None
	for name in names:
	    headers, text = self.read(name)
	    if headers:
		title = headers['title']
		i = string.find(title, '.')
		nsec = title[:i]
		if nsec != section:
		    section = nsec
		    print "<H1>Section %s</H1>" % section
		    print "<HR>"
		self.show(name, title, text)
	if not section:
	    print "No FAQ entries?!?!"

    def do_roulette(self):
	import whrandom
	self.prologue("Python FAQ Roulette")
	print """
	Please check the correctness of the entry below.
	If you find any problems, please edit the entry.
	<P>
	<HR>
	"""
	names = os.listdir(os.curdir)
	while names:
	    name = whrandom.choice(names)
	    headers, text = self.read(name)
	    if headers:
		self.show(name, headers['title'], text)
		print "<P>Use `Reload' to show another one."
		break
	    else:
		names.remove(name)
	else:
	    print "No FAQ entries?!?!"

    def do_recent(self):
	import fnmatch, stat
	names = os.listdir(os.curdir)
	now = time.time()
	list = []
	for name in names:
	    if not fnmatch.fnmatch(name, NAMEPAT):
		continue
	    try:
		st = os.stat(name)
	    except os.error:
		continue
	    tuple = (st[stat.ST_MTIME], name)
	    list.append(tuple)
	list.sort()
	list.reverse()
	self.prologue("Python FAQ, Most Recently Modified First")
	print "<HR>"
	n = 0
	for (mtime, name) in list:
	    headers, text = self.read(name)
	    if headers:
		self.show(name, headers['title'], text)
		n = n+1
	if not n:
	    print "No FAQ entries?!?!"

    def do_query(self):
	query = self.query
	if not query:
	    self.error("No query string")
	    return
	import regex
	self.prologue("Python FAQ Query Results")
	p = regex.compile(query, regex.casefold)
	names = os.listdir(os.curdir)
	names.sort()
	print "<HR>"
	n = 0
	for name in names:
	    headers, text = self.read(name)
	    if headers:
		title = headers['title']
		if p.search(title) >= 0 or p.search(text) >= 0:
		    self.show(name, title, text)
		    n = n+1
	if not n:
	    print "No hits."

    def do_add(self):
	section = self.section
	if not section:
	    self.prologue("How to add a new FAQ entry")
	    print """
	    Go to the <A HREF="faq.py?req=index">FAQ index</A>
	    and click on the "Add new entry" link at the end
	    of the section to which you want to add the entry.
	    """
	    return
	try:
	    nsec = string.atoi(section)
	except ValueError:
	    print "Bad section number", nsec
	names = os.listdir(os.curdir)
	max = 0
	import regex
	prog = regex.compile(NAMEREG)
	for name in names:
	    if prog.match(name) >= 0:
		s1, s2 = prog.group(1, 2)
		n1, n2 = string.atoi(s1), string.atoi(s2)
		if n1 == nsec:
		    if n2 > max:
			max = n2
	if not max:
	    self.error("Can't add new sections yet.")
	    return
	num = max+1
	name = "faq%02d.%03d.htp" % (nsec, num)
	self.name = name
	self.add = "yes"
	self.number = str(num)
	self.do_edit()

    def do_delete(self):
	self.prologue("How to delete a FAQ entry")
	print """
	At the moment, there's no direct way to delete entries.
	This is because the entry numbers are also their
	unique identifiers -- it's a bad idea to renumber entries.
	<P>
	If you really think an entry needs to be deleted,
	change the title to "(deleted)" and make the body
	empty (keep the entry number in the title though).
	"""

    def do_edit(self):
	name = self.name
	headers, text = self.read(name)
	if not headers:
	    self.error("Invalid file name", name)
	    return
	self.prologue("Python FAQ Edit Form")
	title = headers['title']
	version = self.getversion(name)
	print "<FORM METHOD=POST ACTION=faq.py>"
	self.showedit(name, title, text)
	if self.add:
	    print """
	    <INPUT TYPE=hidden NAME=add VALUE=%s>
	    <INPUT TYPE=hidden NAME=section VALUE=%s>
	    <INPUT TYPE=hidden NAME=number VALUE=%s>
	    """ % (self.add, self.section, self.number)
	print """
	<INPUT TYPE=submit VALUE="Review Edit">
	<INPUT TYPE=hidden NAME=req VALUE=review>
	<INPUT TYPE=hidden NAME=name VALUE=%s>
	<INPUT TYPE=hidden NAME=version VALUE=%s>
	</FORM>
	<HR>
	""" % (name, version)
	self.show(name, title, text, edit=0)

    def do_review(self):
	if self.commit:
	    self.checkin()
	    return
	name = self.name
	text = self.text
	title = self.title
	headers, oldtext = self.read(name)
	if not headers:
	    self.error("Invalid file name", name)
	    return
	if self.author and '@' in self.email:
	    self.set_cookie(self.author, self.email)
	self.prologue("Python FAQ Review Form")
	print "<HR>"
	self.show(name, title, text, edit=0)
	print "<FORM METHOD=POST ACTION=faq.py>"
	if self.log and self.author and '@' in self.email:
	    print """
	    <INPUT TYPE=submit NAME=commit VALUE="Commit">
	    Click this button to commit the change.
	    <P>
	    <HR>
	    <P>
	    """
	else:
	    print """
	    To commit this change, please enter your name,
	    email and a log message in the form below.
	    <P>
	    <HR>
	    <P>
	    """
	self.showedit(name, title, text)
	if self.add:
	    print """
	    <INPUT TYPE=hidden NAME=add VALUE=%s>
	    <INPUT TYPE=hidden NAME=section VALUE=%s>
	    <INPUT TYPE=hidden NAME=number VALUE=%s>
	    """ % (self.add, self.section, self.number)
	print """
	<BR>
	<INPUT TYPE=submit VALUE="Review Edit">
	<INPUT TYPE=hidden NAME=req VALUE=review>
	<INPUT TYPE=hidden NAME=name VALUE=%s>
	<INPUT TYPE=hidden NAME=version VALUE=%s>
	</FORM>
	<HR>
	""" % (name, self.version)

    def do_info(self):
	name = self.name
	headers, text = self.read(name)
	if not headers:
	    self.error("Invalid file name", name)
	    return
	print '<PRE>'
	sys.stdout.flush()
	os.system("/depot/gnu/plat/bin/rlog -r %s </dev/null 2>&1" % self.name)
	print '</PRE>'
	print '<A HREF="faq.py?req=rlog&name=%s">View full rcs log</A>' % name

    def do_rlog(self):
	name = self.name
	headers, text = self.read(name)
	if not headers:
	    self.error("Invalid file name", name)
	    return
	print '<PRE>'
	sys.stdout.flush()
	os.system("/depot/gnu/plat/bin/rlog %s </dev/null 2>&1" % self.name)
	print '</PRE>'

    def checkin(self):
	import regsub, time, tempfile
	name = self.name

	headers, oldtext = self.read(name)
	if not headers:
	    self.error("Invalid file name", name)
	    return
	version = self.version
	curversion = self.getversion(name)
	if version != curversion:
	    self.error("Version conflict.",
		       "You edited version %s but current version is %s." % (
			   version, curversion),
		       '<A HREF="faq.py?req=show&name=%s">Reload.</A>' % name)
	    return
	text = self.text
	title = self.title
	author = self.author
	email = self.email
	log = self.log
	text = regsub.gsub("\r\n", "\n", text)
	log = regsub.gsub("\r\n", "\n", log)
	author = string.join(string.split(author))
	email = string.join(string.split(email))
	title = string.join(string.split(title))
	oldtitle = headers['title']
	oldtitle = string.join(string.split(oldtitle))
	text = string.strip(text)
	oldtext = string.strip(oldtext)
	if text == oldtext and title == oldtitle:
	    self.error("No changes.")
	    return
	# Check that the FAQ entry number didn't change
	if string.split(title)[:1] != string.split(oldtitle)[:1]:
	    self.error("Don't change the FAQ entry number please.")
	    return
	remhost = os.environ["REMOTE_HOST"]
	remaddr = os.environ["REMOTE_ADDR"]
	try:
	    os.unlink(name + "~")
	except os.error:
	    pass
	try:
	    os.rename(name, name + "~")
	except os.error:
	    pass
	try:
	    os.unlink(name)
	except os.error:
	    pass
	try:
	    f = open(name, "w")
	except IOError, msg:
	    self.error("Can't open", name, "for writing:", cgi.escape(str(msg)))
	    return
	now = time.ctime(time.time())
	f.write("Title: %s\n" % title)
	f.write("Last-Changed-Date: %s\n" % now)
	f.write("Last-Changed-Author: %s\n" % author)
	f.write("Last-Changed-Email: %s\n" % email)
	f.write("Last-Changed-Remote-Host: %s\n" % remhost)
	f.write("Last-Changed-Remote-Address: %s\n" % remaddr)
	keys = headers.keys()
	keys.sort()
	keys.remove('title')
	for key in keys:
	    if key[:13] != 'last-changed-':
		f.write("%s: %s\n" % (string.capwords(key, '-'),
				      headers[key]))
	f.write("\n")
	f.write(text)
	f.write("\n")
	f.close()

	tfn = tempfile.mktemp()
	f = open(tfn, "w")
	f.write("Last-Changed-Date: %s\n" % now)
	f.write("Last-Changed-Author: %s\n" % author)
	f.write("Last-Changed-Email: %s\n" % email)
	f.write("Last-Changed-Remote-Host: %s\n" % remhost)
	f.write("Last-Changed-Remote-Address: %s\n" % remaddr)
	f.write("\n")
	f.write(log)
	f.write("\n")
	f.close()
	
	p = os.popen("""
	/depot/gnu/plat/bin/rcs -l %s </dev/null 2>&1
	/depot/gnu/plat/bin/ci -u %s <%s 2>&1
	rm -f %s
	""" % (name, name, tfn, tfn))
	output = p.read()
	sts = p.close()
	if not sts:
	    self.set_cookie(author, email)
	    self.prologue("Python FAQ Entry Edited")
	    print "<HR>"
	    self.show(name, title, text)
	    if output:
		print "<PRE>%s</PRE>" % cgi.escape(output)
	else:
	    self.error("Python FAQ Entry Commit Failed",
		       "Exit status 0x%04x" % sts)
	    if output:
		print "<PRE>%s</PRE>" % cgi.escape(output)
	print '<HR>'
	print '<A HREF="faq.py?req=show&name=%s">Reload this entry.</A>' % name

    def set_cookie(self, author, email):
	name = "Python-FAQ-ID"
	value = "%s;%s" % (author, email)
	import urllib
	value = urllib.quote(value)
	print "Set-Cookie: %s=%s; path=/cgi-bin/;" % (name, value),
	print "domain=%s;" % os.environ['HTTP_HOST'],
	print "expires=Sat, 01-Jan-2000 00:00:00 GMT"

    def get_cookie(self):
	if not os.environ.has_key('HTTP_COOKIE'):
	    return "", ""
	raw = os.environ['HTTP_COOKIE']
	words = string.split(raw, ';')
	cookies = {}
	for word in words:
	    i = string.find(word, '=')
	    if i >= 0:
		key, value = word[:i], word[i+1:]
		cookies[key] = value
	if not cookies.has_key('Python-FAQ-ID'):
	    return "", ""
	value = cookies['Python-FAQ-ID']
	import urllib
	value = urllib.unquote(value)
	i = string.rfind(value, ';')
	author, email = value[:i], value[i+1:]
	return author, email

    def showedit(self, name, title, text):
	author = self.author
	email = self.email
	if not author or not email:
	    a, e = self.get_cookie()
	    author = author or a
	    email = email or e
	print """
	Title: <INPUT TYPE=text SIZE=70 NAME=title VALUE="%s"><BR>
	<TEXTAREA COLS=80 ROWS=20 NAME=text>""" % title
	print cgi.escape(string.strip(text))
	print """</TEXTAREA>
	<BR>
	Please provide the following information for logging purposes:
	<BR>
	<CODE>Name : </CODE><INPUT TYPE=text SIZE=40 NAME=author VALUE="%s">
	<BR>
	<CODE>Email: </CODE><INPUT TYPE=text SIZE=40 NAME=email VALUE="%s">
	<BR>
	Log message (reason for the change):<BR>
	<TEXTAREA COLS=80 ROWS=5 NAME=log>%s\n</TEXTAREA>
	""" % (author, email, self.log)

    def showheaders(self, headers):
	print "<UL>"
	keys = map(string.lower, headers.keys())
	keys.sort()
	for key in keys:
	    print "<LI><B>%s:</B> %s" % (string.capwords(key, '-'),
					 headers[key] or '')
	print "</UL>"

    headers = None

    def read(self, name):
	self.headers = None
	import fnmatch, rfc822
	if not fnmatch.fnmatch(name, NAMEPAT):
	    return None, None
	if self.add:
	    try:
		fname = "faq%02d.%03d.htp" % (string.atoi(self.section),
					      string.atoi(self.number))
	    except ValueError:
		return None, None
	    if fname != name:
		return None, None
	    headers = {'title': "%s.%s. " % (self.section, self.number)}
	    text = ""
	else:
	    f = open(name)
	    headers = rfc822.Message(f)
	    text = f.read()
	    f.close()
	self.headers = headers
	return headers, text

    def show(self, name, title, text, edit=1):
	# XXX Should put <A> tags around recognizable URLs
	# XXX Should also turn "see section N" into hyperlinks
	print "<H2>%s</H2>" % cgi.escape(title)
	pre = 0
	for line in string.split(text, '\n'):
	    if not string.strip(line):
		if pre:
		    print '</PRE>'
		    pre = 0
		else:
		    print '<P>'
	    else:
		if line == string.lstrip(line):	# I.e., no leading whitespace
		    if pre:
			print '</PRE>'
			pre = 0
		else:
		    if not pre:
			print '<PRE>'
			pre = 1
		print cgi.escape(line)
	if pre:
	    print '</PRE>'
	    pre = 0
	print '<P>'
	if edit:
	    print """
	    <A HREF="faq.py?req=edit&name=%s">Edit this entry</A> /
	    <A HREF="faq.py?req=info&name=%s" TARGET=_blank>Log info</A>
	    """ % (name, name)
	    if self.headers:
		try:
		    date = self.headers['last-changed-date']
		    author = self.headers['last-changed-author']
		    email = self.headers['last-changed-email']
		except KeyError:
		    pass
		else:
		    s = '/ Last changed on %s by <A HREF="%s">%s</A>'
		    print s % (date, email, author)
	    print '<P>'
	print "<HR>"

    def getversion(self, name):
	p = os.popen("/depot/gnu/plat/bin/rlog -h %s </dev/null 2>&1" % name)
	head = "*new*"
	while 1:
	    line = p.readline()
	    if not line:
		break
	    if line[:5] == 'head:':
		head = string.strip(line[5:])
	p.close()
	return head

    def prologue(self, title):
	title = cgi.escape(title)
	print '''
	<HTML>
	<HEAD>
	<TITLE>%s</TITLE>
	</HEAD>
	<BODY BACKGROUND="http://www.python.org/pics/RedShort.gif"
	      BGCOLOR="#FFFFFF"
	      TEXT="#000000"
	      LINK="#AA0000"
	      VLINK="#906A6A">
	<H1>%s</H1>
	''' % (title, title)

    def error(self, *messages):
	self.prologue("Python FAQ error")
	print "Sorry, an error occurred:<BR>"
	for message in messages:
	    print message,
	print

    def epilogue(self):
	print '''
	<P>
	<HR>
	<A HREF="http://www.python.org">Python home</A> /
	<A HREF="faq.py">FAQ home</A> /
	Feedback to <A HREF="mailto:guido@python.org">GvR</A>
	</BODY>
	</HTML>
	'''

print "Content-type: text/html"
dt = 0
try:
    import time
    t1 = time.time()
    import cgi, string, os, sys
    x = FAQServer()
    x.main()
    t2 = time.time()
    dt = t2-t1
except:
    print "\n<HR>Sorry, an error occurred"
    cgi.print_exception()
print "<P>(running time = %s seconds)" % str(round(dt, 3))

# The following bootstrap script must be placed in cgi-bin/faq.py:
BOOTSTRAP = """
#! /usr/local/bin/python
FAQDIR = "/usr/people/guido/python/FAQ"
import os, sys
os.chdir(FAQDIR)
sys.path.insert(0, os.curdir)
import faqmain
"""
