from test_support import verify, verbose
import sys

def check_all(modname):
    names = {}
    exec "import %s" % modname in names
    verify(hasattr(sys.modules[modname], "__all__"),
           "%s has no __all__ attribute" % modname)
    names = {}
    exec "from %s import *" % modname in names
    del names["__builtins__"]
    keys = names.keys()
    keys.sort()
    all = list(sys.modules[modname].__all__) # in case it's a tuple
    all.sort()
    verify(keys==all, "%s != %s" % (keys, all))

check_all("BaseHTTPServer")
check_all("Bastion")
check_all("CGIHTTPServer")
check_all("ConfigParser")
check_all("Cookie")
check_all("MimeWriter")
check_all("Queue")
check_all("SimpleHTTPServer")
check_all("SocketServer")
check_all("StringIO")
check_all("UserDict")
check_all("UserList")
check_all("UserString")
check_all("aifc")
check_all("anydbm")
check_all("atexit")
check_all("audiodev")
check_all("base64")
check_all("bdb")
check_all("binhex")
check_all("bisect")
check_all("calendar")
check_all("cgi")
check_all("chunk")
check_all("cmd")
check_all("code")
check_all("codecs")
check_all("codeop")
check_all("colorsys")
check_all("commands")
check_all("compileall")
check_all("copy")
check_all("copy_reg")
try:
    import bsddb
except ImportError:
    if verbose:
        print "can't import bsddb, so skipping dbhash"
else:
    check_all("dbhash")
check_all("dircache")
check_all("dis")
check_all("doctest")
check_all("dospath")
check_all("dumbdbm")
check_all("filecmp")
check_all("fileinput")
check_all("fnmatch")
check_all("fpformat")
check_all("ftplib")
check_all("getopt")
check_all("getpass")
check_all("glob")
check_all("robotparser")
