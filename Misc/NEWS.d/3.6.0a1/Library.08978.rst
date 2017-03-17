Issue #26586: In http.server, respond with "413 Request header fields too
large" if there are too many header fields to parse, rather than killing
the connection and raising an unhandled exception.  Patch by Xiang Zhang.