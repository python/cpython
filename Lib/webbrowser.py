"""Interfaces for launching and remotely controlling Web browsers."""

import os
import sys

__all__ = ["Error", "open", "get", "register"]

class Error(Exception):
    pass

_browsers = {}          # Dictionary of available browser controllers
_tryorder = []          # Preference order of available browsers

def register(name, klass, instance=None):
    """Register a browser connector and, optionally, connection."""
    _browsers[name.lower()] = [klass, instance]

def get(using=None):
    """Return a browser launcher instance appropriate for the environment."""
    if using is not None:
        alternatives = [using]
    else:
        alternatives = _tryorder
    for browser in alternatives:
        if '%s' in browser:
            # User gave us a command line, don't mess with it.
            return GenericBrowser(browser)
        else:
            # User gave us a browser name.
            try:
                command = _browsers[browser.lower()]
            except KeyError:
                command = _synthesize(browser)
            if command[1] is None:
                return command[0]()
            else:
                return command[1]
    raise Error("could not locate runnable browser")

# Please note: the following definition hides a builtin function.

def open(url, new=0, autoraise=1):
    get().open(url, new, autoraise)

def open_new(url):
    get().open(url, 1)


def _synthesize(browser):
    """Attempt to synthesize a controller base on existing controllers.

    This is useful to create a controller when a user specifies a path to
    an entry in the BROWSER environment variable -- we can copy a general
    controller to operate using a specific installation of the desired
    browser in this way.

    If we can't create a controller in this way, or if there is no
    executable for the requested browser, return [None, None].

    """
    if not os.path.exists(browser):
        return [None, None]
    name = os.path.basename(browser)
    try:
        command = _browsers[name.lower()]
    except KeyError:
        return [None, None]
    # now attempt to clone to fit the new name:
    controller = command[1]
    if controller and name.lower() == controller.basename:
        import copy
        controller = copy.copy(controller)
        controller.name = browser
        controller.basename = os.path.basename(browser)
        register(browser, None, controller)
        return [None, controller]
    return [None, None]


def _iscommand(cmd):
    """Return True if cmd can be found on the executable search path."""
    path = os.environ.get("PATH")
    if not path:
        return False
    for d in path.split(os.pathsep):
        exe = os.path.join(d, cmd)
        if os.path.isfile(exe):
            return True
    return False


PROCESS_CREATION_DELAY = 4


class GenericBrowser:
    def __init__(self, cmd):
        self.name, self.args = cmd.split(None, 1)
        self.basename = os.path.basename(self.name)

    def open(self, url, new=0, autoraise=1):
        assert "'" not in url
        command = "%s %s" % (self.name, self.args)
        os.system(command % url)

    def open_new(self, url):
        self.open(url)


class Netscape:
    "Launcher class for Netscape browsers."
    def __init__(self, name):
        self.name = name
        self.basename = os.path.basename(name)

    def _remote(self, action, autoraise):
        raise_opt = ("-noraise", "-raise")[autoraise]
        cmd = "%s %s -remote '%s' >/dev/null 2>&1" % (self.name,
                                                      raise_opt,
                                                      action)
        rc = os.system(cmd)
        if rc:
            import time
            os.system("%s &" % self.name)
            time.sleep(PROCESS_CREATION_DELAY)
            rc = os.system(cmd)
        return not rc

    def open(self, url, new=0, autoraise=1):
        if new:
            self._remote("openURL(%s, new-window)"%url, autoraise)
        else:
            self._remote("openURL(%s)" % url, autoraise)

    def open_new(self, url):
        self.open(url, 1)


class Galeon:
    """Launcher class for Galeon browsers."""
    def __init__(self, name):
        self.name = name
        self.basename = os.path.basename(name)

    def _remote(self, action, autoraise):
        raise_opt = ("--noraise", "")[autoraise]
        cmd = "%s %s %s >/dev/null 2>&1" % (self.name, raise_opt, action)
        rc = os.system(cmd)
        if rc:
            import time
            os.system("%s >/dev/null 2>&1 &" % self.name)
            time.sleep(PROCESS_CREATION_DELAY)
            rc = os.system(cmd)
        return not rc

    def open(self, url, new=0, autoraise=1):
        if new:
            self._remote("-w '%s'" % url, autoraise)
        else:
            self._remote("-n '%s'" % url, autoraise)

    def open_new(self, url):
        self.open(url, 1)


class Konqueror:
    """Controller for the KDE File Manager (kfm, or Konqueror).

    See http://developer.kde.org/documentation/other/kfmclient.html
    for more information on the Konqueror remote-control interface.

    """
    def __init__(self):
        if _iscommand("konqueror"):
            self.name = self.basename = "konqueror"
        else:
            self.name = self.basename = "kfm"

    def _remote(self, action):
        cmd = "kfmclient %s >/dev/null 2>&1" % action
        rc = os.system(cmd)
        if rc:
            import time
            if self.basename == "konqueror":
                os.system(self.name + " --silent &")
            else:
                os.system(self.name + " -d &")
            time.sleep(PROCESS_CREATION_DELAY)
            rc = os.system(cmd)
        return not rc

    def open(self, url, new=1, autoraise=1):
        # XXX Currently I know no way to prevent KFM from
        # opening a new win.
        assert "'" not in url
        self._remote("openURL '%s'" % url)

    open_new = open


