import unittest
import re
import tkinter
from tkinter import TclError
from test.support import requires

from tkinter.test.support import pixels_conv, tcl_version, requires_tcl
from tkinter.test.widget_tests import AbstractWidgetTest

requires('gui')


class PackTest(AbstractWidgetTest, unittest.TestCase):

    def create2(self):
        pack = tkinter.Toplevel(self.root, name='pack')
        pack.wm_geometry('300x200+0+0')
        pack.wm_minsize(1, 1)
        a = tkinter.Frame(pack, name='a', width=20, height=40, bg='red')
        b = tkinter.Frame(pack, name='b', width=50, height=30, bg='blue')
        c = tkinter.Frame(pack, name='c', width=80, height=80, bg='green')
        d = tkinter.Frame(pack, name='d', width=40, height=30, bg='yellow')
        return pack, a, b, c, d

    def test_pack_configure_after(self):
        pack, a, b, c, d = self.create2()
        with self.assertRaisesRegex(TclError, 'window "%s" isn\'t packed' % b):
            a.pack_configure(after=b)
        with self.assertRaisesRegex(TclError, 'bad window path name ".foo"'):
            a.pack_configure(after='.foo')
        a.pack_configure(side='top')
        b.pack_configure(side='top')
        c.pack_configure(side='top')
        d.pack_configure(side='top')
        self.assertEqual(pack.pack_slaves(), [a, b, c, d])
        a.pack_configure(after=b)
        self.assertEqual(pack.pack_slaves(), [b, a, c, d])
        a.pack_configure(after=a)
        self.assertEqual(pack.pack_slaves(), [b, a, c, d])

    def test_pack_configure_anchor(self):
        pack, a, b, c, d = self.create2()
        def check(anchor, geom):
            a.pack_configure(side='top', ipadx=5, padx=10, ipady=15, pady=20,
                             expand=True, anchor=anchor)
            self.root.update()
            self.assertEqual(a.winfo_geometry(), geom)
        check('n', '30x70+135+20')
        check('ne', '30x70+260+20')
        check('e', '30x70+260+65')
        check('se', '30x70+260+110')
        check('s', '30x70+135+110')
        check('sw', '30x70+10+110')
        check('w', '30x70+10+65')
        check('nw', '30x70+10+20')
        check('center', '30x70+135+65')

    def test_pack_configure_before(self):
        pack, a, b, c, d = self.create2()
        with self.assertRaisesRegex(TclError, 'window "%s" isn\'t packed' % b):
            a.pack_configure(before=b)
        with self.assertRaisesRegex(TclError, 'bad window path name ".foo"'):
            a.pack_configure(before='.foo')
        a.pack_configure(side='top')
        b.pack_configure(side='top')
        c.pack_configure(side='top')
        d.pack_configure(side='top')
        self.assertEqual(pack.pack_slaves(), [a, b, c, d])
        a.pack_configure(before=d)
        self.assertEqual(pack.pack_slaves(), [b, c, a, d])
        a.pack_configure(before=a)
        self.assertEqual(pack.pack_slaves(), [b, c, a, d])

    def test_pack_configure_expand(self):
        pack, a, b, c, d = self.create2()
        def check(*geoms):
            self.root.update()
            self.assertEqual(a.winfo_geometry(), geoms[0])
            self.assertEqual(b.winfo_geometry(), geoms[1])
            self.assertEqual(c.winfo_geometry(), geoms[2])
            self.assertEqual(d.winfo_geometry(), geoms[3])
        a.pack_configure(side='left')
        b.pack_configure(side='top')
        c.pack_configure(side='right')
        d.pack_configure(side='bottom')
        check('20x40+0+80', '50x30+135+0', '80x80+220+75', '40x30+100+170')
        a.pack_configure(side='left', expand='yes')
        b.pack_configure(side='top', expand='on')
        c.pack_configure(side='right', expand=True)
        d.pack_configure(side='bottom', expand=1)
        check('20x40+40+80', '50x30+175+35', '80x80+180+110', '40x30+100+135')
        a.pack_configure(side='left', expand='yes', fill='both')
        b.pack_configure(side='top', expand='on', fill='both')
        c.pack_configure(side='right', expand=True, fill='both')
        d.pack_configure(side='bottom', expand=1, fill='both')
        check('100x200+0+0', '200x100+100+0', '160x100+140+100', '40x100+100+100')

    def test_pack_configure_in(self):
        pack, a, b, c, d = self.create2()
        a.pack_configure(side='top')
        b.pack_configure(side='top')
        c.pack_configure(side='top')
        d.pack_configure(side='top')
        a.pack_configure(in_=pack)
        self.assertEqual(pack.pack_slaves(), [b, c, d, a])
        a.pack_configure(in_=c)
        self.assertEqual(pack.pack_slaves(), [b, c, d])
        self.assertEqual(c.pack_slaves(), [a])
        with self.assertRaisesRegex(TclError,
                                    'can\'t pack %s inside itself' % (a,)):
            a.pack_configure(in_=a)
        with self.assertRaisesRegex(TclError, 'bad window path name ".foo"'):
            a.pack_configure(in_='.foo')

    def test_pack_configure_padx_ipadx_fill(self):
        pack, a, b, c, d = self.create2()
        def check(geom1, geom2, **kwargs):
            a.pack_forget()
            b.pack_forget()
            a.pack_configure(**kwargs)
            b.pack_configure(expand=True, fill='both')
            self.root.update()
            self.assertEqual(a.winfo_geometry(), geom1)
            self.assertEqual(b.winfo_geometry(), geom2)
        check('20x40+260+80', '240x200+0+0', side='right', padx=20)
        check('20x40+250+80', '240x200+0+0', side='right', padx=(10, 30))
        check('60x40+240+80', '240x200+0+0', side='right', ipadx=20)
        check('30x40+260+80', '250x200+0+0', side='right', ipadx=5, padx=10)
        check('20x40+260+80', '240x200+0+0', side='right', padx=20, fill='x')
        check('20x40+249+80', '240x200+0+0',
              side='right', padx=(9, 31), fill='x')
        check('60x40+240+80', '240x200+0+0', side='right', ipadx=20, fill='x')
        check('30x40+260+80', '250x200+0+0',
              side='right', ipadx=5, padx=10, fill='x')
        check('30x40+255+80', '250x200+0+0',
              side='right', ipadx=5, padx=(5, 15), fill='x')
        check('20x40+140+0', '300x160+0+40', side='top', padx=20)
        check('20x40+120+0', '300x160+0+40', side='top', padx=(0, 40))
        check('60x40+120+0', '300x160+0+40', side='top', ipadx=20)
        check('30x40+135+0', '300x160+0+40', side='top', ipadx=5, padx=10)
        check('30x40+130+0', '300x160+0+40', side='top', ipadx=5, padx=(5, 15))
        check('260x40+20+0', '300x160+0+40', side='top', padx=20, fill='x')
        check('260x40+25+0', '300x160+0+40',
              side='top', padx=(25, 15), fill='x')
        check('300x40+0+0', '300x160+0+40', side='top', ipadx=20, fill='x')
        check('280x40+10+0', '300x160+0+40',
              side='top', ipadx=5, padx=10, fill='x')
        check('280x40+5+0', '300x160+0+40',
              side='top', ipadx=5, padx=(5, 15), fill='x')
        a.pack_configure(padx='1c')
        self.assertEqual(a.pack_info()['padx'],
                         self._str(pack.winfo_pixels('1c')))
        a.pack_configure(ipadx='1c')
        self.assertEqual(a.pack_info()['ipadx'],
                         self._str(pack.winfo_pixels('1c')))

    def test_pack_configure_pady_ipady_fill(self):
        pack, a, b, c, d = self.create2()
        def check(geom1, geom2, **kwargs):
            a.pack_forget()
            b.pack_forget()
            a.pack_configure(**kwargs)
            b.pack_configure(expand=True, fill='both')
            self.root.update()
            self.assertEqual(a.winfo_geometry(), geom1)
            self.assertEqual(b.winfo_geometry(), geom2)
        check('20x40+280+80', '280x200+0+0', side='right', pady=20)
        check('20x40+280+70', '280x200+0+0', side='right', pady=(10, 30))
        check('20x80+280+60', '280x200+0+0', side='right', ipady=20)
        check('20x50+280+75', '280x200+0+0', side='right', ipady=5, pady=10)
        check('20x40+280+80', '280x200+0+0', side='right', pady=20, fill='x')
        check('20x40+280+69', '280x200+0+0',
              side='right', pady=(9, 31), fill='x')
        check('20x80+280+60', '280x200+0+0', side='right', ipady=20, fill='x')
        check('20x50+280+75', '280x200+0+0',
              side='right', ipady=5, pady=10, fill='x')
        check('20x50+280+70', '280x200+0+0',
              side='right', ipady=5, pady=(5, 15), fill='x')
        check('20x40+140+20', '300x120+0+80', side='top', pady=20)
        check('20x40+140+0', '300x120+0+80', side='top', pady=(0, 40))
        check('20x80+140+0', '300x120+0+80', side='top', ipady=20)
        check('20x50+140+10', '300x130+0+70', side='top', ipady=5, pady=10)
        check('20x50+140+5', '300x130+0+70', side='top', ipady=5, pady=(5, 15))
        check('300x40+0+20', '300x120+0+80', side='top', pady=20, fill='x')
        check('300x40+0+25', '300x120+0+80',
              side='top', pady=(25, 15), fill='x')
        check('300x80+0+0', '300x120+0+80', side='top', ipady=20, fill='x')
        check('300x50+0+10', '300x130+0+70',
              side='top', ipady=5, pady=10, fill='x')
        check('300x50+0+5', '300x130+0+70',
              side='top', ipady=5, pady=(5, 15), fill='x')
        a.pack_configure(pady='1c')
        self.assertEqual(a.pack_info()['pady'],
                         self._str(pack.winfo_pixels('1c')))
        a.pack_configure(ipady='1c')
        self.assertEqual(a.pack_info()['ipady'],
                         self._str(pack.winfo_pixels('1c')))

    def test_pack_configure_side(self):
        pack, a, b, c, d = self.create2()
        def check(side, geom1, geom2):
            a.pack_configure(side=side)
            self.assertEqual(a.pack_info()['side'], side)
            b.pack_configure(expand=True, fill='both')
            self.root.update()
            self.assertEqual(a.winfo_geometry(), geom1)
            self.assertEqual(b.winfo_geometry(), geom2)
        check('top', '20x40+140+0', '300x160+0+40')
        check('bottom', '20x40+140+160', '300x160+0+0')
        check('left', '20x40+0+80', '280x200+20+0')
        check('right', '20x40+280+80', '280x200+0+0')

    def test_pack_forget(self):
        pack, a, b, c, d = self.create2()
        a.pack_configure()
        b.pack_configure()
        c.pack_configure()
        self.assertEqual(pack.pack_slaves(), [a, b, c])
        b.pack_forget()
        self.assertEqual(pack.pack_slaves(), [a, c])
        b.pack_forget()
        self.assertEqual(pack.pack_slaves(), [a, c])
        d.pack_forget()

    def test_pack_info(self):
        pack, a, b, c, d = self.create2()
        with self.assertRaisesRegex(TclError, 'window "%s" isn\'t packed' % a):
            a.pack_info()
        a.pack_configure()
        b.pack_configure(side='right', in_=a, anchor='s', expand=True, fill='x',
                         ipadx=5, padx=10, ipady=2, pady=(5, 15))
        info = a.pack_info()
        self.assertIsInstance(info, dict)
        self.assertEqual(info['anchor'], 'center')
        self.assertEqual(info['expand'], self._str(0))
        self.assertEqual(info['fill'], 'none')
        self.assertEqual(info['in'], pack)
        self.assertEqual(info['ipadx'], self._str(0))
        self.assertEqual(info['ipady'], self._str(0))
        self.assertEqual(info['padx'], self._str(0))
        self.assertEqual(info['pady'], self._str(0))
        self.assertEqual(info['side'], 'top')
        info = b.pack_info()
        self.assertIsInstance(info, dict)
        self.assertEqual(info['anchor'], 's')
        self.assertEqual(info['expand'], self._str(1))
        self.assertEqual(info['fill'], 'x')
        self.assertEqual(info['in'], a)
        self.assertEqual(info['ipadx'], self._str(5))
        self.assertEqual(info['ipady'], self._str(2))
        self.assertEqual(info['padx'], self._str(10))
        self.assertEqual(info['pady'], self._str((5, 15)))
        self.assertEqual(info['side'], 'right')

    def test_pack_propagate(self):
        pack, a, b, c, d = self.create2()
        pack.configure(width=300, height=200)
        a.pack_configure()
        pack.pack_propagate(False)
        self.root.update()
        self.assertEqual(pack.winfo_reqwidth(), 300)
        self.assertEqual(pack.winfo_reqheight(), 200)
        pack.pack_propagate(True)
        self.root.update()
        self.assertEqual(pack.winfo_reqwidth(), 20)
        self.assertEqual(pack.winfo_reqheight(), 40)

    def test_pack_slaves(self):
        pack, a, b, c, d = self.create2()
        self.assertEqual(pack.pack_slaves(), [])
        a.pack_configure()
        self.assertEqual(pack.pack_slaves(), [a])
        b.pack_configure()
        self.assertEqual(pack.pack_slaves(), [a, b])


