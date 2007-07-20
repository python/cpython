"""
A number of function that enhance IDLE on MacOSX when it used as a normal
GUI application (as opposed to an X11 application).
"""
import sys
import Tkinter

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
    from .EditorWindow import prepstr, get_accelerator
    from . import Bindings
    from . import WindowList
    from .MultiCall import MultiCallCreator

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
        from . import aboutDialog
        aboutDialog.AboutDialog(root, 'About IDLE')

    def config_dialog(event=None):
        from . import configDialog
        configDialog.ConfigDialog(root, 'Settings')


    root.bind('<<about-idle>>', about_dialog)
    root.bind('<<open-config-dialog>>', config_dialog)
    if flist:
        root.bind('<<close-all-windows>>', flist.close_all_callback)


    ###check if Tk version >= 8.4.14; if so, use hard-coded showprefs binding
    tkversion = root.tk.eval('info patchlevel')
    if tkversion >= '8.4.14':
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
