""" Test script for the unicodedata module.

    Written by Marc-Andre Lemburg (mal@lemburg.com).

    (c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""#"
import sha

encoding = 'utf-8'

def test_methods():

    h = sha.sha()
    for i in range(65536):
        char = unichr(i)
        data = [
            
            # Predicates (single char)
            char.isalnum() and u'1' or u'0',
            char.isalpha() and u'1' or u'0',
            char.isdecimal() and u'1' or u'0',
            char.isdigit() and u'1' or u'0',
            char.islower() and u'1' or u'0',
            char.isnumeric() and u'1' or u'0',
            char.isspace() and u'1' or u'0',
            char.istitle() and u'1' or u'0',
            char.isupper() and u'1' or u'0',
            
            # Predicates (multiple chars)
            (char + u'abc').isalnum() and u'1' or u'0',
            (char + u'abc').isalpha() and u'1' or u'0',
            (char + u'123').isdecimal() and u'1' or u'0',
            (char + u'123').isdigit() and u'1' or u'0',
            (char + u'abc').islower() and u'1' or u'0',
            (char + u'123').isnumeric() and u'1' or u'0',
            (char + u' \t').isspace() and u'1' or u'0',
            (char + u'abc').istitle() and u'1' or u'0',
            (char + u'ABC').isupper() and u'1' or u'0',

            # Mappings (single char)
            char.lower(),
            char.upper(),
            char.title(),
            
            # Mappings (multiple chars)
            (char + u'abc').lower(),
            (char + u'ABC').upper(),
            (char + u'abc').title(),
            (char + u'ABC').title(),
            
            ]
        h.update(u''.join(data).encode(encoding))
    return h.hexdigest()

def test_unicodedata():

    h = sha.sha()
    for i in range(65536):
        char = unichr(i)
        data = [
            # Properties
            str(unicodedata.digit(char, -1)),
            str(unicodedata.numeric(char, -1)),
            str(unicodedata.decimal(char, -1)),
            unicodedata.category(char),
            unicodedata.bidirectional(char),
            unicodedata.decomposition(char),
            str(unicodedata.mirrored(char)),
            str(unicodedata.combining(char)),
            ] 
        h.update(''.join(data))
    return h.hexdigest()

### Run tests

print 'Testing Unicode Database...'
print 'Methods:',
print test_methods()

# In case unicodedata is not available, this will raise an ImportError,
# but still test the above cases...
import unicodedata
print 'Functions:',
print test_unicodedata()

# Some additional checks of the API:
print 'API:',

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

print 'ok'
