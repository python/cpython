[Security] Issue #26657: Fix directory traversal vulnerability with
http.server on Windows.  This fixes a regression that was introduced in
3.3.4rc1 and 3.4.0rc1.  Based on patch by Philipp Hagemeister.