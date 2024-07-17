import unittest
import tkinter
from tkinter import TclError
import os
from test.support import requires

from test.test_tkinter.support import (requires_tk, tk_version,
                                  get_tk_patchlevel, widget_eq,
                                  AbstractDefaultRootTest)
from test.test_tkinter.widget_tests import (
    add_standard_options,
    AbstractWidgetTest, StandardOptionsTests, IntegerSizeTests, PixelSizeTests)

requires('gui')


EXPECTED_SCREEN_DISTANCE_ERRMSG = '(bad|expected) screen distance (but got )?"{}"'
EXPECTED_SCREEN_DISTANCE_OR_EMPTY_ERRMSG = '(bad|expected) screen distance (or "" but got )?"{}"'

def float_round(x):
    return float(round(x))


class AbstractToplevelTest(AbstractWidgetTest, PixelSizeTests):
    _conv_pad_pixels = False

    def test_configure_class(self):
        widget = self.create()
        self.assertEqual(widget['class'],
                         widget.__class__.__name__.title())
        self.checkInvalidParam(widget, 'class', 'Foo',
                errmsg="can't modify -class option after widget is created")
        widget2 = self.create(class_='Foo')
        self.assertEqual(widget2['class'], 'Foo')

    def test_configure_colormap(self):
        widget = self.create()
        self.assertEqual(widget['colormap'], '')
        self.checkInvalidParam(widget, 'colormap', 'new',
                errmsg="can't modify -colormap option after widget is created")
        widget2 = self.create(colormap='new')
        self.assertEqual(widget2['colormap'], 'new')

    def test_configure_container(self):
        widget = self.create()
        self.assertEqual(widget['container'], 0 if self.wantobjects else '0')
        self.checkInvalidParam(widget, 'container', 1,
                errmsg="can't modify -container option after widget is created")
        widget2 = self.create(container=True)
        self.assertEqual(widget2['container'], 1 if self.wantobjects else '1')

    def test_configure_visual(self):
        widget = self.create()
        self.assertEqual(widget['visual'], '')
        self.checkInvalidParam(widget, 'visual', 'default',
                errmsg="can't modify -visual option after widget is created")
        widget2 = self.create(visual='default')
        self.assertEqual(widget2['visual'], 'default')


@add_standard_options(StandardOptionsTests)
class ToplevelTest(AbstractToplevelTest, unittest.TestCase):
    OPTIONS = (
        'background', 'backgroundimage', 'borderwidth',
        'class', 'colormap', 'container', 'cursor', 'height',
        'highlightbackground', 'highlightcolor', 'highlightthickness',
        'menu', 'padx', 'pady', 'relief', 'screen',
        'takefocus', 'tile', 'use', 'visual', 'width',
    )

    def create(self, **kwargs):
        return tkinter.Toplevel(self.root, **kwargs)

    def test_configure_menu(self):
        widget = self.create()
        menu = tkinter.Menu(self.root)
        self.checkParam(widget, 'menu', menu, eq=widget_eq)
        self.checkParam(widget, 'menu', '')

    def test_configure_screen(self):
        widget = self.create()
        if widget._windowingsystem != 'x11':
            self.skipTest('Not using Tk for X11')
        self.assertEqual(widget['screen'], '')
        try:
            display = os.environ['DISPLAY']
        except KeyError:
            self.skipTest('No $DISPLAY set.')
        self.checkInvalidParam(widget, 'screen', display,
                errmsg="can't modify -screen option after widget is created")
        widget2 = self.create(screen=display)
        self.assertEqual(widget2['screen'], display)

    def test_configure_use(self):
        widget = self.create()
        self.assertEqual(widget['use'], '')
        parent = self.create(container=True)
        wid = hex(parent.winfo_id())
        with self.subTest(wid=wid):
            widget2 = self.create(use=wid)
            self.assertEqual(widget2['use'], wid)


@add_standard_options(StandardOptionsTests)
class FrameTest(AbstractToplevelTest, unittest.TestCase):
    OPTIONS = (
        'background', 'backgroundimage', 'borderwidth',
        'class', 'colormap', 'container', 'cursor', 'height',
        'highlightbackground', 'highlightcolor', 'highlightthickness',
        'padx', 'pady', 'relief', 'takefocus', 'tile', 'visual', 'width',
    )

    def create(self, **kwargs):
        return tkinter.Frame(self.root, **kwargs)


@add_standard_options(StandardOptionsTests)
class LabelFrameTest(AbstractToplevelTest, unittest.TestCase):
    OPTIONS = (
        'background', 'borderwidth',
        'class', 'colormap', 'container', 'cursor',
        'font', 'foreground', 'height',
        'highlightbackground', 'highlightcolor', 'highlightthickness',
        'labelanchor', 'labelwidget', 'padx', 'pady', 'relief',
        'takefocus', 'text', 'visual', 'width',
    )

    def create(self, **kwargs):
        return tkinter.LabelFrame(self.root, **kwargs)

    def test_configure_labelanchor(self):
        widget = self.create()
        self.checkEnumParam(widget, 'labelanchor',
                            'e', 'en', 'es', 'n', 'ne', 'nw',
                            's', 'se', 'sw', 'w', 'wn', 'ws')
        self.checkInvalidParam(widget, 'labelanchor', 'center')

    def test_configure_labelwidget(self):
        widget = self.create()
        label = tkinter.Label(self.root, text='Mupp', name='foo')
        self.checkParam(widget, 'labelwidget', label, expected='.foo')
        label.destroy()


class AbstractLabelTest(AbstractWidgetTest, IntegerSizeTests):
    _conv_pixels = False
    _clip_highlightthickness = tk_version >= (8, 7)
    _clip_pad = tk_version >= (8, 7)
    _clip_borderwidth = tk_version >= (8, 7)


@add_standard_options(StandardOptionsTests)
class LabelTest(AbstractLabelTest, unittest.TestCase):
    OPTIONS = (
        'activebackground', 'activeforeground', 'anchor',
        'background', 'bitmap', 'borderwidth', 'compound', 'cursor',
        'disabledforeground', 'font', 'foreground', 'height',
        'highlightbackground', 'highlightcolor', 'highlightthickness',
        'image', 'justify', 'padx', 'pady', 'relief', 'state',
        'takefocus', 'text', 'textvariable',
        'underline', 'width', 'wraplength',
    )

    def create(self, **kwargs):
        return tkinter.Label(self.root, **kwargs)


@add_standard_options(StandardOptionsTests)
class ButtonTest(AbstractLabelTest, unittest.TestCase):
    OPTIONS = (
        'activebackground', 'activeforeground', 'anchor',
        'background', 'bitmap', 'borderwidth',
        'command', 'compound', 'cursor', 'default',
        'disabledforeground', 'font', 'foreground', 'height',
        'highlightbackground', 'highlightcolor', 'highlightthickness',
        'image', 'justify', 'overrelief', 'padx', 'pady', 'relief',
        'repeatdelay', 'repeatinterval',
        'state', 'takefocus', 'text', 'textvariable',
        'underline', 'width', 'wraplength')

    def create(self, **kwargs):
        return tkinter.Button(self.root, **kwargs)

    def test_configure_default(self):
        widget = self.create()
        self.checkEnumParam(widget, 'default', 'active', 'disabled', 'normal')


