'''Mock classes that imitate idlelib modules or classes.

Attributes and methods will be added as needed for tests.
'''

from idlelib.idle_test.mock_tk import Text

class Editor:
    '''Minimally imitate EditorWindow.EditorWindow class.
    '''
    def __init__(self, flist=None, filename=None, key=None, root=None):
        self.text = Text()
        self.undo = UndoDelegator()

    def get_selection_indices(self):
        first = self.text.index('1.0')
        last = self.text.index('end')
        return first, last

class UndoDelegator:
    '''Minimally imitate UndoDelegator,UndoDelegator class.
    '''
    # A real undo block is only needed for user interaction.
    def undo_block_start(*args):
        pass
    def undo_block_stop(*args):
        pass
