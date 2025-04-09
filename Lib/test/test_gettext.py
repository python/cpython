import os
import base64
import gettext
import unittest
import unittest.mock
from functools import partial

from test import support
from test.support import os_helper


# TODO:
#  - Add new tests, for example for "dgettext"
#  - Tests should have only one assert.

GNU_MO_DATA = b'''\
3hIElQAAAAAJAAAAHAAAAGQAAAAAAAAArAAAAAAAAACsAAAAFQAAAK0AAAAjAAAAwwAAAKEAAADn
AAAAMAAAAIkBAAAHAAAAugEAABYAAADCAQAAHAAAANkBAAALAAAA9gEAAEIBAAACAgAAFgAAAEUD
AAAeAAAAXAMAAKEAAAB7AwAAMgAAAB0EAAAFAAAAUAQAABsAAABWBAAAIQAAAHIEAAAJAAAAlAQA
AABSYXltb25kIEx1eHVyeSBZYWNoLXQAVGhlcmUgaXMgJXMgZmlsZQBUaGVyZSBhcmUgJXMgZmls
ZXMAVGhpcyBtb2R1bGUgcHJvdmlkZXMgaW50ZXJuYXRpb25hbGl6YXRpb24gYW5kIGxvY2FsaXph
dGlvbgpzdXBwb3J0IGZvciB5b3VyIFB5dGhvbiBwcm9ncmFtcyBieSBwcm92aWRpbmcgYW4gaW50
ZXJmYWNlIHRvIHRoZSBHTlUKZ2V0dGV4dCBtZXNzYWdlIGNhdGFsb2cgbGlicmFyeS4AV2l0aCBj
b250ZXh0BFRoZXJlIGlzICVzIGZpbGUAVGhlcmUgYXJlICVzIGZpbGVzAG11bGx1c2sAbXkgY29u
dGV4dARudWRnZSBudWRnZQBteSBvdGhlciBjb250ZXh0BG51ZGdlIG51ZGdlAG51ZGdlIG51ZGdl
AFByb2plY3QtSWQtVmVyc2lvbjogMi4wClBPLVJldmlzaW9uLURhdGU6IDIwMDMtMDQtMTEgMTQ6
MzItMDQwMApMYXN0LVRyYW5zbGF0b3I6IEouIERhdmlkIEliYW5leiA8ai1kYXZpZEBub29zLmZy
PgpMYW5ndWFnZS1UZWFtOiBYWCA8cHl0aG9uLWRldkBweXRob24ub3JnPgpNSU1FLVZlcnNpb246
IDEuMApDb250ZW50LVR5cGU6IHRleHQvcGxhaW47IGNoYXJzZXQ9aXNvLTg4NTktMQpDb250ZW50
LVRyYW5zZmVyLUVuY29kaW5nOiA4Yml0CkdlbmVyYXRlZC1CeTogcHlnZXR0ZXh0LnB5IDEuMQpQ
bHVyYWwtRm9ybXM6IG5wbHVyYWxzPTI7IHBsdXJhbD1uIT0xOwoAVGhyb2F0d29iYmxlciBNYW5n
cm92ZQBIYXkgJXMgZmljaGVybwBIYXkgJXMgZmljaGVyb3MAR3V2ZiB6YnFoeXIgY2ViaXZxcmYg
dmFncmVhbmd2YmFueXZtbmd2YmEgbmFxIHlicG55dm1uZ3ZiYQpmaGNjYmVnIHNiZSBsYmhlIENs
Z3ViYSBjZWJ0ZW56ZiBvbCBjZWJpdnF2YXQgbmEgdmFncmVzbnByIGdiIGd1ciBUQUgKdHJnZ3Jr
ZyB6cmZmbnRyIHBuZ255YnQgeXZvZW5lbC4ASGF5ICVzIGZpY2hlcm8gKGNvbnRleHQpAEhheSAl
cyBmaWNoZXJvcyAoY29udGV4dCkAYmFjb24Ad2luayB3aW5rIChpbiAibXkgY29udGV4dCIpAHdp
bmsgd2luayAoaW4gIm15IG90aGVyIGNvbnRleHQiKQB3aW5rIHdpbmsA
'''

# .mo file with an invalid magic number
GNU_MO_DATA_BAD_MAGIC_NUMBER = base64.b64encode(b'ABCD')

# This data contains an invalid major version number (5)
# An unexpected major version number should be treated as an error when
# parsing a .mo file

GNU_MO_DATA_BAD_MAJOR_VERSION = b'''\
3hIElQAABQAGAAAAHAAAAEwAAAALAAAAfAAAAAAAAACoAAAAFQAAAKkAAAAjAAAAvwAAAKEAAADj
AAAABwAAAIUBAAALAAAAjQEAAEUBAACZAQAAFgAAAN8CAAAeAAAA9gIAAKEAAAAVAwAABQAAALcD
AAAJAAAAvQMAAAEAAAADAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAAAABQAAAAYAAAACAAAAAFJh
eW1vbmQgTHV4dXJ5IFlhY2gtdABUaGVyZSBpcyAlcyBmaWxlAFRoZXJlIGFyZSAlcyBmaWxlcwBU
aGlzIG1vZHVsZSBwcm92aWRlcyBpbnRlcm5hdGlvbmFsaXphdGlvbiBhbmQgbG9jYWxpemF0aW9u
CnN1cHBvcnQgZm9yIHlvdXIgUHl0aG9uIHByb2dyYW1zIGJ5IHByb3ZpZGluZyBhbiBpbnRlcmZh
Y2UgdG8gdGhlIEdOVQpnZXR0ZXh0IG1lc3NhZ2UgY2F0YWxvZyBsaWJyYXJ5LgBtdWxsdXNrAG51
ZGdlIG51ZGdlAFByb2plY3QtSWQtVmVyc2lvbjogMi4wClBPLVJldmlzaW9uLURhdGU6IDIwMDAt
MDgtMjkgMTI6MTktMDQ6MDAKTGFzdC1UcmFuc2xhdG9yOiBKLiBEYXZpZCBJYsOhw7FleiA8ai1k
YXZpZEBub29zLmZyPgpMYW5ndWFnZS1UZWFtOiBYWCA8cHl0aG9uLWRldkBweXRob24ub3JnPgpN
SU1FLVZlcnNpb246IDEuMApDb250ZW50LVR5cGU6IHRleHQvcGxhaW47IGNoYXJzZXQ9aXNvLTg4
NTktMQpDb250ZW50LVRyYW5zZmVyLUVuY29kaW5nOiBub25lCkdlbmVyYXRlZC1CeTogcHlnZXR0
ZXh0LnB5IDEuMQpQbHVyYWwtRm9ybXM6IG5wbHVyYWxzPTI7IHBsdXJhbD1uIT0xOwoAVGhyb2F0
d29iYmxlciBNYW5ncm92ZQBIYXkgJXMgZmljaGVybwBIYXkgJXMgZmljaGVyb3MAR3V2ZiB6YnFo
eXIgY2ViaXZxcmYgdmFncmVhbmd2YmFueXZtbmd2YmEgbmFxIHlicG55dm1uZ3ZiYQpmaGNjYmVn
IHNiZSBsYmhlIENsZ3ViYSBjZWJ0ZW56ZiBvbCBjZWJpdnF2YXQgbmEgdmFncmVzbnByIGdiIGd1
ciBUQUgKdHJnZ3JrZyB6cmZmbnRyIHBuZ255YnQgeXZvZW5lbC4AYmFjb24Ad2luayB3aW5rAA==
'''

