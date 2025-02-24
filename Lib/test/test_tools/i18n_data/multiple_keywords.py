from gettext import gettext as foo

foo('bar')
foo('baz', 'qux')
# The 't' specifier is not supported, so this is extracted as pgettext
foo('corge', 'grault', 1)
foo('xyzzy', 'foo', 'foos', 1)
