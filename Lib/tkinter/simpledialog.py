#
# An Introduction to Tkinter
#
# Copyright (c) 1997 by Fredrik Lundh
#
# This copyright applies to Dialog, askinteger, askfloat and asktring
#
# fredrik@pythonware.com
# http://www.pythonware.com
#
"""This module handles dialog boxes.

It contains the following public symbols:

SimpleDialog -- A simple but flexible modal dialog box

Dialog -- a base class for dialogs

askinteger -- get an integer from the user

askfloat -- get a float from the user

askstring -- get a string from the user
"""

import tkinter
from tkinter import Button, Label, Tk, Toplevel, TclError
from tkinter import _get_temp_root, _destroy_temp_root
from tkinter import messagebox
from tkinter import ttk
from tkinter.constants import *
import contextlib

__all__ = ["SimpleDialog", "Dialog", "askinteger", "askfloat", "askstring"]


# The standard dialog icons, which tk::MessageBox draws with themed images
# instead of the classic monochrome bitmaps.
_ICON_IMAGES = {
    'error': '::tk::icons::error',
    'info': '::tk::icons::information',
    'question': '::tk::icons::question',
    'warning': '::tk::icons::warning',
}


# Based on the Tk ::tk_dialog procedure, themed like ::tk::MessageBox by
# default.
class SimpleDialog:

    def _widget(self, klass, master, **kw):
        # Create a themed (ttk) or classic (tkinter) widget.
        return getattr(ttk if self.use_ttk else tkinter, klass)(master, **kw)

    def __init__(self, master,
                 text='', buttons=[], default=None, cancel=None,
                 title=None, class_=None, *, bitmap=None, detail='',
                 use_ttk=True):
        # Use the themed (ttk) widgets (modelled on tk::MessageBox) by default,
        # or the classic Tk widgets (tk_dialog) if use_ttk is false.
        self.use_ttk = use_ttk

        # 1. Create the top-level window and divide it into top
        # and bottom parts.

        class_ = class_ or 'Dialog'
        self.root = Toplevel(master, class_=class_)
        # The default value of the title is space (" ") not the empty string
        # because for some window managers, a
        #           w.title("")
        # causes the window title to be w._name instead of the empty string.
        self.root.title(title or ' ')
        self.root.iconname(class_)
        toplevel = master.winfo_toplevel()
        if toplevel.winfo_viewable():
            self.root.wm_transient(toplevel)
        _setup_dialog(self.root)
        if self.use_ttk:
            self.root.configure(
                background=ttk.Style(self.root).lookup('.', 'background'))

        bot = self._widget('Frame', self.root, name='bot')
        top = self._widget('Frame', self.root, name='top')
        # The classic dialog (tk_dialog) gives its frames a raised border on
        # X11; the themed one (tk::MessageBox) does not.
        if not self.use_ttk and self.root._windowingsystem == 'x11':
            bot.configure(relief=RAISED, bd=1)
            top.configure(relief=RAISED, bd=1)
        bot.pack(side=BOTTOM, fill=BOTH)
        top.pack(side=TOP, fill=BOTH, expand=1)
        bot.grid_anchor(CENTER)

        # 2. Fill the top part with bitmap and message (use the option
        # database for -wraplength and -font so that they can be
        # overridden by the caller).

        master.option_add(f'*{class_}.msg.wrapLength', '3i', 'widgetDefault')
        master.option_add(f'*{class_}.msg.font', 'TkCaptionFont', 'widgetDefault')
        master.option_add(f'*{class_}.dtl.wrapLength', '3i', 'widgetDefault')
        master.option_add(f'*{class_}.dtl.font', 'TkDefaultFont', 'widgetDefault')

        # tk::MessageBox and tk_dialog pad the top part differently.
        pad = '2m' if self.use_ttk else '3m'

        # The bitmap is packed first to claim the whole left side; the message
        # and detail stack on its right, as in tk::MessageBox.
        if bitmap:
            if self.root._windowingsystem == 'aqua' and bitmap == 'error':
                bitmap = 'stop'
            image = _ICON_IMAGES.get(bitmap) if self.use_ttk else None
            if image is not None and self.root.winfo_depth() >= 4:
                # tk::MessageBox draws the standard icons with themed images.
                self.bitmap = ttk.Label(self.root, name='bitmap', image=image)
            else:
                # ttk.Label has no -bitmap option, so use a classic label.
                self.bitmap = Label(self.root, name='bitmap', bitmap=bitmap)
            # The themed dialog anchors the icon to the top (like the bitmap's
            # "nw" sticky in tk::MessageBox); the classic one centers it.
            anchor = N if self.use_ttk else CENTER
            self.bitmap.pack(in_=top, side=LEFT, anchor=anchor,
                             padx=pad, pady=pad)

        self.message = self._widget('Label', self.root, name='msg',
                                    justify=LEFT, text=text)
        self.detail = None
        if self.use_ttk:
            # tk::MessageBox anchors the message to the top-left corner.
            self.message.configure(anchor=NW)
        # The message expands to fill the space, unless there is a detail
        # message below it which takes the extra space instead (cf.
        # tk::MessageBox).
        self.message.pack(in_=top, side=TOP, expand=not detail, fill=BOTH,
                          padx=pad, pady=pad)
        if detail:
            self.detail = self._widget('Label', self.root, name='dtl',
                                       justify=LEFT, text=detail)
            if self.use_ttk:
                self.detail.configure(anchor=NW)
            self.detail.pack(in_=top, side=TOP, expand=1, fill=BOTH,
                             padx=pad, pady=(0, pad))

        self.frame = bot
        self.num = default
        self.cancel = cancel
        self.default = default

        # 3. Create a row of buttons at the bottom of the dialog.  Each entry
        # of "buttons" is either a label, or a mapping of button options -- like
        # the "[name ?-option value ...?]" button specs in tk::MessageBox.

        # tk::MessageBox and tk_dialog space the buttons differently.
        padx, pady = ('3m', '2m') if self.use_ttk else ('7.5p', '3p')
        self._buttons = []
        for i, but in enumerate(buttons):
            opts = {'text': but} if isinstance(but, str) else dict(but)
            b = self._widget('Button', self.root, name=f'button{i}', **opts)
            # The dialog controls the command and the default ring, overriding
            # anything set in the button options (cf. tk::MessageBox).
            b.configure(command=(lambda self=self, i=i: self.done(i)),
                        default=ACTIVE if i == default else NORMAL)
            # Alt + the underlined character (an "underline" button option)
            # invokes the button (cf. tk::AmpWidget in tk::MessageBox).
            b.bind('<<AltUnderlined>>', lambda e: e.widget.invoke())
            b.grid(in_=bot, column=i, row=0, sticky=EW, padx=padx, pady=pady)
            # tk::MessageBox makes the buttons equal width; tk_dialog does not.
            bot.grid_columnconfigure(i, uniform='buttons' if self.use_ttk else '')
            # We boost the size of some Mac buttons for l&f
            if self.root._windowingsystem == 'aqua':
                if str(opts.get('text', '')).lower() in ('ok', 'cancel'):
                    bot.grid_columnconfigure(i, minsize=90)
                b.grid_configure(pady=7)
            self._buttons.append(b)

        # 4. Bind <Return> to invoke the focused button, or the default button
        # if the focus is elsewhere.  Unlike tk_dialog (which tracks the focus
        # by rebinding <Return> on <<PrevWindow>>/<<NextWindow>>), this reads
        # the live focus, like tk::MessageBox, so it also works with the mouse.

        def on_return(event):
            if event.widget.winfo_class() in ('Button', 'TButton'):
                event.widget.invoke()
            else:
                self.return_event(event)
        self.root.bind('<Return>', on_return)
        # Alt + an underlined character invokes the matching button (cf.
        # ::tk::AltKeyInDialog, bound by tk::MessageBox).
        self.root.bind('<Alt-Key>', self._alt_key)
        # The default ring follows the keyboard focus among the buttons
        # (cf. tk::MessageBox).
        self.root.bind('<FocusIn>', lambda e: self._set_default(e.widget, ACTIVE))
        self.root.bind('<FocusOut>', lambda e: self._set_default(e.widget, NORMAL))

        # 5. Bind <Destroy> to record the cancel index, in case the window is
        # destroyed by something else (e.g. its parent being destroyed).

        def on_destroy(event):
            self.num = cancel
            self.root.quit()
        self.root.bind('<Destroy>', on_destroy)

        self.root.protocol('WM_DELETE_WINDOW', self.wm_delete_window)
        _place_window(self.root, master)

    def go(self):
        self.root.wait_visibility()
        if self.default is not None:
            focus = self._buttons[self.default]
        else:
            focus = self.root

        with _temp_grab_focus(self.root, focus):
            try:
                self.root.mainloop()
            finally:
                try:
                    # It's possible that the window has already been destroyed,
                    # hence this "try/except".  Delete the Destroy handler so that
                    # self.num doesn't get reset by it.
                    self.root.bind('<Destroy>', '')
                except TclError:
                    pass
        return self.num

    def return_event(self, event):
        if self.default is None:
            self.root.bell()
        else:
            self.done(self.default)

    def wm_delete_window(self):
        if self.cancel is None:
            self.root.bell()
        else:
            self.done(self.cancel)

    def done(self, num):
        self.num = num
        self.root.quit()

    def _alt_key(self, event):
        # Invoke the button whose accelerator matches the Alt key.
        target = _find_alt_key_target(self.root, event.char)
        if target is not None:
            target.event_generate('<<AltUnderlined>>')

    def _set_default(self, widget, state):
        # Set a button's default ring.
        if widget.winfo_class() in ('Button', 'TButton'):
            widget.configure(default=state)


