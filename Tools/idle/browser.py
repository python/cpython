"""Remote-control interfaces to some browsers."""

import os
import string
import sys


DEFAULT_CONFIG_FILE = "~/.browser.ini"
PROCESS_CREATION_DELAY = 4

DEFAULT_BROWSER = "netscape"

_browsers = {}


def get(name=None):
    if name is None:
        name = get_default_browser()
    else:
        name = string.lower(name)
    L = _browsers[name]
    if L[1] is None:
        L[1] = L[0]()
    return L[1]


def get_default_browser(file=None):
    if file is None:
        files = [DEFAULT_CONFIG_FILE]
    else:
        files = [file, DEFAULT_CONFIG_FILE]
    for file in files:
        file = os.path.expandvars(os.path.expanduser(file))
        if file and os.path.isfile(file):
            import ConfigParser
            cf = ConfigParser.ConfigParser()
            cf.read([file])
            try:
                return string.lower(cf.get("Browser", "name"))
            except ConfigParser.Error:
                pass
    return DEFAULT_BROWSER


_default_browser = None

def _get_browser():
    global _default_browser
    if _default_browser is None:
        _default_browser = get()
    return _default_browser


def open(url, new=0):
    _get_browser().open(url, new)


def open_new(url):
    _get_browser().open_new(url)


def register(name, klass):
    _browsers[string.lower(name)] = [klass, None]


class Netscape:
    autoRaise = 0

    def _remote(self, action):
        raise_opt = ("-noraise", "-raise")[self.autoRaise]
        cmd = "netscape %s -remote '%s' >/dev/null 2>&1" % (raise_opt, action)
        rc = os.system(cmd)
        if rc:
            import time
            os.system("netscape -no-about-splash &")
            time.sleep(PROCESS_CREATION_DELAY)
            rc = os.system(cmd)
        return not rc

    def open(self, url, new=0):
        if new:
            self.open_new(url)
        else:
            self._remote("openURL(%s)" % url)

    def open_new(self, url):
        self._remote("openURL(%s, new-window)" % url)

register("netscape", Netscape)


class Grail:
    # There should be a way to maintain a connection to Grail, but the
    # Grail remote control protocol doesn't really allow that at this
    # point.  It probably never will!

    def _find_grail_rc(self):
        import glob
        import pwd
        import socket
        import tempfile
        tempdir = os.path.join(tempfile.gettempdir(), ".grail-unix")
        user = pwd.getpwuid(_os.getuid())[0]
        filename = os.path.join(tempdir, user + "-*")
        maybes = glob.glob(filename)
        if not maybes:
            return None
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        for fn in maybes:
            # need to PING each one until we find one that's live
            try:
                s.connect(fn)
            except socket.error:
                # no good; attempt to clean it out, but don't fail:
                try:
                    os.unlink(fn)
                except IOError:
                    pass
            else:
                return s

    def _remote(self, action):
        s = self._find_grail_rc()
        if not s:
            return 0
        s.send(action)
        s.close()
        return 1

    def open(self, url, new=0):
        if new:
            self.open_new(url)
        else:
            self._remote("LOAD " + url)

    def open_new(self, url):
        self._remote("LOADNEW " + url)

register("grail", Grail)


class WindowsDefault:
    def open(self, url, new=0):
        import win32api, win32con
        try:
            win32api.ShellExecute(0, "open", url, None, ".",
                                  win32con.SW_SHOWNORMAL)
        except:
            traceback.print_exc()
            raise

    def open_new(self, url):
        self.open(url)

if sys.platform[:3] == "win":
    register("windows-default", WindowsDefault)
    DEFAULT_BROWSER = "windows-default"
