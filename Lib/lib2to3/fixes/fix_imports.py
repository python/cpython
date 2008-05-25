"""Fix incompatible imports and module references.

Fixes:
  * StringIO -> io
  * cStringIO -> io
  * md5 -> hashlib
"""
# Author: Collin Winter

# Local imports
from . import basefix
from .util import Name, attr_chain, any, set
import __builtin__
builtin_names = [name for name in dir(__builtin__)
                 if name not in ("__name__", "__doc__")]

# XXX(alexandre): It would be possible to get the modules exports by fetching
# XXX: their __all__ attribute. However, I fear that this would add an additional
# XXX: overhead to the fixer.
MAPPING = {"StringIO":  ("io", ["StringIO"]),
           "cStringIO": ("io", ["StringIO"]),
           "__builtin__" : ("builtins", builtin_names),
           'copy_reg': ('copyreg', ['pickle',
                                    'constructor',
                                    'add_extension',
                                    'remove_extension',
                                    'clear_extension_cache']),
           'Queue': ('queue', ['Empty', 'Full', 'Queue',
                               'PriorityQueue', 'LifoQueue']),
           'SocketServer': ('socketserver',
                            ['TCPServer', 'UDPServer', 'BaseServer',
                             'ForkingUDPServer', 'ForkingTCPServer',
                             'ThreadingUDPServer', 'ThreadingTCPServer',
                             'BaseRequestHandler', 'StreamRequestHandler',
                             'DatagramRequestHandler', 'ThreadingMixIn',
                             'ForkingMixIn', 'UnixStreamServer',
                             'UnixDatagramServer', 'ThreadingUnixStreamServer',
                             'ThreadingUnixDatagramServer']),
           'ConfigParser': ('configparser',
                            ['NoSectionError', 'DuplicateSectionError',
                             'NoOptionError', 'InterpolationError',
                             'InterpolationDepthError',
                             'InterpolationSyntaxError',
                             'ParsingError', 'MissingSectionHeaderError',
                             'ConfigParser', 'SafeConfigParser',
                             'RawConfigParser', 'DEFAULTSECT',
                             'MAX_INTERPOLATION_DEPTH']),
           'repr': ('reprlib', ['Repr', 'repr']),
           'FileDialog': ('tkinter.filedialog',
                          ['FileDialog', 'LoadFileDialog', 'SaveFileDialog']),
           'tkFileDialog': ('tkinter.filedialog',
                            ['Open', 'SaveAs', 'Directory', 'askopenfilename',
                             'asksaveasfilename', 'askopenfilenames',
                             'askopenfile', 'askopenfiles', 'asksaveasfile',
                             'askdirectory']),
           'SimpleDialog': ('tkinter.simpledialog', ['SimpleDialog']),
           'tkSimpleDialog': ('tkinter.simpledialog',
                              ['Dialog', 'askinteger', 'askfloat',
                               'askstring']),
           'tkColorChooser': ('tkinter.colorchooser', ['Chooser', 'askcolor']),
           'tkCommonDialog': ('tkinter.commondialog', ['Dialog']),
           'Dialog': ('tkinter.dialog', ['Dialog']),
           'Tkdnd': ('tkinter.dnd', ['DndHandler']),
           'tkFont': ('tkinter.font',
                      ['nametofont', 'Font', 'families', 'names']),
           'tkMessageBox': ('tkinter.messagebox',
                            ['Message', 'showinfo', 'showwarning', 'showerror',
                             'askquestion', 'askokcancel', 'askyesno',
                             'askyesnocancel', 'askretrycancel']),
           'ScrolledText': ('tkinter.scrolledtext', ['ScrolledText']),
           'turtle': ('tkinter.turtle',
                      ['RawPen', 'Pen', 'Turtle', 'degrees', 'radian', 'reset',
                       'clear', 'tracer', 'forward', 'backward', 'left',
                       'right', 'up', 'down', 'width', 'color', 'write', 'fill',
                       'begin_fill', 'end_fill', 'circle', 'goto', 'heading',
                       'setheading', 'position', 'window_width', 'setx', 'sety',
                       'towards', 'done', 'delay', 'speed', 'setup', 'title']),
           'Tkconstants': ('tkinter.constants',
                           ['NO', 'FALSE', 'OFF', 'YES', 'TRUE', 'ON', 'N', 'S',
                            'W', 'E', 'NW', 'SW', 'SE', 'NE', 'NS', 'EW',
                            'NSEW', 'CENTER', 'NONE', 'X', 'Y', 'BOTH', 'LEFT',
                            'TOP', 'RIGHT', 'BOTTOM', 'RAISED', 'SUNKEN',
                            'FLAT', 'RIDGE', 'GROOVE', 'SOLID', 'HORIZONTAL',
                            'VERTICAL', 'NUMERIC', 'CHAR', 'WORD', 'BASELINE',
                            'INSIDE', 'OUTSIDE', 'SEL', 'SEL_FIRST', 'SEL_LAST',
                            'END', 'INSERT', 'CURRENT', 'ANCHOR', 'ALL',
                            'NORMAL', 'DISABLED', 'ACTIVE', 'HIDDEN', 'CASCADE',
                            'CHECKBUTTON', 'COMMAND', 'RADIOBUTTON',
                            'SEPARATOR', 'SINGLE', 'BROWSE', 'MULTIPLE',
                            'EXTENDED', 'DOTBOX', 'UNDERLINE', 'PIESLICE',
                            'CHORD', 'ARC', 'FIRST', 'LAST', 'BUTT',
                            'PROJECTING', 'ROUND', 'BEVEL', 'MITTER', 'MOVETO',
                            'SCROLL', 'UNITS', 'PAGES']),
           'Tix': ('tkinter.tix',
                   ['tixCommand', 'Tk', 'Form', 'TixWidget', 'TixSubWidget',
                    'DisplayStyle', 'Balloon', 'ButtonBox', 'ComboBox',
                    'Control', 'DirList', 'DirTree', 'DirSelectBox',
                    'ExFileSelectBox', 'DirSelectDialog', 'ExFileSelectDialog',
                    'FileSelectBox', 'FileSelectDialog', 'FileEntry', 'HList',
                    'InputOnly', 'LabelEntry', 'LabelFrame', 'ListNoteBook',
                    'Meter', 'NoteBook', 'OptionMenu', 'PanedWindow',
                    'PopupMenu', 'ResizeHandle', 'ScrolledHList',
                    'ScrolledListBox', 'ScrolledText', 'ScrolledTList',
                    'ScrolledWindow', 'Select', 'Shell', 'DialogShell',
                    'StdButtonBox', 'TList', 'Tree', 'CheckList', 'OptionName',
                    'FileTypeList', 'Grid', 'ScrolledGrid']),
           'Tkinter': ('tkinter',
                       ['_flatten', 'TclError', 'TkVersion', 'TclVersion',
                        'Variable', 'StringVar', 'IntVar', 'DoubleVar',
                        'BooleanVar','mainloop', 'Tk', 'Tcl', 'Toplevel',
                        'Button', 'Canvas', 'Checkbutton', 'Entry', 'Frame',
                        'Label', 'Listbox', 'Menu', 'Menubutton',
                        'Radiobutton', 'Scale', 'Scrollbar', 'Text',
                        'OptionMenu', 'Image', 'PhotoImage', 'BitmapImage',
                        'image_names', 'image_types', 'Spinbox', 'LabelFrame',
                        'PanedWindow', 'Studbutton', 'Tributton']),
           'markupbase': ('_markupbase', ['ParserBase']),
           '_winreg': ('winreg', [
               'CloseKey', 'ConnectRegistry', 'CreateKey', 'DeleteKey',
               'DeleteValue', 'DisableReflectionKey', 'EnableReflectionKey',
               'EnumKey', 'EnumValue', 'ExpandEnvironmentStrings', 'FlushKey',
               'LoadKey', 'OpenKey', 'OpenKeyEx', 'QueryValue', 'QueryValueEx',
               'QueryInfoKey', 'QueryReflectionKey', 'SaveKey', 'SetValue',
               'SetValueEx', 'HKEY_CLASSES_ROOT', 'HKEY_CURRENT_USER',
               'HKEY_LOCAL_MACHINE', 'HKEY_USERS', 'HKEY_PERFORMANCE_DATA',
               'HKEY_CURRENT_CONFIG', 'HKEY_DYN_DATA', 'KEY_QUERY_VALUE',
               'KEY_SET_VALUE', 'KEY_CREATE_SUB_KEY', 'KEY_ENUMERATE_SUB_KEYS',
               'KEY_NOTIFY', 'KEY_CREATE_LINK', 'KEY_READ', 'KEY_WRITE',
               'KEY_EXECUTE', 'KEY_ALL_ACCESS', 'KEY_WOW64_64KEY',
               'KEY_WOW64_32KEY', 'REG_OPTION_RESERVED',
               'REG_OPTION_NON_VOLATILE', 'REG_OPTION_VOLATILE',
               'REG_OPTION_CREATE_LINK', 'REG_OPTION_BACKUP_RESTORE',
               'REG_OPTION_OPEN_LINK', 'REG_LEGAL_OPTION',
               'REG_CREATED_NEW_KEY', 'REG_OPENED_EXISTING_KEY',
               'REG_WHOLE_HIVE_VOLATILE', 'REG_REFRESH_HIVE',
               'REG_NO_LAZY_FLUSH', 'REG_NOTIFY_CHANGE_NAME',
               'REG_NOTIFY_CHANGE_ATTRIBUTES', 'REG_NOTIFY_CHANGE_LAST_SET',
               'REG_NOTIFY_CHANGE_SECURITY', 'REG_LEGAL_CHANGE_FILTER',
               'REG_NONE', 'REG_SZ', 'REG_EXPAND_SZ', 'REG_BINARY', 'REG_DWORD',
               'REG_DWORD_LITTLE_ENDIAN', 'REG_DWORD_BIG_ENDIAN', 'REG_LINK',
               'REG_MULTI_SZ', 'REG_RESOURCE_LIST',
               'REG_FULL_RESOURCE_DESCRIPTOR', 'REG_RESOURCE_REQUIREMENTS_LIST']),
           'thread': ('_thread',
                      ['LockType', '_local', 'allocate', 'allocate_lock',
                       'error', 'exit', 'exit_thread', 'get_ident',
                       'interrupt_main', 'stack_size', 'start_new',
                       'start_new_thread']),
           'dummy_thread': ('_dummy_thread',
                      ['LockType', '_local', 'allocate', 'allocate_lock',
                       'error', 'exit', 'exit_thread', 'get_ident',
                       'interrupt_main', 'stack_size', 'start_new',
                       'start_new_thread']),
}


