Issue #28548: In the "http.server" module, parse the protocol version if
possible, to avoid using HTTP 0.9 in some error responses.