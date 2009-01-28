"""Ttk Theme Selector v2.

This is an improvement from the other theme selector (themes_combo.py)
since now you can notice theme changes in Ttk Combobox, Ttk Frame,
Ttk Label and Ttk Button.
"""
import Tkinter
import ttk

class App(ttk.Frame):
    def __init__(self):
        ttk.Frame.__init__(self, borderwidth=3)

        self.style = ttk.Style()

        # XXX Ideally I wouldn't want to create a Tkinter.IntVar to make
        #     it works with Checkbutton variable option.
        self.theme_autochange = Tkinter.IntVar(self, 0)
        self._setup_widgets()

    def _change_theme(self):
        self.style.theme_use(self.themes_combo.get())

    def _theme_sel_changed(self, widget):
        if self.theme_autochange.get():
            self._change_theme()

    def _setup_widgets(self):
        themes_lbl = ttk.Label(self, text="Themes")

        themes = self.style.theme_names()
        self.themes_combo = ttk.Combobox(self, values=themes, state="readonly")
        self.themes_combo.set(themes[0])
        self.themes_combo.bind("<<ComboboxSelected>>", self._theme_sel_changed)

        change_btn = ttk.Button(self, text='Change Theme',
            command=self._change_theme)

        theme_change_checkbtn = ttk.Checkbutton(self,
            text="Change themes when combobox item is activated",
            variable=self.theme_autochange)

        themes_lbl.grid(ipadx=6, sticky="w")
        self.themes_combo.grid(row=0, column=1, padx=6, sticky="ew")
        change_btn.grid(row=0, column=2, padx=6, sticky="e")
        theme_change_checkbtn.grid(row=1, columnspan=3, sticky="w", pady=6)

        top = self.winfo_toplevel()
        top.rowconfigure(0, weight=1)
        top.columnconfigure(0, weight=1)
        self.columnconfigure(1, weight=1)
        self.grid(row=0, column=0, sticky="nsew", columnspan=3, rowspan=2)


def main():
    app = App()
    app.master.title("Theme Selector")
    app.mainloop()

if __name__ == "__main__":
    main()
