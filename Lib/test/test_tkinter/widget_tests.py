# Common tests for test_tkinter/test_widgets.py and test_ttk/test_widgets.py

import re
import tkinter
from test.test_tkinter.support import (AbstractTkTest, requires_tk, tk_version,
                                  pixels_conv, tcl_obj_eq)
import test.support

_sentinel = object()

# Options which accept all values allowed by Tk_GetPixels
# borderwidth = bd

class AbstractWidgetTest(AbstractTkTest):
    _default_pixels = ''   # Value for unset pixel options.
    _rounds_pixels = True  # True if some pixel options are rounded.
    _no_round = {}         # Pixel options which are not rounded nonetheless
    _stringify = False     # Whether to convert tuples to strings
    _allow_empty_justify = False

    @property
    def scaling(self):
        try:
            return self._scaling
        except AttributeError:
            self._scaling = float(self.root.call('tk', 'scaling'))
            return self._scaling

    def _str(self, value):
        if not self._stringify and self.wantobjects and tk_version >= (8, 6):
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
        if name in self._clipped:
            if not isinstance(expected, str):
                expected = max(expected, 0)
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

    def checkInvalidParam(self, widget, name, value, errmsg=None):
        orig = widget[name]
        if errmsg is not None:
            errmsg = errmsg.format(re.escape(str(value)))
            errmsg = fr'\A{errmsg}\z'
        with self.assertRaisesRegex(tkinter.TclError, errmsg or ''):
            widget[name] = value
        self.assertEqual(widget[name], orig)
        with self.assertRaisesRegex(tkinter.TclError, errmsg or ''):
            widget.configure({name: value})
        self.assertEqual(widget[name], orig)

    def checkParams(self, widget, name, *values, **kwargs):
        for value in values:
            self.checkParam(widget, name, value, **kwargs)

    def checkIntegerParam(self, widget, name, *values, **kwargs):
        self.checkParams(widget, name, *values, **kwargs)
        errmsg = 'expected integer but got "{}"'
        self.checkInvalidParam(widget, name, '', errmsg=errmsg)
        self.checkInvalidParam(widget, name, '10p', errmsg=errmsg)
        self.checkInvalidParam(widget, name, 3.2, errmsg=errmsg)

    def checkFloatParam(self, widget, name, *values, conv=float, **kwargs):
        for value in values:
            self.checkParam(widget, name, value, conv=conv, **kwargs)
        errmsg = 'expected floating-point number but got "{}"'
        self.checkInvalidParam(widget, name, '', errmsg=errmsg)
        self.checkInvalidParam(widget, name, 'spam', errmsg=errmsg)

    def checkBooleanParam(self, widget, name):
        for value in (False, 0, 'false', 'no', 'off'):
            self.checkParam(widget, name, value, expected=0)
        for value in (True, 1, 'true', 'yes', 'on'):
            self.checkParam(widget, name, value, expected=1)
        errmsg = 'expected boolean value but got "{}"'
        self.checkInvalidParam(widget, name, '', errmsg=errmsg)
        self.checkInvalidParam(widget, name, 'spam', errmsg=errmsg)

    def checkColorParam(self, widget, name, *, allow_empty=None, **kwargs):
        self.checkParams(widget, name,
                         '#ff0000', '#00ff00', '#0000ff', '#123456',
                         'red', 'green', 'blue', 'white', 'black', 'grey',
                         **kwargs)
        self.checkInvalidParam(widget, name, 'spam',
                errmsg='unknown color name "spam"')

    def checkCursorParam(self, widget, name, **kwargs):
        self.checkParams(widget, name, 'arrow', 'watch', 'cross', '',**kwargs)
        self.checkParam(widget, name, 'none')
        self.checkInvalidParam(widget, name, 'spam',
                errmsg='bad cursor spec "spam"')

    def checkCommandParam(self, widget, name):
        def command(*args):
            pass
        widget[name] = command
        self.assertTrue(widget[name])
        self.checkParams(widget, name, '')

    def checkEnumParam(self, widget, name, *values,
                       errmsg=None, allow_empty=False, fullname=None,
                       sort=False, **kwargs):
        self.checkParams(widget, name, *values, **kwargs)
        if errmsg is None:
            if sort:
                if values[-1]:
                    values = tuple(sorted(values))
                else:
                    values = tuple(sorted(values[:-1])) + ('',)
            errmsg2 = ' %s "{}": must be %s%s or %s' % (
                    fullname or name,
                    ', '.join(values[:-1]),
                    ',' if len(values) > 2 else '',
                    values[-1] or '""')
            if '' not in values and not allow_empty:
                self.checkInvalidParam(widget, name, '',
                                       errmsg='ambiguous' + errmsg2)
            errmsg = 'bad' + errmsg2
        self.checkInvalidParam(widget, name, 'spam', errmsg=errmsg)

    def checkPixelsParam(self, widget, name, *values, conv=None, **kwargs):
        if not self._rounds_pixels or name in self._no_round:
            conv = False
        elif conv != str:
            conv = round
        for value in values:
            expected = _sentinel
            conv1 = conv
            if isinstance(value, str):
                if not getattr(self, '_converts_pixels', True):
                    conv1 = str
                if conv1 and conv1 is not str:
                    expected = pixels_conv(value) * self.scaling
                    conv1 = round
            self.checkParam(widget, name, value, expected=expected,
                            conv=conv1, **kwargs)
        errmsg = '(bad|expected) screen distance ((or "" )?but got )?"{}"'
        self.checkInvalidParam(widget, name, '6x', errmsg=errmsg)
        self.checkInvalidParam(widget, name, 'spam', errmsg=errmsg)

    def checkReliefParam(self, widget, name, *, allow_empty=False):
        values = ('flat', 'groove', 'raised', 'ridge', 'solid', 'sunken')
        if allow_empty:
            values += ('',)
        self.checkParams(widget, name, *values)
        errmsg = 'bad relief "{}": must be %s, or %s' % (
                ', '.join(values[:-1]),
                values[-1] or '""')
        if tk_version < (8, 6):
            errmsg = None
        self.checkInvalidParam(widget, name, 'spam', errmsg=errmsg)

    def checkImageParam(self, widget, name):
        image = tkinter.PhotoImage(master=self.root, name='image1')
        self.checkParam(widget, name, image, conv=str)
        if tk_version < (9, 0):
            errmsg = 'image "spam" doesn\'t exist'
        else:
            errmsg = 'image "spam" does not exist'
        self.checkInvalidParam(widget, name, 'spam',
                               errmsg=errmsg)
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
                'bgimg': 'backgroundimage',
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

