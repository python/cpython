# Deprecated alias for xml.etree.ElementTree

from xml.etree.ElementTree import *

from warnings import warn as _warn

_warn(
    "xml.etree.cElementTree is deprecated, use xml.etree.ElementTree instead",
    PendingDeprecationWarning
)

del _warn
