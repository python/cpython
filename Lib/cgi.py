#! /usr/local/bin/python

"""Support module for CGI (Common Gateway Interface) scripts.

This module defines a number of utilities for use by CGI scripts
written in Python.


Introduction
------------

A CGI script is invoked by an HTTP server, usually to process user
input submitted through an HTML <FORM> or <ISINPUT> element.

Most often, CGI scripts live in the server's special cgi-bin
directory.  The HTTP server places all sorts of information about the
request (such as the client's hostname, the requested URL, the query
string, and lots of other goodies) in the script's shell environment,
executes the script, and sends the script's output back to the client.

The script's input is connected to the client too, and sometimes the
form data is read this way; at other times the form data is passed via
the "query string" part of the URL.  This module (cgi.py) is intended
to take care of the different cases and provide a simpler interface to
the Python script.  It also provides a number of utilities that help
in debugging scripts, and the latest addition is support for file
uploads from a form (if your browser supports it -- Grail 0.3 and
Netscape 2.0 do).

The output of a CGI script should consist of two sections, separated
by a blank line.  The first section contains a number of headers,
telling the client what kind of data is following.  Python code to
generate a minimal header section looks like this:

        print "Content-type: text/html" # HTML is following
        print                           # blank line, end of headers

The second section is usually HTML, which allows the client software
to display nicely formatted text with header, in-line images, etc.
Here's Python code that prints a simple piece of HTML:

        print "<TITLE>CGI script output</TITLE>"
        print "<H1>This is my first CGI script</H1>"
        print "Hello, world!"

It may not be fully legal HTML according to the letter of the
standard, but any browser will understand it.


Using the cgi module
--------------------

Begin by writing "import cgi".  Don't use "from cgi import *" -- the
module defines all sorts of names for its own use or for backward 
compatibility that you don't want in your namespace.

It's best to use the FieldStorage class.  The other classes define in this 
module are provided mostly for backward compatibility.  Instantiate it 
exactly once, without arguments.  This reads the form contents from 
standard input or the environment (depending on the value of various 
environment variables set according to the CGI standard).  Since it may 
consume standard input, it should be instantiated only once.

The FieldStorage instance can be accessed as if it were a Python 
dictionary.  For instance, the following code (which assumes that the 
Content-type header and blank line have already been printed) checks that 
the fields "name" and "addr" are both set to a non-empty string:

        form = cgi.FieldStorage()
        form_ok = 0
        if form.has_key("name") and form.has_key("addr"):
                if form["name"].value != "" and form["addr"].value != "":
                        form_ok = 1
        if not form_ok:
                print "<H1>Error</H1>"
                print "Please fill in the name and addr fields."
                return
        ...further form processing here...

Here the fields, accessed through form[key], are themselves instances
of FieldStorage (or MiniFieldStorage, depending on the form encoding).

If the submitted form data contains more than one field with the same
name, the object retrieved by form[key] is not a (Mini)FieldStorage
instance but a list of such instances.  If you are expecting this
possibility (i.e., when your HTML form comtains multiple fields with
the same name), use the type() function to determine whether you have
a single instance or a list of instances.  For example, here's code
that concatenates any number of username fields, separated by commas:

        username = form["username"]
        if type(username) is type([]):
                # Multiple username fields specified
                usernames = ""
                for item in username:
                        if usernames:
                                # Next item -- insert comma
                                usernames = usernames + "," + item.value
                        else:
                                # First item -- don't insert comma
                                usernames = item.value
        else:
                # Single username field specified
                usernames = username.value

If a field represents an uploaded file, the value attribute reads the 
entire file in memory as a string.  This may not be what you want.  You can 
test for an uploaded file by testing either the filename attribute or the 
file attribute.  You can then read the data at leasure from the file 
attribute:

        fileitem = form["userfile"]
        if fileitem.file:
                # It's an uploaded file; count lines
                linecount = 0
                while 1:
                        line = fileitem.file.readline()
                        if not line: break
                        linecount = linecount + 1

The file upload draft standard entertains the possibility of uploading
multiple files from one field (using a recursive multipart/*
encoding).  When this occurs, the item will be a dictionary-like
FieldStorage item.  This can be determined by testing its type
attribute, which should have the value "multipart/form-data" (or
perhaps another string beginning with "multipart/").  It this case, it
can be iterated over recursively just like the top-level form object.

When a form is submitted in the "old" format (as the query string or as a 
single data part of type application/x-www-form-urlencoded), the items 
will actually be instances of the class MiniFieldStorage.  In this case,
the list, file and filename attributes are always None.


Old classes
-----------

These classes, present in earlier versions of the cgi module, are still 
supported for backward compatibility.  New applications should use the
FieldStorage class.

SvFormContentDict: single value form content as dictionary; assumes each 
field name occurs in the form only once.

FormContentDict: multiple value form content as dictionary (the form
items are lists of values).  Useful if your form contains multiple
fields with the same name.

Other classes (FormContent, InterpFormContentDict) are present for
backwards compatibility with really old applications only.  If you still 
use these and would be inconvenienced when they disappeared from a next 
version of this module, drop me a note.


Functions
---------

These are useful if you want more control, or if you want to employ
some of the algorithms implemented in this module in other
circumstances.

parse(fp, [environ, [keep_blank_values, [strict_parsing]]]): parse a
form into a Python dictionary.

parse_qs(qs, [keep_blank_values, [strict_parsing]]): parse a query
string (data of type application/x-www-form-urlencoded).

parse_multipart(fp, pdict): parse input of type multipart/form-data (for 
file uploads).

parse_header(string): parse a header like Content-type into a main
value and a dictionary of parameters.

test(): complete test program.

print_environ(): format the shell environment in HTML.

print_form(form): format a form in HTML.

print_environ_usage(): print a list of useful environment variables in
HTML.

escape(): convert the characters "&", "<" and ">" to HTML-safe
sequences.  Use this if you need to display text that might contain
such characters in HTML.  To translate URLs for inclusion in the HREF
attribute of an <A> tag, use urllib.quote().

log(fmt, ...): write a line to a log file; see docs for initlog().


Caring about security
---------------------

There's one important rule: if you invoke an external program (e.g.
via the os.system() or os.popen() functions), make very sure you don't
pass arbitrary strings received from the client to the shell.  This is
a well-known security hole whereby clever hackers anywhere on the web
can exploit a gullible CGI script to invoke arbitrary shell commands.
Even parts of the URL or field names cannot be trusted, since the
request doesn't have to come from your form!

To be on the safe side, if you must pass a string gotten from a form
to a shell command, you should make sure the string contains only
alphanumeric characters, dashes, underscores, and periods.


Installing your CGI script on a Unix system
-------------------------------------------

Read the documentation for your HTTP server and check with your local
system administrator to find the directory where CGI scripts should be
installed; usually this is in a directory cgi-bin in the server tree.

Make sure that your script is readable and executable by "others"; the
Unix file mode should be 755 (use "chmod 755 filename").  Make sure
that the first line of the script contains #! starting in column 1
followed by the pathname of the Python interpreter, for instance:

        #! /usr/local/bin/python

Make sure the Python interpreter exists and is executable by "others".

Note that it's probably not a good idea to use #! /usr/bin/env python
here, since the Python interpreter may not be on the default path
given to CGI scripts!!!

Make sure that any files your script needs to read or write are
readable or writable, respectively, by "others" -- their mode should
be 644 for readable and 666 for writable.  This is because, for
security reasons, the HTTP server executes your script as user
"nobody", without any special privileges.  It can only read (write,
execute) files that everybody can read (write, execute).  The current
directory at execution time is also different (it is usually the
server's cgi-bin directory) and the set of environment variables is
also different from what you get at login.  in particular, don't count
on the shell's search path for executables ($PATH) or the Python
module search path ($PYTHONPATH) to be set to anything interesting.

If you need to load modules from a directory which is not on Python's
default module search path, you can change the path in your script,
before importing other modules, e.g.:

        import sys
        sys.path.insert(0, "/usr/home/joe/lib/python")
        sys.path.insert(0, "/usr/local/lib/python")

This way, the directory inserted last will be searched first!

Instructions for non-Unix systems will vary; check your HTTP server's
documentation (it will usually have a section on CGI scripts).


Testing your CGI script
-----------------------

Unfortunately, a CGI script will generally not run when you try it
from the command line, and a script that works perfectly from the
command line may fail mysteriously when run from the server.  There's
one reason why you should still test your script from the command
line: if it contains a syntax error, the python interpreter won't
execute it at all, and the HTTP server will most likely send a cryptic
error to the client.

Assuming your script has no syntax errors, yet it does not work, you
have no choice but to read the next section:


Debugging CGI scripts
---------------------

First of all, check for trivial installation errors -- reading the
section above on installing your CGI script carefully can save you a
lot of time.  If you wonder whether you have understood the
installation procedure correctly, try installing a copy of this module
file (cgi.py) as a CGI script.  When invoked as a script, the file
will dump its environment and the contents of the form in HTML form.
Give it the right mode etc, and send it a request.  If it's installed
in the standard cgi-bin directory, it should be possible to send it a
request by entering a URL into your browser of the form:

        http://yourhostname/cgi-bin/cgi.py?name=Joe+Blow&addr=At+Home

If this gives an error of type 404, the server cannot find the script
-- perhaps you need to install it in a different directory.  If it
gives another error (e.g.  500), there's an installation problem that
you should fix before trying to go any further.  If you get a nicely
formatted listing of the environment and form content (in this
example, the fields should be listed as "addr" with value "At Home"
and "name" with value "Joe Blow"), the cgi.py script has been
installed correctly.  If you follow the same procedure for your own
script, you should now be able to debug it.

The next step could be to call the cgi module's test() function from
your script: replace its main code with the single statement

        cgi.test()
        
This should produce the same results as those gotten from installing
the cgi.py file itself.

When an ordinary Python script raises an unhandled exception (e.g.,
because of a typo in a module name, a file that can't be opened,
etc.), the Python interpreter prints a nice traceback and exits.
While the Python interpreter will still do this when your CGI script
raises an exception, most likely the traceback will end up in one of
the HTTP server's log file, or be discarded altogether.

Fortunately, once you have managed to get your script to execute
*some* code, it is easy to catch exceptions and cause a traceback to
be printed.  The test() function below in this module is an example.
Here are the rules:

        1. Import the traceback module (before entering the
           try-except!)
        
        2. Make sure you finish printing the headers and the blank
           line early
        
        3. Assign sys.stderr to sys.stdout
        
        3. Wrap all remaining code in a try-except statement
        
        4. In the except clause, call traceback.print_exc()

For example:

        import sys
        import traceback
        print "Content-type: text/html"
        print
        sys.stderr = sys.stdout
        try:
                ...your code here...
        except:
                print "\n\n<PRE>"
                traceback.print_exc()

Notes: The assignment to sys.stderr is needed because the traceback
prints to sys.stderr.  The print "\n\n<PRE>" statement is necessary to
disable the word wrapping in HTML.

If you suspect that there may be a problem in importing the traceback
module, you can use an even more robust approach (which only uses
built-in modules):

        import sys
        sys.stderr = sys.stdout
        print "Content-type: text/plain"
        print
        ...your code here...

This relies on the Python interpreter to print the traceback.  The
content type of the output is set to plain text, which disables all
HTML processing.  If your script works, the raw HTML will be displayed
by your client.  If it raises an exception, most likely after the
first two lines have been printed, a traceback will be displayed.
Because no HTML interpretation is going on, the traceback will
readable.

When all else fails, you may want to insert calls to log() to your
program or even to a copy of the cgi.py file.  Note that this requires
you to set cgi.logfile to the name of a world-writable file before the
first call to log() is made!

Good luck!


Common problems and solutions
-----------------------------

- Most HTTP servers buffer the output from CGI scripts until the
script is completed.  This means that it is not possible to display a
progress report on the client's display while the script is running.

- Check the installation instructions above.

- Check the HTTP server's log files.  ("tail -f logfile" in a separate
window may be useful!)

- Always check a script for syntax errors first, by doing something
like "python script.py".

- When using any of the debugging techniques, don't forget to add
"import sys" to the top of the script.

- When invoking external programs, make sure they can be found.
Usually, this means using absolute path names -- $PATH is usually not
set to a very useful value in a CGI script.

- When reading or writing external files, make sure they can be read
or written by every user on the system.

- Don't try to give a CGI script a set-uid mode.  This doesn't work on
most systems, and is a security liability as well.


History
-------

Michael McLay started this module.  Steve Majewski changed the
interface to SvFormContentDict and FormContentDict.  The multipart
parsing was inspired by code submitted by Andreas Paepcke.  Guido van
Rossum rewrote, reformatted and documented the module and is currently
responsible for its maintenance.


XXX The module is getting pretty heavy with all those docstrings.
Perhaps there should be a slimmed version that doesn't contain all those 
backwards compatible and debugging classes and functions?

"""

