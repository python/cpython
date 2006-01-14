
# http://python.org/sf/1174712

import types

class X(types.ModuleType, str):
    """Such a subclassing is incorrectly allowed --
    see the SF bug report for explanations"""

if __name__ == '__main__':
    X('name')    # segfault: ModuleType.__init__() reads
                 # the dict at the wrong offset
