# -*-mode: python; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
# $Id$
#
# tixwidgets.py --
#
#       For Tix, see http://tix.sourceforge.net
#
#       This is a demo program of some of the Tix widgets available in Python.
#       If you have installed Python & Tix properly, you can execute this as
#
#               % python tixwidgets.py
#

import os, os.path, sys, tkinter.tix
from tkinter.constants import *
import traceback, tkinter.messagebox

TCL_DONT_WAIT           = 1<<1
TCL_WINDOW_EVENTS       = 1<<2
TCL_FILE_EVENTS         = 1<<3
TCL_TIMER_EVENTS        = 1<<4
TCL_IDLE_EVENTS         = 1<<5
TCL_ALL_EVENTS          = 0

class Demo:
    def __init__(self, top):
        self.root = top
        self.exit = -1

        self.dir = None                         # script directory
        self.balloon = None                     # balloon widget
        self.useBalloons = tkinter.tix.StringVar()
        self.useBalloons.set('0')
        self.statusbar = None                   # status bar widget
        self.welmsg = None                      # Msg widget
        self.welfont = ''                       # font name
        self.welsize = ''                       # font size

        progname = sys.argv[0]
        dirname = os.path.dirname(progname)
        if dirname and dirname != os.curdir:
            self.dir = dirname
            index = -1
            for i in range(len(sys.path)):
                p = sys.path[i]
                if p in ("", os.curdir):
                    index = i
            if index >= 0:
                sys.path[index] = dirname
            else:
                sys.path.insert(0, dirname)
        else:
            self.dir = os.getcwd()
        sys.path.insert(0, self.dir+'/samples')

    def MkMainMenu(self):
        top = self.root
        w = tkinter.tix.Frame(top, bd=2, relief=RAISED)
        file = tkinter.tix.Menubutton(w, text='File', underline=0, takefocus=0)
        help = tkinter.tix.Menubutton(w, text='Help', underline=0, takefocus=0)
        file.pack(side=LEFT)
        help.pack(side=RIGHT)
        fm = tkinter.tix.Menu(file, tearoff=0)
        file['menu'] = fm
        hm = tkinter.tix.Menu(help, tearoff=0)
        help['menu'] = hm

        fm.add_command(label='Exit', underline=1,
                     command = lambda self=self: self.quitcmd () )
        hm.add_checkbutton(label='BalloonHelp', underline=0, command=ToggleHelp,
                           variable=self.useBalloons)
        # The trace variable option doesn't seem to work, instead I use 'command'
        #w.tk.call('trace', 'variable', self.useBalloons, 'w', ToggleHelp))

        return w

    def MkMainNotebook(self):
        top = self.root
        w = tkinter.tix.NoteBook(top, ipadx=5, ipady=5, options="""
        tagPadX 6
        tagPadY 4
        borderWidth 2
        """)
        # This may be required if there is no *Background option
        top['bg'] = w['bg']

        w.add('wel', label='Welcome', underline=0,
              createcmd=lambda w=w, name='wel': MkWelcome(w, name))
        w.add('cho', label='Choosers', underline=0,
              createcmd=lambda w=w, name='cho': MkChoosers(w, name))
        w.add('scr', label='Scrolled Widgets', underline=0,
              createcmd=lambda w=w, name='scr': MkScroll(w, name))
        w.add('mgr', label='Manager Widgets', underline=0,
              createcmd=lambda w=w, name='mgr': MkManager(w, name))
        w.add('dir', label='Directory List', underline=0,
              createcmd=lambda w=w, name='dir': MkDirList(w, name))
        w.add('exp', label='Run Sample Programs', underline=0,
              createcmd=lambda w=w, name='exp': MkSample(w, name))
        return w

    def MkMainStatus(self):
        global demo
        top = self.root

        w = tkinter.tix.Frame(top, relief=tkinter.tix.RAISED, bd=1)
        demo.statusbar = tkinter.tix.Label(w, relief=tkinter.tix.SUNKEN, bd=1)
        demo.statusbar.form(padx=3, pady=3, left=0, right='%70')
        return w

    def build(self):
        root = self.root
        z = root.winfo_toplevel()
        z.wm_title('Tix Widget Demonstration')
        if z.winfo_screenwidth() <= 800:
            z.geometry('790x590+10+10')
        else:
            z.geometry('890x640+10+10')
        demo.balloon = tkinter.tix.Balloon(root)
        frame1 = self.MkMainMenu()
        frame2 = self.MkMainNotebook()
        frame3 = self.MkMainStatus()
        frame1.pack(side=TOP, fill=X)
        frame3.pack(side=BOTTOM, fill=X)
        frame2.pack(side=TOP, expand=1, fill=BOTH, padx=4, pady=4)
        demo.balloon['statusbar'] = demo.statusbar
        z.wm_protocol("WM_DELETE_WINDOW", lambda self=self: self.quitcmd())

        # To show Tcl errors - uncomment this to see the listbox bug.
        # Tkinter defines a Tcl tkerror procedure that in effect
        # silences all background Tcl error reporting.
        # root.tk.eval('if {[info commands tkerror] != ""} {rename tkerror pytkerror}')
    def quitcmd (self):
        """Quit our mainloop. It is up to you to call root.destroy() after."""
        self.exit = 0

    def loop(self):
        """This is an explict replacement for _tkinter mainloop()
        It lets you catch keyboard interrupts easier, and avoids
        the 20 msec. dead sleep() which burns a constant CPU."""
        while self.exit < 0:
            # There are 2 whiles here. The outer one lets you continue
            # after a ^C interrupt.
            try:
                # This is the replacement for _tkinter mainloop()
                # It blocks waiting for the next Tcl event using select.
                while self.exit < 0:
                    self.root.tk.dooneevent(TCL_ALL_EVENTS)
            except SystemExit:
                # Tkinter uses SystemExit to exit
                #print 'Exit'
                self.exit = 1
                return
            except KeyboardInterrupt:
                if tkinter.messagebox.askquestion ('Interrupt', 'Really Quit?') == 'yes':
                    # self.tk.eval('exit')
                    self.exit = 1
                    return
                continue
            except:
                # Otherwise it's some other error - be nice and say why
                t, v, tb = sys.exc_info()
                text = ""
                for line in traceback.format_exception(t,v,tb):
                    text += line + '\n'
                try: tkinter.messagebox.showerror ('Error', text)
                except: pass
                self.exit = 1
                raise SystemExit(1)

    def destroy (self):
        self.root.destroy()

