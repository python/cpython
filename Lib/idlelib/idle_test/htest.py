'''Run a human test of Idle wndow, dialog, and other widget classes.

run(klass) runs a test for one class.
runall() runs all the defined tests

The file wih the widget class should end with
if __name__ == '__main__':
    <unittest, if there is one>
    from idlelib.idle_test.htest import run
    run(X)
where X is a global object of the module. X must be a callable with a
.__name__ attribute that accepts a 'parent' attribute. X will usually be
a widget class, but a callable instance with .__name__ or a wrapper
function also work. The name of wrapper functions, like _Editor_Window,
should start with '_'.

This file must then contain an instance of this template.
_spec = {
    'file': '',
    'kwds': {'title': ''},
    'msg': ""
    }
with X.__name__ prepended to _spec.
File (no .py) is used in runall() to import the file and get the class.
Kwds is passed to X (**kwds) after 'parent' is added, to initialize X.
Msg. displayed is a window with a start button. hint as to how the user
might test the widget. Closing The box skips or ends the test.
'''
from importlib import import_module
import tkinter as tk

# Template for class_spec dicts, copy and uncomment

_Editor_window_spec = {
    'file': 'EditorWindow',
    'kwds': {},
    'msg': "Test editor functions of interest"
    }

_Help_dialog_spec = {
    'file': 'EditorWindow',
    'kwds': {},
    'msg': "If the help text displays, this works"
    }

AboutDialog_spec = {
    'file': 'aboutDialog',
    'kwds': {'title': 'About test'},
    'msg': "Try each button"
    }


GetCfgSectionNameDialog_spec = {
    'file': 'configSectionNameDialog',
    'kwds': {'title':'Get Name',
                 'message':'Enter something',
                 'used_names': {'abc'},
                 '_htest': True},
    'msg': "After the text entered with [Ok] is stripped, <nothing>, "
              "'abc', or more that 30 chars are errors.\n"
              "Close 'Get Name' with a valid entry (printed to Shell), [Cancel], or [X]",
    }

def run(klas):
    "Test the widget class klas using _spec dict"
    root = tk.Tk()
    klas_spec = globals()[klas.__name__+'_spec']
    klas_kwds = klas_spec['kwds']
    klas_kwds['parent'] = root
    # This presumes that Idle consistently uses 'parent'
    def run_klas():
        widget = klas(**klas_kwds)
        try:
            print(widget.result)
        except AttributeError:
            pass
    tk.Label(root, text=klas_spec['msg'], justify='left').pack()
    tk.Button(root, text='Test ' + klas.__name__, command=run_klas).pack()
    root.mainloop()

def runall():
    'Run all tests. Quick and dirty version.'
    for k, d in globals().items():
        if k.endswith('_spec'):
            mod = import_module('idlelib.' + d['file'])
            klas = getattr(mod, k[:-5])
            run(klas)

if __name__ == '__main__':
    runall()
