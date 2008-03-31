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
import optparse
import logging

# Local imports
from .pgen2 import driver
from .pgen2 import tokenize

from . import pytree
from . import patcomp
from . import fixes
from . import pygram

def main(args=None):
    """Main program.

    Call without arguments to use sys.argv[1:] as the arguments; or
    call with a list of arguments (excluding sys.argv[0]).

    Returns a suggested exit status (0, 1, 2).
    """
    # Set up option parser
    parser = optparse.OptionParser(usage="refactor.py [options] file|dir ...")
    parser.add_option("-d", "--doctests_only", action="store_true",
                      help="Fix up doctests only")
    parser.add_option("-f", "--fix", action="append", default=[],
                      help="Each FIX specifies a transformation; default all")
    parser.add_option("-l", "--list-fixes", action="store_true",
                      help="List available transformations (fixes/fix_*.py)")
    parser.add_option("-p", "--print-function", action="store_true",
                      help="Modify the grammar so that print() is a function")
    parser.add_option("-v", "--verbose", action="store_true",
                      help="More verbose logging")
    parser.add_option("-w", "--write", action="store_true",
                      help="Write back modified files")

    # Parse command line arguments
    options, args = parser.parse_args(args)
    if options.list_fixes:
        print("Available transformations for the -f/--fix option:")
        for fixname in get_all_fix_names():
            print(fixname)
        if not args:
            return 0
    if not args:
        print("At least one file or directory argument required.", file=sys.stderr)
        print("Use --help to show usage.", file=sys.stderr)
        return 2

    # Set up logging handler
    if sys.version_info < (2, 4):
        hdlr = logging.StreamHandler()
        fmt = logging.Formatter('%(name)s: %(message)s')
        hdlr.setFormatter(fmt)
        logging.root.addHandler(hdlr)
    else:
        logging.basicConfig(format='%(name)s: %(message)s', level=logging.INFO)

    # Initialize the refactoring tool
    rt = RefactoringTool(options)

    # Refactor all files and directories passed as arguments
    if not rt.errors:
        rt.refactor_args(args)
        rt.summarize()

    # Return error status (0 if rt.errors is zero)
    return int(bool(rt.errors))


def get_all_fix_names():
    """Return a sorted list of all available fix names."""
    fix_names = []
    names = os.listdir(os.path.dirname(fixes.__file__))
    names.sort()
    for name in names:
        if name.startswith("fix_") and name.endswith(".py"):
            fix_names.append(name[4:-3])
    fix_names.sort()
    return fix_names


