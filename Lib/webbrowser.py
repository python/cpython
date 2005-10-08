#! /usr/bin/env python
"""Interfaces for launching and remotely controlling Web browsers."""

import os
import sys
import stat

__all__ = ["Error", "open", "open_new", "open_new_tab", "get", "register"]

class Error(Exception):
    pass

_browsers = {}          # Dictionary of available browser controllers
_tryorder = []          # Preference order of available browsers

def register(name, klass, instance=None, update_tryorder=1):
    """Register a browser connector and, optionally, connection."""
    _browsers[name.lower()] = [klass, instance]
    if update_tryorder > 0:
        _tryorder.append(name)
    elif update_tryorder < 0:
        _tryorder.insert(0, name)

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
            # User gave us a browser name or path.
            try:
                command = _browsers[browser.lower()]
            except KeyError:
                command = _synthesize(browser)
            if command[1] is not None:
                return command[1]
            elif command[0] is not None:
                return command[0]()
    raise Error("could not locate runnable browser")

# Please note: the following definition hides a builtin function.
# It is recommended one does "import webbrowser" and uses webbrowser.open(url)
# instead of "from webbrowser import *".

def open(url, new=0, autoraise=1):
    for name in _tryorder:
        browser = get(name)
        if browser.open(url, new, autoraise):
            return True
    return False

def open_new(url):
    return open(url, 1)

def open_new_tab(url):
    return open(url, 2)


def _synthesize(browser, update_tryorder=1):
    """Attempt to synthesize a controller base on existing controllers.

    This is useful to create a controller when a user specifies a path to
    an entry in the BROWSER environment variable -- we can copy a general
    controller to operate using a specific installation of the desired
    browser in this way.

    If we can't create a controller in this way, or if there is no
    executable for the requested browser, return [None, None].

    """
    cmd = browser.split()[0]
    if not _iscommand(cmd):
        return [None, None]
    name = os.path.basename(cmd)
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
        register(browser, None, controller, update_tryorder)
        return [None, controller]
    return [None, None]


if sys.platform[:3] == "win":
    def _isexecutable(cmd):
        cmd = cmd.lower()
        if os.path.isfile(cmd) and (cmd.endswith(".exe") or 
                                    cmd.endswith(".bat")):
            return True
        for ext in ".exe", ".bat":
            if os.path.isfile(cmd + ext):
                return True
        return False
else:
    def _isexecutable(cmd):
        if os.path.isfile(cmd):
            mode = os.stat(cmd)[stat.ST_MODE]
            if mode & stat.S_IXUSR or mode & stat.S_IXGRP or mode & stat.S_IXOTH:
                return True
        return False

def _iscommand(cmd):
    """Return True if cmd is executable or can be found on the executable
    search path."""
    if _isexecutable(cmd):
        return True
    path = os.environ.get("PATH")
    if not path:
        return False
    for d in path.split(os.pathsep):
        exe = os.path.join(d, cmd)
        if _isexecutable(exe):
            return True
    return False


# General parent classes

class BaseBrowser(object):
    """Parent class for all browsers."""

    def __init__(self, name=""):
        self.name = name
        self.basename = name
    
    def open(self, url, new=0, autoraise=1):
        raise NotImplementedError

    def open_new(self, url):
        return self.open(url, 1)

    def open_new_tab(self, url):
        return self.open(url, 2)


class GenericBrowser(BaseBrowser):
    """Class for all browsers started with a command
       and without remote functionality."""

    def __init__(self, cmd):
        self.name, self.args = cmd.split(None, 1)
        self.basename = os.path.basename(self.name)

    def open(self, url, new=0, autoraise=1):
        assert "'" not in url
        command = "%s %s" % (self.name, self.args)
        rc = os.system(command % url)
        return not rc


class UnixBrowser(BaseBrowser):
    """Parent class for all Unix browsers with remote functionality."""

    raise_opts = None

    remote_cmd = ''
    remote_action = None
    remote_action_newwin = None
    remote_action_newtab = None
    remote_background = False

    def _remote(self, url, action, autoraise):
        autoraise = int(bool(autoraise)) # always 0/1
        raise_opt = self.raise_opts and self.raise_opts[autoraise] or ''
        cmd = "%s %s %s '%s' >/dev/null 2>&1" % (self.name, raise_opt,
                                                 self.remote_cmd, action)
        if self.remote_background:
            cmd += ' &'
        rc = os.system(cmd)
        if rc:
            # bad return status, try again with simpler command
            rc = os.system("%s %s" % (self.name, url))
        return not rc

    def open(self, url, new=0, autoraise=1):
        assert "'" not in url
        if new == 0:
            action = self.remote_action
        elif new == 1:
            action = self.remote_action_newwin
        elif new == 2:
            if self.remote_action_newtab is None:
                action = self.remote_action_newwin
            else:
                action = self.remote_action_newtab
        else:
            raise Error("Bad 'new' parameter to open(); expected 0, 1, or 2, got %s" % new)
        return self._remote(url, action % url, autoraise)


