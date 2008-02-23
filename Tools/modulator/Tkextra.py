#! /usr/bin/env python

# A Python function that generates dialog boxes with a text message,
# optional bitmap, and any number of buttons.
# Cf. Ousterhout, Tcl and the Tk Toolkit, Figs. 27.2-3, pp. 269-270.

from Tkinter import *

mainWidget = None

def dialog(master, title, text, bitmap, default, *args):

    # 1. Create the top-level window and divide it into top
    # and bottom parts.

    w = Toplevel(master, {'class': 'Dialog'})
    w.title(title)
    w.iconname('Dialog')

    top = Frame(w, {'relief': 'raised', 'bd': 1,
                    Pack: {'side': 'top', 'fill': 'both'}})
    bot = Frame(w, {'relief': 'raised', 'bd': 1,
                    Pack: {'side': 'bottom', 'fill': 'both'}})

    # 2. Fill the top part with the bitmap and message.

    msg = Message(top,
                  {'width': '3i',
                   'text': text,
                   'font': '-Adobe-Times-Medium-R-Normal-*-180-*',
                   Pack: {'side': 'right', 'expand': 1,
                          'fill': 'both',
                          'padx': '3m', 'pady': '3m'}})
    if bitmap:
        bm = Label(top, {'bitmap': bitmap,
                         Pack: {'side': 'left',
                                'padx': '3m', 'pady': '3m'}})

    # 3. Create a row of buttons at the bottom of the dialog.

    buttons = []
    i = 0
    for but in args:
        b = Button(bot, {'text': but,
                         'command': ('set', 'button', i)})
        buttons.append(b)
        if i == default:
            bd = Frame(bot, {'relief': 'sunken', 'bd': 1,
                             Pack: {'side': 'left', 'expand': 1,
                                    'padx': '3m', 'pady': '2m'}})
            b.lift()
            b.pack ({'in': bd, 'side': 'left',
                     'padx': '2m', 'pady': '2m',
                     'ipadx': '2m', 'ipady': '1m'})
        else:
            b.pack ({'side': 'left', 'expand': 1,
                     'padx': '3m', 'pady': '3m',
                     'ipady': '2m', 'ipady': '1m'})
        i = i+1

    # 4. Set up a binding for <Return>, if there's a default,
    # set a grab, and claim the focus too.

    if default >= 0:
        w.bind('<Return>',
               lambda e, b=buttons[default], i=default:
               (b.flash(),
                b.setvar('button', i)))

    oldFocus = w.tk.call('focus') # XXX
    w.grab_set()
    w.focus()

    # 5. Wait for the user to respond, then restore the focus
    # and return the index of the selected button.

    w.waitvar('button')
    w.destroy()
    w.tk.call('focus', oldFocus) # XXX
    return w.getint(w.getvar('button'))

def strdialog(master, title, text, bitmap, default, *args):

    # 1. Create the top-level window and divide it into top
    # and bottom parts.

    w = Toplevel(master, {'class': 'Dialog'})
    w.title(title)
    w.iconname('Dialog')

    top = Frame(w, {'relief': 'raised', 'bd': 1,
                    Pack: {'side': 'top', 'fill': 'both'}})
    if args:
        bot = Frame(w, {'relief': 'raised', 'bd': 1,
                    Pack: {'side': 'bottom', 'fill': 'both'}})

    # 2. Fill the top part with the bitmap, message and input field.

    if bitmap:
        bm = Label(top, {'bitmap': bitmap,
                         Pack: {'side': 'left',
                                'padx': '3m', 'pady': '3m'}})

    msg = Message(top,
                  {'width': '3i',
                   'text': text,
                   'font': '-Adobe-Times-Medium-R-Normal-*-180-*',
                   Pack: {'side': 'left',
                          'fill': 'both',
                          'padx': '3m', 'pady': '3m'}})

    field = Entry(top,
                  {'relief':'sunken',
                   Pack:{'side':'left',
                         'fill':'x',
                         'expand':1,
                         'padx':'3m', 'pady':'3m'}})
    # 3. Create a row of buttons at the bottom of the dialog.

    buttons = []
    i = 0
    for but in args:
        b = Button(bot, {'text': but,
                         'command': ('set', 'button', i)})
        buttons.append(b)
        if i == default:
            bd = Frame(bot, {'relief': 'sunken', 'bd': 1,
                             Pack: {'side': 'left', 'expand': 1,
                                    'padx': '3m', 'pady': '2m'}})
            b.lift()
            b.pack ({'in': bd, 'side': 'left',
                     'padx': '2m', 'pady': '2m',
                     'ipadx': '2m', 'ipady': '1m'})
        else:
            b.pack ({'side': 'left', 'expand': 1,
                     'padx': '3m', 'pady': '3m',
                     'ipady': '2m', 'ipady': '1m'})
        i = i+1

    # 4. Set up a binding for <Return>, if there's a default,
    # set a grab, and claim the focus too.

    if not args:
        w.bind('<Return>', lambda arg, top=top: top.setvar('button', 0))
        field.bind('<Return>', lambda arg, top=top: top.setvar('button', 0))
    elif default >= 0:
        w.bind('<Return>',
               lambda e, b=buttons[default], i=default:
               (b.flash(),
                b.setvar('button', i)))
        field.bind('<Return>',
               lambda e, b=buttons[default], i=default:
               (b.flash(),
                b.setvar('button', i)))

    oldFocus = w.tk.call('focus') # XXX
    w.grab_set()
    field.focus()

    # 5. Wait for the user to respond, then restore the focus
    # and return the index of the selected button.

    w.waitvar('button')
    v = field.get()
    w.destroy()
    w.tk.call('focus', oldFocus) # XXX
    if args:
        return v, w.getint(w.getvar('button'))
    else:
        return v

def message(str):
    i = dialog(mainWidget, 'Message', str, '', 0, 'OK')

def askyn(str):
    i = dialog(mainWidget, 'Question', str, '', 0, 'No', 'Yes')
    return i

def askync(str):
    i = dialog(mainWidget, 'Question', str, '', 0, 'Cancel', 'No', 'Yes')
    return i-1

def askstr(str):
    i = strdialog(mainWidget, 'Question', str, '', 0)
    return i

def askfile(str):       # XXXX For now...
    i = strdialog(mainWidget, 'Question', str, '', 0)
    return i

# The rest is the test program.

def _go():
    i = dialog(mainWidget,
               'Not Responding',
               "The file server isn't responding right now; "
               "I'll keep trying.",
               '',
               -1,
               'OK')
    print 'pressed button', i
    i = dialog(mainWidget,
               'File Modified',
               'File "tcl.h" has been modified since '
               'the last time it was saved. '
               'Do you want to save it before exiting the application?',
               'warning',
               0,
               'Save File',
               'Discard Changes',
               'Return To Editor')
    print 'pressed button', i
    print message('Test of message')
    print askyn('Test of yes/no')
    print askync('Test of yes/no/cancel')
    print askstr('Type a string:')
    print strdialog(mainWidget, 'Question', 'Another string:', '',
                  0, 'Save', 'Save as text')

def _test():
    global mainWidget
    mainWidget = Frame()
    Pack.config(mainWidget)
    start = Button(mainWidget,
                   {'text': 'Press Here To Start', 'command': _go})
    start.pack()
    endit = Button(mainWidget,
                   {'text': 'Exit',
                    'command': 'exit',
                    Pack: {'fill' : 'both'}})
    mainWidget.mainloop()

if __name__ == '__main__':
    _test()
