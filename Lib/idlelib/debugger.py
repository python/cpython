import bdb
import os
import linecache
import re

from tkinter import BooleanVar, Menu, TclError
from tkinter.ttk import PanedWindow, Frame, Label, Treeview, Scrollbar
from tkinter import PhotoImage
from tkinter.font import Font

from idlelib.window import ListedToplevel


def underscore_at_end(s):
    """Helper used when displaying a sorted list of local or global variables
    so that internal variables (starting with an underscore) are displayed
    after others, not before.
    """
    return s.replace('_', '~')


class Idb(bdb.Bdb):

    def __init__(self, gui):
        self.gui = gui  # An instance of Debugger or proxy of remote.
        bdb.Bdb.__init__(self)

    def user_line(self, frame):
        if self.in_rpc_code(frame):
            self.set_step()
            return
        message = self.__frame2message(frame)
        try:
            self.gui.interaction(message, frame)
        except TclError:  # When closing debugger window with [x] in 3.x
            pass

    def user_exception(self, frame, info):
        if self.in_rpc_code(frame):
            self.set_step()
            return
        message = self.__frame2message(frame)
        self.gui.interaction(message, frame, info)

    def in_rpc_code(self, frame):
        if frame.f_code.co_filename.count('rpc.py'):
            return True
        else:
            prev_frame = frame.f_back
            prev_name = prev_frame.f_code.co_filename
            if 'idlelib' in prev_name and 'debugger' in prev_name:
                # catch both idlelib/debugger.py and idlelib/debugger_r.py
                # on both Posix and Windows
                return False
            return self.in_rpc_code(prev_frame)

    def __frame2message(self, frame):
        code = frame.f_code
        filename = code.co_filename
        lineno = frame.f_lineno
        basename = os.path.basename(filename)
        message = f"{basename}:{lineno}"
        if code.co_name != "?":
            message = f"{message}: {code.co_name}()"
        return message