def RunMain(root):
    global demo

    demo = Demo(root)

    demo.build()
    demo.loop()
    demo.destroy()

# Tabs
def MkWelcome(nb, name):
    w = nb.page(name)
    bar = MkWelcomeBar(w)
    text = MkWelcomeText(w)
    bar.pack(side=TOP, fill=X, padx=2, pady=2)
    text.pack(side=TOP, fill=BOTH, expand=1)

def MkWelcomeBar(top):
    global demo

    w = tkinter.tix.Frame(top, bd=2, relief=tkinter.tix.GROOVE)
    b1 = tkinter.tix.ComboBox(w, command=lambda w=top: MainTextFont(w))
    b2 = tkinter.tix.ComboBox(w, command=lambda w=top: MainTextFont(w))
    b1.entry['width'] = 15
    b1.slistbox.listbox['height'] = 3
    b2.entry['width'] = 4
    b2.slistbox.listbox['height'] = 3

    demo.welfont = b1
    demo.welsize = b2

    b1.insert(tkinter.tix.END, 'Courier')
    b1.insert(tkinter.tix.END, 'Helvetica')
    b1.insert(tkinter.tix.END, 'Lucida')
    b1.insert(tkinter.tix.END, 'Times Roman')

    b2.insert(tkinter.tix.END, '8')
    b2.insert(tkinter.tix.END, '10')
    b2.insert(tkinter.tix.END, '12')
    b2.insert(tkinter.tix.END, '14')
    b2.insert(tkinter.tix.END, '18')

    b1.pick(1)
    b2.pick(3)

    b1.pack(side=tkinter.tix.LEFT, padx=4, pady=4)
    b2.pack(side=tkinter.tix.LEFT, padx=4, pady=4)

    demo.balloon.bind_widget(b1, msg='Choose\na font',
                             statusmsg='Choose a font for this page')
    demo.balloon.bind_widget(b2, msg='Point size',
                             statusmsg='Choose the font size for this page')
    return w

def MkWelcomeText(top):
    global demo

    w = tkinter.tix.ScrolledWindow(top, scrollbar='auto')
    win = w.window
    text = 'Welcome to TIX in Python'
    title = tkinter.tix.Label(win,
                      bd=0, width=30, anchor=tkinter.tix.N, text=text)
    msg = tkinter.tix.Message(win,
                      bd=0, width=400, anchor=tkinter.tix.N,
                      text='Tix is a set of mega-widgets based on TK. This program \
demonstrates the widgets in the Tix widget set. You can choose the pages \
in this window to look at the corresponding widgets. \n\n\
To quit this program, choose the "File | Exit" command.\n\n\
For more information, see http://tix.sourceforge.net.')
    title.pack(expand=1, fill=tkinter.tix.BOTH, padx=10, pady=10)
    msg.pack(expand=1, fill=tkinter.tix.BOTH, padx=10, pady=10)
    demo.welmsg = msg
    return w

def MainTextFont(w):
    global demo

    if not demo.welmsg:
        return
    font = demo.welfont['value']
    point = demo.welsize['value']
    if font == 'Times Roman':
        font = 'times'
    fontstr = '%s %s' % (font, point)
    demo.welmsg['font'] = fontstr

def ToggleHelp():
    if demo.useBalloons.get() == '1':
        demo.balloon['state'] = 'both'
    else:
        demo.balloon['state'] = 'none'

def MkChoosers(nb, name):
    w = nb.page(name)
    options = "label.padX 4"

    til = tkinter.tix.LabelFrame(w, label='Chooser Widgets', options=options)
    cbx = tkinter.tix.LabelFrame(w, label='tixComboBox', options=options)
    ctl = tkinter.tix.LabelFrame(w, label='tixControl', options=options)
    sel = tkinter.tix.LabelFrame(w, label='tixSelect', options=options)
    opt = tkinter.tix.LabelFrame(w, label='tixOptionMenu', options=options)
    fil = tkinter.tix.LabelFrame(w, label='tixFileEntry', options=options)
    fbx = tkinter.tix.LabelFrame(w, label='tixFileSelectBox', options=options)
    tbr = tkinter.tix.LabelFrame(w, label='Tool Bar', options=options)

    MkTitle(til.frame)
    MkCombo(cbx.frame)
    MkControl(ctl.frame)
    MkSelect(sel.frame)
    MkOptMenu(opt.frame)
    MkFileEnt(fil.frame)
    MkFileBox(fbx.frame)
    MkToolBar(tbr.frame)

    # First column: comBox and selector
    cbx.form(top=0, left=0, right='%33')
    sel.form(left=0, right='&'+str(cbx), top=cbx)
    opt.form(left=0, right='&'+str(cbx), top=sel, bottom=-1)

    # Second column: title .. etc
    til.form(left=cbx, top=0,right='%66')
    ctl.form(left=cbx, right='&'+str(til), top=til)
    fil.form(left=cbx, right='&'+str(til), top=ctl)
    tbr.form(left=cbx, right='&'+str(til), top=fil, bottom=-1)

    #
    # Third column: file selection
    fbx.form(right=-1, top=0, left='%66')

