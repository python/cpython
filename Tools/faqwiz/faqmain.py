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

    KEYS = ['req', 'query', 'name', 'text', 'commit', 'title']

    def __getattr__(self, key):
	if key not in self.KEYS:
	    raise AttributeError
	if self.form.has_key(key):
	    item = self.form[key]
	    return item.value
	return ''

    def do_frontpage(self):
	print """
	<TITLE>Python FAQ (alpha 1)</TITLE>
	<H1>Python FAQ Front Page</H1>
	<UL>
	<LI><A HREF="faq.py?req=search">Search the FAQ</A>
	<LI><A HREF="faq.py?req=browse">Browse the FAQ</A>
	<LI><A HREF="faq.py?req=submit">Submit a new FAQ entry</A>
	<LI><A HREF="faq.py?req=roulette">Random FAQ entry</A>
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
	    headers, body = self.read(name)
	    if headers:
		self.show(name, headers, body, 1)
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
	    headers, body = self.read(name)
	    if headers:
		self.show(name, headers, body, 1)
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
	    headers, body = self.read(name)
	    if headers:
		title = headers['title']
		if p.search(title) >= 0 or p.search(body) >= 0:
		    self.show(name, headers, body, 1)
		    n = n+1
	if not n:
	    print "No hits."

    def do_edit(self):
	name = self.name
	headers, body = self.read(name)
	if not headers:
	    print "Invalid file name", name
	    return
	print """
	<TITLE>Python FAQ Edit Form</TITLE>
	<H1>Python FAQ Edit Form</H1>
	"""
	self.showheaders(headers)
	title = headers['title']
	print """
	<FORM METHOD=POST ACTION=faq.py>
	<INPUT TYPE=text SIZE=80 NAME=title VALUE="%s"<BR>
	<TEXTAREA COLS=80 ROWS=20 NAME=text>""" % title
	print cgi.escape(string.strip(body))
	print """</TEXTAREA>
	<BR>
	<INPUT TYPE=submit VALUE="Review Edit">
	<INPUT TYPE=hidden NAME=req VALUE=review>
	<INPUT TYPE=hidden NAME=name VALUE=%s>
	</FORM>
	<HR>
	""" % name
	self.show(name, headers, body)

    def do_review(self):
	name = self.name
	text = self.text
	commit = self.commit
	title = self.title
	if commit:
	    self.precheckin(name, text, title)
	    return
	headers, body = self.read(name)
	if not headers:
	    print "Invalid file name", name
	    return
	print """
	<TITLE>Python FAQ Review Form</TITLE>
	<H1>Python FAQ Review Form</H1>
	"""
	self.show(name, {'title': title}, text)
	print """
	<FORM METHOD=POST ACTION=faq.py>
	<INPUT TYPE=submit NAME=commit VALUE="Commit">
	<P>
	<HR>
	"""
	self.showheaders(headers)
	print """
	<INPUT TYPE=text SIZE=80 NAME=title VALUE="%s"<BR>
	<TEXTAREA COLS=80 ROWS=20 NAME=text>""" % title
	print cgi.escape(string.strip(text))
	print """</TEXTAREA>
	<BR>
	<INPUT TYPE=submit VALUE="Review Edit">
	<INPUT TYPE=hidden NAME=req VALUE=review>
	<INPUT TYPE=hidden NAME=name VALUE=%s>
	</FORM>
	<HR>
	""" % name

    def precheckin(self, name, text, title):
	pass

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
	body = f.read()
	f.close()
	return headers, body

    def show(self, name, headers, body, edit=0):
	# XXX Should put <A> tags around recognizable URLs
	# XXX Should also turn "see section N" into hyperlinks
	title = headers['title']
	print "<H2>%s</H2>" % title
	pre = 0
	for line in string.split(body, '\n'):
	    if not string.strip(line):
		if pre:
		    print '</PRE>'
		    pre = 0
		else:
		    print '<P>'
	    else:
		if line == string.lstrip(line):
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
try:
    x = FAQServer()
    x.main()
except:
    print "<HR>Sorry, an error occurred"
    cgi.print_exception()
