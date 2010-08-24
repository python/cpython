'''Test case for "self.__dict__ = self" circular reference bug (#1469629)'''

import gc

class LeakyDict(dict):
    pass

def leak():
    ld = LeakyDict()
    ld.__dict__ = ld
    del ld
    gc.collect(); gc.collect(); gc.collect()