__version__ = "2.2"


# Imports
# =======

import string
import sys
import os
import urllib
import mimetools
import rfc822
from StringIO import StringIO


# Logging support
# ===============

logfile = ""            # Filename to log to, if not empty
logfp = None            # File object to log to, if not None

def initlog(*allargs):
    """Write a log message, if there is a log file.

    Even though this function is called initlog(), you should always
    use log(); log is a variable that is set either to initlog
    (initially), to dolog (once the log file has been opened), or to
    nolog (when logging is disabled).

    The first argument is a format string; the remaining arguments (if
    any) are arguments to the % operator, so e.g.
        log("%s: %s", "a", "b")
    will write "a: b" to the log file, followed by a newline.

    If the global logfp is not None, it should be a file object to
    which log data is written.

    If the global logfp is None, the global logfile may be a string
    giving a filename to open, in append mode.  This file should be
    world writable!!!  If the file can't be opened, logging is
    silently disabled (since there is no safe place where we could
    send an error message).

    """
    global logfp, log
    if logfile and not logfp:
        try:
            logfp = open(logfile, "a")
        except IOError:
            pass
    if not logfp:
        log = nolog
    else:
        log = dolog
    apply(log, allargs)

def dolog(fmt, *args):
    """Write a log message to the log file.  See initlog() for docs."""
    logfp.write(fmt%args + "\n")

