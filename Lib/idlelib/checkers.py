from functools import partial
import os
import sys
import traceback
from subprocess import Popen
from tempfile import NamedTemporaryFile

from tkinter import END, RIGHT
from tkinter import messagebox

from idlelib.config import idleConf


CHECKER_PARAMS = (
    # (name, type, default, warn_on_default)
    ('enabled', 'bool', True, False),
    ('name', 'str', None, True),
    ('command', 'str', None, True),
    ('additional', 'str', '', False),
    ('reload_source', 'bool', False, False),
    ('show_result', 'bool', True, False),
)


def get_checkers():
    """
    Return list of checkers defined in users config file.
    """
    return sorted(
        set(idleConf.GetSectionList('default', 'checkers')) |
        set(idleConf.GetSectionList('user', 'checkers'))
    )


def get_enabled_checkers():
    """
    Return list of currently enabled checkers.
    """
    return [
        c for c in get_checkers()
        if idleConf.GetOption(
            'checkers', c, 'enabled', default=False, type='bool')
    ]


def get_checker_config(checker_name):
    """
    Return configuration of checker as dict.
    """
    if checker_name == '':
        raise ValueError('Checker name cannot be empty')

    checker_params = {
        name: idleConf.GetOption('checkers', checker_name,
                                 name, type=type_, default=default,
                                 warn_on_default=warn_on_default)
        for name, type_, default, warn_on_default
        in CHECKER_PARAMS
        if name != 'name'
    }
    checker_params['name'] = checker_name
    return checker_params


def run_checker(editwin, checker):
    """
    Run the 3rd party checker 'checker'.

    If 'show_result' for the checker is configured to True, display
    its output in a new CheckerWindow.

    Return True if successful, False otherwise.
    """
    filename = editwin.scriptbinding.getfilename()
    if not filename:
        return False

    config = get_checker_config(checker)
    command = config['command']
    if not command:
        location = 'Config Checkers'
        menu = 'Run'
        message = ("'Command' option for '{checker}' checker is empty.\n"
                   "Please update the 'Command' option for '{checker}' "
                   "in {location} in {menu} menu, before running "
                   "'{checker}' again.".format(checker=checker,
                                               location=location,
                                               menu=menu))
        messagebox.showerror(title="Empty Command", message=message,
                             parent=editwin.top)
        return False
    additional = config['additional']
    reload_source = config['reload_source']
    show_result = config['show_result']

    dirname, _file = os.path.split(filename)
    args = [
        os.path.expanduser(arg)
        for arg in command.split() + additional.split() + [_file]
        if arg
    ]

    # CheckerWindow closes file if no exception
    with NamedTemporaryFile(delete=False) as error_file, \
            NamedTemporaryFile(delete=False) as output_file:
        try:
            process = Popen(args, stdout=output_file, stderr=error_file,
                            cwd=dirname)
            if show_result:
                if CheckerWindow is None:
                    define_CheckerWindow()
                CheckerWindow(editwin, filename, checker, output_file,
                              error_file, process, reload_source)
            elif reload_source:
                while process.poll() is None:
                    continue
                editwin.io.loadfile(filename)
            return True
        except Exception:
            *_, value, tb = sys.exc_info()
            message = ("File: {}\n"
                       "Checker: {}\n"
                       "Command: {}\n"
                       "Additional Args: {}\n"
                       "Call details: {}\n\n\n"
                       "Traceback: {}".format(filename, checker, command,
                                              additional, ' '.join(args),
                                              traceback.format_exc(limit=1)
                                              ))
            messagebox.showerror(title=value,
                                 message=message,
                                 parent=editwin.top)
        return False


class Checkers:
    checker_names = tuple(get_enabled_checkers())
    editwin_instance_dict = None

    def __init__(self, editwin):
        self.editwin = editwin
        self.parent = self.editwin.top
        self.set_editwin_instance_dict(self.parent.instance_dict)
        self.update_checkers_menu(self.editwin)

    @classmethod
    def set_editwin_instance_dict(cls, editwin_instance_dict):
        cls.editwin_instance_dict = editwin_instance_dict

    @classmethod
    def reload(cls):
        """Load class variables from config."""
        cls.checker_names = tuple(get_enabled_checkers())
        for editwin in cls.editwin_instance_dict:
            cls.update_checkers_menu(editwin)

    @classmethod
    def update_checkers_menu(cls, editwin):
        """
        Utility method to update the Run menu to display the
        currently enabled checkers.
        """
        if not hasattr(editwin, 'checkers_menu'):  # a shell window
            return
        menu = editwin.checkers_menu
        menu.delete(0, END)
        for checker_name in cls.checker_names:
            menu.add_command(
                label='Run {}'.format(checker_name),
                command=partial(run_checker, editwin, checker_name),
            )


