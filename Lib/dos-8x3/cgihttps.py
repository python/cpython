"""CGI-savvy HTTP Server.

This module builds on SimpleHTTPServer by implementing GET and POST
requests to cgi-bin scripts.

"""


__version__ = "0.3"


import os
import sys
import time
import socket
import string
import urllib
import BaseHTTPServer
import SimpleHTTPServer


class CGIHTTPRequestHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):

    """Complete HTTP server with GET, HEAD and POST commands.

    GET and HEAD also support running CGI scripts.

    The POST command is *only* implemented for CGI scripts.

    """

    def do_POST(self):
        """Serve a POST request.

        This is only implemented for CGI scripts.

        """

        if self.is_cgi():
            self.run_cgi()
        else:
            self.send_error(501, "Can only POST to CGI scripts")

    def send_head(self):
        """Version of send_head that support CGI scripts"""
        if self.is_cgi():
            return self.run_cgi()
        else:
            return SimpleHTTPServer.SimpleHTTPRequestHandler.send_head(self)

    def is_cgi(self):
        """test whether PATH corresponds to a CGI script.

        Return a tuple (dir, rest) if PATH requires running a
        CGI script, None if not.  Note that rest begins with a
        slash if it is not empty.

        The default implementation tests whether the path
        begins with one of the strings in the list
        self.cgi_directories (and the next character is a '/'
        or the end of the string).

        """

        path = self.path

        for x in self.cgi_directories:
            i = len(x)
            if path[:i] == x and (not path[i:] or path[i] == '/'):
                self.cgi_info = path[:i], path[i+1:]
                return 1
        return 0

    cgi_directories = ['/cgi-bin', '/htbin']

    def run_cgi(self):
        """Execute a CGI script."""
        dir, rest = self.cgi_info
        i = string.rfind(rest, '?')
        if i >= 0:
            rest, query = rest[:i], rest[i+1:]
        else:
            query = ''
        i = string.find(rest, '/')
        if i >= 0:
            script, rest = rest[:i], rest[i:]
        else:
            script, rest = rest, ''
        scriptname = dir + '/' + script
        scriptfile = self.translate_path(scriptname)
        if not os.path.exists(scriptfile):
            self.send_error(404, "No such CGI script (%s)" % `scriptname`)
            return
        if not os.path.isfile(scriptfile):
            self.send_error(403, "CGI script is not a plain file (%s)" %
                            `scriptname`)
            return
        if not executable(scriptfile):
            self.send_error(403, "CGI script is not executable (%s)" %
                            `scriptname`)
            return
        nobody = nobody_uid()
        self.send_response(200, "Script output follows")
        self.wfile.flush() # Always flush before forking
        pid = os.fork()
        if pid != 0:
            # Parent
            pid, sts = os.waitpid(pid, 0)
            if sts:
                self.log_error("CGI script exit status x%x" % sts)
            return
        # Child
        try:
            # Reference: http://hoohoo.ncsa.uiuc.edu/cgi/env.html
            # XXX Much of the following could be prepared ahead of time!
            env = {}
            env['SERVER_SOFTWARE'] = self.version_string()
            env['SERVER_NAME'] = self.server.server_name
            env['GATEWAY_INTERFACE'] = 'CGI/1.1'
            env['SERVER_PROTOCOL'] = self.protocol_version
            env['SERVER_PORT'] = str(self.server.server_port)
            env['REQUEST_METHOD'] = self.command
            uqrest = urllib.unquote(rest)
            env['PATH_INFO'] = uqrest
            env['PATH_TRANSLATED'] = self.translate_path(uqrest)
            env['SCRIPT_NAME'] = scriptname
            if query:
                env['QUERY_STRING'] = query
            host = self.address_string()
            if host != self.client_address[0]:
                env['REMOTE_HOST'] = host
            env['REMOTE_ADDR'] = self.client_address[0]
            # AUTH_TYPE
            # REMOTE_USER
            # REMOTE_IDENT
            env['CONTENT_TYPE'] = self.headers.type
            length = self.headers.getheader('content-length')
            if length:
                env['CONTENT_LENGTH'] = length
            accept = []
            for line in self.headers.getallmatchingheaders('accept'):
                if line[:1] in string.whitespace:
                    accept.append(string.strip(line))
                else:
                    accept = accept + string.split(line[7:])
            env['HTTP_ACCEPT'] = string.joinfields(accept, ',')
            ua = self.headers.getheader('user-agent')
            if ua:
                env['HTTP_USER_AGENT'] = ua
            # XXX Other HTTP_* headers
            decoded_query = string.replace(query, '+', ' ')
            try:
                os.setuid(nobody)
            except os.error:
                pass
            os.dup2(self.rfile.fileno(), 0)
            os.dup2(self.wfile.fileno(), 1)
            print scriptfile, script, decoded_query
            os.execve(scriptfile,
                      [script, decoded_query],
                      env)
        except:
            self.server.handle_error(self.request, self.client_address)
            os._exit(127)


nobody = None

def nobody_uid():
    """Internal routine to get nobody's uid"""
    global nobody
    if nobody:
        return nobody
    import pwd
    try:
        nobody = pwd.getpwnam('nobody')[2]
    except pwd.error:
        nobody = 1 + max(map(lambda x: x[2], pwd.getpwall()))
    return nobody


def executable(path):
    """Test for executable file."""
    try:
        st = os.stat(path)
    except os.error:
        return 0
    return st[0] & 0111 != 0


def test(HandlerClass = CGIHTTPRequestHandler,
         ServerClass = BaseHTTPServer.HTTPServer):
    SimpleHTTPServer.test(HandlerClass, ServerClass)


if __name__ == '__main__':
    test()
