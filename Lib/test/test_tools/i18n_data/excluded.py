from gettext import gettext as _, ngettext, npgettext, pgettext


_('foo')

_('excluded')

_('multiline\nexcluded')

ngettext('singular excluded', 'plural excluded', 2)

pgettext('context', 'context excluded')

npgettext('context', 'context singular excluded', 'context plural excluded', 2)
