Issue #28022: Deprecate ssl-related arguments in favor of SSLContext. The
deprecation include manual creation of SSLSocket and certfile/keyfile
(or similar) in ftplib, httplib, imaplib, smtplib, poplib and urllib.