class Mozilla(UnixBrowser):
    """Launcher class for Mozilla/Netscape browsers."""

    raise_opts = ("-noraise", "-raise")

    remote_cmd = '-remote'
    remote_action = "openURL(%s)"
    remote_action_newwin = "openURL(%s,new-window)"
    remote_action_newtab = "openURL(%s,new-tab)"

Netscape = Mozilla


class Galeon(UnixBrowser):
    """Launcher class for Galeon/Epiphany browsers."""

    raise_opts = ("-noraise", "")
    remote_action = "-n '%s'"
    remote_action_newwin = "-w '%s'"

    remote_background = True


class Konqueror(BaseBrowser):
    """Controller for the KDE File Manager (kfm, or Konqueror).

    See http://developer.kde.org/documentation/other/kfmclient.html
    for more information on the Konqueror remote-control interface.

    """

    def _remote(self, url, action):
        # kfmclient is the new KDE way of opening URLs.
        cmd = "kfmclient %s >/dev/null 2>&1" % action
        rc = os.system(cmd)
        # Fall back to other variants.
        if rc:
            if _iscommand("konqueror"):
                rc = os.system(self.name + " --silent '%s' &" % url)
            elif _iscommand("kfm"):
                rc = os.system(self.name + " -d '%s'" % url)
        return not rc

    def open(self, url, new=0, autoraise=1):
        # XXX Currently I know no way to prevent KFM from
        # opening a new win.
        assert "'" not in url
        if new == 2:
            action = "newTab '%s'" % url
        else:
            action = "openURL '%s'" % url
        ok = self._remote(url, action)
        return ok


class Opera(UnixBrowser):
    "Launcher class for Opera browser."

    raise_opts = ("", "-raise")

    remote_cmd = '-remote'
    remote_action = "openURL(%s)"
    remote_action_newwin = "openURL(%s,new-window)"
    remote_action_newtab = "openURL(%s,new-page)"


class Elinks(UnixBrowser):
    "Launcher class for Elinks browsers."

    remote_cmd = '-remote'
    remote_action = "openURL(%s)"
    remote_action_newwin = "openURL(%s,new-window)"
    remote_action_newtab = "openURL(%s,new-tab)"

    def _remote(self, url, action, autoraise):
        # elinks doesn't like its stdout to be redirected -
        # it uses redirected stdout as a signal to do -dump
        cmd = "%s %s '%s' 2>/dev/null" % (self.name,
                                          self.remote_cmd, action)
        rc = os.system(cmd)
        if rc:
            rc = os.system("%s %s" % (self.name, url))
        return not rc


class Grail(BaseBrowser):
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
            ok = self._remote("LOADNEW " + url)
        else:
            ok = self._remote("LOAD " + url)
        return ok


#
# Platform support for Unix
#

# These are the right tests because all these Unix browsers require either
# a console terminal or an X display to run.

def register_X_browsers():
    # First, the Mozilla/Netscape browsers
    for browser in ("mozilla-firefox", "firefox",
                    "mozilla-firebird", "firebird",
                    "mozilla", "netscape"):
        if _iscommand(browser):
            register(browser, None, Mozilla(browser))

    # The default Gnome browser
    if _iscommand("gconftool-2"):
        # get the web browser string from gconftool
        gc = 'gconftool-2 -g /desktop/gnome/url-handlers/http/command'
        out = os.popen(gc)
        commd = out.read().strip()
        retncode = out.close()

        # if successful, register it
        if retncode == None and len(commd) != 0:
            register("gnome", None, GenericBrowser(
                commd + " '%s' >/dev/null &"))

    # Konqueror/kfm, the KDE browser.
    if _iscommand("kfm"):
        register("kfm", Konqueror, Konqueror("kfm"))
    elif _iscommand("konqueror"):
        register("konqueror", Konqueror, Konqueror("konqueror"))

    # Gnome's Galeon and Epiphany
    for browser in ("galeon", "epiphany"):
        if _iscommand(browser):
            register(browser, None, Galeon(browser))

    # Skipstone, another Gtk/Mozilla based browser
    if _iscommand("skipstone"):
        register("skipstone", None, GenericBrowser("skipstone '%s' &"))

    # Opera, quite popular
    if _iscommand("opera"):
        register("opera", None, Opera("opera"))

    # Next, Mosaic -- old but still in use.
    if _iscommand("mosaic"):
        register("mosaic", None, GenericBrowser("mosaic '%s' &"))

    # Grail, the Python browser. Does anybody still use it?
    if _iscommand("grail"):
        register("grail", Grail, None)

