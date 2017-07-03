"""IDLE Configuration Dialog: support user customization of IDLE by GUI

Customize font faces, sizes, and colorization attributes.  Set indentation
defaults.  Customize keybindings.  Colorization and keybindings can be
saved as user defined sets.  Select startup options including shell/editor
and default window size.  Define additional help sources.

Note that tab width in IDLE is currently fixed at eight due to Tk issues.
Refer to comments in EditorWindow autoindent code for details.

"""
from tkinter import (Toplevel, Frame, LabelFrame, Listbox, Label, Button,
                     Entry, Text, Scale, Radiobutton, Checkbutton, Canvas,
                     StringVar, BooleanVar, IntVar, TRUE, FALSE,
                     TOP, BOTTOM, RIGHT, LEFT, SOLID, GROOVE, NORMAL, DISABLED,
                     NONE, BOTH, X, Y, W, E, EW, NS, NSEW, NW,
                     HORIZONTAL, VERTICAL, ANCHOR, END)
from tkinter.ttk import Scrollbar
import tkinter.colorchooser as tkColorChooser
import tkinter.font as tkFont
import tkinter.messagebox as tkMessageBox

from idlelib.config import idleConf
from idlelib.config_key import GetKeysDialog
from idlelib.dynoption import DynOptionMenu
from idlelib import macosx
from idlelib.query import SectionName, HelpSource
from idlelib.tabbedpages import TabbedPageSet
from idlelib.textview import view_text