# A base class for custom dialogs, with a button box modelled on
# ::tk::MessageBox.
class Dialog(Toplevel):

    '''Class to open dialogs.

    This class is intended as a base class for custom dialogs
    '''

    def _widget(self, klass, master, **kw):
        # Create a themed (ttk) or classic (tkinter) widget.
        return getattr(ttk if self.use_ttk else tkinter, klass)(master, **kw)

    def _frame(self, name):
        # The classic dialog (tk_dialog) gives its frames a raised border on
        # X11; the themed one (tk::MessageBox) does not.
        frame = self._widget('Frame', self, name=name)
        if not self.use_ttk and self._windowingsystem == 'x11':
            frame.configure(relief=RAISED, bd=1)
        return frame

    def __init__(self, parent, title=None, *, use_ttk=False):
        '''Initialize a dialog.

        Arguments:

            parent -- a parent window (the application window)

            title -- the dialog title

            use_ttk -- use the classic Tk widgets (the default), or the themed
                (ttk) widgets if true
        '''
        # Use the classic Tk widgets by default, for compatibility: the themed
        # (ttk) widgets set a themed background that classic widgets added by a
        # subclass in body() would not match.  The query dialogs opt into ttk.
        self.use_ttk = use_ttk

        master = parent
        if master is None:
            master = _get_temp_root()

        Toplevel.__init__(self, master, class_='Dialog')

        if self.use_ttk:
            # Use a single background colour for the whole dialog so that it
            # blends with the ttk widgets (cf. tk::MessageBox).
            self.configure(
                background=ttk.Style(self).lookup('.', 'background'))

        self.withdraw() # remain invisible for now
        # If the parent is not viewable, don't
        # make the child transient, or else it
        # would be opened withdrawn
        if parent is not None and parent.winfo_viewable():
            self.transient(parent)

        self.title(title or ' ')
        self.iconname('Dialog')

        _setup_dialog(self)

        self.parent = parent

        self.result = None

        body = self._frame('top')
        self.initial_focus = self.body(body)
        body.pack(side=TOP, fill=BOTH, expand=1)

        self.buttonbox()

        if self.initial_focus is None:
            self.initial_focus = self

        self.protocol("WM_DELETE_WINDOW", self.cancel)

        _place_window(self, parent)

        # wait for window to appear on screen before calling grab_set
        self.wait_visibility()
        # Dialog destroys itself in ok()/cancel(), so let _temp_grab_focus
        # save/restore the focus and grab without destroying the window.
        with _temp_grab_focus(self, self.initial_focus, destroy=False):
            self.wait_window(self)

    def destroy(self):
        '''Destroy the window'''
        self.initial_focus = None
        Toplevel.destroy(self)
        _destroy_temp_root(self.master)

    #
    # construction hooks

    def body(self, master):
        '''create dialog body.

        return widget that should have initial focus.
        This method should be overridden, and is called
        by the __init__ method.
        '''
        pass

    def buttonbox(self):
        '''add standard button box.

        override if you do not want the standard buttons
        '''

        box = self._frame('bot')

        # tk::MessageBox and tk_dialog space the buttons differently.
        padx, pady = ('3m', '2m') if self.use_ttk else ('7.5p', '3p')
        for i, (name, label, command) in enumerate(
                (('ok', '&OK', self.ok), ('cancel', '&Cancel', self.cancel))):
            # Create a button with an accelerator key marked by "&" in the text
            # (cf. tk::AmpWidget).
            text, underline = _underline_ampersand(label)
            b = self._widget('Button', self, name=name, text=text,
                             underline=underline, command=command,
                             default=ACTIVE if name == 'ok' else NORMAL)
            b.bind('<<AltUnderlined>>', lambda e: e.widget.invoke())
            b.grid(in_=box, column=i, row=0, sticky=EW, padx=padx, pady=pady)
            # tk::MessageBox makes the buttons equal width; tk_dialog does not.
            box.grid_columnconfigure(i, uniform='buttons' if self.use_ttk else '')
            if self._windowingsystem == 'aqua':
                box.grid_columnconfigure(i, minsize=90)
                b.grid_configure(pady=7)

        # Alt + an underlined character invokes the matching button (cf.
        # ::tk::AltKeyInDialog, bound by tk::MessageBox).
        self.bind('<Alt-Key>', self._alt_key)
        # The default ring follows the keyboard focus among the buttons
        # (cf. tk::MessageBox).
        self.bind('<FocusIn>', lambda e: self._set_default(e.widget, ACTIVE))
        self.bind('<FocusOut>', lambda e: self._set_default(e.widget, NORMAL))
        self.bind('<Return>', self._return_event)
        self.bind('<Escape>', self.cancel)

        box.pack(side=BOTTOM, fill=BOTH)
        box.grid_anchor(CENTER)

    def _set_default(self, widget, state):
        # Set a button's default ring.
        if widget.winfo_class() in ('Button', 'TButton'):
            widget.configure(default=state)

    def _return_event(self, event):
        # Invoke the focused button, or accept the dialog if the focus is
        # elsewhere (e.g. in an entry).
        widget = event.widget
        if widget.winfo_class() in ('Button', 'TButton'):
            widget.invoke()
        else:
            self.ok()

    def _alt_key(self, event):
        # Invoke the button whose accelerator matches the Alt key.
        target = _find_alt_key_target(self, event.char)
        if target is not None:
            target.event_generate('<<AltUnderlined>>')

    #
    # standard button semantics

    def ok(self, event=None):

        if not self.validate():
            self.initial_focus.focus_set() # put focus back
            return

        self.withdraw()
        self.update_idletasks()

        try:
            self.apply()
        finally:
            self.cancel()

    def cancel(self, event=None):
        self.destroy()

    #
    # command hooks

    def validate(self):
        '''validate the data

        This method is called automatically to validate the data before the
        dialog is destroyed. By default, it always validates OK.
        '''

        return True # override

    def apply(self):
        '''process the data

        This method is called automatically to process the data, *after*
        the dialog is destroyed. By default, it does nothing.
        '''

        pass # override


