# -*-mode: python; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
# $Id$
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "tixwidgets.py": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program.

# This file demonstrates the use of the tixComboBox widget, which is close
# to the MS Window Combo Box control.
#
import tkinter.tix

def RunSample(w):
    global demo_month, demo_year

    top = tkinter.tix.Frame(w, bd=1, relief=tkinter.tix.RAISED)

    demo_month = tkinter.tix.StringVar()
    demo_year = tkinter.tix.StringVar()

    # $w.top.a is a drop-down combo box. It is not editable -- who wants
    # to invent new months?
    #
    # [Hint] The -options switch sets the options of the subwidgets.
    # [Hint] We set the label.width subwidget option of both comboboxes to
    #        be 10 so that their labels appear to be aligned.
    #
    a = tkinter.tix.ComboBox(top, label="Month: ", dropdown=1,
        command=select_month, editable=0, variable=demo_month,
        options='listbox.height 6 label.width 10 label.anchor e')

    # $w.top.b is a non-drop-down combo box. It is not editable: we provide
    # four choices for the user, but he can enter an alternative year if he
    # wants to.
    #
    # [Hint] Use the padY and anchor options of the label subwidget to
    #        align the label with the entry subwidget.
    # [Hint] Notice that you should use padY (the NAME of the option) and not
    #        pady (the SWITCH of the option).
    #
    b = tkinter.tix.ComboBox(top, label="Year: ", dropdown=0,
        command=select_year, editable=1, variable=demo_year,
        options='listbox.height 4 label.padY 5 label.width 10 label.anchor ne')

    a.pack(side=tkinter.tix.TOP, anchor=tkinter.tix.W)
    b.pack(side=tkinter.tix.TOP, anchor=tkinter.tix.W)

    a.insert(tkinter.tix.END, 'January')
    a.insert(tkinter.tix.END, 'February')
    a.insert(tkinter.tix.END, 'March')
    a.insert(tkinter.tix.END, 'April')
    a.insert(tkinter.tix.END, 'May')
    a.insert(tkinter.tix.END, 'June')
    a.insert(tkinter.tix.END, 'July')
    a.insert(tkinter.tix.END, 'August')
    a.insert(tkinter.tix.END, 'September')
    a.insert(tkinter.tix.END, 'October')
    a.insert(tkinter.tix.END, 'November')
    a.insert(tkinter.tix.END, 'December')

    b.insert(tkinter.tix.END, '1992')
    b.insert(tkinter.tix.END, '1993')
    b.insert(tkinter.tix.END, '1994')
    b.insert(tkinter.tix.END, '1995')
    b.insert(tkinter.tix.END, '1996')

    # Use "tixSetSilent" to set the values of the combo box if you
    # don't want your -command procedures (cbx:select_month and
    # cbx:select_year) to be called.
    #
    a.set_silent('January')
    b.set_silent('1995')

    box = tkinter.tix.ButtonBox(w, orientation=tkinter.tix.HORIZONTAL)
    box.add('ok', text='Ok', underline=0, width=6,
            command=lambda w=w: ok_command(w))
    box.add('cancel', text='Cancel', underline=0, width=6,
            command=lambda w=w: w.destroy())
    box.pack(side=tkinter.tix.BOTTOM, fill=tkinter.tix.X)
    top.pack(side=tkinter.tix.TOP, fill=tkinter.tix.BOTH, expand=1)

def select_month(event=None):
    # tixDemo:Status "Month = %s" % demo_month.get()
    pass

def select_year(event=None):
    # tixDemo:Status "Year = %s" % demo_year.get()
    pass

def ok_command(w):
    # tixDemo:Status "Month = %s, Year= %s" % (demo_month.get(), demo_year.get())
    w.destroy()

if __name__ == '__main__':
    root = tkinter.tix.Tk()
    RunSample(root)
    root.mainloop()
