import unittest
import tkinter
from tkinter import TclError
import os
from test.support import requires

from test.test_tkinter.support import setUpModule  # noqa: F401
from test.test_tkinter.support import (requires_tk, tk_version,
                                  get_tk_patchlevel, widget_eq,
                                  wait_until_mapped,
                                  AbstractDefaultRootTest)

from test.test_tkinter.widget_tests import (
    add_configure_tests,
    AbstractWidgetTest,
    StandardOptionsTests,
    IntegerSizeTests,
    PixelSizeTests)

requires('gui')


EXPECTED_SCREEN_DISTANCE_ERRMSG = '(bad|expected) screen distance (but got )?"{}"'
EXPECTED_SCREEN_DISTANCE_OR_EMPTY_ERRMSG = '(bad|expected) screen distance (or "" but got )?"{}"'

def float_round(x):
    return float(round(x))

class AbstractToplevelTest(AbstractWidgetTest, PixelSizeTests):
    _no_round = {'padx', 'pady'}
    if tk_version < (8, 7):
        _clipped = {'highlightthickness'}
    else:
        _clipped = {'borderwidth', 'height', 'highlightthickness', 'padx',
                    'pady', 'width'}

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


@add_configure_tests(StandardOptionsTests)
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


@add_configure_tests(StandardOptionsTests)
class FrameTest(AbstractToplevelTest, unittest.TestCase):
    OPTIONS = (
        'background', 'backgroundimage', 'borderwidth',
        'class', 'colormap', 'container', 'cursor', 'height',
        'highlightbackground', 'highlightcolor', 'highlightthickness',
        'padx', 'pady', 'relief', 'takefocus', 'tile', 'visual', 'width',
    )

    def create(self, **kwargs):
        return tkinter.Frame(self.root, **kwargs)


@add_configure_tests(StandardOptionsTests)
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

# Label, Button, Checkbutton, Radiobutton, MenuButton
class AbstractLabelTest(AbstractWidgetTest, IntegerSizeTests):
    _rounds_pixels = False
    if tk_version < (8, 7):
        _clipped = {}
    elif tk_version < (9, 0):
        _clipped = {'borderwidth', 'height', 'highlightthickness', 'padx', 'pady', 'width'}
    else:
        _clipped = {'borderwidth', 'height', 'highlightthickness',
                    'insertborderwidth', 'padx', 'pady', 'width'}

    def setUp(self):
        super().setUp()
        if tk_version[:2] == (9, 0) and get_tk_patchlevel(self.root) < (9, 0, 2):
            self._clipped = self._clipped - {'height', 'width'}


@add_configure_tests(StandardOptionsTests)
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


@add_configure_tests(StandardOptionsTests)
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
    if tk_version < (8, 7):
        _clipped = {}
    else:
        _clipped = {'borderwidth', 'height', 'highlightthickness',
                    'padx', 'pady', 'width'}

    def create(self, **kwargs):
        return tkinter.Button(self.root, **kwargs)

    def test_configure_default(self):
        widget = self.create()
        self.checkEnumParam(widget, 'default', 'active', 'disabled', 'normal')

    def test_invoke(self):
        success = []
        widget = self.create(command=lambda: success.append(1))
        widget.pack()
        widget.invoke()
        self.assertEqual(success, [1])
        # invoke does nothing for a disabled button.
        widget.configure(state='disabled')
        widget.invoke()
        self.assertEqual(success, [1])

    def test_flash(self):
        widget = self.create()
        widget.pack()
        widget.update_idletasks()
        widget.flash()  # No exception.


@add_configure_tests(StandardOptionsTests)
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
        variables = [b['variable'] for b in buttons]
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

    def test_invoke(self):
        success = []
        v = tkinter.IntVar(self.root)
        widget = self.create(variable=v, onvalue=1, offvalue=0,
                             command=lambda: success.append(v.get()))
        widget.pack()
        widget.invoke()
        self.assertEqual(v.get(), 1)
        self.assertEqual(success, [1])
        widget.invoke()
        self.assertEqual(v.get(), 0)
        self.assertEqual(success, [1, 0])
        # A disabled checkbutton is not toggled and its command is not called.
        widget.configure(state='disabled')
        widget.invoke()
        self.assertEqual(v.get(), 0)
        self.assertEqual(success, [1, 0])

    def test_toggle(self):
        v = tkinter.IntVar(self.root)
        widget = self.create(variable=v, onvalue=1, offvalue=0)
        self.assertEqual(v.get(), 0)
        widget.toggle()
        self.assertEqual(v.get(), 1)
        widget.toggle()
        self.assertEqual(v.get(), 0)

    def test_flash(self):
        widget = self.create()
        widget.pack()
        widget.update_idletasks()
        widget.flash()  # No exception.

@add_configure_tests(StandardOptionsTests)
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

    def test_invoke(self):
        success = []
        v = tkinter.StringVar(self.root)
        widget = self.create(variable=v, value='on',
                             command=lambda: success.append(v.get()))
        widget.pack()
        widget.invoke()
        self.assertEqual(v.get(), 'on')
        self.assertEqual(success, ['on'])
        # invoke does nothing for a disabled radiobutton.
        v.set('')
        widget.configure(state='disabled')
        widget.invoke()
        self.assertEqual(v.get(), '')
        self.assertEqual(success, ['on'])

    def test_flash(self):
        widget = self.create()
        widget.pack()
        widget.update_idletasks()
        widget.flash()  # No exception.


@add_configure_tests(StandardOptionsTests)
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
    _rounds_pixels = (tk_version < (9, 0))
    if tk_version < (8, 7):
        _clipped = {'highlightthickness', 'padx', 'pady'}
    elif tk_version < (9, 0):
        _clipped = {'borderwidth', 'highlightthickness', 'padx', 'pady'}
    else:
        _clipped = {'borderwidth', 'highlightthickness', 'insertborderwidth', 'padx', 'pady'}

    def setUp(self):
        super().setUp()
        if tk_version[:2] == (9, 0) and get_tk_patchlevel(self.root) < (9, 0, 1):
            self._clipped = self._clipped - {'borderwidth'}

    def create(self, **kwargs):
        return tkinter.Menubutton(self.root, **kwargs)

    def test_configure_direction(self):
        widget = self.create()
        self.checkEnumParam(widget, 'direction',
                'above', 'below', 'flush', 'left', 'right')

    def test_configure_height(self):
        widget = self.create()
        if tk_version < (8, 7) or (tk_version[:2] == (9, 0) and get_tk_patchlevel(self.root) < (9, 0, 1)):
            conv = str
        else:
            conv = False
        self.checkIntegerParam(widget, 'height', 100, -100, 0, conv=conv)

    def test_configure_image(self):
        widget = self.create()
        image = tkinter.PhotoImage(master=self.root, name='image1')
        self.checkParam(widget, 'image', image, conv=str)
        if tk_version < (8, 7):
            errmsg = 'image "spam" doesn\'t exist'
        else:
            errmsg = 'image "spam" does not exist'
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
        if tk_version < (8, 7) or (tk_version[:2] == (9, 0) and get_tk_patchlevel(self.root) < (9, 0, 1)):
            conv = str
        else:
            conv = False
        self.checkIntegerParam(widget, 'width', 402, -402, 0, conv=conv)


class OptionMenuTest(MenubuttonTest, unittest.TestCase):

    def create(self, default='b', values=('a', 'b', 'c'), **kwargs):
        return tkinter.OptionMenu(self.root, None, default, *values, **kwargs)

    def test_kwargs(self):
        # Menubutton options can be passed at construction (gh-101284).
        widget = tkinter.OptionMenu(self.root, None, 'b',
                                    width=10, direction='right')
        # Menubutton -width is a string on Tk 8.6, an int on 9.0+.
        self.assertEqual(int(widget['width']), 10)
        self.assertEqual(widget['direction'], 'right')
        # They override OptionMenu's own appearance defaults,
        widget = tkinter.OptionMenu(self.root, None, 'b', relief='flat')
        self.assertEqual(widget['relief'], 'flat')
        # which otherwise keep their historical values.
        widget = tkinter.OptionMenu(self.root, None, 'b')
        self.assertEqual(widget['relief'], 'raised')

    def test_bad_kwarg(self):
        with self.assertRaisesRegex(TclError, r'^unknown option "-spam"$'):
            tkinter.OptionMenu(self.root, None, 'b', spam='')

    def test_specify_name(self):
        widget = tkinter.OptionMenu(self.root, None, ':)', name="option_menu")
        self.assertEqual(str(widget), ".option_menu")
        self.assertIs(self.root.children["option_menu"], widget)