# This data contains an invalid minor version number (7)
# An unexpected minor version number only indicates that some of the file's
# contents may not be able to be read. It does not indicate an error.

GNU_MO_DATA_BAD_MINOR_VERSION = b'''\
3hIElQcAAAAGAAAAHAAAAEwAAAALAAAAfAAAAAAAAACoAAAAFQAAAKkAAAAjAAAAvwAAAKEAAADj
AAAABwAAAIUBAAALAAAAjQEAAEUBAACZAQAAFgAAAN8CAAAeAAAA9gIAAKEAAAAVAwAABQAAALcD
AAAJAAAAvQMAAAEAAAADAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAAAABQAAAAYAAAACAAAAAFJh
eW1vbmQgTHV4dXJ5IFlhY2gtdABUaGVyZSBpcyAlcyBmaWxlAFRoZXJlIGFyZSAlcyBmaWxlcwBU
aGlzIG1vZHVsZSBwcm92aWRlcyBpbnRlcm5hdGlvbmFsaXphdGlvbiBhbmQgbG9jYWxpemF0aW9u
CnN1cHBvcnQgZm9yIHlvdXIgUHl0aG9uIHByb2dyYW1zIGJ5IHByb3ZpZGluZyBhbiBpbnRlcmZh
Y2UgdG8gdGhlIEdOVQpnZXR0ZXh0IG1lc3NhZ2UgY2F0YWxvZyBsaWJyYXJ5LgBtdWxsdXNrAG51
ZGdlIG51ZGdlAFByb2plY3QtSWQtVmVyc2lvbjogMi4wClBPLVJldmlzaW9uLURhdGU6IDIwMDAt
MDgtMjkgMTI6MTktMDQ6MDAKTGFzdC1UcmFuc2xhdG9yOiBKLiBEYXZpZCBJYsOhw7FleiA8ai1k
YXZpZEBub29zLmZyPgpMYW5ndWFnZS1UZWFtOiBYWCA8cHl0aG9uLWRldkBweXRob24ub3JnPgpN
SU1FLVZlcnNpb246IDEuMApDb250ZW50LVR5cGU6IHRleHQvcGxhaW47IGNoYXJzZXQ9aXNvLTg4
NTktMQpDb250ZW50LVRyYW5zZmVyLUVuY29kaW5nOiBub25lCkdlbmVyYXRlZC1CeTogcHlnZXR0
ZXh0LnB5IDEuMQpQbHVyYWwtRm9ybXM6IG5wbHVyYWxzPTI7IHBsdXJhbD1uIT0xOwoAVGhyb2F0
d29iYmxlciBNYW5ncm92ZQBIYXkgJXMgZmljaGVybwBIYXkgJXMgZmljaGVyb3MAR3V2ZiB6YnFo
eXIgY2ViaXZxcmYgdmFncmVhbmd2YmFueXZtbmd2YmEgbmFxIHlicG55dm1uZ3ZiYQpmaGNjYmVn
IHNiZSBsYmhlIENsZ3ViYSBjZWJ0ZW56ZiBvbCBjZWJpdnF2YXQgbmEgdmFncmVzbnByIGdiIGd1
ciBUQUgKdHJnZ3JrZyB6cmZmbnRyIHBuZ255YnQgeXZvZW5lbC4AYmFjb24Ad2luayB3aW5rAA==
'''

# Corrupt .mo file
# Generated from
#
# msgid "foo"
# msgstr "bar"
#
# with msgfmt --no-hash
#
# The translation offset is changed to 0xFFFFFFFF,
# making it larger than the file size, which should
# raise an error when parsing.
GNU_MO_DATA_CORRUPT = base64.b64encode(bytes([
    0xDE, 0x12, 0x04, 0x95,  # Magic
    0x00, 0x00, 0x00, 0x00,  # Version
    0x01, 0x00, 0x00, 0x00,  # Message count
    0x1C, 0x00, 0x00, 0x00,  # Message offset
    0x24, 0x00, 0x00, 0x00,  # Translation offset
    0x00, 0x00, 0x00, 0x00,  # Hash table size
    0x2C, 0x00, 0x00, 0x00,  # Hash table offset
    0x03, 0x00, 0x00, 0x00,  # 1st message length
    0x2C, 0x00, 0x00, 0x00,  # 1st message offset
    0x03, 0x00, 0x00, 0x00,  # 1st trans length
    0xFF, 0xFF, 0xFF, 0xFF,  # 1st trans offset (Modified to make it invalid)
    0x66, 0x6F, 0x6F, 0x00,  # Message data
    0x62, 0x61, 0x72, 0x00,  # Message data
]))