# class ConfigCheckerDialog(tk.Toplevel):
#     def __init__(self, _checker, checker_name=None):
#         tk.Toplevel.__init__(self, _checker.editwin.top)
#         self._checker = _checker
#         self.editwin = _checker.editwin
#         self.checker_name = checker_name
#         self.parent = parent = self.editwin.top
#
#         self.name = tk.StringVar(parent)
#         self.command = tk.StringVar(parent)
#         self.additional = tk.StringVar(parent)
#         self.reload_source = tk.StringVar(parent)
#         self.show_result = tk.StringVar(parent)
#         self.enabled = tk.StringVar(parent)
#         self.call_string = tk.StringVar(parent)
#         self.command.trace('w', self.update_call_string)
#         self.additional.trace('w', self.update_call_string)
#
#         if checker_name:
#             config = get_checker_config(checker_name)
#         self.name.set(config['name'] if checker_name else '')
#         self.command.set(config['command'] if checker_name else '')
#         self.additional.set(config['additional'] if checker_name else '')
#         self.reload_source.set(config['reload_source'] if checker_name else 0)
#         self.show_result.set(config['show_result'] if checker_name else 1)
#         self.enabled.set(config['enabled'] if checker_name else 1)
#
#         self.grab_set()
#         self.resizable(width=False, height=False)
#         self.create_widgets()
#
#     def create_widgets(self):
#         parent = self.parent
#         self.configure(borderwidth=5)
#         if self.checker_name:
#             title = 'Edit {} checker'.format(self.checker_name)
#         else:
#             title = 'Config new checker'
#         self.wm_title(title)
#         self.geometry('+%d+%d' % (parent.winfo_rootx() + 30,
#                                   parent.winfo_rooty() + 30))
#         self.transient(parent)
#         self.focus_set()
#         # frames creation
#         optionsFrame = tk.Frame(self)
#         buttonFrame = tk.Frame(self)
#
#         # optionsFrame
#         nameLabel = tk.Label(optionsFrame, text='Name of Checker')
#         commandLabel = tk.Label(optionsFrame, text='Command')
#         additionalLabel = tk.Label(optionsFrame, text='Additional Args')
#         currentCallStringLabel = tk.Label(optionsFrame,
#                                           text='Call Command string')
#
#         self.nameEntry = tk.Entry(optionsFrame, textvariable=self.name,
#                                   width=40)
#         self.commandEntry = tk.Entry(optionsFrame, textvariable=self.command,
#                                      width=40)
#         self.additionalEntry = tk.Entry(optionsFrame,
#                                         textvariable=self.additional, width=40)
#         reload_sourceCheckbutton = tk.Checkbutton(optionsFrame,
#                                                   variable=self.reload_source,
#                                                   text='Reload file?')
#         showResultCheckbutton = tk.Checkbutton(optionsFrame,
#                                                variable=self.show_result,
#                                                text='Show result after run?')
#         self.currentCallStringEntry = tk.Entry(optionsFrame, state='readonly',
#                                                textvariable=self.call_string,
#                                                width=40)
#         enabledCheckbutton = tk.Checkbutton(optionsFrame,
#                                             variable=self.enabled,
#                                             text='Enable Checker?')
#
#         # buttonFrame
#         okButton = tk.Button(buttonFrame, text='Ok', command=self.ok)
#         cancelButton = tk.Button(buttonFrame, text='Cancel',
#                                  command=self.cancel)
#
#         # frames packing
#         optionsFrame.pack()
#         buttonFrame.pack()
#         # optionsFrame packing
#         nameLabel.pack()
#         self.nameEntry.pack()
#         commandLabel.pack()
#         self.commandEntry.pack()
#         additionalLabel.pack()
#         self.additionalEntry.pack()
#         reload_sourceCheckbutton.pack()
#         showResultCheckbutton.pack()
#         currentCallStringLabel.pack()
#         self.currentCallStringEntry.pack()
#         enabledCheckbutton.pack()
#         # buttonFrame packing
#         okButton.pack(side=tk.LEFT)
#         cancelButton.pack()
#
#     def update_call_string(self, *args, **kwargs):
#         filename = self.editwin.io.filename or ''
#         call_string = ' '.join([self.command.get(), self.additional.get(),
#                                 os.path.split(filename)[1] or '<filename>'])
#         self.call_string.set(call_string)
#
#     def name_ok(self):
#         name = self.name.get()
#         ok = True
#         if name.strip() == '':
#             messagebox.showerror(title='Name Error',
#                                  message='No Name Specified', parent=self)
#             ok = False
#         return ok
#
#     def command_ok(self):
#         command = self.command.get()
#         ok = True
#         if command.strip() == '':
#             message = ('No command specified. \nCommand is name or full path'
#                        ' to the program that IDLE has to execute')
#             messagebox.showerror(title='Command Error',
#                                  message=message, parent=self)
#             ok = False
#         return ok
#
#     def additional_ok(self):
#         ok = True
#         return ok
#
#     def ok(self):
#         _ok = self.name_ok() and self.command_ok() and self.additional_ok()
#         if _ok:
#             name = self.name.get()
#             idleConf.userCfg['checkers'].SetOption(name, 'enabled',
#                                                   self.enabled.get())
#             idleConf.userCfg['checkers'].SetOption(name, 'command',
#                                                   self.command.get())
#             idleConf.userCfg['checkers'].SetOption(name, 'additional',
#                                                   self.additional.get())
#             idleConf.userCfg['checkers'].SetOption(name, 'reload_source',
#                                                   self.reload_source.get()),
#             idleConf.userCfg['checkers'].SetOption(name, 'show_result',
#                                                   self.show_result.get())
#
#             idleConf.userCfg['checkers'].Save()
#             self.close()
#
#     def cancel(self):
#         self.close()
#
#     def close(self):
#         self._checker.update_listbox()
#         self._checker.update_menu()
#         self._checker.dialog.grab_set()
#         self.destroy()


