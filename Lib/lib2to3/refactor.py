#!/usr/bin/env python2.5
# Copyright 2006 Google, Inc. All Rights Reserved.
# Licensed to PSF under a Contributor Agreement.

"""Refactoring framework.

Used as a main program, this can refactor any number of files and/or
recursively descend down directories.  Imported as a module, this
provides infrastructure to write your own refactoring tool.
"""

__author__ = "Guido van Rossum <guido@python.org>"


# Python imports
import os
import sys
import difflib
import logging
import operator
from collections import defaultdict
from itertools import chain

# Local imports
from .pgen2 import driver
from .pgen2 import tokenize

from . import pytree
from . import patcomp
from . import fixes
from . import pygram


def get_all_fix_names(fixer_pkg, remove_prefix=True):
    """Return a sorted list of all available fix names in the given package."""
    pkg = __import__(fixer_pkg, [], [], ["*"])
    fixer_dir = os.path.dirname(pkg.__file__)
    fix_names = []
    for name in sorted(os.listdir(fixer_dir)):
        if name.startswith("fix_") and name.endswith(".py"):
            if remove_prefix:
                name = name[4:]
            fix_names.append(name[:-3])
    return fix_names

def get_head_types(pat):
    """ Accepts a pytree Pattern Node and returns a set
        of the pattern types which will match first. """

    if isinstance(pat, (pytree.NodePattern, pytree.LeafPattern)):
        # NodePatters must either have no type and no content
        #   or a type and content -- so they don't get any farther
        # Always return leafs
        return set([pat.type])

    if isinstance(pat, pytree.NegatedPattern):
        if pat.content:
            return get_head_types(pat.content)
        return set([None]) # Negated Patterns don't have a type

    if isinstance(pat, pytree.WildcardPattern):
        # Recurse on each node in content
        r = set()
        for p in pat.content:
            for x in p:
                r.update(get_head_types(x))
        return r

    raise Exception("Oh no! I don't understand pattern %s" %(pat))

def get_headnode_dict(fixer_list):
    """ Accepts a list of fixers and returns a dictionary
        of head node type --> fixer list.  """
    head_nodes = defaultdict(list)
    for fixer in fixer_list:
        if not fixer.pattern:
            head_nodes[None].append(fixer)
            continue
        for t in get_head_types(fixer.pattern):
            head_nodes[t].append(fixer)
    return head_nodes

def get_fixers_from_package(pkg_name):
    """
    Return the fully qualified names for fixers in the package pkg_name.
    """
    return [pkg_name + "." + fix_name
            for fix_name in get_all_fix_names(pkg_name, False)]


class FixerError(Exception):
    """A fixer could not be loaded."""


