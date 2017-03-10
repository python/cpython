# Remove all the .pyc files under ../Lib.


def deltree(root):
    import os
    from os.path import join

    npyc = 0
    for root, dirs, files in os.walk(root):
        for name in files:
            delete = False
            if name.endswith('.pyc'):
                delete = True
                npyc += 1

            if delete:
                os.remove(join(root, name))

    return npyc

npyc = deltree("../Lib")
print(npyc, ".pyc deleted")
