Issue #24291: Fix wsgiref.simple_server.WSGIRequestHandler to completely
write data to the client.  Previously it could do partial writes and
truncate data.  Also, wsgiref.handler.ServerHandler can now handle stdout
doing partial writes, but this is deprecated.