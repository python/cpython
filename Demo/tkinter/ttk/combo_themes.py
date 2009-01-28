"""Ttk Theme Selector.

Although it is a theme selector, you won't notice many changes since
there is only a combobox and a frame around.
"""
import ttk

class App(ttk.Frame):
    def __init__(self):
        ttk.Frame.__init__(self)

        self.style = ttk.Style()
        self._setup_widgets()

    def _change_theme(self, event):
        if event.widget.current(): # value #0 is not a theme
            newtheme = event.widget.get()
            # change to the new theme and refresh all the widgets
            self.style.theme_use(newtheme)

    def _setup_widgets(self):
        themes = list(self.style.theme_names())
        themes.insert(0, "Pick a theme")
        # Create a readonly Combobox which will display 4 values at max,
        # which will cause it to create a scrollbar if there are more
        # than 4 values in total.
        themes_combo = ttk.Combobox(self, values=themes, state="readonly",
                                    height=4)
        themes_combo.set(themes[0]) # sets the combobox value to "Pick a theme"
        # Combobox widget generates a <<ComboboxSelected>> virtual event
        # when the user selects an element. This event is generated after
        # the listbox is unposted (after you select an item, the combobox's
        # listbox disappears, then it is said that listbox is now unposted).
        themes_combo.bind("<<ComboboxSelected>>", self._change_theme)
        themes_combo.pack(fill='x')

        self.pack(fill='both', expand=1)


def main():
    app = App()
    app.master.title("Ttk Combobox")
    app.mainloop()

if __name__ == "__main__":
    main()