class Grail:
    # There should be a way to maintain a connection to Grail, but the
    # Grail remote control protocol doesn't really allow that at this
    # point.  It probably neverwill!
    def _find_grail_rc(self):
        import glob
        import pwd
        import socket
        import tempfile
        tempdir = os.path.join(tempfile.gettempdir(),
                               ".grail-unix")
        user = pwd.getpwuid(os.getuid())[0]
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

    def open(self, url, new=0, autoraise=1):
        if new:
            self._remote("LOADNEW " + url)
        else:
            self._remote("LOAD " + url)

    def open_new(self, url):
        self.open(url, 1)


class WindowsDefault:
    def open(self, url, new=0, autoraise=1):
        os.startfile(url)

    def open_new(self, url):
        self.open(url)

#
# Platform support for Unix
#

# This is the right test because all these Unix browsers require either
# a console terminal of an X display to run.  Note that we cannot split
# the TERM and DISPLAY cases, because we might be running Python from inside
# an xterm.
if os.environ.get("TERM") or os.environ.get("DISPLAY"):
    _tryorder = ["links", "lynx", "w3m"]

    # Easy cases first -- register console browsers if we have them.
    if os.environ.get("TERM"):
        # The Links browser <http://artax.karlin.mff.cuni.cz/~mikulas/links/>
        if _iscommand("links"):
            register("links", None, GenericBrowser("links '%s'"))
        # The Lynx browser <http://lynx.browser.org/>
        if _iscommand("lynx"):
            register("lynx", None, GenericBrowser("lynx '%s'"))
        # The w3m browser <http://ei5nazha.yz.yamagata-u.ac.jp/~aito/w3m/eng/>
        if _iscommand("w3m"):
            register("w3m", None, GenericBrowser("w3m '%s'"))

    # X browsers have more in the way of options
    if os.environ.get("DISPLAY"):
        _tryorder = ["galeon", "skipstone",
                     "mozilla-firefox", "mozilla-firebird", "mozilla", "netscape",
                     "kfm", "grail"] + _tryorder

        # First, the Netscape series
        for browser in ("mozilla-firefox", "mozilla-firebird",
                        "mozilla", "netscape"):
            if _iscommand(browser):
                register(browser, None, Netscape(browser))

        # Next, Mosaic -- old but still in use.
        if _iscommand("mosaic"):
            register("mosaic", None, GenericBrowser(
                "mosaic '%s' >/dev/null &"))

        # Gnome's Galeon
        if _iscommand("galeon"):
            register("galeon", None, Galeon("galeon"))

        # Skipstone, another Gtk/Mozilla based browser
        if _iscommand("skipstone"):
            register("skipstone", None, GenericBrowser(
                "skipstone '%s' >/dev/null &"))

        # Konqueror/kfm, the KDE browser.
        if _iscommand("kfm") or _iscommand("konqueror"):
            register("kfm", Konqueror, Konqueror())

        # Grail, the Python browser.
        if _iscommand("grail"):
            register("grail", Grail, None)


class InternetConfig:
    def open(self, url, new=0, autoraise=1):
        ic.launchurl(url)

    def open_new(self, url):
        self.open(url)


#
# Platform support for Windows
#

if sys.platform[:3] == "win":
    _tryorder = ["netscape", "windows-default"]
    register("windows-default", WindowsDefault)

#
# Platform support for MacOS
#

try:
    import ic
except ImportError:
    pass
else:
    # internet-config is the only supported controller on MacOS,
    # so don't mess with the default!
    _tryorder = ["internet-config"]
    register("internet-config", InternetConfig)

#
# Platform support for OS/2
#

if sys.platform[:3] == "os2" and _iscommand("netscape.exe"):
    _tryorder = ["os2netscape"]
    register("os2netscape", None,
             GenericBrowser("start netscape.exe %s"))

# OK, now that we know what the default preference orders for each
# platform are, allow user to override them with the BROWSER variable.
#
if "BROWSER" in os.environ:
    # It's the user's responsibility to register handlers for any unknown
    # browser referenced by this value, before calling open().
    _tryorder = os.environ["BROWSER"].split(os.pathsep)

for cmd in _tryorder:
    if not cmd.lower() in _browsers:
        if _iscommand(cmd.lower()):
            register(cmd.lower(), None, GenericBrowser(
                "%s '%%s'" % cmd.lower()))
cmd = None # to make del work if _tryorder was empty
del cmd

_tryorder = filter(lambda x: x.lower() in _browsers
                   or x.find("%s") > -1, _tryorder)
# what to do if _tryorder is now empty?
