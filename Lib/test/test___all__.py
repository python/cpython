
from test_support import verify, verbose, TestFailed
import sys

def check_all(_modname):
    exec "import %s" % _modname
    verify(hasattr(sys.modules[_modname],"__all__"),
           "%s has no __all__ attribute" % _modname)
    exec "del %s" % _modname
    exec "from %s import *" % _modname
    
    _keys = locals().keys()
    _keys.remove("_modname")
    _keys.sort()
    all = list(sys.modules[_modname].__all__) # in case it's a tuple
    all.sort()
    verify(_keys==all,"%s != %s" % (_keys,all))

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
check_all("dbhash")
check_all("dircache")
check_all("dis")
check_all("robotparser")
