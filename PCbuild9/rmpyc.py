# Remove all the .pyc and .pyo files under ../Lib.


def deltree(root):
    import os
    from os.path import join

    npyc = npyo = 0
    for root, dirs, files in os.walk(root):
        for name in files:
            delete = False
            if name.endswith('.pyc'):
                delete = True
                npyc += 1
            elif name.endswith('.pyo'):
                delete = True
                npyo += 1

            if delete:
                os.remove(join(root, name))

    return npyc, npyo

npyc, npyo = deltree("../Lib")
print(npyc, ".pyc deleted,", npyo, ".pyo deleted")
