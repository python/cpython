import unittest
import tkinter
from test import support
from tkinter.test.support import AbstractTkTest, AbstractDefaultRootTest, requires_tcl

support.requires('gui')


class MiscTest(AbstractTkTest, unittest.TestCase):

    def test_image_types(self):
        image_types = self.root.image_types()
        self.assertIsInstance(image_types, tuple)
        self.assertIn('photo', image_types)
        self.assertIn('bitmap', image_types)

    def test_image_names(self):
        image_names = self.root.image_names()
        self.assertIsInstance(image_names, tuple)


class DefaultRootTest(AbstractDefaultRootTest, unittest.TestCase):

    def test_image_types(self):
        self.assertRaises(RuntimeError, tkinter.image_types)
        root = tkinter.Tk()
        image_types = tkinter.image_types()
        self.assertIsInstance(image_types, tuple)
        self.assertIn('photo', image_types)
        self.assertIn('bitmap', image_types)
        root.destroy()
        tkinter.NoDefaultRoot()
        self.assertRaises(RuntimeError, tkinter.image_types)

    def test_image_names(self):
        self.assertRaises(RuntimeError, tkinter.image_names)
        root = tkinter.Tk()
        image_names = tkinter.image_names()
        self.assertIsInstance(image_names, tuple)
        root.destroy()
        tkinter.NoDefaultRoot()
        self.assertRaises(RuntimeError, tkinter.image_names)

    def test_image_create_bitmap(self):
        self.assertRaises(RuntimeError, tkinter.BitmapImage)
        root = tkinter.Tk()
        image = tkinter.BitmapImage()
        self.assertIn(image.name, tkinter.image_names())
        root.destroy()
        tkinter.NoDefaultRoot()
        self.assertRaises(RuntimeError, tkinter.BitmapImage)

    def test_image_create_photo(self):
        self.assertRaises(RuntimeError, tkinter.PhotoImage)
        root = tkinter.Tk()
        image = tkinter.PhotoImage()
        self.assertIn(image.name, tkinter.image_names())
        root.destroy()
        tkinter.NoDefaultRoot()
        self.assertRaises(RuntimeError, tkinter.PhotoImage)


