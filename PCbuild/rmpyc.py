# Remove all the .pyc and .pyo files under ../Lib.

from __future__ import nested_scopes

def deltree(root):
    import os
    def rm(path):
        os.unlink(path)
    npyc = npyo = 0
    dirs = [root]
    while dirs:
        dir = dirs.pop()
        for short in os.listdir(dir):
            full = os.path.join(dir, short)
            if os.path.isdir(full):
                dirs.append(full)
            elif short.endswith(".pyc"):
                npyc += 1
                rm(full)
            elif short.endswith(".pyo"):
                npyo += 1
                rm(full)
    return npyc, npyo

npyc, npyo = deltree("../Lib")
print npyc, ".pyc deleted,", npyo, ".pyo deleted"
