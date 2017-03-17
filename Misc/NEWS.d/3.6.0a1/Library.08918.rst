Issue #26227: On Windows, getnameinfo(), gethostbyaddr() and
gethostbyname_ex() functions of the socket module now decode the hostname
from the ANSI code page rather than UTF-8.