Issue #26644: Raise ValueError rather than SystemError when a negative
length is passed to SSLSocket.recv() or read().