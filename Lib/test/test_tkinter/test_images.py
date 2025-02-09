import unittest
import tkinter
from test import support
from test.support import os_helper
from test.test_tkinter.support import AbstractTkTest, AbstractDefaultRootTest, requires_tk

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
        cls.testfile = support.findfile('python.xbm', subdir='tkinterdata')

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

    def test_bug_100814(self):
        # gh-100814: Passing a callable option value causes AttributeError.
        with self.assertRaises(tkinter.TclError):
            tkinter.BitmapImage('::img::test', master=self.root, spam=print)
        image = tkinter.BitmapImage('::img::test', master=self.root)
        with self.assertRaises(tkinter.TclError):
            image.configure(spam=print)


class PhotoImageTest(AbstractTkTest, unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        AbstractTkTest.setUpClass.__func__(cls)
        cls.testfile = support.findfile('python.gif', subdir='tkinterdata')

    def create(self):
        return tkinter.PhotoImage('::img::test', master=self.root,
                                  file=self.testfile)

    def colorlist(self, *args):
        if tkinter.TkVersion >= 8.6 and self.wantobjects:
            return args
        else:
            return tkinter._join(args)

    def check_create_from_file(self, ext):
        testfile = support.findfile('python.' + ext, subdir='tkinterdata')
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
        testfile = support.findfile('python.' + ext, subdir='tkinterdata')
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

    @requires_tk(8, 6)
    def test_create_from_png_file(self):
        self.check_create_from_file('png')

    @requires_tk(8, 6)
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

    def test_bug_100814(self):
        # gh-100814: Passing a callable option value causes AttributeError.
        with self.assertRaises(tkinter.TclError):
            tkinter.PhotoImage('::img::test', master=self.root, spam=print)
        image = tkinter.PhotoImage('::img::test', master=self.root)
        with self.assertRaises(tkinter.TclError):
            image.configure(spam=print)

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
        self.assertEqual(image2.get(4, 6), image.get(4, 6))

        image2 = image.copy(from_coords=(2, 3, 14, 11))
        self.assertEqual(image2.width(), 12)
        self.assertEqual(image2.height(), 8)
        self.assertEqual(image2.get(0, 0), image.get(2, 3))
        self.assertEqual(image2.get(11, 7), image.get(13, 10))
        self.assertEqual(image2.get(2, 4), image.get(2+2, 4+3))

        image2 = image.copy(from_coords=(2, 3, 14, 11), zoom=2)
        self.assertEqual(image2.width(), 24)
        self.assertEqual(image2.height(), 16)
        self.assertEqual(image2.get(0, 0), image.get(2, 3))
        self.assertEqual(image2.get(23, 15), image.get(13, 10))
        self.assertEqual(image2.get(2*2, 4*2), image.get(2+2, 4+3))
        self.assertEqual(image2.get(2*2+1, 4*2+1), image.get(6+2, 2+3))

        image2 = image.copy(from_coords=(2, 3, 14, 11), subsample=2)
        self.assertEqual(image2.width(), 6)
        self.assertEqual(image2.height(), 4)
        self.assertEqual(image2.get(0, 0), image.get(2, 3))
        self.assertEqual(image2.get(5, 3), image.get(12, 9))
        self.assertEqual(image2.get(3, 2), image.get(3*2+2, 2*2+3))

        image2 = image.copy(from_coords=(2, 3, 14, 11), subsample=2, zoom=3)
        self.assertEqual(image2.width(), 18)
        self.assertEqual(image2.height(), 12)
        self.assertEqual(image2.get(0, 0), image.get(2, 3))
        self.assertEqual(image2.get(17, 11), image.get(12, 9))
        self.assertEqual(image2.get(1*3, 2*3), image.get(1*2+2, 2*2+3))
        self.assertEqual(image2.get(1*3+2, 2*3+2), image.get(1*2+2, 2*2+3))

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

        image2 = image.subsample(2, from_coords=(2, 3, 14, 11))
        self.assertEqual(image2.width(), 6)
        self.assertEqual(image2.height(), 4)
        self.assertEqual(image2.get(0, 0), image.get(2, 3))
        self.assertEqual(image2.get(5, 3), image.get(12, 9))
        self.assertEqual(image2.get(1, 2), image.get(1*2+2, 2*2+3))

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

        image2 = image.zoom(2, from_coords=(2, 3, 14, 11))
        self.assertEqual(image2.width(), 24)
        self.assertEqual(image2.height(), 16)
        self.assertEqual(image2.get(0, 0), image.get(2, 3))
        self.assertEqual(image2.get(23, 15), image.get(13, 10))
        self.assertEqual(image2.get(2*2, 4*2), image.get(2+2, 4+3))
        self.assertEqual(image2.get(2*2+1, 4*2+1), image.get(6+2, 2+3))

    def test_copy_replace(self):
        image = self.create()
        image2 = tkinter.PhotoImage(master=self.root)
        image2.copy_replace(image)
        self.assertEqual(image2.width(), 16)
        self.assertEqual(image2.height(), 16)
        self.assertEqual(image2.get(4, 6), image.get(4, 6))

        image2 = tkinter.PhotoImage(master=self.root)
        image2.copy_replace(image, from_coords=(2, 3, 14, 11))
        self.assertEqual(image2.width(), 12)
        self.assertEqual(image2.height(), 8)
        self.assertEqual(image2.get(0, 0), image.get(2, 3))
        self.assertEqual(image2.get(11, 7), image.get(13, 10))
        self.assertEqual(image2.get(2, 4), image.get(2+2, 4+3))

        image2 = tkinter.PhotoImage(master=self.root)
        image2.copy_replace(image)
        image2.copy_replace(image, from_coords=(2, 3, 14, 11), shrink=True)
        self.assertEqual(image2.width(), 12)
        self.assertEqual(image2.height(), 8)
        self.assertEqual(image2.get(0, 0), image.get(2, 3))
        self.assertEqual(image2.get(11, 7), image.get(13, 10))
        self.assertEqual(image2.get(2, 4), image.get(2+2, 4+3))

        image2 = tkinter.PhotoImage(master=self.root)
        image2.copy_replace(image, from_coords=(2, 3, 14, 11), to=(3, 6))
        self.assertEqual(image2.width(), 15)
        self.assertEqual(image2.height(), 14)
        self.assertEqual(image2.get(0+3, 0+6), image.get(2, 3))
        self.assertEqual(image2.get(11+3, 7+6), image.get(13, 10))
        self.assertEqual(image2.get(2+3, 4+6), image.get(2+2, 4+3))

        image2 = tkinter.PhotoImage(master=self.root)
        image2.copy_replace(image, from_coords=(2, 3, 14, 11), to=(0, 0, 100, 50))
        self.assertEqual(image2.width(), 100)
        self.assertEqual(image2.height(), 50)
        self.assertEqual(image2.get(0, 0), image.get(2, 3))
        self.assertEqual(image2.get(11, 7), image.get(13, 10))
        self.assertEqual(image2.get(2, 4), image.get(2+2, 4+3))
        self.assertEqual(image2.get(2+12, 4+8), image.get(2+2, 4+3))
        self.assertEqual(image2.get(2+12*2, 4), image.get(2+2, 4+3))
        self.assertEqual(image2.get(2, 4+8*3), image.get(2+2, 4+3))

        image2 = tkinter.PhotoImage(master=self.root)
        image2.copy_replace(image, from_coords=(2, 3, 14, 11), zoom=2)
        self.assertEqual(image2.width(), 24)
        self.assertEqual(image2.height(), 16)
        self.assertEqual(image2.get(0, 0), image.get(2, 3))
        self.assertEqual(image2.get(23, 15), image.get(13, 10))
        self.assertEqual(image2.get(2*2, 4*2), image.get(2+2, 4+3))
        self.assertEqual(image2.get(2*2+1, 4*2+1), image.get(6+2, 2+3))

        image2 = tkinter.PhotoImage(master=self.root)
        image2.copy_replace(image, from_coords=(2, 3, 14, 11), subsample=2)
        self.assertEqual(image2.width(), 6)
        self.assertEqual(image2.height(), 4)
        self.assertEqual(image2.get(0, 0), image.get(2, 3))
        self.assertEqual(image2.get(5, 3), image.get(12, 9))
        self.assertEqual(image2.get(1, 2), image.get(1*2+2, 2*2+3))

        image2 = tkinter.PhotoImage(master=self.root)
        image2.copy_replace(image, from_coords=(2, 3, 14, 11), subsample=2, zoom=3)
        self.assertEqual(image2.width(), 18)
        self.assertEqual(image2.height(), 12)
        self.assertEqual(image2.get(0, 0), image.get(2, 3))
        self.assertEqual(image2.get(17, 11), image.get(12, 9))
        self.assertEqual(image2.get(3*3, 2*3), image.get(3*2+2, 2*2+3))
        self.assertEqual(image2.get(3*3+2, 2*3+2), image.get(3*2+2, 2*2+3))
        self.assertEqual(image2.get(1*3, 2*3), image.get(1*2+2, 2*2+3))
        self.assertEqual(image2.get(1*3+2, 2*3+2), image.get(1*2+2, 2*2+3))

    def checkImgTrans(self, image, expected):
        actual = {(x, y)
                  for x in range(image.width())
                  for y in range(image.height())
                  if image.transparency_get(x, y)}
        self.assertEqual(actual, expected)

    def test_copy_replace_compositingrule(self):
        image1 = tkinter.PhotoImage(master=self.root, width=2, height=2)
        image1.blank()
        image1.put('black', to=(0, 0, 2, 2))
        image1.transparency_set(0, 0, True)

        # default compositingrule
        image2 = tkinter.PhotoImage(master=self.root, width=3, height=3)
        image2.blank()
        image2.put('white', to=(0, 0, 2, 2))
        image2.copy_replace(image1, to=(1, 1))
        self.checkImgTrans(image2, {(0, 2), (2, 0)})

        image3 = tkinter.PhotoImage(master=self.root, width=3, height=3)
        image3.blank()
        image3.put('white', to=(0, 0, 2, 2))
        image3.copy_replace(image1, to=(1, 1), compositingrule='overlay')
        self.checkImgTrans(image3, {(0, 2), (2, 0)})

        image4 = tkinter.PhotoImage(master=self.root, width=3, height=3)
        image4.blank()
        image4.put('white', to=(0, 0, 2, 2))
        image4.copy_replace(image1, to=(1, 1), compositingrule='set')
        self.checkImgTrans(image4, {(0, 2), (1, 1), (2, 0)})

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

    def test_read(self):
        # Due to the Tk bug https://core.tcl-lang.org/tk/tktview/1576528
        # the -from option does not work correctly for GIF and PNG files.
        # Use the PPM file for this test.
        testfile = support.findfile('python.ppm', subdir='tkinterdata')
        image = tkinter.PhotoImage(master=self.root, file=testfile)

        image2 = tkinter.PhotoImage(master=self.root)
        image2.read(testfile)
        self.assertEqual(image2.type(), 'photo')
        self.assertEqual(image2.width(), 16)
        self.assertEqual(image2.height(), 16)
        self.assertEqual(image2.get(0, 0), image.get(0, 0))
        self.assertEqual(image2.get(4, 6), image.get(4, 6))

        self.assertRaises(tkinter.TclError, image2.read, self.testfile, 'ppm')

        image2 = tkinter.PhotoImage(master=self.root)
        image2.read(testfile, from_coords=(2, 3, 14, 11))
        self.assertEqual(image2.width(), 12)
        self.assertEqual(image2.height(), 8)
        self.assertEqual(image2.get(0, 0), image.get(2, 3))
        self.assertEqual(image2.get(11, 7), image.get(13, 10))
        self.assertEqual(image2.get(2, 4), image.get(2+2, 4+3))

        image2 = tkinter.PhotoImage(master=self.root, file=testfile)
        self.assertEqual(image2.width(), 16)
        self.assertEqual(image2.height(), 16)
        image2.read(testfile, from_coords=(2, 3, 14, 11), shrink=True)
        self.assertEqual(image2.width(), 12)
        self.assertEqual(image2.height(), 8)
        self.assertEqual(image2.get(0, 0), image.get(2, 3))
        self.assertEqual(image2.get(11, 7), image.get(13, 10))
        self.assertEqual(image2.get(2, 4), image.get(2+2, 4+3))

        image2 = tkinter.PhotoImage(master=self.root)
        image2.read(testfile, from_coords=(2, 3, 14, 11), to=(3, 6))
        self.assertEqual(image2.type(), 'photo')
        self.assertEqual(image2.width(), 15)
        self.assertEqual(image2.height(), 14)
        self.assertEqual(image2.get(0+3, 0+6), image.get(2, 3))
        self.assertEqual(image2.get(11+3, 7+6), image.get(13, 10))
        self.assertEqual(image2.get(2+3, 4+6), image.get(2+2, 4+3))

    def test_write(self):
        filename = os_helper.TESTFN
        import locale
        if locale.getlocale()[0] is None:
            # Tcl uses Latin1 in the C locale
            filename = os_helper.TESTFN_ASCII
        image = self.create()
        self.addCleanup(os_helper.unlink, filename)

        image.write(filename)
        image2 = tkinter.PhotoImage('::img::test2', master=self.root,
                                    format='ppm', file=filename)
        self.assertEqual(str(image2), '::img::test2')
        self.assertEqual(image2.type(), 'photo')
        self.assertEqual(image2.width(), 16)
        self.assertEqual(image2.height(), 16)
        self.assertEqual(image2.get(0, 0), image.get(0, 0))
        self.assertEqual(image2.get(4, 6), image.get(4, 6))

        image.write(filename, format='gif', from_coords=(4, 6, 6, 9))
        image3 = tkinter.PhotoImage('::img::test3', master=self.root,
                                    format='gif', file=filename)
        self.assertEqual(str(image3), '::img::test3')
        self.assertEqual(image3.type(), 'photo')
        self.assertEqual(image3.width(), 2)
        self.assertEqual(image3.height(), 3)
        self.assertEqual(image3.get(0, 0), image.get(4, 6))
        self.assertEqual(image3.get(1, 2), image.get(5, 8))

        image.write(filename, background='#ff0000')
        image4 = tkinter.PhotoImage('::img::test4', master=self.root,
                                    format='ppm', file=filename)
        self.assertEqual(image4.get(0, 0), (255, 0, 0) if self.wantobjects else '255 0 0')
        self.assertEqual(image4.get(4, 6), image.get(4, 6))

        image.write(filename, grayscale=True)
        image5 = tkinter.PhotoImage('::img::test5', master=self.root,
                                    format='ppm', file=filename)
        c = image5.get(4, 6)
        if not self.wantobjects:
            c = c.split()
        self.assertTrue(c[0] == c[1] == c[2], c)

    def test_data(self):
        image = self.create()

        data = image.data()
        self.assertIsInstance(data, tuple)
        for row in data:
            self.assertIsInstance(row, str)
        c = image.get(4, 6)
        if not self.wantobjects:
            c = tuple(map(int, c.split()))
        self.assertEqual(data[6].split()[4], '#%02x%02x%02x' % c)

        data = image.data('ppm')
        image2 = tkinter.PhotoImage('::img::test2', master=self.root,
                                    format='ppm', data=data)
        self.assertEqual(str(image2), '::img::test2')
        self.assertEqual(image2.type(), 'photo')
        self.assertEqual(image2.width(), 16)
        self.assertEqual(image2.height(), 16)
        self.assertEqual(image2.get(0, 0), image.get(0, 0))
        self.assertEqual(image2.get(4, 6), image.get(4, 6))

        data = image.data(format='gif', from_coords=(4, 6, 6, 9))
        image3 = tkinter.PhotoImage('::img::test3', master=self.root,
                                    format='gif', data=data)
        self.assertEqual(str(image3), '::img::test3')
        self.assertEqual(image3.type(), 'photo')
        self.assertEqual(image3.width(), 2)
        self.assertEqual(image3.height(), 3)
        self.assertEqual(image3.get(0, 0), image.get(4, 6))
        self.assertEqual(image3.get(1, 2), image.get(5, 8))

        data = image.data('ppm', background='#ff0000')
        image4 = tkinter.PhotoImage('::img::test4', master=self.root,
                                    format='ppm', data=data)
        self.assertEqual(image4.get(0, 0), (255, 0, 0) if self.wantobjects else '255 0 0')
        self.assertEqual(image4.get(4, 6), image.get(4, 6))

        data = image.data('ppm', grayscale=True)
        image5 = tkinter.PhotoImage('::img::test5', master=self.root,
                                    format='ppm', data=data)
        c = image5.get(4, 6)
        if not self.wantobjects:
            c = c.split()
        self.assertTrue(c[0] == c[1] == c[2], c)


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