class PixelOptionsTests:
    """Standard options that accept all formats acceptable to Tk_GetPixels.

    In addition to numbers, these options can be set with distances
    specified as a string consisting of a number followed by a single
    character giving the unit of distance. The allowed units are:
    millimeters ('m'), centimeters ('c'), inches ('i') or points ('p').
    In Tk 9 a cget call for one of these options returns a Tcl_Obj of
    type "pixels", whose string representation is the distance string
    passed to configure.
    """
    PIXEL_OPTIONS = ('activeborderwidth', 'borderwidth', 'highlightthickness',
      'insertborderwidth', 'insertwidth', 'padx', 'pady', 'selectborderwidth')

    def test_configure_activeborderwidth(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'activeborderwidth',
                              0, 1.3, 2.9, 6, -2, '10p')

    def test_configure_borderwidth(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'borderwidth',
                              0, 1.3, 2.6, 6, '10p')
        if tk_version < (9, 0):
            self.checkParam(widget, 'borderwidth', -2)
        else:
            self.checkParam(widget, 'borderwidth', 0)

        if 'bd' in self.OPTIONS:
            self.checkPixelsParam(widget, 'bd', 0, 1.3, 2.6, 6, '10p')
            self.checkParam(widget, 'bd', -2, expected=expected)

    def test_configure_highlightthickness(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'highlightthickness',
                              0, 1.3, 2.6, 6, '10p')
        self.checkParam(widget, 'highlightthickness', -2)

    def test_configure_insertborderwidth(self):
        widget = self.create()
        if tk_version < (9, 0):
            values = (0, 1.3, 2.6, 6, -2, '10p')
            value = -2
        else:
            values = (0, 1, 3, 6, 13)
            value = 0
        self.checkPixelsParam(widget, 'insertborderwidth', *values)
        self.checkParam(widget, 'insertborderwidth', value)

    def test_configure_insertwidth(self):
        widget = self.create()
        if tk_version < (9, 0):
            self.checkPixelsParam(widget, 'insertwidth', 1.3, 2.6, -2, '10p')
        else:
            self.checkPixelsParam(widget, 'insertwidth', 1, 3, 0, 13)

    def test_configure_padx(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'padx', 3, 4.4, 5.6, '12m')
        if tk_version < (9, 0):
            self.checkParam(widget, 'padx', -2)
        else:
            self.checkParam(widget, 'padx', 0)

    def test_configure_pady(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'pady', 3, 4.4, 5.6, '12m')
        if tk_version < (9, 0):
            self.checkParam(widget, 'pady', -2)
        else:
            self.checkParam(widget, 'pady', 0)

    def test_configure_selectborderwidth(self):
        widget = self.create()
        if tk_version < (9, 0):
            values = (1.3, 2.6, -2, '10p')
        else:
            values = (1, 3, 0, 13)
        self.checkPixelsParam(widget, 'selectborderwidth', *values)


