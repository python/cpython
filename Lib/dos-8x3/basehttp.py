"""HTTP server base class.

Note: the class in this module doesn't implement any HTTP request; see
SimpleHTTPServer for simple implementations of GET, HEAD and POST
(including CGI scripts).

Contents:

- BaseHTTPRequestHandler: HTTP request handler base class
- test: test function

XXX To do:

- send server version
- log requests even later (to capture byte count)
- log user-agent header and other interesting goodies
- send error log to separate file
- are request names really case sensitive?

"""


# See also:
#
# HTTP Working Group                                        T. Berners-Lee
# INTERNET-DRAFT                                            R. T. Fielding
# <draft-ietf-http-v10-spec-00.txt>                     H. Frystyk Nielsen
# Expires September 8, 1995                                  March 8, 1995
#
# URL: http://www.ics.uci.edu/pub/ietf/http/draft-ietf-http-v10-spec-00.txt


# Log files
# ---------
# 
# Here's a quote from the NCSA httpd docs about log file format.
# 
# | The logfile format is as follows. Each line consists of: 
# | 
# | host rfc931 authuser [DD/Mon/YYYY:hh:mm:ss] "request" ddd bbbb 
# | 
# |        host: Either the DNS name or the IP number of the remote client 
# |        rfc931: Any information returned by identd for this person,
# |                - otherwise. 
# |        authuser: If user sent a userid for authentication, the user name,
# |                  - otherwise. 
# |        DD: Day 
# |        Mon: Month (calendar name) 
# |        YYYY: Year 
# |        hh: hour (24-hour format, the machine's timezone) 
# |        mm: minutes 
# |        ss: seconds 
# |        request: The first line of the HTTP request as sent by the client. 
# |        ddd: the status code returned by the server, - if not available. 
# |        bbbb: the total number of bytes sent,
# |              *not including the HTTP/1.0 header*, - if not available 
# | 
# | You can determine the name of the file accessed through request.
# 
# (Actually, the latter is only true if you know the server configuration
# at the time the request was made!)


__version__ = "0.2"


import sys
import time
import socket # For gethostbyaddr()
import string
import mimetools
import SocketServer

# Default error message
DEFAULT_ERROR_MESSAGE = """\
<head>
<title>Error response</title>
</head>
<body>
<h1>Error response</h1>
<p>Error code %(code)d.
<p>Message: %(message)s.
<p>Error code explanation: %(code)s = %(explain)s.
</body>
"""


class HTTPServer(SocketServer.TCPServer):

    allow_reuse_address = 1    # Seems to make sense in testing environment

    def server_bind(self):
        """Override server_bind to store the server name."""
        SocketServer.TCPServer.server_bind(self)
        host, port = self.socket.getsockname()
        self.server_name = socket.getfqdn(host)
        self.server_port = port