def nolog(*allargs):
    """Dummy function, assigned to log when logging is disabled."""
    pass

log = initlog           # The current logging function


# Parsing functions
# =================

# Maximum input we will accept when REQUEST_METHOD is POST
# 0 ==> unlimited input
maxlen = 0

def parse(fp=None, environ=os.environ, keep_blank_values=0, strict_parsing=0):
    """Parse a query in the environment or from a file (default stdin)

        Arguments, all optional:

        fp              : file pointer; default: sys.stdin

        environ         : environment dictionary; default: os.environ

        keep_blank_values: flag indicating whether blank values in
            URL encoded forms should be treated as blank strings.  
            A true value inicates that blanks should be retained as 
            blank strings.  The default false value indicates that
            blank values are to be ignored and treated as if they were
            not included.

        strict_parsing: flag indicating what to do with parsing errors.
            If false (the default), errors are silently ignored.
            If true, errors raise a ValueError exception.
    """
    if not fp:
        fp = sys.stdin
    if not environ.has_key('REQUEST_METHOD'):
        environ['REQUEST_METHOD'] = 'GET'       # For testing stand-alone
    if environ['REQUEST_METHOD'] == 'POST':
        ctype, pdict = parse_header(environ['CONTENT_TYPE'])
        if ctype == 'multipart/form-data':
            return parse_multipart(fp, pdict)
        elif ctype == 'application/x-www-form-urlencoded':
            clength = string.atoi(environ['CONTENT_LENGTH'])
            if maxlen and clength > maxlen:
                raise ValueError, 'Maximum content length exceeded'
            qs = fp.read(clength)
        else:
            qs = ''                     # Unknown content-type
        if environ.has_key('QUERY_STRING'): 
            if qs: qs = qs + '&'
            qs = qs + environ['QUERY_STRING']
        elif sys.argv[1:]: 
            if qs: qs = qs + '&'
            qs = qs + sys.argv[1]
        environ['QUERY_STRING'] = qs    # XXX Shouldn't, really
    elif environ.has_key('QUERY_STRING'):
        qs = environ['QUERY_STRING']
    else:
        if sys.argv[1:]:
            qs = sys.argv[1]
        else:
            qs = ""
        environ['QUERY_STRING'] = qs    # XXX Shouldn't, really
    return parse_qs(qs, keep_blank_values, strict_parsing)


