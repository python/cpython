#
# XML-RPC SERVER
# $Id$
#
# a simple XML-RPC server for Python
#
# History:
# 1999-02-01 fl  added to xmlrpclib distribution
#
# written by Fredrik Lundh, January 1999.
#
# Copyright (c) 1999 by Secret Labs AB.
# Copyright (c) 1999 by Fredrik Lundh.
#
# fredrik@pythonware.com
# http://www.pythonware.com
#
# --------------------------------------------------------------------
# Permission to use, copy, modify, and distribute this software and
# its associated documentation for any purpose and without fee is
# hereby granted.  This software is provided as is.
# --------------------------------------------------------------------
#

import SocketServer, BaseHTTPServer
import xmlrpclib
import sys

class RequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):

    def do_POST(self):
        try:
            # get arguments
            data = self.rfile.read(int(self.headers["content-length"]))
            params, method = xmlrpclib.loads(data)

            # generate response
            try:
                response = self.call(method, params)
                # wrap response in a singleton tuple
                response = (response,)
            except:
                # report exception back to server
                response = xmlrpclib.dumps(
                    xmlrpclib.Fault(1, "%s:%s" % sys.exc_info()[:2])
                    )
            else:
                response = xmlrpclib.dumps(
                    response,
                    methodresponse=1
                    )
        except:
            # internal error, report as HTTP server error
            self.send_response(500)
            self.end_headers()
        else:
            # got a valid XML RPC response
            self.send_response(200)
            self.send_header("Content-type", "text/xml")
            self.send_header("Content-length", str(len(response)))
            self.end_headers()
            self.wfile.write(response)

            # shut down the connection (from Skip Montanaro)
            self.wfile.flush()
            self.connection.shutdown(1)

    def call(self, method, params):
        # override this method to implement RPC methods
        print "CALL", method, params
        return params

if __name__ == '__main__':
    server = SocketServer.TCPServer(('', 8000), RequestHandler)
    server.serve_forever()