class RefactoringTool(object):

    def __init__(self, options):
        """Initializer.

        The argument is an optparse.Values instance.
        """
        self.options = options
        self.errors = []
        self.logger = logging.getLogger("RefactoringTool")
        self.fixer_log = []
        if self.options.print_function:
            del pygram.python_grammar.keywords["print"]
        self.driver = driver.Driver(pygram.python_grammar,
                                    convert=pytree.convert,
                                    logger=self.logger)
        self.pre_order, self.post_order = self.get_fixers()
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
        fix_names = self.options.fix
        if not fix_names or "all" in fix_names:
            fix_names = get_all_fix_names()
        for fix_name in fix_names:
            try:
                mod = __import__("lib2to3.fixes.fix_" + fix_name, {}, {}, ["*"])
            except ImportError:
                self.log_error("Can't find transformation %s", fix_name)
                continue
            parts = fix_name.split("_")
            class_name = "Fix" + "".join([p.title() for p in parts])
            try:
                fix_class = getattr(mod, class_name)
            except AttributeError:
                self.log_error("Can't find fixes.fix_%s.%s",
                               fix_name, class_name)
                continue
            try:
                fixer = fix_class(self.options, self.fixer_log)
            except Exception as err:
                self.log_error("Can't instantiate fixes.fix_%s.%s()",
                               fix_name, class_name, exc_info=True)
                continue
            if fixer.explicit and fix_name not in self.options.fix:
                self.log_message("Skipping implicit fixer: %s", fix_name)
                continue

            if self.options.verbose:
                self.log_message("Adding transformation: %s", fix_name)
            if fixer.order == "pre":
                pre_order_fixers.append(fixer)
            elif fixer.order == "post":
                post_order_fixers.append(fixer)
            else:
                raise ValueError("Illegal fixer order: %r" % fixer.order)

        pre_order_fixers.sort(key=lambda x: x.run_order)
        post_order_fixers.sort(key=lambda x: x.run_order)
        return (pre_order_fixers, post_order_fixers)

    def log_error(self, msg, *args, **kwds):
        """Increments error count and log a message."""
        self.errors.append((msg, args, kwds))
        self.logger.error(msg, *args, **kwds)

    def log_message(self, msg, *args):
        """Hook to log a message."""
        if args:
            msg = msg % args
        self.logger.info(msg)

    def refactor_args(self, args):
        """Refactors files and directories from an argument list."""
        for arg in args:
            if arg == "-":
                self.refactor_stdin()
            elif os.path.isdir(arg):
                self.refactor_dir(arg)
            else:
                self.refactor_file(arg)

    def refactor_dir(self, arg):
        """Descends down a directory and refactor every Python file found.

        Python files are assumed to have a .py extension.

        Files and subdirectories starting with '.' are skipped.
        """
        for dirpath, dirnames, filenames in os.walk(arg):
            if self.options.verbose:
                self.log_message("Descending into %s", dirpath)
            dirnames.sort()
            filenames.sort()
            for name in filenames:
                if not name.startswith(".") and name.endswith("py"):
                    fullname = os.path.join(dirpath, name)
                    self.refactor_file(fullname)
            # Modify dirnames in-place to remove subdirs with leading dots
            dirnames[:] = [dn for dn in dirnames if not dn.startswith(".")]

    def refactor_file(self, filename):
        """Refactors a file."""
        try:
            f = open(filename)
        except IOError as err:
            self.log_error("Can't open %s: %s", filename, err)
            return
        try:
            input = f.read() + "\n" # Silence certain parse errors
        finally:
            f.close()
        if self.options.doctests_only:
            if self.options.verbose:
                self.log_message("Refactoring doctests in %s", filename)
            output = self.refactor_docstring(input, filename)
            if output != input:
                self.write_file(output, filename, input)
            elif self.options.verbose:
                self.log_message("No doctest changes in %s", filename)
        else:
            tree = self.refactor_string(input, filename)
            if tree and tree.was_changed:
                # The [:-1] is to take off the \n we added earlier
                self.write_file(str(tree)[:-1], filename)
            elif self.options.verbose:
                self.log_message("No changes in %s", filename)

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
            tree = self.driver.parse_string(data,1)
        except Exception as err:
            self.log_error("Can't parse %s: %s: %s",
                           name, err.__class__.__name__, err)
            return
        if self.options.verbose:
            self.log_message("Refactoring %s", name)
        self.refactor_tree(tree, name)
        return tree

    def refactor_stdin(self):
        if self.options.write:
            self.log_error("Can't write changes back to stdin")
            return
        input = sys.stdin.read()
        if self.options.doctests_only:
            if self.options.verbose:
                self.log_message("Refactoring doctests in stdin")
            output = self.refactor_docstring(input, "<stdin>")
            if output != input:
                self.write_file(output, "<stdin>", input)
            elif self.options.verbose:
                self.log_message("No doctest changes in stdin")
        else:
            tree = self.refactor_string(input, "<stdin>")
            if tree and tree.was_changed:
                self.write_file(str(tree), "<stdin>", input)
            elif self.options.verbose:
                self.log_message("No changes in stdin")

    def refactor_tree(self, tree, name):
        """Refactors a parse tree (modifying the tree in place).

        Args:
            tree: a pytree.Node instance representing the root of the tree
                  to be refactored.
            name: a human-readable name for this tree.

        Returns:
            True if the tree was modified, False otherwise.
        """
        all_fixers = self.pre_order + self.post_order
        for fixer in all_fixers:
            fixer.start_tree(tree, name)

        self.traverse_by(self.pre_order, tree.pre_order())
        self.traverse_by(self.post_order, tree.post_order())

        for fixer in all_fixers:
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
            for fixer in fixers:
                results = fixer.match(node)
                if results:
                    new = fixer.transform(node, results)
                    if new is not None and (new != node or
                                            str(new) != str(node)):
                        node.replace(new)
                        node = new

    def write_file(self, new_text, filename, old_text=None):
        """Writes a string to a file.

        If there are no changes, this is a no-op.

        Otherwise, it first shows a unified diff between the old text
        and the new text, and then rewrites the file; the latter is
        only done if the write option is set.
        """
        self.files.append(filename)
        if old_text is None:
            try:
                f = open(filename, "r")
            except IOError as err:
                self.log_error("Can't read %s: %s", filename, err)
                return
            try:
                old_text = f.read()
            finally:
                f.close()
        if old_text == new_text:
            if self.options.verbose:
                self.log_message("No changes to %s", filename)
            return
        diff_texts(old_text, new_text, filename)
        if not self.options.write:
            if self.options.verbose:
                self.log_message("Not writing changes to %s", filename)
            return
        backup = filename + ".bak"
        if os.path.lexists(backup):
            try:
                os.remove(backup)
            except os.error as err:
                self.log_message("Can't remove backup %s", backup)
        try:
            os.rename(filename, backup)
        except os.error as err:
            self.log_message("Can't rename %s to %s", filename, backup)
        try:
            f = open(filename, "w")
        except os.error as err:
            self.log_error("Can't create %s: %s", filename, err)
            return
        try:
            try:
                f.write(new_text)
            except os.error as err:
                self.log_error("Can't write %s: %s", filename, err)
        finally:
            f.close()
        if self.options.verbose:
            self.log_message("Wrote changes to %s", filename)

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
        except Exception as err:
            if self.options.verbose:
                for line in block:
                    self.log_message("Source: %s", line.rstrip("\n"))
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
        if self.options.write:
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
        tokens = tokenize.generate_tokens(self.gen_lines(block, indent).__next__)
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
    """Prints a unified diff of two strings."""
    a = a.splitlines()
    b = b.splitlines()
    for line in difflib.unified_diff(a, b, filename, filename,
                                     "(original)", "(refactored)",
                                     lineterm=""):
        print(line)


if __name__ == "__main__":
    sys.exit(main())
