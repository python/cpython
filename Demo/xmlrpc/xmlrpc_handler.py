#
# XML-RPC SERVER
# $Id$
#
# an asynchronous XML-RPC server for Medusa
#
# written by Sam Rushing
#
# Based on "xmlrpcserver.py" by Fredrik Lundh (fredrik@pythonware.com)
#

import http_server
import xmlrpclib

import sys

class xmlrpc_handler:

    def match (self, request):
        # Note: /RPC2 is not required by the spec, so you may override this method.
        if request.uri[:5] == '/RPC2':
            return 1
        else:
            return 0

    def handle_request (self, request):
        [path, params, query, fragment] = request.split_uri()

        if request.command in ('post', 'put'):
            request.collector = collector (self, request)
        else:
            request.error (400)

    def continue_request (self, data, request):
        params, method = xmlrpclib.loads (data)
        try:
            # generate response
            try:
                response = self.call (method, params)
                response = (response,)
            except:
                # report exception back to server
                response = xmlrpclib.dumps (
                    xmlrpclib.Fault (1, "%s:%s" % sys.exc_info()[:2])
                    )
            else:
                response = xmlrpclib.dumps (response, methodresponse=1)
        except:
            # internal error, report as HTTP server error
            request.error (500)
        else:
            # got a valid XML RPC response
            request['Content-Type'] = 'text/xml'
            request.push (response)
            request.done()

    def call (self, method, params):
        # override this method to implement RPC methods
        raise "NotYetImplemented"

class collector:

    "gathers input for POST and PUT requests"

    def __init__ (self, handler, request):

        self.handler = handler
        self.request = request
        self.data = ''

        # make sure there's a content-length header
        cl = request.get_header ('content-length')

        if not cl:
            request.error (411)
        else:
            cl = int (cl)
            # using a 'numeric' terminator
            self.request.channel.set_terminator (cl)

    def collect_incoming_data (self, data):
        self.data = self.data + data

    def found_terminator (self):
        # set the terminator back to the default
        self.request.channel.set_terminator ('\r\n\r\n')
        self.handler.continue_request (self.data, self.request)

if __name__ == '__main__':

    class rpc_demo (xmlrpc_handler):

        def call (self, method, params):
            print 'method="%s" params=%s' % (method, params)
            return "Sure, that works"

    import asyncore
    import http_server

    hs = http_server.http_server ('', 8000)
    rpc = rpc_demo()
    hs.install_handler (rpc)

    asyncore.loop()
