import os
import base64
import gettext


def test_api_1(localedir, mofile):
    print 'test api 1'

    # Test basic interface
    os.environ['LANGUAGE'] = 'xx'

    print 'installing gettext'
    gettext.install('gettext', localedir)

    # test some translations
    print _('albatross')
    print _(u'mullusk')
    print _(r'Raymond Luxury Yach-t')
    print _(ur'nudge nudge')

    # double quotes
    print _("albatross")
    print _(u"mullusk")
    print _(r"Raymond Luxury Yach-t")
    print _(ur"nudge nudge")

    # triple single quotes
    print _('''albatross''')
    print _(u'''mullusk''')
    print _(r'''Raymond Luxury Yach-t''')
    print _(ur'''nudge nudge''')

    # triple double quotes
    print _("""albatross""")
    print _(u"""mullusk""")
    print _(r"""Raymond Luxury Yach-t""")
    print _(ur"""nudge nudge""")

    # multiline strings
    print _('''This module provides internationalization and localization
support for your Python programs by providing an interface to the GNU
gettext message catalog library.''')

    # test the alternative interface
    fp = open(os.path.join(mofile), 'rb')
    t = gettext.GNUTranslations(fp)
    fp.close()

    t.install()

    print _('nudge nudge')

    # try unicode return type
    t.install(unicode=1)

    print _('mullusk')



def test_api_2(localedir, mofile):
    print 'test api 2'

    gettext.bindtextdomain('gettext', localedir)
    print gettext.bindtextdomain('gettext') == localedir

    gettext.textdomain('gettext')
    # should return 'gettext'
    print gettext.textdomain()

    # local function override builtin
    _ = gettext.gettext

    # test some translations
    print _('albatross')
    print _(u'mullusk')
    print _(r'Raymond Luxury Yach-t')
    print _(ur'nudge nudge')

    # double quotes
    print _("albatross")
    print _(u"mullusk")
    print _(r"Raymond Luxury Yach-t")
    print _(ur"nudge nudge")

    # triple single quotes
    print _('''albatross''')
    print _(u'''mullusk''')
    print _(r'''Raymond Luxury Yach-t''')
    print _(ur'''nudge nudge''')

    # triple double quotes
    print _("""albatross""")
    print _(u"""mullusk""")
    print _(r"""Raymond Luxury Yach-t""")
    print _(ur"""nudge nudge""")

    # multiline strings
    print _('''This module provides internationalization and localization
support for your Python programs by providing an interface to the GNU
gettext message catalog library.''')

    # Now test dgettext()
    def _(message):
        return gettext.dgettext('gettext')



GNU_MO_DATA = '''\
3hIElQAAAAAFAAAAHAAAAEQAAAAHAAAAbAAAAAAAAACIAAAAFQAAAIkAAAChAAAAnwAAAAcAAABB
AQAACwAAAEkBAAAbAQAAVQEAABYAAABxAgAAoQAAAIgCAAAFAAAAKgMAAAkAAAAwAwAAAQAAAAQA
AAACAAAAAAAAAAUAAAAAAAAAAwAAAABSYXltb25kIEx1eHVyeSBZYWNoLXQAVGhpcyBtb2R1bGUg
cHJvdmlkZXMgaW50ZXJuYXRpb25hbGl6YXRpb24gYW5kIGxvY2FsaXphdGlvbgpzdXBwb3J0IGZv
ciB5b3VyIFB5dGhvbiBwcm9ncmFtcyBieSBwcm92aWRpbmcgYW4gaW50ZXJmYWNlIHRvIHRoZSBH
TlUKZ2V0dGV4dCBtZXNzYWdlIGNhdGFsb2cgbGlicmFyeS4AbXVsbHVzawBudWRnZSBudWRnZQBQ
cm9qZWN0LUlkLVZlcnNpb246IDIuMApQTy1SZXZpc2lvbi1EYXRlOiAyMDAwLTA4LTI5IDEyOjE5
LTA0OjAwCkxhc3QtVHJhbnNsYXRvcjogQmFycnkgQS4gV2Fyc2F3IDxid2Fyc2F3QGJlb3Blbi5j
b20+Ckxhbmd1YWdlLVRlYW06IFhYIDxweXRob24tZGV2QHB5dGhvbi5vcmc+Ck1JTUUtVmVyc2lv
bjogMS4wCkNvbnRlbnQtVHlwZTogdGV4dC9wbGFpbjsgY2hhcnNldD1rb2k4X3IKQ29udGVudC1U
cmFuc2Zlci1FbmNvZGluZzogbm9uZQpHZW5lcmF0ZWQtQnk6IHB5Z2V0dGV4dC5weSAxLjEKAFRo
cm9hdHdvYmJsZXIgTWFuZ3JvdmUAR3V2ZiB6YnFoeXIgY2ViaXZxcmYgdmFncmVhbmd2YmFueXZt
bmd2YmEgbmFxIHlicG55dm1uZ3ZiYQpmaGNjYmVnIHNiZSBsYmhlIENsZ3ViYSBjZWJ0ZW56ZiBv
bCBjZWJpdnF2YXQgbmEgdmFncmVzbnByIGdiIGd1ciBUQUgKdHJnZ3JrZyB6cmZmbnRyIHBuZ255
YnQgeXZvZW5lbC4AYmFjb24Ad2luayB3aW5rAA==
'''


LOCALEDIR = os.path.join('xx', 'LC_MESSAGES')
MOFILE = os.path.join(LOCALEDIR, 'gettext.mo')

def setup():
    os.makedirs(LOCALEDIR)
    fp = open(MOFILE, 'wb')
    fp.write(base64.decodestring(GNU_MO_DATA))
    fp.close()

def teardown():
    os.unlink(MOFILE)
    os.removedirs(LOCALEDIR)


try:
    setup()
    test_api_1(os.curdir, MOFILE)
    test_api_2(os.curdir, MOFILE)
finally:
    teardown()
    pass



# For reference, here's the .po file used to created the .mo data above.

'''
# Dummy translation for Python's test_gettext.py module.
# Copyright (C) 2000 BeOpen.com
# Barry Warsaw <bwarsaw@beopen.com>, 2000.
#
msgid ""
msgstr ""
"Project-Id-Version: 2.0\n"
"PO-Revision-Date: 2000-08-29 12:19-04:00\n"
"Last-Translator: Barry A. Warsaw <bwarsaw@beopen.com>\n"
"Language-Team: XX <python-dev@python.org>\n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=koi8_r\n"
"Content-Transfer-Encoding: none\n"
"Generated-By: pygettext.py 1.1\n"

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
'''