UMO_DATA = b'''\
3hIElQAAAAADAAAAHAAAADQAAAAAAAAAAAAAAAAAAABMAAAABAAAAE0AAAAQAAAAUgAAAA8BAABj
AAAABAAAAHMBAAAWAAAAeAEAAABhYsOeAG15Y29udGV4dMOeBGFiw54AUHJvamVjdC1JZC1WZXJz
aW9uOiAyLjAKUE8tUmV2aXNpb24tRGF0ZTogMjAwMy0wNC0xMSAxMjo0Mi0wNDAwCkxhc3QtVHJh
bnNsYXRvcjogQmFycnkgQS4gV0Fyc2F3IDxiYXJyeUBweXRob24ub3JnPgpMYW5ndWFnZS1UZWFt
OiBYWCA8cHl0aG9uLWRldkBweXRob24ub3JnPgpNSU1FLVZlcnNpb246IDEuMApDb250ZW50LVR5
cGU6IHRleHQvcGxhaW47IGNoYXJzZXQ9dXRmLTgKQ29udGVudC1UcmFuc2Zlci1FbmNvZGluZzog
N2JpdApHZW5lcmF0ZWQtQnk6IG1hbnVhbGx5CgDCpHl6AMKkeXogKGNvbnRleHQgdmVyc2lvbikA
'''

MMO_DATA = b'''\
3hIElQAAAAABAAAAHAAAACQAAAADAAAALAAAAAAAAAA4AAAAeAEAADkAAAABAAAAAAAAAAAAAAAA
UHJvamVjdC1JZC1WZXJzaW9uOiBObyBQcm9qZWN0IDAuMApQT1QtQ3JlYXRpb24tRGF0ZTogV2Vk
IERlYyAxMSAwNzo0NDoxNSAyMDAyClBPLVJldmlzaW9uLURhdGU6IDIwMDItMDgtMTQgMDE6MTg6
NTgrMDA6MDAKTGFzdC1UcmFuc2xhdG9yOiBKb2huIERvZSA8amRvZUBleGFtcGxlLmNvbT4KSmFu
ZSBGb29iYXIgPGpmb29iYXJAZXhhbXBsZS5jb20+Ckxhbmd1YWdlLVRlYW06IHh4IDx4eEBleGFt
cGxlLmNvbT4KTUlNRS1WZXJzaW9uOiAxLjAKQ29udGVudC1UeXBlOiB0ZXh0L3BsYWluOyBjaGFy
c2V0PWlzby04ODU5LTE1CkNvbnRlbnQtVHJhbnNmZXItRW5jb2Rpbmc6IHF1b3RlZC1wcmludGFi
bGUKR2VuZXJhdGVkLUJ5OiBweWdldHRleHQucHkgMS4zCgA=
'''

LOCALEDIR = os.path.join('xx', 'LC_MESSAGES')
MOFILE = os.path.join(LOCALEDIR, 'gettext.mo')
MOFILE_BAD_MAGIC_NUMBER = os.path.join(LOCALEDIR, 'gettext_bad_magic_number.mo')
MOFILE_BAD_MAJOR_VERSION = os.path.join(LOCALEDIR, 'gettext_bad_major_version.mo')
MOFILE_BAD_MINOR_VERSION = os.path.join(LOCALEDIR, 'gettext_bad_minor_version.mo')
MOFILE_CORRUPT = os.path.join(LOCALEDIR, 'gettext_corrupt.mo')
UMOFILE = os.path.join(LOCALEDIR, 'ugettext.mo')
MMOFILE = os.path.join(LOCALEDIR, 'metadata.mo')


def reset_gettext():
    gettext._localedirs.clear()
    gettext._current_domain = 'messages'
    gettext._translations.clear()


class GettextBaseTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.addClassCleanup(os_helper.rmtree, os.path.split(LOCALEDIR)[0])
        if not os.path.isdir(LOCALEDIR):
            os.makedirs(LOCALEDIR)
        with open(MOFILE, 'wb') as fp:
            fp.write(base64.decodebytes(GNU_MO_DATA))
        with open(MOFILE_BAD_MAGIC_NUMBER, 'wb') as fp:
            fp.write(base64.decodebytes(GNU_MO_DATA_BAD_MAGIC_NUMBER))
        with open(MOFILE_BAD_MAJOR_VERSION, 'wb') as fp:
            fp.write(base64.decodebytes(GNU_MO_DATA_BAD_MAJOR_VERSION))
        with open(MOFILE_BAD_MINOR_VERSION, 'wb') as fp:
            fp.write(base64.decodebytes(GNU_MO_DATA_BAD_MINOR_VERSION))
        with open(MOFILE_CORRUPT, 'wb') as fp:
            fp.write(base64.decodebytes(GNU_MO_DATA_CORRUPT))
        with open(UMOFILE, 'wb') as fp:
            fp.write(base64.decodebytes(UMO_DATA))
        with open(MMOFILE, 'wb') as fp:
            fp.write(base64.decodebytes(MMO_DATA))

    def setUp(self):
        self.env = self.enterContext(os_helper.EnvironmentVarGuard())
        self.env['LANGUAGE'] = 'xx'
        reset_gettext()
        self.addCleanup(reset_gettext)


GNU_MO_DATA_ISSUE_17898 = b'''\
3hIElQAAAAABAAAAHAAAACQAAAAAAAAAAAAAAAAAAAAsAAAAggAAAC0AAAAAUGx1cmFsLUZvcm1z
OiBucGx1cmFscz0yOyBwbHVyYWw9KG4gIT0gMSk7CiMtIy0jLSMtIyAgbWVzc2FnZXMucG8gKEVk
WCBTdHVkaW8pICAjLSMtIy0jLSMKQ29udGVudC1UeXBlOiB0ZXh0L3BsYWluOyBjaGFyc2V0PVVU
Ri04CgA=
'''

