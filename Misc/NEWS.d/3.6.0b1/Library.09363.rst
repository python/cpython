Issue #12319: Chunked transfer encoding support added to
http.client.HTTPConnection requests.  The
urllib.request.AbstractHTTPHandler class does not enforce a Content-Length
header any more.  If a HTTP request has a file or iterable body, but no
Content-Length header, the library now falls back to use chunked transfer-
encoding.