class Debugger:

    vstack = vsource = vlocals = vglobals = None

    def __init__(self, pyshell, idb=None):
        if idb is None:
            idb = Idb(self)
        self.pyshell = pyshell
        self.idb = idb  # If passed, a proxy of remote instance.
        self.frame = None
        self.make_gui()
        self.interacting = 0
        self.nesting_level = 0
        self.framevars = {}
        self.running = False

    def getimage(self, filename):
        "Return an image object for a file in our 'Icons' directory"
        dir = os.path.join(os.path.abspath(os.path.dirname(__file__)), 'Icons')
        return PhotoImage(master=self.root, file=os.path.join(dir, filename))

    def run(self, *args):
        # Deal with the scenario where we've already got a program running
        # in the debugger and we want to start another. If that is the case,
        # our second 'run' was invoked from an event dispatched not from
        # the main event loop, but from the nested event loop in 'interaction'
        # below. So our stack looks something like this:
        #       outer main event loop
        #         run()
        #           <running program with traces>
        #             callback to debugger's interaction()
        #               nested event loop
        #                 run() for second command
        #
        # This kind of nesting of event loops causes all kinds of problems
        # (see e.g. issue #24455) especially when dealing with running as a
        # subprocess, where there's all kinds of extra stuff happening in
        # there - insert a traceback.print_stack() to check it out.
        #
        # By this point, we've already called restart_subprocess() in
        # ScriptBinding. However, we also need to unwind the stack back to
        # that outer event loop.  To accomplish this, we:
        #   - return immediately from the nested run()
        #   - abort_loop ensures the nested event loop will terminate
        #   - the debugger's interaction routine completes normally
        #   - the restart_subprocess() will have taken care of stopping
        #     the running program, which will also let the outer run complete
        #
        # That leaves us back at the outer main event loop, at which point our
        # after event can fire, and we'll come back to this routine with a
        # clean stack.
        if self.nesting_level > 0:
            self.abort_loop()
            self.root.after(100, lambda: self.run(*args))
            return
        try:
            self.interacting = 1
            return self.idb.run(*args)
        finally:
            self.interacting = 0

    def beginexecuting(self):
        self.running = True

    def endexecuting(self):
        self.running = False
        self.show_status('')
        self.enable_buttons(['prefs'])
        self.clear_stack()

    def close(self, event=None):
        try:
            self.quit()
        except Exception:
            pass
        if self.interacting:
            self.top.bell()
            return
        self.abort_loop()
        # Clean up pyshell if user clicked debugger control close widget.
        # (Causes a harmless extra cycle through close_debugger() if user
        # toggled debugger from pyshell Debug menu)
        self.pyshell.close_debugger()
        # Now close the debugger control window....
        self.top.destroy()

    def make_gui(self):
        pyshell = self.pyshell
        self.flist = pyshell.flist
        self.root = root = pyshell.root
        self.clickable_cursor = 'hand2'
        if self.root._windowingsystem == 'aqua':
            self.clickable_cursor = 'pointinghand'
        self.tooltip = None
        self.var_values = {}
        self.top = top = ListedToplevel(root)
        self.top.wm_title("Debug Control")
        self.top.wm_iconname("Debug")
        top.wm_protocol("WM_DELETE_WINDOW", self.close)
        self.top.bind("<Escape>", self.close)

        self.var_open_source_windows = BooleanVar(top, False)

        self.pane = PanedWindow(self.top, orient='horizontal')
        self.pane.grid(column=0, row=0, sticky='nwes')
        self.top.grid_columnconfigure(0, weight=1)
        self.top.grid_rowconfigure(0, weight=1)
        self.left = left = Frame(self.pane, padding=5)
        self.pane.add(left, weight=1)
        controls = Frame(left)
        self.buttondata = {}
        self.buttons = ['go', 'step', 'over', 'out', 'stop', 'prefs']
        button_names = {'go':'Go', 'step':'Step', 'over':'Over',
                             'out':'Out', 'stop':'Stop', 'prefs':'Options'}
        self.button_cmds = {'go':self.cont, 'step':self.step,
                            'over':self.next, 'out':self.ret,
                            'stop':self.quit, 'prefs':self.options}
        for col, key in enumerate(self.buttons):
            normal = self.getimage('debug_'+key+'.gif')
            disabled = self.getimage('debug_'+key+'_disabled.gif')
            b = Label(controls, image=normal, text=button_names[key],
                          compound='top', font='TkIconFont')
            b.grid(column=col, row=0, padx=[0,5])
            self.buttondata[key] = (b, normal, disabled)
        self.enable_buttons(['prefs'])
        self.status_normal_font = Font(root=self.root, name='TkDefaultFont', exists=True)
        self.status_error_font = self.status_normal_font.copy()
        self.status_error_font['slant'] = 'italic'
        self.status = Label(controls, text=' ', font=self.status_normal_font)
        self.status.grid(column=0, row=1, columnspan=8, sticky='nw', padx=[5,0])
        controls.grid(column=0, row=0, sticky='new', pady=[0,6])
        controls.grid_columnconfigure(7, weight=1)

        self.current_line_img = self.getimage('debug_current.gif')
        self.regular_line_img = self.getimage('debug_line.gif')
        self.stack = Treeview(left, columns=('statement', ),
                                  height=5, selectmode='browse')
        self.stack.column('#0', width=100)
        self.stack.column('#1', width=150)
        self.stack.tag_configure('error', foreground='red')
        self.stack.bind('<<TreeviewSelect>>',
                        lambda e: self.stack_selection_changed())
        self.stack.bind('<Double-1>', lambda e: self.stack_doubleclick())
        if self.root._windowingsystem == 'aqua':
            self.stack.bind('<Button-2>', self.stack_contextmenu)
            self.stack.bind('<Control-Button-1>', self.stack_contextmenu)
        else:
            self.stack.bind('<Button-3>', self.stack_contextmenu)

        scroll = Scrollbar(left, command=self.stack.yview)
        self.stack['yscrollcommand'] = scroll.set
        self.stack.grid(column=0, row=2, sticky='nwes')
        scroll.grid(column=1, row=2, sticky='ns')
        left.grid_columnconfigure(0, weight=1)
        left.grid_rowconfigure(2, weight=1)

        right = Frame(self.pane, padding=5)
        self.pane.add(right, weight=1)
        self.vars = Treeview(right, columns=('value',), height=5,
                                                selectmode='none')
        self.locals = self.vars.insert('', 'end', text='Locals',
                                       open=True)
        self.globals = self.vars.insert('', 'end', text='Globals',
                                        open=False)
        self.vars.column('#0', width=100)
        self.vars.column('#1', width=150)
        self.vars.bind('<Motion>', self.mouse_moved_vars)
        self.vars.bind('<Leave>', self.leave_vars)
        scroll2 = Scrollbar(right, command=self.vars.yview)
        self.vars['yscrollcommand'] = scroll2.set
        self.vars.grid(column=0, row=0, sticky='nwes')
        scroll2.grid(column=1, row=0, sticky='ns')
        right.grid_columnconfigure(0, weight=1)
        right.grid_rowconfigure(0, weight=1)
        self.clear_stack()

    def enable_buttons(self, buttons=None):
        for key in self.buttons:
            if buttons is None or key not in buttons:
                self.buttondata[key][0]['image'] = self.buttondata[key][2]
                self.buttondata[key][0]['foreground'] = '#aaaaaa'
                self.buttondata[key][0]['cursor'] = ''
                self.buttondata[key][0].bind('<1>', 'break')
                self.buttondata[key][0].bind('<<context-menu>>', 'break')
            else:
                self.buttondata[key][0]['image'] = self.buttondata[key][1]
                self.buttondata[key][0]['foreground'] = '#000000'
                self.buttondata[key][0].bind('<1>', self.button_cmds[key])
                self.buttondata[key][0].bind('<<context-menu>>',
                                             self.button_cmds[key])
                self.buttondata[key][0]['cursor'] = self.clickable_cursor

    def stack_selection_changed(self):
        self.show_vars()

    def stack_doubleclick(self):
        sel = self.stack.selection()
        if len(sel) == 1:
            self.show_source(sel[0])

    def stack_contextmenu(self, event):
        item = self.stack.identify('item', event.x, event.y)
        if item is not None and item != -1 and item != '':
            menu = Menu(self.top, tearoff=0)
            menu.add_command(label='View Source',
                             command = lambda: self.show_source(item))
            menu.tk_popup(event.x_root, event.y_root)

    def show_source(self, item):
        if item in self.framevars:
            fname = self.framevars[item][2]
            lineno = self.framevars[item][3]
            if fname[:1] + fname[-1:] != "<>" and os.path.exists(fname):
                self.flist.gotofileline(fname, lineno)

    def show_status(self, msg, error=False):
        self.status['text'] = msg
        self.status['foreground'] = '#ff0000' if error else '#006600'
        self.status['font'] = self.status_error_font if error \
                            else self.status_normal_font

    def clear_stack(self):
        self.stack.delete(*self.stack.get_children(''))
        self.vars.delete(*self.vars.get_children(self.locals))
        self.vars.delete(*self.vars.get_children(self.globals))
        self.vars.detach(self.locals)
        self.vars.detach(self.globals)
        self.var_values = {}

    def add_stackframe(self, frame, lineno, current=False):
        func = frame.f_code.co_name
        if func in ("?", "", None):
            func = '.'
        try:
            selfval = frame.f_locals['self']
        except KeyError:
            selfval = None;
        if selfval:
            # This stackframe represents an object method; preface the method
            # name with the name of the class.
            if selfval.__class__.__name__ == 'str':
                # We've got the string representation of the object sent from
                # the remote debugger; parse out the name of the class, e.g.
                # from "<random.Random object at 0x...>" extract "Random".
                match = re.match(r'^<(?:.*)\.([^.]*) object at 0x[0-9a-f]+>$',
                                 selfval)
                if match:
                    func = match.group(1) + '.' + func
            else:
                func = selfval.__class__.__name__ + '.' + func
        stmt = linecache.getline(frame.f_code.co_filename, lineno).strip()
        image = self.current_line_img if current else self.regular_line_img
        item = self.stack.insert('', 'end', text=func,
                               values=(stmt,), image=image)
        self.framevars[item] = (frame.f_locals, frame.f_globals,
                                frame.f_code.co_filename, lineno)
        if current:
            self.stack.selection_set(item)

    def interaction(self, message, frame, info=None):
        self.frame = frame
        self.show_status(message)
        #
        if info:
            type, value, tb = info
            try:
                m1 = type.__name__
            except AttributeError:
                m1 = "%s" % str(type)
            if value is not None:
                try:
                   # TODO redo entire section, tries not needed.
                    m1 = f"{m1}: {value}"
                except:
                    pass
        else:
            m1 = ""
            tb = None

        if m1 != '':
            self.show_status(m1, error=True)
        stack, idx = self.idb.get_stack(self.frame, tb)
        self.clear_stack()
        for i in range(len(stack)):
            frame, lineno = stack[i]
            self.add_stackframe(frame, lineno, current=(i == idx))
        self.show_vars()
        self.sync_source_line()
        self.enable_buttons(self.buttons)

        self.top.wakeup()
        # Nested main loop: Tkinter's main loop is not reentrant, so use
        # Tcl's vwait facility, which reenters the event loop until an
        # event handler sets the variable we're waiting on.
        self.nesting_level += 1
        self.root.tk.call('vwait', '::idledebugwait')
        self.nesting_level -= 1
        self.frame = None

    def show_vars(self):
        self.vars.move(self.locals, '', 0)
        self.vars.move(self.globals, '', 1)
        self.vars.delete(*self.vars.get_children(self.locals))
        self.vars.delete(*self.vars.get_children(self.globals))
        self.var_values = {}
        sel = self.stack.selection()
        if len(sel) == 1 and sel[0] in self.framevars:
            locals, globals, _, _ = self.framevars[sel[0]]
            # locals/globals are normally rpc DictProxy objects, which are
            # not iterable.
            self.add_varheader()
            for name in sorted(locals.keys(), key=underscore_at_end):
                self.add_var(name, locals[name])
            self.add_varheader(is_global=True)
            for name in sorted(globals.keys(), key=underscore_at_end):
                self.add_var(name, globals[name], is_global=True)

    def add_varheader(self, is_global=False):
        pass

    def add_var(self, varname, value, is_global=False):
        item = self.vars.insert(self.globals if is_global else self.locals,
                         'end', text=varname, values=(value, ))
        self.var_values[item] = value

    def mouse_moved_vars(self, ev):
        # tooltip_schedule(ev, self.var_tooltip)
        pass

    def leave_vars(self, ev):
        # tooltip_clear()
        pass

    def var_tooltip(self, ev):
        # Callback from tooltip package to return text of tooltip.
        item = None
        if self.vars.identify('column', ev.x, ev.y) == '#1':
            item = self.vars.identify('item', ev.x, ev.y)
        if item and item in self.var_values:
            return(self.var_values[item], ev.x + self.vars.winfo_rootx() + 10,
                                          ev.y + self.vars.winfo_rooty() + 5)
        return None

    def sync_source_line(self):
        frame = self.frame
        if not frame:
            return
        filename, lineno = self.__frame2fileline(frame)
        if filename[:1] + filename[-1:] != "<>" and os.path.exists(filename):
            if (self.var_open_source_windows.get() or
                self.flist.already_open(filename)):
                self.flist.gotofileline(filename, lineno)

    def __frame2fileline(self, frame):
        code = frame.f_code
        filename = code.co_filename
        lineno = frame.f_lineno
        return filename, lineno

    def invoke_program(self):
        "Called just before taking the next action in debugger, adjust state"
        self.enable_buttons(['stop'])
        self.show_status('Running...')

    def cont(self, ev=None):
        self.invoke_program()
        self.idb.set_continue()
        self.abort_loop()

    def step(self, ev=None):
        self.invoke_program()
        self.idb.set_step()
        self.abort_loop()

    def next(self, ev=None):
        self.invoke_program()
        self.idb.set_next(self.frame)
        self.abort_loop()

    def ret(self, ev=None):
        self.invoke_program()
        self.idb.set_return(self.frame)
        self.abort_loop()

    def quit(self, ev=None):
        self.invoke_program()
        self.idb.set_quit()
        self.abort_loop()

    def abort_loop(self):
        self.root.tk.call('set', '::idledebugwait', '1')

    def options(self, ev=None):
        menu = Menu(self.top, tearoff=0)
        menu.add_checkbutton(label='Show Source in Open Files Only',
                variable=self.var_open_source_windows, onvalue=False)
        menu.add_checkbutton(label='Automatically Open Files to Show Source',
                variable=self.var_open_source_windows, onvalue=True)
        menu.tk_popup(ev.x_root, ev.y_root)

    def set_breakpoint_here(self, filename, lineno):
        self.idb.set_break(filename, lineno)

    def clear_breakpoint_here(self, filename, lineno):
        self.idb.clear_break(filename, lineno)

    def clear_file_breaks(self, filename):
        self.idb.clear_all_file_breaks(filename)

    def load_breakpoints(self):
        "Load PyShellEditorWindow breakpoints into subprocess debugger"
        for editwin in self.pyshell.flist.inversedict:
            filename = editwin.io.filename
            try:
                for lineno in editwin.breakpoints:
                    self.set_breakpoint_here(filename, lineno)
            except AttributeError:
                continue


if __name__ == "__main__":
    from unittest import main
    main('idlelib.idle_test.test_debugger', verbosity=2, exit=False)

# TODO: htest?
