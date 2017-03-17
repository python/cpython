Issue #17214: The "urllib.request" module now percent-encodes non-ASCII
bytes found in redirect target URLs.  Some servers send Location header
fields with non-ASCII bytes, but "http.client" requires the request target
to be ASCII-encodable, otherwise a UnicodeEncodeError is raised.  Based on
patch by Christian Heimes.