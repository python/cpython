# Miscellaneous customization constants
PASSWORD = "Spam"			# Edit password.  Change this!
FAQCGI = 'faqw.py'			# Relative URL of the FAQ cgi script
FAQNAME = "Python FAQ"			# Name of the FAQ
OWNERNAME = "GvR"			# Name for feedback
OWNEREMAIL = "guido@python.org"		# Email for feedback
HOMEURL = "http://www.python.org"	# Related home page
HOMENAME = "Python home"		# Name of related home page
MAXHITS = 10				# Max #hits to be shown directly
COOKIE_NAME = "Python-FAQ-Wizard"	# Name used for Netscape cookie
COOKIE_LIFETIME = 4 *7 * 24 * 3600	# Cookie expiration in seconds

# RCS commands
RCSBINDIR = "/depot/gnu/plat/bin/"	# Directory containing RCS commands
SH_RLOG = RCSBINDIR + "rlog %(file)s </dev/null 2>&1"
SH_RLOG_H = RCSBINDIR + "rlog -h %(file)s </dev/null 2>&1"
SH_RDIFF = RCSBINDIR + "rcsdiff -r%(prev)s -r%(rev)s %(file)s </dev/null 2>&1"
SH_LOCK = RCSBINDIR + "rcs -l %(file)s </dev/null 2>&1"
SH_CHECKIN =  RCSBINDIR + "ci -u %(file)s <%(tfn)s 2>&1"

# Titles for various output pages
T_HOME = FAQNAME + " Wizard 0.2 (alpha)"
T_ERROR = "Sorry, an error occurred"
T_ROULETTE = FAQNAME + " Roulette"
T_ALL = "The Whole " + FAQNAME
T_INDEX = FAQNAME + " Index"
T_SEARCH = FAQNAME + " Search Results"
T_RECENT = "Recently Changed %s Entries" % FAQNAME
T_SHOW = FAQNAME + " Entry"
T_LOG = "RCS log for %s entry" % FAQNAME
T_DIFF = "RCS diff for %s entry" % FAQNAME
T_ADD = "How to add an entry to the " + FAQNAME
T_DELETE = "How to delete an entry from the " + FAQNAME
T_EDIT = FAQNAME + " Edit Wizard"
T_REVIEW = T_EDIT + " - Review Changes"
T_COMMITTED = T_EDIT + " - Changes Committed"
T_COMMITFAILED = T_EDIT + " - Commit Failed"
T_CANTCOMMIT = T_EDIT + " - Commit Rejected"
T_HELP = T_EDIT + " - Help"

# Titles of FAQ sections
SECTION_TITLES = {
    1: "General information and availability",
    2: "Python in the real world",
    3: "Building Python and Other Known Bugs",
    4: "Programming in Python",
    5: "Extending Python",
    6: "Python's design",
    7: "Using Python on non-UNIX platforms",
}

# Generic prologue and epilogue

PROLOGUE = '''
<HTML>
<HEAD>
<TITLE>%(title)s</TITLE>
</HEAD>

<BODY BACKGROUND="http://www.python.org/pics/RedShort.gif"
      BGCOLOR="#FFFFFF"
      TEXT="#000000"
      LINK="#AA0000"
      VLINK="#906A6A">
<H1>%(title)s</H1>
'''

EPILOGUE = '''
<HR>
<A HREF="%(HOMEURL)s">%(HOMENAME)s</A> /
<A HREF="%(FAQCGI)s?req=home">%(FAQNAME)s Wizard</A> /
Feedback to <A HREF="mailto:%(OWNEREMAIL)s">%(OWNERNAME)s</A>

</BODY>
</HTML>
'''

# Home page

HOME = """
<FORM ACTION="%(FAQCGI)s">
    <INPUT TYPE=text NAME=query>
    <INPUT TYPE=submit VALUE="Search"><BR>
    (Case insensitive regular expressions.)
    <INPUT TYPE=hidden NAME=req VALUE=search>
</FORM>

<UL>
<LI><A HREF="%(FAQCGI)s?req=index">FAQ index</A>
<LI><A HREF="%(FAQCGI)s?req=all">The whole FAQ</A>
<LI><A HREF="%(FAQCGI)s?req=recent">Recently changed FAQ entries</A>
<LI><A HREF="%(FAQCGI)s?req=roulette">FAQ roulette</A>
</UL>
"""

# Index formatting