def parse_qs(qs, keep_blank_values=0, strict_parsing=0):
    """Parse a query given as a string argument.

        Arguments:

        qs: URL-encoded query string to be parsed

        keep_blank_values: flag indicating whether blank values in
            URL encoded queries should be treated as blank strings.  
            A true value inicates that blanks should be retained as 
            blank strings.  The default false value indicates that
            blank values are to be ignored and treated as if they were
            not included.

        strict_parsing: flag indicating what to do with parsing errors.
            If false (the default), errors are silently ignored.
            If true, errors raise a ValueError exception.
    """
    name_value_pairs = string.splitfields(qs, '&')
    dict = {}
    for name_value in name_value_pairs:
        nv = string.splitfields(name_value, '=')
        if len(nv) != 2:
            if strict_parsing:
                raise ValueError, "bad query field: %s" % `name_value`
            continue
        name = urllib.unquote(string.replace(nv[0], '+', ' '))
        value = urllib.unquote(string.replace(nv[1], '+', ' '))
        if len(value) or keep_blank_values:
            if dict.has_key (name):
                dict[name].append(value)
            else:
                dict[name] = [value]
    return dict


def parse_multipart(fp, pdict):
    """Parse multipart input.

    Arguments:
    fp   : input file
    pdict: dictionary containing other parameters of conten-type header

    Returns a dictionary just like parse_qs(): keys are the field names, each 
    value is a list of values for that field.  This is easy to use but not 
    much good if you are expecting megabytes to be uploaded -- in that case, 
    use the FieldStorage class instead which is much more flexible.  Note 
    that content-type is the raw, unparsed contents of the content-type 
    header.
    
    XXX This does not parse nested multipart parts -- use FieldStorage for 
    that.
    
    XXX This should really be subsumed by FieldStorage altogether -- no 
    point in having two implementations of the same parsing algorithm.

    """
    if pdict.has_key('boundary'):
        boundary = pdict['boundary']
    else:
        boundary = ""
    nextpart = "--" + boundary
    lastpart = "--" + boundary + "--"
    partdict = {}
    terminator = ""

    while terminator != lastpart:
        bytes = -1
        data = None
        if terminator:
            # At start of next part.  Read headers first.
            headers = mimetools.Message(fp)
            clength = headers.getheader('content-length')
            if clength:
                try:
                    bytes = string.atoi(clength)
                except string.atoi_error:
                    pass
            if bytes > 0:
                if maxlen and bytes > maxlen:
                    raise ValueError, 'Maximum content length exceeded'
                data = fp.read(bytes)
            else:
                data = ""
        # Read lines until end of part.
        lines = []
        while 1:
            line = fp.readline()
            if not line:
                terminator = lastpart # End outer loop
                break
            if line[:2] == "--":
                terminator = string.strip(line)
                if terminator in (nextpart, lastpart):
                    break
            lines.append(line)
        # Done with part.
        if data is None:
            continue
        if bytes < 0:
            if lines:
                # Strip final line terminator
                line = lines[-1]
                if line[-2:] == "\r\n":
                    line = line[:-2]
                elif line[-1:] == "\n":
                    line = line[:-1]
                lines[-1] = line
                data = string.joinfields(lines, "")
        line = headers['content-disposition']
        if not line:
            continue
        key, params = parse_header(line)
        if key != 'form-data':
            continue
        if params.has_key('name'):
            name = params['name']
        else:
            continue
        if partdict.has_key(name):
            partdict[name].append(data)
        else:
            partdict[name] = [data]

    return partdict