# Place a toplevel window at the center of parent or screen
# This is a Python implementation of ::tk::PlaceWindow.
def _wm_dimension(w, command):
    # tk::WMFrameWidth and tk::WMTitleHeight (added in Tk 9.1) return the size
    # of the window manager decoration.  They are 0 except in SDL2 builds of
    # Tk, and are missing in older versions.
    try:
        return int(w.tk.call(command))
    except TclError:
        return 0


def _place_window(w, parent=None):
    w.wm_withdraw() # Remain invisible while we figure out the geometry
    w.update_idletasks() # Actualize geometry information

    screenwidth = w.winfo_screenwidth()
    screenheight = w.winfo_screenheight()
    minwidth = w.winfo_reqwidth()
    minheight = w.winfo_reqheight()
    maxwidth = w.winfo_vrootwidth()
    maxheight = w.winfo_vrootheight()
    # "wm geometry" operates in window manager coordinates and thus includes a
    # possible decoration frame and the title bar.
    framewidth = _wm_dimension(w, '::tk::WMFrameWidth')
    titleheight = _wm_dimension(w, '::tk::WMTitleHeight')
    constrain = False
    if minwidth + 2*framewidth > screenwidth:
        minwidth = screenwidth - 2*framewidth
        constrain = True
    if minheight + titleheight + framewidth > screenheight:
        minheight = screenheight - titleheight - framewidth
        constrain = True

    if parent is not None and parent.winfo_ismapped():
        # Center the window over the parent (which must be mapped).
        x = parent.winfo_rootx() + (parent.winfo_width() - minwidth) // 2
        y = parent.winfo_rooty() + (parent.winfo_height() - minheight) // 2
        # Make sure that the window is on the screen and does not cover the
        # window manager decoration.
        vrootx = w.winfo_vrootx()
        vrooty = w.winfo_vrooty()
        x = min(x, vrootx + maxwidth - minwidth - framewidth)
        x = max(x, vrootx + framewidth)
        y = min(y, vrooty + maxheight - minheight - framewidth)
        y = max(y, vrooty + titleheight)
        if w._windowingsystem == 'aqua':
            # Avoid the native menu bar which sits on top of everything.
            y = max(y, 22 + titleheight)
    else:
        # Center the window on the screen.
        x = (screenwidth - minwidth) // 2
        y = (screenheight - minheight) // 2

    w.wm_maxsize(maxwidth, maxheight)
    geometry = f'{minwidth}x{minheight}' if constrain else ''
    geometry += '+%d+%d' % (x - framewidth, y - titleheight)
    w.wm_geometry(geometry)
    w.wm_deiconify() # Become visible at the desired location


