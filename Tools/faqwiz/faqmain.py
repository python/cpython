#! /depot/sundry/plat/bin/python1.4

"""Interactive FAQ project."""

import cgi, string, os

NAMEPAT = "faq??.???.htp"

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
	    'author', 'email', 'log']

    def __getattr__(self, key):
	if key not in self.KEYS:
	    raise AttributeError
	try:
	    item = self.form[key]
	    return item.value
	except KeyError:
	    return ''

    def do_frontpage(self):
	print """
	<TITLE>Python FAQ (alpha 1)</TITLE>
	<H1>Python FAQ Front Page</H1>
	<UL>
	<LI><A HREF="faq.py?req=search">Search the FAQ</A>
	<LI><A HREF="faq.py?req=browse">Browse the FAQ</A>
	<LI><A HREF="faq.py?req=submit">Submit a new FAQ entry</A> (not yet)
	<LI><A HREF="faq.py?req=roulette">FAQ roulette</A>
	</UL>

	<FORM ACTION="faq.py?req=query">
	<INPUT TYPE=text NAME=query>
	<INPUT TYPE=submit VALUE="Search">
	<INPUT TYPE=hidden NAME=req VALUE=query>
	</FORM>

	Disclaimer: these pages are intended to be edited by anyone.
	Please exercise discretion when editing, don't be rude, etc.
	"""

    def do_browse(self):
	print """
	<TITLE>Python FAQ</TITLE>
	<H1>Python FAQ</H1>
	<HR>
	"""
	names = os.listdir(os.curdir)
	names.sort()
	n = 0
	for name in names:
	    headers, text = self.read(name)
	    if headers:
		self.show(name, headers['title'], text, 1)
		n = n+1
	if not n:
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

    def do_search(self):
	print """
	<TITLE>Search the Python FAQ</TITLE>
	<H1>Search the Python FAQ</H1>

	<FORM ACTION="faq.py?req=query">
	<INPUT TYPE=text NAME=query>
	<INPUT TYPE=submit VALUE="Search">
	<INPUT TYPE=hidden NAME=req VALUE=query>
	</FORM>
	"""

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
	self.showedit(name, headers, text)
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
	self.showedit(name, headers, text)
	print """
	<BR>
	<INPUT TYPE=submit VALUE="Review Edit">
	<INPUT TYPE=hidden NAME=req VALUE=review>
	<INPUT TYPE=hidden NAME=name VALUE=%s>
	</FORM>
	<HR>
	""" % name

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
	# Check that the question number didn't change
	if string.split(title)[:1] != string.split(oldtitle)[:1]:
	    print "Don't change the question number please."
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

    def showedit(self, name, headers, text):
	title = headers['title']
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
	f = open(name)
	headers = rfc822.Message(f)
	text = f.read()
	f.close()
	return headers, text

    def show(self, name, title, text, edit=0):
	# XXX Should put <A> tags around recognizable URLs
	# XXX Should also turn "see section N" into hyperlinks
	print "<H2>%s</H2>" % title
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
		print line
	if pre:
	    print '</PRE>'
	    pre = 0
	print '<P>'
	if edit:
	    print '<A HREF="faq.py?req=edit&name=%s">Edit this entry</A>' %name
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
print "<!-- dt = %s -->" % str(round(dt, 3))