class BaseHTTPRequestHandler(SocketServer.StreamRequestHandler):

    """HTTP request handler base class.

    The following explanation of HTTP serves to guide you through the
    code as well as to expose any misunderstandings I may have about
    HTTP (so you don't need to read the code to figure out I'm wrong
    :-).

    HTTP (HyperText Transfer Protocol) is an extensible protocol on
    top of a reliable stream transport (e.g. TCP/IP).  The protocol
    recognizes three parts to a request:

    1. One line identifying the request type and path
    2. An optional set of RFC-822-style headers
    3. An optional data part

    The headers and data are separated by a blank line.

    The first line of the request has the form

    <command> <path> <version>

    where <command> is a (case-sensitive) keyword such as GET or POST,
    <path> is a string containing path information for the request,
    and <version> should be the string "HTTP/1.0".  <path> is encoded
    using the URL encoding scheme (using %xx to signify the ASCII
    character with hex code xx).

    The protocol is vague about whether lines are separated by LF
    characters or by CRLF pairs -- for compatibility with the widest
    range of clients, both should be accepted.  Similarly, whitespace
    in the request line should be treated sensibly (allowing multiple
    spaces between components and allowing trailing whitespace).

    Similarly, for output, lines ought to be separated by CRLF pairs
    but most clients grok LF characters just fine.

    If the first line of the request has the form

    <command> <path>

    (i.e. <version> is left out) then this is assumed to be an HTTP
    0.9 request; this form has no optional headers and data part and
    the reply consists of just the data.

    The reply form of the HTTP 1.0 protocol again has three parts:

    1. One line giving the response code
    2. An optional set of RFC-822-style headers
    3. The data

    Again, the headers and data are separated by a blank line.

    The response code line has the form

    <version> <responsecode> <responsestring>

    where <version> is the protocol version (always "HTTP/1.0"),
    <responsecode> is a 3-digit response code indicating success or
    failure of the request, and <responsestring> is an optional
    human-readable string explaining what the response code means.

    This server parses the request and the headers, and then calls a
    function specific to the request type (<command>).  Specifically,
    a request SPAM will be handled by a method do_SPAM().  If no
    such method exists the server sends an error response to the
    client.  If it exists, it is called with no arguments:

    do_SPAM()

    Note that the request name is case sensitive (i.e. SPAM and spam
    are different requests).

    The various request details are stored in instance variables:

    - client_address is the client IP address in the form (host,
    port);

    - command, path and version are the broken-down request line;

    - headers is an instance of mimetools.Message (or a derived
    class) containing the header information;

    - rfile is a file object open for reading positioned at the
    start of the optional input data part;

    - wfile is a file object open for writing.

    IT IS IMPORTANT TO ADHERE TO THE PROTOCOL FOR WRITING!

    The first thing to be written must be the response line.  Then
    follow 0 or more header lines, then a blank line, and then the
    actual data (if any).  The meaning of the header lines depends on
    the command executed by the server; in most cases, when data is
    returned, there should be at least one header line of the form

    Content-type: <type>/<subtype>

    where <type> and <subtype> should be registered MIME types,
    e.g. "text/html" or "text/plain".

    """

    # The Python system version, truncated to its first component.
    sys_version = "Python/" + string.split(sys.version)[0]

    # The server software version.  You may want to override this.
    # The format is multiple whitespace-separated strings,
    # where each string is of the form name[/version].
    server_version = "BaseHTTP/" + __version__

    def parse_request(self):
        """Parse a request (internal).

        The request should be stored in self.raw_request; the results
        are in self.command, self.path, self.request_version and
        self.headers.

        Return value is 1 for success, 0 for failure; on failure, an
        error is sent back.

        """
        self.request_version = version = "HTTP/0.9" # Default
        requestline = self.raw_requestline
        if requestline[-2:] == '\r\n':
            requestline = requestline[:-2]
        elif requestline[-1:] == '\n':
            requestline = requestline[:-1]
        self.requestline = requestline
        words = string.split(requestline)
        if len(words) == 3:
            [command, path, version] = words
            if version[:5] != 'HTTP/':
                self.send_error(400, "Bad request version (%s)" % `version`)
                return 0
        elif len(words) == 2:
            [command, path] = words
            if command != 'GET':
                self.send_error(400,
                                "Bad HTTP/0.9 request type (%s)" % `command`)
                return 0
        else:
            self.send_error(400, "Bad request syntax (%s)" % `requestline`)
            return 0
        self.command, self.path, self.request_version = command, path, version
        self.headers = self.MessageClass(self.rfile, 0)
        return 1

    def handle(self):
        """Handle a single HTTP request.

        You normally don't need to override this method; see the class
        __doc__ string for information on how to handle specific HTTP
        commands such as GET and POST.

        """

        self.raw_requestline = self.rfile.readline()
        if not self.parse_request(): # An error code has been sent, just exit
            return
        mname = 'do_' + self.command
        if not hasattr(self, mname):
            self.send_error(501, "Unsupported method (%s)" % `self.command`)
            return
        method = getattr(self, mname)
        method()

    def send_error(self, code, message=None):
        """Send and log an error reply.

        Arguments are the error code, and a detailed message.
        The detailed message defaults to the short entry matching the
        response code.

        This sends an error response (so it must be called before any
        output has been generated), logs the error, and finally sends
        a piece of HTML explaining the error to the user.

        """

        try:
            short, long = self.responses[code]
        except KeyError:
            short, long = '???', '???'
        if not message:
            message = short
        explain = long
        self.log_error("code %d, message %s", code, message)
        self.send_response(code, message)
        self.end_headers()
        self.wfile.write(self.error_message_format %
                         {'code': code,
                          'message': message,
                          'explain': explain})

    error_message_format = DEFAULT_ERROR_MESSAGE

    def send_response(self, code, message=None):
        """Send the response header and log the response code.

        Also send two standard headers with the server software
        version and the current date.

        """
        self.log_request(code)
        if message is None:
            if self.responses.has_key(code):
                message = self.responses[code][0]
            else:
                message = ''
        if self.request_version != 'HTTP/0.9':
            self.wfile.write("%s %s %s\r\n" %
                             (self.protocol_version, str(code), message))
        self.send_header('Server', self.version_string())
        self.send_header('Date', self.date_time_string())

    def send_header(self, keyword, value):
        """Send a MIME header."""
        if self.request_version != 'HTTP/0.9':
            self.wfile.write("%s: %s\r\n" % (keyword, value))

    def end_headers(self):
        """Send the blank line ending the MIME headers."""
        if self.request_version != 'HTTP/0.9':
            self.wfile.write("\r\n")

    def log_request(self, code='-', size='-'):
        """Log an accepted request.

        This is called by send_reponse().

        """

        self.log_message('"%s" %s %s',
                         self.requestline, str(code), str(size))

    def log_error(self, *args):
        """Log an error.

        This is called when a request cannot be fulfilled.  By
        default it passes the message on to log_message().

        Arguments are the same as for log_message().

        XXX This should go to the separate error log.

        """

        apply(self.log_message, args)

    def log_message(self, format, *args):
        """Log an arbitrary message.

        This is used by all other logging functions.  Override
        it if you have specific logging wishes.

        The first argument, FORMAT, is a format string for the
        message to be logged.  If the format string contains
        any % escapes requiring parameters, they should be
        specified as subsequent arguments (it's just like
        printf!).

        The client host and current date/time are prefixed to
        every message.

        """

        sys.stderr.write("%s - - [%s] %s\n" %
                         (self.address_string(),
                          self.log_date_time_string(),
                          format%args))

    def version_string(self):
        """Return the server software version string."""
        return self.server_version + ' ' + self.sys_version

    def date_time_string(self):
        """Return the current date and time formatted for a message header."""
        now = time.time()
        year, month, day, hh, mm, ss, wd, y, z = time.gmtime(now)
        s = "%s, %02d %3s %4d %02d:%02d:%02d GMT" % (
                self.weekdayname[wd],
                day, self.monthname[month], year,
                hh, mm, ss)
        return s

    def log_date_time_string(self):
        """Return the current time formatted for logging."""
        now = time.time()
        year, month, day, hh, mm, ss, x, y, z = time.localtime(now)
        s = "%02d/%3s/%04d %02d:%02d:%02d" % (
                day, self.monthname[month], year, hh, mm, ss)
        return s

    weekdayname = ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun']

    monthname = [None,
                 'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun',
                 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec']

    def address_string(self):
        """Return the client address formatted for logging.

        This version looks up the full hostname using gethostbyaddr(),
        and tries to find a name that contains at least one dot.

        """

        host, port = self.client_address
        return socket.getfqdn(host)

    # Essentially static class variables

    # The version of the HTTP protocol we support.
    # Don't override unless you know what you're doing (hint: incoming
    # requests are required to have exactly this version string).
    protocol_version = "HTTP/1.0"

    # The Message-like class used to parse headers
    MessageClass = mimetools.Message

    # Table mapping response codes to messages; entries have the
    # form {code: (shortmessage, longmessage)}.
    # See http://www.w3.org/hypertext/WWW/Protocols/HTTP/HTRESP.html
    responses = {
        200: ('OK', 'Request fulfilled, document follows'),
        201: ('Created', 'Document created, URL follows'),
        202: ('Accepted',
              'Request accepted, processing continues off-line'),
        203: ('Partial information', 'Request fulfilled from cache'),
        204: ('No response', 'Request fulfilled, nothing follows'),
        
        301: ('Moved', 'Object moved permanently -- see URI list'),
        302: ('Found', 'Object moved temporarily -- see URI list'),
        303: ('Method', 'Object moved -- see Method and URL list'),
        304: ('Not modified',
              'Document has not changed singe given time'),
        
        400: ('Bad request',
              'Bad request syntax or unsupported method'),
        401: ('Unauthorized',
              'No permission -- see authorization schemes'),
        402: ('Payment required',
              'No payment -- see charging schemes'),
        403: ('Forbidden',
              'Request forbidden -- authorization will not help'),
        404: ('Not found', 'Nothing matches the given URI'),
        
        500: ('Internal error', 'Server got itself in trouble'),
        501: ('Not implemented',
              'Server does not support this operation'),
        502: ('Service temporarily overloaded',
              'The server cannot process the request due to a high load'),
        503: ('Gateway timeout',
              'The gateway server did not receive a timely response'),
        
        }


def test(HandlerClass = BaseHTTPRequestHandler,
         ServerClass = HTTPServer):
    """Test the HTTP request handler class.

    This runs an HTTP server on port 8000 (or the first command line
    argument).

    """

    if sys.argv[1:]:
        port = string.atoi(sys.argv[1])
    else:
        port = 8000
    server_address = ('', port)

    httpd = ServerClass(server_address, HandlerClass)

    print "Serving HTTP on port", port, "..."
    httpd.serve_forever()


if __name__ == '__main__':
    test()
