""" Test script for the unicodedata module.

Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""#"
from test_support import verbose
import sys

# Test Unicode database APIs
import unicodedata

print 'Testing unicodedata module...',

assert unicodedata.digit(u'A',None) is None
assert unicodedata.digit(u'9') == 9
assert unicodedata.digit(u'\u215b',None) is None
assert unicodedata.digit(u'\u2468') == 9

assert unicodedata.numeric(u'A',None) is None
assert unicodedata.numeric(u'9') == 9
assert unicodedata.numeric(u'\u215b') == 0.125
assert unicodedata.numeric(u'\u2468') == 9.0

assert unicodedata.decimal(u'A',None) is None
assert unicodedata.decimal(u'9') == 9
assert unicodedata.decimal(u'\u215b',None) is None
assert unicodedata.decimal(u'\u2468',None) is None

assert unicodedata.category(u'\uFFFE') == 'Cn'
assert unicodedata.category(u'a') == 'Ll'
assert unicodedata.category(u'A') == 'Lu'

assert unicodedata.bidirectional(u'\uFFFE') == ''
assert unicodedata.bidirectional(u' ') == 'WS'
assert unicodedata.bidirectional(u'A') == 'L'

assert unicodedata.decomposition(u'\uFFFE') == ''
assert unicodedata.decomposition(u'\u00bc') == '<fraction> 0031 2044 0034'

assert unicodedata.mirrored(u'\uFFFE') == 0
assert unicodedata.mirrored(u'a') == 0
assert unicodedata.mirrored(u'\u2201') == 1

assert unicodedata.combining(u'\uFFFE') == 0
assert unicodedata.combining(u'a') == 0
assert unicodedata.combining(u'\u20e1') == 230

print 'done.'
