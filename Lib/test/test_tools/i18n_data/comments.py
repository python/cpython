from gettext import gettext as _

# Not a translator comment
_('foo')

# i18n: This is a translator comment
_('bar')

# i18n: This is a translator comment
# i18n: This is another translator comment
_('baz')

# i18n: This is a translator comment
# with multiple
# lines
_('qux')

# This comment should not be included because
# it does not start with the prefix
# i18n: This is a translator comment
_('quux')

# i18n: This is a translator comment
# with multiple lines
# i18n: This is another translator comment
# with multiple lines
_('corge')

# i18n: This comment should be ignored

_('grault')

# i18n: This comment should be ignored

# i18n: This is another translator comment
_('garply')

# i18n: comment should be ignored
x = 1
_('george')

# i18n: This comment should be ignored
x = 1
# i18n: This is another translator comment
_('waldo')

# i18n: This is a translator comment
x = 1  # i18n: This is also a translator comment
# i18n: This is another translator comment
_('waldo2')

# i18n: This is a translator comment
_('fred')

# i18n: This is another translator comment
_('fred')

# i18n: This is yet another translator comment
_('fred')

# i18n: This is a translator comment
# with multiple lines
_('fred')

_('plugh')  # i18n: This comment should be ignored

_('foo'  # i18n: This comment should be ignored
  'bar')  # i18n: This comment should be ignored

# i18n: This is a translator comment
_('xyzzy')
_('thud')


## i18n: This is a translator comment
# # i18n: This is another translator comment
### ###    i18n: This is yet another translator comment
_('foos')