# Prefer X browsers if present
if os.environ.get("DISPLAY"):
    register_X_browsers()

# Also try console browsers
if os.environ.get("TERM"):
    # The Links/elinks browsers <http://artax.karlin.mff.cuni.cz/~mikulas/links/>
    if _iscommand("links"):
        register("links", None, GenericBrowser("links '%s'"))
    if _iscommand("elinks"):
        register("elinks", None, Elinks("elinks"))
    # The Lynx browser <http://lynx.isc.org/>, <http://lynx.browser.org/>
    if _iscommand("lynx"):
        register("lynx", None, GenericBrowser("lynx '%s'"))
    # The w3m browser <http://w3m.sourceforge.net/>
    if _iscommand("w3m"):
        register("w3m", None, GenericBrowser("w3m '%s'"))

#
# Platform support for Windows
#

if sys.platform[:3] == "win":
    class WindowsDefault(BaseBrowser):
        def open(self, url, new=0, autoraise=1):
            os.startfile(url)
            return True # Oh, my...

    _tryorder = []
    _browsers = {}
    # Prefer mozilla/netscape/opera if present
    for browser in ("firefox", "firebird", "mozilla", "netscape", "opera"):
        if _iscommand(browser):
            register(browser, None, GenericBrowser(browser + ' %s'))
    register("windows-default", WindowsDefault)

#
# Platform support for MacOS
#

try:
    import ic
except ImportError:
    pass
else:
    class InternetConfig(BaseBrowser):
        def open(self, url, new=0, autoraise=1):
            ic.launchurl(url)
            return True # Any way to get status?

    register("internet-config", InternetConfig, update_tryorder=-1)

if sys.platform == 'darwin':
    # Adapted from patch submitted to SourceForge by Steven J. Burr
    class MacOSX(BaseBrowser):
        """Launcher class for Aqua browsers on Mac OS X

        Optionally specify a browser name on instantiation.  Note that this
        will not work for Aqua browsers if the user has moved the application
        package after installation.

        If no browser is specified, the default browser, as specified in the
        Internet System Preferences panel, will be used.
        """
        def __init__(self, name):
            self.name = name

        def open(self, url, new=0, autoraise=1):
            assert "'" not in url
            # new must be 0 or 1
            new = int(bool(new))
            if self.name == "default":
                # User called open, open_new or get without a browser parameter
                script = _safequote('open location "%s"', url) # opens in default browser
            else:
                # User called get and chose a browser
                if self.name == "OmniWeb":
                    toWindow = ""
                else:
                    # Include toWindow parameter of OpenURL command for browsers
                    # that support it.  0 == new window; -1 == existing
                    toWindow = "toWindow %d" % (new - 1)
                cmd = _safequote('OpenURL "%s"', url)
                script = '''tell application "%s"
                                activate
                                %s %s
                            end tell''' % (self.name, cmd, toWindow)
            # Open pipe to AppleScript through osascript command
            osapipe = os.popen("osascript", "w")
            if osapipe is None:
                return False
            # Write script to osascript's stdin
            osapipe.write(script)
            rc = osapipe.close()
            return not rc

    # Don't clear _tryorder or _browsers since OS X can use above Unix support
    # (but we prefer using the OS X specific stuff)
    register("MacOSX", None, MacOSX('default'), -1)


#
# Platform support for OS/2
#

if sys.platform[:3] == "os2" and _iscommand("netscape"):
    _tryorder = []
    _browsers = {}
    register("os2netscape", None,
             GenericBrowser("start netscape %s"), -1)


# OK, now that we know what the default preference orders for each
# platform are, allow user to override them with the BROWSER variable.
if "BROWSER" in os.environ:
    _userchoices = os.environ["BROWSER"].split(os.pathsep)
    _userchoices.reverse()

    # Treat choices in same way as if passed into get() but do register
    # and prepend to _tryorder
    for cmdline in _userchoices:
        if cmdline != '':
            _synthesize(cmdline, -1)
    cmdline = None # to make del work if _userchoices was empty
    del cmdline
    del _userchoices

# what to do if _tryorder is now empty?


def main():
    import getopt
    usage = """Usage: %s [-n | -t] url
    -n: open new window
    -t: open new tab""" % sys.argv[0]
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'ntd')
    except getopt.error, msg:
        print >>sys.stderr, msg
        print >>sys.stderr, usage
        sys.exit(1)
    new_win = 0
    for o, a in opts:
        if o == '-n': new_win = 1
        elif o == '-t': new_win = 2
    if len(args) <> 1:
        print >>sys.stderr, usage
        sys.exit(1)

    url = args[0]
    open(url, new_win)

if __name__ == "__main__":
    main()
