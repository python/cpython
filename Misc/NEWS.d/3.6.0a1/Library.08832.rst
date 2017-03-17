Issue #25287: Don't add crypt.METHOD_CRYPT to crypt.methods if it's not
supported. Check if it is supported, it may not be supported on OpenBSD for
example.