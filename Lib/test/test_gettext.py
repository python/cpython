import os
import base64
import gettext

import unittest
from unittest import TestCase

# TODO:
#  - Add new tests, for example for "dgettext"
#  - Remove dummy tests, for example testing for single and double quotes
#    has no sense, it would have if we were testing a parser (i.e. pygettext)
#  - Tests should have only one assert.


GNU_MO_DATA = '''\
3hIElQAAAAAGAAAAHAAAAEwAAAALAAAAfAAAAAAAAACoAAAAFQAAAKkAAAAjAAAAvwAAAKEAAADj
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


LOCALEDIR = os.path.join('xx', 'LC_MESSAGES')
MOFILE = os.path.join(LOCALEDIR, 'gettext.mo')

def setup():
    os.makedirs(LOCALEDIR)
    fp = open(MOFILE, 'wb')
    fp.write(base64.decodestring(GNU_MO_DATA))
    fp.close()
    os.environ['LANGUAGE'] = 'xx'

def teardown():
    os.environ['LANGUAGE'] = 'en'
    os.unlink(MOFILE)
    os.removedirs(LOCALEDIR)


class GettextTestCase1(TestCase):
    def setUp(self):
        self.localedir = os.curdir
        self.mofile = MOFILE

        gettext.install('gettext', self.localedir)


    def test_some_translations(self):
        # test some translations
        assert _('albatross') == 'albatross'
        assert _(u'mullusk') == 'bacon'
        assert _(r'Raymond Luxury Yach-t') == 'Throatwobbler Mangrove'
        assert _(ur'nudge nudge') == 'wink wink'


    def test_double_quotes(self):
        # double quotes
        assert _("albatross") == 'albatross'
        assert _(u"mullusk") == 'bacon'
        assert _(r"Raymond Luxury Yach-t") == 'Throatwobbler Mangrove'
        assert _(ur"nudge nudge") == 'wink wink'


    def test_triple_single_quotes(self):
        # triple single quotes
        assert _('''albatross''') == 'albatross'
        assert _(u'''mullusk''') == 'bacon'
        assert _(r'''Raymond Luxury Yach-t''') == 'Throatwobbler Mangrove'
        assert _(ur'''nudge nudge''') == 'wink wink'


    def test_triple_double_quotes(self):
        # triple double quotes
        assert _("""albatross""") == 'albatross'
        assert _(u"""mullusk""") == 'bacon'
        assert _(r"""Raymond Luxury Yach-t""") == 'Throatwobbler Mangrove'
        assert _(ur"""nudge nudge""") == 'wink wink'


    def test_multiline_strings(self):
        # multiline strings
        assert _('''This module provides internationalization and localization
support for your Python programs by providing an interface to the GNU
gettext message catalog library.''') == '''Guvf zbqhyr cebivqrf vagreangvbanyvmngvba naq ybpnyvmngvba
fhccbeg sbe lbhe Clguba cebtenzf ol cebivqvat na vagresnpr gb gur TAH
trggrkg zrffntr pngnybt yvoenel.'''


    def test_the_alternative_interface(self):
        # test the alternative interface
        fp = open(os.path.join(self.mofile), 'rb')
        t = gettext.GNUTranslations(fp)
        fp.close()

        t.install()

        assert _('nudge nudge') == 'wink wink'

        # try unicode return type
        t.install(unicode=1)

        assert _('mullusk') == 'bacon'


class GettextTestCase2(TestCase):
    def setUp(self):
        self.localedir = os.curdir

        gettext.bindtextdomain('gettext', self.localedir)
        gettext.textdomain('gettext')

        self._ = gettext.gettext


    def test_bindtextdomain(self):
        assert gettext.bindtextdomain('gettext') == self.localedir


    def test_textdomain(self):
        assert gettext.textdomain() == 'gettext'


    def test_some_translations(self):
        # test some translations
        assert self._('albatross') == 'albatross'
        assert self._(u'mullusk') == 'bacon'
        assert self._(r'Raymond Luxury Yach-t') == 'Throatwobbler Mangrove'
        assert self._(ur'nudge nudge') == 'wink wink'


    def test_double_quotes(self):
        # double quotes
        assert self._("albatross") == 'albatross'
        assert self._(u"mullusk") == 'bacon'
        assert self._(r"Raymond Luxury Yach-t") == 'Throatwobbler Mangrove'
        assert self._(ur"nudge nudge") == 'wink wink'


    def test_triple_single_quotes(self):
        # triple single quotes
        assert self._('''albatross''') == 'albatross'
        assert self._(u'''mullusk''') == 'bacon'
        assert self._(r'''Raymond Luxury Yach-t''') == 'Throatwobbler Mangrove'
        assert self._(ur'''nudge nudge''') == 'wink wink'


    def test_triple_double_quotes(self):
        # triple double quotes
        assert self._("""albatross""") == 'albatross'
        assert self._(u"""mullusk""") == 'bacon'
        assert self._(r"""Raymond Luxury Yach-t""") == 'Throatwobbler Mangrove'
        assert self._(ur"""nudge nudge""") == 'wink wink'


    def test_multiline_strings(self):
        # multiline strings
        assert self._('''This module provides internationalization and localization