class GettextTestCase1(GettextBaseTest):
    def setUp(self):
        GettextBaseTest.setUp(self)
        self.localedir = os.curdir
        self.mofile = MOFILE
        gettext.install('gettext', self.localedir, names=['pgettext'])

    def test_some_translations(self):
        eq = self.assertEqual
        # test some translations
        eq(_('albatross'), 'albatross')
        eq(_('mullusk'), 'bacon')
        eq(_(r'Raymond Luxury Yach-t'), 'Throatwobbler Mangrove')
        eq(_(r'nudge nudge'), 'wink wink')

    def test_some_translations_with_context(self):
        eq = self.assertEqual
        eq(pgettext('my context', 'nudge nudge'),
           'wink wink (in "my context")')
        eq(pgettext('my other context', 'nudge nudge'),
           'wink wink (in "my other context")')

    def test_multiline_strings(self):
        eq = self.assertEqual
        # multiline strings
        eq(_('''This module provides internationalization and localization
support for your Python programs by providing an interface to the GNU
gettext message catalog library.'''),
           '''Guvf zbqhyr cebivqrf vagreangvbanyvmngvba naq ybpnyvmngvba
fhccbeg sbe lbhe Clguba cebtenzf ol cebivqvat na vagresnpr gb gur TAH
trggrkg zrffntr pngnybt yvoenel.''')

    def test_the_alternative_interface(self):
        eq = self.assertEqual
        neq = self.assertNotEqual
        # test the alternative interface
        with open(self.mofile, 'rb') as fp:
            t = gettext.GNUTranslations(fp)
        # Install the translation object
        t.install()
        eq(_('nudge nudge'), 'wink wink')
        # Try unicode return type
        t.install()
        eq(_('mullusk'), 'bacon')
        # Test installation of other methods
        import builtins
        t.install(names=["gettext", "ngettext"])
        eq(_, t.gettext)
        eq(builtins.gettext, t.gettext)
        eq(ngettext, t.ngettext)
        neq(pgettext, t.pgettext)
        del builtins.gettext
        del builtins.ngettext


class GettextTestCase2(GettextBaseTest):
    def setUp(self):
        GettextBaseTest.setUp(self)
        self.localedir = os.curdir
        # Set up the bindings
        gettext.bindtextdomain('gettext', self.localedir)
        gettext.textdomain('gettext')
        # For convenience
        self._ = gettext.gettext

    def test_bindtextdomain(self):
        self.assertEqual(gettext.bindtextdomain('gettext'), self.localedir)

    def test_textdomain(self):
        self.assertEqual(gettext.textdomain(), 'gettext')

    def test_bad_magic_number(self):
        with open(MOFILE_BAD_MAGIC_NUMBER, 'rb') as fp:
            with self.assertRaises(OSError) as cm:
                gettext.GNUTranslations(fp)

            exception = cm.exception
            self.assertEqual(exception.errno, 0)
            self.assertEqual(exception.strerror, "Bad magic number")
            self.assertEqual(exception.filename, MOFILE_BAD_MAGIC_NUMBER)

    def test_bad_major_version(self):
        with open(MOFILE_BAD_MAJOR_VERSION, 'rb') as fp:
            with self.assertRaises(OSError) as cm:
                gettext.GNUTranslations(fp)

            exception = cm.exception
            self.assertEqual(exception.errno, 0)
            self.assertEqual(exception.strerror, "Bad version number 5")
            self.assertEqual(exception.filename, MOFILE_BAD_MAJOR_VERSION)

    def test_bad_minor_version(self):
        with open(MOFILE_BAD_MINOR_VERSION, 'rb') as fp:
            # Check that no error is thrown with a bad minor version number
            gettext.GNUTranslations(fp)

    def test_corrupt_file(self):
        with open(MOFILE_CORRUPT, 'rb') as fp:
            with self.assertRaises(OSError) as cm:
                gettext.GNUTranslations(fp)

            exception = cm.exception
            self.assertEqual(exception.errno, 0)
            self.assertEqual(exception.strerror, "File is corrupt")
            self.assertEqual(exception.filename, MOFILE_CORRUPT)

    def test_some_translations(self):
        eq = self.assertEqual
        # test some translations
        eq(self._('albatross'), 'albatross')
        eq(self._('mullusk'), 'bacon')
        eq(self._(r'Raymond Luxury Yach-t'), 'Throatwobbler Mangrove')
        eq(self._(r'nudge nudge'), 'wink wink')

    def test_some_translations_with_context(self):
        eq = self.assertEqual
        eq(gettext.pgettext('my context', 'nudge nudge'),
           'wink wink (in "my context")')
        eq(gettext.pgettext('my other context', 'nudge nudge'),
           'wink wink (in "my other context")')

    def test_some_translations_with_context_and_domain(self):
        eq = self.assertEqual
        eq(gettext.dpgettext('gettext', 'my context', 'nudge nudge'),
           'wink wink (in "my context")')
        eq(gettext.dpgettext('gettext', 'my other context', 'nudge nudge'),
           'wink wink (in "my other context")')

    def test_multiline_strings(self):
        eq = self.assertEqual
        # multiline strings
        eq(self._('''This module provides internationalization and localization
support for your Python programs by providing an interface to the GNU
gettext message catalog library.'''),
           '''Guvf zbqhyr cebivqrf vagreangvbanyvmngvba naq ybpnyvmngvba
fhccbeg sbe lbhe Clguba cebtenzf ol cebivqvat na vagresnpr gb gur TAH
trggrkg zrffntr pngnybt yvoenel.''')


class PluralFormsTests:

    def _test_plural_forms(self, ngettext, gettext,
                           singular, plural, tsingular, tplural,
                           numbers_only=True):
        x = ngettext(singular, plural, 1)
        self.assertEqual(x, tsingular)
        x = ngettext(singular, plural, 2)
        self.assertEqual(x, tplural)
        x = gettext(singular)
        self.assertEqual(x, tsingular)

        lineno = self._test_plural_forms.__code__.co_firstlineno + 12
        with self.assertWarns(DeprecationWarning) as cm:
            x = ngettext(singular, plural, 1.0)
        self.assertEqual(cm.filename, __file__)
        self.assertEqual(cm.lineno, lineno)
        self.assertEqual(x, tsingular)
        with self.assertWarns(DeprecationWarning) as cm:
            x = ngettext(singular, plural, 1.1)
        self.assertEqual(cm.filename, __file__)
        self.assertEqual(cm.lineno, lineno + 5)
        self.assertEqual(x, tplural)

        if numbers_only:
            with self.assertRaises(TypeError):
                ngettext(singular, plural, None)
        else:
            with self.assertWarns(DeprecationWarning) as cm:
                x = ngettext(singular, plural, None)
            self.assertEqual(x, tplural)

    def test_plural_forms(self):
        self._test_plural_forms(
            self.ngettext, self.gettext,
            'There is %s file', 'There are %s files',
            'Hay %s fichero', 'Hay %s ficheros')
        self._test_plural_forms(
            self.ngettext, self.gettext,
            '%d file deleted', '%d files deleted',
            '%d file deleted', '%d files deleted')

    def test_plural_context_forms(self):
        ngettext = partial(self.npgettext, 'With context')
        gettext = partial(self.pgettext, 'With context')
        self._test_plural_forms(
            ngettext, gettext,
            'There is %s file', 'There are %s files',
            'Hay %s fichero (context)', 'Hay %s ficheros (context)')
        self._test_plural_forms(
            ngettext, gettext,
            '%d file deleted', '%d files deleted',
            '%d file deleted', '%d files deleted')

    def test_plural_wrong_context_forms(self):
        self._test_plural_forms(
            partial(self.npgettext, 'Unknown context'),
            partial(self.pgettext, 'Unknown context'),
            'There is %s file', 'There are %s files',
            'There is %s file', 'There are %s files')


