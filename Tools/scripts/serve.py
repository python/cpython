#!/usr/bin/env python
'''
Small wsgiref based web server. Takes a path to serve from and an
optional port number (defaults to 8000), then tries to serve files.
Mime types are guessed from the file names, 404 errors are thrown
if the file is not found. Used for the make serve target in Doc.
'''
import sys
import os
import mimetypes
from wsgiref import simple_server, util

def app(environ, respond):

    fn = os.path.join(path, environ['PATH_INFO'][1:])
    if '.' not in fn.split(os.path.sep)[-1]:
        fn = os.path.join(fn, 'index.html')
    type = mimetypes.guess_type(fn)[0]

    if os.path.exists(fn):
        respond('200 OK', [('Content-Type', type)])
        return util.FileWrapper(open(fn))
    else:
        respond('404 Not Found', [('Content-Type', 'text/plain')])
        return ['not found']

if __name__ == '__main__':
    path = sys.argv[1]
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 8000
    httpd = simple_server.make_server('', port, app)
    print "Serving %s on port %s, control-C to stop" % (path, port)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print "\b\bShutting down."
