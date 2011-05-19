# -*- coding: utf-8 -*-

class Towel(object):
    """A towel, that one should never be without."""

    def __init__(self, color='tie-dye'):
        self.color = color
        self.wrapped_obj = None

    def wrap(self, obj):
        """Wrap an object up in our towel."""
        self.wrapped_obj = obj

    def unwrap(self):
        """Unwrap whatever is in our towel and return whatever it is."""
        obj = self.wrapped_obj
        self.wrapped_obj = None
        return obj
