# -*- coding: utf-8 -*-
from towel_stuff import Towel

class Chocolate(object):
    """A piece of chocolate."""

    def wrap_with_towel(self):
        towel = Towel()
        towel.wrap(self)
        return towel
