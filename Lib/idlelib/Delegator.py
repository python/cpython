
class Delegator:

    # The cache is only used to be able to change delegates!

    def __init__(self, delegate=None):
        self.delegate = delegate
        self.__cache = {}

    def __getattr__(self, name):
        attr = getattr(self.delegate, name) # May raise AttributeError
        setattr(self, name, attr)
        self.__cache[name] = attr
        return attr

    def resetcache(self):
        for key in self.__cache.keys():
            try:
                delattr(self, key)
            except AttributeError:
                pass
        self.__cache.clear()

    def cachereport(self):
        keys = self.__cache.keys()
        keys.sort()
        print keys

    def setdelegate(self, delegate):
        self.resetcache()
        self.delegate = delegate

    def getdelegate(self):
        return self.delegate
