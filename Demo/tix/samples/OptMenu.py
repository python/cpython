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

# This file demonstrates the use of the tixOptionMenu widget -- you can
# use it for the user to choose from a fixed set of options
#
import Tix

options = {'text':'Plain Text', 'post':'PostScript', 'html':'HTML',
           'tex':'LaTeX', 'rtf':'Rich Text Format'}

def RunSample(w):
    global demo_opt_from, demo_opt_to

    demo_opt_from = Tix.StringVar()
    demo_opt_to = Tix.StringVar()

    top = Tix.Frame(w, bd=1, relief=Tix.RAISED)

    from_file = Tix.OptionMenu(top, label="From File Format : ",
                               variable=demo_opt_from,
                               options = 'label.width  19 label.anchor e menubutton.width 15')

    to_file = Tix.OptionMenu(top, label="To File Format : ",
                             variable=demo_opt_to,
                             options='label.width  19 label.anchor e menubutton.width 15')

    # Add the available options to the two OptionMenu widgets
    #
    # [Hint] You have to add the options first before you set the
    #        global variables "demo_opt_from" and "demo_opt_to". Otherwise
    #        the OptionMenu widget will complain about "unknown options"!
    #
    for opt in list(options.keys()):
        from_file.add_command(opt, label=options[opt])
        to_file.add_command(opt, label=options[opt])

    demo_opt_from.set('html')
    demo_opt_to.set('post')

    from_file.pack(side=Tix.TOP, anchor=Tix.W, pady=3, padx=6)
    to_file.pack(side=Tix.TOP, anchor=Tix.W, pady=3, padx=6)

    box = Tix.ButtonBox(w, orientation=Tix.HORIZONTAL)
    box.add('ok', text='Ok', underline=0, width=6,
            command=lambda w=w: ok_command(w))
    box.add('cancel', text='Cancel', underline=0, width=6,
            command=lambda w=w: w.destroy())
    box.pack(side=Tix.BOTTOM, fill=Tix.X)
    top.pack(side=Tix.TOP, fill=Tix.BOTH, expand=1)

def ok_command(w):
    # tixDemo:Status "Convert file from %s to %s" % ( demo_opt_from.get(), demo_opt_to.get())
    w.destroy()

if __name__ == '__main__':
    root = Tix.Tk()
    RunSample(root)
    root.mainloop()
