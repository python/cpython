# Common tests for test_tkinter/test_widgets.py and test_ttk/test_widgets.py

import unittest
import tkinter
from tkinter.test.support import (AbstractTkTest, tcl_version, requires_tcl,
                                  get_tk_patchlevel, pixels_conv, tcl_obj_eq)
import test.support


noconv = False
if get_tk_patchlevel() < (8, 5, 11):
    noconv = str

pixels_round = round
if get_tk_patchlevel()[:3] == (8, 5, 11):
    # Issue #19085: Workaround a bug in Tk
    # http://core.tcl.tk/tk/info/3497848
    pixels_round = int


_sentinel = object()

class AbstractWidgetTest(AbstractTkTest):
    _conv_pixels = staticmethod(pixels_round)
    _conv_pad_pixels = None
    _stringify = False

    @property
    def scaling(self):
        try:
            return self._scaling
        except AttributeError:
            self._scaling = float(self.root.call('tk', 'scaling'))
            return self._scaling

    def _str(self, value):
        if not self._stringify and self.wantobjects and tcl_version >= (8, 6):
            return value
        if isinstance(value, tuple):
            return ' '.join(map(self._str, value))
        return str(value)

    def assertEqual2(self, actual, expected, msg=None, eq=object.__eq__):
        if eq(actual, expected):
            return
        self.assertEqual(actual, expected, msg)

    def checkParam(self, widget, name, value, *, expected=_sentinel,
                   conv=False, eq=None):
        widget[name] = value
        if expected is _sentinel:
            expected = value
        if conv:
            expected = conv(expected)
        if self._stringify or not self.wantobjects:
            if isinstance(expected, tuple):
                expected = tkinter._join(expected)
            else:
                expected = str(expected)
        if eq is None:
            eq = tcl_obj_eq
        self.assertEqual2(widget[name], expected, eq=eq)
        self.assertEqual2(widget.cget(name), expected, eq=eq)
        t = widget.configure(name)
        self.assertEqual(len(t), 5)
        self.assertEqual2(t[4], expected, eq=eq)

    def checkInvalidParam(self, widget, name, value, errmsg=None, *,
                          keep_orig=True):
        orig = widget[name]
        if errmsg is not None:
            errmsg = errmsg.format(value)
        with self.assertRaises(tkinter.TclError) as cm:
            widget[name] = value
        if errmsg is not None:
            self.assertEqual(str(cm.exception), errmsg)
        if keep_orig:
            self.assertEqual(widget[name], orig)
        else:
            widget[name] = orig
        with self.assertRaises(tkinter.TclError) as cm:
            widget.configure({name: value})
        if errmsg is not None:
            self.assertEqual(str(cm.exception), errmsg)
        if keep_orig:
            self.assertEqual(widget[name], orig)
        else:
            widget[name] = orig

    def checkParams(self, widget, name, *values, **kwargs):
        for value in values:
            self.checkParam(widget, name, value, **kwargs)

    def checkIntegerParam(self, widget, name, *values, **kwargs):
        self.checkParams(widget, name, *values, **kwargs)
        self.checkInvalidParam(widget, name, '',
                errmsg='expected integer but got ""')
        self.checkInvalidParam(widget, name, '10p',
                errmsg='expected integer but got "10p"')
        self.checkInvalidParam(widget, name, 3.2,
                errmsg='expected integer but got "3.2"')

    def checkFloatParam(self, widget, name, *values, conv=float, **kwargs):
        for value in values:
            self.checkParam(widget, name, value, conv=conv, **kwargs)
        self.checkInvalidParam(widget, name, '',
                errmsg='expected floating-point number but got ""')
        self.checkInvalidParam(widget, name, 'spam',
                errmsg='expected floating-point number but got "spam"')

    def checkBooleanParam(self, widget, name):
        for value in (False, 0, 'false', 'no', 'off'):
            self.checkParam(widget, name, value, expected=0)
        for value in (True, 1, 'true', 'yes', 'on'):
            self.checkParam(widget, name, value, expected=1)
        self.checkInvalidParam(widget, name, '',
                errmsg='expected boolean value but got ""')
        self.checkInvalidParam(widget, name, 'spam',
                errmsg='expected boolean value but got "spam"')

    def checkColorParam(self, widget, name, *, allow_empty=None, **kwargs):
        self.checkParams(widget, name,
                         '#ff0000', '#00ff00', '#0000ff', '#123456',
                         'red', 'green', 'blue', 'white', 'black', 'grey',
                         **kwargs)
        self.checkInvalidParam(widget, name, 'spam',
                errmsg='unknown color name "spam"')

    def checkCursorParam(self, widget, name, **kwargs):
        self.checkParams(widget, name, 'arrow', 'watch', 'cross', '',**kwargs)
        if tcl_version >= (8, 5):
            self.checkParam(widget, name, 'none')
        self.checkInvalidParam(widget, name, 'spam',
                errmsg='bad cursor spec "spam"')

    def checkCommandParam(self, widget, name):
        def command(*args):
            pass
        widget[name] = command
        self.assertTrue(widget[name])
        self.checkParams(widget, name, '')

    def checkEnumParam(self, widget, name, *values, errmsg=None, **kwargs):
        self.checkParams(widget, name, *values, **kwargs)
        if errmsg is None:
            errmsg2 = ' %s "{}": must be %s%s or %s' % (
                    name,
                    ', '.join(values[:-1]),
                    ',' if len(values) > 2 else '',
                    values[-1])
            self.checkInvalidParam(widget, name, '',
                                   errmsg='ambiguous' + errmsg2)
            errmsg = 'bad' + errmsg2
        self.checkInvalidParam(widget, name, 'spam', errmsg=errmsg)

    def checkPixelsParam(self, widget, name, *values,
                         conv=None, keep_orig=True, **kwargs):
        if conv is None:
            conv = self._conv_pixels
        for value in values:
            expected = _sentinel
            conv1 = conv
            if isinstance(value, str):
                if conv1 and conv1 is not str:
                    expected = pixels_conv(value) * self.scaling
                    conv1 = round
            self.checkParam(widget, name, value, expected=expected,
                            conv=conv1, **kwargs)
        self.checkInvalidParam(widget, name, '6x',
                errmsg='bad screen distance "6x"', keep_orig=keep_orig)
        self.checkInvalidParam(widget, name, 'spam',
                errmsg='bad screen distance "spam"', keep_orig=keep_orig)

    def checkReliefParam(self, widget, name):
        self.checkParams(widget, name,
                         'flat', 'groove', 'raised', 'ridge', 'solid', 'sunken')
        errmsg='bad relief "spam": must be '\
               'flat, groove, raised, ridge, solid, or sunken'
        if tcl_version < (8, 6):
            errmsg = None
        self.checkInvalidParam(widget, name, 'spam',
                errmsg=errmsg)

    def checkImageParam(self, widget, name):
        image = tkinter.PhotoImage(master=self.root, name='image1')
        self.checkParam(widget, name, image, conv=str)
        self.checkInvalidParam(widget, name, 'spam',
                errmsg='image "spam" doesn\'t exist')
        widget[name] = ''

    def checkVariableParam(self, widget, name, var):
        self.checkParam(widget, name, var, conv=str)

    def assertIsBoundingBox(self, bbox):
        self.assertIsNotNone(bbox)
        self.assertIsInstance(bbox, tuple)
        if len(bbox) != 4:
            self.fail('Invalid bounding box: %r' % (bbox,))
        for item in bbox:
            if not isinstance(item, int):
                self.fail('Invalid bounding box: %r' % (bbox,))
                break


    def test_keys(self):
        widget = self.create()
        keys = widget.keys()
        self.assertEqual(sorted(keys), sorted(widget.configure()))
        for k in keys:
            widget[k]
        # Test if OPTIONS contains all keys
        if test.support.verbose:
            aliases = {
                'bd': 'borderwidth',
                'bg': 'background',
                'fg': 'foreground',
                'invcmd': 'invalidcommand',
                'vcmd': 'validatecommand',
            }
            keys = set(keys)
            expected = set(self.OPTIONS)
            for k in sorted(keys - expected):
                if not (k in aliases and
                        aliases[k] in keys and
                        aliases[k] in expected):
                    print('%s.OPTIONS doesn\'t contain "%s"' %
                          (self.__class__.__name__, k))