@add_standard_options(StandardOptionsTests)
class CheckbuttonTest(AbstractLabelTest, unittest.TestCase):
    OPTIONS = (
        'activebackground', 'activeforeground', 'anchor',
        'background', 'bitmap', 'borderwidth',
        'command', 'compound', 'cursor',
        'disabledforeground', 'font', 'foreground', 'height',
        'highlightbackground', 'highlightcolor', 'highlightthickness',
        'image', 'indicatoron', 'justify',
        'offrelief', 'offvalue', 'onvalue', 'overrelief',
        'padx', 'pady', 'relief', 'selectcolor', 'selectimage', 'state',
        'takefocus', 'text', 'textvariable',
        'tristateimage', 'tristatevalue',
        'underline', 'variable', 'width', 'wraplength',
    )

    def create(self, **kwargs):
        return tkinter.Checkbutton(self.root, **kwargs)


    def test_configure_offvalue(self):
        widget = self.create()
        self.checkParams(widget, 'offvalue', 1, 2.3, '', 'any string')

    def test_configure_onvalue(self):
        widget = self.create()
        self.checkParams(widget, 'onvalue', 1, 2.3, '', 'any string')

    def test_unique_variables(self):
        frames = []
        buttons = []
        for i in range(2):
            f = tkinter.Frame(self.root)
            f.pack()
            frames.append(f)
            for j in 'AB':
                b = tkinter.Checkbutton(f, text=j)
                b.pack()
                buttons.append(b)
        variables = [str(b['variable']) for b in buttons]
        self.assertEqual(len(set(variables)), 4, variables)

    def test_same_name(self):
        f1 = tkinter.Frame(self.root)
        f2 = tkinter.Frame(self.root)
        b1 = tkinter.Checkbutton(f1, name='test', text='Test1')
        b2 = tkinter.Checkbutton(f2, name='test', text='Test2')

        v = tkinter.IntVar(self.root, name='test')
        b1.select()
        self.assertEqual(v.get(), 1)
        b2.deselect()
        self.assertEqual(v.get(), 0)


@add_standard_options(StandardOptionsTests)
class RadiobuttonTest(AbstractLabelTest, unittest.TestCase):
    OPTIONS = (
        'activebackground', 'activeforeground', 'anchor',
        'background', 'bitmap', 'borderwidth',
        'command', 'compound', 'cursor',
        'disabledforeground', 'font', 'foreground', 'height',
        'highlightbackground', 'highlightcolor', 'highlightthickness',
        'image', 'indicatoron', 'justify', 'offrelief', 'overrelief',
        'padx', 'pady', 'relief', 'selectcolor', 'selectimage', 'state',
        'takefocus', 'text', 'textvariable',
        'tristateimage', 'tristatevalue',
        'underline', 'value', 'variable', 'width', 'wraplength',
    )

    def create(self, **kwargs):
        return tkinter.Radiobutton(self.root, **kwargs)

    def test_configure_value(self):
        widget = self.create()
        self.checkParams(widget, 'value', 1, 2.3, '', 'any string')


@add_standard_options(StandardOptionsTests)
class MenubuttonTest(AbstractLabelTest, unittest.TestCase):
    OPTIONS = (
        'activebackground', 'activeforeground', 'anchor',
        'background', 'bitmap', 'borderwidth',
        'compound', 'cursor', 'direction',
        'disabledforeground', 'font', 'foreground', 'height',
        'highlightbackground', 'highlightcolor', 'highlightthickness',
        'image', 'indicatoron', 'justify', 'menu',
        'padx', 'pady', 'relief', 'state',
        'takefocus', 'text', 'textvariable',
        'underline', 'width', 'wraplength',
    )
    _conv_pixels = round
    _clip_highlightthickness = True
    _clip_pad = True
    _clip_borderwidth = False

    def create(self, **kwargs):
        return tkinter.Menubutton(self.root, **kwargs)

    def test_configure_direction(self):
        widget = self.create()
        self.checkEnumParam(widget, 'direction',
                'above', 'below', 'flush', 'left', 'right')

    def test_configure_height(self):
        widget = self.create()
        self.checkIntegerParam(widget, 'height', 100, -100, 0, conv=str)

    def test_configure_image(self):
        widget = self.create()
        image = tkinter.PhotoImage(master=self.root, name='image1')
        self.checkParam(widget, 'image', image, conv=str)
        errmsg = 'image "spam" doesn\'t exist'
        with self.assertRaises(tkinter.TclError) as cm:
            widget['image'] = 'spam'
        if errmsg is not None:
            self.assertEqual(str(cm.exception), errmsg)
        with self.assertRaises(tkinter.TclError) as cm:
            widget.configure({'image': 'spam'})
        if errmsg is not None:
            self.assertEqual(str(cm.exception), errmsg)

    def test_configure_menu(self):
        widget = self.create()
        menu = tkinter.Menu(widget, name='menu')
        self.checkParam(widget, 'menu', menu, eq=widget_eq)
        menu.destroy()

    def test_configure_width(self):
        widget = self.create()
        self.checkIntegerParam(widget, 'width', 402, -402, 0, conv=str)


class OptionMenuTest(MenubuttonTest, unittest.TestCase):

    def create(self, default='b', values=('a', 'b', 'c'), **kwargs):
        return tkinter.OptionMenu(self.root, None, default, *values, **kwargs)

    def test_bad_kwarg(self):
        with self.assertRaisesRegex(TclError, r"^unknown option -image$"):
            tkinter.OptionMenu(self.root, None, 'b', image='')


