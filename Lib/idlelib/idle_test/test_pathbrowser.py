import unittest
import os
import sys
import idlelib
from idlelib import PathBrowser

class PathBrowserTest(unittest.TestCase):

    def test_DirBrowserTreeItem(self):
        # Issue16226 - make sure that getting a sublist works
        d = PathBrowser.DirBrowserTreeItem('')
        d.GetSubList()
        self.assertEqual('', d.GetText())

        dir = os.path.split(os.path.abspath(idlelib.__file__))[0]
        self.assertEqual(d.ispackagedir(dir), True)
        self.assertEqual(d.ispackagedir(dir + '/Icons'), False)

    def test_PathBrowserTreeItem(self):
        p = PathBrowser.PathBrowserTreeItem()
        self.assertEqual(p.GetText(), 'sys.path')
        sub = p.GetSubList()
        self.assertEqual(len(sub), len(sys.path))
        # Following fails in 2.7 because old-style class
        #self.assertEqual(type(sub[0]), PathBrowser.DirBrowserTreeItem)

if __name__ == '__main__':
    unittest.main(verbosity=2, exit=False)