class GNUTranslationsPluralFormsTestCase(PluralFormsTests, GettextBaseTest):
    def setUp(self):
        GettextBaseTest.setUp(self)
        # Set up the bindings
        gettext.bindtextdomain('gettext', os.curdir)
        gettext.textdomain('gettext')

        self.gettext = gettext.gettext
        self.ngettext = gettext.ngettext
        self.pgettext = gettext.pgettext
        self.npgettext = gettext.npgettext


class GNUTranslationsWithDomainPluralFormsTestCase(PluralFormsTests, GettextBaseTest):
    def setUp(self):
        GettextBaseTest.setUp(self)
        # Set up the bindings
        gettext.bindtextdomain('gettext', os.curdir)

        self.gettext = partial(gettext.dgettext, 'gettext')
        self.ngettext = partial(gettext.dngettext, 'gettext')
        self.pgettext = partial(gettext.dpgettext, 'gettext')
        self.npgettext = partial(gettext.dnpgettext, 'gettext')

    def test_plural_forms_wrong_domain(self):
        self._test_plural_forms(
            partial(gettext.dngettext, 'unknown'),
            partial(gettext.dgettext, 'unknown'),
            'There is %s file', 'There are %s files',
            'There is %s file', 'There are %s files',
            numbers_only=False)

    def test_plural_context_forms_wrong_domain(self):
        self._test_plural_forms(
            partial(gettext.dnpgettext, 'unknown', 'With context'),
            partial(gettext.dpgettext, 'unknown', 'With context'),
            'There is %s file', 'There are %s files',
            'There is %s file', 'There are %s files',
            numbers_only=False)


class GNUTranslationsClassPluralFormsTestCase(PluralFormsTests, GettextBaseTest):
    def setUp(self):
        GettextBaseTest.setUp(self)
        with open(MOFILE, 'rb') as fp:
            t = gettext.GNUTranslations(fp)

        self.gettext = t.gettext
        self.ngettext = t.ngettext
        self.pgettext = t.pgettext
        self.npgettext = t.npgettext

    def test_plural_forms_null_translations(self):
        t = gettext.NullTranslations()
        self._test_plural_forms(
            t.ngettext, t.gettext,
            'There is %s file', 'There are %s files',
            'There is %s file', 'There are %s files',
            numbers_only=False)

    def test_plural_context_forms_null_translations(self):
        t = gettext.NullTranslations()
        self._test_plural_forms(
            partial(t.npgettext, 'With context'),
            partial(t.pgettext, 'With context'),
            'There is %s file', 'There are %s files',
            'There is %s file', 'There are %s files',
            numbers_only=False)