def MkCombo(w):
    options="label.width %d label.anchor %s entry.width %d" % (10, tkinter.tix.E, 14)

    static = tkinter.tix.ComboBox(w, label='Static', editable=0, options=options)
    editable = tkinter.tix.ComboBox(w, label='Editable', editable=1, options=options)
    history = tkinter.tix.ComboBox(w, label='History', editable=1, history=1,
                           anchor=tkinter.tix.E, options=options)
    static.insert(tkinter.tix.END, 'January')
    static.insert(tkinter.tix.END, 'February')
    static.insert(tkinter.tix.END, 'March')
    static.insert(tkinter.tix.END, 'April')
    static.insert(tkinter.tix.END, 'May')
    static.insert(tkinter.tix.END, 'June')
    static.insert(tkinter.tix.END, 'July')
    static.insert(tkinter.tix.END, 'August')
    static.insert(tkinter.tix.END, 'September')
    static.insert(tkinter.tix.END, 'October')
    static.insert(tkinter.tix.END, 'November')
    static.insert(tkinter.tix.END, 'December')

    editable.insert(tkinter.tix.END, 'Angola')
    editable.insert(tkinter.tix.END, 'Bangladesh')
    editable.insert(tkinter.tix.END, 'China')
    editable.insert(tkinter.tix.END, 'Denmark')
    editable.insert(tkinter.tix.END, 'Ecuador')

    history.insert(tkinter.tix.END, '/usr/bin/ksh')
    history.insert(tkinter.tix.END, '/usr/local/lib/python')
    history.insert(tkinter.tix.END, '/var/adm')

    static.pack(side=tkinter.tix.TOP, padx=5, pady=3)
    editable.pack(side=tkinter.tix.TOP, padx=5, pady=3)
    history.pack(side=tkinter.tix.TOP, padx=5, pady=3)

states = ['Bengal', 'Delhi', 'Karnataka', 'Tamil Nadu']

def spin_cmd(w, inc):
    idx = states.index(demo_spintxt.get()) + inc
    if idx < 0:
        idx = len(states) - 1
    elif idx >= len(states):
        idx = 0
# following doesn't work.
#    return states[idx]
    demo_spintxt.set(states[idx])       # this works

def spin_validate(w):
    global states, demo_spintxt

    try:
        i = states.index(demo_spintxt.get())
    except ValueError:
        return states[0]
    return states[i]
    # why this procedure works as opposed to the previous one beats me.

def MkControl(w):
    global demo_spintxt

    options="label.width %d label.anchor %s entry.width %d" % (10, tkinter.tix.E, 13)

    demo_spintxt = tkinter.tix.StringVar()
    demo_spintxt.set(states[0])
    simple = tkinter.tix.Control(w, label='Numbers', options=options)
    spintxt = tkinter.tix.Control(w, label='States', variable=demo_spintxt,
                          options=options)
    spintxt['incrcmd'] = lambda w=spintxt: spin_cmd(w, 1)
    spintxt['decrcmd'] = lambda w=spintxt: spin_cmd(w, -1)
    spintxt['validatecmd'] = lambda w=spintxt: spin_validate(w)

    simple.pack(side=tkinter.tix.TOP, padx=5, pady=3)
    spintxt.pack(side=tkinter.tix.TOP, padx=5, pady=3)

def MkSelect(w):
    options = "label.anchor %s" % tkinter.tix.CENTER

    sel1 = tkinter.tix.Select(w, label='Mere Mortals', allowzero=1, radio=1,
                      orientation=tkinter.tix.VERTICAL,
                      labelside=tkinter.tix.TOP,
                      options=options)
    sel2 = tkinter.tix.Select(w, label='Geeks', allowzero=1, radio=0,
                      orientation=tkinter.tix.VERTICAL,
                      labelside= tkinter.tix.TOP,
                      options=options)

    sel1.add('eat', text='Eat')
    sel1.add('work', text='Work')
    sel1.add('play', text='Play')
    sel1.add('party', text='Party')
    sel1.add('sleep', text='Sleep')

    sel2.add('eat', text='Eat')
    sel2.add('prog1', text='Program')
    sel2.add('prog2', text='Program')
    sel2.add('prog3', text='Program')
    sel2.add('sleep', text='Sleep')

    sel1.pack(side=tkinter.tix.LEFT, padx=5, pady=3, fill=tkinter.tix.X)
    sel2.pack(side=tkinter.tix.LEFT, padx=5, pady=3, fill=tkinter.tix.X)

def MkOptMenu(w):
    options='menubutton.width 15 label.anchor %s' % tkinter.tix.E

    m = tkinter.tix.OptionMenu(w, label='File Format : ', options=options)
    m.add_command('text', label='Plain Text')
    m.add_command('post', label='PostScript')
    m.add_command('format', label='Formatted Text')
    m.add_command('html', label='HTML')
    m.add_command('sep')
    m.add_command('tex', label='LaTeX')
    m.add_command('rtf', label='Rich Text Format')

    m.pack(fill=tkinter.tix.X, padx=5, pady=3)

def MkFileEnt(w):
    msg = tkinter.tix.Message(w,
                      relief=tkinter.tix.FLAT, width=240, anchor=tkinter.tix.N,
                      text='Press the "open file" icon button and a TixFileSelectDialog will popup.')
    ent = tkinter.tix.FileEntry(w, label='Select a file : ')
    msg.pack(side=tkinter.tix.TOP, expand=1, fill=tkinter.tix.BOTH, padx=3, pady=3)
    ent.pack(side=tkinter.tix.TOP, fill=tkinter.tix.X, padx=3, pady=3)

