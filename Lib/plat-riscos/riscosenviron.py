"""A more or less complete dictionary like interface for the RISC OS environment."""

import riscos

class _Environ:
    def __init__(self, initial = None):
        pass
    def __repr__(self):
        return repr(riscos.getenvdict())
    def __cmp__(self, dict):
        return cmp(riscos.getenvdict(), dict)
    def __len__(self):
        return len(riscos.getenvdict())
    def __getitem__(self, key):
        ret = riscos.getenv(key)
        if ret<>None:
            return ret
        else:
            raise KeyError
    def __setitem__(self, key, item):
        riscos.putenv(key, item)
    def __delitem__(self, key):
        riscos.delenv(key)
    def clear(self):
        # too dangerous on RISC OS
        pass
    def copy(self):
        return riscos.getenvdict()
    def keys(self): return riscos.getenvdict().keys()
    def items(self): return riscos.getenvdict().items()
    def values(self): return riscos.getenvdict().values()
    def has_key(self, key):
        value = riscos.getenv(key)
        return value<>None
    def __contains__(self, key):
        return riscos.getenv(key) is not None
    def update(self, dict):
        for k, v in dict.items():
            riscos.putenv(k, v)
    def get(self, key, failobj=None):
        value = riscos.getenv(key)
        if value<>None:
            return value
        else:
            return failobj
