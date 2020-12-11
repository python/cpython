#!/usr/local/bin/python
# 
# $Id: Balloon.py,v 1.1 2000/11/05 19:54:53 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "tixwidgets": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program.

# This file demonstrates the use of the tixBalloon widget, which provides
# a interesting way to give help tips about elements in your user interface.
# Your can display the help message in a "balloon" and a status bar widget.
#

import Tix

def RunSample(w):
    status = Tix.Label(w, width=40, relief=Tix.SUNKEN, bd=1)
    status.pack(side=Tix.BOTTOM, fill=Tix.Y, padx=2, pady=1)

    # Create two mysterious widgets that need balloon help
    button1 = Tix.Button(w, text='Something Unexpected',
			 command=lambda w=w: w.destroy())
    button2 = Tix.Button(w, text='Something Else Unexpected')
    button2['command'] = lambda w=button2: w.destroy()
    button1.pack(side=Tix.TOP, expand=1)
    button2.pack(side=Tix.TOP, expand=1)

    # Create the balloon widget and associate it with the widgets that we want
    # to provide tips for:
    b = Tix.Balloon(w, statusbar=status)

    b.bind_widget(button1, balloonmsg='Close Window',
		  statusmsg='Press this button to close this window')
    b.bind_widget(button2, balloonmsg='Self-destruct button',
		  statusmsg='Press this button and it will destroy itself')

if __name__ == '__main__':
    root = Tix.Tk()

    RunSample(root)
    root.mainloop()