class ConfigDialog(Toplevel):

    def __init__(self, parent, title='', _htest=False, _utest=False):
        """
        _htest - bool, change box location when running htest
        _utest - bool, don't wait_window when running unittest
        """
        Toplevel.__init__(self, parent)
        self.parent = parent
        if _htest:
            parent.instance_dict = {}
        self.withdraw()

        self.configure(borderwidth=5)
        self.title(title or 'IDLE Preferences')
        x = parent.winfo_rootx() + 20
        y = parent.winfo_rooty() + (30 if not _htest else 150)
        self.geometry(f'+{x}+{y}')
        #Theme Elements. Each theme element key is its display name.
        #The first value of the tuple is the sample area tag name.
        #The second value is the display name list sort index.
        self.theme_elements={
            'Normal Text': ('normal', '00'),
            'Python Keywords': ('keyword', '01'),
            'Python Definitions': ('definition', '02'),
            'Python Builtins': ('builtin', '03'),
            'Python Comments': ('comment', '04'),
            'Python Strings': ('string', '05'),
            'Selected Text': ('hilite', '06'),
            'Found Text': ('hit', '07'),
            'Cursor': ('cursor', '08'),
            'Editor Breakpoint': ('break', '09'),
            'Shell Normal Text': ('console', '10'),
            'Shell Error Text': ('error', '11'),
            'Shell Stdout Text': ('stdout', '12'),
            'Shell Stderr Text': ('stderr', '13'),
            }
        self.reset_changed_items() #load initial values in changed items dict
        self.create_widgets()
        self.resizable(height=FALSE, width=FALSE)
        self.transient(parent)
        self.grab_set()
        self.protocol("WM_DELETE_WINDOW", self.cancel)
        self.tab_pages.focus_set()
        #key bindings for this dialog
        #self.bind('<Escape>', self.Cancel) #dismiss dialog, no save
        #self.bind('<Alt-a>', self.Apply) #apply changes, save
        #self.bind('<F1>', self.Help) #context help
        self.load_configs()
        self.attach_var_callbacks() #avoid callbacks during load_configs

        if not _utest:
            self.wm_deiconify()
            self.wait_window()

    def create_widgets(self):
        self.tab_pages = TabbedPageSet(self,
                page_names=['Fonts/Tabs', 'Highlighting', 'Keys', 'General',
                            'Extensions'])
        self.tab_pages.pack(side=TOP, expand=TRUE, fill=BOTH)
        self.create_page_font_tab()
        self.create_page_highlight()
        self.create_page_keys()
        self.create_page_general()
        self.create_page_extensions()
        self.create_action_buttons().pack(side=BOTTOM)

    def create_action_buttons(self):
        if macosx.isAquaTk():
            # Changing the default padding on OSX results in unreadable
            # text in the buttons
            padding_args = {}
        else:
            padding_args = {'padx':6, 'pady':3}
        outer = Frame(self, pady=2)
        buttons = Frame(outer, pady=2)
        for txt, cmd in (
            ('Ok', self.ok),
            ('Apply', self.apply),
            ('Cancel', self.cancel),
            ('Help', self.help)):
            Button(buttons, text=txt, command=cmd, takefocus=FALSE,
                   **padding_args).pack(side=LEFT, padx=5)
        # add space above buttons
        Frame(outer, height=2, borderwidth=0).pack(side=TOP)
        buttons.pack(side=BOTTOM)
        return outer

    def create_page_font_tab(self):
        parent = self.parent
        self.font_size = StringVar(parent)
        self.font_bold = BooleanVar(parent)
        self.font_name = StringVar(parent)
        self.space_num = IntVar(parent)
        self.edit_font = tkFont.Font(parent, ('courier', 10, 'normal'))

        ##widget creation
        #body frame
        frame = self.tab_pages.pages['Fonts/Tabs'].frame
        #body section frames
        frame_font = LabelFrame(
                frame, borderwidth=2, relief=GROOVE, text=' Base Editor Font ')
        frame_indent = LabelFrame(
                frame, borderwidth=2, relief=GROOVE, text=' Indentation Width ')
        #frame_font
        frame_font_name = Frame(frame_font)
        frame_font_param = Frame(frame_font)
        font_name_title = Label(
                frame_font_name, justify=LEFT, text='Font Face :')
        self.list_fonts = Listbox(
                frame_font_name, height=5, takefocus=FALSE, exportselection=FALSE)
        self.list_fonts.bind(
                '<ButtonRelease-1>', self.on_list_fonts_button_release)
        scroll_font = Scrollbar(frame_font_name)
        scroll_font.config(command=self.list_fonts.yview)
        self.list_fonts.config(yscrollcommand=scroll_font.set)
        font_size_title = Label(frame_font_param, text='Size :')
        self.opt_menu_font_size = DynOptionMenu(
                frame_font_param, self.font_size, None, command=self.set_font_sample)
        check_font_bold = Checkbutton(
                frame_font_param, variable=self.font_bold, onvalue=1,
                offvalue=0, text='Bold', command=self.set_font_sample)
        frame_font_sample = Frame(frame_font, relief=SOLID, borderwidth=1)
        self.font_sample = Label(
                frame_font_sample, justify=LEFT, font=self.edit_font,
                text='AaBbCcDdEe\nFfGgHhIiJjK\n1234567890\n#:+=(){}[]')
        #frame_indent
        frame_indent_size = Frame(frame_indent)
        indent_size_title = Label(
                frame_indent_size, justify=LEFT,
                text='Python Standard: 4 Spaces!')
        self.scale_indent_size = Scale(
                frame_indent_size, variable=self.space_num,
                orient='horizontal', tickinterval=2, from_=2, to=16)

        #widget packing
        #body
        frame_font.pack(side=LEFT, padx=5, pady=5, expand=TRUE, fill=BOTH)
        frame_indent.pack(side=LEFT, padx=5, pady=5, fill=Y)
        #frame_font
        frame_font_name.pack(side=TOP, padx=5, pady=5, fill=X)
        frame_font_param.pack(side=TOP, padx=5, pady=5, fill=X)
        font_name_title.pack(side=TOP, anchor=W)
        self.list_fonts.pack(side=LEFT, expand=TRUE, fill=X)
        scroll_font.pack(side=LEFT, fill=Y)
        font_size_title.pack(side=LEFT, anchor=W)
        self.opt_menu_font_size.pack(side=LEFT, anchor=W)
        check_font_bold.pack(side=LEFT, anchor=W, padx=20)
        frame_font_sample.pack(side=TOP, padx=5, pady=5, expand=TRUE, fill=BOTH)
        self.font_sample.pack(expand=TRUE, fill=BOTH)
        #frame_indent
        frame_indent_size.pack(side=TOP, fill=X)
        indent_size_title.pack(side=TOP, anchor=W, padx=5)
        self.scale_indent_size.pack(side=TOP, padx=5, fill=X)
        return frame

    def create_page_highlight(self):
        parent = self.parent
        self.builtin_theme = StringVar(parent)
        self.custom_theme = StringVar(parent)
        self.fg_bg_toggle = BooleanVar(parent)
        self.colour = StringVar(parent)
        self.font_name = StringVar(parent)
        self.is_builtin_theme = BooleanVar(parent)
        self.highlight_target = StringVar(parent)

        ##widget creation
        #body frame
        frame = self.tab_pages.pages['Highlighting'].frame
        #body section frames
        frame_custom = LabelFrame(frame, borderwidth=2, relief=GROOVE,
                                 text=' Custom Highlighting ')
        frame_theme = LabelFrame(frame, borderwidth=2, relief=GROOVE,
                                text=' Highlighting Theme ')
        #frame_custom
        self.text_highlight_sample=Text(
                frame_custom, relief=SOLID, borderwidth=1,
                font=('courier', 12, ''), cursor='hand2', width=21, height=11,
                takefocus=FALSE, highlightthickness=0, wrap=NONE)
        text=self.text_highlight_sample
        text.bind('<Double-Button-1>', lambda e: 'break')
        text.bind('<B1-Motion>', lambda e: 'break')
        text_and_tags=(
            ('#you can click here', 'comment'), ('\n', 'normal'),
            ('#to choose items', 'comment'), ('\n', 'normal'),
            ('def', 'keyword'), (' ', 'normal'),
            ('func', 'definition'), ('(param):\n  ', 'normal'),
            ('"""string"""', 'string'), ('\n  var0 = ', 'normal'),
            ("'string'", 'string'), ('\n  var1 = ', 'normal'),
            ("'selected'", 'hilite'), ('\n  var2 = ', 'normal'),
            ("'found'", 'hit'), ('\n  var3 = ', 'normal'),
            ('list', 'builtin'), ('(', 'normal'),
            ('None', 'keyword'), (')\n', 'normal'),
            ('  breakpoint("line")', 'break'), ('\n\n', 'normal'),
            (' error ', 'error'), (' ', 'normal'),
            ('cursor |', 'cursor'), ('\n ', 'normal'),
            ('shell', 'console'), (' ', 'normal'),
            ('stdout', 'stdout'), (' ', 'normal'),
            ('stderr', 'stderr'), ('\n', 'normal'))
        for texttag in text_and_tags:
            text.insert(END, texttag[0], texttag[1])
        for element in self.theme_elements:
            def tem(event, elem=element):
                event.widget.winfo_toplevel().highlight_target.set(elem)
            text.tag_bind(
                    self.theme_elements[element][0], '<ButtonPress-1>', tem)
        text.config(state=DISABLED)
        self.frame_colour_set = Frame(frame_custom, relief=SOLID, borderwidth=1)
        frame_fg_bg_toggle = Frame(frame_custom)
        button_set_colour = Button(
                self.frame_colour_set, text='Choose Colour for :',
                command=self.get_colour, highlightthickness=0)
        self.opt_menu_highlight_target = DynOptionMenu(
                self.frame_colour_set, self.highlight_target, None,
                highlightthickness=0) #, command=self.set_highlight_targetBinding
        self.radio_fg = Radiobutton(
                frame_fg_bg_toggle, variable=self.fg_bg_toggle, value=1,
                text='Foreground', command=self.set_colour_sample_binding)
        self.radio_bg=Radiobutton(
                frame_fg_bg_toggle, variable=self.fg_bg_toggle, value=0,
                text='Background', command=self.set_colour_sample_binding)
        self.fg_bg_toggle.set(1)
        button_save_custom_theme = Button(
                frame_custom, text='Save as New Custom Theme',
                command=self.save_as_new_theme)
        #frame_theme
        theme_type_title = Label(frame_theme, text='Select : ')
        self.radio_theme_builtin = Radiobutton(
                frame_theme, variable=self.is_builtin_theme, value=1,
                command=self.set_theme_type, text='a Built-in Theme')
        self.radio_theme_custom = Radiobutton(
                frame_theme, variable=self.is_builtin_theme, value=0,
                command=self.set_theme_type, text='a Custom Theme')
        self.opt_menu_theme_builtin = DynOptionMenu(
                frame_theme, self.builtin_theme, None, command=None)
        self.opt_menu_theme_custom=DynOptionMenu(
                frame_theme, self.custom_theme, None, command=None)
        self.button_delete_custom_theme=Button(
                frame_theme, text='Delete Custom Theme',
                command=self.delete_custom_theme)
        self.new_custom_theme = Label(frame_theme, bd=2)

        ##widget packing
        #body
        frame_custom.pack(side=LEFT, padx=5, pady=5, expand=TRUE, fill=BOTH)
        frame_theme.pack(side=LEFT, padx=5, pady=5, fill=Y)
        #frame_custom
        self.frame_colour_set.pack(side=TOP, padx=5, pady=5, expand=TRUE, fill=X)
        frame_fg_bg_toggle.pack(side=TOP, padx=5, pady=0)
        self.text_highlight_sample.pack(
                side=TOP, padx=5, pady=5, expand=TRUE, fill=BOTH)
        button_set_colour.pack(side=TOP, expand=TRUE, fill=X, padx=8, pady=4)
        self.opt_menu_highlight_target.pack(
                side=TOP, expand=TRUE, fill=X, padx=8, pady=3)
        self.radio_fg.pack(side=LEFT, anchor=E)
        self.radio_bg.pack(side=RIGHT, anchor=W)
        button_save_custom_theme.pack(side=BOTTOM, fill=X, padx=5, pady=5)
        #frame_theme
        theme_type_title.pack(side=TOP, anchor=W, padx=5, pady=5)
        self.radio_theme_builtin.pack(side=TOP, anchor=W, padx=5)
        self.radio_theme_custom.pack(side=TOP, anchor=W, padx=5, pady=2)
        self.opt_menu_theme_builtin.pack(side=TOP, fill=X, padx=5, pady=5)
        self.opt_menu_theme_custom.pack(side=TOP, fill=X, anchor=W, padx=5, pady=5)
        self.button_delete_custom_theme.pack(side=TOP, fill=X, padx=5, pady=5)
        self.new_custom_theme.pack(side=TOP, fill=X, pady=5)
        return frame

    def create_page_keys(self):
        parent = self.parent
        self.binding_target = StringVar(parent)
        self.builtin_keys = StringVar(parent)
        self.custom_keys = StringVar(parent)
        self.are_keys_builtin = BooleanVar(parent)
        self.keybinding = StringVar(parent)

        ##widget creation
        #body frame
        frame = self.tab_pages.pages['Keys'].frame
        #body section frames
        frame_custom = LabelFrame(
                frame, borderwidth=2, relief=GROOVE,
                text=' Custom Key Bindings ')
        frame_key_sets = LabelFrame(
                frame, borderwidth=2, relief=GROOVE, text=' Key Set ')
        #frame_custom
        frame_target = Frame(frame_custom)
        target_title = Label(frame_target, text='Action - Key(s)')
        scroll_target_y = Scrollbar(frame_target)
        scroll_target_x = Scrollbar(frame_target, orient=HORIZONTAL)
        self.list_bindings = Listbox(
                frame_target, takefocus=FALSE, exportselection=FALSE)
        self.list_bindings.bind('<ButtonRelease-1>', self.keybinding_selected)
        scroll_target_y.config(command=self.list_bindings.yview)
        scroll_target_x.config(command=self.list_bindings.xview)
        self.list_bindings.config(yscrollcommand=scroll_target_y.set)
        self.list_bindings.config(xscrollcommand=scroll_target_x.set)
        self.button_new_keys = Button(
                frame_custom, text='Get New Keys for Selection',
                command=self.get_new_keys, state=DISABLED)
        #frame_key_sets
        frames = [Frame(frame_key_sets, padx=2, pady=2, borderwidth=0)
                  for i in range(2)]
        self.radio_keys_builtin = Radiobutton(
                frames[0], variable=self.are_keys_builtin, value=1,
                command=self.set_keys_type, text='Use a Built-in Key Set')
        self.radio_keys_custom = Radiobutton(
                frames[0], variable=self.are_keys_builtin,  value=0,
                command=self.set_keys_type, text='Use a Custom Key Set')
        self.opt_menu_keys_builtin = DynOptionMenu(
                frames[0], self.builtin_keys, None, command=None)
        self.opt_menu_keys_custom = DynOptionMenu(
                frames[0], self.custom_keys, None, command=None)
        self.button_delete_custom_keys = Button(
                frames[1], text='Delete Custom Key Set',
                command=self.delete_custom_keys)
        button_save_custom_keys = Button(
                frames[1], text='Save as New Custom Key Set',
                command=self.save_as_new_key_set)
        self.new_custom_keys = Label(frames[0], bd=2)

        ##widget packing
        #body
        frame_custom.pack(side=BOTTOM, padx=5, pady=5, expand=TRUE, fill=BOTH)
        frame_key_sets.pack(side=BOTTOM, padx=5, pady=5, fill=BOTH)
        #frame_custom
        self.button_new_keys.pack(side=BOTTOM, fill=X, padx=5, pady=5)
        frame_target.pack(side=LEFT, padx=5, pady=5, expand=TRUE, fill=BOTH)
        #frame target
        frame_target.columnconfigure(0, weight=1)
        frame_target.rowconfigure(1, weight=1)
        target_title.grid(row=0, column=0, columnspan=2, sticky=W)
        self.list_bindings.grid(row=1, column=0, sticky=NSEW)
        scroll_target_y.grid(row=1, column=1, sticky=NS)
        scroll_target_x.grid(row=2, column=0, sticky=EW)
        #frame_key_sets
        self.radio_keys_builtin.grid(row=0, column=0, sticky=W+NS)
        self.radio_keys_custom.grid(row=1, column=0, sticky=W+NS)
        self.opt_menu_keys_builtin.grid(row=0, column=1, sticky=NSEW)
        self.opt_menu_keys_custom.grid(row=1, column=1, sticky=NSEW)
        self.new_custom_keys.grid(row=0, column=2, sticky=NSEW, padx=5, pady=5)
        self.button_delete_custom_keys.pack(side=LEFT, fill=X, expand=True, padx=2)
        button_save_custom_keys.pack(side=LEFT, fill=X, expand=True, padx=2)
        frames[0].pack(side=TOP, fill=BOTH, expand=True)
        frames[1].pack(side=TOP, fill=X, expand=True, pady=2)
        return frame

    def create_page_general(self):
        parent = self.parent
        self.win_width = StringVar(parent)
        self.win_height = StringVar(parent)
        self.startup_edit = IntVar(parent)
        self.autosave = IntVar(parent)
        self.encoding = StringVar(parent)
        self.user_help_browser = BooleanVar(parent)
        self.help_browser = StringVar(parent)

        #widget creation
        #body
        frame = self.tab_pages.pages['General'].frame
        #body section frames
        frame_run = LabelFrame(frame, borderwidth=2, relief=GROOVE,
                              text=' Startup Preferences ')
        frame_save = LabelFrame(frame, borderwidth=2, relief=GROOVE,
                               text=' autosave Preferences ')
        frame_win_size = Frame(frame, borderwidth=2, relief=GROOVE)
        frame_help = LabelFrame(frame, borderwidth=2, relief=GROOVE,
                               text=' Additional Help Sources ')
        #frame_run
        startup_title = Label(frame_run, text='At Startup')
        self.radio_startup_edit = Radiobutton(
                frame_run, variable=self.startup_edit, value=1,
                text="Open Edit Window")
        self.radio_startup_shell = Radiobutton(
                frame_run, variable=self.startup_edit, value=0,
                text='Open Shell Window')
        #frame_save
        run_save_title = Label(frame_save, text='At Start of Run (F5)  ')
        self.radio_save_ask = Radiobutton(
                frame_save, variable=self.autosave, value=0,
                text="Prompt to Save")
        self.radio_save_auto = Radiobutton(
                frame_save, variable=self.autosave, value=1,
                text='No Prompt')
        #frame_win_size
        win_size_title = Label(
                frame_win_size, text='Initial Window Size  (in characters)')
        win_width_title = Label(frame_win_size, text='Width')
        self.entry_win_width = Entry(
                frame_win_size, textvariable=self.win_width, width=3)
        win_height_title = Label(frame_win_size, text='Height')
        self.entry_win_height = Entry(
                frame_win_size, textvariable=self.win_height, width=3)
        #frame_help
        frame_helplist = Frame(frame_help)
        frame_helplist_buttons = Frame(frame_helplist)
        scroll_helplist = Scrollbar(frame_helplist)
        self.list_help = Listbox(
                frame_helplist, height=5, takefocus=FALSE,
                exportselection=FALSE)
        scroll_helplist.config(command=self.list_help.yview)
        self.list_help.config(yscrollcommand=scroll_helplist.set)
        self.list_help.bind('<ButtonRelease-1>', self.help_source_selected)
        self.button_helplist_edit = Button(
                frame_helplist_buttons, text='Edit', state=DISABLED,
                width=8, command=self.helplist_item_edit)
        self.button_helplist_add = Button(
                frame_helplist_buttons, text='Add',
                width=8, command=self.helplist_item_add)
        self.button_helplist_remove = Button(
                frame_helplist_buttons, text='Remove', state=DISABLED,
                width=8, command=self.helplist_item_remove)

        #widget packing
        #body
        frame_run.pack(side=TOP, padx=5, pady=5, fill=X)
        frame_save.pack(side=TOP, padx=5, pady=5, fill=X)
        frame_win_size.pack(side=TOP, padx=5, pady=5, fill=X)
        frame_help.pack(side=TOP, padx=5, pady=5, expand=TRUE, fill=BOTH)
        #frame_run
        startup_title.pack(side=LEFT, anchor=W, padx=5, pady=5)
        self.radio_startup_shell.pack(side=RIGHT, anchor=W, padx=5, pady=5)
        self.radio_startup_edit.pack(side=RIGHT, anchor=W, padx=5, pady=5)
        #frame_save
        run_save_title.pack(side=LEFT, anchor=W, padx=5, pady=5)
        self.radio_save_auto.pack(side=RIGHT, anchor=W, padx=5, pady=5)
        self.radio_save_ask.pack(side=RIGHT, anchor=W, padx=5, pady=5)
        #frame_win_size
        win_size_title.pack(side=LEFT, anchor=W, padx=5, pady=5)
        self.entry_win_height.pack(side=RIGHT, anchor=E, padx=10, pady=5)
        win_height_title.pack(side=RIGHT, anchor=E, pady=5)
        self.entry_win_width.pack(side=RIGHT, anchor=E, padx=10, pady=5)
        win_width_title.pack(side=RIGHT, anchor=E, pady=5)
        #frame_help
        frame_helplist_buttons.pack(side=RIGHT, padx=5, pady=5, fill=Y)
        frame_helplist.pack(side=TOP, padx=5, pady=5, expand=TRUE, fill=BOTH)
        scroll_helplist.pack(side=RIGHT, anchor=W, fill=Y)
        self.list_help.pack(side=LEFT, anchor=E, expand=TRUE, fill=BOTH)
        self.button_helplist_edit.pack(side=TOP, anchor=W, pady=5)
        self.button_helplist_add.pack(side=TOP, anchor=W)
        self.button_helplist_remove.pack(side=TOP, anchor=W, pady=5)
        return frame

    def attach_var_callbacks(self):
        self.font_size.trace_add('write', self.var_changed_font)
        self.font_name.trace_add('write', self.var_changed_font)
        self.font_bold.trace_add('write', self.var_changed_font)
        self.space_num.trace_add('write', self.var_changed_space_num)
        self.colour.trace_add('write', self.var_changed_colour)
        self.builtin_theme.trace_add('write', self.var_changed_builtin_theme)
        self.custom_theme.trace_add('write', self.var_changed_custom_theme)
        self.is_builtin_theme.trace_add('write', self.var_changed_is_builtin_theme)
        self.highlight_target.trace_add('write', self.var_changed_highlight_target)
        self.keybinding.trace_add('write', self.var_changed_keybinding)
        self.builtin_keys.trace_add('write', self.var_changed_builtin_keys)
        self.custom_keys.trace_add('write', self.var_changed_custom_keys)
        self.are_keys_builtin.trace_add('write', self.var_changed_are_keys_builtin)
        self.win_width.trace_add('write', self.var_changed_win_width)
        self.win_height.trace_add('write', self.var_changed_win_height)
        self.startup_edit.trace_add('write', self.var_changed_startup_edit)
        self.autosave.trace_add('write', self.var_changed_autosave)
        self.encoding.trace_add('write', self.var_changed_encoding)

    def remove_var_callbacks(self):
        "Remove callbacks to prevent memory leaks."
        for var in (
                self.font_size, self.font_name, self.font_bold,
                self.space_num, self.colour, self.builtin_theme,
                self.custom_theme, self.is_builtin_theme, self.highlight_target,
                self.keybinding, self.builtin_keys, self.custom_keys,
                self.are_keys_builtin, self.win_width, self.win_height,
                self.startup_edit, self.autosave, self.encoding,):
            var.trace_remove('write', var.trace_info()[0][1])

    def var_changed_font(self, *params):
        '''When one font attribute changes, save them all, as they are
        not independent from each other. In particular, when we are
        overriding the default font, we need to write out everything.
        '''
        value = self.font_name.get()
        self.add_changed_item('main', 'EditorWindow', 'font', value)
        value = self.font_size.get()
        self.add_changed_item('main', 'EditorWindow', 'font-size', value)
        value = self.font_bold.get()
        self.add_changed_item('main', 'EditorWindow', 'font-bold', value)

    def var_changed_space_num(self, *params):
        value = self.space_num.get()
        self.add_changed_item('main', 'Indent', 'num-spaces', value)

    def var_changed_colour(self, *params):
        self.on_new_colour_set()

    def var_changed_builtin_theme(self, *params):
        old_themes = ('IDLE Classic', 'IDLE New')
        value = self.builtin_theme.get()
        if value not in old_themes:
            if idleConf.GetOption('main', 'Theme', 'name') not in old_themes:
                self.add_changed_item('main', 'Theme', 'name', old_themes[0])
            self.add_changed_item('main', 'Theme', 'name2', value)
            self.new_custom_theme.config(text='New theme, see Help',
                                         fg='#500000')
        else:
            self.add_changed_item('main', 'Theme', 'name', value)
            self.add_changed_item('main', 'Theme', 'name2', '')
            self.new_custom_theme.config(text='', fg='black')
        self.paint_theme_sample()

    def var_changed_custom_theme(self, *params):
        value = self.custom_theme.get()
        if value != '- no custom themes -':
            self.add_changed_item('main', 'Theme', 'name', value)
            self.paint_theme_sample()

    def var_changed_is_builtin_theme(self, *params):
        value = self.is_builtin_theme.get()
        self.add_changed_item('main', 'Theme', 'default', value)
        if value:
            self.var_changed_builtin_theme()
        else:
            self.var_changed_custom_theme()

    def var_changed_highlight_target(self, *params):
        self.set_highlight_target()

    def var_changed_keybinding(self, *params):
        value = self.keybinding.get()
        key_set = self.custom_keys.get()
        event = self.list_bindings.get(ANCHOR).split()[0]
        if idleConf.IsCoreBinding(event):
            #this is a core keybinding
            self.add_changed_item('keys', key_set, event, value)
        else: #this is an extension key binding
            ext_name = idleConf.GetExtnNameForEvent(event)
            ext_keybind_section = ext_name + '_cfgBindings'
            self.add_changed_item('extensions', ext_keybind_section, event, value)

    def var_changed_builtin_keys(self, *params):
        old_keys = (
            'IDLE Classic Windows',
            'IDLE Classic Unix',
            'IDLE Classic Mac',
            'IDLE Classic OSX',
        )
        value = self.builtin_keys.get()
        if value not in old_keys:
            if idleConf.GetOption('main', 'Keys', 'name') not in old_keys:
                self.add_changed_item('main', 'Keys', 'name', old_keys[0])
            self.add_changed_item('main', 'Keys', 'name2', value)
            self.new_custom_keys.config(text='New key set, see Help',
                                        fg='#500000')
        else:
            self.add_changed_item('main', 'Keys', 'name', value)
            self.add_changed_item('main', 'Keys', 'name2', '')
            self.new_custom_keys.config(text='', fg='black')
        self.load_keys_list(value)

    def var_changed_custom_keys(self, *params):
        value = self.custom_keys.get()
        if value != '- no custom keys -':
            self.add_changed_item('main', 'Keys', 'name', value)
            self.load_keys_list(value)

    def var_changed_are_keys_builtin(self, *params):
        value = self.are_keys_builtin.get()
        self.add_changed_item('main', 'Keys', 'default', value)
        if value:
            self.var_changed_builtin_keys()
        else:
            self.var_changed_custom_keys()

    def var_changed_win_width(self, *params):
        value = self.win_width.get()
        self.add_changed_item('main', 'EditorWindow', 'width', value)

    def var_changed_win_height(self, *params):
        value = self.win_height.get()
        self.add_changed_item('main', 'EditorWindow', 'height', value)

    def var_changed_startup_edit(self, *params):
        value = self.startup_edit.get()
        self.add_changed_item('main', 'General', 'editor-on-startup', value)

    def var_changed_autosave(self, *params):
        value = self.autosave.get()
        self.add_changed_item('main', 'General', 'autosave', value)

    def var_changed_encoding(self, *params):
        value = self.encoding.get()
        self.add_changed_item('main', 'EditorWindow', 'encoding', value)

    def reset_changed_items(self):
        #When any config item is changed in this dialog, an entry
        #should be made in the relevant section (config type) of this
        #dictionary. The key should be the config file section name and the
        #value a dictionary, whose key:value pairs are item=value pairs for
        #that config file section.
        self.changed_items = {'main':{}, 'highlight':{}, 'keys':{},
                             'extensions':{}}

    def add_changed_item(self, typ, section, item, value):
        value = str(value) #make sure we use a string
        if section not in self.changed_items[typ]:
            self.changed_items[typ][section] = {}
        self.changed_items[typ][section][item] = value

    def GetDefaultItems(self):
        d_items={'main':{}, 'highlight':{}, 'keys':{}, 'extensions':{}}
        for config_type in d_items:
            sections = idleConf.GetSectionList('default', config_type)
            for section in sections:
                d_items[config_type][section] = {}
                options = idleConf.defaultCfg[config_type].GetOptionList(section)
                for option in options:
                    d_items[config_type][section][option] = (
                            idleConf.defaultCfg[config_type].Get(section, option))
        return d_items

    def set_theme_type(self):
        if self.is_builtin_theme.get():
            self.opt_menu_theme_builtin.config(state=NORMAL)
            self.opt_menu_theme_custom.config(state=DISABLED)
            self.button_delete_custom_theme.config(state=DISABLED)
        else:
            self.opt_menu_theme_builtin.config(state=DISABLED)
            self.radio_theme_custom.config(state=NORMAL)
            self.opt_menu_theme_custom.config(state=NORMAL)
            self.button_delete_custom_theme.config(state=NORMAL)

    def set_keys_type(self):
        if self.are_keys_builtin.get():
            self.opt_menu_keys_builtin.config(state=NORMAL)
            self.opt_menu_keys_custom.config(state=DISABLED)
            self.button_delete_custom_keys.config(state=DISABLED)
        else:
            self.opt_menu_keys_builtin.config(state=DISABLED)
            self.radio_keys_custom.config(state=NORMAL)
            self.opt_menu_keys_custom.config(state=NORMAL)
            self.button_delete_custom_keys.config(state=NORMAL)

    def get_new_keys(self):
        list_index = self.list_bindings.index(ANCHOR)
        binding = self.list_bindings.get(list_index)
        bind_name = binding.split()[0] #first part, up to first space
        if self.are_keys_builtin.get():
            current_key_set_name = self.builtin_keys.get()
        else:
            current_key_set_name = self.custom_keys.get()
        current_bindings = idleConf.GetCurrentKeySet()
        if current_key_set_name in self.changed_items['keys']: #unsaved changes
            key_set_changes = self.changed_items['keys'][current_key_set_name]
            for event in key_set_changes:
                current_bindings[event] = key_set_changes[event].split()
        current_key_sequences = list(current_bindings.values())
        new_keys = GetKeysDialog(self, 'Get New Keys', bind_name,
                current_key_sequences).result
        if new_keys: #new keys were specified
            if self.are_keys_builtin.get(): #current key set is a built-in
                message = ('Your changes will be saved as a new Custom Key Set.'
                           ' Enter a name for your new Custom Key Set below.')
                new_keyset = self.get_new_keys_name(message)
                if not new_keyset: #user cancelled custom key set creation
                    self.list_bindings.select_set(list_index)
                    self.list_bindings.select_anchor(list_index)
                    return
                else: #create new custom key set based on previously active key set
                    self.create_new_key_set(new_keyset)
            self.list_bindings.delete(list_index)
            self.list_bindings.insert(list_index, bind_name+' - '+new_keys)
            self.list_bindings.select_set(list_index)
            self.list_bindings.select_anchor(list_index)
            self.keybinding.set(new_keys)
        else:
            self.list_bindings.select_set(list_index)
            self.list_bindings.select_anchor(list_index)

    def get_new_keys_name(self, message):
        used_names = (idleConf.GetSectionList('user', 'keys') +
                idleConf.GetSectionList('default', 'keys'))
        new_keyset = SectionName(
                self, 'New Custom Key Set', message, used_names).result
        return new_keyset

    def save_as_new_key_set(self):
        new_keys_name = self.get_new_keys_name('New Key Set Name:')
        if new_keys_name:
            self.create_new_key_set(new_keys_name)

    def keybinding_selected(self, event):
        self.button_new_keys.config(state=NORMAL)

    def create_new_key_set(self, new_key_set_name):
        #creates new custom key set based on the previously active key set,
        #and makes the new key set active
        if self.are_keys_builtin.get():
            prev_key_set_name = self.builtin_keys.get()
        else:
            prev_key_set_name = self.custom_keys.get()
        prev_keys = idleConf.GetCoreKeys(prev_key_set_name)
        new_keys = {}
        for event in prev_keys: #add key set to changed items
            event_name = event[2:-2] #trim off the angle brackets
            binding = ' '.join(prev_keys[event])
            new_keys[event_name] = binding
        #handle any unsaved changes to prev key set
        if prev_key_set_name in self.changed_items['keys']:
            key_set_changes = self.changed_items['keys'][prev_key_set_name]
            for event in key_set_changes:
                new_keys[event] = key_set_changes[event]
        #save the new theme
        self.save_new_key_set(new_key_set_name, new_keys)
        #change gui over to the new key set
        custom_key_list = idleConf.GetSectionList('user', 'keys')
        custom_key_list.sort()
        self.opt_menu_keys_custom.SetMenu(custom_key_list, new_key_set_name)
        self.are_keys_builtin.set(0)
        self.set_keys_type()

    def load_keys_list(self, keyset_name):
        reselect = 0
        new_keyset = 0
        if self.list_bindings.curselection():
            reselect = 1
            list_index = self.list_bindings.index(ANCHOR)
        keyset = idleConf.GetKeySet(keyset_name)
        bind_names = list(keyset.keys())
        bind_names.sort()
        self.list_bindings.delete(0, END)
        for bind_name in bind_names:
            key = ' '.join(keyset[bind_name]) #make key(s) into a string
            bind_name = bind_name[2:-2] #trim off the angle brackets
            if keyset_name in self.changed_items['keys']:
                #handle any unsaved changes to this key set
                if bind_name in self.changed_items['keys'][keyset_name]:
                    key = self.changed_items['keys'][keyset_name][bind_name]
            self.list_bindings.insert(END, bind_name+' - '+key)
        if reselect:
            self.list_bindings.see(list_index)
            self.list_bindings.select_set(list_index)
            self.list_bindings.select_anchor(list_index)

    def delete_custom_keys(self):
        keyset_name=self.custom_keys.get()
        delmsg = 'Are you sure you wish to delete the key set %r ?'
        if not tkMessageBox.askyesno(
                'Delete Key Set',  delmsg % keyset_name, parent=self):
            return
        self.deactivate_current_config()
        #remove key set from config
        idleConf.userCfg['keys'].remove_section(keyset_name)
        if keyset_name in self.changed_items['keys']:
            del(self.changed_items['keys'][keyset_name])
        #write changes
        idleConf.userCfg['keys'].Save()
        #reload user key set list
        item_list = idleConf.GetSectionList('user', 'keys')
        item_list.sort()
        if not item_list:
            self.radio_keys_custom.config(state=DISABLED)
            self.opt_menu_keys_custom.SetMenu(item_list, '- no custom keys -')
        else:
            self.opt_menu_keys_custom.SetMenu(item_list, item_list[0])
        #revert to default key set
        self.are_keys_builtin.set(idleConf.defaultCfg['main']
                                .Get('Keys', 'default'))
        self.builtin_keys.set(idleConf.defaultCfg['main'].Get('Keys', 'name')
                             or idleConf.default_keys())
        #user can't back out of these changes, they must be applied now
        self.save_all_changed_configs()
        self.activate_config_changes()
        self.set_keys_type()

    def delete_custom_theme(self):
        theme_name = self.custom_theme.get()
        delmsg = 'Are you sure you wish to delete the theme %r ?'
        if not tkMessageBox.askyesno(
                'Delete Theme',  delmsg % theme_name, parent=self):
            return
        self.deactivate_current_config()
        #remove theme from config
        idleConf.userCfg['highlight'].remove_section(theme_name)
        if theme_name in self.changed_items['highlight']:
            del(self.changed_items['highlight'][theme_name])
        #write changes
        idleConf.userCfg['highlight'].Save()
        #reload user theme list
        item_list = idleConf.GetSectionList('user', 'highlight')
        item_list.sort()
        if not item_list:
            self.radio_theme_custom.config(state=DISABLED)
            self.opt_menu_theme_custom.SetMenu(item_list, '- no custom themes -')
        else:
            self.opt_menu_theme_custom.SetMenu(item_list, item_list[0])
        #revert to default theme
        self.is_builtin_theme.set(idleConf.defaultCfg['main'].Get('Theme', 'default'))
        self.builtin_theme.set(idleConf.defaultCfg['main'].Get('Theme', 'name'))
        #user can't back out of these changes, they must be applied now
        self.save_all_changed_configs()
        self.activate_config_changes()
        self.set_theme_type()

    def get_colour(self):
        target = self.highlight_target.get()
        prev_colour = self.frame_colour_set.cget('bg')
        rgbTuplet, colour_string = tkColorChooser.askcolor(
                parent=self, title='Pick new colour for : '+target,
                initialcolor=prev_colour)
        if colour_string and (colour_string != prev_colour):
            #user didn't cancel, and they chose a new colour
            if self.is_builtin_theme.get():  #current theme is a built-in
                message = ('Your changes will be saved as a new Custom Theme. '
                           'Enter a name for your new Custom Theme below.')
                new_theme = self.get_new_theme_name(message)
                if not new_theme:  #user cancelled custom theme creation
                    return
                else:  #create new custom theme based on previously active theme
                    self.create_new_theme(new_theme)
                    self.colour.set(colour_string)
            else:  #current theme is user defined
                self.colour.set(colour_string)

    def on_new_colour_set(self):
        new_colour=self.colour.get()
        self.frame_colour_set.config(bg=new_colour)  #set sample
        plane ='foreground' if self.fg_bg_toggle.get() else 'background'
        sample_element = self.theme_elements[self.highlight_target.get()][0]
        self.text_highlight_sample.tag_config(sample_element, **{plane:new_colour})
        theme = self.custom_theme.get()
        theme_element = sample_element + '-' + plane
        self.add_changed_item('highlight', theme, theme_element, new_colour)

    def get_new_theme_name(self, message):
        used_names = (idleConf.GetSectionList('user', 'highlight') +
                idleConf.GetSectionList('default', 'highlight'))
        new_theme = SectionName(
                self, 'New Custom Theme', message, used_names).result
        return new_theme

    def save_as_new_theme(self):
        new_theme_name = self.get_new_theme_name('New Theme Name:')
        if new_theme_name:
            self.create_new_theme(new_theme_name)

    def create_new_theme(self, new_theme_name):
        #creates new custom theme based on the previously active theme,
        #and makes the new theme active
        if self.is_builtin_theme.get():
            theme_type = 'default'
            theme_name = self.builtin_theme.get()
        else:
            theme_type = 'user'
            theme_name = self.custom_theme.get()
        new_theme = idleConf.GetThemeDict(theme_type, theme_name)
        #apply any of the old theme's unsaved changes to the new theme
        if theme_name in self.changed_items['highlight']:
            theme_changes = self.changed_items['highlight'][theme_name]
            for element in theme_changes:
                new_theme[element] = theme_changes[element]
        #save the new theme
        self.save_new_theme(new_theme_name, new_theme)
        #change gui over to the new theme
        custom_theme_list = idleConf.GetSectionList('user', 'highlight')
        custom_theme_list.sort()
        self.opt_menu_theme_custom.SetMenu(custom_theme_list, new_theme_name)
        self.is_builtin_theme.set(0)
        self.set_theme_type()

    def on_list_fonts_button_release(self, event):
        font = self.list_fonts.get(ANCHOR)
        self.font_name.set(font.lower())
        self.set_font_sample()

    def set_font_sample(self, event=None):
        font_name = self.font_name.get()
        font_weight = tkFont.BOLD if self.font_bold.get() else tkFont.NORMAL
        new_font = (font_name, self.font_size.get(), font_weight)
        self.font_sample.config(font=new_font)
        self.text_highlight_sample.configure(font=new_font)

    def set_highlight_target(self):
        if self.highlight_target.get() == 'Cursor':  #bg not possible
            self.radio_fg.config(state=DISABLED)
            self.radio_bg.config(state=DISABLED)
            self.fg_bg_toggle.set(1)
        else:  #both fg and bg can be set
            self.radio_fg.config(state=NORMAL)
            self.radio_bg.config(state=NORMAL)
            self.fg_bg_toggle.set(1)
        self.set_colour_sample()

    def set_colour_sample_binding(self, *args):
        self.set_colour_sample()

    def set_colour_sample(self):
        #set the colour smaple area
        tag = self.theme_elements[self.highlight_target.get()][0]
        plane = 'foreground' if self.fg_bg_toggle.get() else 'background'
        colour = self.text_highlight_sample.tag_cget(tag, plane)
        self.frame_colour_set.config(bg=colour)

    def paint_theme_sample(self):
        if self.is_builtin_theme.get():  #a default theme
            theme = self.builtin_theme.get()
        else:  #a user theme
            theme = self.custom_theme.get()
        for element_title in self.theme_elements:
            element = self.theme_elements[element_title][0]
            colours = idleConf.GetHighlight(theme, element)
            if element == 'cursor': #cursor sample needs special painting
                colours['background'] = idleConf.GetHighlight(
                        theme, 'normal', fgBg='bg')
            #handle any unsaved changes to this theme
            if theme in self.changed_items['highlight']:
                theme_dict = self.changed_items['highlight'][theme]
                if element + '-foreground' in theme_dict:
                    colours['foreground'] = theme_dict[element + '-foreground']
                if element + '-background' in theme_dict:
                    colours['background'] = theme_dict[element + '-background']
            self.text_highlight_sample.tag_config(element, **colours)
        self.set_colour_sample()

    def help_source_selected(self, event):
        self.set_helplist_button_states()

    def set_helplist_button_states(self):
        if self.list_help.size() < 1:  #no entries in list
            self.button_helplist_edit.config(state=DISABLED)
            self.button_helplist_remove.config(state=DISABLED)
        else: #there are some entries
            if self.list_help.curselection():  #there currently is a selection
                self.button_helplist_edit.config(state=NORMAL)
                self.button_helplist_remove.config(state=NORMAL)
            else:  #there currently is not a selection
                self.button_helplist_edit.config(state=DISABLED)
                self.button_helplist_remove.config(state=DISABLED)

    def helplist_item_add(self):
        help_source = HelpSource(self, 'New Help Source',
                                ).result
        if help_source:
            self.user_helplist.append((help_source[0], help_source[1]))
            self.list_help.insert(END, help_source[0])
            self.update_user_help_changed_items()
        self.set_helplist_button_states()

    def helplist_item_edit(self):
        item_index = self.list_help.index(ANCHOR)
        help_source = self.user_helplist[item_index]
        new_help_source = HelpSource(
                self, 'Edit Help Source',
                menuitem=help_source[0],
                filepath=help_source[1],
                ).result
        if new_help_source and new_help_source != help_source:
            self.user_helplist[item_index] = new_help_source
            self.list_help.delete(item_index)
            self.list_help.insert(item_index, new_help_source[0])
            self.update_user_help_changed_items()
            self.set_helplist_button_states()

    def helplist_item_remove(self):
        item_index = self.list_help.index(ANCHOR)
        del(self.user_helplist[item_index])
        self.list_help.delete(item_index)
        self.update_user_help_changed_items()
        self.set_helplist_button_states()

    def update_user_help_changed_items(self):
        "Clear and rebuild the HelpFiles section in self.changed_items"
        self.changed_items['main']['HelpFiles'] = {}
        for num in range(1, len(self.user_helplist) + 1):
            self.add_changed_item(
                    'main', 'HelpFiles', str(num),
                    ';'.join(self.user_helplist[num-1][:2]))

    def load_font_cfg(self):
        ##base editor font selection list
        fonts = list(tkFont.families(self))
        fonts.sort()
        for font in fonts:
            self.list_fonts.insert(END, font)
        configured_font = idleConf.GetFont(self, 'main', 'EditorWindow')
        font_name = configured_font[0].lower()
        font_size = configured_font[1]
        font_bold  = configured_font[2]=='bold'
        self.font_name.set(font_name)
        lc_fonts = [s.lower() for s in fonts]
        try:
            current_font_index = lc_fonts.index(font_name)
            self.list_fonts.see(current_font_index)
            self.list_fonts.select_set(current_font_index)
            self.list_fonts.select_anchor(current_font_index)
        except ValueError:
            pass
        ##font size dropdown
        self.opt_menu_font_size.SetMenu(('7', '8', '9', '10', '11', '12', '13',
                                      '14', '16', '18', '20', '22',
                                      '25', '29', '34', '40'), font_size )
        ##font_weight
        self.font_bold.set(font_bold)
        ##font sample
        self.set_font_sample()

    def load_tab_cfg(self):
        ##indent sizes
        space_num = idleConf.GetOption(
            'main', 'Indent', 'num-spaces', default=4, type='int')
        self.space_num.set(space_num)

    def load_theme_cfg(self):
        ##current theme type radiobutton
        self.is_builtin_theme.set(idleConf.GetOption(
                'main', 'Theme', 'default', type='bool', default=1))
        ##currently set theme
        current_option = idleConf.CurrentTheme()
        ##load available theme option menus
        if self.is_builtin_theme.get(): #default theme selected
            item_list = idleConf.GetSectionList('default', 'highlight')
            item_list.sort()
            self.opt_menu_theme_builtin.SetMenu(item_list, current_option)
            item_list = idleConf.GetSectionList('user', 'highlight')
            item_list.sort()
            if not item_list:
                self.radio_theme_custom.config(state=DISABLED)
                self.custom_theme.set('- no custom themes -')
            else:
                self.opt_menu_theme_custom.SetMenu(item_list, item_list[0])
        else: #user theme selected
            item_list = idleConf.GetSectionList('user', 'highlight')
            item_list.sort()
            self.opt_menu_theme_custom.SetMenu(item_list, current_option)
            item_list = idleConf.GetSectionList('default', 'highlight')
            item_list.sort()
            self.opt_menu_theme_builtin.SetMenu(item_list, item_list[0])
        self.set_theme_type()
        ##load theme element option menu
        theme_names = list(self.theme_elements.keys())
        theme_names.sort(key=lambda x: self.theme_elements[x][1])
        self.opt_menu_highlight_target.SetMenu(theme_names, theme_names[0])
        self.paint_theme_sample()
        self.set_highlight_target()

    def load_key_cfg(self):
        ##current keys type radiobutton
        self.are_keys_builtin.set(idleConf.GetOption(
                'main', 'Keys', 'default', type='bool', default=1))
        ##currently set keys
        current_option = idleConf.CurrentKeys()
        ##load available keyset option menus
        if self.are_keys_builtin.get(): #default theme selected
            item_list = idleConf.GetSectionList('default', 'keys')
            item_list.sort()
            self.opt_menu_keys_builtin.SetMenu(item_list, current_option)
            item_list = idleConf.GetSectionList('user', 'keys')
            item_list.sort()
            if not item_list:
                self.radio_keys_custom.config(state=DISABLED)
                self.custom_keys.set('- no custom keys -')
            else:
                self.opt_menu_keys_custom.SetMenu(item_list, item_list[0])
        else: #user key set selected
            item_list = idleConf.GetSectionList('user', 'keys')
            item_list.sort()
            self.opt_menu_keys_custom.SetMenu(item_list, current_option)
            item_list = idleConf.GetSectionList('default', 'keys')
            item_list.sort()
            self.opt_menu_keys_builtin.SetMenu(item_list, idleConf.default_keys())
        self.set_keys_type()
        ##load keyset element list
        keyset_name = idleConf.CurrentKeys()
        self.load_keys_list(keyset_name)

    def load_general_cfg(self):
        #startup state
        self.startup_edit.set(idleConf.GetOption(
                'main', 'General', 'editor-on-startup', default=1, type='bool'))
        #autosave state
        self.autosave.set(idleConf.GetOption(
                'main', 'General', 'autosave', default=0, type='bool'))
        #initial window size
        self.win_width.set(idleConf.GetOption(
                'main', 'EditorWindow', 'width', type='int'))
        self.win_height.set(idleConf.GetOption(
                'main', 'EditorWindow', 'height', type='int'))
        # default source encoding
        self.encoding.set(idleConf.GetOption(
                'main', 'EditorWindow', 'encoding', default='none'))
        # additional help sources
        self.user_helplist = idleConf.GetAllExtraHelpSourcesList()
        for help_item in self.user_helplist:
            self.list_help.insert(END, help_item[0])
        self.set_helplist_button_states()

    def load_configs(self):
        """
        load configuration from default and user config files and populate
        the widgets on the config dialog pages.
        """
        ### fonts / tabs page
        self.load_font_cfg()
        self.load_tab_cfg()
        ### highlighting page
        self.load_theme_cfg()
        ### keys page
        self.load_key_cfg()
        ### general page
        self.load_general_cfg()
        # note: extension page handled separately

    def save_new_key_set(self, keyset_name, keyset):
        """
        save a newly created core key set.
        keyset_name - string, the name of the new key set
        keyset - dictionary containing the new key set
        """
        if not idleConf.userCfg['keys'].has_section(keyset_name):
            idleConf.userCfg['keys'].add_section(keyset_name)
        for event in keyset:
            value = keyset[event]
            idleConf.userCfg['keys'].SetOption(keyset_name, event, value)

    def save_new_theme(self, theme_name, theme):
        """
        save a newly created theme.
        theme_name - string, the name of the new theme
        theme - dictionary containing the new theme
        """
        if not idleConf.userCfg['highlight'].has_section(theme_name):
            idleConf.userCfg['highlight'].add_section(theme_name)
        for element in theme:
            value = theme[element]
            idleConf.userCfg['highlight'].SetOption(theme_name, element, value)

    def set_user_value(self, config_type, section, item, value):
        if idleConf.defaultCfg[config_type].has_option(section, item):
            if idleConf.defaultCfg[config_type].Get(section, item) == value:
                #the setting equals a default setting, remove it from user cfg
                return idleConf.userCfg[config_type].RemoveOption(section, item)
        #if we got here set the option
        return idleConf.userCfg[config_type].SetOption(section, item, value)

    def save_all_changed_configs(self):
        "Save configuration changes to the user config file."
        idleConf.userCfg['main'].Save()
        for config_type in self.changed_items:
            cfg_type_changed = False
            for section in self.changed_items[config_type]:
                if section == 'HelpFiles':
                    #this section gets completely replaced
                    idleConf.userCfg['main'].remove_section('HelpFiles')
                    cfg_type_changed = True
                for item in self.changed_items[config_type][section]:
                    value = self.changed_items[config_type][section][item]
                    if self.set_user_value(config_type, section, item, value):
                        cfg_type_changed = True
            if cfg_type_changed:
                idleConf.userCfg[config_type].Save()
        for config_type in ['keys', 'highlight']:
            # save these even if unchanged!
            idleConf.userCfg[config_type].Save()
        self.reset_changed_items() #clear the changed items dict
        self.save_all_changed_extensions()  # uses a different mechanism

    def deactivate_current_config(self):
        #Before a config is saved, some cleanup of current
        #config must be done - remove the previous keybindings
        win_instances = self.parent.instance_dict.keys()
        for instance in win_instances:
            instance.RemoveKeybindings()

    def activate_config_changes(self):
        "Dynamically apply configuration changes"
        win_instances = self.parent.instance_dict.keys()
        for instance in win_instances:
            instance.ResetColorizer()
            instance.ResetFont()
            instance.set_notabs_indentwidth()
            instance.ApplyKeybindings()
            instance.reset_help_menu_entries()

    def cancel(self):
        self.destroy()

    def ok(self):
        self.apply()
        self.destroy()

    def apply(self):
        self.deactivate_current_config()
        self.save_all_changed_configs()
        self.activate_config_changes()

    def help(self):
        page = self.tab_pages._current_page
        view_text(self, title='Help for IDLE preferences',
                 text=help_common+help_pages.get(page, ''))

    def create_page_extensions(self):
        """Part of the config dialog used for configuring IDLE extensions.

        This code is generic - it works for any and all IDLE extensions.

        IDLE extensions save their configuration options using idleConf.
        This code reads the current configuration using idleConf, supplies a
        GUI interface to change the configuration values, and saves the
        changes using idleConf.

        Not all changes take effect immediately - some may require restarting IDLE.
        This depends on each extension's implementation.

        All values are treated as text, and it is up to the user to supply
        reasonable values. The only exception to this are the 'enable*' options,
        which are boolean, and can be toggled with a True/False button.
        """
        parent = self.parent
        frame = self.tab_pages.pages['Extensions'].frame
        self.ext_defaultCfg = idleConf.defaultCfg['extensions']
        self.ext_userCfg = idleConf.userCfg['extensions']
        self.is_int = self.register(is_int)
        self.load_extensions()
        # create widgets - a listbox shows all available extensions, with the
        # controls for the extension selected in the listbox to the right
        self.extension_names = StringVar(self)
        frame.rowconfigure(0, weight=1)
        frame.columnconfigure(2, weight=1)
        self.extension_list = Listbox(frame, listvariable=self.extension_names,
                                      selectmode='browse')
        self.extension_list.bind('<<ListboxSelect>>', self.extension_selected)
        scroll = Scrollbar(frame, command=self.extension_list.yview)
        self.extension_list.yscrollcommand=scroll.set
        self.details_frame = LabelFrame(frame, width=250, height=250)
        self.extension_list.grid(column=0, row=0, sticky='nws')
        scroll.grid(column=1, row=0, sticky='ns')
        self.details_frame.grid(column=2, row=0, sticky='nsew', padx=[10, 0])
        frame.configure(padx=10, pady=10)
        self.config_frame = {}
        self.current_extension = None

        self.outerframe = self                      # TEMPORARY
        self.tabbed_page_set = self.extension_list  # TEMPORARY

        # create the frame holding controls for each extension
        ext_names = ''
        for ext_name in sorted(self.extensions):
            self.create_extension_frame(ext_name)
            ext_names = ext_names + '{' + ext_name + '} '
        self.extension_names.set(ext_names)
        self.extension_list.selection_set(0)
        self.extension_selected(None)

    def load_extensions(self):
        "Fill self.extensions with data from the default and user configs."
        self.extensions = {}
        for ext_name in idleConf.GetExtensions(active_only=False):
            self.extensions[ext_name] = []

        for ext_name in self.extensions:
            opt_list = sorted(self.ext_defaultCfg.GetOptionList(ext_name))

            # bring 'enable' options to the beginning of the list
            enables = [opt_name for opt_name in opt_list
                       if opt_name.startswith('enable')]
            for opt_name in enables:
                opt_list.remove(opt_name)
            opt_list = enables + opt_list

            for opt_name in opt_list:
                def_str = self.ext_defaultCfg.Get(
                        ext_name, opt_name, raw=True)
                try:
                    def_obj = {'True':True, 'False':False}[def_str]
                    opt_type = 'bool'
                except KeyError:
                    try:
                        def_obj = int(def_str)
                        opt_type = 'int'
                    except ValueError:
                        def_obj = def_str
                        opt_type = None
                try:
                    value = self.ext_userCfg.Get(
                            ext_name, opt_name, type=opt_type, raw=True,
                            default=def_obj)
                except ValueError:  # Need this until .Get fixed
                    value = def_obj  # bad values overwritten by entry
                var = StringVar(self)
                var.set(str(value))

                self.extensions[ext_name].append({'name': opt_name,
                                                  'type': opt_type,
                                                  'default': def_str,
                                                  'value': value,
                                                  'var': var,
                                                 })

    def extension_selected(self, event):
        newsel = self.extension_list.curselection()
        if newsel:
            newsel = self.extension_list.get(newsel)
        if newsel is None or newsel != self.current_extension:
            if self.current_extension:
                self.details_frame.config(text='')
                self.config_frame[self.current_extension].grid_forget()
                self.current_extension = None
        if newsel:
            self.details_frame.config(text=newsel)
            self.config_frame[newsel].grid(column=0, row=0, sticky='nsew')
            self.current_extension = newsel

    def create_extension_frame(self, ext_name):
        """Create a frame holding the widgets to configure one extension"""
        f = VerticalScrolledFrame(self.details_frame, height=250, width=250)
        self.config_frame[ext_name] = f
        entry_area = f.interior
        # create an entry for each configuration option
        for row, opt in enumerate(self.extensions[ext_name]):
            # create a row with a label and entry/checkbutton
            label = Label(entry_area, text=opt['name'])
            label.grid(row=row, column=0, sticky=NW)
            var = opt['var']
            if opt['type'] == 'bool':
                Checkbutton(entry_area, textvariable=var, variable=var,
                            onvalue='True', offvalue='False',
                            indicatoron=FALSE, selectcolor='', width=8
                            ).grid(row=row, column=1, sticky=W, padx=7)
            elif opt['type'] == 'int':
                Entry(entry_area, textvariable=var, validate='key',
                      validatecommand=(self.is_int, '%P')
                      ).grid(row=row, column=1, sticky=NSEW, padx=7)

            else:
                Entry(entry_area, textvariable=var
                      ).grid(row=row, column=1, sticky=NSEW, padx=7)
        return

    def set_extension_value(self, section, opt):
        name = opt['name']
        default = opt['default']
        value = opt['var'].get().strip() or default
        opt['var'].set(value)
        # if self.defaultCfg.has_section(section):
        # Currently, always true; if not, indent to return
        if (value == default):
            return self.ext_userCfg.RemoveOption(section, name)
        # set the option
        return self.ext_userCfg.SetOption(section, name, value)

    def save_all_changed_extensions(self):
        """Save configuration changes to the user config file."""
        has_changes = False
        for ext_name in self.extensions:
            options = self.extensions[ext_name]
            for opt in options:
                if self.set_extension_value(ext_name, opt):
                    has_changes = True
        if has_changes:
            self.ext_userCfg.Save()


