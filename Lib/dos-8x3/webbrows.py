"""Remote-control interfaces to some browsers."""

import os
import sys


PROCESS_CREATION_DELAY = 4


class Error(Exception):
    pass


_browsers = {}

def register(name, klass, instance=None):
    """Register a browser connector and, optionally, connection."""
    _browsers[name.lower()] = [klass, instance]


def get(name=None):
    """Retrieve a connection to a browser by type name, or the default
    browser."""
    name = name or DEFAULT_BROWSER
    try:
        L = _browsers[name.lower()]
    except KeyError:
        raise ValueError, "unknown browser type: " + `name`
    if L[1] is None:
        L[1] = L[0]()
    return L[1]


# Please note: the following definition hides a builtin function.

def open(url, new=0):
    get().open(url, new)


def open_new(url):
    get().open_new(url)


def _iscommand(cmd):
    """Return true if cmd can be found on the executable search path."""
    path = os.environ.get("PATH")
    if not path:
        return 0
    for d in path.split(os.pathsep):
        exe = os.path.join(d, cmd)
        if os.path.isfile(exe):
            return 1
    return 0


class CommandLineBrowser:
    _browsers = []
    if os.environ.get("DISPLAY"):
        _browsers.extend([
            ("netscape", "netscape %s >/dev/null &"),
            ("mosaic", "mosaic %s >/dev/null &"),
            ])
    _browsers.extend([
        ("lynx", "lynx %s"),
        ("w3m", "w3m %s"),
        ])

    def open(self, url, new=0):
        for exe, cmd in self._browsers:
            if _iscommand(exe):
                os.system(cmd % url)
                return
        raise Error("could not locate runnable browser")

    def open_new(self, url):
        self.open(url)

register("command-line", CommandLineBrowser)


class Netscape:
    autoRaise = 1

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


class Konquerer:
    """Controller for the KDE File Manager (kfm, or Konquerer).

    See http://developer.kde.org/documentation/other/kfmclient.html
    for more information on the Konquerer remote-control interface.

    """
    def _remote(self, action):
        cmd = "kfmclient %s >/dev/null 2>&1" % action
        rc = os.system(cmd)
        if rc:
            import time
            os.system("kfm -d &")
            time.sleep(PROCESS_CREATION_DELAY)
            rc = os.system(cmd)
        return not rc

    def open(self, url, new=1):
	# XXX currently I know no way to prevent KFM from opening a new win.
        self.open_new(url)

    def open_new(self, url):
        self._remote("openURL %s" % url)

register("kfm", Konquerer)


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
        os.startfile(url)

    def open_new(self, url):
        self.open(url)


DEFAULT_BROWSER = "command-line"

if sys.platform[:3] == "win":
    del _browsers["kfm"]
    register("windows-default", WindowsDefault)
    DEFAULT_BROWSER = "windows-default"
elif os.environ.get("DISPLAY"):
    if _iscommand("netscape"):
        DEFAULT_BROWSER = "netscape"

# If the $BROWSER environment variable is set and true, let that be
# the name of the browser to use:
#
DEFAULT_BROWSER = os.environ.get("BROWSER") or DEFAULT_BROWSER


# Now try to support the MacOS world.  This is the only supported
# controller on that platform, so don't mess with the default!

try:
    import ic
except ImportError:
    pass
else:
    class InternetConfig:
        def open(self, url, new=0):
            ic.launcurl(url)

        def open_new(self, url):
            self.open(url)

    _browsers.clear()
    register("internet-config", InternetConfig)
    DEFAULT_BROWSER = "internet-config"