class StandardOptionsTests(PixelOptionsTests):

    STANDARD_OPTIONS = ( 'activebackground', 'activeforeground',
    'anchor', 'background', 'bitmap', 'compound', 'cursor',
    'disabledforeground', 'exportselection', 'font', 'foreground',
    'highlightbackground', 'highlightcolor', 'image',
    'insertbackground', 'insertofftime', 'insertontime', 'jump',
    'justify', 'orient', 'relief', 'repeatdelay', 'repeatinterval',
    'selectbackground', 'selectforeground', 'setgrid', 'takefocus',
    'text', 'textvariable', 'troughcolor', 'underline', 'wraplength',
    'xscrollcommand', 'yscrollcommand', ) + PixelOptionsTests.PIXEL_OPTIONS

    def test_configure_activebackground(self):
        widget = self.create()
        self.checkColorParam(widget, 'activebackground')

    def test_configure_activeforeground(self):
        widget = self.create()
        self.checkColorParam(widget, 'activeforeground')

    def test_configure_activerelief(self):
        widget = self.create()
        self.checkReliefParam(widget, 'activerelief')

    def test_configure_anchor(self):
        widget = self.create()
        self.checkEnumParam(widget, 'anchor',
                'n', 'ne', 'e', 'se', 's', 'sw', 'w', 'nw', 'center')

    def test_configure_background(self):
        widget = self.create()
        self.checkColorParam(widget, 'background')
        if 'bg' in self.OPTIONS:
            self.checkColorParam(widget, 'bg')

    @requires_tk(8, 7)
    def test_configure_backgroundimage(self):
        widget = self.create()
        self.checkImageParam(widget, 'backgroundimage')

    def test_configure_bitmap(self):
        widget = self.create()
        self.checkParam(widget, 'bitmap', 'questhead')
        self.checkParam(widget, 'bitmap', 'gray50')
        filename = test.support.findfile('python.xbm', subdir='tkinterdata')
        self.checkParam(widget, 'bitmap', '@' + filename)
        # Cocoa Tk widgets don't detect invalid -bitmap values
        # See https://core.tcl.tk/tk/info/31cd33dbf0
        if not ('aqua' in self.root.tk.call('tk', 'windowingsystem') and
                'AppKit' in self.root.winfo_server()):
            self.checkInvalidParam(widget, 'bitmap', 'spam',
                    errmsg='bitmap "spam" not defined')

    def test_configure_compound(self):
        widget = self.create()
        self.checkEnumParam(widget, 'compound',
                'bottom', 'center', 'left', 'none', 'right', 'top')

    def test_configure_cursor(self):
        widget = self.create()
        self.checkCursorParam(widget, 'cursor')

    def test_configure_disabledforeground(self):
        widget = self.create()
        self.checkColorParam(widget, 'disabledforeground')

    def test_configure_exportselection(self):
        widget = self.create()
        self.checkBooleanParam(widget, 'exportselection')

    def test_configure_font(self):
        widget = self.create()
        self.checkParam(widget, 'font',
                        '-Adobe-Helvetica-Medium-R-Normal--*-120-*-*-*-*-*-*')
        is_ttk = widget.__class__.__module__ == 'tkinter.ttk'
        if not is_ttk:
            errmsg = 'font "" does ?n[o\']t exist'
            self.checkInvalidParam(widget, 'font', '', errmsg=errmsg)

    def test_configure_foreground(self):
        widget = self.create()
        self.checkColorParam(widget, 'foreground')
        if 'fg' in self.OPTIONS:
            self.checkColorParam(widget, 'fg')

    def test_configure_highlightbackground(self):
        widget = self.create()
        self.checkColorParam(widget, 'highlightbackground')

    def test_configure_highlightcolor(self):
        widget = self.create()
        self.checkColorParam(widget, 'highlightcolor')

    def test_configure_image(self):
        widget = self.create()
        self.checkImageParam(widget, 'image')

    def test_configure_insertbackground(self):
        widget = self.create()
        self.checkColorParam(widget, 'insertbackground')

    def test_configure_insertofftime(self):
        widget = self.create()
        self.checkIntegerParam(widget, 'insertofftime', 100)

    def test_configure_insertontime(self):
        widget = self.create()
        self.checkIntegerParam(widget, 'insertontime', 100)

    def test_configure_jump(self):
        widget = self.create()
        self.checkBooleanParam(widget, 'jump')

    def test_configure_justify(self):
        widget = self.create()
        values = ('left', 'right', 'center')
        if self._allow_empty_justify:
            values += ('',)
        self.checkEnumParam(widget, 'justify', *values,
                            fullname='justification')

    def test_configure_orient(self):
        widget = self.create()
        self.assertEqual(str(widget['orient']), self.default_orient)
        self.checkEnumParam(widget, 'orient', 'horizontal', 'vertical')

    @requires_tk(8, 7)
    def test_configure_placeholder(self):
        widget = self.create()
        self.checkParam(widget, 'placeholder', 'xxx')

    @requires_tk(8, 7)
    def test_configure_placeholderforeground(self):
        widget = self.create()
        self.checkColorParam(widget, 'placeholderforeground')

    def test_configure_relief(self):
        widget = self.create()
        self.checkReliefParam(widget, 'relief')

    def test_configure_repeatdelay(self):
        widget = self.create()
        self.checkIntegerParam(widget, 'repeatdelay', -500, 500)

    def test_configure_repeatinterval(self):
        widget = self.create()
        self.checkIntegerParam(widget, 'repeatinterval', -500, 500)

    def test_configure_selectbackground(self):
        widget = self.create()
        self.checkColorParam(widget, 'selectbackground')

    def test_configure_selectforeground(self):
        widget = self.create()
        self.checkColorParam(widget, 'selectforeground')

    def test_configure_setgrid(self):
        widget = self.create()
        self.checkBooleanParam(widget, 'setgrid')

    def test_configure_state(self):
        widget = self.create()
        self.checkEnumParam(widget, 'state', 'active', 'disabled', 'normal')

    def test_configure_takefocus(self):
        widget = self.create()
        self.checkParams(widget, 'takefocus', '0', '1', '')

    def test_configure_text(self):
        widget = self.create()
        self.checkParams(widget, 'text', '', 'any string')

    def test_configure_textvariable(self):
        widget = self.create()
        var = tkinter.StringVar(self.root)
        self.checkVariableParam(widget, 'textvariable', var)

    @requires_tk(8, 7)
    def test_configure_tile(self):
        widget = self.create()
        self.checkBooleanParam(widget, 'tile')

    def test_configure_troughcolor(self):
        widget = self.create()
        self.checkColorParam(widget, 'troughcolor')

    def test_configure_underline(self):
        widget = self.create()
        self.checkParams(widget, 'underline', 0, 1, 10)
        if tk_version >= (8, 7):
            is_ttk = widget.__class__.__module__ == 'tkinter.ttk'
            self.checkParam(widget, 'underline', '',
                            expected='' if is_ttk else self._default_pixels)
            self.checkParam(widget, 'underline', '5+2',
                            expected='5+2' if is_ttk else 7)
            self.checkParam(widget, 'underline', '5-2',
                            expected='5-2' if is_ttk else 3)
            self.checkParam(widget, 'underline', 'end', expected='end')
            self.checkParam(widget, 'underline', 'end-2', expected='end-2')
            errmsg = (r'bad index "{}": must be integer\?\[\+-\]integer\?, '
                      r'end\?\[\+-\]integer\?, or ""')
        else:
            errmsg = 'expected integer but got "{}"'
            self.checkInvalidParam(widget, 'underline', '', errmsg=errmsg)
        self.checkInvalidParam(widget, 'underline', '10p', errmsg=errmsg)
        self.checkInvalidParam(widget, 'underline', 3.2, errmsg=errmsg)

    def test_configure_wraplength(self):
        widget = self.create()
        self.checkPixelsParam(widget, 'wraplength', 100)

    def test_configure_xscrollcommand(self):
        widget = self.create()
        self.checkCommandParam(widget, 'xscrollcommand')

    def test_configure_yscrollcommand(self):
        widget = self.create()
        self.checkCommandParam(widget, 'yscrollcommand')

    # non-standard but common options

    def test_configure_command(self):
        widget = self.create()
        self.checkCommandParam(widget, 'command')

    def test_configure_indicatoron(self):
        widget = self.create()
        self.checkBooleanParam(widget, 'indicatoron')

    def test_configure_offrelief(self):
        widget = self.create()
        self.checkReliefParam(widget, 'offrelief')

    def test_configure_overrelief(self):
        widget = self.create()
        self.checkReliefParam(widget, 'overrelief',
                              allow_empty=(tk_version >= (8, 7)))

    def test_configure_selectcolor(self):
        widget = self.create()
        self.checkColorParam(widget, 'selectcolor')

    def test_configure_selectimage(self):
        widget = self.create()
        self.checkImageParam(widget, 'selectimage')

    def test_configure_tristateimage(self):
        widget = self.create()
        self.checkImageParam(widget, 'tristateimage')

    def test_configure_tristatevalue(self):
        widget = self.create()
        self.checkParam(widget, 'tristatevalue', 'unknowable')

    def test_configure_variable(self):
        widget = self.create()
        var = tkinter.DoubleVar(self.root)
        self.checkVariableParam(widget, 'variable', var)