class PlaceTest(AbstractWidgetTest, unittest.TestCase):

    def create2(self):
        t = tkinter.Toplevel(self.root, width=300, height=200, bd=0)
        t.wm_geometry('+0+0')
        f = tkinter.Frame(t, width=154, height=84, bd=2, relief='raised')
        f.place_configure(x=48, y=38)
        f2 = tkinter.Frame(t, width=30, height=60, bd=2, relief='raised')
        self.root.update()
        return t, f, f2

    def test_place_configure_in(self):
        t, f, f2 = self.create2()
        self.assertEqual(f2.winfo_manager(), '')
        with self.assertRaisesRegex(TclError, "can't place %s relative to "
                                    "itself" % re.escape(str(f2))):
            f2.place_configure(in_=f2)
        if tcl_version >= (8, 5):
            self.assertEqual(f2.winfo_manager(), '')
        with self.assertRaisesRegex(TclError, 'bad window path name'):
            f2.place_configure(in_='spam')
        f2.place_configure(in_=f)
        self.assertEqual(f2.winfo_manager(), 'place')

    def test_place_configure_x(self):
        t, f, f2 = self.create2()
        f2.place_configure(in_=f)
        self.assertEqual(f2.place_info()['x'], '0')
        self.root.update()
        self.assertEqual(f2.winfo_x(), 50)
        f2.place_configure(x=100)
        self.assertEqual(f2.place_info()['x'], '100')
        self.root.update()
        self.assertEqual(f2.winfo_x(), 150)
        f2.place_configure(x=-10, relx=1)
        self.assertEqual(f2.place_info()['x'], '-10')
        self.root.update()
        self.assertEqual(f2.winfo_x(), 190)
        with self.assertRaisesRegex(TclError, 'bad screen distance "spam"'):
            f2.place_configure(in_=f, x='spam')

    def test_place_configure_y(self):
        t, f, f2 = self.create2()
        f2.place_configure(in_=f)
        self.assertEqual(f2.place_info()['y'], '0')
        self.root.update()
        self.assertEqual(f2.winfo_y(), 40)
        f2.place_configure(y=50)
        self.assertEqual(f2.place_info()['y'], '50')
        self.root.update()
        self.assertEqual(f2.winfo_y(), 90)
        f2.place_configure(y=-10, rely=1)
        self.assertEqual(f2.place_info()['y'], '-10')
        self.root.update()
        self.assertEqual(f2.winfo_y(), 110)
        with self.assertRaisesRegex(TclError, 'bad screen distance "spam"'):
            f2.place_configure(in_=f, y='spam')

    def test_place_configure_relx(self):
        t, f, f2 = self.create2()
        f2.place_configure(in_=f)
        self.assertEqual(f2.place_info()['relx'], '0')
        self.root.update()
        self.assertEqual(f2.winfo_x(), 50)
        f2.place_configure(relx=0.5)
        self.assertEqual(f2.place_info()['relx'], '0.5')
        self.root.update()
        self.assertEqual(f2.winfo_x(), 125)
        f2.place_configure(relx=1)
        self.assertEqual(f2.place_info()['relx'], '1')
        self.root.update()
        self.assertEqual(f2.winfo_x(), 200)
        with self.assertRaisesRegex(TclError, 'expected floating-point number '
                                    'but got "spam"'):
            f2.place_configure(in_=f, relx='spam')

    def test_place_configure_rely(self):
        t, f, f2 = self.create2()
        f2.place_configure(in_=f)
        self.assertEqual(f2.place_info()['rely'], '0')
        self.root.update()
        self.assertEqual(f2.winfo_y(), 40)
        f2.place_configure(rely=0.5)
        self.assertEqual(f2.place_info()['rely'], '0.5')
        self.root.update()
        self.assertEqual(f2.winfo_y(), 80)
        f2.place_configure(rely=1)
        self.assertEqual(f2.place_info()['rely'], '1')
        self.root.update()
        self.assertEqual(f2.winfo_y(), 120)
        with self.assertRaisesRegex(TclError, 'expected floating-point number '
                                    'but got "spam"'):
            f2.place_configure(in_=f, rely='spam')

    def test_place_configure_anchor(self):
        f = tkinter.Frame(self.root)
        with self.assertRaisesRegex(TclError, 'bad anchor "j"'):
            f.place_configure(anchor='j')
        with self.assertRaisesRegex(TclError, 'ambiguous anchor ""'):
            f.place_configure(anchor='')
        for value in 'n', 'ne', 'e', 'se', 's', 'sw', 'w', 'nw', 'center':
            f.place_configure(anchor=value)
            self.assertEqual(f.place_info()['anchor'], value)

    def test_place_configure_width(self):
        t, f, f2 = self.create2()
        f2.place_configure(in_=f, width=120)
        self.root.update()
        self.assertEqual(f2.winfo_width(), 120)
        f2.place_configure(width='')
        self.root.update()
        self.assertEqual(f2.winfo_width(), 30)
        with self.assertRaisesRegex(TclError, 'bad screen distance "abcd"'):
            f2.place_configure(width='abcd')

    def test_place_configure_height(self):
        t, f, f2 = self.create2()
        f2.place_configure(in_=f, height=120)
        self.root.update()
        self.assertEqual(f2.winfo_height(), 120)
        f2.place_configure(height='')
        self.root.update()
        self.assertEqual(f2.winfo_height(), 60)
        with self.assertRaisesRegex(TclError, 'bad screen distance "abcd"'):
            f2.place_configure(height='abcd')

    def test_place_configure_relwidth(self):
        t, f, f2 = self.create2()
        f2.place_configure(in_=f, relwidth=0.5)
        self.root.update()
        self.assertEqual(f2.winfo_width(), 75)
        f2.place_configure(relwidth='')
        self.root.update()
        self.assertEqual(f2.winfo_width(), 30)
        with self.assertRaisesRegex(TclError, 'expected floating-point number '
                                    'but got "abcd"'):
            f2.place_configure(relwidth='abcd')

    def test_place_configure_relheight(self):
        t, f, f2 = self.create2()
        f2.place_configure(in_=f, relheight=0.5)
        self.root.update()
        self.assertEqual(f2.winfo_height(), 40)
        f2.place_configure(relheight='')
        self.root.update()
        self.assertEqual(f2.winfo_height(), 60)
        with self.assertRaisesRegex(TclError, 'expected floating-point number '
                                    'but got "abcd"'):
            f2.place_configure(relheight='abcd')

    def test_place_configure_bordermode(self):
        f = tkinter.Frame(self.root)
        with self.assertRaisesRegex(TclError, 'bad bordermode "j"'):
            f.place_configure(bordermode='j')
        with self.assertRaisesRegex(TclError, 'ambiguous bordermode ""'):
            f.place_configure(bordermode='')
        for value in 'inside', 'outside', 'ignore':
            f.place_configure(bordermode=value)
            self.assertEqual(f.place_info()['bordermode'], value)

    def test_place_forget(self):
        foo = tkinter.Frame(self.root)
        foo.place_configure(width=50, height=50)
        self.root.update()
        foo.place_forget()
        self.root.update()
        self.assertFalse(foo.winfo_ismapped())
        with self.assertRaises(TypeError):
            foo.place_forget(0)

    def test_place_info(self):
        t, f, f2 = self.create2()
        f2.place_configure(in_=f, x=1, y=2, width=3, height=4,
                           relx=0.1, rely=0.2, relwidth=0.3, relheight=0.4,
                           anchor='se', bordermode='outside')
        info = f2.place_info()
        self.assertIsInstance(info, dict)
        self.assertEqual(info['x'], '1')
        self.assertEqual(info['y'], '2')
        self.assertEqual(info['width'], '3')
        self.assertEqual(info['height'], '4')
        self.assertEqual(info['relx'], '0.1')
        self.assertEqual(info['rely'], '0.2')
        self.assertEqual(info['relwidth'], '0.3')
        self.assertEqual(info['relheight'], '0.4')
        self.assertEqual(info['anchor'], 'se')
        self.assertEqual(info['bordermode'], 'outside')
        self.assertEqual(info['x'], '1')
        self.assertEqual(info['x'], '1')
        with self.assertRaises(TypeError):
            f2.place_info(0)

    def test_place_slaves(self):
        foo = tkinter.Frame(self.root)
        bar = tkinter.Frame(self.root)
        self.assertEqual(foo.place_slaves(), [])
        bar.place_configure(in_=foo)
        self.assertEqual(foo.place_slaves(), [bar])
        with self.assertRaises(TypeError):
            foo.place_slaves(0)


