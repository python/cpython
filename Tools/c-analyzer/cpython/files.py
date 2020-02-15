from c_analyzer.common.files import (
        C_SOURCE_SUFFIXES, walk_tree, iter_files_by_suffix,
        )

from . import SOURCE_DIRS, REPO_ROOT

# XXX need tests:
# * iter_files()


def iter_files(*,
               walk=walk_tree,
               _files=iter_files_by_suffix,
               ):
    """Yield each file in the tree for each of the given directory names."""
    excludedtrees = [
        os.path.join('Include', 'cpython', ''),
        ]
    def is_excluded(filename):
        for root in excludedtrees:
            if filename.startswith(root):
                return True
        return False
    for filename in _files(SOURCE_DIRS, C_SOURCE_SUFFIXES, REPO_ROOT,
                           walk=walk,
                           ):
        if is_excluded(filename):
            continue
        yield filename