INDEX_SECTION = """
<P>
<HR>
<H2>%(sec)d. %(title)s</H2>
<UL>
"""

INDEX_ENDSECTION = """
</UL>
"""

INDEX_ENTRY = """\
<LI><A HREF="%(FAQCGI)s?req=show&file=%(file)s">%(title)s</A><BR>
"""

# Entry formatting

ENTRY_FOOTER = """
<A HREF="%(FAQCGI)s?req=edit&file=%(file)s">Edit this entry</A> /
<A HREF="%(FAQCGI)s?req=log&file=%(file)s">Log info</A>
"""

ENTRY_LOGINFO = """
/ Last changed on %(last_changed_date)s by
<A HREF="mailto:%(last_changed_email)s">%(last_changed_author)s</A>
"""

# Search

NO_HITS = """
No hits.
"""

ONE_HIT = """
Your search matched the following entry:
"""

FEW_HITS = """
Your search matched the following %(count)d entries:
"""

MANY_HITS = """
Your search matched more than %(MAXHITS)d entries.
The %(count)d matching entries are presented here ordered by section:
"""

# RCS log and diff

LOG = """
Click on a revision line to see the diff between that revision and the
previous one.
"""

DIFFLINK = """\
<A HREF="%(FAQCGI)s?req=diff&file=%(file)s&rev=%(rev)s">%(line)s</A>
"""

# Recently changed entries

NO_RECENT = """
<HR>
No %(FAQNAME)s entries were changed in the last %(period)s.
"""

ONE_RECENT = """
<HR>
View entries changed in the last:
<UL>
<LI><A HREF="%(FAQCGI)s?req=recent&days=1">24 hours</A>
<LI><A HREF="%(FAQCGI)s?req=recent&days=2">2 days</A>
<LI><A HREF="%(FAQCGI)s?req=recent&days=3">3 days</A>
<LI><A HREF="%(FAQCGI)s?req=recent&days=7">week</A>
<LI><A HREF="%(FAQCGI)s?req=recent&days=28">4 weeks</A>
<LI><A HREF="%(FAQCGI)s?req=recent&days=365250">millennium</A>
</UL>
The following %(FAQNAME)s entry was changed in the last %(period)s:
"""

SOME_RECENT = """
<HR>
View entries changed in the last:
<UL>
<LI><A HREF="%(FAQCGI)s?req=recent&days=1">24 hours</A>
<LI><A HREF="%(FAQCGI)s?req=recent&days=2">2 days</A>
<LI><A HREF="%(FAQCGI)s?req=recent&days=3">3 days</A>
<LI><A HREF="%(FAQCGI)s?req=recent&days=7">week</A>
<LI><A HREF="%(FAQCGI)s?req=recent&days=28">4 weeks</A>
<LI><A HREF="%(FAQCGI)s?req=recent&days=365250">millennium</A>
</UL>
The following %(count)d %(FAQNAME)s entries were changed
in the last %(period)s, most recently changed shown first:
"""

TAIL_RECENT = """
<HR>
View entries changed in the last:
<UL>
<LI><A HREF="%(FAQCGI)s?req=recent&days=1">24 hours</A>
<LI><A HREF="%(FAQCGI)s?req=recent&days=2">2 days</A>
<LI><A HREF="%(FAQCGI)s?req=recent&days=3">3 days</A>
<LI><A HREF="%(FAQCGI)s?req=recent&days=7">week</A>
<LI><A HREF="%(FAQCGI)s?req=recent&days=28">4 weeks</A>
<LI><A HREF="%(FAQCGI)s?req=recent&days=365250">millennium</A>
</UL>
"""

# Last changed banner on "all" (strftime format)
LAST_CHANGED = "Last changed on %c %Z"

# "Compat" command prologue (no <BODY> tag)
COMPAT = """
<H1>The whole %(FAQNAME)s</H1>
"""

# Editing

EDITHEAD = """
<A HREF="%(FAQCGI)s?req=help">Click for Help</A>
"""

REVIEWHEAD = EDITHEAD


EDITFORM1 = """
<FORM ACTION="%(FAQCGI)s" METHOD=POST>
<INPUT TYPE=hidden NAME=req VALUE=review>
<INPUT TYPE=hidden NAME=file VALUE=%(file)s>
<INPUT TYPE=hidden NAME=editversion VALUE=%(editversion)s>
<HR>
"""