def parse_header(line):
    """Parse a Content-type like header.

    Return the main content-type and a dictionary of options.

    """
    plist = map(string.strip, string.splitfields(line, ';'))
    key = string.lower(plist[0])
    del plist[0]
    pdict = {}
    for p in plist:
        i = string.find(p, '=')
        if i >= 0:
            name = string.lower(string.strip(p[:i]))
            value = string.strip(p[i+1:])
            if len(value) >= 2 and value[0] == value[-1] == '"':
                value = value[1:-1]
            pdict[name] = value
    return key, pdict


# Classes for field storage
# =========================

class MiniFieldStorage:

    """Like FieldStorage, for use when no file uploads are possible."""

    # Dummy attributes
    filename = None
    list = None
    type = None
    file = None
    type_options = {}
    disposition = None
    disposition_options = {}
    headers = {}

    def __init__(self, name, value):
        """Constructor from field name and value."""
        self.name = name
        self.value = value
        # self.file = StringIO(value)

    def __repr__(self):
        """Return printable representation."""
        return "MiniFieldStorage(%s, %s)" % (`self.name`, `self.value`)


class FieldStorage:

    """Store a sequence of fields, reading multipart/form-data.

    This class provides naming, typing, files stored on disk, and
    more.  At the top level, it is accessible like a dictionary, whose
    keys are the field names.  (Note: None can occur as a field name.)
    The items are either a Python list (if there's multiple values) or
    another FieldStorage or MiniFieldStorage object.  If it's a single
    object, it has the following attributes:

    name: the field name, if specified; otherwise None

    filename: the filename, if specified; otherwise None; this is the
        client side filename, *not* the file name on which it is
        stored (that's a temporary file you don't deal with)

    value: the value as a *string*; for file uploads, this
        transparently reads the file every time you request the value

    file: the file(-like) object from which you can read the data;
        None if the data is stored a simple string

    type: the content-type, or None if not specified

    type_options: dictionary of options specified on the content-type
        line

    disposition: content-disposition, or None if not specified

    disposition_options: dictionary of corresponding options

    headers: a dictionary(-like) object (sometimes rfc822.Message or a
        subclass thereof) containing *all* headers

    The class is subclassable, mostly for the purpose of overriding
    the make_file() method, which is called internally to come up with
    a file open for reading and writing.  This makes it possible to
    override the default choice of storing all files in a temporary
    directory and unlinking them as soon as they have been opened.

    """

    def __init__(self, fp=None, headers=None, outerboundary="",
                 environ=os.environ, keep_blank_values=0, strict_parsing=0):
        """Constructor.  Read multipart/* until last part.

        Arguments, all optional:

        fp              : file pointer; default: sys.stdin
            (not used when the request method is GET)

        headers         : header dictionary-like object; default:
            taken from environ as per CGI spec

        outerboundary   : terminating multipart boundary
            (for internal use only)

        environ         : environment dictionary; default: os.environ

        keep_blank_values: flag indicating whether blank values in
            URL encoded forms should be treated as blank strings.  
            A true value inicates that blanks should be retained as 
            blank strings.  The default false value indicates that
            blank values are to be ignored and treated as if they were
            not included.

        strict_parsing: flag indicating what to do with parsing errors.
            If false (the default), errors are silently ignored.
            If true, errors raise a ValueError exception.

        """
        method = 'GET'
        self.keep_blank_values = keep_blank_values
        self.strict_parsing = strict_parsing
        if environ.has_key('REQUEST_METHOD'):
            method = string.upper(environ['REQUEST_METHOD'])
        if method == 'GET':
            if environ.has_key('QUERY_STRING'):
                qs = environ['QUERY_STRING']
            elif sys.argv[1:]:
                qs = sys.argv[1]
            else:
                qs = ""
            fp = StringIO(qs)
            if headers is None:
                headers = {'content-type':
                           "application/x-www-form-urlencoded"}
        if headers is None:
            headers = {}
            if environ.has_key('CONTENT_TYPE'):
                headers['content-type'] = environ['CONTENT_TYPE']
            if environ.has_key('CONTENT_LENGTH'):
                headers['content-length'] = environ['CONTENT_LENGTH']
        self.fp = fp or sys.stdin
        self.headers = headers
        self.outerboundary = outerboundary

        # Process content-disposition header
        cdisp, pdict = "", {}
        if self.headers.has_key('content-disposition'):
            cdisp, pdict = parse_header(self.headers['content-disposition'])
        self.disposition = cdisp
        self.disposition_options = pdict
        self.name = None
        if pdict.has_key('name'):
            self.name = pdict['name']
        self.filename = None
        if pdict.has_key('filename'):
            self.filename = pdict['filename']

        # Process content-type header
        ctype, pdict = "text/plain", {}
        if self.headers.has_key('content-type'):
            ctype, pdict = parse_header(self.headers['content-type'])
        self.type = ctype
        self.type_options = pdict
        self.innerboundary = ""
        if pdict.has_key('boundary'):
            self.innerboundary = pdict['boundary']
        clen = -1
        if self.headers.has_key('content-length'):
            try:
                clen = string.atoi(self.headers['content-length'])
            except:
                pass
            if maxlen and clen > maxlen:
                raise ValueError, 'Maximum content length exceeded'
        self.length = clen

        self.list = self.file = None
        self.done = 0
        self.lines = []
        if ctype == 'application/x-www-form-urlencoded':
            self.read_urlencoded()
        elif ctype[:10] == 'multipart/':
            self.read_multi()
        else:
            self.read_single()

    def __repr__(self):
        """Return a printable representation."""
        return "FieldStorage(%s, %s, %s)" % (
                `self.name`, `self.filename`, `self.value`)

    def __getattr__(self, name):
        if name != 'value':
            raise AttributeError, name
        if self.file:
            self.file.seek(0)
            value = self.file.read()
            self.file.seek(0)
        elif self.list is not None:
            value = self.list
        else:
            value = None
        return value

    def __getitem__(self, key):
        """Dictionary style indexing."""
        if self.list is None:
            raise TypeError, "not indexable"
        found = []
        for item in self.list:
            if item.name == key: found.append(item)
        if not found:
            raise KeyError, key
        if len(found) == 1:
            return found[0]
        else:
            return found

    def keys(self):
        """Dictionary style keys() method."""
        if self.list is None:
            raise TypeError, "not indexable"
        keys = []
        for item in self.list:
            if item.name not in keys: keys.append(item.name)
        return keys

    def has_key(self, key):
        """Dictionary style has_key() method."""
        if self.list is None:
            raise TypeError, "not indexable"
        for item in self.list:
            if item.name == key: return 1
        return 0

    def __len__(self):
        """Dictionary style len(x) support."""
        return len(self.keys())

    def read_urlencoded(self):
        """Internal: read data in query string format."""
        qs = self.fp.read(self.length)
        dict = parse_qs(qs, self.keep_blank_values, self.strict_parsing)
        self.list = []
        for key, valuelist in dict.items():
            for value in valuelist:
                self.list.append(MiniFieldStorage(key, value))
        self.skip_lines()

    def read_multi(self):
        """Internal: read a part that is itself multipart."""
        self.list = []
        part = self.__class__(self.fp, {}, self.innerboundary)
        # Throw first part away
        while not part.done:
            headers = rfc822.Message(self.fp)
            part = self.__class__(self.fp, headers, self.innerboundary)
            self.list.append(part)
        self.skip_lines()

    def read_single(self):
        """Internal: read an atomic part."""
        if self.length >= 0:
            self.read_binary()
            self.skip_lines()
        else:
            self.read_lines()
        self.file.seek(0)

    bufsize = 8*1024            # I/O buffering size for copy to file

    def read_binary(self):
        """Internal: read binary data."""
        self.file = self.make_file('b')
        todo = self.length
        if todo >= 0:
            while todo > 0:
                data = self.fp.read(min(todo, self.bufsize))
                if not data:
                    self.done = -1
                    break
                self.file.write(data)
                todo = todo - len(data)

    def read_lines(self):
        """Internal: read lines until EOF or outerboundary."""
        self.file = self.make_file('')
        if self.outerboundary:
            self.read_lines_to_outerboundary()
        else:
            self.read_lines_to_eof()

    def read_lines_to_eof(self):
        """Internal: read lines until EOF."""
        while 1:
            line = self.fp.readline()
            if not line:
                self.done = -1
                break
            self.lines.append(line)
            self.file.write(line)

    def read_lines_to_outerboundary(self):
        """Internal: read lines until outerboundary."""
        next = "--" + self.outerboundary
        last = next + "--"
        delim = ""
        while 1:
            line = self.fp.readline()
            if not line:
                self.done = -1
                break
            self.lines.append(line)
            if line[:2] == "--":
                strippedline = string.strip(line)
                if strippedline == next:
                    break
                if strippedline == last:
                    self.done = 1
                    break
            odelim = delim
            if line[-2:] == "\r\n":
                delim = "\r\n"
                line = line[:-2]
            elif line[-1] == "\n":
                delim = "\n"
                line = line[:-1]
            else:
                delim = ""
            self.file.write(odelim + line)

    def skip_lines(self):
        """Internal: skip lines until outer boundary if defined."""
        if not self.outerboundary or self.done:
            return
        next = "--" + self.outerboundary
        last = next + "--"
        while 1:
            line = self.fp.readline()
            if not line:
                self.done = -1
                break
            self.lines.append(line)
            if line[:2] == "--":
                strippedline = string.strip(line)
                if strippedline == next:
                    break
                if strippedline == last:
                    self.done = 1
                    break

    def make_file(self, binary=None):
        """Overridable: return a readable & writable file.

        The file will be used as follows:
        - data is written to it
        - seek(0)
        - data is read from it

        The 'binary' argument is unused -- the file is always opened
        in binary mode.

        This version opens a temporary file for reading and writing,
        and immediately deletes (unlinks) it.  The trick (on Unix!) is
        that the file can still be used, but it can't be opened by
        another process, and it will automatically be deleted when it
        is closed or when the current process terminates.

        If you want a more permanent file, you derive a class which
        overrides this method.  If you want a visible temporary file
        that is nevertheless automatically deleted when the script
        terminates, try defining a __del__ method in a derived class
        which unlinks the temporary files you have created.

        """
        import tempfile
        return tempfile.TemporaryFile("w+b")
        


