# -*-mode: python; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
# $Id$
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "tixwidget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates how to use multiple columns and multiple styles
# in the tixHList widget
#
# In a tixHList widget, you can have one ore more columns.
#

import Tix

TCL_ALL_EVENTS          = 0

def RunSample (root):
    shlist = DemoSHList(root)
    shlist.mainloop()
    shlist.destroy()

class DemoSHList:
    def __init__(self, w):
        self.root = w
        self.exit = -1

        z = w.winfo_toplevel()
        z.wm_protocol("WM_DELETE_WINDOW", lambda self=self: self.quitcmd())

        # We create the frame and the ScrolledHList widget
        # at the top of the dialog box
        #
        top = Tix.Frame( w, relief=Tix.RAISED, bd=1)

        # Put a simple hierachy into the HList (two levels). Use colors and
        # separator widgets (frames) to make the list look fancy
        #
        top.a = Tix.ScrolledHList(top, options='hlist.columns 3 hlist.header 1' )
        top.a.pack( expand=1, fill=Tix.BOTH, padx=10, pady=10, side=Tix.TOP)

        hlist=top.a.hlist

        # Create the title for the HList widget
        #       >> Notice that we have set the hlist.header subwidget option to true
        #      so that the header is displayed
        #

        boldfont=hlist.tk.call('tix','option','get','bold_font')

        # First some styles for the headers
        style={}
        style['header'] = Tix.DisplayStyle(Tix.TEXT, refwindow=hlist,
            anchor=Tix.CENTER, padx=8, pady=2, font = boldfont )

        hlist.header_create(0, itemtype=Tix.TEXT, text='Name',
            style=style['header'])
        hlist.header_create(1, itemtype=Tix.TEXT, text='Position',
            style=style['header'])

        # Notice that we use 3 columns in the hlist widget. This way when the user
        # expands the windows wide, the right side of the header doesn't look
        # chopped off. The following line ensures that the 3 column header is
        # not shown unless the hlist window is wider than its contents.
        #
        hlist.column_width(2,0)

        # This is our little relational database
        #
        boss = ('doe', 'John Doe',      'Director')

        managers = [
            ('jeff',  'Jeff Waxman',    'Manager'),
            ('john',  'John Lee',               'Manager'),
            ('peter', 'Peter Kenson',   'Manager')
        ]

        employees = [
            ('alex',  'john',   'Alex Kellman',         'Clerk'),
            ('alan',  'john',       'Alan Adams',               'Clerk'),
            ('andy',  'peter',      'Andreas Crawford', 'Salesman'),
            ('doug',  'jeff',       'Douglas Bloom',    'Clerk'),
            ('jon',   'peter',      'Jon Baraki',               'Salesman'),
            ('chris', 'jeff',       'Chris Geoffrey',   'Clerk'),
            ('chuck', 'jeff',       'Chuck McLean',             'Cleaner')
        ]

        style['mgr_name'] = Tix.DisplayStyle(Tix.TEXT, refwindow=hlist)

        style['mgr_posn'] = Tix.DisplayStyle(Tix.TEXT, padx=8, refwindow=hlist)

        style['empl_name'] = Tix.DisplayStyle(Tix.TEXT, refwindow=hlist)

        style['empl_posn'] = Tix.DisplayStyle(Tix.TEXT, padx=8, refwindow=hlist)

        # Let configure the appearance of the HList subwidget
        #
        hlist.config(separator='.', width=25, drawbranch=0, indent=10)
        hlist.column_width(0, chars=20)

        # Create the boss
        #
        hlist.add ('.',           itemtype=Tix.TEXT, text=boss[1],
            style=style['mgr_name'])
        hlist.item_create('.', 1, itemtype=Tix.TEXT, text=boss[2],
            style=style['mgr_posn'])

        # Create the managers
        #

        for key,name,posn in managers :
            e= '.'+ key
            hlist.add(e, itemtype=Tix.TEXT, text=name,
                style=style['mgr_name'])
            hlist.item_create(e, 1, itemtype=Tix.TEXT, text=posn,
                style=style['mgr_posn'])


        for key,mgr,name,posn in employees :
            # "." is the separator character we chose above

            entrypath = '.' + mgr        + '.' + key

            #           ^^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^^
            #       parent entryPath / child's name

            hlist.add(entrypath, text=name, style=style['empl_name'])
            hlist.item_create(entrypath, 1, itemtype=Tix.TEXT,
                text = posn, style = style['empl_posn'] )


        # Use a ButtonBox to hold the buttons.
        #
        box= Tix.ButtonBox(top, orientation=Tix.HORIZONTAL )
        box.add( 'ok',  text='Ok', underline=0,  width=6,
            command = self.okcmd )

        box.add( 'cancel', text='Cancel', underline=0, width=6,
            command = self.quitcmd )

        box.pack( side=Tix.BOTTOM, fill=Tix.X)
        top.pack( side=Tix.TOP,    fill=Tix.BOTH, expand=1 )

    def okcmd (self):
        self.quitcmd()

    def quitcmd (self):
        self.exit = 0

    def mainloop(self):
        while self.exit < 0:
            self.root.tk.dooneevent(TCL_ALL_EVENTS)

    def destroy (self):
        self.root.destroy()


# This "if" statement makes it possible to run this script file inside or
# outside of the main demo program "tixwidgets.py".
#
if __name__== '__main__' :
    root=Tix.Tk()
    RunSample(root)
