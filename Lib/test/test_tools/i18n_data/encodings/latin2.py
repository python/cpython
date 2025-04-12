# -*- coding: iso-8859-2 -*-

from gettext import gettext as _


# ascii text
_('foo')

# latin-2 text
_('Ř ř')

# non-latin-2 text
_('\u03b1 \u03b2')

# ascii text with non-ascii comment
# Ř ř
_('bar')