class GridTest(AbstractWidgetTest, unittest.TestCase):

    def tearDown(self):
        super().tearDown()
        cols, rows = self.root.grid_size()
        for i in range(cols + 1):
            self.root.grid_columnconfigure(i, weight=0, minsize=0, pad=0, uniform='')
        for i in range(rows + 1):
            self.root.grid_rowconfigure(i, weight=0, minsize=0, pad=0, uniform='')
        self.root.grid_propagate(1)
        if tcl_version >= (8, 5):
            self.root.grid_anchor('nw')

    def test_grid_configure(self):
        b = tkinter.Button(self.root)
        self.addCleanup(b.destroy)
        self.assertEqual(b.grid_info(), {})
        b.grid_configure()
        self.assertEqual(b.grid_info()['in'], self.root)
        self.assertEqual(b.grid_info()['column'], self._str(0))
        self.assertEqual(b.grid_info()['row'], self._str(0))
        b.grid_configure({'column': 1}, row=2)
        self.assertEqual(b.grid_info()['column'], self._str(1))
        self.assertEqual(b.grid_info()['row'], self._str(2))

    def test_grid_configure_column(self):
        b = tkinter.Button(self.root)
        with self.assertRaisesRegex(TclError, 'bad column value "-1": '
                                    'must be a non-negative integer'):
            b.grid_configure(column=-1)
        b.grid_configure(column=2)
        self.assertEqual(b.grid_info()['column'], self._str(2))

    def test_grid_configure_columnspan(self):
        b = tkinter.Button(self.root)
        with self.assertRaisesRegex(TclError, 'bad columnspan value "0": '
                                    'must be a positive integer'):
            b.grid_configure(columnspan=0)
        b.grid_configure(columnspan=2)
        self.assertEqual(b.grid_info()['columnspan'], self._str(2))

    def test_grid_configure_in(self):
        f = tkinter.Frame(self.root)
        b = tkinter.Button(self.root)
        self.assertEqual(b.grid_info(), {})
        b.grid_configure()
        self.assertEqual(b.grid_info()['in'], self.root)
        b.grid_configure(in_=f)
        self.assertEqual(b.grid_info()['in'], f)
        b.grid_configure({'in': self.root})
        self.assertEqual(b.grid_info()['in'], self.root)

    def test_grid_configure_ipadx(self):
        b = tkinter.Button(self.root)
        with self.assertRaisesRegex(TclError, 'bad ipadx value "-1": '
                                    'must be positive screen distance'):
            b.grid_configure(ipadx=-1)
        b.grid_configure(ipadx=1)
        self.assertEqual(b.grid_info()['ipadx'], self._str(1))
        b.grid_configure(ipadx='.5c')
        self.assertEqual(b.grid_info()['ipadx'],
                         self._str(round(pixels_conv('.5c') * self.scaling)))

    def test_grid_configure_ipady(self):
        b = tkinter.Button(self.root)
        with self.assertRaisesRegex(TclError, 'bad ipady value "-1": '
                                    'must be positive screen distance'):
            b.grid_configure(ipady=-1)
        b.grid_configure(ipady=1)
        self.assertEqual(b.grid_info()['ipady'], self._str(1))
        b.grid_configure(ipady='.5c')
        self.assertEqual(b.grid_info()['ipady'],
                         self._str(round(pixels_conv('.5c') * self.scaling)))

    def test_grid_configure_padx(self):
        b = tkinter.Button(self.root)
        with self.assertRaisesRegex(TclError, 'bad pad value "-1": '
                                    'must be positive screen distance'):
            b.grid_configure(padx=-1)
        b.grid_configure(padx=1)
        self.assertEqual(b.grid_info()['padx'], self._str(1))
        b.grid_configure(padx=(10, 5))
        self.assertEqual(b.grid_info()['padx'], self._str((10, 5)))
        b.grid_configure(padx='.5c')
        self.assertEqual(b.grid_info()['padx'],
                         self._str(round(pixels_conv('.5c') * self.scaling)))

    def test_grid_configure_pady(self):
        b = tkinter.Button(self.root)
        with self.assertRaisesRegex(TclError, 'bad pad value "-1": '
                                    'must be positive screen distance'):
            b.grid_configure(pady=-1)
        b.grid_configure(pady=1)
        self.assertEqual(b.grid_info()['pady'], self._str(1))
        b.grid_configure(pady=(10, 5))
        self.assertEqual(b.grid_info()['pady'], self._str((10, 5)))
        b.grid_configure(pady='.5c')
        self.assertEqual(b.grid_info()['pady'],
                         self._str(round(pixels_conv('.5c') * self.scaling)))

    def test_grid_configure_row(self):
        b = tkinter.Button(self.root)
        self.addCleanup(b.destroy)
        with self.assertRaisesRegex(TclError, 'bad (row|grid) value "-1": '
                                    'must be a non-negative integer'):
            b.grid_configure(row=-1)
        b.grid_configure(row=2)
        self.assertEqual(b.grid_info()['row'], self._str(2))

    def test_grid_configure_rownspan(self):
        b = tkinter.Button(self.root)
        with self.assertRaisesRegex(TclError, 'bad rowspan value "0": '
                                    'must be a positive integer'):
            b.grid_configure(rowspan=0)
        b.grid_configure(rowspan=2)
        self.assertEqual(b.grid_info()['rowspan'], self._str(2))

    def test_grid_configure_sticky(self):
        f = tkinter.Frame(self.root, bg='red')
        with self.assertRaisesRegex(TclError, 'bad stickyness value "glue"'):
            f.grid_configure(sticky='glue')
        f.grid_configure(sticky='ne')
        self.assertEqual(f.grid_info()['sticky'], 'ne')
        f.grid_configure(sticky='n,s,e,w')
        self.assertEqual(f.grid_info()['sticky'], 'nesw')

    def test_grid_columnconfigure(self):
        with self.assertRaises(TypeError):
            self.root.grid_columnconfigure()
        self.assertEqual(self.root.grid_columnconfigure(0),
                         {'minsize': 0, 'pad': 0, 'uniform': None, 'weight': 0})
        with self.assertRaisesRegex(TclError, 'bad option "-foo"'):
            self.root.grid_columnconfigure(0, 'foo')
        self.root.grid_columnconfigure((0, 3), weight=2)
        with self.assertRaisesRegex(TclError,
                                    'must specify a single element on retrieval'):
            self.root.grid_columnconfigure((0, 3))
        b = tkinter.Button(self.root)
        b.grid_configure(column=0, row=0)
        if tcl_version >= (8, 5):
            self.root.grid_columnconfigure('all', weight=3)
            with self.assertRaisesRegex(TclError, 'expected integer but got "all"'):
                self.root.grid_columnconfigure('all')
            self.assertEqual(self.root.grid_columnconfigure(0, 'weight'), 3)
        self.assertEqual(self.root.grid_columnconfigure(3, 'weight'), 2)
        self.assertEqual(self.root.grid_columnconfigure(265, 'weight'), 0)
        if tcl_version >= (8, 5):
            self.root.grid_columnconfigure(b, weight=4)
            self.assertEqual(self.root.grid_columnconfigure(0, 'weight'), 4)

    def test_grid_columnconfigure_minsize(self):
        with self.assertRaisesRegex(TclError, 'bad screen distance "foo"'):
            self.root.grid_columnconfigure(0, minsize='foo')
        self.root.grid_columnconfigure(0, minsize=10)
        self.assertEqual(self.root.grid_columnconfigure(0, 'minsize'), 10)
        self.assertEqual(self.root.grid_columnconfigure(0)['minsize'], 10)

    def test_grid_columnconfigure_weight(self):
        with self.assertRaisesRegex(TclError, 'expected integer but got "bad"'):
            self.root.grid_columnconfigure(0, weight='bad')
        with self.assertRaisesRegex(TclError, 'invalid arg "-weight": '
                                    'should be non-negative'):
            self.root.grid_columnconfigure(0, weight=-3)
        self.root.grid_columnconfigure(0, weight=3)
        self.assertEqual(self.root.grid_columnconfigure(0, 'weight'), 3)
        self.assertEqual(self.root.grid_columnconfigure(0)['weight'], 3)

    def test_grid_columnconfigure_pad(self):
        with self.assertRaisesRegex(TclError, 'bad screen distance "foo"'):
            self.root.grid_columnconfigure(0, pad='foo')
        with self.assertRaisesRegex(TclError, 'invalid arg "-pad": '
                                    'should be non-negative'):
            self.root.grid_columnconfigure(0, pad=-3)
        self.root.grid_columnconfigure(0, pad=3)
        self.assertEqual(self.root.grid_columnconfigure(0, 'pad'), 3)
        self.assertEqual(self.root.grid_columnconfigure(0)['pad'], 3)

    def test_grid_columnconfigure_uniform(self):
        self.root.grid_columnconfigure(0, uniform='foo')
        self.assertEqual(self.root.grid_columnconfigure(0, 'uniform'), 'foo')
        self.assertEqual(self.root.grid_columnconfigure(0)['uniform'], 'foo')

    def test_grid_rowconfigure(self):
        with self.assertRaises(TypeError):
            self.root.grid_rowconfigure()
        self.assertEqual(self.root.grid_rowconfigure(0),
                         {'minsize': 0, 'pad': 0, 'uniform': None, 'weight': 0})
        with self.assertRaisesRegex(TclError, 'bad option "-foo"'):
            self.root.grid_rowconfigure(0, 'foo')
        self.root.grid_rowconfigure((0, 3), weight=2)
        with self.assertRaisesRegex(TclError,
                                    'must specify a single element on retrieval'):
            self.root.grid_rowconfigure((0, 3))
        b = tkinter.Button(self.root)
        b.grid_configure(column=0, row=0)
        if tcl_version >= (8, 5):
            self.root.grid_rowconfigure('all', weight=3)
            with self.assertRaisesRegex(TclError, 'expected integer but got "all"'):
                self.root.grid_rowconfigure('all')
            self.assertEqual(self.root.grid_rowconfigure(0, 'weight'), 3)
        self.assertEqual(self.root.grid_rowconfigure(3, 'weight'), 2)
        self.assertEqual(self.root.grid_rowconfigure(265, 'weight'), 0)
        if tcl_version >= (8, 5):
            self.root.grid_rowconfigure(b, weight=4)
            self.assertEqual(self.root.grid_rowconfigure(0, 'weight'), 4)

    def test_grid_rowconfigure_minsize(self):
        with self.assertRaisesRegex(TclError, 'bad screen distance "foo"'):
            self.root.grid_rowconfigure(0, minsize='foo')
        self.root.grid_rowconfigure(0, minsize=10)
        self.assertEqual(self.root.grid_rowconfigure(0, 'minsize'), 10)
        self.assertEqual(self.root.grid_rowconfigure(0)['minsize'], 10)

    def test_grid_rowconfigure_weight(self):
        with self.assertRaisesRegex(TclError, 'expected integer but got "bad"'):
            self.root.grid_rowconfigure(0, weight='bad')
        with self.assertRaisesRegex(TclError, 'invalid arg "-weight": '
                                    'should be non-negative'):
            self.root.grid_rowconfigure(0, weight=-3)
        self.root.grid_rowconfigure(0, weight=3)
        self.assertEqual(self.root.grid_rowconfigure(0, 'weight'), 3)
        self.assertEqual(self.root.grid_rowconfigure(0)['weight'], 3)

    def test_grid_rowconfigure_pad(self):
        with self.assertRaisesRegex(TclError, 'bad screen distance "foo"'):
            self.root.grid_rowconfigure(0, pad='foo')
        with self.assertRaisesRegex(TclError, 'invalid arg "-pad": '
                                    'should be non-negative'):
            self.root.grid_rowconfigure(0, pad=-3)
        self.root.grid_rowconfigure(0, pad=3)
        self.assertEqual(self.root.grid_rowconfigure(0, 'pad'), 3)
        self.assertEqual(self.root.grid_rowconfigure(0)['pad'], 3)

    def test_grid_rowconfigure_uniform(self):
        self.root.grid_rowconfigure(0, uniform='foo')
        self.assertEqual(self.root.grid_rowconfigure(0, 'uniform'), 'foo')
        self.assertEqual(self.root.grid_rowconfigure(0)['uniform'], 'foo')

    def test_grid_forget(self):
        b = tkinter.Button(self.root)
        c = tkinter.Button(self.root)
        b.grid_configure(row=2, column=2, rowspan=2, columnspan=2,
                         padx=3, pady=4, sticky='ns')
        self.assertEqual(self.root.grid_slaves(), [b])
        b.grid_forget()
        c.grid_forget()
        self.assertEqual(self.root.grid_slaves(), [])
        self.assertEqual(b.grid_info(), {})
        b.grid_configure(row=0, column=0)
        info = b.grid_info()
        self.assertEqual(info['row'], self._str(0))
        self.assertEqual(info['column'], self._str(0))
        self.assertEqual(info['rowspan'], self._str(1))
        self.assertEqual(info['columnspan'], self._str(1))
        self.assertEqual(info['padx'], self._str(0))
        self.assertEqual(info['pady'], self._str(0))
        self.assertEqual(info['sticky'], '')

    def test_grid_remove(self):
        b = tkinter.Button(self.root)
        c = tkinter.Button(self.root)
        b.grid_configure(row=2, column=2, rowspan=2, columnspan=2,
                         padx=3, pady=4, sticky='ns')
        self.assertEqual(self.root.grid_slaves(), [b])
        b.grid_remove()
        c.grid_remove()
        self.assertEqual(self.root.grid_slaves(), [])
        self.assertEqual(b.grid_info(), {})
        b.grid_configure(row=0, column=0)
        info = b.grid_info()
        self.assertEqual(info['row'], self._str(0))
        self.assertEqual(info['column'], self._str(0))
        self.assertEqual(info['rowspan'], self._str(2))
        self.assertEqual(info['columnspan'], self._str(2))
        self.assertEqual(info['padx'], self._str(3))
        self.assertEqual(info['pady'], self._str(4))
        self.assertEqual(info['sticky'], 'ns')

    def test_grid_info(self):
        b = tkinter.Button(self.root)
        self.assertEqual(b.grid_info(), {})
        b.grid_configure(row=2, column=2, rowspan=2, columnspan=2,
                         padx=3, pady=4, sticky='ns')
        info = b.grid_info()
        self.assertIsInstance(info, dict)
        self.assertEqual(info['in'], self.root)
        self.assertEqual(info['row'], self._str(2))
        self.assertEqual(info['column'], self._str(2))
        self.assertEqual(info['rowspan'], self._str(2))
        self.assertEqual(info['columnspan'], self._str(2))
        self.assertEqual(info['padx'], self._str(3))
        self.assertEqual(info['pady'], self._str(4))
        self.assertEqual(info['sticky'], 'ns')

    @requires_tcl(8, 5)
    def test_grid_anchor(self):
        with self.assertRaisesRegex(TclError, 'bad anchor "x"'):
            self.root.grid_anchor('x')
        with self.assertRaisesRegex(TclError, 'ambiguous anchor ""'):
            self.root.grid_anchor('')
        with self.assertRaises(TypeError):
            self.root.grid_anchor('se', 'nw')
        self.root.grid_anchor('se')
        self.assertEqual(self.root.tk.call('grid', 'anchor', self.root), 'se')

    def test_grid_bbox(self):
        self.assertEqual(self.root.grid_bbox(), (0, 0, 0, 0))
        self.assertEqual(self.root.grid_bbox(0, 0), (0, 0, 0, 0))
        self.assertEqual(self.root.grid_bbox(0, 0, 1, 1), (0, 0, 0, 0))
        with self.assertRaisesRegex(TclError, 'expected integer but got "x"'):
            self.root.grid_bbox('x', 0)
        with self.assertRaisesRegex(TclError, 'expected integer but got "x"'):
            self.root.grid_bbox(0, 'x')
        with self.assertRaisesRegex(TclError, 'expected integer but got "x"'):
            self.root.grid_bbox(0, 0, 'x', 0)
        with self.assertRaisesRegex(TclError, 'expected integer but got "x"'):
            self.root.grid_bbox(0, 0, 0, 'x')
        with self.assertRaises(TypeError):
            self.root.grid_bbox(0, 0, 0, 0, 0)
        t = tkinter.Toplevel(self.root)
        # de-maximize
        t.wm_geometry('1x1+0+0')
        t.wm_geometry('')
        f1 = tkinter.Frame(t, width=75, height=75, bg='red')
        f2 = tkinter.Frame(t, width=90, height=90, bg='blue')
        f1.grid_configure(row=0, column=0)
        f2.grid_configure(row=1, column=1)
        self.root.update()
        self.assertEqual(t.grid_bbox(), (0, 0, 165, 165))
        self.assertEqual(t.grid_bbox(0, 0), (0, 0, 75, 75))
        self.assertEqual(t.grid_bbox(0, 0, 1, 1), (0, 0, 165, 165))
        self.assertEqual(t.grid_bbox(1, 1), (75, 75, 90, 90))
        self.assertEqual(t.grid_bbox(10, 10, 0, 0), (0, 0, 165, 165))
        self.assertEqual(t.grid_bbox(-2, -2, -1, -1), (0, 0, 0, 0))
        self.assertEqual(t.grid_bbox(10, 10, 12, 12), (165, 165, 0, 0))

    def test_grid_location(self):
        with self.assertRaises(TypeError):
            self.root.grid_location()
        with self.assertRaises(TypeError):
            self.root.grid_location(0)
        with self.assertRaises(TypeError):
            self.root.grid_location(0, 0, 0)
        with self.assertRaisesRegex(TclError, 'bad screen distance "x"'):
            self.root.grid_location('x', 'y')
        with self.assertRaisesRegex(TclError, 'bad screen distance "y"'):
            self.root.grid_location('1c', 'y')
        t = tkinter.Toplevel(self.root)
        # de-maximize
        t.wm_geometry('1x1+0+0')
        t.wm_geometry('')
        f = tkinter.Frame(t, width=200, height=100,
                          highlightthickness=0, bg='red')
        self.assertEqual(f.grid_location(10, 10), (-1, -1))
        f.grid_configure()
        self.root.update()
        self.assertEqual(t.grid_location(-10, -10), (-1, -1))
        self.assertEqual(t.grid_location(-10, 0), (-1, 0))
        self.assertEqual(t.grid_location(-1, 0), (-1, 0))
        self.assertEqual(t.grid_location(0, -10), (0, -1))
        self.assertEqual(t.grid_location(0, -1), (0, -1))
        self.assertEqual(t.grid_location(0, 0), (0, 0))
        self.assertEqual(t.grid_location(200, 0), (0, 0))
        self.assertEqual(t.grid_location(201, 0), (1, 0))
        self.assertEqual(t.grid_location(0, 100), (0, 0))
        self.assertEqual(t.grid_location(0, 101), (0, 1))
        self.assertEqual(t.grid_location(201, 101), (1, 1))

    def test_grid_propagate(self):
        self.assertEqual(self.root.grid_propagate(), True)
        with self.assertRaises(TypeError):
            self.root.grid_propagate(False, False)
        self.root.grid_propagate(False)
        self.assertFalse(self.root.grid_propagate())
        f = tkinter.Frame(self.root, width=100, height=100, bg='red')
        f.grid_configure(row=0, column=0)
        self.root.update()
        self.assertEqual(f.winfo_width(), 100)
        self.assertEqual(f.winfo_height(), 100)
        f.grid_propagate(False)
        g = tkinter.Frame(self.root, width=75, height=85, bg='green')
        g.grid_configure(in_=f, row=0, column=0)
        self.root.update()
        self.assertEqual(f.winfo_width(), 100)
        self.assertEqual(f.winfo_height(), 100)
        f.grid_propagate(True)
        self.root.update()
        self.assertEqual(f.winfo_width(), 75)
        self.assertEqual(f.winfo_height(), 85)

    def test_grid_size(self):
        with self.assertRaises(TypeError):
            self.root.grid_size(0)
        self.assertEqual(self.root.grid_size(), (0, 0))
        f = tkinter.Scale(self.root)
        f.grid_configure(row=0, column=0)
        self.assertEqual(self.root.grid_size(), (1, 1))
        f.grid_configure(row=4, column=5)
        self.assertEqual(self.root.grid_size(), (6, 5))

    def test_grid_slaves(self):
        self.assertEqual(self.root.grid_slaves(), [])
        a = tkinter.Label(self.root)
        a.grid_configure(row=0, column=1)
        b = tkinter.Label(self.root)
        b.grid_configure(row=1, column=0)
        c = tkinter.Label(self.root)
        c.grid_configure(row=1, column=1)
        d = tkinter.Label(self.root)
        d.grid_configure(row=1, column=1)
        self.assertEqual(self.root.grid_slaves(), [d, c, b, a])
        self.assertEqual(self.root.grid_slaves(row=0), [a])
        self.assertEqual(self.root.grid_slaves(row=1), [d, c, b])
        self.assertEqual(self.root.grid_slaves(column=0), [b])
        self.assertEqual(self.root.grid_slaves(column=1), [d, c, a])
        self.assertEqual(self.root.grid_slaves(row=1, column=1), [d, c])


tests_gui = (
    PackTest, PlaceTest, GridTest,
)

if __name__ == '__main__':
    unittest.main()
