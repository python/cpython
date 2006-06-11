"""
A number of function that enhance IDLE on MacOSX when it used as a normal
GUI application (as opposed to an X11 application).
"""
import sys

def runningAsOSXApp():
    """ Returns True iff running from the IDLE.app bundle on OSX """
    return (sys.platform == 'darwin' and 'IDLE.app' in sys.argv[0])

def addOpenEventSupport(root, flist):
    """
    This ensures that the application will respont to open AppleEvents, which
    makes is feaseable to use IDLE as the default application for python files.
    """
    def doOpenFile(*args):
        for fn in args:
            flist.open(fn)

    # The command below is a hook in aquatk that is called whenever the app
    # receives a file open event. The callback can have multiple arguments,
    # one for every file that should be opened.
    root.createcommand("::tk::mac::OpenDocument", doOpenFile)

def hideTkConsole(root):
    root.tk.call('console', 'hide')


def setupApp(root, flist):
    """
    Perform setup for the OSX application bundle.
    """
    if not runningAsOSXApp(): return

    hideTkConsole(root)
    addOpenEventSupport(root, flist)