help_common = '''\
When you click either the Apply or Ok buttons, settings in this
dialog that are different from IDLE's default are saved in
a .idlerc directory in your home directory. Except as noted,
these changes apply to all versions of IDLE installed on this
machine. Some do not take affect until IDLE is restarted.
[Cancel] only cancels changes made since the last save.
'''
help_pages = {
    'Highlighting': '''
Highlighting:
The IDLE Dark color theme is new in October 2015.  It can only
be used with older IDLE releases if it is saved as a custom
theme, with a different name.
''',
    'Keys': '''
Keys:
The IDLE Modern Unix key set is new in June 2016.  It can only
be used with older IDLE releases if it is saved as a custom
key set, with a different name.
''',
    'Extensions': '''
Extensions:

Autocomplete: Popupwait is milleseconds to wait after key char, without
cursor movement, before popping up completion box.  Key char is '.' after
identifier or a '/' (or '\\' on Windows) within a string.

FormatParagraph: Max-width is max chars in lines after re-formatting.
Use with paragraphs in both strings and comment blocks.

ParenMatch: Style indicates what is highlighted when closer is entered:
'opener' - opener '({[' corresponding to closer; 'parens' - both chars;
'expression' (default) - also everything in between.  Flash-delay is how
long to highlight if cursor is not moved (0 means forever).
'''
}