class PluralFormsInternalTestCase(unittest.TestCase):
    # Examples from http://www.gnu.org/software/gettext/manual/gettext.html

    def test_ja(self):
        eq = self.assertEqual
        f = gettext.c2py('0')
        s = ''.join([ str(f(x)) for x in range(200) ])
        eq(s, "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000")

    def test_de(self):
        eq = self.assertEqual
        f = gettext.c2py('n != 1')
        s = ''.join([ str(f(x)) for x in range(200) ])
        eq(s, "10111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111")

    def test_fr(self):
        eq = self.assertEqual
        f = gettext.c2py('n>1')
        s = ''.join([ str(f(x)) for x in range(200) ])
        eq(s, "00111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111")

    def test_lv(self):
        eq = self.assertEqual
        f = gettext.c2py('n%10==1 && n%100!=11 ? 0 : n != 0 ? 1 : 2')
        s = ''.join([ str(f(x)) for x in range(200) ])
        eq(s, "20111111111111111111101111111110111111111011111111101111111110111111111011111111101111111110111111111011111111111111111110111111111011111111101111111110111111111011111111101111111110111111111011111111")

    def test_gd(self):
        eq = self.assertEqual
        f = gettext.c2py('n==1 ? 0 : n==2 ? 1 : 2')
        s = ''.join([ str(f(x)) for x in range(200) ])
        eq(s, "20122222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222")

    def test_gd2(self):
        eq = self.assertEqual
        # Tests the combination of parentheses and "?:"
        f = gettext.c2py('n==1 ? 0 : (n==2 ? 1 : 2)')
        s = ''.join([ str(f(x)) for x in range(200) ])
        eq(s, "20122222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222")

    def test_ro(self):
        eq = self.assertEqual
        f = gettext.c2py('n==1 ? 0 : (n==0 || (n%100 > 0 && n%100 < 20)) ? 1 : 2')
        s = ''.join([ str(f(x)) for x in range(200) ])
        eq(s, "10111111111111111111222222222222222222222222222222222222222222222222222222222222222222222222222222222111111111111111111122222222222222222222222222222222222222222222222222222222222222222222222222222222")

    def test_lt(self):
        eq = self.assertEqual
        f = gettext.c2py('n%10==1 && n%100!=11 ? 0 : n%10>=2 && (n%100<10 || n%100>=20) ? 1 : 2')
        s = ''.join([ str(f(x)) for x in range(200) ])
        eq(s, "20111111112222222222201111111120111111112011111111201111111120111111112011111111201111111120111111112011111111222222222220111111112011111111201111111120111111112011111111201111111120111111112011111111")

    def test_ru(self):
        eq = self.assertEqual
        f = gettext.c2py('n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2')
        s = ''.join([ str(f(x)) for x in range(200) ])
        eq(s, "20111222222222222222201112222220111222222011122222201112222220111222222011122222201112222220111222222011122222222222222220111222222011122222201112222220111222222011122222201112222220111222222011122222")

    def test_cs(self):
        eq = self.assertEqual
        f = gettext.c2py('(n==1) ? 0 : (n>=2 && n<=4) ? 1 : 2')
        s = ''.join([ str(f(x)) for x in range(200) ])
        eq(s, "20111222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222")

    def test_pl(self):
        eq = self.assertEqual
        f = gettext.c2py('n==1 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2')
        s = ''.join([ str(f(x)) for x in range(200) ])
        eq(s, "20111222222222222222221112222222111222222211122222221112222222111222222211122222221112222222111222222211122222222222222222111222222211122222221112222222111222222211122222221112222222111222222211122222")

    def test_sl(self):
        eq = self.assertEqual
        f = gettext.c2py('n%100==1 ? 0 : n%100==2 ? 1 : n%100==3 || n%100==4 ? 2 : 3')
        s = ''.join([ str(f(x)) for x in range(200) ])
        eq(s, "30122333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333012233333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333")

    def test_ar(self):
        eq = self.assertEqual
        f = gettext.c2py('n==0 ? 0 : n==1 ? 1 : n==2 ? 2 : n%100>=3 && n%100<=10 ? 3 : n%100>=11 ? 4 : 5')
        s = ''.join([ str(f(x)) for x in range(200) ])
        eq(s, "01233333333444444444444444444444444444444444444444444444444444444444444444444444444444444444444444445553333333344444444444444444444444444444444444444444444444444444444444444444444444444444444444444444")

    @support.skip_wasi_stack_overflow()
    def test_security(self):
        raises = self.assertRaises
        # Test for a dangerous expression
        raises(ValueError, gettext.c2py, "os.chmod('/etc/passwd',0777)")
        # issue28563
        raises(ValueError, gettext.c2py, '"(eval(foo) && ""')
        raises(ValueError, gettext.c2py, 'f"{os.system(\'sh\')}"')
        # Maximum recursion depth exceeded during compilation
        raises(ValueError, gettext.c2py, 'n+'*10000 + 'n')
        self.assertEqual(gettext.c2py('n+'*100 + 'n')(1), 101)
        # MemoryError during compilation
        raises(ValueError, gettext.c2py, '('*100 + 'n' + ')'*100)
        # Maximum recursion depth exceeded in C to Python translator
        raises(ValueError, gettext.c2py, '('*10000 + 'n' + ')'*10000)
        self.assertEqual(gettext.c2py('('*20 + 'n' + ')'*20)(1), 1)

    def test_chained_comparison(self):
        # C doesn't chain comparison as Python so 2 == 2 == 2 gets different results
        f = gettext.c2py('n == n == n')
        self.assertEqual(''.join(str(f(x)) for x in range(3)), '010')
        f = gettext.c2py('1 < n == n')
        self.assertEqual(''.join(str(f(x)) for x in range(3)), '100')
        f = gettext.c2py('n == n < 2')
        self.assertEqual(''.join(str(f(x)) for x in range(3)), '010')
        f = gettext.c2py('0 < n < 2')
        self.assertEqual(''.join(str(f(x)) for x in range(3)), '111')

    def test_decimal_number(self):
        self.assertEqual(gettext.c2py('0123')(1), 123)

    def test_invalid_syntax(self):
        invalid_expressions = [
            'x>1', '(n>1', 'n>1)', '42**42**42', '0xa', '1.0', '1e2',
            'n>0x1', '+n', '-n', 'n()', 'n(1)', '1+', 'nn', 'n n', 'n ? 1 2'
        ]
        for expr in invalid_expressions:
            with self.assertRaises(ValueError):
                gettext.c2py(expr)

    def test_negation(self):
        f = gettext.c2py('!!!n')
        self.assertEqual(f(0), 1)
        self.assertEqual(f(1), 0)
        self.assertEqual(f(2), 0)

    def test_nested_condition_operator(self):
        self.assertEqual(gettext.c2py('n?1?2:3:4')(0), 4)
        self.assertEqual(gettext.c2py('n?1?2:3:4')(1), 2)
        self.assertEqual(gettext.c2py('n?1:3?4:5')(0), 4)
        self.assertEqual(gettext.c2py('n?1:3?4:5')(1), 1)

    def test_division(self):
        f = gettext.c2py('2/n*3')
        self.assertEqual(f(1), 6)
        self.assertEqual(f(2), 3)
        self.assertEqual(f(3), 0)
        self.assertEqual(f(-1), -6)
        self.assertRaises(ZeroDivisionError, f, 0)

    def test_plural_number(self):
        f = gettext.c2py('n != 1')
        self.assertEqual(f(1), 0)
        self.assertEqual(f(2), 1)
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(f(1.0), 0)
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(f(2.0), 1)
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(f(1.1), 1)
        self.assertRaises(TypeError, f, '2')
        self.assertRaises(TypeError, f, b'2')
        self.assertRaises(TypeError, f, [])
        self.assertRaises(TypeError, f, object())


class GNUTranslationParsingTest(GettextBaseTest):
    def test_plural_form_error_issue17898(self):
        with open(MOFILE, 'wb') as fp:
            fp.write(base64.decodebytes(GNU_MO_DATA_ISSUE_17898))
        with open(MOFILE, 'rb') as fp:
            # If this runs cleanly, the bug is fixed.
            t = gettext.GNUTranslations(fp)

    def test_ignore_comments_in_headers_issue36239(self):
        """Checks that comments like:

            #-#-#-#-#  messages.po (EdX Studio)  #-#-#-#-#

        are ignored.
        """
        with open(MOFILE, 'wb') as fp:
            fp.write(base64.decodebytes(GNU_MO_DATA_ISSUE_17898))
        with open(MOFILE, 'rb') as fp:
            t = gettext.GNUTranslations(fp)
            self.assertEqual(t.info()["plural-forms"], "nplurals=2; plural=(n != 1);")


class UnicodeTranslationsTest(GettextBaseTest):
    def setUp(self):
        GettextBaseTest.setUp(self)
        with open(UMOFILE, 'rb') as fp:
            self.t = gettext.GNUTranslations(fp)
        self._ = self.t.gettext
        self.pgettext = self.t.pgettext

    def test_unicode_msgid(self):
        self.assertIsInstance(self._(''), str)

    def test_unicode_msgstr(self):
        self.assertEqual(self._('ab\xde'), '\xa4yz')

    def test_unicode_context_msgstr(self):
        t = self.pgettext('mycontext\xde', 'ab\xde')
        self.assertTrue(isinstance(t, str))
        self.assertEqual(t, '\xa4yz (context version)')