EDITFORM2 = """
Title: <INPUT TYPE=text SIZE=70 NAME=title VALUE="%(title)s"><BR>
<TEXTAREA COLS=72 ROWS=20 NAME=body>%(body)s
</TEXTAREA><BR>
Log message (reason for the change):<BR>
<TEXTAREA COLS=72 ROWS=5 NAME=log>%(log)s
</TEXTAREA><BR>
Please provide the following information for logging purposes:
<TABLE FRAME=none COLS=2>
    <TR>
	<TD>Name:
	<TD><INPUT TYPE=text SIZE=40 NAME=author VALUE="%(author)s">
    <TR>
	<TD>Email:
	<TD><INPUT TYPE=text SIZE=40 NAME=email VALUE="%(email)s">
    <TR>
	<TD>Password:
	<TD><INPUT TYPE=password SIZE=20 NAME=password VALUE="%(password)s">
</TABLE>

<INPUT TYPE=submit NAME=review VALUE="Preview Edit">
Click this button to preview your changes.
"""

EDITFORM3 = """
</FORM>
"""

COMMIT = """
<INPUT TYPE=submit NAME=commit VALUE="Commit">
Click this button to commit your changes.
<HR>
"""

NOCOMMIT = """
You can't commit your changes unless you enter a log message, your
name, email addres, and the correct password in the form below.
<HR>
"""

CANTCOMMIT_HEAD = """
Some required information is missing:
<UL>
"""
NEED_PASSWD = "<LI>You must provide the correct passwd.\n"
NEED_AUTHOR = "<LI>You must enter your name.\n"
NEED_EMAIL = "<LI>You must enter your email address.\n"
NEED_LOG = "<LI>You must enter a log message.\n"
CANTCOMMIT_TAIL = """
</UL>
Please use your browser's Back command to correct the form and commit
again.
"""

VERSIONCONFLICT = """
<P>
You edited version %(editversion)s but the current version is %(version)s.
<P>
The two most common causes of this problem are:
<UL>
<LI>After committing a change, you went back in your browser,
    edited the entry some more, and clicked Commit again.
<LI>Someone else started editing the same entry and committed
    before you did.
</UL>
<P>
<A HREF="%(FAQCGI)s?req=show&file=%(file)s">Click here to reload the entry
and try again.</A>
<P>
"""

CANTWRITE = """
Can't write file %(file)s (%(why)s).
"""

FILEHEADER = """\
Title: %(title)s
Last-Changed-Date: %(date)s
Last-Changed-Author: %(author)s
Last-Changed-Email: %(email)s
Last-Changed-Remote-Host: %(REMOTE_HOST)s
Last-Changed-Remote-Address: %(REMOTE_ADDR)s
"""

LOGHEADER = """\
Last-Changed-Date: %(date)s
Last-Changed-Author: %(author)s
Last-Changed-Email: %(email)s
Last-Changed-Remote-Host: %(REMOTE_HOST)s
Last-Changed-Remote-Address: %(REMOTE_ADDR)s

%(log)s
"""

COMMITTED = """
Your changes have been committed.
"""

COMMITFAILED = """
Exit status %(sts)04x.
"""

HELP = """
Using the %(FAQNAME)s Edit Wizard speaks mostly for itself.  Here are
some answers to questions you are likely to ask:

<P><HR>

<H2>I can review an entry but I can't commit it.</H2>

The commit button only appears if the following conditions are met:

<UL>

<LI>The Name field is not empty.

<LI>The Email field contains at least an @ character.

<LI>The Log message box is not empty.

<LI>The Password field contains the proper password.

</UL>

<P><HR>

<H2>What is the password?</H2>

At the moment, only PSA members will be told the password.  This is a
good time to join the PSA!  See <A
HREF="http://www.python.org/psa/">the PSA home page</A>.

<P><HR>

<H2>Can I use HTML in the FAQ entry?</H2>

No, but if you include a URL or an email address in the text it will
automatigally become an anchor of the right type.  Also, *word*
is made italic (but only for single alphabetic words).

<P><HR>

<H2>How do I delineate paragraphs?</H2>

Use blank lines to separate paragraphs.

<P><HR>

<H2>How do I enter example text?</H2>

Any line that begins with a space or tab is assumed to be part of
literal text.  Blocks of literal text delineated by blank lines are
placed inside &lt;PRE&gt;...&lt;/PRE&gt;.
"""
