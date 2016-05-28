'''Mock classes that imitate idlelib modules or classes.

Attributes and methods will be added as needed for tests.
'''

from idlelib.idle_test.mock_tk import Text

class Func:
    '''Mock function captures args and returns result set by test.

    Attributes:
    self.called - records call even if no args, kwds passed.
    self.result - set by init, returned by call.
    self.args - captures positional arguments.
    self.kwds - captures keyword arguments.

    Most common use will probably be to mock methods.
    Mock_tk.Var and Mbox_func are special variants of this.
    '''
    def __init__(self, result=None):
        self.called = False
        self.result = result
        self.args = None
        self.kwds = None
    def __call__(self, *args, **kwds):
        self.called = True
        self.args = args
        self.kwds = kwds
        if isinstance(self.result, BaseException):
            raise self.result
        else:
            return self.result


class Editor:
    '''Minimally imitate editor.EditorWindow class.
    '''
    def __init__(self, flist=None, filename=None, key=None, root=None):
        self.text = Text()
        self.undo = UndoDelegator()

    def get_selection_indices(self):
        first = self.text.index('1.0')
        last = self.text.index('end')
        return first, last


class UndoDelegator:
    '''Minimally imitate undo.UndoDelegator class.
    '''
    # A real undo block is only needed for user interaction.
    def undo_block_start(*args):
        pass
    def undo_block_stop(*args):
        pass