def _underline_ampersand(text):
    # Like tk::UnderlineAmpersand: "&&" is a literal "&"; a single "&" marks
    # the following character as the underlined accelerator.  Return the text
    # without the markers and the index of the accelerator (-1 if none).
    chars = []
    underline = -1
    i = 0
    while i < len(text):
        if text[i] == '&':
            if text[i+1:i+2] == '&':
                chars.append('&')
                i += 2
                continue
            if underline < 0:
                underline = len(chars)
        else:
            chars.append(text[i])
        i += 1
    return ''.join(chars), underline


def _find_alt_key_target(widget, char):
    # Like tk::FindAltKeyTarget: find the widget whose underlined character
    # matches CHAR, searching the widget and its descendants.
    if widget.winfo_class() in ('Button', 'Checkbutton', 'Label', 'Radiobutton',
                                'TButton', 'TCheckbutton', 'TLabel',
                                'TRadiobutton'):
        try:
            under = int(widget.cget('underline'))
        except (TclError, ValueError):
            under = -1
        text = str(widget.cget('text'))
        if 0 <= under < len(text) and char.lower() == text[under].lower():
            return widget
    for child in widget.winfo_children():
        target = _find_alt_key_target(child, char)
        if target is not None:
            return target
    return None


def _setup_dialog(w):
    if w._windowingsystem == "aqua":
        if w.info_patchlevel() >= (9, 1):
            w.wm_attributes(stylemask='titled')
        else:
            w.tk.call('::tk::unsupported::MacWindowStyle', 'style',
                      w, 'moveableModal', '')
    elif w._windowingsystem == "x11":
        w.wm_attributes(type="dialog")

