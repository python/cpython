from gettext import gettext as foo

foo('bar')

foo('baz', 'qux')

# The 't' specifier is not supported, so the following
# call is extracted as pgettext instead of ngettext.
foo('corge', 'grault', 1)

foo('xyzzy', 'foo', 'foos', 1)
