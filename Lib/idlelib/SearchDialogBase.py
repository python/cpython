from tkinter import *

class SearchDialogBase:

    title = "Search Dialog"
    icon = "Search"
    needwrapbutton = 1

    def __init__(self, root, engine):
        self.root = root
        self.engine = engine
        self.top = None

    def open(self, text, searchphrase=None):
        self.text = text
        if not self.top:
            self.create_widgets()
        else:
            self.top.deiconify()
            self.top.tkraise()
        if searchphrase:
            self.ent.delete(0,"end")
            self.ent.insert("end",searchphrase)
        self.ent.focus_set()
        self.ent.selection_range(0, "end")
        self.ent.icursor(0)
        self.top.grab_set()

    def close(self, event=None):
        if self.top:
            self.top.grab_release()
            self.top.withdraw()

    def create_widgets(self):
        top = Toplevel(self.root)
        top.bind("<Return>", self.default_command)
        top.bind("<Escape>", self.close)
        top.protocol("WM_DELETE_WINDOW", self.close)
        top.wm_title(self.title)
        top.wm_iconname(self.icon)
        self.top = top

        self.row = 0
        self.top.grid_columnconfigure(0, pad=2, weight=0)
        self.top.grid_columnconfigure(1, pad=2, minsize=100, weight=100)

        self.create_entries()
        self.create_option_buttons()
        self.create_other_buttons()
        return self.create_command_buttons()

    def make_entry(self, label, var):
        l = Label(self.top, text=label)
        l.grid(row=self.row, column=0, sticky="nw")
        e = Entry(self.top, textvariable=var, exportselection=0)
        e.grid(row=self.row, column=1, sticky="nwe")
        self.row = self.row + 1
        return e

    def make_frame(self,labeltext=None):
        if labeltext:
            l = Label(self.top, text=labeltext)
            l.grid(row=self.row, column=0, sticky="nw")
        f = Frame(self.top)
        f.grid(row=self.row, column=1, columnspan=1, sticky="nwe")
        self.row = self.row + 1
        return f

    def make_button(self, label, command, isdef=0):
        b = Button(self.buttonframe,
                   text=label, command=command,
                   default=isdef and "active" or "normal")
        cols,rows=self.buttonframe.grid_size()
        b.grid(pady=1,row=rows,column=0,sticky="ew")
        self.buttonframe.grid(rowspan=rows+1)
        return b

    def create_entries(self):
        self.ent = self.make_entry("Find:", self.engine.patvar)

    def create_option_buttons(self):
        f = self.make_frame("Options")

        btn = Checkbutton(f, anchor="w",
                variable=self.engine.revar,
                text="Regular expression")
        btn.pack(side="left", fill="both")
        if self.engine.isre():
            btn.select()

        btn = Checkbutton(f, anchor="w",
                variable=self.engine.casevar,
                text="Match case")
        btn.pack(side="left", fill="both")
        if self.engine.iscase():
            btn.select()

        btn = Checkbutton(f, anchor="w",
                variable=self.engine.wordvar,
                text="Whole word")
        btn.pack(side="left", fill="both")
        if self.engine.isword():
            btn.select()

        if self.needwrapbutton:
            btn = Checkbutton(f, anchor="w",
                    variable=self.engine.wrapvar,
                    text="Wrap around")
            btn.pack(side="left", fill="both")
            if self.engine.iswrap():
                btn.select()

    def create_other_buttons(self):
        f = self.make_frame("Direction")

        #lbl = Label(f, text="Direction: ")
        #lbl.pack(side="left")

        btn = Radiobutton(f, anchor="w",
                variable=self.engine.backvar, value=1,
                text="Up")
        btn.pack(side="left", fill="both")
        if self.engine.isback():
            btn.select()

        btn = Radiobutton(f, anchor="w",
                variable=self.engine.backvar, value=0,
                text="Down")
        btn.pack(side="left", fill="both")
        if not self.engine.isback():
            btn.select()

    def create_command_buttons(self):
        #
        # place button frame on the right
        f = self.buttonframe = Frame(self.top)
        f.grid(row=0,column=2,padx=2,pady=2,ipadx=2,ipady=2)

        b = self.make_button("close", self.close)
        b.lower()