# --------------------------------------------------------------------
# convenience dialogues

class _QueryDialog(Dialog):

    def __init__(self, title, prompt,
                 initialvalue=None,
                 minvalue = None, maxvalue = None,
                 parent = None, *, use_ttk=True):

        self.prompt   = prompt
        self.minvalue = minvalue
        self.maxvalue = maxvalue

        self.initialvalue = initialvalue

        Dialog.__init__(self, parent, title, use_ttk=use_ttk)

    def destroy(self):
        self.entry = None
        Dialog.destroy(self)

    def body(self, master):

        # Wrap a long prompt, like tk::MessageBox wraps its message at 3 inches.
        w = self._widget('Label', master, anchor=NW, text=self.prompt,
                         justify=LEFT, wraplength='3i')
        w.grid(in_=master, padx='2m', pady='2m', sticky=NSEW)

        self.entry = self._widget('Entry', master, name='entry')
        self.entry.grid(row=1, in_=master, padx='2m', pady=(0, '2m'), sticky=NSEW)
        master.grid_rowconfigure(1, weight=1)
        # The prompt and entry expand to the full width, like tk::MessageBox
        # gives weight to its message column.
        master.grid_columnconfigure(0, weight=1)

        if self.initialvalue is not None:
            self.entry.insert(0, self.initialvalue)
            self.entry.select_range(0, END)

        return self.entry

    def validate(self):
        try:
            result = self.getresult()
        except ValueError:
            messagebox.showwarning(
                "Illegal value",
                self.errormessage + "\nPlease try again",
                parent = self
            )
            return False

        if self.minvalue is not None and result < self.minvalue:
            messagebox.showwarning(
                "Too small",
                "The allowed minimum value is %s. "
                "Please try again." % self.minvalue,
                parent = self
            )
            return False

        if self.maxvalue is not None and result > self.maxvalue:
            messagebox.showwarning(
                "Too large",
                "The allowed maximum value is %s. "
                "Please try again." % self.maxvalue,
                parent = self
            )
            return False

        self.result = result

        return True


