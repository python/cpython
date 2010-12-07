"""
A number of function that enhance IDLE on MacOSX when it used as a normal
GUI application (as opposed to an X11 application).
"""
import sys
import Tkinter


_appbundle = None

def runningAsOSXApp():
    """
    Returns True if Python is running from within an app on OSX.
    If so, assume that Python was built with Aqua Tcl/Tk rather than
    X11 Tcl/Tk.
    """
    global _appbundle
    if _appbundle is None:
        _appbundle = (sys.platform == 'darwin' and '.app' in sys.executable)
    return _appbundle

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
    try:
        root.tk.call('console', 'hide')
    except Tkinter.TclError:
        # Some versions of the Tk framework don't have a console object
        pass

def overrideRootMenu(root, flist):
    """
    Replace the Tk root menu by something that's more appropriate for
    IDLE.
    """
    # The menu that is attached to the Tk root (".") is also used by AquaTk for
    # all windows that don't specify a menu of their own. The default menubar
    # contains a number of menus, none of which are appropriate for IDLE. The
    # Most annoying of those is an 'About Tck/Tk...' menu in the application
    # menu.
    #
    # This function replaces the default menubar by a mostly empty one, it
    # should only contain the correct application menu and the window menu.
    #
    # Due to a (mis-)feature of TkAqua the user will also see an empty Help
    # menu.
    from Tkinter import Menu, Text, Text
    from idlelib.EditorWindow import prepstr, get_accelerator
    from idlelib import Bindings
    from idlelib import WindowList
    from idlelib.MultiCall import MultiCallCreator

    menubar = Menu(root)
    root.configure(menu=menubar)
    menudict = {}

    menudict['windows'] = menu = Menu(menubar, name='windows')
    menubar.add_cascade(label='Window', menu=menu, underline=0)

    def postwindowsmenu(menu=menu):
        end = menu.index('end')
        if end is None:
            end = -1

        if end > 0:
            menu.delete(0, end)
        WindowList.add_windows_to_menu(menu)
    WindowList.register_callback(postwindowsmenu)

    menudict['application'] = menu = Menu(menubar, name='apple')
    menubar.add_cascade(label='IDLE', menu=menu)

    def about_dialog(event=None):
        from idlelib import aboutDialog
        aboutDialog.AboutDialog(root, 'About IDLE')

    def config_dialog(event=None):
        from idlelib import configDialog
        root.instance_dict = flist.inversedict
        configDialog.ConfigDialog(root, 'Settings')


    root.bind('<<about-idle>>', about_dialog)
    root.bind('<<open-config-dialog>>', config_dialog)
    if flist:
        root.bind('<<close-all-windows>>', flist.close_all_callback)

        # The binding above doesn't reliably work on all versions of Tk
        # on MacOSX. Adding command definition below does seem to do the
        # right thing for now.
        root.createcommand('exit', flist.close_all_callback)


    ###check if Tk version >= 8.4.14; if so, use hard-coded showprefs binding
    tkversion = root.tk.eval('info patchlevel')
    # Note: we cannot check if the string tkversion >= '8.4.14', because
    # the string '8.4.7' is greater than the string '8.4.14'.
    if tuple(map(int, tkversion.split('.'))) >= (8, 4, 14):
        Bindings.menudefs[0] =  ('application', [
                ('About IDLE', '<<about-idle>>'),
                None,
            ])
        root.createcommand('::tk::mac::ShowPreferences', config_dialog)
    else:
        for mname, entrylist in Bindings.menudefs:
            menu = menudict.get(mname)
            if not menu:
                continue
            else:
                for entry in entrylist:
                    if not entry:
                        menu.add_separator()
                    else:
                        label, eventname = entry
                        underline, label = prepstr(label)
                        accelerator = get_accelerator(Bindings.default_keydefs,
                        eventname)
                        def command(text=root, eventname=eventname):
                            text.event_generate(eventname)
                        menu.add_command(label=label, underline=underline,
                        command=command, accelerator=accelerator)

def setupApp(root, flist):
    """
    Perform setup for the OSX application bundle.
    """
    if not runningAsOSXApp(): return

    hideTkConsole(root)
    overrideRootMenu(root, flist)
    addOpenEventSupport(root, flist)
