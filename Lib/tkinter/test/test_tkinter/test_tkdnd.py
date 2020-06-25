import tempfile
import tkinter as tk
import unittest
from tkinter.test.support import AbstractTkTest

class BaseDndTest(AbstractTkTest, unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        sup = super(BaseDndTest, cls)
        sup.setUpClass()
        if not cls.root.dnd_loaded():
            sup.skipTest(cls, "TkDND installation not found")

class DropTargetTest(BaseDndTest):

    def test_register(self):
        self.root.drop_target_register()
        self.root.drop_target_register(tk.DND_ALL)
        self.root.drop_target_register((tk.DND_TEXT, tk.DND_FILES))

    def test_unregister(self):
        self.root.drop_target_unregister()

    def test_bind(self):
        # Being virtual events, they should not raise an exception
        # even if undefined (unlike a regular event)
        self.root.bind("<<Drop>>", lambda event: event.action)
        self.root.event_generate("<<Drop>>")
        self.root.bind(tk.DND_FILES_DROP, lambda event: event.action)
        self.root.bind("<<DropEnter>>", lambda event: event.action)
        self.root.bind("<<DropPosition>>", lambda event: event.action)
        self.root.bind("<<DropLeave>>", lambda event: event.action)

    def test_unbind(self):
        def fail_test(event):
            nonlocal failed
            failed = True
        self.root.bind("<<Drop>>", fail_test)
        self.root.unbind("<<Drop>>")
        failed = False
        self.root.event_generate("<<Drop>>")
        if failed:
            raise RuntimeError()

class DragSourceTest(BaseDndTest):

    def test_register(self):
        self.root.drag_source_register()
        self.root.drag_source_register(tk.DND_ALL, 3)
        self.root.drag_source_register((tk.DND_TEXT, tk.DND_FILES), (1, 3))

    def test_unregister(self):
        self.root.drag_source_unregister()

    def test_bind(self):
        self.root.bind("<<DragInitCmd>>", lambda event: ((tk.COPY, tk.MOVE),
                                                         tk.DND_TEXT,
                                                         'Hello from Tk!'))
        self.root.event_generate("<<DragInitCmd>>")
        self.root.bind("<<DragEndCmd>>", lambda event: event.action)

    def test_unbind(self):
        def fail_test(event):
            nonlocal failed
            failed = True
        self.root.bind("<<DragInitCmd>>", fail_test)
        self.root.unbind("<<DragInitCmd>>")
        failed = False
        self.root.event_generate("<<DragInitCmd>>")
        if failed:
            raise RuntimeError()

class MiscTest(BaseDndTest):

    def test_loaded(self):
        self.assertEqual(self.root.load_dnd(), True)

    def test_specific(self):
        self.root.platform_specific_types(tk.DND_ALL)
        self.root.platform_specific_types(tk.DND_TEXT)
        self.root.platform_specific_types(tk.DND_FILES)
        for tps in ((tk.CF_UNICODETEXT, tk.CF_TEXT),  # Windows
                    (tk.TEXT_PLAIN_UNICODE, tk.TEXT_PLAIN),  # Unix
                    (tk.NSSTRINGPBOARDTYPE,)):  # MacOS
            self.assertEqual(self.root.platform_specific_types(tps), tps)

    def test_independent(self):
        try:
            self.root.platform_independent_types(tk.DND_ALL)
        except tk.TclError:
            # tkdnd has a bug which causes this method to fail in many versions
            self.skipTest("Possible TkDND issue ignored")
        # If the previous call succeeded, so should these
        self.assertEqual(self.root.platform_independent_types(tk.DND_TEXT),
                         (tk.DND_TEXT,))
        self.assertEqual(self.root.platform_independent_types(tk.DND_FILES),
                         (tk.DND_FILES,))
        self.assertTrue(tk.DND_TEXT in \
            self.root.platform_independent_types((tk.CF_TEXT,
                                                  tk.TEXT_PLAIN,
                                                  tk.NSSTRINGPBOARDTYPE)))
        self.assertTrue(tk.DND_TEXT in \
            self.root.platform_independent_types((tk.CF_UNICODETEXT,
                                                  tk.TEXT_PLAIN_UNICODE,
                                                  tk.NSSTRINGPBOARDTYPE)))
        self.assertTrue(tk.DND_FILES in \
            self.root.platform_independent_types((tk.CF_HDROP,
                                                  tk.URI_LIST,
                                                  tk.NSFILENAMESPBOARDTYPE)))
        self.assertTrue(tk.DND_HTML in \
            self.root.platform_independent_types((tk.CF_HTML,
                                                  tk.TEXT_HTML,
                                                  tk.NSPASTEBOARDTYPEHTML)))
        self.assertTrue(tk.DND_HTML in \
            self.root.platform_independent_types((tk.HTML_FORMAT,
                                                  tk.TEXT_HTML_UNICODE,
                                                  tk.NSPASTEBOARDTYPEHTML)))

    def test_tempdir_get(self):
        self.root.get_drop_file_temp_directory()

    def test_tempdir_set(self):
        with tempfile.TemporaryDirectory() as di:
            self.root.set_drop_file_temp_directory(di)
            self.assertEqual(self.root.get_drop_file_temp_directory(), di)

tests_gui = (
        DropTargetTest,
        DragSourceTest,
        MiscTest
)

if __name__ == "__main__":
    unittest.main()
