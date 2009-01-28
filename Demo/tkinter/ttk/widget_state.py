"""Sample demo showing widget states and some font styling."""
import ttk

states = ['active', 'disabled', 'focus', 'pressed', 'selected',
          'background', 'readonly', 'alternate', 'invalid']

for state in states[:]:
    states.append("!" + state)

def reset_state(widget):
    nostate = states[len(states) // 2:]
    widget.state(nostate)

class App(ttk.Frame):
    def __init__(self, title=None):
        ttk.Frame.__init__(self, borderwidth=6)
        self.master.title(title)

        self.style = ttk.Style()

        # get default font size and family
        btn_font = self.style.lookup("TButton", "font")
        fsize = str(self.tk.eval("font configure %s -size" % btn_font))
        self.font_family = self.tk.eval("font configure %s -family" % btn_font)
        if ' ' in self.font_family:
            self.font_family = '{%s}' % self.font_family
        self.fsize_prefix = fsize[0] if fsize[0] == '-' else ''
        self.base_fsize = int(fsize[1 if fsize[0] == '-' else 0:])

        # a list to hold all the widgets that will have their states changed
        self.update_widgets = []

        self._setup_widgets()

    def _set_font(self, extra=0):
        self.style.configure("TButton", font="%s %s%d" % (self.font_family,
            self.fsize_prefix, self.base_fsize + extra))

    def _new_state(self, widget, newtext):
        widget = self.nametowidget(widget)

        if not newtext:
            goodstates = ["disabled"]
            font_extra = 0
        else:
            # set widget state according to what has been entered in the entry
            newstates = set(newtext.split()) # eliminate duplicates

            # keep only the valid states
            goodstates = [state for state in newstates if state in states]
            # define a new font size based on amount of states
            font_extra = 2 * len(goodstates)

        # set new widget state
        for widget in self.update_widgets:
            reset_state(widget) # remove any previous state from the widget
            widget.state(goodstates)

        # update Ttk Button font size
        self._set_font(font_extra)
        return 1

    def _setup_widgets(self):
        btn = ttk.Button(self, text='Enter states and watch')

        entry = ttk.Entry(self, cursor='xterm', validate="key")
        entry['validatecommand'] = (self.register(self._new_state), '%W', '%P')
        entry.focus()

        self.update_widgets.append(btn)
        entry.validate()

        entry.pack(fill='x', padx=6)
        btn.pack(side='left', pady=6, padx=6, anchor='n')
        self.pack(fill='both', expand=1)


def main():
    app = App("Widget State Tester")
    app.mainloop()

if __name__ == "__main__":
    main()