def MkFileBox(w):
    """The FileSelectBox is a Motif-style box with various enhancements.
    For example, you can adjust the size of the two listboxes
    and your past selections are recorded.
    """
    msg = tkinter.tix.Message(w,
                      relief=tkinter.tix.FLAT, width=240, anchor=tkinter.tix.N,
                      text='The Tix FileSelectBox is a Motif-style box with various enhancements. For example, you can adjust the size of the two listboxes and your past selections are recorded.')
    box = tkinter.tix.FileSelectBox(w)
    msg.pack(side=tkinter.tix.TOP, expand=1, fill=tkinter.tix.BOTH, padx=3, pady=3)
    box.pack(side=tkinter.tix.TOP, fill=tkinter.tix.X, padx=3, pady=3)

def MkToolBar(w):
    """The Select widget is also good for arranging buttons in a tool bar.
    """
    global demo

    options='frame.borderWidth 1'

    msg = tkinter.tix.Message(w,
                      relief=tkinter.tix.FLAT, width=240, anchor=tkinter.tix.N,
                      text='The Select widget is also good for arranging buttons in a tool bar.')
    bar = tkinter.tix.Frame(w, bd=2, relief=tkinter.tix.RAISED)
    font = tkinter.tix.Select(w, allowzero=1, radio=0, label='', options=options)
    para = tkinter.tix.Select(w, allowzero=0, radio=1, label='', options=options)

    font.add('bold', bitmap='@' + demo.dir + '/bitmaps/bold.xbm')
    font.add('italic', bitmap='@' + demo.dir + '/bitmaps/italic.xbm')
    font.add('underline', bitmap='@' + demo.dir + '/bitmaps/underline.xbm')
    font.add('capital', bitmap='@' + demo.dir + '/bitmaps/capital.xbm')

    para.add('left', bitmap='@' + demo.dir + '/bitmaps/leftj.xbm')
    para.add('right', bitmap='@' + demo.dir + '/bitmaps/rightj.xbm')
    para.add('center', bitmap='@' + demo.dir + '/bitmaps/centerj.xbm')
    para.add('justify', bitmap='@' + demo.dir + '/bitmaps/justify.xbm')

    msg.pack(side=tkinter.tix.TOP, expand=1, fill=tkinter.tix.BOTH, padx=3, pady=3)
    bar.pack(side=tkinter.tix.TOP, fill=tkinter.tix.X, padx=3, pady=3)
    font.pack({'in':bar}, side=tkinter.tix.LEFT, padx=3, pady=3)
    para.pack({'in':bar}, side=tkinter.tix.LEFT, padx=3, pady=3)

def MkTitle(w):
    msg = tkinter.tix.Message(w,
                      relief=tkinter.tix.FLAT, width=240, anchor=tkinter.tix.N,
                      text='There are many types of "chooser" widgets that allow the user to input different types of information')
    msg.pack(side=tkinter.tix.TOP, expand=1, fill=tkinter.tix.BOTH, padx=3, pady=3)

def MkScroll(nb, name):
    w = nb.page(name)
    options='label.padX 4'

    sls = tkinter.tix.LabelFrame(w, label='Tix.ScrolledListBox', options=options)
    swn = tkinter.tix.LabelFrame(w, label='Tix.ScrolledWindow', options=options)
    stx = tkinter.tix.LabelFrame(w, label='Tix.ScrolledText', options=options)

    MkSList(sls.frame)
    MkSWindow(swn.frame)
    MkSText(stx.frame)

    sls.form(top=0, left=0, right='%33', bottom=-1)
    swn.form(top=0, left=sls, right='%66', bottom=-1)
    stx.form(top=0, left=swn, right=-1, bottom=-1)


def MkSList(w):
    """This TixScrolledListBox is configured so that it uses scrollbars
    only when it is necessary. Use the handles to resize the listbox and
    watch the scrollbars automatically appear and disappear.  """
    top = tkinter.tix.Frame(w, width=300, height=330)
    bot = tkinter.tix.Frame(w)
    msg = tkinter.tix.Message(top,
                      relief=tkinter.tix.FLAT, width=200, anchor=tkinter.tix.N,
                      text='This TixScrolledListBox is configured so that it uses scrollbars only when it is necessary. Use the handles to resize the listbox and watch the scrollbars automatically appear and disappear.')

    list = tkinter.tix.ScrolledListBox(top, scrollbar='auto')
    list.place(x=50, y=150, width=120, height=80)
    list.listbox.insert(tkinter.tix.END, 'Alabama')
    list.listbox.insert(tkinter.tix.END, 'California')
    list.listbox.insert(tkinter.tix.END, 'Montana')
    list.listbox.insert(tkinter.tix.END, 'New Jersey')
    list.listbox.insert(tkinter.tix.END, 'New York')
    list.listbox.insert(tkinter.tix.END, 'Pennsylvania')
    list.listbox.insert(tkinter.tix.END, 'Washington')

    rh = tkinter.tix.ResizeHandle(top, bg='black',
                          relief=tkinter.tix.RAISED,
                          handlesize=8, gridded=1, minwidth=50, minheight=30)
    btn = tkinter.tix.Button(bot, text='Reset', command=lambda w=rh, x=list: SList_reset(w,x))
    top.propagate(0)
    msg.pack(fill=tkinter.tix.X)
    btn.pack(anchor=tkinter.tix.CENTER)
    top.pack(expand=1, fill=tkinter.tix.BOTH)
    bot.pack(fill=tkinter.tix.BOTH)
    list.bind('<Map>', func=lambda arg=0, rh=rh, list=list:
              list.tk.call('tixDoWhenIdle', str(rh), 'attachwidget', str(list)))

def SList_reset(rh, list):
    list.place(x=50, y=150, width=120, height=80)
    list.update()
    rh.attach_widget(list)

