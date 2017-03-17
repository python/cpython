Issue #26499: Account for remaining Content-Length in
HTTPResponse.readline() and read1().  Based on patch by Silent Ghost.
Also document that HTTPResponse now supports these methods.