class RefactoringTool(object):

    _default_options = {"print_function": False}

    CLASS_PREFIX = "Fix" # The prefix for fixer classes
    FILE_PREFIX = "fix_" # The prefix for modules with a fixer within

    def __init__(self, fixer_names, options=None, explicit=None):
        """Initializer.

        Args:
            fixer_names: a list of fixers to import
            options: an dict with configuration.
            explicit: a list of fixers to run even if they are explicit.
        """
        self.fixers = fixer_names
        self.explicit = explicit or []
        self.options = self._default_options.copy()
        if options is not None:
            self.options.update(options)
        self.errors = []
        self.logger = logging.getLogger("RefactoringTool")
        self.fixer_log = []
        self.wrote = False
        if self.options["print_function"]:
            del pygram.python_grammar.keywords["print"]
        self.driver = driver.Driver(pygram.python_grammar,
                                    convert=pytree.convert,
                                    logger=self.logger)
        self.pre_order, self.post_order = self.get_fixers()

        self.pre_order_heads = get_headnode_dict(self.pre_order)
        self.post_order_heads = get_headnode_dict(self.post_order)

        self.files = []  # List of files that were or should be modified

    def get_fixers(self):
        """Inspects the options to load the requested patterns and handlers.

        Returns:
          (pre_order, post_order), where pre_order is the list of fixers that
          want a pre-order AST traversal, and post_order is the list that want
          post-order traversal.
        """
        pre_order_fixers = []
        post_order_fixers = []
        for fix_mod_path in self.fixers:
            mod = __import__(fix_mod_path, {}, {}, ["*"])
            fix_name = fix_mod_path.rsplit(".", 1)[-1]
            if fix_name.startswith(self.FILE_PREFIX):
                fix_name = fix_name[len(self.FILE_PREFIX):]
            parts = fix_name.split("_")
            class_name = self.CLASS_PREFIX + "".join([p.title() for p in parts])
            try:
                fix_class = getattr(mod, class_name)
            except AttributeError:
                raise FixerError("Can't find %s.%s" % (fix_name, class_name))
            fixer = fix_class(self.options, self.fixer_log)
            if fixer.explicit and self.explicit is not True and \
                    fix_mod_path not in self.explicit:
                self.log_message("Skipping implicit fixer: %s", fix_name)
                continue

            self.log_debug("Adding transformation: %s", fix_name)
            if fixer.order == "pre":
                pre_order_fixers.append(fixer)
            elif fixer.order == "post":
                post_order_fixers.append(fixer)
            else:
                raise FixerError("Illegal fixer order: %r" % fixer.order)

        key_func = operator.attrgetter("run_order")
        pre_order_fixers.sort(key=key_func)
        post_order_fixers.sort(key=key_func)
        return (pre_order_fixers, post_order_fixers)

    def log_error(self, msg, *args, **kwds):
        """Called when an error occurs."""
        raise

    def log_message(self, msg, *args):
        """Hook to log a message."""
        if args:
            msg = msg % args
        self.logger.info(msg)

    def log_debug(self, msg, *args):
        if args:
            msg = msg % args
        self.logger.debug(msg)

    def print_output(self, lines):
        """Called with lines of output to give to the user."""
        pass

    def refactor(self, items, write=False, doctests_only=False):
        """Refactor a list of files and directories."""
        for dir_or_file in items:
            if os.path.isdir(dir_or_file):
                self.refactor_dir(dir_or_file, write, doctests_only)
            else:
                self.refactor_file(dir_or_file, write, doctests_only)

    def refactor_dir(self, dir_name, write=False, doctests_only=False):
        """Descends down a directory and refactor every Python file found.

        Python files are assumed to have a .py extension.

        Files and subdirectories starting with '.' are skipped.
        """
        for dirpath, dirnames, filenames in os.walk(dir_name):
            self.log_debug("Descending into %s", dirpath)
            dirnames.sort()
            filenames.sort()
            for name in filenames:
                if not name.startswith(".") and name.endswith("py"):
                    fullname = os.path.join(dirpath, name)
                    self.refactor_file(fullname, write, doctests_only)
            # Modify dirnames in-place to remove subdirs with leading dots
            dirnames[:] = [dn for dn in dirnames if not dn.startswith(".")]

    def refactor_file(self, filename, write=False, doctests_only=False):
        """Refactors a file."""
        try:
            f = open(filename)
        except IOError, err:
            self.log_error("Can't open %s: %s", filename, err)
            return
        try:
            input = f.read() + "\n" # Silence certain parse errors
        finally:
            f.close()
        if doctests_only:
            self.log_debug("Refactoring doctests in %s", filename)
            output = self.refactor_docstring(input, filename)
            if output != input:
                self.processed_file(output, filename, input, write=write)
            else:
                self.log_debug("No doctest changes in %s", filename)
        else:
            tree = self.refactor_string(input, filename)
            if tree and tree.was_changed:
                # The [:-1] is to take off the \n we added earlier
                self.processed_file(str(tree)[:-1], filename, write=write)
            else:
                self.log_debug("No changes in %s", filename)

    def refactor_string(self, data, name):
        """Refactor a given input string.

        Args:
            data: a string holding the code to be refactored.
            name: a human-readable name for use in error/log messages.

        Returns:
            An AST corresponding to the refactored input stream; None if
            there were errors during the parse.
        """
        try:
            tree = self.driver.parse_string(data)
        except Exception, err:
            self.log_error("Can't parse %s: %s: %s",
                           name, err.__class__.__name__, err)
            return
        self.log_debug("Refactoring %s", name)
        self.refactor_tree(tree, name)
        return tree

    def refactor_stdin(self, doctests_only=False):
        input = sys.stdin.read()
        if doctests_only:
            self.log_debug("Refactoring doctests in stdin")
            output = self.refactor_docstring(input, "<stdin>")
            if output != input:
                self.processed_file(output, "<stdin>", input)
            else:
                self.log_debug("No doctest changes in stdin")
        else:
            tree = self.refactor_string(input, "<stdin>")
            if tree and tree.was_changed:
                self.processed_file(str(tree), "<stdin>", input)
            else:
                self.log_debug("No changes in stdin")

    def refactor_tree(self, tree, name):
        """Refactors a parse tree (modifying the tree in place).

        Args:
            tree: a pytree.Node instance representing the root of the tree
                  to be refactored.
            name: a human-readable name for this tree.

        Returns:
            True if the tree was modified, False otherwise.
        """
        for fixer in chain(self.pre_order, self.post_order):
            fixer.start_tree(tree, name)

        self.traverse_by(self.pre_order_heads, tree.pre_order())
        self.traverse_by(self.post_order_heads, tree.post_order())

        for fixer in chain(self.pre_order, self.post_order):
            fixer.finish_tree(tree, name)
        return tree.was_changed

    def traverse_by(self, fixers, traversal):
        """Traverse an AST, applying a set of fixers to each node.

        This is a helper method for refactor_tree().

        Args:
            fixers: a list of fixer instances.
            traversal: a generator that yields AST nodes.

        Returns:
            None
        """
        if not fixers:
            return
        for node in traversal:
            for fixer in fixers[node.type] + fixers[None]:
                results = fixer.match(node)
                if results:
                    new = fixer.transform(node, results)
                    if new is not None and (new != node or
                                            str(new) != str(node)):
                        node.replace(new)
                        node = new

    def processed_file(self, new_text, filename, old_text=None, write=False):
        """
        Called when a file has been refactored, and there are changes.
        """
        self.files.append(filename)
        if old_text is None:
            try:
                f = open(filename, "r")
            except IOError, err:
                self.log_error("Can't read %s: %s", filename, err)
                return
            try:
                old_text = f.read()
            finally:
                f.close()
        if old_text == new_text:
            self.log_debug("No changes to %s", filename)
            return
        self.print_output(diff_texts(old_text, new_text, filename))
        if write:
            self.write_file(new_text, filename, old_text)
        else:
            self.log_debug("Not writing changes to %s", filename)

    def write_file(self, new_text, filename, old_text):
        """Writes a string to a file.

        It first shows a unified diff between the old text and the new text, and
        then rewrites the file; the latter is only done if the write option is
        set.
        """
        try:
            f = open(filename, "w")
        except os.error, err:
            self.log_error("Can't create %s: %s", filename, err)
            return
        try:
            f.write(new_text)
        except os.error, err:
            self.log_error("Can't write %s: %s", filename, err)
        finally:
            f.close()
        self.log_debug("Wrote changes to %s", filename)
        self.wrote = True

    PS1 = ">>> "
    PS2 = "... "

    def refactor_docstring(self, input, filename):
        """Refactors a docstring, looking for doctests.

        This returns a modified version of the input string.  It looks
        for doctests, which start with a ">>>" prompt, and may be
        continued with "..." prompts, as long as the "..." is indented
        the same as the ">>>".

        (Unfortunately we can't use the doctest module's parser,
        since, like most parsers, it is not geared towards preserving
        the original source.)
        """
        result = []
        block = None
        block_lineno = None
        indent = None
        lineno = 0
        for line in input.splitlines(True):
            lineno += 1
            if line.lstrip().startswith(self.PS1):
                if block is not None:
                    result.extend(self.refactor_doctest(block, block_lineno,
                                                        indent, filename))
                block_lineno = lineno
                block = [line]
                i = line.find(self.PS1)
                indent = line[:i]
            elif (indent is not None and
                  (line.startswith(indent + self.PS2) or
                   line == indent + self.PS2.rstrip() + "\n")):
                block.append(line)
            else:
                if block is not None:
                    result.extend(self.refactor_doctest(block, block_lineno,
                                                        indent, filename))
                block = None
                indent = None
                result.append(line)
        if block is not None:
            result.extend(self.refactor_doctest(block, block_lineno,
                                                indent, filename))
        return "".join(result)

    def refactor_doctest(self, block, lineno, indent, filename):
        """Refactors one doctest.

        A doctest is given as a block of lines, the first of which starts
        with ">>>" (possibly indented), while the remaining lines start
        with "..." (identically indented).

        """
        try:
            tree = self.parse_block(block, lineno, indent)
        except Exception, err:
            if self.log.isEnabledFor(logging.DEBUG):
                for line in block:
                    self.log_debug("Source: %s", line.rstrip("\n"))
            self.log_error("Can't parse docstring in %s line %s: %s: %s",
                           filename, lineno, err.__class__.__name__, err)
            return block
        if self.refactor_tree(tree, filename):
            new = str(tree).splitlines(True)
            # Undo the adjustment of the line numbers in wrap_toks() below.
            clipped, new = new[:lineno-1], new[lineno-1:]
            assert clipped == ["\n"] * (lineno-1), clipped
            if not new[-1].endswith("\n"):
                new[-1] += "\n"
            block = [indent + self.PS1 + new.pop(0)]
            if new:
                block += [indent + self.PS2 + line for line in new]
        return block

    def summarize(self):
        if self.wrote:
            were = "were"
        else:
            were = "need to be"
        if not self.files:
            self.log_message("No files %s modified.", were)
        else:
            self.log_message("Files that %s modified:", were)
            for file in self.files:
                self.log_message(file)
        if self.fixer_log:
            self.log_message("Warnings/messages while refactoring:")
            for message in self.fixer_log:
                self.log_message(message)
        if self.errors:
            if len(self.errors) == 1:
                self.log_message("There was 1 error:")
            else:
                self.log_message("There were %d errors:", len(self.errors))
            for msg, args, kwds in self.errors:
                self.log_message(msg, *args, **kwds)

    def parse_block(self, block, lineno, indent):
        """Parses a block into a tree.

        This is necessary to get correct line number / offset information
        in the parser diagnostics and embedded into the parse tree.
        """
        return self.driver.parse_tokens(self.wrap_toks(block, lineno, indent))

    def wrap_toks(self, block, lineno, indent):
        """Wraps a tokenize stream to systematically modify start/end."""
        tokens = tokenize.generate_tokens(self.gen_lines(block, indent).next)
        for type, value, (line0, col0), (line1, col1), line_text in tokens:
            line0 += lineno - 1
            line1 += lineno - 1
            # Don't bother updating the columns; this is too complicated
            # since line_text would also have to be updated and it would
            # still break for tokens spanning lines.  Let the user guess
            # that the column numbers for doctests are relative to the
            # end of the prompt string (PS1 or PS2).
            yield type, value, (line0, col0), (line1, col1), line_text


    def gen_lines(self, block, indent):
        """Generates lines as expected by tokenize from a list of lines.

        This strips the first len(indent + self.PS1) characters off each line.
        """
        prefix1 = indent + self.PS1
        prefix2 = indent + self.PS2
        prefix = prefix1
        for line in block:
            if line.startswith(prefix):
                yield line[len(prefix):]
            elif line == prefix.rstrip() + "\n":
                yield "\n"
            else:
                raise AssertionError("line=%r, prefix=%r" % (line, prefix))
            prefix = prefix2
        while True:
            yield ""


def diff_texts(a, b, filename):
    """Return a unified diff of two strings."""
    a = a.splitlines()
    b = b.splitlines()
    return difflib.unified_diff(a, b, filename, filename,
                                "(original)", "(refactored)",
                                lineterm="")