class UnicodeTranslationsPluralTest(GettextBaseTest):
    def setUp(self):
        GettextBaseTest.setUp(self)
        with open(MOFILE, 'rb') as fp:
            self.t = gettext.GNUTranslations(fp)
        self.ngettext = self.t.ngettext
        self.npgettext = self.t.npgettext

    def test_unicode_msgid(self):
        unless = self.assertTrue
        unless(isinstance(self.ngettext('', '', 1), str))
        unless(isinstance(self.ngettext('', '', 2), str))

    def test_unicode_context_msgid(self):
        unless = self.assertTrue
        unless(isinstance(self.npgettext('', '', '', 1), str))
        unless(isinstance(self.npgettext('', '', '', 2), str))

    def test_unicode_msgstr(self):
        eq = self.assertEqual
        unless = self.assertTrue
        t = self.ngettext("There is %s file", "There are %s files", 1)
        unless(isinstance(t, str))
        eq(t, "Hay %s fichero")
        unless(isinstance(t, str))
        t = self.ngettext("There is %s file", "There are %s files", 5)
        unless(isinstance(t, str))
        eq(t, "Hay %s ficheros")

    def test_unicode_msgstr_with_context(self):
        eq = self.assertEqual
        unless = self.assertTrue
        t = self.npgettext("With context",
                           "There is %s file", "There are %s files", 1)
        unless(isinstance(t, str))
        eq(t, "Hay %s fichero (context)")
        t = self.npgettext("With context",
                           "There is %s file", "There are %s files", 5)
        unless(isinstance(t, str))
        eq(t, "Hay %s ficheros (context)")


class WeirdMetadataTest(GettextBaseTest):
    def setUp(self):
        GettextBaseTest.setUp(self)
        with open(MMOFILE, 'rb') as fp:
            try:
                self.t = gettext.GNUTranslations(fp)
            except:
                self.tearDown()
                raise

    def test_weird_metadata(self):
        info = self.t.info()
        self.assertEqual(len(info), 9)
        self.assertEqual(info['last-translator'],
           'John Doe <jdoe@example.com>\nJane Foobar <jfoobar@example.com>')


class DummyGNUTranslations(gettext.GNUTranslations):
    def foo(self):
        return 'foo'


class GettextCacheTestCase(GettextBaseTest):
    def test_cache(self):
        self.localedir = os.curdir
        self.mofile = MOFILE

        self.assertEqual(len(gettext._translations), 0)

        t = gettext.translation('gettext', self.localedir)

        self.assertEqual(len(gettext._translations), 1)

        t = gettext.translation('gettext', self.localedir,
                                class_=DummyGNUTranslations)

        self.assertEqual(len(gettext._translations), 2)
        self.assertEqual(t.__class__, DummyGNUTranslations)

        # Calling it again doesn't add to the cache

        t = gettext.translation('gettext', self.localedir,
                                class_=DummyGNUTranslations)

        self.assertEqual(len(gettext._translations), 2)
        self.assertEqual(t.__class__, DummyGNUTranslations)


class ExpandLangTestCase(unittest.TestCase):
    def test_expand_lang(self):
        # Test all combinations of territory, charset and
        # modifier (locale extension)
        locales = {
            'cs': ['cs'],
            'cs_CZ': ['cs_CZ', 'cs'],
            'cs.ISO8859-2': ['cs.ISO8859-2', 'cs'],
            'cs@euro': ['cs@euro', 'cs'],
            'cs_CZ.ISO8859-2': ['cs_CZ.ISO8859-2', 'cs_CZ', 'cs.ISO8859-2',
                                'cs'],
            'cs_CZ@euro': ['cs_CZ@euro', 'cs@euro', 'cs_CZ', 'cs'],
            'cs.ISO8859-2@euro': ['cs.ISO8859-2@euro', 'cs@euro',
                                  'cs.ISO8859-2', 'cs'],
            'cs_CZ.ISO8859-2@euro': ['cs_CZ.ISO8859-2@euro', 'cs_CZ@euro',
                                     'cs.ISO8859-2@euro', 'cs@euro',
                                     'cs_CZ.ISO8859-2', 'cs_CZ',
                                     'cs.ISO8859-2', 'cs'],
        }
        for locale, expanded in locales.items():
            with self.subTest(locale=locale):
                with unittest.mock.patch("locale.normalize",
                                         return_value=locale):
                    self.assertEqual(gettext._expand_lang(locale), expanded)


class FindTestCase(unittest.TestCase):

    def setUp(self):
        self.env = self.enterContext(os_helper.EnvironmentVarGuard())
        self.tempdir = self.enterContext(os_helper.temp_cwd())

        for key in ('LANGUAGE', 'LC_ALL', 'LC_MESSAGES', 'LANG'):
            self.env.unset(key)

    def create_mo_file(self, lang):
        locale_dir = os.path.join(self.tempdir, "locale")
        mofile_dir = os.path.join(locale_dir, lang, "LC_MESSAGES")
        os.makedirs(mofile_dir)
        mo_file = os.path.join(mofile_dir, "mofile.mo")
        with open(mo_file, "wb") as f:
            f.write(GNU_MO_DATA)
        return mo_file

    def test_find_with_env_vars(self):
        # test that find correctly finds the environment variables
        # when languages are not supplied
        mo_file = self.create_mo_file("ga_IE")
        for var in ('LANGUAGE', 'LC_ALL', 'LC_MESSAGES', 'LANG'):
            self.env.set(var, 'ga_IE')
            result = gettext.find("mofile",
                                  localedir=os.path.join(self.tempdir, "locale"))
            self.assertEqual(result, mo_file)
            self.env.unset(var)

    def test_find_with_languages(self):
        # test that passed languages are used
        self.env.set('LANGUAGE', 'pt_BR')
        mo_file = self.create_mo_file("ga_IE")

        result = gettext.find("mofile",
                              localedir=os.path.join(self.tempdir, "locale"),
                              languages=['ga_IE'])
        self.assertEqual(result, mo_file)

    @unittest.mock.patch('gettext._expand_lang')
    def test_find_with_no_lang(self, patch_expand_lang):
        # no language can be found
        gettext.find('foo')
        patch_expand_lang.assert_called_with('C')

    @unittest.mock.patch('gettext._expand_lang')
    def test_find_with_c(self, patch_expand_lang):
        # 'C' is already in languages
        self.env.set('LANGUAGE', 'C')
        gettext.find('foo')
        patch_expand_lang.assert_called_with('C')

    def test_find_all(self):
        # test that all are returned when all is set
        paths = []
        for lang in ["ga_IE", "es_ES"]:
            paths.append(self.create_mo_file(lang))
        result = gettext.find('mofile',
                              localedir=os.path.join(self.tempdir, "locale"),
                              languages=["ga_IE", "es_ES"], all=True)
        self.assertEqual(sorted(result), sorted(paths))

    def test_find_deduplication(self):
        # test that find removes duplicate languages
        mo_file = [self.create_mo_file('ga_IE')]
        result = gettext.find("mofile", localedir=os.path.join(self.tempdir, "locale"),
                              languages=['ga_IE', 'ga_IE'], all=True)
        self.assertEqual(result, mo_file)