def MkSWindow(w):
    """The ScrolledWindow widget allows you to scroll any kind of Tk
    widget. It is more versatile than a scrolled canvas widget.
    """
    global demo

    text = 'The Tix ScrolledWindow widget allows you to scroll any kind of Tk widget. It is more versatile than a scrolled canvas widget.'

    file = os.path.join(demo.dir, 'bitmaps', 'tix.gif')
    if not os.path.isfile(file):
        text += ' (Image missing)'

    top = tkinter.tix.Frame(w, width=330, height=330)
    bot = tkinter.tix.Frame(w)
    msg = tkinter.tix.Message(top,
                      relief=tkinter.tix.FLAT, width=200, anchor=tkinter.tix.N,
                      text=text)

    win = tkinter.tix.ScrolledWindow(top, scrollbar='auto')

    image1 = win.window.image_create('photo', file=file)
    lbl = tkinter.tix.Label(win.window, image=image1)
    lbl.pack(expand=1, fill=tkinter.tix.BOTH)

    win.place(x=30, y=150, width=190, height=120)

    rh = tkinter.tix.ResizeHandle(top, bg='black',
                          relief=tkinter.tix.RAISED,
                          handlesize=8, gridded=1, minwidth=50, minheight=30)
    btn = tkinter.tix.Button(bot, text='Reset', command=lambda w=rh, x=win: SWindow_reset(w,x))
    top.propagate(0)
    msg.pack(fill=tkinter.tix.X)
    btn.pack(anchor=tkinter.tix.CENTER)
    top.pack(expand=1, fill=tkinter.tix.BOTH)
    bot.pack(fill=tkinter.tix.BOTH)

    win.bind('<Map>', func=lambda arg=0, rh=rh, win=win:
             win.tk.call('tixDoWhenIdle', str(rh), 'attachwidget', str(win)))

def SWindow_reset(rh, win):
    win.place(x=30, y=150, width=190, height=120)
    win.update()
    rh.attach_widget(win)

def MkSText(w):
    """The TixScrolledWindow widget allows you to scroll any kind of Tk
    widget. It is more versatile than a scrolled canvas widget."""
    top = tkinter.tix.Frame(w, width=330, height=330)
    bot = tkinter.tix.Frame(w)
    msg = tkinter.tix.Message(top,
                      relief=tkinter.tix.FLAT, width=200, anchor=tkinter.tix.N,
                      text='The Tix ScrolledWindow widget allows you to scroll any kind of Tk widget. It is more versatile than a scrolled canvas widget.')

    win = tkinter.tix.ScrolledText(top, scrollbar='auto')
    win.text['wrap'] = 'none'
    win.text.insert(tkinter.tix.END, '''When -scrollbar is set to "auto", the
scrollbars are shown only when needed.
Additional modifiers can be used to force a
scrollbar to be shown or hidden. For example,
"auto -y" means the horizontal scrollbar
should be shown when needed but the vertical
scrollbar should always be hidden;
"auto +x" means the vertical scrollbar
should be shown when needed but the horizontal
scrollbar should always be shown, and so on.'''
)
    win.place(x=30, y=150, width=190, height=100)

    rh = tkinter.tix.ResizeHandle(top, bg='black',
                          relief=tkinter.tix.RAISED,
                          handlesize=8, gridded=1, minwidth=50, minheight=30)
    btn = tkinter.tix.Button(bot, text='Reset', command=lambda w=rh, x=win: SText_reset(w,x))
    top.propagate(0)
    msg.pack(fill=tkinter.tix.X)
    btn.pack(anchor=tkinter.tix.CENTER)
    top.pack(expand=1, fill=tkinter.tix.BOTH)
    bot.pack(fill=tkinter.tix.BOTH)
    win.bind('<Map>', func=lambda arg=0, rh=rh, win=win:
             win.tk.call('tixDoWhenIdle', str(rh), 'attachwidget', str(win)))

def SText_reset(rh, win):
    win.place(x=30, y=150, width=190, height=120)
    win.update()
    rh.attach_widget(win)

def MkManager(nb, name):
    w = nb.page(name)
    options='label.padX 4'

    pane = tkinter.tix.LabelFrame(w, label='Tix.PanedWindow', options=options)
    note = tkinter.tix.LabelFrame(w, label='Tix.NoteBook', options=options)

    MkPanedWindow(pane.frame)
    MkNoteBook(note.frame)

    pane.form(top=0, left=0, right=note, bottom=-1)
    note.form(top=0, right=-1, bottom=-1)

def MkPanedWindow(w):
    """The PanedWindow widget allows the user to interactively manipulate
    the sizes of several panes. The panes can be arranged either vertically
    or horizontally.
    """
    msg = tkinter.tix.Message(w,
                      relief=tkinter.tix.FLAT, width=240, anchor=tkinter.tix.N,
                      text='The PanedWindow widget allows the user to interactively manipulate the sizes of several panes. The panes can be arranged either vertically or horizontally.')
    group = tkinter.tix.LabelEntry(w, label='Newsgroup:', options='entry.width 25')
    group.entry.insert(0,'comp.lang.python')
    pane = tkinter.tix.PanedWindow(w, orientation='vertical')

    p1 = pane.add('list', min=70, size=100)
    p2 = pane.add('text', min=70)
    list = tkinter.tix.ScrolledListBox(p1)
    text = tkinter.tix.ScrolledText(p2)

    list.listbox.insert(tkinter.tix.END, "  12324 Re: Tkinter is good for your health")
    list.listbox.insert(tkinter.tix.END, "+ 12325 Re: Tkinter is good for your health")
    list.listbox.insert(tkinter.tix.END, "+ 12326 Re: Tix is even better for your health (Was: Tkinter is good...)")
    list.listbox.insert(tkinter.tix.END, "  12327 Re: Tix is even better for your health (Was: Tkinter is good...)")
    list.listbox.insert(tkinter.tix.END, "+ 12328 Re: Tix is even better for your health (Was: Tkinter is good...)")
    list.listbox.insert(tkinter.tix.END, "  12329 Re: Tix is even better for your health (Was: Tkinter is good...)")
    list.listbox.insert(tkinter.tix.END, "+ 12330 Re: Tix is even better for your health (Was: Tkinter is good...)")

    text.text['bg'] = list.listbox['bg']
    text.text['wrap'] = 'none'
    text.text.insert(tkinter.tix.END, """
Mon, 19 Jun 1995 11:39:52        comp.lang.python              Thread   34 of  220
Lines 353       A new way to put text and bitmaps together iNo responses
ioi@blue.seas.upenn.edu                Ioi K. Lam at University of Pennsylvania

Hi,

I have implemented a new image type called "compound". It allows you
to glue together a bunch of bitmaps, images and text strings together
to form a bigger image. Then you can use this image with widgets that
support the -image option. For example, you can display a text string string
together with a bitmap, at the same time, inside a TK button widget.
""")
    list.pack(expand=1, fill=tkinter.tix.BOTH, padx=4, pady=6)
    text.pack(expand=1, fill=tkinter.tix.BOTH, padx=4, pady=6)

    msg.pack(side=tkinter.tix.TOP, padx=3, pady=3, fill=tkinter.tix.BOTH)
    group.pack(side=tkinter.tix.TOP, padx=3, pady=3, fill=tkinter.tix.BOTH)
    pane.pack(side=tkinter.tix.TOP, padx=3, pady=3, fill=tkinter.tix.BOTH, expand=1)

