import unittest
import tkinter
from tkinter.scrolledtext import ScrolledText
from test.support import requires
from test.test_tkinter.support import setUpModule  # noqa: F401
from test.test_tkinter.support import AbstractTkTest

requires('gui')


class ScrolledTextTest(AbstractTkTest, unittest.TestCase):

    def create(self, **kwargs):
        st = ScrolledText(self.root, **kwargs)
        self.addCleanup(st.destroy)
        return st

    def test_create(self):
        st = self.create(background='red', height=5)
        # It is a Text widget held in a Frame together with a Scrollbar.
        self.assertIsInstance(st, tkinter.Text)
        self.assertIsInstance(st.frame, tkinter.Frame)
        self.assertIsInstance(st.vbar, tkinter.Scrollbar)
        self.assertEqual(st.winfo_parent(), str(st.frame))
        # str() returns the frame, so that geometry managers manage it.
        self.assertEqual(str(st), str(st.frame))
        # Keyword options configure the Text.
        self.assertEqual(str(st['background']), 'red')
        self.assertEqual(st['height'], 5 if self.wantobjects else '5')

    def test_text_methods(self):
        st = self.create()
        st.insert('1.0', 'hello\nworld')
        self.assertEqual(st.get('1.0', 'end-1c'), 'hello\nworld')
        self.assertEqual(st.index('end-1c'), '2.5')
        st.delete('1.0', 'end')
        self.assertEqual(st.get('1.0', 'end-1c'), '')

    def test_geometry_methods(self):
        st = self.create()
        # configure is not redirected; it configures the Text.
        st.configure(height=8)
        self.assertEqual(st['height'], 8 if self.wantobjects else '8')
        # Pack, Grid and Place methods are redirected to the frame.
        st.pack()
        self.root.update()
        self.assertEqual(st.frame.winfo_manager(), 'pack')
        self.assertEqual(st.pack_info(), st.frame.pack_info())
        st.pack_forget()
        self.assertEqual(st.frame.winfo_manager(), '')

    def test_scrollbar(self):
        st = self.create(height=5)
        st.pack()
        st.insert('1.0', '\n'.join(map(str, range(100))))
        self.root.update()
        # The scrollbar tracks the text view.
        self.assertEqual(st.vbar.get(), st.yview())
        st.yview_moveto(1.0)
        self.root.update()
        self.assertEqual(st.vbar.get()[1], 1.0)


if __name__ == "__main__":
    unittest.main()