def is_int(s):
    "Return 's is blank or represents an int'"
    if not s:
        return True
    try:
        int(s)
        return True
    except ValueError:
        return False


class VerticalScrolledFrame(Frame):
    """A pure Tkinter vertically scrollable frame.

    * Use the 'interior' attribute to place widgets inside the scrollable frame
    * Construct and pack/place/grid normally
    * This frame only allows vertical scrolling
    """
    def __init__(self, parent, *args, **kw):
        Frame.__init__(self, parent, *args, **kw)

        # create a canvas object and a vertical scrollbar for scrolling it
        vscrollbar = Scrollbar(self, orient=VERTICAL)
        vscrollbar.pack(fill=Y, side=RIGHT, expand=FALSE)
        canvas = Canvas(self, bd=0, highlightthickness=0,
                        yscrollcommand=vscrollbar.set, width=240)
        canvas.pack(side=LEFT, fill=BOTH, expand=TRUE)
        vscrollbar.config(command=canvas.yview)

        # reset the view
        canvas.xview_moveto(0)
        canvas.yview_moveto(0)

        # create a frame inside the canvas which will be scrolled with it
        self.interior = interior = Frame(canvas)
        interior_id = canvas.create_window(0, 0, window=interior, anchor=NW)

        # track changes to the canvas and frame width and sync them,
        # also updating the scrollbar
        def _configure_interior(event):
            # update the scrollbars to match the size of the inner frame
            size = (interior.winfo_reqwidth(), interior.winfo_reqheight())
            canvas.config(scrollregion="0 0 %s %s" % size)
        interior.bind('<Configure>', _configure_interior)

        def _configure_canvas(event):
            if interior.winfo_reqwidth() != canvas.winfo_width():
                # update the inner frame's width to fill the canvas
                canvas.itemconfigure(interior_id, width=canvas.winfo_width())
        canvas.bind('<Configure>', _configure_canvas)

        return


if __name__ == '__main__':
    import unittest
    unittest.main('idlelib.idle_test.test_configdialog',
                  verbosity=2, exit=False)
    from idlelib.idle_test.htest import run
    run(ConfigDialog)
