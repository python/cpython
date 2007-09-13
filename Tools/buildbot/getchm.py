# We cannot build the documentation on Windows, so download an old
# one instead.
import urllib, os
if not os.path.exists("Doc/python25.chm"):
    urllib.urlretrieve("http://www.python.org/ftp/python/2.5.1/Python25.chm",
        "Doc/python25.chm")