class StandardOptionsTests:
    STANDARD_OPTIONS = (
        'activebackground', 'activeborderwidth', 'activeforeground', 'anchor',
        'background', 'bitmap', 'borderwidth', 'compound', 'cursor',
        'disabledforeground', 'exportselection', 'font', 'foreground',
        'highlightbackground', 'highlightcolor', 'highlightthickness',
        'image', 'insertbackground', 'insertborderwidth',
        'insertofftime', 'insertontime', 'insertwidth',
        'jump', 'justify', 'orient', 'padx', 'pady', 'relief',
        'repeatdelay', 'repeatinterval',
        'selectbackground', 'selectborderwidth', 'selectforeground',
        'setgrid', 'takefocus', 'text', 'textvariable', 'troughcolor',
        'underline', 'wraplength', 'xscrollcommand', 'yscrollcommand',
    )

    def test_activebackground(self):
        widget = self.create()
        self.checkColorParam(widget, 'activebackground')

    def test_activeborderwidth(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'activeborderwidth',
                              0, 1.3, 2.9, 6, -2, '10p')

    def test_activeforeground(self):
        widget = self.create()
        self.checkColorParam(widget, 'activeforeground')

    def test_anchor(self):
        widget = self.create()
        self.checkEnumParam(widget, 'anchor',
                'n', 'ne', 'e', 'se', 's', 'sw', 'w', 'nw', 'center')

    def test_background(self):
        widget = self.create()
        self.checkColorParam(widget, 'background')
        if 'bg' in self.OPTIONS:
            self.checkColorParam(widget, 'bg')

    def test_bitmap(self):
        widget = self.create()
        self.checkParam(widget, 'bitmap', 'questhead')
        self.checkParam(widget, 'bitmap', 'gray50')
        filename = test.support.findfile('python.xbm', subdir='imghdrdata')
        self.checkParam(widget, 'bitmap', '@' + filename)
        # Cocoa Tk widgets don't detect invalid -bitmap values
        # See https://core.tcl.tk/tk/info/31cd33dbf0
        if not ('aqua' in self.root.tk.call('tk', 'windowingsystem') and
                'AppKit' in self.root.winfo_server()):
            self.checkInvalidParam(widget, 'bitmap', 'spam',
                    errmsg='bitmap "spam" not defined')

    def test_borderwidth(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'borderwidth',
                              0, 1.3, 2.6, 6, -2, '10p')
        if 'bd' in self.OPTIONS:
            self.checkPixelsParam(widget, 'bd', 0, 1.3, 2.6, 6, -2, '10p')

    def test_compound(self):
        widget = self.create()
        self.checkEnumParam(widget, 'compound',
                'bottom', 'center', 'left', 'none', 'right', 'top')

    def test_cursor(self):
        widget = self.create()
        self.checkCursorParam(widget, 'cursor')

    def test_disabledforeground(self):
        widget = self.create()
        self.checkColorParam(widget, 'disabledforeground')

    def test_exportselection(self):
        widget = self.create()
        self.checkBooleanParam(widget, 'exportselection')

    def test_font(self):
        widget = self.create()
        self.checkParam(widget, 'font',
                        '-Adobe-Helvetica-Medium-R-Normal--*-120-*-*-*-*-*-*')
        self.checkInvalidParam(widget, 'font', '',
                               errmsg='font "" doesn\'t exist')

    def test_foreground(self):
        widget = self.create()
        self.checkColorParam(widget, 'foreground')
        if 'fg' in self.OPTIONS:
            self.checkColorParam(widget, 'fg')

    def test_highlightbackground(self):
        widget = self.create()
        self.checkColorParam(widget, 'highlightbackground')

    def test_highlightcolor(self):
        widget = self.create()
        self.checkColorParam(widget, 'highlightcolor')

    def test_highlightthickness(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'highlightthickness',
                              0, 1.3, 2.6, 6, '10p')
        self.checkParam(widget, 'highlightthickness', -2, expected=0,
                        conv=self._conv_pixels)

    def test_image(self):
        widget = self.create()
        self.checkImageParam(widget, 'image')

    def test_insertbackground(self):
        widget = self.create()
        self.checkColorParam(widget, 'insertbackground')

    def test_insertborderwidth(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'insertborderwidth',
                              0, 1.3, 2.6, 6, -2, '10p')

    def test_insertofftime(self):
        widget = self.create()
        self.checkIntegerParam(widget, 'insertofftime', 100)

    def test_insertontime(self):
        widget = self.create()
        self.checkIntegerParam(widget, 'insertontime', 100)

    def test_insertwidth(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'insertwidth', 1.3, 2.6, -2, '10p')

    def test_jump(self):
        widget = self.create()
        self.checkBooleanParam(widget, 'jump')

    def test_justify(self):
        widget = self.create()
        self.checkEnumParam(widget, 'justify', 'left', 'right', 'center',
                errmsg='bad justification "{}": must be '
                       'left, right, or center')
        self.checkInvalidParam(widget, 'justify', '',
                errmsg='ambiguous justification "": must be '
                       'left, right, or center')

    def test_orient(self):
        widget = self.create()
        self.assertEqual(str(widget['orient']), self.default_orient)
        self.checkEnumParam(widget, 'orient', 'horizontal', 'vertical')

    def test_padx(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'padx', 3, 4.4, 5.6, -2, '12m',
                              conv=self._conv_pad_pixels)

    def test_pady(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'pady', 3, 4.4, 5.6, -2, '12m',
                              conv=self._conv_pad_pixels)

    def test_relief(self):
        widget = self.create()
        self.checkReliefParam(widget, 'relief')

    def test_repeatdelay(self):
        widget = self.create()
        self.checkIntegerParam(widget, 'repeatdelay', -500, 500)

    def test_repeatinterval(self):
        widget = self.create()
        self.checkIntegerParam(widget, 'repeatinterval', -500, 500)

    def test_selectbackground(self):
        widget = self.create()
        self.checkColorParam(widget, 'selectbackground')

    def test_selectborderwidth(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'selectborderwidth', 1.3, 2.6, -2, '10p')

    def test_selectforeground(self):
        widget = self.create()
        self.checkColorParam(widget, 'selectforeground')

    def test_setgrid(self):
        widget = self.create()
        self.checkBooleanParam(widget, 'setgrid')

    def test_state(self):
        widget = self.create()
        self.checkEnumParam(widget, 'state', 'active', 'disabled', 'normal')

    def test_takefocus(self):
        widget = self.create()
        self.checkParams(widget, 'takefocus', '0', '1', '')

    def test_text(self):
        widget = self.create()
        self.checkParams(widget, 'text', '', 'any string')

    def test_textvariable(self):
        widget = self.create()
        var = tkinter.StringVar(self.root)
        self.checkVariableParam(widget, 'textvariable', var)

    def test_troughcolor(self):
        widget = self.create()
        self.checkColorParam(widget, 'troughcolor')

    def test_underline(self):
        widget = self.create()
        self.checkIntegerParam(widget, 'underline', 0, 1, 10)

    def test_wraplength(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'wraplength', 100)

    def test_xscrollcommand(self):
        widget = self.create()
        self.checkCommandParam(widget, 'xscrollcommand')

    def test_yscrollcommand(self):
        widget = self.create()
        self.checkCommandParam(widget, 'yscrollcommand')

    # non-standard but common options

    def test_command(self):
        widget = self.create()
        self.checkCommandParam(widget, 'command')

    def test_indicatoron(self):
        widget = self.create()
        self.checkBooleanParam(widget, 'indicatoron')

    def test_offrelief(self):
        widget = self.create()
        self.checkReliefParam(widget, 'offrelief')

    def test_overrelief(self):
        widget = self.create()
        self.checkReliefParam(widget, 'overrelief')

    def test_selectcolor(self):
        widget = self.create()
        self.checkColorParam(widget, 'selectcolor')

    def test_selectimage(self):
        widget = self.create()
        self.checkImageParam(widget, 'selectimage')

    @requires_tcl(8, 5)
    def test_tristateimage(self):
        widget = self.create()
        self.checkImageParam(widget, 'tristateimage')

    @requires_tcl(8, 5)
    def test_tristatevalue(self):
        widget = self.create()
        self.checkParam(widget, 'tristatevalue', 'unknowable')

    def test_variable(self):
        widget = self.create()
        var = tkinter.DoubleVar(self.root)
        self.checkVariableParam(widget, 'variable', var)


