Issue #26721: Change the socketserver.StreamRequestHandler.wfile attribute
to implement BufferedIOBase. In particular, the write() method no longer
does partial writes.