@add_configure_tests(IntegerSizeTests, StandardOptionsTests)
class EntryTest(AbstractWidgetTest, unittest.TestCase):
    if tk_version < (8, 7):
        _clipped = {'highlightthickness'}
    else:
        _clipped = {'borderwidth', 'highlightthickness', 'insertborderwidth',
                    'insertwidth', 'selectborderwidth'}

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
        expected =  100 // 2 if tk_version < (8, 7) else 60
        self.checkParam(widget, 'insertborderwidth', 60, expected=expected)

    def test_configure_insertwidth(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'insertwidth', 1.3, 3.6, 0.9, '10p')
        if tk_version < (8, 7):
            self.checkParam(widget, 'insertwidth', 0, expected=2)
            self.checkParam(widget, 'insertwidth', 0.1, expected=2)
            self.checkParam(widget, 'insertwidth', -2, expected=2)
        else:
            self.checkPixelsParam(widget, 'insertwidth', 0, 0.1, -2)

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

    def test_delete(self):
        widget = self.create()
        widget.insert(0, 'abcdef')
        widget.delete(1, 3)
        self.assertEqual(widget.get(), 'adef')
        widget.delete(1)
        self.assertEqual(widget.get(), 'aef')
        widget.delete(0, 'end')
        self.assertEqual(widget.get(), '')
        self.assertRaisesRegex(TclError, r'bad (entry|spinbox) index "xyz"',
                               widget.delete, 'xyz')
        self.assertRaises(TypeError, widget.delete)

    def test_icursor(self):
        widget = self.create()
        widget.insert(0, 'abcdef')
        widget.icursor(3)
        widget.insert('insert', 'XYZ')
        self.assertEqual(widget.get(), 'abcXYZdef')
        self.assertRaisesRegex(TclError, r'bad (entry|spinbox) index "xyz"',
                               widget.icursor, 'xyz')
        self.assertRaises(TypeError, widget.icursor)

    def test_select_aliases(self):
        # The select_* methods are aliases of the selection_* methods.
        widget = self.create()
        widget.insert(0, '12345')
        self.assertFalse(widget.select_present())
        widget.select_range(0, 'end')
        self.assertTrue(widget.select_present())
        self.assertEqual(widget.selection_get(), '12345')
        widget.select_from(1)
        widget.select_to(3)
        self.assertEqual(widget.selection_get(), '23')
        widget.select_adjust(4)
        self.assertEqual(widget.selection_get(), '234')
        widget.select_clear()
        self.assertFalse(widget.select_present())
        self.assertRaisesRegex(TclError, 'bad entry index "xyz"',
                               widget.select_range, 'xyz', 'end')

    def test_validate(self):
        calls = []
        def validatecommand(value):
            calls.append(value)
            return value.isdigit()
        # validate='none' means validation is never triggered automatically,
        # so validate() exercises the forced evaluation.
        widget = self.create(validate='none',
                validatecommand=(self.root.register(validatecommand), '%P'))
        widget.insert(0, '123')
        result = widget.validate()
        self.assertIs(result, True)
        self.assertEqual(calls, ['123'])
        widget.delete(0, 'end')
        widget.insert(0, 'abc')
        calls.clear()
        self.assertIs(widget.validate(), False)
        self.assertEqual(calls, ['abc'])


@add_configure_tests(StandardOptionsTests)
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
        if tk_version < (8, 7) or (tk_version[:2] == (9, 0) and get_tk_patchlevel(self.root) < (9, 0, 1)):
            expected = 'mon tue wed thur'
        else:
            expected = ('mon', 'tue', 'wed', 'thur')
        self.checkParam(widget, 'values', 'mon tue wed thur',
                        expected=expected)
        self.checkParam(widget, 'values', ('mon', 'tue', 'wed', 'thur'),
                        expected=expected)

        if tk_version < (8, 7) or (tk_version[:2] == (9, 0) and get_tk_patchlevel(self.root) < (9, 0, 1)):
            expected = '42 3.14 {} {any string}'
        else:
            expected = (42, 3.14, '', 'any string')
        self.checkParam(widget, 'values', (42, 3.14, '', 'any string'),
                        expected=expected)

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

    # Spinbox has no select_* aliases, unlike Entry.
    test_select_aliases = None

    def test_invoke(self):
        widget = self.create(from_=0, to=10)
        widget.delete(0, 'end')
        widget.insert(0, '5')
        widget.invoke('buttonup')
        self.assertEqual(widget.get(), '6')
        widget.invoke('buttondown')
        self.assertEqual(widget.get(), '5')
        self.assertRaisesRegex(TclError, 'bad element "spam"',
                               widget.invoke, 'spam')

    def test_identify(self):
        widget = self.create()
        widget.pack()
        # Identifying the element under a point requires the widget to be
        # mapped with a real size.
        if wait_until_mapped(widget):
            self.assertIn(widget.identify(5, 5),
                          ('entry', 'buttonup', 'buttondown', 'none'))
        self.assertRaises(TclError, widget.identify, 'a', 'b')

    def test_scan(self):
        widget = self.create()
        widget.insert(0, 'a' * 100)
        widget.pack()
        widget.update_idletasks()
        self.assertEqual(widget.scan_mark(10), ())
        self.assertEqual(widget.scan_dragto(0), ())
        self.assertRaises(TypeError, widget.scan_mark)


@add_configure_tests(StandardOptionsTests)
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
    _no_round = {'selectborderwidth'}
    if tk_version < (9, 0):
        _clipped = {'highlightthickness', 'spacing1', 'spacing2', 'spacing3'}
    else:
        _clipped = {'borderwidth', 'height', 'highlightthickness',
                    'insertborderwidth', 'insertwidth', 'padx', 'pady',
                    'selectborderwidth', 'spacing1', 'spacing2', 'spacing3'}

    def setUp(self):
        super().setUp()
        if tk_version[:2] == (9, 0) and get_tk_patchlevel(self.root) < (9, 0, 2):
            self._clipped = self._clipped - {'borderwidth', 'height', 'padx', 'pady'}
        if tk_version[:2] == (9, 0) and get_tk_patchlevel(self.root) < (9, 0, 1):
            self._clipped = self._clipped - {'insertborderwidth', 'insertwidth', 'selectborderwidth'}

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
        if tk_version < (9, 0):
            self.checkParam(widget, 'height', 0, expected=1)
            self.checkParam(widget, 'height', -100, expected=1)
        else:
            self.checkPixelsParam(widget, 'height', 0, -100)

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

    def test_configure_spacing1(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'spacing1', 20, 21.4, 22.6, -5, '0.5c')

    def test_configure_spacing2(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'spacing2', 5, 6.4, 7.6, -1, '0.1c')

    def test_configure_spacing3(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'spacing3', 20, 21.4, 22.6, -10, '0.5c')

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
                        expected=(10.2, 20.7, '1i', '2i'))
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


