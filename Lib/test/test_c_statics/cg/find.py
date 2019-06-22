from . import info


def statics(dirs, ignored, known):
    """Return a list of (StaticVar, <supported>) for each found static var."""
    if not dirs:
        return []

    return [
            (info.StaticVar(
                filename='src1/spam.c',
                funcname=None,
                name='var1',
                vartype='const char *',
                ),
             True,
             ),
            (info.StaticVar(
                filename='src1/spam.c',
                funcname='ham',
                name='initialized',
                vartype='int',
                ),
             True,
             ),
            (info.StaticVar(
                filename='src1/spam.c',
                funcname=None,
                name='var2',
                vartype='PyObject *',
                ),
             False,
             ),
            (info.StaticVar(
                filename='src1/eggs.c',
                funcname='tofu',
                name='ready',
                vartype='int',
                ),
             False,
             ),
            (info.StaticVar(
                filename='src1/spam.c',
                funcname=None,
                name='freelist',
                vartype='(PyTupleObject *)[10]',
                ),
             False,
             ),
            (info.StaticVar(
                filename='src1/sub/ham.c',
                funcname=None,
                name='var1',
                vartype='const char const *',
                ),
             True,
             ),
            (info.StaticVar(
                filename='src2/jam.c',
                funcname=None,
                name='var1',
                vartype='int',
                ),
             True,
             ),
            (info.StaticVar(
                filename='src2/jam.c',
                funcname=None,
                name='var2',
                vartype='MyObject *',
                ),
             False,
             ),
            (info.StaticVar(
                filename='Include/spam.h',
                funcname=None,
                name='data',
                vartype='const int',
                ),
             True,
             ),
            ]