support for your Python programs by providing an interface to the GNU
gettext message catalog library.''') == '''Guvf zbqhyr cebivqrf vagreangvbanyvmngvba naq ybpnyvmngvba
fhccbeg sbe lbhe Clguba cebtenzf ol cebivqvat na vagresnpr gb gur TAH
trggrkg zrffntr pngnybt yvoenel.'''




class PluralFormsTestCase(TestCase):
    def setUp(self):
        self.mofile = MOFILE

    def test_plural_forms1(self):
        x = gettext.ngettext('There is %s file', 'There are %s files', 1)
        assert x == 'Hay %s fichero'

        x = gettext.ngettext('There is %s file', 'There are %s files', 2)
        assert x == 'Hay %s ficheros'


    def test_plural_forms2(self):
        fp = open(os.path.join(self.mofile), 'rb')
        t = gettext.GNUTranslations(fp)
        fp.close()

        x = t.ngettext('There is %s file', 'There are %s files', 1)
        assert x == 'Hay %s fichero'

        x = t.ngettext('There is %s file', 'There are %s files', 2)
        assert x == 'Hay %s ficheros'


    def test_hu(self):
        f = gettext.c2py('0')
        s = ''.join([ str(f(x)) for x in range(200) ])
        assert s == "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"


    def test_de(self):
        f = gettext.c2py('n != 1')
        s = ''.join([ str(f(x)) for x in range(200) ])
        assert s == "10111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111"


    def test_fr(self):
        f = gettext.c2py('n>1')
        s = ''.join([ str(f(x)) for x in range(200) ])
        assert s == "00111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111"


    def test_gd(self):
        f = gettext.c2py('n==1 ? 0 : n==2 ? 1 : 2')
        s = ''.join([ str(f(x)) for x in range(200) ])
        assert s == "20122222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222"


    def test_gd2(self):
        # Tests the combination of parentheses and "?:"
        f = gettext.c2py('n==1 ? 0 : (n==2 ? 1 : 2)')
        s = ''.join([ str(f(x)) for x in range(200) ])
        assert s == "20122222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222"


    def test_lt(self):
        f = gettext.c2py('n%10==1 && n%100!=11 ? 0 : n%10>=2 && (n%100<10 || n%100>=20) ? 1 : 2')
        s = ''.join([ str(f(x)) for x in range(200) ])
        assert s == "20111111112222222222201111111120111111112011111111201111111120111111112011111111201111111120111111112011111111222222222220111111112011111111201111111120111111112011111111201111111120111111112011111111"


    def test_ru(self):
        f = gettext.c2py('n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2')
        s = ''.join([ str(f(x)) for x in range(200) ])
        assert s == "20111222222222222222201112222220111222222011122222201112222220111222222011122222201112222220111222222011122222222222222220111222222011122222201112222220111222222011122222201112222220111222222011122222"


    def test_pl(self):
        f = gettext.c2py('n==1 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2')
        s = ''.join([ str(f(x)) for x in range(200) ])
        assert s == "20111222222222222222221112222222111222222211122222221112222222111222222211122222221112222222111222222211122222222222222222111222222211122222221112222222111222222211122222221112222222111222222211122222"


    def test_sl(self):
        f = gettext.c2py('n%100==1 ? 0 : n%100==2 ? 1 : n%100==3 || n%100==4 ? 2 : 3')
        s = ''.join([ str(f(x)) for x in range(200) ])
        assert s == "30122333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333012233333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333"


    def test_security(self):
        # Test for a dangerous expression
        try:
            gettext.c2py("os.chmod('/etc/passwd',0777)")
        except ValueError:
            pass
        else:
            raise AssertionError



if __name__ == '__main__':
    try:
        setup()
        unittest.main()
    finally:
        teardown()




# For reference, here's the .po file used to created the .mo data above.
#
# The original version was automatically generated from the sources with
# pygettext. Later it was manually modified to add plural forms support.

'''
# Dummy translation for Python's test_gettext.py module.
# Copyright (C) 2001 Python Software Foundation
# Barry Warsaw <barry@python.org>, 2000.
#
msgid ""
msgstr ""
"Project-Id-Version: 2.0\n"
"PO-Revision-Date: 2000-08-29 12:19-04:00\n"
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
'''
