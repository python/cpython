'''Run human tests of Idle's window, dialog, and popup widgets.

run(test): run *test*, a callable that causes a widget to be displayed.
runall(): run all tests defined in this file.

Let X be a global name bound to a widget callable. End the module with

if __name__ == '__main__':
    <unittest, if there is one>
    from idlelib.idle_test.htest import run
    run(X)

The X object must have a .__name__ attribute and a 'parent' parameter.
X will often be a widget class, but a callable instance with .__name__
or a wrapper function also work. The name of wrapper functions, like
'_Editor_Window', should start with '_'.

This file must contain a matching instance of the folling template,
with X.__name__ prepended, as in '_Editor_window_spec ...'.

_spec = {
    'file': '',
    'kwds': {'title': ''},
    'msg': ""
    }

file (no .py): used in runall() to import the file and get X.
kwds: passed to X (**kwds), after 'parent' is added, to initialize X.
title: an example; used for some widgets, delete if not.
msg: displayed in a master window. Hints as to how the user might
  test the widget. Close the window to skip or end the test.
'''
from importlib import import_module
import tkinter as tk


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

def run(test):
    "Display a widget with callable *test* using a _spec dict"
    root = tk.Tk()
    test_spec = globals()[test.__name__ + '_spec']
    test_kwds = test_spec['kwds']
    test_kwds['parent'] = root

    def run_test():
        widget = test(**test_kwds)
        try:
            print(widget.result)
        except AttributeError:
            pass
    tk.Label(root, text=test_spec['msg'], justify='left').pack()
    tk.Button(root, text='Test ' + test.__name__, command=run_test).pack()
    root.mainloop()

def runall():
    "Run all tests. Quick and dirty version."
    for k, d in globals().items():
        if k.endswith('_spec'):
            mod = import_module('idlelib.' + d['file'])
            test = getattr(mod, k[:-5])
            run(test)

if __name__ == '__main__':
    runall()