# Backwards Compatibility Classes
# ===============================

class FormContentDict:
    """Basic (multiple values per field) form content as dictionary.

    form = FormContentDict()

    form[key] -> [value, value, ...]
    form.has_key(key) -> Boolean
    form.keys() -> [key, key, ...]
    form.values() -> [[val, val, ...], [val, val, ...], ...]
    form.items() ->  [(key, [val, val, ...]), (key, [val, val, ...]), ...]
    form.dict == {key: [val, val, ...], ...}

    """
    def __init__(self, environ=os.environ):
        self.dict = parse(environ=environ)
        self.query_string = environ['QUERY_STRING']
    def __getitem__(self,key):
        return self.dict[key]
    def keys(self):
        return self.dict.keys()
    def has_key(self, key):
        return self.dict.has_key(key)
    def values(self):
        return self.dict.values()
    def items(self):
        return self.dict.items() 
    def __len__( self ):
        return len(self.dict)


class SvFormContentDict(FormContentDict):
    """Strict single-value expecting form content as dictionary.

    IF you only expect a single value for each field, then form[key]
    will return that single value.  It will raise an IndexError if
    that expectation is not true.  IF you expect a field to have
    possible multiple values, than you can use form.getlist(key) to
    get all of the values.  values() and items() are a compromise:
    they return single strings where there is a single value, and
    lists of strings otherwise.

    """
    def __getitem__(self, key):
        if len(self.dict[key]) > 1: 
            raise IndexError, 'expecting a single value' 
        return self.dict[key][0]
    def getlist(self, key):
        return self.dict[key]
    def values(self):
        lis = []
        for each in self.dict.values(): 
            if len( each ) == 1 : 
                lis.append(each[0])
            else: lis.append(each)
        return lis
    def items(self):
        lis = []
        for key,value in self.dict.items():
            if len(value) == 1 :
                lis.append((key, value[0]))
            else:       lis.append((key, value))
        return lis