@add_configure_tests(PixelSizeTests, StandardOptionsTests)
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
    if tk_version < (8, 7):
        _clipped = {'highlightthickness'}
    else:
        _clipped = {'borderwidth', 'height', 'highlightthickness',
                    'insertborderwidth', 'insertwidth', 'selectborderwidth',
                    'width', 'xscrollincrement', 'yscrollincrement'}
    _stringify = True

    def setUp(self):
        super().setUp()
        if tk_version[:2] == (9, 0) and get_tk_patchlevel(self.root) < (9, 0, 1):
            self._rounds_pixels = True
            self._no_round = {'borderwidth', 'height', 'highlightthickness',
                              'width', 'xscrollincrement', 'yscrollincrement'}
            self._clipped = self._clipped - {'insertborderwidth', 'insertwidth', 'selectborderwidth'}

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

    def test_create_arc(self):
        c = self.create()
        i = c.create_arc(10, 20, 30, 40)
        self.assertEqual(c.coords(i), [10.0, 20.0, 30.0, 40.0])
        self.assertEqual(c.itemcget(i, 'style'), 'pieslice')
        self.assertEqual(float(c.itemcget(i, 'start')), 0.0)
        self.assertEqual(float(c.itemcget(i, 'extent')), 90.0)

        for style in 'pieslice', 'chord', 'arc':
            i = c.create_arc(10, 20, 30, 40, style=style)
            self.assertEqual(c.itemcget(i, 'style'), style)
        self.assertRaises(TclError, c.create_arc, 10, 20, 30, 40, style='spam')

        i = c.create_arc(10, 20, 30, 40, start=45, extent=120,
                         outline='red', fill='blue', width=3)
        self.assertEqual(float(c.itemcget(i, 'start')), 45.0)
        self.assertEqual(float(c.itemcget(i, 'extent')), 120.0)
        self.assertEqual(str(c.itemcget(i, 'outline')), 'red')
        self.assertEqual(str(c.itemcget(i, 'fill')), 'blue')
        self.assertEqual(float(c.itemcget(i, 'width')), 3.0)
        self.assertRaises(TclError, c.create_arc, 10, 20, 30, 40, extent='spam')

    def test_create_oval(self):
        c = self.create()
        i = c.create_oval(10, 20, 30, 40)
        self.assertEqual(c.coords(i), [10.0, 20.0, 30.0, 40.0])
        self.assertEqual(c.bbox(i), (9, 19, 31, 41))
        self.assertEqual(c.itemcget(i, 'stipple'), '')

        i = c.create_oval(10, 20, 30, 40, fill='red', outline='blue', width=2)
        self.assertEqual(str(c.itemcget(i, 'fill')), 'red')
        self.assertEqual(str(c.itemcget(i, 'outline')), 'blue')
        self.assertEqual(float(c.itemcget(i, 'width')), 2.0)
        self.assertRaises(TclError, c.create_oval, 10, 20, 30, 40, width='spam')

    def test_create_bitmap(self):
        c = self.create()
        i = c.create_bitmap(10, 20, bitmap='gray50')
        self.assertEqual(c.coords(i), [10.0, 20.0])
        self.assertEqual(c.itemcget(i, 'bitmap'), 'gray50')
        self.assertEqual(c.itemcget(i, 'anchor'), 'center')

        for anchor in 'n', 'ne', 'e', 'se', 's', 'sw', 'w', 'nw', 'center':
            i = c.create_bitmap(10, 20, bitmap='gray50', anchor=anchor)
            self.assertEqual(c.itemcget(i, 'anchor'), anchor)
        self.assertRaises(TclError, c.create_bitmap, 10, 20,
                          bitmap='gray50', anchor='spam')

        i = c.create_bitmap(10, 20, bitmap='gray50',
                            foreground='red', background='blue')
        self.assertEqual(str(c.itemcget(i, 'foreground')), 'red')
        self.assertEqual(str(c.itemcget(i, 'background')), 'blue')
        if c._windowingsystem != 'aqua':
            # Aqua resolves bitmaps lazily and does not report a bad name here.
            self.assertRaises(TclError, c.create_bitmap, 10, 20, bitmap='spam')

    def test_create_image(self):
        c = self.create()
        image = tkinter.PhotoImage(master=self.root, width=10, height=10)
        i = c.create_image(10, 20, image=image)
        self.assertEqual(c.coords(i), [10.0, 20.0])
        self.assertEqual(str(c.itemcget(i, 'image')), str(image))
        self.assertEqual(c.itemcget(i, 'anchor'), 'center')

        for anchor in 'n', 'ne', 'e', 'se', 's', 'sw', 'w', 'nw', 'center':
            i = c.create_image(10, 20, image=image, anchor=anchor)
            self.assertEqual(c.itemcget(i, 'anchor'), anchor)
        self.assertRaises(TclError, c.create_image, 10, 20,
                          image=image, anchor='spam')

    def test_create_text(self):
        c = self.create()
        i = c.create_text(10, 20, text='Hello')
        self.assertEqual(c.coords(i), [10.0, 20.0])
        self.assertEqual(c.itemcget(i, 'text'), 'Hello')
        self.assertEqual(c.itemcget(i, 'anchor'), 'center')
        self.assertEqual(c.itemcget(i, 'justify'), 'left')

        for justify in 'left', 'right', 'center':
            i = c.create_text(10, 20, text='Hello', justify=justify)
            self.assertEqual(c.itemcget(i, 'justify'), justify)
        self.assertRaises(TclError, c.create_text, 10, 20, justify='spam')

        for anchor in 'n', 'ne', 'e', 'se', 's', 'sw', 'w', 'nw', 'center':
            i = c.create_text(10, 20, text='Hello', anchor=anchor)
            self.assertEqual(c.itemcget(i, 'anchor'), anchor)
        self.assertRaises(TclError, c.create_text, 10, 20, anchor='spam')

        i = c.create_text(10, 20, text='Hello', fill='red', angle=45,
                          font='TkFixedFont')
        self.assertEqual(str(c.itemcget(i, 'fill')), 'red')
        self.assertEqual(float(c.itemcget(i, 'angle')), 45.0)
        self.assertEqual(str(c.itemcget(i, 'font')), 'TkFixedFont')
        self.assertRaises(TclError, c.create_text, 10, 20, angle='spam')

    def test_create_window(self):
        c = self.create()
        button = tkinter.Button(c, text='ok')
        i = c.create_window(10, 20, window=button)
        self.assertEqual(c.coords(i), [10.0, 20.0])
        self.assertEqual(c.itemcget(i, 'window'), str(button))
        self.assertEqual(c.itemcget(i, 'anchor'), 'center')

        for anchor in 'n', 'ne', 'e', 'se', 's', 'sw', 'w', 'nw', 'center':
            i = c.create_window(10, 20, window=tkinter.Button(c), anchor=anchor)
            self.assertEqual(c.itemcget(i, 'anchor'), anchor)
        self.assertRaises(TclError, c.create_window, 10, 20,
                          window=button, anchor='spam')

        i = c.create_window(10, 20, window=tkinter.Button(c),
                            width=30, height=40)
        self.assertEqual(int(c.itemcget(i, 'width')), 30)
        self.assertEqual(int(c.itemcget(i, 'height')), 40)
        self.assertRaises(TclError, c.create_window, 10, 20,
                          window=button, width='spam')

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

    def test_create_items(self):
        c = self.create()
        image = tkinter.PhotoImage(master=self.root, width=10, height=10)
        button = tkinter.Button(c, text='ok')
        items = {
            'arc': c.create_arc(10, 20, 30, 40),
            'bitmap': c.create_bitmap(10, 20, bitmap='gray50'),
            'image': c.create_image(10, 20, image=image),
            'line': c.create_line(10, 20, 30, 40),
            'oval': c.create_oval(10, 20, 30, 40),
            'polygon': c.create_polygon(10, 20, 30, 40, 50, 20),
            'rectangle': c.create_rectangle(10, 20, 30, 40),
            'text': c.create_text(10, 20, text='hello'),
            'window': c.create_window(10, 20, window=button),
        }
        for itemtype, item in items.items():
            self.assertIsInstance(item, int)
            self.assertEqual(c.type(item), itemtype)
        # All items are reported by find_all in creation (stacking) order.
        self.assertEqual(c.find_all(), tuple(sorted(items.values())))

        # No coordinates at all is an IndexError; a bad number is a TclError.
        self.assertRaises(IndexError, c.create_arc)
        self.assertRaises(TclError, c.create_arc, 1, 2, 3)
        self.assertRaises(TclError, c.create_oval, 1, 2)

    def test_type(self):
        c = self.create()
        rect = c.create_rectangle(10, 20, 30, 40)
        self.assertEqual(c.type(rect), 'rectangle')
        # An unmatched tag or id is not an error.
        self.assertIsNone(c.type('nonexistent'))
        self.assertIsNone(c.type(9999))
        self.assertRaises(TypeError, c.type)
        self.assertRaises(TypeError, c.type, rect, 'extra')

    def test_bbox(self):
        c = self.create()
        rect = c.create_rectangle(10, 10, 30, 30)
        bbox = c.bbox(rect)
        self.assertIsInstance(bbox, tuple)
        self.assertEqual(len(bbox), 4)
        for v in bbox:
            self.assertIsInstance(v, int)
        # The bounding box encloses the item (with a small margin).
        self.assertEqual(bbox, (9, 9, 31, 31))
        # bbox over several items is their union.
        rect2 = c.create_rectangle(50, 50, 70, 70)
        self.assertEqual(c.bbox(rect, rect2), (9, 9, 71, 71))
        # An unmatched tag has no bounding box.
        self.assertIsNone(c.bbox('nonexistent'))
        # At least one tag or id is required.
        self.assertRaises(TclError, c.bbox)

    def test_canvasx_canvasy(self):
        c = self.create(borderwidth=0, highlightthickness=0)
        self.assertEqual(c.canvasx(0), 0.0)
        self.assertEqual(c.canvasx(10), 10.0)
        self.assertIsInstance(c.canvasx(10), float)
        # gridspacing rounds to the nearest multiple.
        self.assertEqual(c.canvasx(13, 5), 15.0)
        self.assertEqual(c.canvasy(0), 0.0)
        self.assertEqual(c.canvasy(10), 10.0)
        self.assertRaises(TypeError, c.canvasx)
        self.assertRaises(TypeError, c.canvasx, 0, 5, 1)
        self.assertRaises(TypeError, c.canvasy)
        self.assertRaises(TypeError, c.canvasy, 0, 5, 1)
        self.assertRaises(TclError, c.canvasx, 'spam')
        self.assertRaises(TclError, c.canvasy, 'spam')

    def test_move(self):
        c = self.create()
        rect = c.create_rectangle(10, 10, 30, 30)
        c.move(rect, 5, 7)
        self.assertEqual(c.coords(rect), [15.0, 17.0, 35.0, 37.0])
        c.move(rect, -5, -7)
        self.assertEqual(c.coords(rect), [10.0, 10.0, 30.0, 30.0])
        # move() takes variable arguments; bad or missing values reach Tcl.
        self.assertRaises(TclError, c.move, rect, 'spam', 0)
        self.assertRaises(TclError, c.move, rect)

    def test_scale(self):
        c = self.create()
        rect = c.create_rectangle(10, 10, 30, 30)
        c.scale(rect, 0, 0, 2, 2)
        self.assertEqual(c.coords(rect), [20.0, 20.0, 60.0, 60.0])
        c.scale(rect, 0, 0, 0.5, 0.5)
        self.assertEqual(c.coords(rect), [10.0, 10.0, 30.0, 30.0])
        self.assertRaises(TclError, c.scale, rect, 0, 0, 'spam', 2)
        self.assertRaises(TclError, c.scale, rect, 0, 0)  # missing factors

    @requires_tk(8, 6)
    def test_rchars(self):
        c = self.create()
        # On a line item, rchars replaces a range of the coordinate list.
        line = c.create_line(0, 0, 10, 10, 20, 0)
        c.rchars(line, 2, 5, (30, 30, 40, 40))
        self.assertEqual(c.coords(line), [0.0, 0.0, 30.0, 30.0, 40.0, 40.0])
        # On a text item, rchars replaces a range of characters.
        text = c.create_text(10, 10, text='hello')
        c.rchars(text, 0, 2, 'HE')
        self.assertEqual(c.itemcget(text, 'text'), 'HElo')
        self.assertRaises(TclError, c.rchars)

    @requires_tk(9, 0)
    def test_rotate(self):
        c = self.create()
        line = c.create_line(10, 0, 20, 0)
        # The canvas y-axis points down, so an anticlockwise rotation about
        # the origin maps (x, y) to (y, -x).
        c.rotate(line, 0, 0, 90)
        for got, expected in zip(c.coords(line), [0, -10, 0, -20]):
            self.assertAlmostEqual(got, expected, places=3)
        # A negative angle rotates clockwise, restoring the original position.
        c.rotate(line, 0, 0, -90)
        for got, expected in zip(c.coords(line), [10, 0, 20, 0]):
            self.assertAlmostEqual(got, expected, places=3)
        self.assertRaises(TclError, c.rotate, line, 0, 0, 'spam')

    def test_delete(self):
        c = self.create()
        r1 = c.create_rectangle(10, 10, 30, 30)
        r2 = c.create_rectangle(50, 50, 70, 70)
        r3 = c.create_rectangle(90, 90, 110, 110)
        self.assertEqual(c.find_all(), (r1, r2, r3))
        c.delete(r2)
        self.assertEqual(c.find_all(), (r1, r3))
        # Deleting a non-existent item is not an error.
        c.delete(9999)
        c.delete('all')
        self.assertEqual(c.find_all(), ())

    def test_find(self):
        c = self.create()
        r1 = c.create_rectangle(10, 10, 30, 30)
        r2 = c.create_rectangle(50, 50, 70, 70)
        r3 = c.create_rectangle(100, 100, 120, 120)

        self.assertEqual(c.find_all(), (r1, r2, r3))
        # find_above/find_below return the single adjacent item.
        self.assertEqual(c.find_above(r1), (r2,))
        self.assertEqual(c.find_below(r3), (r2,))
        self.assertEqual(c.find_above(r3), ())  # nothing above the top item
        self.assertEqual(c.find_withtag(r2), (r2,))
        self.assertEqual(c.find_closest(60, 60), (r2,))
        self.assertEqual(c.find_enclosed(0, 0, 80, 80), (r1, r2))
        self.assertEqual(c.find_overlapping(0, 0, 20, 20), (r1,))
        # An unmatched query returns an empty tuple.
        self.assertEqual(c.find_withtag('nonexistent'), ())
        for result in (c.find_all(), c.find_withtag(r1)):
            self.assertIsInstance(result, tuple)

        # An automatically generated widget name can be used as a tag
        # (gh-143070).
        w = tkinter.Frame(c)
        r4 = c.create_window(0, 0, window=w, tags=str(w))
        self.assertEqual(c.find_withtag(str(w)), (r4,))

        self.assertRaises(TclError, c.find_closest, 'spam', 0)
        self.assertRaises(TclError, c.find_enclosed, 0, 0, 'spam', 0)
        self.assertRaises(TclError, c.find_overlapping, 0, 0, 'spam', 0)
        self.assertRaises(TypeError, c.find_withtag)
        self.assertRaises(TypeError, c.find_withtag, r1, 'extra')
        self.assertRaises(TypeError, c.find_above)
        self.assertRaises(TypeError, c.find_above, r1, 'extra')
        self.assertRaises(TypeError, c.find_below)
        self.assertRaises(TypeError, c.find_closest)
        self.assertRaises(TypeError, c.find_closest, 0, 0, 1, 2, 3)
        self.assertRaises(TypeError, c.find_enclosed, 0, 0, 1)
        self.assertRaises(TypeError, c.find_enclosed, 0, 0, 1, 2, 3)

    def test_addtag_gettags_dtag(self):
        c = self.create()
        r1 = c.create_rectangle(10, 10, 30, 30)
        r2 = c.create_rectangle(50, 50, 70, 70)

        self.assertEqual(c.gettags(r1), ())
        c.addtag_withtag('spam', r1)
        self.assertEqual(c.gettags(r1), ('spam',))
        self.assertEqual(c.find_withtag('spam'), (r1,))

        c.addtag_all('all')
        self.assertIn('all', c.gettags(r1))
        self.assertIn('all', c.gettags(r2))

        c.addtag_above('above1', r1)
        self.assertIn('above1', c.gettags(r2))
        c.addtag_below('below2', r2)
        self.assertIn('below2', c.gettags(r1))

        c.addtag_enclosed('enc', 0, 0, 40, 40)
        self.assertEqual(c.find_withtag('enc'), (r1,))
        c.addtag_overlapping('ov', 0, 0, 20, 20)
        self.assertEqual(c.find_withtag('ov'), (r1,))
        c.addtag_closest('close', 60, 60)
        self.assertEqual(c.find_withtag('close'), (r2,))

        # gettags of an unmatched tag is empty.
        self.assertEqual(c.gettags('nonexistent'), ())

        # dtag removes a tag from an item.
        c.dtag(r1, 'spam')
        self.assertNotIn('spam', c.gettags(r1))

        self.assertRaises(TypeError, c.addtag_withtag, 'tag')
        self.assertRaises(TypeError, c.addtag_withtag, 'tag', r1, 'extra')
        self.assertRaises(TypeError, c.addtag_all)
        self.assertRaises(TypeError, c.addtag_enclosed, 'tag', 0, 0, 1)
        self.assertRaises(TypeError, c.addtag_enclosed, 'tag', 0, 0, 1, 2, 3)
        self.assertRaises(TclError, c.addtag_closest, 'tag', 'spam', 0)
        self.assertRaises(TclError, c.addtag_enclosed, 'tag', 0, 0, 'spam', 0)
        self.assertRaises(TclError, c.gettags)  # needs an item

    def test_itemconfigure(self):
        c = self.create()
        rect = c.create_rectangle(10, 10, 30, 30)
        c.itemconfigure(rect, fill='red', width=3)
        self.assertEqual(str(c.itemcget(rect, 'fill')), 'red')
        self.assertEqual(float(c.itemcget(rect, 'width')), 3.0)

        # Querying all options returns a dict; a single one returns its spec.
        cnf = c.itemconfigure(rect)
        self.assertIsInstance(cnf, dict)
        self.assertIn('fill', cnf)
        self.assertEqual(c.itemconfigure(rect, 'fill')[-1], 'red')

        # itemconfig is an alias of itemconfigure.
        self.assertEqual(c.itemconfig, c.itemconfigure)

        self.assertRaises(TclError, c.itemcget, rect, 'badoption')
        self.assertRaises(TclError, c.itemconfigure, rect, badoption='x')
        self.assertRaises(TypeError, c.itemcget, rect)
        self.assertRaises(TypeError, c.itemcget, rect, 'fill', 'extra')

    def test_tag_raise_lower(self):
        c = self.create()
        r1 = c.create_rectangle(10, 10, 30, 30)
        r2 = c.create_rectangle(50, 50, 70, 70)
        r3 = c.create_rectangle(90, 90, 110, 110)
        self.assertEqual(c.find_all(), (r1, r2, r3))

        c.tag_raise(r1)
        self.assertEqual(c.find_all(), (r2, r3, r1))
        c.tag_lower(r1)
        self.assertEqual(c.find_all(), (r1, r2, r3))
        # Raise above / lower below a specific item.
        c.tag_raise(r1, r2)
        self.assertEqual(c.find_all(), (r2, r1, r3))
        c.tag_lower(r3, r2)
        self.assertEqual(c.find_all(), (r3, r2, r1))

        # lower/lift are aliases of tag_lower/tag_raise.
        self.assertEqual(c.lower, c.tag_lower)
        self.assertEqual(c.lift, c.tag_raise)

        # raise/lower need at least an item; an unmatched tag is not an error.
        self.assertRaises(TclError, c.tag_raise)
        self.assertRaises(TclError, c.tag_lower)
        self.assertIsNone(c.tag_raise('nonexistent'))

    def test_text_item(self):
        c = self.create()
        item = c.create_text(10, 10, text='Hello')
        self.assertEqual(c.index(item, 'end'), 5)
        self.assertIsInstance(c.index(item, 'end'), int)

        c.insert(item, 'end', ' world')
        self.assertEqual(c.itemcget(item, 'text'), 'Hello world')
        self.assertEqual(c.index(item, 'end'), 11)
        c.insert(item, 0, '>> ')
        self.assertEqual(c.itemcget(item, 'text'), '>> Hello world')

        c.dchars(item, 0, 2)
        self.assertEqual(c.itemcget(item, 'text'), 'Hello world')
        c.icursor(item, 3)

        # index requires an indexable (text) item and a valid index.
        self.assertRaises(TclError, c.index, item, 'badspec')
        self.assertRaises(TclError, c.index, item)  # missing index
        self.assertRaises(TclError, c.dchars, item, 'badspec', 'end')
        rect = c.create_rectangle(10, 10, 30, 30)
        self.assertRaises(TclError, c.index, rect, 'end')

    def test_select(self):
        c = self.create()
        item = c.create_text(10, 10, text='Hello world')
        self.assertIsNone(c.select_item())

        c.select_from(item, 0)
        c.select_to(item, 4)
        self.assertEqual(int(c.select_item()), item)
        c.select_adjust(item, 6)

        c.select_clear()
        self.assertIsNone(c.select_item())
        self.assertRaises(TypeError, c.select_from, item)
        self.assertRaises(TypeError, c.select_from, item, 0, 'extra')
        # A bad index reaches Tcl; selection applies only to text items.
        self.assertRaises(TclError, c.select_from, item, 'badspec')
        rect = c.create_rectangle(10, 10, 30, 30)
        self.assertRaises(TclError, c.select_from, rect, 0)

    def test_focus(self):
        c = self.create()
        item = c.create_text(10, 10, text='Hello')
        # Only text items can take the focus.
        c.focus(item)
        self.assertEqual(int(c.focus()), item)
        c.focus('')
        self.assertIn(c.focus(), ('', None))

    def test_scan(self):
        c = self.create()
        c.create_rectangle(10, 10, 300, 300)
        c.scan_mark(0, 0)
        c.scan_dragto(5, 5)  # default gain=10
        c.scan_dragto(5, 5, 1)
        self.assertRaises(TypeError, c.scan_mark)
        self.assertRaises(TypeError, c.scan_mark, 0, 0, 0)
        self.assertRaises(TclError, c.scan_mark, 'spam', 0)

    def test_postscript(self):
        c = self.create()
        c.create_rectangle(10, 10, 30, 30, fill='black')
        ps = c.postscript()
        self.assertIsInstance(ps, str)
        self.assertStartsWith(ps, '%!PS-Adobe')
        self.assertRaises(TclError, c.postscript, badoption='spam')

    def assertCommandExist(self, widget, funcid):
        self.assertEqual(
            widget.tk.splitlist(widget.tk.call('info', 'commands', funcid)),
            (funcid,))

    def assertCommandNotExist(self, widget, funcid):
        self.assertEqual(
            widget.tk.splitlist(widget.tk.call('info', 'commands', funcid)),
            ())

    def test_tag_bind(self):
        c = self.create()
        c.pack()
        item = c.create_rectangle(20, 20, 100, 100, fill='red')
        self.assertEqual(c.tag_bind(item), ())
        self.assertEqual(c.tag_bind(item, '<Button-1>'), '')

        events = []
        def test1(e): events.append('a')
        def test2(e): events.append('b')

        funcid = c.tag_bind(item, '<Button-1>', test1)
        self.assertEqual(c.tag_bind(item), ('<Button-1>',))
        script = c.tag_bind(item, '<Button-1>')
        self.assertIn(funcid, script)
        self.assertCommandExist(c, funcid)

        funcid2 = c.tag_bind(item, '<Button-1>', test2, add=True)
        script = c.tag_bind(item, '<Button-1>')
        self.assertIn(funcid, script)
        self.assertIn(funcid2, script)
        self.assertCommandExist(c, funcid)
        self.assertCommandExist(c, funcid2)

        c.wait_visibility()
        c.focus_force()
        c.update()
        c.event_generate('<Button-1>', x=50, y=50)
        c.update()
        self.assertEqual(events, ['a', 'b'])

        # Binding to a tag applies to all items carrying it.
        c.addtag_withtag('spam', item)
        events.clear()
        c.tag_bind('spam', '<Button-3>', test1)
        c.event_generate('<Button-3>', x=50, y=50)
        c.update()
        self.assertEqual(events, ['a'])

    def test_tag_unbind(self):
        c = self.create()
        c.pack()
        item = c.create_rectangle(20, 20, 100, 100, fill='red')

        events = []
        def test1(e): events.append('a')
        def test2(e): events.append('b')

        funcid = c.tag_bind(item, '<Button-1>', test1)
        funcid2 = c.tag_bind(item, '<Button-1>', test2, add=True)
        c.wait_visibility()
        c.focus_force()
        c.update()
        c.event_generate('<Button-1>', x=50, y=50)
        c.update()
        self.assertEqual(events, ['a', 'b'])

        # Removing one function leaves the other in place.
        c.tag_unbind(item, '<Button-1>', funcid)
        script = c.tag_bind(item, '<Button-1>')
        self.assertNotIn(funcid, script)
        self.assertIn(funcid2, script)
        self.assertCommandNotExist(c, funcid)
        self.assertCommandExist(c, funcid2)
        events.clear()
        c.event_generate('<Button-1>', x=50, y=50)
        c.update()
        self.assertEqual(events, ['b'])

        # Without a funcid all bindings for the sequence are removed.
        c.tag_unbind(item, '<Button-1>')
        self.assertEqual(c.tag_bind(item, '<Button-1>'), '')
        self.assertEqual(c.tag_bind(item), ())
        events.clear()
        c.event_generate('<Button-1>', x=50, y=50)
        c.update()
        self.assertEqual(events, [])

        self.assertRaises(TypeError, c.tag_unbind, item)