def alternates(members):
    return "(" + "|".join(map(repr, members)) + ")"


def build_pattern():
    bare = set()
    for old_module, (new_module, members) in MAPPING.items():
        bare.add(old_module)
        bare.update(members)
        members = alternates(members)
        yield """import_name< 'import' (module=%r
                              | dotted_as_names< any* module=%r any* >) >
              """ % (old_module, old_module)
        yield """import_from< 'from' module_name=%r 'import'
                   ( %s | import_as_name< %s 'as' any > |
                     import_as_names< any* >) >
              """ % (old_module, members, members)
        yield """import_from< 'from' module_name=%r 'import' star='*' >
              """ % old_module
        yield """import_name< 'import'
                              dotted_as_name< module_name=%r 'as' any > >
              """ % old_module
        yield """power< module_name=%r trailer< '.' %s > any* >
              """ % (old_module, members)
    yield """bare_name=%s""" % alternates(bare)


class FixImports(basefix.BaseFix):
    PATTERN = "|".join(build_pattern())

    order = "pre" # Pre-order tree traversal

    # Don't match the node if it's within another match
    def match(self, node):
        match = super(FixImports, self).match
        results = match(node)
        if results:
            if any([match(obj) for obj in attr_chain(node, "parent")]):
                return False
            return results
        return False

    def start_tree(self, tree, filename):
        super(FixImports, self).start_tree(tree, filename)
        self.replace = {}

    def transform(self, node, results):
        import_mod = results.get("module")
        mod_name = results.get("module_name")
        bare_name = results.get("bare_name")
        star = results.get("star")

        if import_mod or mod_name:
            new_name, members = MAPPING[(import_mod or mod_name).value]

        if import_mod:
            self.replace[import_mod.value] = new_name
            import_mod.replace(Name(new_name, prefix=import_mod.get_prefix()))
        elif mod_name:
            if star:
                self.cannot_convert(node, "Cannot handle star imports.")
            else:
                mod_name.replace(Name(new_name, prefix=mod_name.get_prefix()))
        elif bare_name:
            bare_name = bare_name[0]
            new_name = self.replace.get(bare_name.value)
            if new_name:
                bare_name.replace(Name(new_name, prefix=bare_name.get_prefix()))