class InterpFormContentDict(SvFormContentDict):
    """This class is present for backwards compatibility only.""" 
    def __getitem__( self, key ):
        v = SvFormContentDict.__getitem__( self, key )
        if v[0] in string.digits+'+-.' : 
            try:  return  string.atoi( v ) 
            except ValueError:
                try:    return string.atof( v )
                except ValueError: pass
        return string.strip(v)
    def values( self ):
        lis = [] 
        for key in self.keys():
            try:
                lis.append( self[key] )
            except IndexError:
                lis.append( self.dict[key] )
        return lis
    def items( self ):
        lis = [] 
        for key in self.keys():
            try:
                lis.append( (key, self[key]) )
            except IndexError:
                lis.append( (key, self.dict[key]) )
        return lis


class FormContent(FormContentDict):
    """This class is present for backwards compatibility only.""" 
    def values(self, key):
        if self.dict.has_key(key) :return self.dict[key]
        else: return None
    def indexed_value(self, key, location):
        if self.dict.has_key(key):
            if len (self.dict[key]) > location:
                return self.dict[key][location]
            else: return None
        else: return None
    def value(self, key):
        if self.dict.has_key(key): return self.dict[key][0]
        else: return None
    def length(self, key):
        return len(self.dict[key])
    def stripped(self, key):
        if self.dict.has_key(key): return string.strip(self.dict[key][0])
        else: return None
    def pars(self):
        return self.dict


