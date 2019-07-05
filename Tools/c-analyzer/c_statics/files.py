import os
import os.path


def iter_files(dirnames, filter_by_name=None, *,
               _walk=os.walk,
               ):
    """Yield each file in the tree for each of the given directory names."""
    if filter_by_name is None:
        def filter_by_name(name):
            return True
    elif not callable(filter_by_name):
        suffixes = tuple(filter_by_name)
        def filter_by_name(name):
            for suffix in suffixes:
                if name.endswith(suffix):
                    return True
            return False

    for dirname in dirnames:
        for parent, _, names in _walk(dirname):
            if parent.endswith(os.path.join('', 'Include', 'cpython')):
                continue
            for name in names:
                if not filter_by_name(name):
                    continue
                yield os.path.join(parent, name)
