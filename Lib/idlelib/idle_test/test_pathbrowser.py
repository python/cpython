import unittest
import idlelib.PathBrowser as PathBrowser

class PathBrowserTest(unittest.TestCase):

    def test_DirBrowserTreeItem(self):
        # Issue16226 - make sure that getting a sublist works
        d = PathBrowser.DirBrowserTreeItem('')
        d.GetSubList()

if __name__ == '__main__':
    unittest.main(verbosity=2, exit=False)