# Test/debug code
# ===============

def test(environ=os.environ):
    """Robust test CGI script, usable as main program.

    Write minimal HTTP headers and dump all information provided to
    the script in HTML form.

    """
    import traceback
    print "Content-type: text/html"
    print
    sys.stderr = sys.stdout
    try:
        form = FieldStorage()   # Replace with other classes to test those
        print_form(form)
        print_environ(environ)
        print_directory()
        print_arguments()
        print_environ_usage()
        def f():
            exec "testing print_exception() -- <I>italics?</I>"
        def g(f=f):
            f()
        print "<H3>What follows is a test, not an actual exception:</H3>"
        g()
    except:
        print_exception()

    # Second try with a small maxlen...
    global maxlen
    maxlen = 50
    try:
        form = FieldStorage()   # Replace with other classes to test those
        print_form(form)
        print_environ(environ)
        print_directory()
        print_arguments()
        print_environ_usage()
    except:
        print_exception()

def print_exception(type=None, value=None, tb=None, limit=None):
    if type is None:
        type, value, tb = sys.exc_info()
    import traceback
    print
    print "<H3>Traceback (innermost last):</H3>"
    list = traceback.format_tb(tb, limit) + \
           traceback.format_exception_only(type, value)
    print "<PRE>%s<B>%s</B></PRE>" % (
        escape(string.join(list[:-1], "")),
        escape(list[-1]),
        )
    del tb

def print_environ(environ=os.environ):
    """Dump the shell environment as HTML."""
    keys = environ.keys()
    keys.sort()
    print
    print "<H3>Shell Environment:</H3>"
    print "<DL>"
    for key in keys:
        print "<DT>", escape(key), "<DD>", escape(environ[key])
    print "</DL>" 
    print

def print_form(form):
    """Dump the contents of a form as HTML."""
    keys = form.keys()
    keys.sort()
    print
    print "<H3>Form Contents:</H3>"
    print "<DL>"
    for key in keys:
        print "<DT>" + escape(key) + ":",
        value = form[key]
        print "<i>" + escape(`type(value)`) + "</i>"
        print "<DD>" + escape(`value`)
    print "</DL>"
    print

def print_directory():
    """Dump the current directory as HTML."""
    print
    print "<H3>Current Working Directory:</H3>"
    try:
        pwd = os.getcwd()
    except os.error, msg:
        print "os.error:", escape(str(msg))
    else:
        print escape(pwd)
    print

def print_arguments():
    print
    print "<H3>Command Line Arguments:</H3>"
    print
    print sys.argv
    print

def print_environ_usage():
    """Dump a list of environment variables used by CGI as HTML."""
    print """
<H3>These environment variables could have been set:</H3>
<UL>
<LI>AUTH_TYPE
<LI>CONTENT_LENGTH
<LI>CONTENT_TYPE
<LI>DATE_GMT
<LI>DATE_LOCAL
<LI>DOCUMENT_NAME
<LI>DOCUMENT_ROOT
<LI>DOCUMENT_URI
<LI>GATEWAY_INTERFACE
<LI>LAST_MODIFIED
<LI>PATH
<LI>PATH_INFO
<LI>PATH_TRANSLATED
<LI>QUERY_STRING
<LI>REMOTE_ADDR
<LI>REMOTE_HOST
<LI>REMOTE_IDENT
<LI>REMOTE_USER
<LI>REQUEST_METHOD
<LI>SCRIPT_NAME
<LI>SERVER_NAME
<LI>SERVER_PORT
<LI>SERVER_PROTOCOL
<LI>SERVER_ROOT
<LI>SERVER_SOFTWARE
</UL>
In addition, HTTP headers sent by the server may be passed in the
environment as well.  Here are some common variable names:
<UL>
<LI>HTTP_ACCEPT
<LI>HTTP_CONNECTION
<LI>HTTP_HOST
<LI>HTTP_PRAGMA
<LI>HTTP_REFERER
<LI>HTTP_USER_AGENT
</UL>
"""


# Utilities
# =========

def escape(s, quote=None):
    """Replace special characters '&', '<' and '>' by SGML entities."""
    s = string.replace(s, "&", "&amp;") # Must be done first!
    s = string.replace(s, "<", "&lt;")
    s = string.replace(s, ">", "&gt;",)
    if quote:
        s = string.replace(s, '"', "&quot;")
    return s


# Invoke mainline
# ===============

# Call test() when this file is run as a script (not imported as a module)
if __name__ == '__main__': 
    test()
