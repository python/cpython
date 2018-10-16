# Remove all the .pyc files under ../Lib.


def deltree(root):
    import os
    from os.path import join

    npyc = 0
    for root, dirs, files in os.walk(root):
        for name in files:
            # to be thorough
            if name.endswith(('.pyc', '.pyo')):
                npyc += 1
                os.remove(join(root, name))

    return npyc

npyc = deltree("../Lib")
print(npyc, ".pyc deleted")