def MkNoteBook(w):
    msg = tkinter.tix.Message(w,
                      relief=tkinter.tix.FLAT, width=240, anchor=tkinter.tix.N,
                      text='The NoteBook widget allows you to layout a complex interface into individual pages.')
    # prefix = Tix.OptionName(w)
    # if not prefix: prefix = ''
    # w.option_add('*' + prefix + '*TixNoteBook*tagPadX', 8)
    options = "entry.width %d label.width %d label.anchor %s" % (10, 18, tkinter.tix.E)

    nb = tkinter.tix.NoteBook(w, ipadx=6, ipady=6, options=options)
    nb.add('hard_disk', label="Hard Disk", underline=0)
    nb.add('network', label="Network", underline=0)

    # Frame for the buttons that are present on all pages
    common = tkinter.tix.Frame(nb.hard_disk)
    common.pack(side=tkinter.tix.RIGHT, padx=2, pady=2, fill=tkinter.tix.Y)
    CreateCommonButtons(common)

    # Widgets belonging only to this page
    a = tkinter.tix.Control(nb.hard_disk, value=12, label='Access Time: ')
    w = tkinter.tix.Control(nb.hard_disk, value=400, label='Write Throughput: ')
    r = tkinter.tix.Control(nb.hard_disk, value=400, label='Read Throughput: ')
    c = tkinter.tix.Control(nb.hard_disk, value=1021, label='Capacity: ')
    a.pack(side=tkinter.tix.TOP, padx=20, pady=2)
    w.pack(side=tkinter.tix.TOP, padx=20, pady=2)
    r.pack(side=tkinter.tix.TOP, padx=20, pady=2)
    c.pack(side=tkinter.tix.TOP, padx=20, pady=2)

    common = tkinter.tix.Frame(nb.network)
    common.pack(side=tkinter.tix.RIGHT, padx=2, pady=2, fill=tkinter.tix.Y)
    CreateCommonButtons(common)

    a = tkinter.tix.Control(nb.network, value=12, label='Access Time: ')
    w = tkinter.tix.Control(nb.network, value=400, label='Write Throughput: ')
    r = tkinter.tix.Control(nb.network, value=400, label='Read Throughput: ')
    c = tkinter.tix.Control(nb.network, value=1021, label='Capacity: ')
    u = tkinter.tix.Control(nb.network, value=10, label='Users: ')
    a.pack(side=tkinter.tix.TOP, padx=20, pady=2)
    w.pack(side=tkinter.tix.TOP, padx=20, pady=2)
    r.pack(side=tkinter.tix.TOP, padx=20, pady=2)
    c.pack(side=tkinter.tix.TOP, padx=20, pady=2)
    u.pack(side=tkinter.tix.TOP, padx=20, pady=2)

    msg.pack(side=tkinter.tix.TOP, padx=3, pady=3, fill=tkinter.tix.BOTH)
    nb.pack(side=tkinter.tix.TOP, padx=5, pady=5, fill=tkinter.tix.BOTH, expand=1)

def CreateCommonButtons(f):
    ok = tkinter.tix.Button(f, text='OK', width = 6)
    cancel = tkinter.tix.Button(f, text='Cancel', width = 6)
    ok.pack(side=tkinter.tix.TOP, padx=2, pady=2)
    cancel.pack(side=tkinter.tix.TOP, padx=2, pady=2)

def MkDirList(nb, name):
    w = nb.page(name)
    options = "label.padX 4"

    dir = tkinter.tix.LabelFrame(w, label='Tix.DirList', options=options)
    fsbox = tkinter.tix.LabelFrame(w, label='Tix.ExFileSelectBox', options=options)
    MkDirListWidget(dir.frame)
    MkExFileWidget(fsbox.frame)
    dir.form(top=0, left=0, right='%40', bottom=-1)
    fsbox.form(top=0, left='%40', right=-1, bottom=-1)

def MkDirListWidget(w):
    """The TixDirList widget gives a graphical representation of the file
    system directory and makes it easy for the user to choose and access
    directories.
    """
    msg = tkinter.tix.Message(w,
                      relief=tkinter.tix.FLAT, width=240, anchor=tkinter.tix.N,
                      text='The Tix DirList widget gives a graphical representation of the file system directory and makes it easy for the user to choose and access directories.')
    dirlist = tkinter.tix.DirList(w, options='hlist.padY 1 hlist.width 25 hlist.height 16')
    msg.pack(side=tkinter.tix.TOP, expand=1, fill=tkinter.tix.BOTH, padx=3, pady=3)
    dirlist.pack(side=tkinter.tix.TOP, padx=3, pady=3)

