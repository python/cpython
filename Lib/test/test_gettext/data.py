import base64

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

GNU_MO_DATA_ISSUE_17898 = b'''\
3hIElQAAAAABAAAAHAAAACQAAAAAAAAAAAAAAAAAAAAsAAAAggAAAC0AAAAAUGx1cmFsLUZvcm1z
OiBucGx1cmFscz0yOyBwbHVyYWw9KG4gIT0gMSk7CiMtIy0jLSMtIyAgbWVzc2FnZXMucG8gKEVk
WCBTdHVkaW8pICAjLSMtIy0jLSMKQ29udGVudC1UeXBlOiB0ZXh0L3BsYWluOyBjaGFyc2V0PVVU
Ri04CgA=
'''

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
# messages.po, used for bug 62098
#

b'''
# test file for http://bugs.python.org/issue17898
msgid ""
msgstr ""
"Plural-Forms: nplurals=2; plural=(n != 1);\n"
"#-#-#-#-#  messages.po (EdX Studio)  #-#-#-#-#\n"
"Content-Type: text/plain; charset=UTF-8\n"
'''
