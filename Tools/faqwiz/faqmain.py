#! /depot/sundry/plat/bin/python1.4

"""Interactive FAQ project.

XXX TO DO

- customize rcs command pathnames
- recognize urls and email addresses and turn them into <A> tags
- use cookies to keep Name/email the same
- explanation of editing somewhere
- various embellishments, GIFs, crosslinks, hints, etc.
- create new sections
- rearrange entries
- delete entries
- log changes
- send email on changes
- optional staging of entries until reviewed?
- review revision log and older versions
- freeze entries
- username/password for editors
- Change references to other Q's and whole sections
- Browse should display menu of 7 sections & let you pick
  (or frontpage should have the option to browse a section or all)
- support adding annotations, too

"""

import cgi, string, os, sys

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

    KEYS = ['req', 'query', 'name', 'text', 'commit', 'title',
	    'author', 'email', 'log', 'section', 'number', 'add']

    def __getattr__(self, key):
	if key not in self.KEYS:
	    raise AttributeError
	try:
	    value = self.form[key].value
	    setattr(self, key, value)
	    return value
	except KeyError:
	    return ''

    def do_frontpage(self):
	print """
	<TITLE>Python FAQ (alpha 1)</TITLE>
	<H1>Python FAQ Front Page</H1>
	<UL>
	<LI><A HREF="faq.py?req=index">FAQ index</A>
	<LI><A HREF="faq.py?req=all">The whole FAQ</A>
	<LI><A HREF="faq.py?req=roulette">FAQ roulette</A>
	<LI><A HREF="faq.py?req=recent">Recently changed FAQ entries</A>
	<LI><A HREF="faq.py?req=add">Add a new FAQ entry</A>
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
	print """
	<TITLE>Python FAQ Index</TITLE>
	<H1>Python FAQ Index</H1>
	"""
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
	name = self.name
	headers, text = self.read(name)
	if not headers:
	    print "Invalid file name", name
	    return
	self.show(name, headers['title'], text, 1)

    def do_all(self):
	print """
	<TITLE>Python FAQ</TITLE>
	<H1>Python FAQ</H1>
	<HR>
	"""
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
		self.show(name, title, text, 1)
	if not section:
	    print "No FAQ entries?!?!"

    def do_roulette(self):
	import whrandom
	print """
	<TITLE>Python FAQ Roulette</TITLE>
	<H1>Python FAQ Roulette</H1>
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
		self.show(name, headers['title'], text, 1)
		print '<P><A HREF="faq.py?req=roulette">Show another one</A>'
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
	print """
	<TITLE>Python FAQ, Most Recently Modified First</TITLE>
	<H1>Python FAQ, Most Recently Modified First</H1>
	<HR>
	"""
	n = 0
	for (mtime, name) in list:
	    headers, text = self.read(name)
	    if headers:
		self.show(name, headers['title'], text, 1)
		n = n+1
	if not n:
	    print "No FAQ entries?!?!"

    def do_query(self):
	import regex
	print "<TITLE>Python FAQ Query Results</TITLE>"
	print "<H1>Python FAQ Query Results</H1>"
	query = self.query
	if not query:
	    print "No query string"
	    return
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
		    self.show(name, title, text, 1)
		    n = n+1
	if not n:
	    print "No hits."

    def do_add(self):
	section = self.section
	if not section:
	    print """
	    <TITLE>How to add a new FAQ entry</TITLE>
	    <H1>How to add a new FAQ entry</H1>

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
	    print "Can't add new sections yet."
	    return
	num = max+1
	name = "faq%02d.%03d.htp" % (nsec, num)
	self.name = name
	self.add = "yes"
	self.number = str(num)
	self.do_edit()

    def do_edit(self):
	name = self.name
	headers, text = self.read(name)
	if not headers:
	    print "Invalid file name", name
	    return
	print """
	<TITLE>Python FAQ Edit Form</TITLE>
	<H1>Python FAQ Edit Form</H1>
	"""
	title = headers['title']
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
	</FORM>
	<HR>
	""" % name
	self.show(name, title, text)

    def do_review(self):
	if self.commit:
	    self.checkin()
	    return
	name = self.name
	text = self.text
	title = self.title
	headers, oldtext = self.read(name)
	if not headers:
	    print "Invalid file name", name
	    return
	print """
	<TITLE>Python FAQ Review Form</TITLE>
	<H1>Python FAQ Review Form</H1>
	<HR>
	"""
	self.show(name, title, text)
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
	</FORM>
	<HR>
	""" % name

    def do_info(self):
	name = self.name
	headers, text = self.read(name)
	if not headers:
	    print "Invalid file name", name
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
	    print "Invalid file name", name
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
	    print "Invalid file name", name
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
	    print "No changes."
	    # XXX Should exit more ceremoniously
	    return
	# Check that the FAQ entry number didn't change
	if string.split(title)[:1] != string.split(oldtitle)[:1]:
	    print "Don't change the FAQ entry number please."
	    # XXX Should exit more ceremoniously
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
	    print "Can't open", name, "for writing:", cgi.escape(str(msg))
	    # XXX Should exit more ceremoniously
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
	    print """
	    <TITLE>Python FAQ Entry Edited</TITLE>
	    <H1>Python FAQ Entry Edited</H1>
	    <HR>
	    """
	    self.show(name, title, text, 1)
	    if output:
		print "<PRE>%s</PRE>" % cgi.escape(output)
	else:
	    print """
	    <H1>Python FAQ Entry Commit Failed</H1>
	    Exit status 0x%04x
	    """ % sts
	    if output:
		print "<PRE>%s</PRE>" % cgi.escape(output)

    def showedit(self, name, title, text):
	print """
	Title: <INPUT TYPE=text SIZE=70 NAME=title VALUE="%s"<BR>
	<TEXTAREA COLS=80 ROWS=20 NAME=text>""" % title
	print cgi.escape(string.strip(text))
	print """</TEXTAREA>
	<BR>
	Please provide the following information for logging purposes:
	<BR>
	<CODE>Name : </CODE><INPUT TYPE=text SIZE=70 NAME=author VALUE="%s"<BR>
	<CODE>Email: </CODE><INPUT TYPE=text SIZE=70 NAME=email VALUE="%s"<BR>
	Log message (reason for the change):<BR>
	<TEXTAREA COLS=80 ROWS=5 NAME=log>\n%s\n</TEXTAREA>
	""" % (self.author, self.email, self.log)

    def showheaders(self, headers):
	print "<UL>"
	keys = map(string.lower, headers.keys())
	keys.sort()
	for key in keys:
	    print "<LI><B>%s:</B> %s" % (string.capwords(key, '-'),
					 headers[key] or '')
	print "</UL>"

    def read(self, name):
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
	    return headers, text
	f = open(name)
	headers = rfc822.Message(f)
	text = f.read()
	f.close()
	return headers, text

    def show(self, name, title, text, edit=0):
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
	    print '<P>'
	print "<HR>"


print "Content-type: text/html\n"
dt = 0
try:
    import time
    t1 = time.time()
    x = FAQServer()
    x.main()
    t2 = time.time()
    dt = t2-t1
except:
    print "<HR>Sorry, an error occurred"
    cgi.print_exception()
print "<P>(time = %s seconds)" % str(round(dt, 3))