class IntegerSizeTests:
    def test_height(self):
        widget = self.create()
        self.checkIntegerParam(widget, 'height', 100, -100, 0)

    def test_width(self):
        widget = self.create()
        self.checkIntegerParam(widget, 'width', 402, -402, 0)


class PixelSizeTests:
    def test_height(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'height', 100, 101.2, 102.6, -100, 0, '3c')

    def test_width(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'width', 402, 403.4, 404.6, -402, 0, '5i')


def add_standard_options(*source_classes):
    # This decorator adds test_xxx methods from source classes for every xxx
    # option in the OPTIONS class attribute if they are not defined explicitly.
    def decorator(cls):
        for option in cls.OPTIONS:
            methodname = 'test_' + option
            if not hasattr(cls, methodname):
                for source_class in source_classes:
                    if hasattr(source_class, methodname):
                        setattr(cls, methodname,
                                getattr(source_class, methodname))
                        break
                else:
                    def test(self, option=option):
                        widget = self.create()
                        widget[option]
                        raise AssertionError('Option "%s" is not tested in %s' %
                                             (option, cls.__name__))
                    test.__name__ = methodname
                    setattr(cls, methodname, test)
        return cls
    return decorator

def setUpModule():
    if test.support.verbose:
        tcl = tkinter.Tcl()
        print('patchlevel =', tcl.call('info', 'patchlevel'))
