"""Utility class and function to get information about the CVS repository
based on checked-out files.
"""

import os


def get_repository_list(paths):
    d = {}
    for name in paths:
        if os.path.isfile(name):
            dir = os.path.dirname(name)
        else:
            dir = name
        rootfile = os.path.join(name, "CVS", "Root")
        root = open(rootfile).readline().strip()
        if not d.has_key(root):
            d[root] = RepositoryInfo(dir), [name]
        else:
            d[root][1].append(name)
    return d.values()


class RepositoryInfo:
    """Record holding information about the repository we want to talk to."""
    cvsroot_path = None
    branch = None

    # type is '', ':ext', or ':pserver:'
    type = ""

    def __init__(self, dir=None):
        if dir is None:
            dir = os.getcwd()
        dir = os.path.join(dir, "CVS")
        root = open(os.path.join(dir, "Root")).readline().strip()
        if root.startswith(":pserver:"):
            self.type = ":pserver:"
            root = root[len(":pserver:"):]
        elif ":" in root:
            if root.startswith(":ext:"):
                root = root[len(":ext:"):]
            self.type = ":ext:"
        self.repository = root
        if ":" in root:
            host, path = root.split(":", 1)
            self.cvsroot_path = path
        else:
            self.cvsroot_path = root
        fn = os.path.join(dir, "Tag")
        if os.path.isfile(fn):
            self.branch = open(fn).readline().strip()[1:]

    def get_cvsroot(self):
        return self.type + self.repository

    _repository_dir_cache = {}

    def get_repository_file(self, path):
        filename = os.path.abspath(path)
        if os.path.isdir(path):
            dir = path
            join = 0
        else:
            dir = os.path.dirname(path)
            join = 1
        try:
            repodir = self._repository_dir_cache[dir]
        except KeyError:
            repofn = os.path.join(dir, "CVS", "Repository")
            repodir = open(repofn).readline().strip()
            repodir = os.path.join(self.cvsroot_path, repodir)
            self._repository_dir_cache[dir] = repodir
        if join:
            fn = os.path.join(repodir, os.path.basename(path))
        else:
            fn = repodir
        return fn[len(self.cvsroot_path)+1:]

    def __repr__(self):
        return "<RepositoryInfo for %r>" % self.get_cvsroot()