class _QueryInteger(_QueryDialog):
    errormessage = "Not an integer."

    def getresult(self):
        return self.getint(self.entry.get())


def askinteger(title, prompt, **kw):
    '''get an integer from the user

    Arguments:

        title -- the dialog title
        prompt -- the label text
        **kw -- see SimpleDialog class

    Return value is an integer
    '''
    d = _QueryInteger(title, prompt, **kw)
    return d.result


class _QueryFloat(_QueryDialog):
    errormessage = "Not a floating-point value."

    def getresult(self):
        return self.getdouble(self.entry.get())


def askfloat(title, prompt, **kw):
    '''get a float from the user

    Arguments:

        title -- the dialog title
        prompt -- the label text
        **kw -- see SimpleDialog class

    Return value is a float
    '''
    d = _QueryFloat(title, prompt, **kw)
    return d.result


class _QueryString(_QueryDialog):
    def __init__(self, *args, **kw):
        if "show" in kw:
            self.__show = kw["show"]
            del kw["show"]
        else:
            self.__show = None
        _QueryDialog.__init__(self, *args, **kw)

    def body(self, master):
        entry = _QueryDialog.body(self, master)
        if self.__show is not None:
            entry.configure(show=self.__show)
        return entry

    def getresult(self):
        return self.entry.get()


def askstring(title, prompt, **kw):
    '''get a string from the user

    Arguments:

        title -- the dialog title
        prompt -- the label text
        **kw -- see SimpleDialog class

    Return value is a string
    '''
    d = _QueryString(title, prompt, **kw)
    return d.result


@contextlib.contextmanager
def _temp_grab_focus(grab, focus=None, destroy=True):
    old_focus = grab.focus_get()
    old_grab = grab.grab_current()
    if old_grab is not None and old_grab.winfo_exists():
        old_status = old_grab.grab_status()
    else:
        old_status = None
    # The "grab" command will fail if another application
    # already holds the grab.  So catch it.
    try:
        grab.grab_set()
    except TclError:
        pass
    try:
        if focus is not None and focus.winfo_exists():
            focus.focus_set()

        yield

    finally:
        if old_focus is not None:
            try:
                old_focus.focus_set()
            except TclError:
                pass
        try:
            grab.grab_release()
        except TclError:
            pass
        if destroy:
            try:
                grab.destroy()
            except TclError:
                pass
        if (old_grab is not None and old_grab.winfo_exists()
                and old_grab.winfo_ismapped()):
            # The "grab" command will fail if another application
            # already holds the grab.  So catch it.
            try:
                if old_status == 'global':
                    old_grab.grab_set_global()
                else:
                    old_grab.grab_set()
            except TclError:
                pass


if __name__ == '__main__':

    def test():
        root = Tk()
        use_ttk = tkinter.BooleanVar(root, value=True)

        def test_dialog():
            d = SimpleDialog(root,
                         text="This is a test dialog.  "
                              "Would this have been an actual dialog, "
                              "the buttons below would have been glowing "
                              "in soft pink light.\n"
                              "Do you believe this?",
                         buttons=[{'text': 'Yes', 'underline': 0},
                                  {'text': 'No', 'underline': 0},
                                  {'text': 'Cancel', 'underline': 0}],
                         default=0,
                         cancel=2,
                         title="Test Dialog",
                         bitmap='question',
                         detail="Alt+Y, Alt+N and Alt+C work too.",
                         use_ttk=use_ttk.get())
            print(d.go())

        def test_integer():
            print(askinteger("Spam", "Egg count", initialvalue=12*12,
                             use_ttk=use_ttk.get()))

        def test_float():
            print(askfloat("Spam", "Egg weight\n(in tons)", minvalue=1,
                           maxvalue=100, use_ttk=use_ttk.get()))

        def test_string():
            print(askstring("Spam", "Egg label", use_ttk=use_ttk.get()))

        tkinter.Checkbutton(root, text='Use themed (ttk) widgets',
                            variable=use_ttk).pack(fill=X)
        Button(root, text='SimpleDialog', command=test_dialog).pack(fill=X)
        Button(root, text='askinteger', command=test_integer).pack(fill=X)
        Button(root, text='askfloat', command=test_float).pack(fill=X)
        Button(root, text='askstring', command=test_string).pack(fill=X)
        Button(root, text='Quit', command=root.quit).pack(fill=X)
        root.mainloop()

    test()
