"""Fix bound method attributes (method.im_? -> method.__?__).
"""
# Author: Christian Heimes

# Local imports
from . import basefix
from .util import Name

MAP = {
    "im_func" : "__func__",
    "im_self" : "__self__",
    "im_class" : "__self__.__class__"
    }

class FixMethodattrs(basefix.BaseFix):
    PATTERN = """
    power< any+ trailer< '.' attr=('im_func' | 'im_self' | 'im_class') > any* >
    """

    def transform(self, node, results):
        attr = results["attr"][0]
        new = MAP[attr.value]
        attr.replace(Name(new, prefix=attr.get_prefix()))
