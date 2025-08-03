from gettext import (
    gettext as _,
)
from gettext import (
    gettext as bar,
)
from gettext import (
    gettext as foo,
)
from gettext import (
    ngettext as nfoo,
)
from gettext import (
    npgettext as npfoo,
)
from gettext import (
    pgettext as pfoo,
)

foo('bar')
foo('bar', 'baz')

nfoo('cat', 'cats', 1)
nfoo('dog', 'dogs')

pfoo('context', 'bar')

npfoo('context', 'cat', 'cats', 1)

# This is an unknown keyword and should be ignored
bar('baz')

# 'nfoo' requires at least 2 arguments
nfoo('dog')

# 'pfoo' requires at least 2 arguments
pfoo('context')

# 'npfoo' requires at least 3 arguments
npfoo('context')
npfoo('context', 'cat')

# --keyword should override the default keyword
_('overridden', 'default')