class MiscTestCase(unittest.TestCase):
    def test__all__(self):
        support.check__all__(self, gettext,
                             not_exported={'c2py', 'ENOENT'})


if __name__ == '__main__':
    unittest.main()


# For reference, here's the .po file used to created the GNU_MO_DATA above.
#
# The original version was automatically generated from the sources with
# pygettext. Later it was manually modified to add plural forms support.

b'''
# Dummy translation for the Python test_gettext.py module.
# Copyright (C) 2001 Python Software Foundation
# Barry Warsaw <barry@python.org>, 2000.
#
msgid ""
msgstr ""
"Project-Id-Version: 2.0\n"
"PO-Revision-Date: 2003-04-11 14:32-0400\n"
"Last-Translator: J. David Ibanez <j-david@noos.fr>\n"
"Language-Team: XX <python-dev@python.org>\n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=iso-8859-1\n"
"Content-Transfer-Encoding: 8bit\n"
"Generated-By: pygettext.py 1.1\n"
"Plural-Forms: nplurals=2; plural=n!=1;\n"

#: test_gettext.py:19 test_gettext.py:25 test_gettext.py:31 test_gettext.py:37
#: test_gettext.py:51 test_gettext.py:80 test_gettext.py:86 test_gettext.py:92
#: test_gettext.py:98
msgid "nudge nudge"
msgstr "wink wink"

msgctxt "my context"
msgid "nudge nudge"
msgstr "wink wink (in \"my context\")"

msgctxt "my other context"
msgid "nudge nudge"
msgstr "wink wink (in \"my other context\")"

#: test_gettext.py:16 test_gettext.py:22 test_gettext.py:28 test_gettext.py:34
#: test_gettext.py:77 test_gettext.py:83 test_gettext.py:89 test_gettext.py:95
msgid "albatross"
msgstr ""

#: test_gettext.py:18 test_gettext.py:24 test_gettext.py:30 test_gettext.py:36
#: test_gettext.py:79 test_gettext.py:85 test_gettext.py:91 test_gettext.py:97
msgid "Raymond Luxury Yach-t"
msgstr "Throatwobbler Mangrove"

#: test_gettext.py:17 test_gettext.py:23 test_gettext.py:29 test_gettext.py:35
#: test_gettext.py:56 test_gettext.py:78 test_gettext.py:84 test_gettext.py:90
#: test_gettext.py:96
msgid "mullusk"
msgstr "bacon"

#: test_gettext.py:40 test_gettext.py:101
msgid ""
"This module provides internationalization and localization\n"
"support for your Python programs by providing an interface to the GNU\n"
"gettext message catalog library."
msgstr ""
"Guvf zbqhyr cebivqrf vagreangvbanyvmngvba naq ybpnyvmngvba\n"
"fhccbeg sbe lbhe Clguba cebtenzf ol cebivqvat na vagresnpr gb gur TAH\n"
"trggrkg zrffntr pngnybt yvoenel."

# Manually added, as neither pygettext nor xgettext support plural forms
# in Python.
msgid "There is %s file"
msgid_plural "There are %s files"
msgstr[0] "Hay %s fichero"
msgstr[1] "Hay %s ficheros"

# Manually added, as neither pygettext nor xgettext support plural forms
# and context in Python.
msgctxt "With context"
msgid "There is %s file"
msgid_plural "There are %s files"
msgstr[0] "Hay %s fichero (context)"
msgstr[1] "Hay %s ficheros (context)"
'''

# Here's the second example po file example, used to generate the UMO_DATA
# containing utf-8 encoded Unicode strings

b'''
# Dummy translation for the Python test_gettext.py module.
# Copyright (C) 2001 Python Software Foundation
# Barry Warsaw <barry@python.org>, 2000.
#
msgid ""
msgstr ""
"Project-Id-Version: 2.0\n"
"PO-Revision-Date: 2003-04-11 12:42-0400\n"
"Last-Translator: Barry A. WArsaw <barry@python.org>\n"
"Language-Team: XX <python-dev@python.org>\n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=utf-8\n"
"Content-Transfer-Encoding: 7bit\n"
"Generated-By: manually\n"

#: nofile:0
msgid "ab\xc3\x9e"
msgstr "\xc2\xa4yz"

#: nofile:1
msgctxt "mycontext\xc3\x9e"
msgid "ab\xc3\x9e"
msgstr "\xc2\xa4yz (context version)"
'''

# Here's the third example po file, used to generate MMO_DATA

b'''
msgid ""
msgstr ""
"Project-Id-Version: No Project 0.0\n"
"POT-Creation-Date: Wed Dec 11 07:44:15 2002\n"
"PO-Revision-Date: 2002-08-14 01:18:58+00:00\n"
"Last-Translator: John Doe <jdoe@example.com>\n"
"Jane Foobar <jfoobar@example.com>\n"
"Language-Team: xx <xx@example.com>\n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=iso-8859-15\n"
"Content-Transfer-Encoding: quoted-printable\n"
"Generated-By: pygettext.py 1.3\n"
'''

#
# messages.po, used for bug 17898
#

b'''
# test file for http://bugs.python.org/issue17898
msgid ""
msgstr ""
"Plural-Forms: nplurals=2; plural=(n != 1);\n"
"#-#-#-#-#  messages.po (EdX Studio)  #-#-#-#-#\n"
"Content-Type: text/plain; charset=UTF-8\n"
'''
