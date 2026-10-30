"Test tree, coverage 95%."

from idlelib import tree
import tkinter.ttk
import unittest
from types import SimpleNamespace
from test.support import requires
requires('gui')
from tkinter import Tk


class Item(tree.TreeItem):
    "A tree item with fixed children, counting the calls that matter."

    def __init__(self, text, children=(), label=None):
        self.text = text
        self.children = children
        self.label = label
        self.sublists = 0    # Number of GetSubList calls.
        self.clicks = 0      # Number of OnDoubleClick calls.

    def GetText(self):
        return self.text

    def GetLabelText(self):
        return self.label

    def IsExpandable(self):
        return bool(self.children)

    def GetSubList(self):
        self.sublists += 1
        return list(self.children)

    def OnDoubleClick(self):
        self.clicks += 1


def tree_of(*texts):
    "Return an item with a child for each text."
    return Item('root', [Item(text) for text in texts])


class ScrolledTreeviewTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.root = Tk()
        cls.root.withdraw()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root

    def widget(self, **kwargs):
        "Return a ScrolledTreeview destroyed at the end of the test."
        widget = tree.ScrolledTreeview(self.root, **kwargs)
        self.addCleanup(widget.destroy)
        return widget

    def test_add_row(self):
        widget = self.widget(columns=('value',))
        iid = widget.add_row(text='spam', values=('eggs',))
        self.assertEqual(widget.tree.item(iid, 'text'), 'spam')
        self.assertEqual(widget.tree.set(iid, 'value'), 'eggs')
        self.assertEqual(widget.tree.get_children(), (iid,))

    def test_clear(self):
        widget = self.widget()
        widget.add_row(text='spam and eggs')
        widget.clear()
        self.assertEqual(widget.tree.get_children(), ())

    def test_headings_columns(self):
        widget = self.widget(columns=('name', 'value'), show='headings')
        self.assertFalse(widget.tree_column)
        iid = widget.add_row(values=('spam', 'eggs'))
        self.assertEqual(widget.tree.set(iid, 'name'), 'spam')
        self.assertEqual(widget.tree.set(iid, 'value'), 'eggs')

    def test_headings(self):
        widget = self.widget(columns=('value',), headings=('Name', 'Value'))
        # Tk 9.1 gives index objects, not strings, for the show option.
        self.assertEqual([str(what) for what in widget.tree['show']],
                         ['tree', 'headings'])
        self.assertEqual(widget.tree.heading('#0', 'text'), 'Name')
        self.assertEqual(widget.tree.heading('#1', 'text'), 'Value')


class TreeWidgetTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.root = Tk()
        cls.root.withdraw()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root

    def widget(self, item=None, **kwargs):
        "Return a TreeWidget destroyed at the end of the test."
        widget = tree.TreeWidget(self.root, item, **kwargs)
        self.addCleanup(widget.destroy)
        return widget

    def texts(self, widget, iid=''):
        "Return the texts of the rows under iid."
        return [widget.tree.item(child, 'text')
                for child in widget.tree.get_children(iid)]

    def test_empty(self):
        widget = self.widget()
        self.assertEqual(widget.root, '')
        self.assertEqual(widget.items, {})
        self.assertEqual(self.texts(widget), [])

    def test_root_item(self):
        item = Item('spam')
        widget = self.widget(item)
        self.assertEqual(self.texts(widget), ['spam'])
        self.assertIs(widget.items[widget.root], item)

    def test_set_root_replaces_tree(self):
        widget = self.widget(tree_of('spam'))
        widget.expand()
        root = widget.set_root(Item('eggs'))
        self.assertEqual(self.texts(widget), ['eggs'])
        self.assertEqual(list(widget.items), [root])

    def test_leaf_has_no_children(self):
        widget = self.widget(Item('spam'))
        self.assertEqual(widget.tree.get_children(widget.root), ())

    def test_expandable_item_has_placeholder(self):
        widget = self.widget(tree_of('spam'))
        children = widget.tree.get_children(widget.root)
        self.assertEqual(len(children), 1)
        # The placeholder only shows the indicator; it has no item.
        self.assertNotIn(children[0], widget.items)

    def test_expand_fills_in_children(self):
        item = tree_of('spam', 'eggs')
        widget = self.widget(item)
        self.assertEqual(item.sublists, 0)  # Not expanded, not asked.
        widget.expand()
        self.assertEqual(item.sublists, 1)
        self.assertEqual(self.texts(widget, widget.root), ['spam', 'eggs'])
        self.assertTrue(widget.tree.item(widget.root, 'open'))
        widget.expand()  # Filled in only once.
        self.assertEqual(item.sublists, 1)

    def test_expand_row(self):
        item = tree_of('spam')
        widget = self.widget(Item('root', [item]))
        widget.expand()
        child = widget.tree.get_children(widget.root)[0]
        widget.expand(child)
        self.assertEqual(self.texts(widget, child), ['spam'])

    def test_expand_without_root(self):
        widget = self.widget()
        widget.expand()  # No root row, nothing to expand.
        self.assertEqual(self.texts(widget), [])

    def test_open_fills_in_children(self):
        item = tree_of('spam')
        widget = self.widget(item)
        widget.tree.focus(widget.root)
        widget.opened()  # As <<TreeviewOpen>> does.
        self.assertEqual(self.texts(widget, widget.root), ['spam'])

    def test_expand_item_without_children(self):
        # IsExpandable may be mistaken; the row becomes a leaf.
        item = Item('spam')
        item.IsExpandable = lambda: True
        widget = self.widget(item)
        widget.expand()
        self.assertEqual(widget.tree.get_children(widget.root), ())

    def test_values_columns(self):
        widget = self.widget(Item('42', label='x ='), columns=('value',))
        self.assertEqual(self.texts(widget), ['x ='])
        self.assertEqual(widget.tree.set(widget.root, 'value'), '42')

    def test_values_column_without_label(self):
        widget = self.widget(Item('spam'), columns=('value',))
        self.assertEqual(self.texts(widget), ['spam'])
        self.assertEqual(widget.tree.set(widget.root, 'value'), '')

    def test_label_without_values_column(self):
        # The label goes in front of the text, as it did on the canvas.
        widget = self.widget(Item('42', label='x ='))
        self.assertEqual(self.texts(widget), ['x = 42'])

    def test_item_without_text(self):
        widget = self.widget(tree.TreeItem())
        self.assertEqual(self.texts(widget), [''])

    def test_activated(self):
        item = Item('spam')
        widget = self.widget(item)
        widget.tree.focus(widget.root)
        widget.activated()
        self.assertEqual(item.clicks, 1)

    def test_activated_without_row(self):
        widget = self.widget(tree_of('spam'))
        widget.tree.focus('')
        widget.activated()  # No current row, nothing to activate.

    def test_double_clicked(self):
        item = Item('spam')
        widget = self.widget(item)
        root = widget.root
        widget.tree.identify_row = lambda y: root if y else ''
        widget.double_clicked(SimpleNamespace(y=10))
        self.assertEqual(item.clicks, 1)
        widget.double_clicked(SimpleNamespace(y=0))  # Below the rows.
        self.assertEqual(item.clicks, 1)

    def test_configure_style(self):
        # The row height follows the font, which ttk would leave at 20.
        widget = self.widget(tree_of('spam'))
        style = tkinter.ttk.Style(widget)
        height = style.lookup(tree.STYLE, 'rowheight')
        self.assertGreaterEqual(int(height),
                                widget.font.metrics('linespace'))


if __name__ == '__main__':
    unittest.main(verbosity=2)