class IntegerSizeTests:
    """ Tests widgets which only accept integral width and height."""
    def test_configure_height(self):
        widget = self.create()
        if tk_version < (9, 0):
            self.checkIntegerParam(widget, 'height', 100, -100, 0)
        else:
            self.checkIntegerParam(widget, 'height', 100, 0, 0)

    def test_configure_width(self):
        widget = self.create()
        if tk_version < (9, 0):
            self.checkIntegerParam(widget, 'width', 402, -402, 0)
        else:
            self.checkIntegerParam(widget, 'width', 402, 0, 0)


class PixelSizeTests:
    """ Tests widgets which accept screen distances for width and height."""
    def test_configure_height(self):
        widget = self.create()
        value = -100 if tk_version < (9, 0) else 0
        self.checkPixelsParam(
            widget, 'height', 100, 101.2, 102.6, value, 0, '3c'
        )

    def test_configure_width(self):
        widget = self.create()
        value = -402 if tk_version < (9, 0) else 0
        self.checkPixelsParam(
            widget, 'width', 402, 403.4, 404.6, value, 0, '5i'
        )


def add_configure_tests(*source_classes):
    # This decorator adds test_configure_xxx methods from source classes for
    # every xxx option in the OPTIONS class attribute if they are not defined
    # explicitly.
    def decorator(cls):
        for option in cls.OPTIONS:
            methodname = 'test_configure_' + option
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
        print('patchlevel =', tcl.call('info', 'patchlevel'), flush=True)