@add_configure_tests(IntegerSizeTests, StandardOptionsTests)
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
    if tk_version < (8, 7):
        _clipped = {'highlightthickness'}
    else:
        _clipped = {'borderwidth', 'highlightthickness', 'selectborderwidth'}

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

    def test_size(self):
        lb = self.create()
        self.assertEqual(lb.size(), 0)
        lb.insert(0, *('el%d' % i for i in range(8)))
        self.assertEqual(lb.size(), 8)
        lb.delete(0, 2)
        self.assertEqual(lb.size(), 5)
        self.assertRaises(TypeError, lb.size, 0)

    def test_delete(self):
        lb = self.create()
        lb.insert(0, *('el%d' % i for i in range(8)))
        lb.delete(0)
        self.assertEqual(lb.get(0, 'end'),
                         ('el1', 'el2', 'el3', 'el4', 'el5', 'el6', 'el7'))
        lb.delete(2, 4)
        self.assertEqual(lb.get(0, 'end'), ('el1', 'el2', 'el6', 'el7'))
        lb.delete(0, 'end')
        self.assertEqual(lb.size(), 0)
        self.assertRaises(TclError, lb.delete, 'noindex')
        self.assertRaises(TypeError, lb.delete)

    def test_index(self):
        lb = self.create()
        lb.insert(0, *('el%d' % i for i in range(8)))
        self.assertEqual(lb.index(3), 3)
        self.assertEqual(lb.index('end'), 8)  # the number of elements
        lb.activate(4)
        self.assertEqual(lb.index('active'), 4)
        lb.selection_anchor(2)
        self.assertEqual(lb.index('anchor'), 2)
        self.assertRaisesRegex(TclError, 'bad listbox index "spam"',
                               lb.index, 'spam')

    def test_nearest(self):
        lb = self.create()
        lb.insert(0, *('el%d' % i for i in range(8)))
        lb.pack()
        lb.wait_visibility()
        lb.update()
        # Derive the line height from the first item, which is always
        # displayed (bbox() returns None for items that are not).
        x, y, w, h = lb.bbox(0)
        self.assertEqual(lb.nearest(y + h // 2), 0)
        self.assertEqual(lb.nearest(y + 3 * h + h // 2), 3)
        self.assertRaises(TclError, lb.nearest, 'spam')
        self.assertRaises(TypeError, lb.nearest)

    def test_see(self):
        lb = self.create(height=5)
        lb.insert(0, *('el%d' % i for i in range(20)))
        lb.pack()
        lb.update_idletasks()
        lb.see('end')
        lb.update_idletasks()
        self.assertEqual(lb.yview()[1], 1.0)
        lb.see(0)
        lb.update_idletasks()
        self.assertEqual(lb.yview()[0], 0.0)
        self.assertRaises(TclError, lb.see, 'spam')

    def test_activate(self):
        lb = self.create()
        lb.insert(0, *('el%d' % i for i in range(8)))
        lb.activate(3)
        self.assertEqual(lb.index('active'), 3)
        lb.activate('end')
        self.assertEqual(lb.index('active'), 7)
        self.assertRaises(TclError, lb.activate, 'spam')
        self.assertRaises(TypeError, lb.activate)

    def test_selection(self):
        lb = self.create()
        lb.insert(0, *('el%d' % i for i in range(8)))
        self.assertEqual(lb.curselection(), ())
        self.assertFalse(lb.selection_includes(0))

        lb.selection_set(2, 4)
        lb.selection_set(6)
        self.assertEqual(lb.curselection(), (2, 3, 4, 6))
        self.assertTrue(lb.selection_includes(3))
        self.assertFalse(lb.selection_includes(5))

        lb.selection_clear(3, 4)
        self.assertEqual(lb.curselection(), (2, 6))

        lb.selection_anchor(5)
        self.assertEqual(lb.index('anchor'), 5)

        # select_* are aliases of the selection_* methods.
        lb.select_clear(0, 'end')
        self.assertEqual(lb.curselection(), ())
        lb.select_set(1)
        self.assertTrue(lb.select_includes(1))
        lb.select_anchor(1)
        self.assertEqual(lb.index('anchor'), 1)

        self.assertRaisesRegex(TclError, 'bad listbox index "spam"',
                               lb.selection_includes, 'spam')

    def test_selection_event(self):
        # Keyboard navigation changes the selection and fires the
        # <<ListboxSelect>> virtual event.
        lb = self.create(selectmode='browse', exportselection=False)
        lb.insert(0, *('el%d' % i for i in range(5)))
        lb.pack()
        self.require_mapped(lb)
        events = []
        lb.bind('<<ListboxSelect>>', lambda e: events.append(lb.curselection()))
        lb.focus_force()
        lb.activate(0)
        lb.event_generate('<Down>')
        lb.event_generate('<Down>')
        lb.update()
        self.assertEqual(events, [(1,), (2,)])
        self.assertEqual(lb.curselection(), (2,))


@add_configure_tests(PixelSizeTests, StandardOptionsTests)
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
    _clipped = {'highlightthickness'}
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

    def test_identify(self):
        widget = self.create()
        widget.pack()
        # Probe a point on the trough centreline (Scale.coords()) rather than
        # a fixed pixel: (5, 5) lies outside the trough and always identifies
        # as '', so it would not actually exercise identify().
        if wait_until_mapped(widget):
            x, y = widget.coords()
            self.assertIn(widget.identify(int(x), int(y)),
                          ('slider', 'trough1', 'trough2'))
        self.assertRaises(TclError, widget.identify, 'a', 'b')


@add_configure_tests(PixelSizeTests, StandardOptionsTests)
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
    if tk_version < (8, 7):
        _clipped = {'highlightthickness'}
    elif tk_version < (9, 0):
        _clipped = {'borderwidth', 'elementborderwidth', 'highlightthickness'}
    else:
        _clipped = {'borderwidth', 'elementborderwidth', 'highlightthickness', 'width'}
    _clipped_to_default = {'elementborderwidth'}
    _stringify = True
    default_orient = 'vertical'

    def create(self, **kwargs):
        return tkinter.Scrollbar(self.root, **kwargs)

    def test_configure_elementborderwidth(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'elementborderwidth', 4.3, 5.6, -2, '1m')

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

    def test_fraction(self):
        sb = self.create()
        sb.pack(fill='y', expand=True)
        sb.update_idletasks()
        self.assertIsInstance(sb.fraction(0, 0), float)
        f = sb.fraction(0, 1000)
        self.assertIsInstance(f, float)
        self.assertGreaterEqual(f, 0.0)
        self.assertLessEqual(f, 1.0)
        self.assertRaises(TclError, sb.fraction, 'a', 'b')
        self.assertRaises(TypeError, sb.fraction, 0)

    def test_delta(self):
        sb = self.create()
        sb.pack(fill='y', expand=True)
        sb.update_idletasks()
        self.assertIsInstance(sb.delta(0, 10), float)
        self.assertRaises(TclError, sb.delta, 'a', 'b')
        self.assertRaises(TypeError, sb.delta, 0)

    def test_identify(self):
        sb = self.create()
        sb.pack(fill='y', expand=True)
        # Identifying the element under a point requires the widget to be
        # mapped with a real size.
        if wait_until_mapped(sb):
            self.assertIn(sb.identify(5, 5),
                          ('arrow1', 'arrow2', 'slider', 'trough1', 'trough2'))
        self.assertRaises(TclError, sb.identify, 'a', 'b')


@add_configure_tests(PixelSizeTests, StandardOptionsTests)
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
    _no_round = {'handlesize', 'height', 'proxyborderwidth', 'sashwidth',
                 'selectborderwidth', 'width'}
    _clipped = {}
    default_orient = 'horizontal'

    def create(self, **kwargs):
        return tkinter.PanedWindow(self.root, **kwargs)

    def test_configure_handlepad(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'handlepad', 5, 6.4, 7.6, -3, '1m')

    def test_configure_handlesize(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'handlesize', 8, 9.4, 10.6, -3, '2m')

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
                              0, 1.3, 2.9, 6, -2, '10p')

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
        self.checkPixelsParam(widget, 'sashwidth', 10, 11.1, 15.6, -3, '1m')

    def test_configure_showhandle(self):
        widget = self.create()
        self.checkBooleanParam(widget, 'showhandle')

    def create2(self):
        p = self.create()
        b = tkinter.Button(p)
        c = tkinter.Button(p)
        p.add(b)
        p.add(c)
        return p, b, c

    def test_panes(self):
        p, b, c = self.create2()
        self.assertEqual([str(x) for x in p.panes()], [str(b), str(c)])

    def test_remove(self):
        p, b, c = self.create2()
        p.remove(b)
        self.assertEqual([str(x) for x in p.panes()], [str(c)])
        p.forget(c)  # forget is an alias of remove.
        self.assertEqual(p.panes(), ())

    def test_sash(self):
        p, b, c = self.create2()
        p.configure(width=200, height=50)
        p.pack()
        p.update()
        x, y = p.sash_coord(0)
        self.assertIsInstance(x, int)
        self.assertIsInstance(y, int)
        p.sash_place(0, 120, 0)
        p.update()
        self.assertEqual(p.sash_coord(0)[0], 120)
        p.sash_mark(0)  # No exception.
        self.assertRaises(TclError, p.sash_coord, 5)

    def test_proxy(self):
        p, b, c = self.create2()
        p.configure(width=200, height=50)
        p.pack()
        p.update()
        p.proxy_place(100, 10)
        p.update()
        self.assertEqual(p.proxy_coord()[0], 100)
        p.proxy_forget()
        p.update()

    def test_identify(self):
        p, b, c = self.create2()
        p.configure(width=200, height=50)
        p.pack()
        # Locating the sash requires the widget to be mapped with a real
        # size; the rest of the checks do not.
        if wait_until_mapped(p):
            x, y = p.sash_coord(0)
            # A point over the sash reports the sash.
            self.assertIn('sash', p.identify(x + 1, y + 5))
        # A point over a pane reports nothing.
        self.assertFalse(p.identify(2, 2))
        self.assertRaises(TclError, p.identify, 'a', 'b')

    def test_add_options(self):
        p = self.create()
        b = tkinter.Button(p)
        p.add(b, minsize=40, padx=3, sticky='ns')
        self.assertEqual(p.panecget(b, 'minsize'),
                         40 if self.wantobjects else '40')
        self.assertEqual(p.panecget(b, 'padx'),
                         3 if self.wantobjects else '3')
        self.assertEqual(p.panecget(b, 'sticky'), 'ns')
        self.assertRaisesRegex(TclError, 'unknown option "-spam"',
                               p.add, tkinter.Button(p), spam='x')
        self.assertRaisesRegex(TclError, 'bad window path name "spam"',
                               p.add, 'spam')

    def test_paneconfigure_errors(self):
        p, b, c = self.create2()
        self.assertRaisesRegex(TclError, 'unknown option "-spam"',
                               p.paneconfigure, b, spam='x')
        self.assertRaises(TclError, p.panecget, b, 'spam')
        self.assertRaises(TclError, p.paneconfigure, 'spam')

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
        self.check_paneconfigure(p, b, 'padx', 1.3, 1 if tk_version < (9, 0) else 1.3)
        self.check_paneconfigure_bad(p, b, 'padx',
                EXPECTED_SCREEN_DISTANCE_ERRMSG.format('badValue'))

    def test_paneconfigure_pady(self):
        p, b, c = self.create2()
        self.check_paneconfigure(p, b, 'pady', 1.3, 1 if tk_version < (9, 0) else 1.3)
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

    def test_paneconfigure_child(self):
        p, b, c = self.create2()
        # The child pane is the first argument, positionally or by keyword.
        p.paneconfigure(b, minsize=40)
        self.assertEqual(p.panecget(b, 'minsize'), 40)
        p.paneconfigure(child=b, minsize=50)
        self.assertEqual(p.panecget(b, 'minsize'), 50)
        self.assertIsInstance(p.paneconfigure(b), dict)
        self.assertEqual(p.paneconfigure(b, 'minsize')[4], 50)
        # Omitting the child is an error.
        self.assertRaises(TypeError, p.paneconfigure)

    def test_paneconfigure_tagOrId_deprecated(self):
        p, b, c = self.create2()
        # 'tagOrId' is a deprecated alias of 'child'.
        with self.assertWarns(DeprecationWarning):
            p.paneconfigure(tagOrId=b, minsize=40)
        self.assertEqual(p.panecget(b, 'minsize'), 40)
        # Giving both 'child' and 'tagOrId' is an error.
        self.assertRaises(TypeError, p.paneconfigure, b, tagOrId=c)


@add_configure_tests(StandardOptionsTests)
class MenuTest(AbstractWidgetTest, unittest.TestCase):
    OPTIONS = (
        'activebackground', 'activeborderwidth', 'activeforeground',
        'activerelief', 'background', 'borderwidth', 'cursor',
        'disabledforeground', 'font', 'foreground',
        'postcommand', 'relief', 'selectcolor', 'takefocus',
        'tearoff', 'tearoffcommand', 'title', 'type',
    )
    _rounds_pixels = False
    _clipped = {}

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
        self.assertEqual(m1.entrycget(1, 'variable'), str(v1))
        m1.entryconfigure(1, variable=v2)
        self.assertEqual(m1.entrycget(1, 'variable'), str(v2))

    def test_add(self):
        m = self.create(tearoff=False)
        m.add_command(label='Command')
        m.add_checkbutton(label='Checkbutton')
        m.add_radiobutton(label='Radiobutton')
        m.add_separator()
        m.add_cascade(label='Cascade', menu=tkinter.Menu(m, tearoff=False))
        self.assertEqual(m.index('end'), 4)
        self.assertEqual([m.type(i) for i in range(5)],
                         ['command', 'checkbutton', 'radiobutton',
                          'separator', 'cascade'])
        self.assertEqual(m.entrycget(0, 'label'), 'Command')
        self.assertRaisesRegex(TclError, 'bad menu entry type "spam"',
                               m.add, 'spam')

    def test_insert(self):
        m = self.create(tearoff=False)
        m.add_command(label='A')
        m.add_command(label='C')
        m.insert_command(1, label='B')
        m.insert_separator(0)
        m.insert_checkbutton('end', label='D')
        m.insert_radiobutton(0, label='top')
        m.insert_cascade(2, label='sub',
                         menu=tkinter.Menu(m, tearoff=False))
        self.assertEqual(
            [m.type(i) for i in range(m.index('end') + 1)],
            ['radiobutton', 'separator', 'cascade', 'command',
             'command', 'command', 'checkbutton'])
        self.assertEqual(
            [m.entrycget(i, 'label') for i in (3, 4, 5)],
            ['A', 'B', 'C'])
        self.assertRaisesRegex(TclError, 'bad menu entry type "spam"',
                               m.insert, 0, 'spam')
        self.assertRaisesRegex(TclError, 'bad menu entry index "spam"',
                               m.insert_command, 'spam', label='z')

    def test_delete(self):
        m = self.create(tearoff=False)
        commands = []
        for label in 'ABCDE':
            m.add_command(label=label,
                          command=lambda label=label: commands.append(label))
        # The Tcl command for a deleted item is cleaned up.
        funcid = str(m.entrycget(2, 'command'))
        self.assertEqual(
            m.tk.splitlist(m.tk.call('info', 'commands', funcid)), (funcid,))

        m.delete(2)  # Delete a single item ('C').
        self.assertEqual([m.entrycget(i, 'label') for i in range(4)],
                         ['A', 'B', 'D', 'E'])
        self.assertEqual(
            m.tk.splitlist(m.tk.call('info', 'commands', funcid)), ())

        m.delete(1, 2)  # Delete a range ('B' and 'D').
        self.assertEqual([m.entrycget(i, 'label') for i in range(2)],
                         ['A', 'E'])
        self.assertRaises(TypeError, m.delete)

    def test_index(self):
        m = self.create(tearoff=False)
        self.assertIsNone(m.index('end'))
        m.add_command(label='First')
        m.add_command(label='Second')
        self.assertEqual(m.index('end'), 1)
        self.assertEqual(m.index('last'), 1)
        self.assertEqual(m.index('Second'), 1)
        self.assertEqual(m.index(0), 0)
        # 'active' and 'none' map to None when no item is active.
        self.assertIsNone(m.index('active'))
        self.assertIsNone(m.index('none'))
        self.assertRaisesRegex(TclError, 'bad menu entry index "spam"',
                               m.index, 'spam')

    def test_invoke(self):
        m = self.create(tearoff=False)
        commands = []
        m.add_command(label='Command',
                      command=lambda: commands.append('invoked'))
        var = tkinter.IntVar(self.root)
        m.add_checkbutton(label='Check', variable=var,
                          onvalue=1, offvalue=0)
        rvar = tkinter.StringVar(self.root)
        m.add_radiobutton(label='Radio', variable=rvar, value='on')

        m.invoke(0)
        self.assertEqual(commands, ['invoked'])
        m.invoke(1)
        self.assertEqual(var.get(), 1)
        m.invoke(1)
        self.assertEqual(var.get(), 0)
        m.invoke(2)
        self.assertEqual(rvar.get(), 'on')
        self.assertRaisesRegex(TclError, 'bad menu entry index "spam"',
                               m.invoke, 'spam')

    def test_xposition_yposition(self):
        m = self.create(tearoff=False)
        m.add_command(label='First')
        m.add_command(label='Second')
        m.update_idletasks()
        self.assertIsInstance(m.xposition(0), int)
        y0 = m.yposition(0)
        y1 = m.yposition(1)
        self.assertIsInstance(y0, int)
        self.assertLess(y0, y1)
        # An out-of-range index gives the position past the last item.
        self.assertEqual(m.xposition('end'), m.xposition(1))
        self.assertRaisesRegex(TclError, 'bad menu entry index "spam"',
                               m.xposition, 'spam')
        self.assertRaisesRegex(TclError, 'bad menu entry index "spam"',
                               m.yposition, 'spam')

    def test_post_unpost(self):
        m = self.create(tearoff=False)
        if m._windowingsystem != 'x11':
            # Posting a menu is modal on Windows and uses a native, unmapped
            # menu on Aqua, so it cannot be tested synchronously there.
            self.skipTest('menu posting is not testable on this platform')
        m.add_command(label='First')
        m.add_command(label='Second')
        self.assertFalse(m.winfo_ismapped())

        m.post(0, 0)
        m.update()
        self.assertTrue(m.winfo_ismapped())
        m.unpost()
        m.update()
        self.assertFalse(m.winfo_ismapped())

        m.tk_popup(0, 0)
        m.update()
        self.assertTrue(m.winfo_ismapped())
        m.unpost()
        m.update()
        self.assertFalse(m.winfo_ismapped())

    def test_postcascade(self):
        m = self.create(tearoff=False)
        submenu = tkinter.Menu(m, tearoff=False)
        submenu.add_command(label='Item')
        m.add_cascade(label='Cascade', menu=submenu)
        m.add_command(label='Plain')
        # No effect (but no error) when the menu is not posted, when the index
        # is not a cascade entry, or when given a label.
        m.postcascade(0)
        m.postcascade(1)
        m.postcascade('Cascade')

        with self.subTest('posted menu'):
            if m._windowingsystem != 'x11':
                # Posting a menu is modal on Windows and uses a native,
                # unmapped menu on Aqua, so it cannot be tested synchronously
                # there.
                self.skipTest('menu posting is not testable on this platform')
            m.post(0, 0)
            m.update()
            m.postcascade('Cascade')
            m.update()
            self.assertTrue(submenu.winfo_ismapped())
            # A non-cascade index unposts the currently posted submenu.
            m.postcascade(1)
            m.update()
            self.assertFalse(submenu.winfo_ismapped())
            m.unpost()

    def check_entry_option(self, m, index, option, value, expected=None):
        if expected is None:
            expected = value
        m.entryconfigure(index, **{option: value})
        self.assertEqual(str(m.entrycget(index, option)), str(expected))
        self.assertEqual(str(m.entryconfigure(index, option)[4]), str(expected))

    def test_entry_options(self):
        m = self.create(tearoff=False)
        m.add_command(label='Command')
        self.check_entry_option(m, 0, 'accelerator', 'Ctrl+O')
        self.check_entry_option(m, 0, 'underline', 2)
        self.check_entry_option(m, 0, 'state', 'disabled')
        self.check_entry_option(m, 0, 'background', 'red')
        self.check_entry_option(m, 0, 'foreground', 'blue')
        self.check_entry_option(m, 0, 'columnbreak', 1)
        self.check_entry_option(m, 0, 'hidemargin', 1)

        m.add_checkbutton(label='Checkbutton')
        self.check_entry_option(m, 1, 'onvalue', 'Y')
        self.check_entry_option(m, 1, 'offvalue', 'N')
        self.check_entry_option(m, 1, 'indicatoron', 0)

        m.add_radiobutton(label='Radiobutton')
        self.check_entry_option(m, 2, 'value', 'V')
        self.check_entry_option(m, 2, 'selectcolor', 'green')

    def test_entry_options_invalid(self):
        m = self.create(tearoff=False)
        m.add_command(label='Command')
        self.assertRaisesRegex(TclError, 'unknown option "-spam"',
                               m.entrycget, 0, 'spam')
        self.assertRaisesRegex(TclError, 'unknown option "-spam"',
                               m.entryconfigure, 0, spam='x')
        self.assertRaisesRegex(TclError, 'bad state "spam"',
                               m.entryconfigure, 0, state='spam')
        # Tk < 9 reports "expected integer but got ...", while Tk 9, where
        # underline accepts an index, reports "bad index ...".
        self.assertRaisesRegex(TclError,
                               r'(expected integer but got|bad index) "spam"',
                               m.entryconfigure, 0, underline='spam')
        self.assertRaisesRegex(TclError, 'unknown color name "spam"',
                               m.entryconfigure, 0, background='spam')
        self.assertRaisesRegex(TclError, 'expected boolean value but got "spam"',
                               m.entryconfigure, 0, columnbreak='spam')
        # onvalue applies only to checkbutton and radiobutton entries.
        self.assertRaisesRegex(TclError, 'unknown option "-onvalue"',
                               m.entrycget, 0, 'onvalue')


@add_configure_tests(PixelSizeTests, StandardOptionsTests)
class MessageTest(AbstractWidgetTest, unittest.TestCase):
    OPTIONS = (
        'anchor', 'aspect', 'background', 'borderwidth',
        'cursor', 'font', 'foreground',
        'highlightbackground', 'highlightcolor', 'highlightthickness',
        'justify', 'padx', 'pady', 'relief',
        'takefocus', 'text', 'textvariable', 'width',
    )
    _no_round = {'padx', 'pady'}
    if tk_version < (8, 7):
        _clipped = {'highlightthickness'}
    else:
        _clipped = {'borderwidth', 'highlightthickness', 'padx', 'pady', 'width'}
    _clipped_to_default = {'padx', 'pady'}

    def create(self, **kwargs):
        return tkinter.Message(self.root, **kwargs)

    def test_configure_aspect(self):
        widget = self.create()
        self.checkIntegerParam(widget, 'aspect', 250, 0, -300)


class DefaultRootTest(AbstractDefaultRootTest, unittest.TestCase):

    def test_frame(self):
        self._test_widget(tkinter.Frame)

    def test_label(self):
        self._test_widget(tkinter.Label)


if __name__ == '__main__':
    unittest.main()
