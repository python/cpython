Issue #27568: Prevent HTTPoxy attack (CVE-2016-1000110). Ignore the
HTTP_PROXY variable when REQUEST_METHOD environment is set, which indicates
that the script is in CGI mode.