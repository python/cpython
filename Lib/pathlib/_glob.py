import functools
import operator
import os


re = glob = None
special_parts = ('', '.', '..')
no_recurse_symlinks = object()


@functools.lru_cache(maxsize=512)
def compile_pattern(pat, sep, case_sensitive, recursive=True):
    """Compile given glob pattern to a re.Pattern object (observing case
    sensitivity)."""
    global re, glob
    if re is None:
        import re, glob

    flags = re.NOFLAG if case_sensitive else re.IGNORECASE
    regex = glob.translate(pat, recursive=recursive, include_hidden=True, seps=sep)
    return re.compile(regex, flags=flags).match


class Globber:
    """Class providing shell-style pattern matching and globbing.
    """

    def __init__(self,  sep, case_sensitive, recursive=False):
        self.sep = sep
        self.case_sensitive = case_sensitive
        self.recursive = recursive

    # Low-level methods

    lstat = staticmethod(os.lstat)
    scandir = staticmethod(os.scandir)
    parse_entry = operator.attrgetter('path')
    concat_path = operator.add

    if os.name == 'nt':
        def add_slash(self, pathname):
            tail = os.path.splitroot(pathname)[2]
            if not tail or tail[-1] in '\\/':
                return pathname
            return f'{pathname}\\'
    else:
        def add_slash(self, pathname):
            if not pathname or pathname[-1] == '/':
                return pathname
            return f'{pathname}/'

    # High-level methods

    def compile(self, pat):
        return compile_pattern(pat, self.sep, self.case_sensitive, self.recursive)

    def selector(self, parts):
        """Returns a function that selects from a given path, walking and
        filtering according to the glob-style pattern parts in *parts*.
        """
        if not parts:
            return self.select_exists
        part = parts.pop()
        if self.recursive and part == '**':
            selector = self.recursive_selector
        elif part in special_parts:
            selector = self.special_selector
        else:
            selector = self.wildcard_selector
        return selector(part, parts)

    def special_selector(self, part, parts):
        """Returns a function that selects special children of the given path.
        """
        select_next = self.selector(parts)

        def select_special(path, exists=False):
            path = self.concat_path(self.add_slash(path), part)
            return select_next(path, exists)
        return select_special

    def wildcard_selector(self, part, parts):
        """Returns a function that selects direct children of a given path,
        filtering by pattern.
        """

        match = None if part == '*' else self.compile(part)
        dir_only = bool(parts)
        if dir_only:
            select_next = self.selector(parts)

        def select_wildcard(path, exists=False):
            try:
                # We must close the scandir() object before proceeding to
                # avoid exhausting file descriptors when globbing deep trees.
                with self.scandir(path) as scandir_it:
                    entries = list(scandir_it)
                for entry in entries:
                    if match is None or match(entry.name):
                        if dir_only:
                            try:
                                if not entry.is_dir():
                                    continue
                            except OSError:
                                continue
                        entry_path = self.parse_entry(entry)
                        if dir_only:
                            yield from select_next(entry_path, exists=True)
                        else:
                            yield entry_path
            except OSError:
                pass
        return select_wildcard

    def recursive_selector(self, part, parts):
        """Returns a function that selects a given path and all its children,
        recursively, filtering by pattern.
        """
        # Optimization: consume following '**' parts, which have no effect.
        while parts and parts[-1] == '**':
            parts.pop()

        # Optimization: consume and join any following non-special parts here,
        # rather than leaving them for the next selector. They're used to
        # build a regular expression, which we use to filter the results of
        # the recursive walk. As a result, non-special pattern segments
        # following a '**' wildcard don't require additional filesystem access
        # to expand.
        follow_symlinks = self.recursive is not no_recurse_symlinks
        if follow_symlinks:
            while parts and parts[-1] not in special_parts:
                part += self.sep + parts.pop()

        match = None if part == '**' else self.compile(part)
        dir_only = bool(parts)
        select_next = self.selector(parts)

        def select_recursive(path, exists=False):
            path = self.add_slash(path)
            match_pos = len(str(path))
            if match is None or match(str(path), match_pos):
                yield from select_next(path, exists)
            stack = [path]
            while stack:
                yield from select_recursive_step(stack, match_pos)

        def select_recursive_step(stack, match_pos):
            path = stack.pop()
            try:
                # We must close the scandir() object before proceeding to
                # avoid exhausting file descriptors when globbing deep trees.
                with self.scandir(path) as scandir_it:
                    entries = list(scandir_it)
            except OSError:
                pass
            else:
                for entry in entries:
                    is_dir = False
                    try:
                        if entry.is_dir(follow_symlinks=follow_symlinks):
                            is_dir = True
                    except OSError:
                        pass

                    if is_dir or not dir_only:
                        entry_path = self.parse_entry(entry)
                        if match is None or match(str(entry_path), match_pos):
                            if dir_only:
                                yield from select_next(entry_path, exists=True)
                            else:
                                # Optimization: directly yield the path if this is
                                # last pattern part.
                                yield entry_path
                        if is_dir:
                            stack.append(entry_path)

        return select_recursive

    def select_exists(self, path, exists=False):
        """Yields the given path, if it exists.
        """
        if exists:
            # Optimization: this path is already known to exist, e.g. because
            # it was returned from os.scandir(), so we skip calling lstat().
            yield path
        else:
            try:
                self.lstat(path)
                yield path
            except OSError:
                pass