CheckerWindow = None

def define_CheckerWindow():
    from idlelib.outwin import OutputWindow

    global CheckerWindow

    class CheckerWindow(OutputWindow):
        def __init__(self, editwin, filename, checker, output_file, error_file,
                     process, reload_source):
            """
            editwin - EditorWindow instance
            filename - string
            checker - name of checker
            output_file, error_file - Temporary file objects
            process - Popen object
            reload_source - bool, reload original file after checker finished
            """
            self.editwin = editwin
            self.filename = filename
            self.checker = checker
            self.process = process
            self.reload_source = reload_source
            OutputWindow.__init__(self, self.editwin.flist)
            self.error_file = open(error_file.name, 'r')
            self.output_file = open(output_file.name, 'r')

            theme = idleConf.CurrentTheme()
            stderr_fg = idleConf.GetHighlight(theme, 'stderr', fgBg='fg')
            stdout_fg = idleConf.GetHighlight(theme, 'stdout', fgBg='fg')
            tagdefs = {'stderr': {'foreground': stderr_fg},
                       'stdout': {'foreground': stdout_fg}, }
            for tag, cnf in tagdefs.items():
                self.text.tag_configure(tag, **cnf)

            self.text.mark_set('stderr_index', '1.0')
            self.text.mark_set('stdout_index', END)
            self.stderr_index = self.text.index('stderr_index')
            self.stdout_index = self.text.index('stdout_index')
            self.status_bar.set_label('Checker status', 'Processing..!',
                                      side=RIGHT)
            self.update()

        def short_title(self):
            return '{} - {}'.format(self.checker, self.filename)

        def update(self):
            self.update_error()
            self.update_output()
            if self.process.poll() is None:
                self.text.after(10, self.update)
            else:
                if self.reload_source:
                    self.editwin.io.loadfile(self.filename)
                self.update_error()
                self.update_output()
                self.status_bar.set_label('Checker status', 'Done', side=RIGHT)
                self._clean_tempfiles()

        def update_error(self):
            for line in self.error_file.read().splitlines(True):
                self.text.insert(self.stderr_index, line, 'stderr')
                self.stderr_index = self.text.index('stderr_index')

        def update_output(self):
            for line in self.output_file.read().splitlines(True):
                self.text.insert(self.stdout_index, line, 'stdout')
                self.stdout_index = self.text.index('stdout_index')

        file_line_pats = [r':\s*(\d+)\s*(:|,)']

        def _file_line_helper(self, line):
            for prog in self.file_line_progs:
                match = prog.search(line)
                if match:
                    lineno = match.group(1)
                else:
                    return None
                try:
                    return self.filename, int(lineno)
                except TypeError:
                    return None

        def _clean_tempfiles(self):
            self.output_file.close()
            self.error_file.close()
            try:
                os.unlink(self.error_file.name)
                os.unlink(self.output_file.name)
            except OSError:
                pass

        def close(self):
            self._clean_tempfiles()
            OutputWindow.close(self)
