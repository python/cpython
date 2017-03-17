Issue #26309: In the "socketserver" module, shut down the request (closing
the connected socket) when verify_request() returns false.  Patch by Aviv
Palivoda.