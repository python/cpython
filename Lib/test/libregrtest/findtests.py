import os

from .utils import StrPath, TestName, TestList


# If these test directories are encountered recurse into them and treat each
# "test_*.py" file or each sub-directory as a separate test module. This can
# increase parallelism.
#
# Beware this can't generally be done for any directory with sub-tests as the
# __init__.py may do things which alter what tests are to be run.
SPLITTESTDIRS: set[TestName] = {
    "test_asyncio",
    "test_concurrent_futures",
    "test_multiprocessing_fork",
    "test_multiprocessing_forkserver",
    "test_multiprocessing_spawn",
}


def findtestdir(path=None):
    return path or os.path.dirname(os.path.dirname(__file__)) or os.curdir


def findtests(*, testdir: StrPath | None = None, exclude=(),
              split_test_dirs: set[TestName] = SPLITTESTDIRS,
              base_mod: str = "") -> TestList:
    """Return a list of all applicable test modules."""
    testdir = findtestdir(testdir)
    tests = []
    for name in os.listdir(testdir):
        mod, ext = os.path.splitext(name)
        if (not mod.startswith("test_")) or (mod in exclude):
            continue
        if mod in split_test_dirs:
            subdir = os.path.join(testdir, mod)
            mod = f"{base_mod or 'test'}.{mod}"
            tests.extend(findtests(testdir=subdir, exclude=exclude,
                                   split_test_dirs=split_test_dirs,
                                   base_mod=mod))
        elif ext in (".py", ""):
            tests.append(f"{base_mod}.{mod}" if base_mod else mod)
    return sorted(tests)


def split_test_packages(tests, *, testdir: StrPath | None = None, exclude=(),
                        split_test_dirs=SPLITTESTDIRS):
    testdir = findtestdir(testdir)
    splitted = []
    for name in tests:
        if name in split_test_dirs:
            subdir = os.path.join(testdir, name)
            splitted.extend(findtests(testdir=subdir, exclude=exclude,
                                      split_test_dirs=split_test_dirs,
                                      base_mod=name))
        else:
            splitted.append(name)
    return splitted
