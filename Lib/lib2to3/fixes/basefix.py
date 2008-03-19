# Copyright 2006 Google, Inc. All Rights Reserved.
# Licensed to PSF under a Contributor Agreement.

"""Base class for fixers (optional, but recommended)."""

# Python imports
import logging
import itertools

# Get a usable 'set' constructor
try:
    set
except NameError:
    from sets import Set as set

# Local imports
from ..patcomp import PatternCompiler
from .. import pygram

class BaseFix(object):

    """Optional base class for fixers.

    The subclass name must be FixFooBar where FooBar is the result of
    removing underscores and capitalizing the words of the fix name.
    For example, the class name for a fixer named 'has_key' should be
    FixHasKey.
    """

    PATTERN = None  # Most subclasses should override with a string literal
    pattern = None  # Compiled pattern, set by compile_pattern()
    options = None  # Options object passed to initializer
    filename = None # The filename (set by set_filename)
    logger = None   # A logger (set by set_filename)
    numbers = itertools.count(1) # For new_name()
    used_names = set() # A set of all used NAMEs
    order = "post" # Does the fixer prefer pre- or post-order traversal
    explicit = False # Is this ignored by refactor.py -f all?

    # Shortcut for access to Python grammar symbols
    syms = pygram.python_symbols

    def __init__(self, options, log):
        """Initializer.  Subclass may override.

        Args:
            options: an optparse.Values instance which can be used
                to inspect the command line options.
            log: a list to append warnings and other messages to.
        """
        self.options = options
        self.log = log
        self.compile_pattern()

    def compile_pattern(self):
        """Compiles self.PATTERN into self.pattern.

        Subclass may override if it doesn't want to use
        self.{pattern,PATTERN} in .match().
        """
        if self.PATTERN is not None:
            self.pattern = PatternCompiler().compile_pattern(self.PATTERN)

    def set_filename(self, filename):
        """Set the filename, and a logger derived from it.

        The main refactoring tool should call this.
        """
        self.filename = filename
        self.logger = logging.getLogger(filename)

    def match(self, node):
        """Returns match for a given parse tree node.

        Should return a true or false object (not necessarily a bool).
        It may return a non-empty dict of matching sub-nodes as
        returned by a matching pattern.

        Subclass may override.
        """
        results = {"node": node}
        return self.pattern.match(node, results) and results

    def transform(self, node, results):
        """Returns the transformation for a given parse tree node.

        Args:
          node: the root of the parse tree that matched the fixer.
          results: a dict mapping symbolic names to part of the match.

        Returns:
          None, or a node that is a modified copy of the
          argument node.  The node argument may also be modified in-place to
          effect the same change.

        Subclass *must* override.
        """
        raise NotImplementedError()

    def parenthesize(self, node):
        """Wrapper around pygram.parenthesize()."""
        return pygram.parenthesize(node)

    def new_name(self, template="xxx_todo_changeme"):
        """Return a string suitable for use as an identifier

        The new name is guaranteed not to conflict with other identifiers.
        """
        name = template
        while name in self.used_names:
            name = template + str(self.numbers.next())
        self.used_names.add(name)
        return name

    def log_message(self, message):
        if self.first_log:
            self.first_log = False
            self.log.append("### In file %s ###" % self.filename)
        self.log.append(message)

    def cannot_convert(self, node, reason=None):
        """Warn the user that a given chunk of code is not valid Python 3,
        but that it cannot be converted automatically.

        First argument is the top-level node for the code in question.
        Optional second argument is why it can't be converted.
        """
        lineno = node.get_lineno()
        for_output = node.clone()
        for_output.set_prefix("")
        msg = "Line %d: could not convert: %s"
        self.log_message(msg % (lineno, for_output))
        if reason:
            self.log_message(reason)

    def warning(self, node, reason):
        """Used for warning the user about possible uncertainty in the
        translation.

        First argument is the top-level node for the code in question.
        Optional second argument is why it can't be converted.
        """
        lineno = node.get_lineno()
        self.log_message("Line %d: %s" % (lineno, reason))

    def start_tree(self, tree, filename):
        """Some fixers need to maintain tree-wide state.
        This method is called once, at the start of tree fix-up.

        tree - the root node of the tree to be processed.
        filename - the name of the file the tree came from.
        """
        self.used_names = tree.used_names
        self.set_filename(filename)
        self.numbers = itertools.count(1)
        self.first_log = True

    def finish_tree(self, tree, filename):
        """Some fixers need to maintain tree-wide state.
        This method is called once, at the conclusion of tree fix-up.

        tree - the root node of the tree to be processed.
        filename - the name of the file the tree came from.
        """
        pass
