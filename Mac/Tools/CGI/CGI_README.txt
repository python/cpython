Python CGI under MacOS

This folder contains two tools that enable Python CGI scripts under
Mac based web servers, like WebStar, Quid Quo Pro, NetPresentz or
Apple's Personal Webserver.

Both tools emulate Unix style CGI's, allowing for cross platform
CGI scripts. In short, this happens by converting an AppleEvent sent
by the web server into os.environ dictionary entries. See below for more
details.

Both tools serve slightly different purposes:
- PythonCGISlave enables execution of Python scripts as plain *.py 
  text files. The web server must be configured to handle .py requests
  over to PythonCGISlave. Not all web servers support that. Eg. WebStar
  does, but NetPresentz does not.
- BuildCGIApplet wraps a Python CGI script in a compatibility layer, and
  creates a CGI Applet which can be executed by any web server.

The pros and cons of using PythonCGISlave are (+ is good, - is bad):
	+ support plain .py files, no need to wrap each script
	- not supported b all servers, requires more complicated configuration
The pros and cons of using BuildCGIApplet are:
	+ supported by more servers
	+ less configuration troubles
	- must wrap each script


Using BuildCGIApplet

Drop your CGI script onto BuildCGIApplet. An applet called <script name>.cgi
will be created. Move it to the appropriate location in the HTTP document tree.
Make sure your web server is configured to handle .cgi applet files. Usually
it is configured correctly by default, since .cgi is a standard extension.
If your CGI applet starts up for the first time, a file <applet name>.errors
is created. If your CGI script causes an exception, debug info will be written
to that file.


Using PythonCGISlave

Place the PythonCGISlave applet somewhere in the HTTP document tree. Configure
your web server so it'll pass requests for .py files to PythonCGISlave. For
Webstar, this goes roughly like this:
- in the WebStar Admin app, create a new "action", call it PYTHON, click the
  "Choose" button and select our applet. Save the settings.
- go to Suffix Mappings, create a new suffix .PY, type TEXT, creator *, and
  choose PYTHON in the actions popup. Save the settings.


How it works

For each Python CGI request, the web server will send an AppleEvent to the
CGI applet. Most relevant CGI parameters are taken from the AppleEvent and
get stuffed into the os.environ dictionary. Then the script gets executed.
This emulates Unix-style CGI as much as possible, so CGI scripts that are
written portably should now also work under a Mac web server.

Since the applet does not quit after each request by default, there is hardly
any startup overhead except the first time it starts up. If an exception occurs
in the CGI script,  the applet will write a traceback to a file called
<applet name>.errors, and then quit. The latter seems a good idea, just in case
we leak memory. The applet will be restarted upon the next request.


Please direct feedback to <just@letterror.com> and/or <pythonmac-sig@python.org>.
