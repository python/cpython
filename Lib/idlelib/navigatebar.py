"""Navigate bar
"""

from tkinter import Listbox, Scrollbar, X, Y, END, VERTICAL, RIGHT
from tkinter.ttk import Frame, Entry


class NavigateBar(Frame):
    """Navigate bar for IDLE.

    Prefix ":" stand for goto lineno
    Prefix "@" stand for goto symbol
    """

    def __init__(self, editor, parent):
        super().__init__(parent, borderwidth=5)
        self.editor = editor
        self.parent = parent  # Should be text
        self.create_widgets()
        self.update_idletasks()
        self.place(x=parent.winfo_width() / 2 - self.winfo_reqwidth() / 2, y=-2)
        self.selection = 0
        self.symbol_line = {}

    def create_widgets(self):
        self.parent.bind('<Configure>', self.winconfig_event)
        self.entry = Entry(self, width=50)
        self.entry.bind('<KeyRelease-Up>', self.key_up_event)
        self.entry.bind('<KeyRelease-Down>', self.key_down_event)
        self.entry.bind('<KeyRelease>', self.navigate)
        self.entry.bind('<KeyRelease-Return>', self.ok)
        self.entry.focus_set()
        self.entry.pack()

        self.scrollbar = Scrollbar(self, orient=VERTICAL)
        self.listbox = Listbox(self, yscrollcommand=self.scrollbar.set)
        self.scrollbar.config(command=self.listbox.yview)
        self.listbox.bind('<Button-1>', self.listselect_event)
        self.listbox.bind('<Up>', self.key_up_event)
        self.listbox.bind('<Down>', self.key_down_event)

        self.entry.bind('<Escape>', self.cancel)
        self.parent.bind('<Escape>', self.cancel)

    def get_entry(self):
        entry = self.entry.get().strip()
        if not entry:
            return ''
        return entry

    def ok(self, event=None):
        if self.listbox.size():
            self.goto_selection()
        self.clear_highlight()
        self.editor.navigatebar = None
        self.parent.focus_set()
        self.destroy()

    def cancel(self, event=None):
        self.clear_highlight()
        self.editor.navigatebar = None
        self.parent.focus_set()
        self.destroy()

    def winconfig_event(self, event):
        self.update_idletasks()
        self.place(x=self.parent.winfo_width() / 2 - self.winfo_reqwidth() / 2, y=-2)

    def listselect_event(self, event=None):
        self.goto_selection()

    def key_up_event(self, event=None):
        if self.selection > 0:
            self.listbox.selection_clear(self.selection)
            self.selection -= 1
            self.listbox.select_set(self.selection)
            self.listbox.see(self.selection)

            self.goto_selection()

    def key_down_event(self, event=None):
        if self.selection < self.listbox.size() - 1:
            self.listbox.selection_clear(self.selection)
            self.selection += 1
            self.listbox.select_set(self.selection)
            self.listbox.see(self.selection)

            self.goto_selection()

    def set_goto_line_mode(self):
        self.entry.insert(0, ':')

    def set_goto_symbol_mode(self):
        self.entry.insert(0, '@')

    def goto_selection(self, event=None):
        if self.listbox.size():
            select = self.listbox.selection_get()
            self.goto_lineno(self.symbol_line[select])

    def goto_lineno(self, lineno):
        self.clear_highlight()
        self.parent.mark_set('insert', '%d.0' % (lineno))
        self.parent.see('insert')
        self.editor.set_line_and_column()

        # Add highlight to target lineno
        self.parent.tag_add('BREAK', '%d.0' % lineno, '%d.0' % (lineno + 1))

    def navigate(self, event=None):
        self.clear_listbox()
        self.clear_highlight()

        entry = self.get_entry()
        if entry.startswith(':'):
            try:
                lineno = int(entry[1:])
            except ValueError:
                return
            self.goto_lineno(lineno)
        elif entry.startswith('@'):
            self.update_symbol(entry[1:])

    def clear_listbox(self):
        """Show off listbox and clear listbox items
        """
        self.scrollbar.pack_forget()
        self.listbox.pack_forget()
        self.listbox.delete(0, END)

    def clear_highlight(self):
        self.parent.tag_remove("BREAK", "insert linestart",
                               "insert lineend +1char")

    def reset_listbox(self):
        """Show out listbox and reset selection
        """
        self.scrollbar.pack(side=RIGHT, fill=Y)
        self.listbox.pack(fill=X)
        self.listbox.delete(0, END)
        self.selection = 0

    def update_symbol(self, entry):
        self.reset_listbox()

        texts = self.parent.get('1.0', 'end')
        for index, text in enumerate(texts.split('\n')):
            dedent_text = text.strip()
            if dedent_text.startswith('class ') or dedent_text.startswith('def '):
                if entry and not dedent_text.strip('class ').strip('def ').startswith(entry):
                    continue
                self.listbox.insert(END, text)
                self.symbol_line[text] = index + 1
        self.listbox.select_set(self.selection)