def MkExFileWidget(w):
    """The TixExFileSelectBox widget is more user friendly than the Motif
    style FileSelectBox.  """
    msg = tkinter.tix.Message(w,
                      relief=tkinter.tix.FLAT, width=240, anchor=tkinter.tix.N,
                      text='The Tix ExFileSelectBox widget is more user friendly than the Motif style FileSelectBox.')
    # There's a bug in the ComboBoxes - the scrolledlistbox is destroyed
    box = tkinter.tix.ExFileSelectBox(w, bd=2, relief=tkinter.tix.RAISED)
    msg.pack(side=tkinter.tix.TOP, expand=1, fill=tkinter.tix.BOTH, padx=3, pady=3)
    box.pack(side=tkinter.tix.TOP, padx=3, pady=3)

###
### List of all the demos we want to show off
comments = {'widget' : 'Widget Demos', 'image' : 'Image Demos'}
samples = {'Balloon'            : 'Balloon',
           'Button Box'         : 'BtnBox',
           'Combo Box'          : 'ComboBox',
           'Compound Image'     : 'CmpImg',
           'Directory List'     : 'DirList',
           'Directory Tree'     : 'DirTree',
           'Control'            : 'Control',
           'Notebook'           : 'NoteBook',
           'Option Menu'        : 'OptMenu',
           'Paned Window'       : 'PanedWin',
           'Popup Menu'         : 'PopMenu',
           'ScrolledHList (1)'  : 'SHList1',
           'ScrolledHList (2)'  : 'SHList2',
           'Tree (dynamic)'     : 'Tree'
}

# There are still a lot of demos to be translated:
##      set root {
##          {d "File Selectors"         file    }
##          {d "Hierachical ListBox"    hlist   }
##          {d "Tabular ListBox"        tlist   {c tixTList}}
##          {d "Grid Widget"            grid    {c tixGrid}}
##          {d "Manager Widgets"        manager }
##          {d "Scrolled Widgets"       scroll  }
##          {d "Miscellaneous Widgets"  misc    }
##          {d "Image Types"            image   }
##      }
##
##      set image {
##          {d "Compound Image"         cmpimg  }
##          {d "XPM Image"              xpm     {i pixmap}}
##      }
##
##      set cmpimg {
##done      {f "In Buttons"             CmpImg.tcl      }
##          {f "In NoteBook"            CmpImg2.tcl     }
##          {f "Notebook Color Tabs"    CmpImg4.tcl     }
##          {f "Icons"                  CmpImg3.tcl     }
##      }
##
##      set xpm {
##          {f "In Button"              Xpm.tcl         {i pixmap}}
##          {f "In Menu"                Xpm1.tcl        {i pixmap}}
##      }
##
##      set file {
##added     {f DirList                          DirList.tcl     }
##added     {f DirTree                          DirTree.tcl     }
##          {f DirSelectDialog                  DirDlg.tcl      }
##          {f ExFileSelectDialog               EFileDlg.tcl    }
##          {f FileSelectDialog                 FileDlg.tcl     }
##          {f FileEntry                        FileEnt.tcl     }
##      }
##
##      set hlist {
##          {f HList                    HList1.tcl      }
##          {f CheckList                ChkList.tcl     {c tixCheckList}}
##done      {f "ScrolledHList (1)"      SHList.tcl      }
##done      {f "ScrolledHList (2)"      SHList2.tcl     }
##done      {f Tree                     Tree.tcl        }
##done      {f "Tree (Dynamic)"         DynTree.tcl     {v win}}
##      }
##
##      set tlist {
##          {f "ScrolledTList (1)"      STList1.tcl     {c tixTList}}
##          {f "ScrolledTList (2)"      STList2.tcl     {c tixTList}}
##      }
##      global tcl_platform
##      #  This demo hangs windows
##      if {$tcl_platform(platform) != "windows"} {
##na    lappend tlist     {f "TList File Viewer"        STList3.tcl     {c tixTList}}
##      }
##
##      set grid {
##na        {f "Simple Grid"            SGrid0.tcl      {c tixGrid}}
##na        {f "ScrolledGrid"           SGrid1.tcl      {c tixGrid}}
##na        {f "Editable Grid"          EditGrid.tcl    {c tixGrid}}
##      }
##
##      set scroll {
##          {f ScrolledListBox          SListBox.tcl    }
##          {f ScrolledText             SText.tcl       }
##          {f ScrolledWindow           SWindow.tcl     }
##na        {f "Canvas Object View"     CObjView.tcl    {c tixCObjView}}
##      }
##
##      set manager {
##          {f ListNoteBook             ListNBK.tcl     }
##done      {f NoteBook                 NoteBook.tcl    }
##done      {f PanedWindow              PanedWin.tcl    }
##      }
##
##      set misc {
##done      {f Balloon                  Balloon.tcl     }
##done      {f ButtonBox                BtnBox.tcl      }
##done      {f ComboBox                 ComboBox.tcl    }
##done      {f Control                  Control.tcl     }
##          {f LabelEntry               LabEntry.tcl    }
##          {f LabelFrame               LabFrame.tcl    }
##          {f Meter                    Meter.tcl       {c tixMeter}}
##done      {f OptionMenu               OptMenu.tcl     }
##done      {f PopupMenu                PopMenu.tcl     }
##          {f Select                   Select.tcl      }
##          {f StdButtonBox             StdBBox.tcl     }
##      }
##