class BitmapImageTest(AbstractTkTest, unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        AbstractTkTest.setUpClass.__func__(cls)
        cls.testfile = support.findfile('python.xbm', subdir='imghdrdata')

    def test_create_from_file(self):
        image = tkinter.BitmapImage('::img::test', master=self.root,
                                    foreground='yellow', background='blue',
                                    file=self.testfile)
        self.assertEqual(str(image), '::img::test')
        self.assertEqual(image.type(), 'bitmap')
        self.assertEqual(image.width(), 16)
        self.assertEqual(image.height(), 16)
        self.assertIn('::img::test', self.root.image_names())
        del image
        support.gc_collect()  # For PyPy or other GCs.
        self.assertNotIn('::img::test', self.root.image_names())

    def test_create_from_data(self):
        with open(self.testfile, 'rb') as f:
            data = f.read()
        image = tkinter.BitmapImage('::img::test', master=self.root,
                                    foreground='yellow', background='blue',
                                    data=data)
        self.assertEqual(str(image), '::img::test')
        self.assertEqual(image.type(), 'bitmap')
        self.assertEqual(image.width(), 16)
        self.assertEqual(image.height(), 16)
        self.assertIn('::img::test', self.root.image_names())
        del image
        support.gc_collect()  # For PyPy or other GCs.
        self.assertNotIn('::img::test', self.root.image_names())

    def assertEqualStrList(self, actual, expected):
        self.assertIsInstance(actual, str)
        self.assertEqual(self.root.splitlist(actual), expected)

    def test_configure_data(self):
        image = tkinter.BitmapImage('::img::test', master=self.root)
        self.assertEqual(image['data'], '-data {} {} {} {}')
        with open(self.testfile, 'rb') as f:
            data = f.read()
        image.configure(data=data)
        self.assertEqualStrList(image['data'],
                                ('-data', '', '', '', data.decode('ascii')))
        self.assertEqual(image.width(), 16)
        self.assertEqual(image.height(), 16)

        self.assertEqual(image['maskdata'], '-maskdata {} {} {} {}')
        image.configure(maskdata=data)
        self.assertEqualStrList(image['maskdata'],
                                ('-maskdata', '', '', '', data.decode('ascii')))

    def test_configure_file(self):
        image = tkinter.BitmapImage('::img::test', master=self.root)
        self.assertEqual(image['file'], '-file {} {} {} {}')
        image.configure(file=self.testfile)
        self.assertEqualStrList(image['file'],
                                ('-file', '', '', '',self.testfile))
        self.assertEqual(image.width(), 16)
        self.assertEqual(image.height(), 16)

        self.assertEqual(image['maskfile'], '-maskfile {} {} {} {}')
        image.configure(maskfile=self.testfile)
        self.assertEqualStrList(image['maskfile'],
                                ('-maskfile', '', '', '', self.testfile))

    def test_configure_background(self):
        image = tkinter.BitmapImage('::img::test', master=self.root)
        self.assertEqual(image['background'], '-background {} {} {} {}')
        image.configure(background='blue')
        self.assertEqual(image['background'], '-background {} {} {} blue')

    def test_configure_foreground(self):
        image = tkinter.BitmapImage('::img::test', master=self.root)
        self.assertEqual(image['foreground'],
                         '-foreground {} {} #000000 #000000')
        image.configure(foreground='yellow')
        self.assertEqual(image['foreground'],
                         '-foreground {} {} #000000 yellow')


class PhotoImageTest(AbstractTkTest, unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        AbstractTkTest.setUpClass.__func__(cls)
        cls.testfile = support.findfile('python.gif', subdir='imghdrdata')

    def create(self):
        return tkinter.PhotoImage('::img::test', master=self.root,
                                  file=self.testfile)

    def colorlist(self, *args):
        if tkinter.TkVersion >= 8.6 and self.wantobjects:
            return args
        else:
            return tkinter._join(args)

    def check_create_from_file(self, ext):
        testfile = support.findfile('python.' + ext, subdir='imghdrdata')
        image = tkinter.PhotoImage('::img::test', master=self.root,
                                   file=testfile)
        self.assertEqual(str(image), '::img::test')
        self.assertEqual(image.type(), 'photo')
        self.assertEqual(image.width(), 16)
        self.assertEqual(image.height(), 16)
        self.assertEqual(image['data'], '')
        self.assertEqual(image['file'], testfile)
        self.assertIn('::img::test', self.root.image_names())
        del image
        support.gc_collect()  # For PyPy or other GCs.
        self.assertNotIn('::img::test', self.root.image_names())

    def check_create_from_data(self, ext):
        testfile = support.findfile('python.' + ext, subdir='imghdrdata')
        with open(testfile, 'rb') as f:
            data = f.read()
        image = tkinter.PhotoImage('::img::test', master=self.root,
                                   data=data)
        self.assertEqual(str(image), '::img::test')
        self.assertEqual(image.type(), 'photo')
        self.assertEqual(image.width(), 16)
        self.assertEqual(image.height(), 16)
        self.assertEqual(image['data'], data if self.wantobjects
                                        else data.decode('latin1'))
        self.assertEqual(image['file'], '')
        self.assertIn('::img::test', self.root.image_names())
        del image
        support.gc_collect()  # For PyPy or other GCs.
        self.assertNotIn('::img::test', self.root.image_names())

    def test_create_from_ppm_file(self):
        self.check_create_from_file('ppm')

    def test_create_from_ppm_data(self):
        self.check_create_from_data('ppm')

    def test_create_from_pgm_file(self):
        self.check_create_from_file('pgm')

    def test_create_from_pgm_data(self):
        self.check_create_from_data('pgm')

    def test_create_from_gif_file(self):
        self.check_create_from_file('gif')

    def test_create_from_gif_data(self):
        self.check_create_from_data('gif')

    @requires_tcl(8, 6)
    def test_create_from_png_file(self):
        self.check_create_from_file('png')

    @requires_tcl(8, 6)
    def test_create_from_png_data(self):
        self.check_create_from_data('png')

    def test_configure_data(self):
        image = tkinter.PhotoImage('::img::test', master=self.root)
        self.assertEqual(image['data'], '')
        with open(self.testfile, 'rb') as f:
            data = f.read()
        image.configure(data=data)
        self.assertEqual(image['data'], data if self.wantobjects
                                        else data.decode('latin1'))
        self.assertEqual(image.width(), 16)
        self.assertEqual(image.height(), 16)

    def test_configure_format(self):
        image = tkinter.PhotoImage('::img::test', master=self.root)
        self.assertEqual(image['format'], '')
        image.configure(file=self.testfile, format='gif')
        self.assertEqual(image['format'], ('gif',) if self.wantobjects
                                          else 'gif')
        self.assertEqual(image.width(), 16)
        self.assertEqual(image.height(), 16)

    def test_configure_file(self):
        image = tkinter.PhotoImage('::img::test', master=self.root)
        self.assertEqual(image['file'], '')
        image.configure(file=self.testfile)
        self.assertEqual(image['file'], self.testfile)
        self.assertEqual(image.width(), 16)
        self.assertEqual(image.height(), 16)

    def test_configure_gamma(self):
        image = tkinter.PhotoImage('::img::test', master=self.root)
        self.assertEqual(image['gamma'], '1.0')
        image.configure(gamma=2.0)
        self.assertEqual(image['gamma'], '2.0')

    def test_configure_width_height(self):
        image = tkinter.PhotoImage('::img::test', master=self.root)
        self.assertEqual(image['width'], '0')
        self.assertEqual(image['height'], '0')
        image.configure(width=20)
        image.configure(height=10)
        self.assertEqual(image['width'], '20')
        self.assertEqual(image['height'], '10')
        self.assertEqual(image.width(), 20)
        self.assertEqual(image.height(), 10)

    def test_configure_palette(self):
        image = tkinter.PhotoImage('::img::test', master=self.root)
        self.assertEqual(image['palette'], '')
        image.configure(palette=256)
        self.assertEqual(image['palette'], '256')
        image.configure(palette='3/4/2')
        self.assertEqual(image['palette'], '3/4/2')

    def test_blank(self):
        image = self.create()
        image.blank()
        self.assertEqual(image.width(), 16)
        self.assertEqual(image.height(), 16)
        self.assertEqual(image.get(4, 6), self.colorlist(0, 0, 0))

    def test_copy(self):
        image = self.create()
        image2 = image.copy()
        self.assertEqual(image2.width(), 16)
        self.assertEqual(image2.height(), 16)
        self.assertEqual(image.get(4, 6), image.get(4, 6))

    def test_subsample(self):
        image = self.create()
        image2 = image.subsample(2, 3)
        self.assertEqual(image2.width(), 8)
        self.assertEqual(image2.height(), 6)
        self.assertEqual(image2.get(2, 2), image.get(4, 6))

        image2 = image.subsample(2)
        self.assertEqual(image2.width(), 8)
        self.assertEqual(image2.height(), 8)
        self.assertEqual(image2.get(2, 3), image.get(4, 6))

    def test_zoom(self):
        image = self.create()
        image2 = image.zoom(2, 3)
        self.assertEqual(image2.width(), 32)
        self.assertEqual(image2.height(), 48)
        self.assertEqual(image2.get(8, 18), image.get(4, 6))
        self.assertEqual(image2.get(9, 20), image.get(4, 6))

        image2 = image.zoom(2)
        self.assertEqual(image2.width(), 32)
        self.assertEqual(image2.height(), 32)
        self.assertEqual(image2.get(8, 12), image.get(4, 6))
        self.assertEqual(image2.get(9, 13), image.get(4, 6))

    def test_put(self):
        image = self.create()
        image.put('{red green} {blue yellow}', to=(4, 6))
        self.assertEqual(image.get(4, 6), self.colorlist(255, 0, 0))
        self.assertEqual(image.get(5, 6),
                         self.colorlist(0, 128 if tkinter.TkVersion >= 8.6
                                           else 255, 0))
        self.assertEqual(image.get(4, 7), self.colorlist(0, 0, 255))
        self.assertEqual(image.get(5, 7), self.colorlist(255, 255, 0))

        image.put((('#f00', '#00ff00'), ('#000000fff', '#ffffffff0000')))
        self.assertEqual(image.get(0, 0), self.colorlist(255, 0, 0))
        self.assertEqual(image.get(1, 0), self.colorlist(0, 255, 0))
        self.assertEqual(image.get(0, 1), self.colorlist(0, 0, 255))
        self.assertEqual(image.get(1, 1), self.colorlist(255, 255, 0))

    def test_get(self):
        image = self.create()
        self.assertEqual(image.get(4, 6), self.colorlist(62, 116, 162))
        self.assertEqual(image.get(0, 0), self.colorlist(0, 0, 0))
        self.assertEqual(image.get(15, 15), self.colorlist(0, 0, 0))
        self.assertRaises(tkinter.TclError, image.get, -1, 0)
        self.assertRaises(tkinter.TclError, image.get, 0, -1)
        self.assertRaises(tkinter.TclError, image.get, 16, 15)
        self.assertRaises(tkinter.TclError, image.get, 15, 16)

    def test_write(self):
        image = self.create()
        self.addCleanup(support.unlink, support.TESTFN)

        image.write(support.TESTFN)
        image2 = tkinter.PhotoImage('::img::test2', master=self.root,
                                    format='ppm',
                                    file=support.TESTFN)
        self.assertEqual(str(image2), '::img::test2')
        self.assertEqual(image2.type(), 'photo')
        self.assertEqual(image2.width(), 16)
        self.assertEqual(image2.height(), 16)
        self.assertEqual(image2.get(0, 0), image.get(0, 0))
        self.assertEqual(image2.get(15, 8), image.get(15, 8))

        image.write(support.TESTFN, format='gif', from_coords=(4, 6, 6, 9))
        image3 = tkinter.PhotoImage('::img::test3', master=self.root,
                                    format='gif',
                                    file=support.TESTFN)
        self.assertEqual(str(image3), '::img::test3')
        self.assertEqual(image3.type(), 'photo')
        self.assertEqual(image3.width(), 2)
        self.assertEqual(image3.height(), 3)
        self.assertEqual(image3.get(0, 0), image.get(4, 6))
        self.assertEqual(image3.get(1, 2), image.get(5, 8))

    def test_transparency(self):
        image = self.create()
        self.assertEqual(image.transparency_get(0, 0), True)
        self.assertEqual(image.transparency_get(4, 6), False)
        image.transparency_set(4, 6, True)
        self.assertEqual(image.transparency_get(4, 6), True)
        image.transparency_set(4, 6, False)
        self.assertEqual(image.transparency_get(4, 6), False)


if __name__ == "__main__":
    unittest.main()
