from test_support import verify, verbose
import sys

def check_all(modname):
    names = {}
    try:
        exec "import %s" % modname in names
    except ImportError:
        # Silent fail here seems the best route since some modules
        # may not be available in all environments.
        # Since an ImportError may leave a partial module object in
        # sys.modules, get rid of that first.  Here's what happens if
        # you don't:  importing pty fails on Windows because pty tries to
        # import FCNTL, which doesn't exist.  That raises an ImportError,
        # caught here.  It also leaves a partial pty module in sys.modules.
        # So when test_pty is called later, the import of pty succeeds,
        # but shouldn't.  As a result, test_pty crashes with an
        # AttributeError instead of an ImportError, and regrtest interprets
        # the latter as a test failure (ImportError is treated as "test
        # skipped" -- which is what test_pty should say on Windows).
        try:
            del sys.modules[modname]
        except KeyError:
            pass
        return
    verify(hasattr(sys.modules[modname], "__all__"),
           "%s has no __all__ attribute" % modname)
    names = {}
    exec "from %s import *" % modname in names
    if names.has_key("__builtins__"):
        del names["__builtins__"]
    keys = names.keys()
    keys.sort()
    all = list(sys.modules[modname].__all__) # in case it's a tuple
    all.sort()
    verify(keys==all, "%s != %s" % (keys, all))

if not sys.platform.startswith('java'):
    # In case _socket fails to build, make this test fail more gracefully
    # than an AttributeError somewhere deep in CGIHTTPServer.
    import _socket

check_all("BaseHTTPServer")
check_all("CGIHTTPServer")
check_all("ConfigParser")
check_all("Cookie")
check_all("MimeWriter")
check_all("SimpleHTTPServer")
check_all("SocketServer")
check_all("StringIO")
check_all("UserString")
check_all("aifc")
check_all("atexit")
check_all("audiodev")
check_all("base64")
check_all("bdb")
check_all("binhex")
check_all("calendar")
check_all("cgi")
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
check_all("doctest")
check_all("dospath")
check_all("filecmp")
check_all("fileinput")
check_all("fnmatch")
check_all("fpformat")
check_all("ftplib")
check_all("getopt")
check_all("getpass")
check_all("gettext")
check_all("glob")
check_all("gopherlib")
check_all("gzip")
check_all("htmllib")
check_all("httplib")
check_all("ihooks")
check_all("imaplib")
check_all("imghdr")
check_all("imputil")
check_all("keyword")
check_all("linecache")
check_all("locale")
check_all("macpath")
check_all("macurl2path")
check_all("mailbox")
check_all("mhlib")
check_all("mimetools")
check_all("mimetypes")
check_all("mimify")
check_all("multifile")
check_all("netrc")
check_all("nntplib")
check_all("ntpath")
check_all("os")
check_all("pdb")
check_all("pickle")
check_all("pipes")
check_all("popen2")
check_all("poplib")
check_all("posixpath")
check_all("pprint")
check_all("pre")
check_all("profile")
check_all("pstats")
check_all("pty")
check_all("py_compile")
check_all("pyclbr")
check_all("quopri")
check_all("random")
check_all("re")
check_all("reconvert")
import warnings
warnings.filterwarnings("ignore", ".* regsub .*", DeprecationWarning, "regsub",
                        append=1)
check_all("regsub")
check_all("repr")
check_all("rexec")
check_all("rfc822")
check_all("rlcompleter")
check_all("robotparser")
check_all("sched")
check_all("sgmllib")
check_all("shelve")
check_all("shlex")
check_all("shutil")
check_all("smtpd")
check_all("smtplib")
check_all("sndhdr")
check_all("socket")
check_all("sre")
check_all("stat_cache")
check_all("tabnanny")
check_all("telnetlib")
check_all("tempfile")
check_all("toaiff")
check_all("tokenize")
check_all("traceback")
check_all("tty")
check_all("urllib")
check_all("urlparse")
check_all("uu")
check_all("warnings")
check_all("wave")
check_all("weakref")
check_all("webbrowser")
check_all("xdrlib")
check_all("zipfile")