@add_standard_options(IntegerSizeTests, StandardOptionsTests)
class EntryTest(AbstractWidgetTest, unittest.TestCase):
    OPTIONS = (
        'background', 'borderwidth', 'cursor',
        'disabledbackground', 'disabledforeground',
        'exportselection', 'font', 'foreground',
        'highlightbackground', 'highlightcolor', 'highlightthickness',
        'insertbackground', 'insertborderwidth',
        'insertofftime', 'insertontime', 'insertwidth',
        'invalidcommand', 'justify', 'placeholder', 'placeholderforeground',
        'readonlybackground', 'relief',
        'selectbackground', 'selectborderwidth', 'selectforeground',
        'show', 'state', 'takefocus', 'textvariable',
        'validate', 'validatecommand', 'width', 'xscrollcommand',
    )

    def create(self, **kwargs):
        return tkinter.Entry(self.root, **kwargs)

    def test_configure_disabledbackground(self):
        widget = self.create()
        self.checkColorParam(widget, 'disabledbackground')

    def test_configure_insertborderwidth(self):
        widget = self.create(insertwidth=100)
        self.checkPixelsParam(widget, 'insertborderwidth',
                              0, 1.3, 2.6, 6, -2, '10p')
        # insertborderwidth is bounded above by a half of insertwidth.
        self.checkParam(widget, 'insertborderwidth', 60, expected=100//2)

    def test_configure_insertwidth(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'insertwidth', 1.3, 3.6, '10p')
        self.checkParam(widget, 'insertwidth', 0.1, expected=2)
        self.checkParam(widget, 'insertwidth', -2, expected=2)
        self.checkParam(widget, 'insertwidth', 0.9, expected=1)

    def test_configure_invalidcommand(self):
        widget = self.create()
        self.checkCommandParam(widget, 'invalidcommand')
        self.checkCommandParam(widget, 'invcmd')

    def test_configure_readonlybackground(self):
        widget = self.create()
        self.checkColorParam(widget, 'readonlybackground')

    def test_configure_show(self):
        widget = self.create()
        self.checkParam(widget, 'show', '*')
        self.checkParam(widget, 'show', '')
        self.checkParam(widget, 'show', ' ')

    def test_configure_state(self):
        widget = self.create()
        self.checkEnumParam(widget, 'state',
                            'disabled', 'normal', 'readonly')

    def test_configure_validate(self):
        widget = self.create()
        self.checkEnumParam(widget, 'validate',
                'all', 'key', 'focus', 'focusin', 'focusout', 'none')

    def test_configure_validatecommand(self):
        widget = self.create()
        self.checkCommandParam(widget, 'validatecommand')
        self.checkCommandParam(widget, 'vcmd')

    def test_selection_methods(self):
        widget = self.create()
        widget.insert(0, '12345')
        self.assertFalse(widget.selection_present())
        widget.selection_range(0, 'end')
        self.assertEqual(widget.selection_get(), '12345')
        self.assertTrue(widget.selection_present())
        widget.selection_from(1)
        widget.selection_to(2)
        self.assertEqual(widget.selection_get(), '2')
        widget.selection_range(3, 4)
        self.assertEqual(widget.selection_get(), '4')
        widget.selection_clear()
        self.assertFalse(widget.selection_present())
        widget.selection_range(0, 'end')
        widget.selection_adjust(4)
        self.assertEqual(widget.selection_get(), '1234')
        widget.selection_adjust(1)
        self.assertEqual(widget.selection_get(), '234')
        widget.selection_adjust(5)
        self.assertEqual(widget.selection_get(), '2345')
        widget.selection_adjust(0)
        self.assertEqual(widget.selection_get(), '12345')
        widget.selection_adjust(0)


@add_standard_options(StandardOptionsTests)
class SpinboxTest(EntryTest, unittest.TestCase):
    OPTIONS = (
        'activebackground', 'background', 'borderwidth',
        'buttonbackground', 'buttoncursor', 'buttondownrelief', 'buttonuprelief',
        'command', 'cursor', 'disabledbackground', 'disabledforeground',
        'exportselection', 'font', 'foreground', 'format', 'from',
        'highlightbackground', 'highlightcolor', 'highlightthickness',
        'increment',
        'insertbackground', 'insertborderwidth',
        'insertofftime', 'insertontime', 'insertwidth',
        'invalidcommand', 'justify', 'placeholder', 'placeholderforeground',
        'relief', 'readonlybackground', 'repeatdelay', 'repeatinterval',
        'selectbackground', 'selectborderwidth', 'selectforeground',
        'state', 'takefocus', 'textvariable', 'to',
        'validate', 'validatecommand', 'values',
        'width', 'wrap', 'xscrollcommand',
    )

    def create(self, **kwargs):
        return tkinter.Spinbox(self.root, **kwargs)

    test_configure_show = None

    def test_configure_buttonbackground(self):
        widget = self.create()
        self.checkColorParam(widget, 'buttonbackground')

    def test_configure_buttoncursor(self):
        widget = self.create()
        self.checkCursorParam(widget, 'buttoncursor')

    def test_configure_buttondownrelief(self):
        widget = self.create()
        self.checkReliefParam(widget, 'buttondownrelief')

    def test_configure_buttonuprelief(self):
        widget = self.create()
        self.checkReliefParam(widget, 'buttonuprelief')

    def test_configure_format(self):
        widget = self.create()
        self.checkParam(widget, 'format', '%2f')
        self.checkParam(widget, 'format', '%2.2f')
        self.checkParam(widget, 'format', '%.2f')
        self.checkParam(widget, 'format', '%2.f')
        self.checkInvalidParam(widget, 'format', '%2e-1f')
        self.checkInvalidParam(widget, 'format', '2.2')
        self.checkInvalidParam(widget, 'format', '%2.-2f')
        self.checkParam(widget, 'format', '%-2.02f')
        self.checkParam(widget, 'format', '% 2.02f')
        self.checkParam(widget, 'format', '% -2.200f')
        self.checkParam(widget, 'format', '%09.200f')
        self.checkInvalidParam(widget, 'format', '%d')

    def test_configure_from(self):
        widget = self.create()
        self.checkParam(widget, 'to', 100.0)
        self.checkFloatParam(widget, 'from', -10, 10.2, 11.7)
        if tk_version >= (8, 7):
            self.checkFloatParam(widget, 'from', 200, expected=100)
        else:
            self.checkInvalidParam(
                    widget, 'from', 200,
                    errmsg='-to value must be greater than -from value')

    def test_configure_increment(self):
        widget = self.create()
        self.checkFloatParam(widget, 'increment', -1, 1, 10.2, 12.8, 0)

    def test_configure_to(self):
        widget = self.create()
        self.checkParam(widget, 'from', -100.0)
        self.checkFloatParam(widget, 'to', -10, 10.2, 11.7)
        if tk_version >= (8, 7):
            self.checkFloatParam(widget, 'to', -200, expected=-100)
        else:
            self.checkInvalidParam(
                    widget, 'to', -200,
                    errmsg='-to value must be greater than -from value')

    def test_configure_values(self):
        # XXX
        widget = self.create()
        self.assertEqual(widget['values'], '')
        self.checkParam(widget, 'values', 'mon tue wed thur')
        self.checkParam(widget, 'values', ('mon', 'tue', 'wed', 'thur'),
                        expected='mon tue wed thur')
        self.checkParam(widget, 'values', (42, 3.14, '', 'any string'),
                        expected='42 3.14 {} {any string}')
        self.checkParam(widget, 'values', '')

    def test_configure_wrap(self):
        widget = self.create()
        self.checkBooleanParam(widget, 'wrap')

    def test_bbox(self):
        widget = self.create()
        self.assertIsBoundingBox(widget.bbox(0))
        self.assertRaises(tkinter.TclError, widget.bbox, 'noindex')
        self.assertRaises(tkinter.TclError, widget.bbox, None)
        self.assertRaises(TypeError, widget.bbox)
        self.assertRaises(TypeError, widget.bbox, 0, 1)

    def test_selection_methods(self):
        widget = self.create()
        widget.insert(0, '12345')
        self.assertFalse(widget.selection_present())
        widget.selection_range(0, 'end')
        self.assertEqual(widget.selection_get(), '12345')
        self.assertTrue(widget.selection_present())
        widget.selection_from(1)
        widget.selection_to(2)
        self.assertEqual(widget.selection_get(), '2')
        widget.selection_range(3, 4)
        self.assertEqual(widget.selection_get(), '4')
        widget.selection_clear()
        self.assertFalse(widget.selection_present())
        widget.selection_range(0, 'end')
        widget.selection_adjust(4)
        self.assertEqual(widget.selection_get(), '1234')
        widget.selection_adjust(1)
        self.assertEqual(widget.selection_get(), '234')
        widget.selection_adjust(5)
        self.assertEqual(widget.selection_get(), '2345')
        widget.selection_adjust(0)
        self.assertEqual(widget.selection_get(), '12345')

    def test_selection_element(self):
        widget = self.create()
        self.assertEqual(widget.selection_element(), "none")
        widget.selection_element("buttonup")
        self.assertEqual(widget.selection_element(), "buttonup")
        widget.selection_element("buttondown")
        self.assertEqual(widget.selection_element(), "buttondown")


@add_standard_options(StandardOptionsTests)
class TextTest(AbstractWidgetTest, unittest.TestCase):
    OPTIONS = (
        'autoseparators', 'background', 'blockcursor', 'borderwidth',
        'cursor', 'endline', 'exportselection',
        'font', 'foreground', 'height',
        'highlightbackground', 'highlightcolor', 'highlightthickness',
        'inactiveselectbackground', 'insertbackground', 'insertborderwidth',
        'insertofftime', 'insertontime', 'insertunfocussed', 'insertwidth',
        'maxundo', 'padx', 'pady', 'relief',
        'selectbackground', 'selectborderwidth', 'selectforeground',
        'setgrid', 'spacing1', 'spacing2', 'spacing3', 'startline', 'state',
        'tabs', 'tabstyle', 'takefocus', 'undo', 'width', 'wrap',
        'xscrollcommand', 'yscrollcommand',
    )

    def create(self, **kwargs):
        return tkinter.Text(self.root, **kwargs)

    def test_configure_autoseparators(self):
        widget = self.create()
        self.checkBooleanParam(widget, 'autoseparators')

    def test_configure_blockcursor(self):
        widget = self.create()
        self.checkBooleanParam(widget, 'blockcursor')

    def test_configure_endline(self):
        widget = self.create()
        text = '\n'.join('Line %d' for i in range(100))
        widget.insert('end', text)
        self.checkParam(widget, 'endline', 200, expected='')
        self.checkParam(widget, 'endline', -10, expected='')
        self.checkInvalidParam(widget, 'endline', 'spam',
                errmsg='expected integer but got "spam"')
        self.checkParam(widget, 'endline', 50)
        self.checkParam(widget, 'startline', 15)
        self.checkInvalidParam(widget, 'endline', 10,
                errmsg='-startline must be less than or equal to -endline')

    def test_configure_height(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'height', 100, 101.2, 102.6, '3c')
        self.checkParam(widget, 'height', -100, expected=1)
        self.checkParam(widget, 'height', 0, expected=1)

    def test_configure_maxundo(self):
        widget = self.create()
        self.checkIntegerParam(widget, 'maxundo', 0, 5, -1)

    def test_configure_inactiveselectbackground(self):
        widget = self.create()
        self.checkColorParam(widget, 'inactiveselectbackground')

    @requires_tk(8, 6)
    def test_configure_insertunfocussed(self):
        widget = self.create()
        self.checkEnumParam(widget, 'insertunfocussed',
                            'hollow', 'none', 'solid')

    def test_configure_selectborderwidth(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'selectborderwidth',
                              1.3, 2.6, -2, '10p', conv=False)

    def test_configure_spacing1(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'spacing1', 20, 21.4, 22.6, '0.5c')
        self.checkParam(widget, 'spacing1', -5, expected=0)

    def test_configure_spacing2(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'spacing2', 5, 6.4, 7.6, '0.1c')
        self.checkParam(widget, 'spacing2', -1, expected=0)

    def test_configure_spacing3(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'spacing3', 20, 21.4, 22.6, '0.5c')
        self.checkParam(widget, 'spacing3', -10, expected=0)

    def test_configure_startline(self):
        widget = self.create()
        text = '\n'.join('Line %d' for i in range(100))
        widget.insert('end', text)
        self.checkParam(widget, 'startline', 200, expected='')
        self.checkParam(widget, 'startline', -10, expected='')
        self.checkInvalidParam(widget, 'startline', 'spam',
                errmsg='expected integer but got "spam"')
        self.checkParam(widget, 'startline', 10)
        self.checkParam(widget, 'endline', 50)
        self.checkInvalidParam(widget, 'startline', 70,
                errmsg='-startline must be less than or equal to -endline')

    def test_configure_state(self):
        widget = self.create()
        self.checkEnumParam(widget, 'state', 'disabled', 'normal')

    def test_configure_tabs(self):
        widget = self.create()
        self.checkParam(widget, 'tabs', (10.2, 20.7, '1i', '2i'))
        self.checkParam(widget, 'tabs', '10.2 20.7 1i 2i',
                        expected=(10.2, 20.7, '1i', '2i')
                                 if get_tk_patchlevel(self.root) >= (8, 6, 14)
                                 else ('10.2', '20.7', '1i', '2i'))
        self.checkParam(widget, 'tabs', '2c left 4c 6c center',
                        expected=('2c', 'left', '4c', '6c', 'center'))
        self.checkInvalidParam(widget, 'tabs', 'spam',
                errmsg=EXPECTED_SCREEN_DISTANCE_ERRMSG.format('spam'))

    def test_configure_tabstyle(self):
        widget = self.create()
        self.checkEnumParam(widget, 'tabstyle', 'tabular', 'wordprocessor')

    def test_configure_undo(self):
        widget = self.create()
        self.checkBooleanParam(widget, 'undo')

    def test_configure_width(self):
        widget = self.create()
        self.checkIntegerParam(widget, 'width', 402)
        self.checkParam(widget, 'width', -402, expected=1)
        self.checkParam(widget, 'width', 0, expected=1)

    def test_configure_wrap(self):
        widget = self.create()
        self.checkEnumParam(widget, 'wrap', 'char', 'none', 'word')

    def test_bbox(self):
        widget = self.create()
        self.assertIsBoundingBox(widget.bbox('1.1'))
        self.assertIsNone(widget.bbox('end'))
        self.assertRaises(tkinter.TclError, widget.bbox, 'noindex')
        self.assertRaises(tkinter.TclError, widget.bbox, None)
        self.assertRaises(TypeError, widget.bbox)
        self.assertRaises(TypeError, widget.bbox, '1.1', 'end')


@add_standard_options(PixelSizeTests, StandardOptionsTests)
class CanvasTest(AbstractWidgetTest, unittest.TestCase):
    OPTIONS = (
        'background', 'borderwidth',
        'closeenough', 'confine', 'cursor', 'height',
        'highlightbackground', 'highlightcolor', 'highlightthickness',
        'insertbackground', 'insertborderwidth',
        'insertofftime', 'insertontime', 'insertwidth',
        'offset', 'relief', 'scrollregion',
        'selectbackground', 'selectborderwidth', 'selectforeground',
        'state', 'takefocus',
        'xscrollcommand', 'xscrollincrement',
        'yscrollcommand', 'yscrollincrement', 'width',
    )

    _conv_pixels = round
    _stringify = True

    def create(self, **kwargs):
        return tkinter.Canvas(self.root, **kwargs)

    def test_configure_closeenough(self):
        widget = self.create()
        self.checkFloatParam(widget, 'closeenough', 24, 2.4, 3.6, -3,
                             conv=float)

    def test_configure_confine(self):
        widget = self.create()
        self.checkBooleanParam(widget, 'confine')

    def test_configure_offset(self):
        widget = self.create()
        self.assertEqual(widget['offset'], '0,0')
        self.checkParams(widget, 'offset',
                'n', 'ne', 'e', 'se', 's', 'sw', 'w', 'nw', 'center')
        self.checkParam(widget, 'offset', '10,20')
        self.checkParam(widget, 'offset', '#5,6')
        self.checkInvalidParam(widget, 'offset', 'spam')

    def test_configure_scrollregion(self):
        widget = self.create()
        self.checkParam(widget, 'scrollregion', '0 0 200 150')
        self.checkParam(widget, 'scrollregion', (0, 0, 200, 150),
                        expected='0 0 200 150')
        self.checkParam(widget, 'scrollregion', '')
        self.checkInvalidParam(widget, 'scrollregion', 'spam',
                               errmsg='bad scrollRegion "spam"')
        self.checkInvalidParam(widget, 'scrollregion', (0, 0, 200, 'spam'))
        self.checkInvalidParam(widget, 'scrollregion', (0, 0, 200))
        self.checkInvalidParam(widget, 'scrollregion', (0, 0, 200, 150, 0))

    def test_configure_state(self):
        widget = self.create()
        self.checkEnumParam(widget, 'state', 'disabled', 'normal',
                errmsg='bad state value "{}": must be normal or disabled')

    def test_configure_xscrollincrement(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'xscrollincrement',
                              40, 0, 41.2, 43.6, -40, '0.5i')

    def test_configure_yscrollincrement(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'yscrollincrement',
                              10, 0, 11.2, 13.6, -10, '0.1i')

    def _test_option_joinstyle(self, c, factory):
        for joinstyle in 'bevel', 'miter', 'round':
            i = factory(joinstyle=joinstyle)
            self.assertEqual(c.itemcget(i, 'joinstyle'), joinstyle)
        self.assertRaises(TclError, factory, joinstyle='spam')

    def _test_option_smooth(self, c, factory):
        for smooth in 1, True, '1', 'true', 'yes', 'on':
            i = factory(smooth=smooth)
            self.assertEqual(c.itemcget(i, 'smooth'), 'true')
        for smooth in 0, False, '0', 'false', 'no', 'off':
            i = factory(smooth=smooth)
            self.assertEqual(c.itemcget(i, 'smooth'), '0')
        i = factory(smooth=True, splinestep=30)
        self.assertEqual(c.itemcget(i, 'smooth'), 'true')
        self.assertEqual(c.itemcget(i, 'splinestep'), '30')
        i = factory(smooth='raw', splinestep=30)
        self.assertEqual(c.itemcget(i, 'smooth'), 'raw')
        self.assertEqual(c.itemcget(i, 'splinestep'), '30')
        self.assertRaises(TclError, factory, smooth='spam')

    def test_create_rectangle(self):
        c = self.create()
        i1 = c.create_rectangle(20, 30, 60, 10)
        self.assertEqual(c.coords(i1), [20.0, 10.0, 60.0, 30.0])
        self.assertEqual(c.bbox(i1), (19, 9, 61, 31))

        i2 = c.create_rectangle([21, 31, 61, 11])
        self.assertEqual(c.coords(i2), [21.0, 11.0, 61.0, 31.0])
        self.assertEqual(c.bbox(i2), (20, 10, 62, 32))

        i3 = c.create_rectangle((22, 32), (62, 12))
        self.assertEqual(c.coords(i3), [22.0, 12.0, 62.0, 32.0])
        self.assertEqual(c.bbox(i3), (21, 11, 63, 33))

        i4 = c.create_rectangle([(23, 33), (63, 13)])
        self.assertEqual(c.coords(i4), [23.0, 13.0, 63.0, 33.0])
        self.assertEqual(c.bbox(i4), (22, 12, 64, 34))

        self.assertRaises(TclError, c.create_rectangle, 20, 30, 60)
        self.assertRaises(TclError, c.create_rectangle, [20, 30, 60])
        self.assertRaises(TclError, c.create_rectangle, 20, 30, 40, 50, 60, 10)
        self.assertRaises(TclError, c.create_rectangle, [20, 30, 40, 50, 60, 10])
        self.assertRaises(TclError, c.create_rectangle, 20, 30)
        self.assertRaises(TclError, c.create_rectangle, [20, 30])
        self.assertRaises(IndexError, c.create_rectangle)
        self.assertRaises(IndexError, c.create_rectangle, [])

    def test_create_line(self):
        c = self.create()
        i1 = c.create_line(20, 30, 40, 50, 60, 10)
        self.assertEqual(c.coords(i1), [20.0, 30.0, 40.0, 50.0, 60.0, 10.0])
        self.assertEqual(c.bbox(i1), (18, 8, 62, 52))
        self.assertEqual(c.itemcget(i1, 'arrow'), 'none')
        self.assertEqual(c.itemcget(i1, 'arrowshape'), '8 10 3')
        self.assertEqual(c.itemcget(i1, 'capstyle'), 'butt')
        self.assertEqual(c.itemcget(i1, 'joinstyle'), 'round')
        self.assertEqual(c.itemcget(i1, 'smooth'), '0')
        self.assertEqual(c.itemcget(i1, 'splinestep'), '12')

        i2 = c.create_line([21, 31, 41, 51, 61, 11])
        self.assertEqual(c.coords(i2), [21.0, 31.0, 41.0, 51.0, 61.0, 11.0])
        self.assertEqual(c.bbox(i2), (19, 9, 63, 53))

        i3 = c.create_line((22, 32), (42, 52), (62, 12))
        self.assertEqual(c.coords(i3), [22.0, 32.0, 42.0, 52.0, 62.0, 12.0])
        self.assertEqual(c.bbox(i3), (20, 10, 64, 54))

        i4 = c.create_line([(23, 33), (43, 53), (63, 13)])
        self.assertEqual(c.coords(i4), [23.0, 33.0, 43.0, 53.0, 63.0, 13.0])
        self.assertEqual(c.bbox(i4), (21, 11, 65, 55))

        self.assertRaises(TclError, c.create_line, 20, 30, 60)
        self.assertRaises(TclError, c.create_line, [20, 30, 60])
        self.assertRaises(TclError, c.create_line, 20, 30)
        self.assertRaises(TclError, c.create_line, [20, 30])
        self.assertRaises(IndexError, c.create_line)
        self.assertRaises(IndexError, c.create_line, [])

        for arrow in 'none', 'first', 'last', 'both':
            i = c.create_line(20, 30, 60, 10, arrow=arrow)
            self.assertEqual(c.itemcget(i, 'arrow'), arrow)
        i = c.create_line(20, 30, 60, 10, arrow='first', arrowshape=[10, 15, 5])
        self.assertEqual(c.itemcget(i, 'arrowshape'), '10 15 5')
        self.assertRaises(TclError, c.create_line, 20, 30, 60, 10, arrow='spam')

        for capstyle in 'butt', 'projecting', 'round':
            i = c.create_line(20, 30, 60, 10, capstyle=capstyle)
            self.assertEqual(c.itemcget(i, 'capstyle'), capstyle)
        self.assertRaises(TclError, c.create_line, 20, 30, 60, 10, capstyle='spam')

        self._test_option_joinstyle(c,
                lambda **kwargs: c.create_line(20, 30, 40, 50, 60, 10, **kwargs))
        self._test_option_smooth(c,
                lambda **kwargs: c.create_line(20, 30, 60, 10, **kwargs))

    def test_create_polygon(self):
        c = self.create()
        tk87 = tk_version >= (8, 7)
        # In Tk < 8.7 polygons are filled, but has no outline by default.
        # This affects its size, so always explicitly specify outline.
        i1 = c.create_polygon(20, 30, 40, 50, 60, 10, outline='red')
        self.assertEqual(c.coords(i1), [20.0, 30.0, 40.0, 50.0, 60.0, 10.0])
        self.assertEqual(c.bbox(i1), (18, 8, 62, 52))
        self.assertEqual(c.itemcget(i1, 'joinstyle'), 'round')
        self.assertEqual(c.itemcget(i1, 'smooth'), '0')
        self.assertEqual(c.itemcget(i1, 'splinestep'), '12')

        i2 = c.create_polygon([21, 31, 41, 51, 61, 11], outline='red')
        self.assertEqual(c.coords(i2), [21.0, 31.0, 41.0, 51.0, 61.0, 11.0])
        self.assertEqual(c.bbox(i2), (19, 9, 63, 53))

        i3 = c.create_polygon((22, 32), (42, 52), (62, 12), outline='red')
        self.assertEqual(c.coords(i3), [22.0, 32.0, 42.0, 52.0, 62.0, 12.0])
        self.assertEqual(c.bbox(i3), (20, 10, 64, 54))

        i4 = c.create_polygon([(23, 33), (43, 53), (63, 13)], outline='red')
        self.assertEqual(c.coords(i4), [23.0, 33.0, 43.0, 53.0, 63.0, 13.0])
        self.assertEqual(c.bbox(i4), (21, 11, 65, 55))

        self.assertRaises(TclError, c.create_polygon, 20, 30, 60)
        self.assertRaises(TclError, c.create_polygon, [20, 30, 60])
        self.assertRaises(IndexError, c.create_polygon)
        self.assertRaises(IndexError, c.create_polygon, [])

        self._test_option_joinstyle(c,
                lambda **kwargs: c.create_polygon(20, 30, 40, 50, 60, 10, **kwargs))
        self._test_option_smooth(c,
                lambda **kwargs: c.create_polygon(20, 30, 40, 50, 60, 10, **kwargs))

    def test_coords(self):
        c = self.create()
        i = c.create_line(20, 30, 40, 50, 60, 10, tags='x')
        self.assertEqual(c.coords(i), [20.0, 30.0, 40.0, 50.0, 60.0, 10.0])
        self.assertEqual(c.coords('x'), [20.0, 30.0, 40.0, 50.0, 60.0, 10.0])
        self.assertEqual(c.bbox(i), (18, 8, 62, 52))

        c.coords(i, 50, 60, 70, 80, 90, 40)
        self.assertEqual(c.coords(i), [50.0, 60.0, 70.0, 80.0, 90.0, 40.0])
        self.assertEqual(c.bbox(i), (48, 38, 92, 82))

        c.coords(i, [21, 31, 41, 51, 61, 11])
        self.assertEqual(c.coords(i), [21.0, 31.0, 41.0, 51.0, 61.0, 11.0])

        c.coords(i, (22, 32), (42, 52), (62, 12))
        self.assertEqual(c.coords(i), [22.0, 32.0, 42.0, 52.0, 62.0, 12.0])

        c.coords(i, [(23, 33), (43, 53), (63, 13)])
        self.assertEqual(c.coords(i), [23.0, 33.0, 43.0, 53.0, 63.0, 13.0])

        c.coords(i, 20, 30, 60, 10)
        self.assertEqual(c.coords(i), [20.0, 30.0, 60.0, 10.0])
        self.assertEqual(c.bbox(i), (18, 8, 62, 32))

        self.assertRaises(TclError, c.coords, i, 20, 30, 60)
        self.assertRaises(TclError, c.coords, i, [20, 30, 60])
        self.assertRaises(TclError, c.coords, i, 20, 30)
        self.assertRaises(TclError, c.coords, i, [20, 30])

        c.coords(i, '20', '30c', '60i', '10p')
        coords = c.coords(i)
        self.assertIsInstance(coords, list)
        self.assertEqual(len(coords), 4)
        self.assertEqual(coords[0], 20)
        for i in range(4):
            self.assertIsInstance(coords[i], float)

    @requires_tk(8, 6)
    def test_moveto(self):
        widget = self.create()
        i1 = widget.create_rectangle(1, 1, 20, 20, tags='group')
        i2 = widget.create_rectangle(30, 30, 50, 70, tags='group')
        x1, y1, _, _ = widget.bbox(i1)
        x2, y2, _, _ = widget.bbox(i2)
        widget.moveto('group', 200, 100)
        x1_2, y1_2, _, _ = widget.bbox(i1)
        x2_2, y2_2, _, _ = widget.bbox(i2)
        self.assertEqual(x1_2, 200)
        self.assertEqual(y1_2, 100)
        self.assertEqual(x2 - x1, x2_2 - x1_2)
        self.assertEqual(y2 - y1, y2_2 - y1_2)
        widget.tag_lower(i2, i1)
        widget.moveto('group', y=50)
        x1_3, y1_3, _, _ = widget.bbox(i1)
        x2_3, y2_3, _, _ = widget.bbox(i2)
        self.assertEqual(y2_3, 50)
        self.assertEqual(x2_3, x2_2)
        self.assertEqual(x2_2 - x1_2, x2_3 - x1_3)
        self.assertEqual(y2_2 - y1_2, y2_3 - y1_3)


@add_standard_options(IntegerSizeTests, StandardOptionsTests)
class ListboxTest(AbstractWidgetTest, unittest.TestCase):
    OPTIONS = (
        'activestyle', 'background', 'borderwidth', 'cursor',
        'disabledforeground', 'exportselection',
        'font', 'foreground', 'height',
        'highlightbackground', 'highlightcolor', 'highlightthickness',
        'justify', 'listvariable', 'relief',
        'selectbackground', 'selectborderwidth', 'selectforeground',
        'selectmode', 'setgrid', 'state',
        'takefocus', 'width', 'xscrollcommand', 'yscrollcommand',
    )

    def create(self, **kwargs):
        return tkinter.Listbox(self.root, **kwargs)

    def test_configure_activestyle(self):
        widget = self.create()
        self.checkEnumParam(widget, 'activestyle',
                            'dotbox', 'none', 'underline')

    test_configure_justify = requires_tk(8, 6, 5)(StandardOptionsTests.test_configure_justify)

    def test_configure_listvariable(self):
        widget = self.create()
        var = tkinter.DoubleVar(self.root)
        self.checkVariableParam(widget, 'listvariable', var)

    def test_configure_selectmode(self):
        widget = self.create()
        self.checkParam(widget, 'selectmode', 'single')
        self.checkParam(widget, 'selectmode', 'browse')
        self.checkParam(widget, 'selectmode', 'multiple')
        self.checkParam(widget, 'selectmode', 'extended')

    def test_configure_state(self):
        widget = self.create()
        self.checkEnumParam(widget, 'state', 'disabled', 'normal')

    def test_itemconfigure(self):
        widget = self.create()
        with self.assertRaisesRegex(TclError, 'item number "0" out of range'):
            widget.itemconfigure(0)
        colors = 'red orange yellow green blue white violet'.split()
        widget.insert('end', *colors)
        for i, color in enumerate(colors):
            widget.itemconfigure(i, background=color)
        with self.assertRaises(TypeError):
            widget.itemconfigure()
        with self.assertRaisesRegex(TclError, 'bad listbox index "red"'):
            widget.itemconfigure('red')
        if get_tk_patchlevel(self.root) >= (8, 6, 14):
            prefix = ('background', '', '', '')
        else:
            prefix = ('background', 'background', 'Background', '')
        self.assertEqual(widget.itemconfigure(0, 'background'),
                         (*prefix, 'red'))
        self.assertEqual(widget.itemconfigure('end', 'background'),
                         (*prefix, 'violet'))
        self.assertEqual(widget.itemconfigure('@0,0', 'background'),
                         (*prefix, 'red'))

        d = widget.itemconfigure(0)
        self.assertIsInstance(d, dict)
        for k, v in d.items():
            self.assertIn(len(v), (2, 5))
            if len(v) == 5:
                self.assertEqual(v, widget.itemconfigure(0, k))
                self.assertEqual(v[4], widget.itemcget(0, k))

    def check_itemconfigure(self, name, value):
        widget = self.create()
        widget.insert('end', 'a', 'b', 'c', 'd')
        widget.itemconfigure(0, **{name: value})
        self.assertEqual(widget.itemconfigure(0, name)[4], value)
        self.assertEqual(widget.itemcget(0, name), value)
        with self.assertRaisesRegex(TclError, 'unknown color name "spam"'):
            widget.itemconfigure(0, **{name: 'spam'})

    def test_itemconfigure_background(self):
        self.check_itemconfigure('background', '#ff0000')

    def test_itemconfigure_bg(self):
        self.check_itemconfigure('bg', '#ff0000')

    def test_itemconfigure_fg(self):
        self.check_itemconfigure('fg', '#110022')

    def test_itemconfigure_foreground(self):
        self.check_itemconfigure('foreground', '#110022')

    def test_itemconfigure_selectbackground(self):
        self.check_itemconfigure('selectbackground', '#110022')

    def test_itemconfigure_selectforeground(self):
        self.check_itemconfigure('selectforeground', '#654321')

    def test_box(self):
        lb = self.create()
        lb.insert(0, *('el%d' % i for i in range(8)))
        lb.pack()
        self.assertIsBoundingBox(lb.bbox(0))
        self.assertIsNone(lb.bbox(-1))
        self.assertIsNone(lb.bbox(10))
        self.assertRaises(TclError, lb.bbox, 'noindex')
        self.assertRaises(TclError, lb.bbox, None)
        self.assertRaises(TypeError, lb.bbox)
        self.assertRaises(TypeError, lb.bbox, 0, 1)

    def test_curselection(self):
        lb = self.create()
        lb.insert(0, *('el%d' % i for i in range(8)))
        lb.selection_clear(0, tkinter.END)
        lb.selection_set(2, 4)
        lb.selection_set(6)
        self.assertEqual(lb.curselection(), (2, 3, 4, 6))
        self.assertRaises(TypeError, lb.curselection, 0)

    def test_get(self):
        lb = self.create()
        lb.insert(0, *('el%d' % i for i in range(8)))
        self.assertEqual(lb.get(0), 'el0')
        self.assertEqual(lb.get(3), 'el3')
        self.assertEqual(lb.get('end'), 'el7')
        self.assertEqual(lb.get(8), '')
        self.assertEqual(lb.get(-1), '')
        self.assertEqual(lb.get(3, 5), ('el3', 'el4', 'el5'))
        self.assertEqual(lb.get(5, 'end'), ('el5', 'el6', 'el7'))
        self.assertEqual(lb.get(5, 0), ())
        self.assertEqual(lb.get(0, 0), ('el0',))
        self.assertRaises(TclError, lb.get, 'noindex')
        self.assertRaises(TclError, lb.get, None)
        self.assertRaises(TypeError, lb.get)
        self.assertRaises(TclError, lb.get, 'end', 'noindex')
        self.assertRaises(TypeError, lb.get, 1, 2, 3)
        self.assertRaises(TclError, lb.get, 2.4)


@add_standard_options(PixelSizeTests, StandardOptionsTests)
class ScaleTest(AbstractWidgetTest, unittest.TestCase):
    OPTIONS = (
        'activebackground', 'background', 'bigincrement', 'borderwidth',
        'command', 'cursor', 'digits', 'font', 'foreground', 'from',
        'highlightbackground', 'highlightcolor', 'highlightthickness',
        'label', 'length', 'orient', 'relief',
        'repeatdelay', 'repeatinterval',
        'resolution', 'showvalue', 'sliderlength', 'sliderrelief', 'state',
        'takefocus', 'tickinterval', 'to', 'troughcolor', 'variable', 'width',
    )
    default_orient = 'vertical'

    def create(self, **kwargs):
        return tkinter.Scale(self.root, **kwargs)

    def test_configure_bigincrement(self):
        widget = self.create()
        self.checkFloatParam(widget, 'bigincrement', 12.4, 23.6, -5)

    def test_configure_digits(self):
        widget = self.create()
        self.checkIntegerParam(widget, 'digits', 5, 0)

    def test_configure_from(self):
        widget = self.create()
        conv = float if get_tk_patchlevel(self.root) >= (8, 6, 10) else float_round
        self.checkFloatParam(widget, 'from', 100, 14.9, 15.1, conv=conv)

    def test_configure_label(self):
        widget = self.create()
        self.checkParam(widget, 'label', 'any string')
        self.checkParam(widget, 'label', '')

    def test_configure_length(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'length', 130, 131.2, 135.6, '5i')

    def test_configure_resolution(self):
        widget = self.create()
        self.checkFloatParam(widget, 'resolution', 4.2, 0, 6.7, -2)

    def test_configure_showvalue(self):
        widget = self.create()
        self.checkBooleanParam(widget, 'showvalue')

    def test_configure_sliderlength(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'sliderlength',
                              10, 11.2, 15.6, -3, '3m')

    def test_configure_sliderrelief(self):
        widget = self.create()
        self.checkReliefParam(widget, 'sliderrelief')

    def test_configure_tickinterval(self):
        widget = self.create()
        self.checkFloatParam(widget, 'tickinterval', 1, 4.3, 7.6, 0,
                             conv=float_round)
        self.checkParam(widget, 'tickinterval', -2, expected=2,
                        conv=float_round)

    def test_configure_to(self):
        widget = self.create()
        self.checkFloatParam(widget, 'to', 300, 14.9, 15.1, -10,
                             conv=float_round)


@add_standard_options(PixelSizeTests, StandardOptionsTests)
class ScrollbarTest(AbstractWidgetTest, unittest.TestCase):
    OPTIONS = (
        'activebackground', 'activerelief',
        'background', 'borderwidth',
        'command', 'cursor', 'elementborderwidth',
        'highlightbackground', 'highlightcolor', 'highlightthickness',
        'jump', 'orient', 'relief',
        'repeatdelay', 'repeatinterval',
        'takefocus', 'troughcolor', 'width',
    )
    _conv_pixels = round
    _stringify = True
    default_orient = 'vertical'

    def create(self, **kwargs):
        return tkinter.Scrollbar(self.root, **kwargs)

    def test_configure_elementborderwidth(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'elementborderwidth', 4.3, 5.6, '1m')
        expected = self._default_pixels if tk_version >= (8, 7) else -2
        self.checkParam(widget, 'elementborderwidth', -2, expected=expected)

    def test_configure_orient(self):
        widget = self.create()
        self.checkEnumParam(widget, 'orient', 'vertical', 'horizontal',
                            fullname='orientation', allow_empty=True)

    def test_activate(self):
        sb = self.create()
        for e in ('arrow1', 'slider', 'arrow2'):
            sb.activate(e)
            self.assertEqual(sb.activate(), e)
        sb.activate('')
        self.assertIsNone(sb.activate())
        self.assertRaises(TypeError, sb.activate, 'arrow1', 'arrow2')

    def test_set(self):
        sb = self.create()
        sb.set(0.2, 0.4)
        self.assertEqual(sb.get(), (0.2, 0.4))
        self.assertRaises(TclError, sb.set, 'abc', 'def')
        self.assertRaises(TclError, sb.set, 0.6, 'def')
        self.assertRaises(TclError, sb.set, 0.6, None)
        self.assertRaises(TypeError, sb.set, 0.6)
        self.assertRaises(TypeError, sb.set, 0.6, 0.7, 0.8)


@add_standard_options(StandardOptionsTests)
class PanedWindowTest(AbstractWidgetTest, unittest.TestCase):
    OPTIONS = (
        'background', 'borderwidth', 'cursor',
        'handlepad', 'handlesize', 'height',
        'opaqueresize', 'orient',
        'proxybackground', 'proxyborderwidth', 'proxyrelief',
        'relief',
        'sashcursor', 'sashpad', 'sashrelief', 'sashwidth',
        'showhandle', 'width',
    )
    default_orient = 'horizontal'

    def create(self, **kwargs):
        return tkinter.PanedWindow(self.root, **kwargs)

    def test_configure_handlepad(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'handlepad', 5, 6.4, 7.6, -3, '1m')

    def test_configure_handlesize(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'handlesize', 8, 9.4, 10.6, -3, '2m',
                              conv=False)

    def test_configure_height(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'height', 100, 101.2, 102.6, -100, 0, '1i',
                              conv=False)

    def test_configure_opaqueresize(self):
        widget = self.create()
        self.checkBooleanParam(widget, 'opaqueresize')

    @requires_tk(8, 6, 5)
    def test_configure_proxybackground(self):
        widget = self.create()
        self.checkColorParam(widget, 'proxybackground')

    @requires_tk(8, 6, 5)
    def test_configure_proxyborderwidth(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'proxyborderwidth',
                              0, 1.3, 2.9, 6, -2, '10p',
                              conv=False)

    @requires_tk(8, 6, 5)
    def test_configure_proxyrelief(self):
        widget = self.create()
        self.checkReliefParam(widget, 'proxyrelief',
                              allow_empty=(tk_version >= (8, 7)))

    def test_configure_sashcursor(self):
        widget = self.create()
        self.checkCursorParam(widget, 'sashcursor')

    def test_configure_sashpad(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'sashpad', 8, 1.3, 2.6, -2, '2m')

    def test_configure_sashrelief(self):
        widget = self.create()
        self.checkReliefParam(widget, 'sashrelief')

    def test_configure_sashwidth(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'sashwidth', 10, 11.1, 15.6, -3, '1m',
                              conv=False)

    def test_configure_showhandle(self):
        widget = self.create()
        self.checkBooleanParam(widget, 'showhandle')

    def test_configure_width(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'width', 402, 403.4, 404.6, -402, 0, '5i',
                              conv=False)

    def create2(self):
        p = self.create()
        b = tkinter.Button(p)
        c = tkinter.Button(p)
        p.add(b)
        p.add(c)
        return p, b, c

    def test_paneconfigure(self):
        p, b, c = self.create2()
        self.assertRaises(TypeError, p.paneconfigure)
        d = p.paneconfigure(b)
        self.assertIsInstance(d, dict)
        for k, v in d.items():
            self.assertEqual(len(v), 5)
            self.assertEqual(v, p.paneconfigure(b, k))
            self.assertEqual(v[4], p.panecget(b, k))

    def check_paneconfigure(self, p, b, name, value, expected):
        if not self.wantobjects:
            expected = str(expected)
        p.paneconfigure(b, **{name: value})
        self.assertEqual(p.paneconfigure(b, name)[4], expected)
        self.assertEqual(p.panecget(b, name), expected)

    def check_paneconfigure_bad(self, p, b, name, msg):
        with self.assertRaisesRegex(TclError, msg):
            p.paneconfigure(b, **{name: 'badValue'})

    def test_paneconfigure_after(self):
        p, b, c = self.create2()
        self.check_paneconfigure(p, b, 'after', c, str(c))
        self.check_paneconfigure_bad(p, b, 'after',
                                     'bad window path name "badValue"')

    def test_paneconfigure_before(self):
        p, b, c = self.create2()
        self.check_paneconfigure(p, b, 'before', c, str(c))
        self.check_paneconfigure_bad(p, b, 'before',
                                     'bad window path name "badValue"')

    def test_paneconfigure_height(self):
        p, b, c = self.create2()
        self.check_paneconfigure(p, b, 'height', 10, 10)
        self.check_paneconfigure_bad(p, b, 'height',
                EXPECTED_SCREEN_DISTANCE_OR_EMPTY_ERRMSG.format('badValue'))

    def test_paneconfigure_hide(self):
        p, b, c = self.create2()
        self.check_paneconfigure(p, b, 'hide', False, 0)
        self.check_paneconfigure_bad(p, b, 'hide',
                                     'expected boolean value but got "badValue"')

    def test_paneconfigure_minsize(self):
        p, b, c = self.create2()
        self.check_paneconfigure(p, b, 'minsize', 10, 10)
        self.check_paneconfigure_bad(p, b, 'minsize',
                EXPECTED_SCREEN_DISTANCE_ERRMSG.format('badValue'))

    def test_paneconfigure_padx(self):
        p, b, c = self.create2()
        self.check_paneconfigure(p, b, 'padx', 1.3, 1)
        self.check_paneconfigure_bad(p, b, 'padx',
                EXPECTED_SCREEN_DISTANCE_ERRMSG.format('badValue'))

    def test_paneconfigure_pady(self):
        p, b, c = self.create2()
        self.check_paneconfigure(p, b, 'pady', 1.3, 1)
        self.check_paneconfigure_bad(p, b, 'pady',
                EXPECTED_SCREEN_DISTANCE_ERRMSG.format('badValue'))

    def test_paneconfigure_sticky(self):
        p, b, c = self.create2()
        self.check_paneconfigure(p, b, 'sticky', 'nsew', 'nesw')
        self.check_paneconfigure_bad(p, b, 'sticky',
                                     'bad stickyness value "badValue": must '
                                     'be a string containing zero or more of '
                                     'n, e, s, and w')

    def test_paneconfigure_stretch(self):
        p, b, c = self.create2()
        self.check_paneconfigure(p, b, 'stretch', 'alw', 'always')
        self.check_paneconfigure_bad(p, b, 'stretch',
                                     'bad stretch "badValue": must be '
                                     'always, first, last, middle, or never')

    def test_paneconfigure_width(self):
        p, b, c = self.create2()
        self.check_paneconfigure(p, b, 'width', 10, 10)
        self.check_paneconfigure_bad(p, b, 'width',
                EXPECTED_SCREEN_DISTANCE_OR_EMPTY_ERRMSG.format('badValue'))


@add_standard_options(StandardOptionsTests)
class MenuTest(AbstractWidgetTest, unittest.TestCase):
    OPTIONS = (
        'activebackground', 'activeborderwidth', 'activeforeground',
        'activerelief',
        'background', 'borderwidth', 'cursor',
        'disabledforeground', 'font', 'foreground',
        'postcommand', 'relief', 'selectcolor', 'takefocus',
        'tearoff', 'tearoffcommand', 'title', 'type',
    )
    _conv_pixels = False

    def create(self, **kwargs):
        return tkinter.Menu(self.root, **kwargs)

    def test_indexcommand_none(self):
        widget = self.create()
        i = widget.index('none')
        self.assertIsNone(i)

    test_configure_activerelief = requires_tk(8, 7)(StandardOptionsTests.test_configure_activerelief)

    def test_configure_postcommand(self):
        widget = self.create()
        self.checkCommandParam(widget, 'postcommand')

    def test_configure_tearoff(self):
        widget = self.create()
        self.checkBooleanParam(widget, 'tearoff')

    def test_configure_tearoffcommand(self):
        widget = self.create()
        self.checkCommandParam(widget, 'tearoffcommand')

    def test_configure_title(self):
        widget = self.create()
        self.checkParam(widget, 'title', 'any string')

    def test_configure_type(self):
        widget = self.create()
        values = ('normal', 'tearoff', 'menubar')
        self.checkEnumParam(widget, 'type', *values,
                            allow_empty=tk_version < (8, 7),
                            sort=tk_version >= (8, 7))

    def test_entryconfigure(self):
        m1 = self.create()
        m1.add_command(label='test')
        self.assertRaises(TypeError, m1.entryconfigure)
        with self.assertRaisesRegex(TclError, 'bad menu entry index "foo"'):
            m1.entryconfigure('foo')
        d = m1.entryconfigure(1)
        self.assertIsInstance(d, dict)
        for k, v in d.items():
            self.assertIsInstance(k, str)
            self.assertIsInstance(v, tuple)
            self.assertEqual(len(v), 5)
            self.assertEqual(v[0], k)
            self.assertEqual(m1.entrycget(1, k), v[4])
        m1.destroy()

    def test_entryconfigure_label(self):
        m1 = self.create()
        m1.add_command(label='test')
        self.assertEqual(m1.entrycget(1, 'label'), 'test')
        m1.entryconfigure(1, label='changed')
        self.assertEqual(m1.entrycget(1, 'label'), 'changed')

    def test_entryconfigure_variable(self):
        m1 = self.create()
        v1 = tkinter.BooleanVar(self.root)
        v2 = tkinter.BooleanVar(self.root)
        m1.add_checkbutton(variable=v1, onvalue=True, offvalue=False,
                           label='Nonsense')
        self.assertEqual(str(m1.entrycget(1, 'variable')), str(v1))
        m1.entryconfigure(1, variable=v2)
        self.assertEqual(str(m1.entrycget(1, 'variable')), str(v2))


@add_standard_options(PixelSizeTests, StandardOptionsTests)
class MessageTest(AbstractWidgetTest, unittest.TestCase):
    OPTIONS = (
        'anchor', 'aspect', 'background', 'borderwidth',
        'cursor', 'font', 'foreground',
        'highlightbackground', 'highlightcolor', 'highlightthickness',
        'justify', 'padx', 'pady', 'relief',
        'takefocus', 'text', 'textvariable', 'width',
    )
    _conv_pad_pixels = False
    if tk_version >= (8, 7):
        _conv_pixels = False
    _clip_pad = tk_version >= (8, 7)
    _clip_borderwidth = tk_version >= (8, 7)

    def create(self, **kwargs):
        return tkinter.Message(self.root, **kwargs)

    def test_configure_aspect(self):
        widget = self.create()
        self.checkIntegerParam(widget, 'aspect', 250, 0, -300)

    def test_configure_padx(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'padx', 3, 4.4, 5.6, '12m',
                              conv=self._conv_pad_pixels)
        expected = self._default_pixels if self._clip_pad else -2
        self.checkParam(widget, 'padx', -2, expected=expected)

    def test_configure_pady(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'pady', 3, 4.4, 5.6, '12m',
                              conv=self._conv_pad_pixels)
        expected = self._default_pixels if self._clip_pad else -2
        self.checkParam(widget, 'pady', -2, expected=expected)

    def test_configure_width(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'width', 402, 403.4, 404.6, 0, '5i')
        expected = 0 if tk_version >= (8, 7) else -402
        self.checkParam(widget, 'width', -402, expected=expected)


class DefaultRootTest(AbstractDefaultRootTest, unittest.TestCase):

    def test_frame(self):
        self._test_widget(tkinter.Frame)

    def test_label(self):
        self._test_widget(tkinter.Label)


if __name__ == '__main__':
    unittest.main()