stypes = {}
stypes['widget'] = ['Balloon', 'Button Box', 'Combo Box', 'Control',
                    'Directory List', 'Directory Tree',
                    'Notebook', 'Option Menu', 'Popup Menu', 'Paned Window',
                    'ScrolledHList (1)', 'ScrolledHList (2)', 'Tree (dynamic)']
stypes['image'] = ['Compound Image']

def MkSample(nb, name):
    w = nb.page(name)
    options = "label.padX 4"

    pane = tkinter.tix.PanedWindow(w, orientation='horizontal')
    pane.pack(side=tkinter.tix.TOP, expand=1, fill=tkinter.tix.BOTH)
    f1 = pane.add('list', expand='1')
    f2 = pane.add('text', expand='5')
    f1['relief'] = 'flat'
    f2['relief'] = 'flat'

    lab = tkinter.tix.LabelFrame(f1, label='Select a sample program:')
    lab.pack(side=tkinter.tix.TOP, expand=1, fill=tkinter.tix.BOTH, padx=5, pady=5)
    lab1 = tkinter.tix.LabelFrame(f2, label='Source:')
    lab1.pack(side=tkinter.tix.TOP, expand=1, fill=tkinter.tix.BOTH, padx=5, pady=5)

    slb = tkinter.tix.Tree(lab.frame, options='hlist.width 20')
    slb.pack(side=tkinter.tix.TOP, expand=1, fill=tkinter.tix.BOTH, padx=5)

    stext = tkinter.tix.ScrolledText(lab1.frame, name='stext')
    font = root.tk.eval('tix option get fixed_font')
    stext.text.config(font=font)

    frame = tkinter.tix.Frame(lab1.frame, name='frame')

    run = tkinter.tix.Button(frame, text='Run ...', name='run')
    view = tkinter.tix.Button(frame, text='View Source ...', name='view')
    run.pack(side=tkinter.tix.LEFT, expand=0, fill=tkinter.tix.NONE)
    view.pack(side=tkinter.tix.LEFT, expand=0, fill=tkinter.tix.NONE)

    stext.text['bg'] = slb.hlist['bg']
    stext.text['state'] = 'disabled'
    stext.text['wrap'] = 'none'
    stext.text['width'] = 80

    frame.pack(side=tkinter.tix.BOTTOM, expand=0, fill=tkinter.tix.X, padx=7)
    stext.pack(side=tkinter.tix.TOP, expand=0, fill=tkinter.tix.BOTH, padx=7)

    slb.hlist['separator'] = '.'
    slb.hlist['width'] = 25
    slb.hlist['drawbranch'] = 0
    slb.hlist['indent'] = 10
    slb.hlist['wideselect'] = 1
    slb.hlist['command'] = lambda args=0, w=w,slb=slb,stext=stext,run=run,view=view: Sample_Action(w, slb, stext, run, view, 'run')
    slb.hlist['browsecmd'] = lambda args=0, w=w,slb=slb,stext=stext,run=run,view=view: Sample_Action(w, slb, stext, run, view, 'browse')

    run['command']      = lambda args=0, w=w,slb=slb,stext=stext,run=run,view=view: Sample_Action(w, slb, stext, run, view, 'run')
    view['command'] = lambda args=0, w=w,slb=slb,stext=stext,run=run,view=view: Sample_Action(w, slb, stext, run, view, 'view')

    for type in ['widget', 'image']:
        if type != 'widget':
            x = tkinter.tix.Frame(slb.hlist, bd=2, height=2, width=150,
                          relief=tkinter.tix.SUNKEN, bg=slb.hlist['bg'])
            slb.hlist.add_child(itemtype=tkinter.tix.WINDOW, window=x, state='disabled')
        x = slb.hlist.add_child(itemtype=tkinter.tix.TEXT, state='disabled',
                                text=comments[type])
        for key in stypes[type]:
            slb.hlist.add_child(x, itemtype=tkinter.tix.TEXT, data=key,
                                text=key)
    slb.hlist.selection_clear()

    run['state'] = 'disabled'
    view['state'] = 'disabled'

def Sample_Action(w, slb, stext, run, view, action):
    global demo

    hlist = slb.hlist
    anchor = hlist.info_anchor()
    if not anchor:
        run['state'] = 'disabled'
        view['state'] = 'disabled'
    elif not hlist.info_parent(anchor):
        # a comment
        return

    run['state'] = 'normal'
    view['state'] = 'normal'
    key = hlist.info_data(anchor)
    title = key
    prog = samples[key]

    if action == 'run':
        exec('import ' + prog)
        w = tkinter.tix.Toplevel()
        w.title(title)
        rtn = eval(prog + '.RunSample')
        rtn(w)
    elif action == 'view':
        w = tkinter.tix.Toplevel()
        w.title('Source view: ' + title)
        LoadFile(w, demo.dir + '/samples/' + prog + '.py')
    elif action == 'browse':
        ReadFile(stext.text, demo.dir + '/samples/' + prog + '.py')

def LoadFile(w, fname):
    global root
    b = tkinter.tix.Button(w, text='Close', command=w.destroy)
    t = tkinter.tix.ScrolledText(w)
    #    b.form(left=0, bottom=0, padx=4, pady=4)
    #    t.form(left=0, bottom=b, right='-0', top=0)
    t.pack()
    b.pack()

    font = root.tk.eval('tix option get fixed_font')
    t.text.config(font=font)
    t.text['bd'] = 2
    t.text['wrap'] = 'none'

    ReadFile(t.text, fname)

def ReadFile(w, fname):
    old_state = w['state']
    w['state'] = 'normal'
    w.delete('0.0', tkinter.tix.END)

    try:
        f = open(fname)
        lines = f.readlines()
        for s in lines:
            w.insert(tkinter.tix.END, s)
        f.close()
    finally:
#       w.see('1.0')
        w['state'] = old_state

if __name__ == '__main__':
    root = tkinter.tix.Tk()
    RunMain(root)
