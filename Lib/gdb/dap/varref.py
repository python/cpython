# Copyright 2023-2024 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

from abc import ABC, abstractmethod
from collections import defaultdict
from contextlib import contextmanager

import gdb

from .server import client_bool_capability
from .startup import DAPException, in_gdb_thread

# A list of all the variable references created during this pause.
all_variables = []


# When the inferior is re-started, we erase all variable references.
# See the section "Lifetime of Objects References" in the spec.
@in_gdb_thread
def clear_vars(event):
    global all_variables
    all_variables = []


gdb.events.cont.connect(clear_vars)


# A null context manager.  Python supplies one, starting in 3.7.
@contextmanager
def _null(**ignore):
    yield


@in_gdb_thread
def apply_format(value_format):
    """Temporarily apply the DAP ValueFormat.

    This returns a new context manager that applies the given DAP
    ValueFormat object globally, then restores gdb's state when finished."""
    if value_format is not None and "hex" in value_format and value_format["hex"]:
        return gdb.with_parameter("output-radix", 16)
    return _null()


class BaseReference(ABC):
    """Represent a variable or a scope.

    This class is just a base class, some methods must be implemented in
    subclasses.

    The 'ref' field can be used as the variablesReference in the protocol.
    """

    @in_gdb_thread
    def __init__(self, name):
        """Create a new variable reference with the given name.

        NAME is a string or None.  None means this does not have a
        name, e.g., the result of expression evaluation."""

        global all_variables
        all_variables.append(self)
        self.ref = len(all_variables)
        self.name = name
        self.reset_children()

    @in_gdb_thread
    def to_object(self):
        """Return a dictionary that describes this object for DAP.

        The resulting object is a starting point that can be filled in
        further.  See the Scope or Variable types in the spec"""
        result = {"variablesReference": self.ref if self.has_children() else 0}
        if self.name is not None:
            result["name"] = str(self.name)
        return result

    @abstractmethod
    def has_children(self):
        """Return True if this object has children."""
        return False

    def reset_children(self):
        """Reset any cached information about the children of this object."""
        # A list of all the children.  Each child is a BaseReference
        # of some kind.
        self.children = None
        # Map from the name of a child to a BaseReference.
        self.by_name = {}
        # Keep track of how many duplicates there are of a given name,
        # so that unique names can be generated.  Map from base name
        # to a count.
        self.name_counts = defaultdict(lambda: 1)

    @abstractmethod
    def fetch_one_child(self, index):
        """Fetch one child of this variable.

        INDEX is the index of the child to fetch.
        This should return a tuple of the form (NAME, VALUE), where
        NAME is the name of the variable, and VALUE is a gdb.Value."""
        return

    @abstractmethod
    def child_count(self):
        """Return the number of children of this variable."""
        return

    # Helper method to compute the final name for a child whose base
    # name is given.  Updates the name_counts map.  This is used to
    # handle shadowing -- in DAP, the adapter is responsible for
    # making sure that all the variables in a a given container have
    # unique names.  See
    # https://github.com/microsoft/debug-adapter-protocol/issues/141
    # and
    # https://github.com/microsoft/debug-adapter-protocol/issues/149
    def _compute_name(self, name):
        if name in self.by_name:
            self.name_counts[name] += 1
            # In theory there's no safe way to compute a name, because
            # a pretty-printer might already be generating names of
            # that form.  In practice I think we should not worry too
            # much.
            name = name + " #" + str(self.name_counts[name])
        return name

    @in_gdb_thread
    def fetch_children(self, start, count):
        """Fetch children of this variable.

        START is the starting index.
        COUNT is the number to return, with 0 meaning return all.
        Returns an iterable of some kind."""
        if count == 0:
            count = self.child_count()
        if self.children is None:
            self.children = [None] * self.child_count()
        for idx in range(start, start + count):
            if self.children[idx] is None:
                (name, value) = self.fetch_one_child(idx)
                name = self._compute_name(name)
                var = VariableReference(name, value)
                self.children[idx] = var
                self.by_name[name] = var
            yield self.children[idx]

    @in_gdb_thread
    def find_child_by_name(self, name):
        """Find a child of this variable, given its name.

        Returns the value of the child, or throws if not found."""
        # A lookup by name can only be done using names previously
        # provided to the client, so we can simply rely on the by-name
        # map here.
        if name in self.by_name:
            return self.by_name[name]
        raise DAPException("no variable named '" + name + "'")


class VariableReference(BaseReference):
    """Concrete subclass of BaseReference that handles gdb.Value."""

    def __init__(self, name, value, result_name="value"):
        """Initializer.

        NAME is the name of this reference, see superclass.
        VALUE is a gdb.Value that holds the value.
        RESULT_NAME can be used to change how the simple string result
        is emitted in the result dictionary."""
        super().__init__(name)
        self.result_name = result_name
        self.value = value
        self._update_value()

    # Internal method to update local data when the value changes.
    def _update_value(self):
        self.reset_children()
        self.printer = gdb.printing.make_visualizer(self.value)
        self.child_cache = None
        if self.has_children():
            self.count = -1
        else:
            self.count = None

    def assign(self, value):
        """Assign VALUE to this object and update."""
        self.value.assign(value)
        self._update_value()

    def has_children(self):
        return hasattr(self.printer, "children")

    def cache_children(self):
        if self.child_cache is None:
            # This discards all laziness.  This could be improved
            # slightly by lazily evaluating children, but because this
            # code also generally needs to know the number of
            # children, it probably wouldn't help much.  Note that
            # this is only needed with legacy (non-ValuePrinter)
            # printers.
            self.child_cache = list(self.printer.children())
        return self.child_cache

    def child_count(self):
        if self.count is None:
            return None
        if self.count == -1:
            num_children = None
            if isinstance(self.printer, gdb.ValuePrinter) and hasattr(
                self.printer, "num_children"
            ):
                num_children = self.printer.num_children()
            if num_children is None:
                num_children = len(self.cache_children())
            self.count = num_children
        return self.count

    def to_object(self):
        result = super().to_object()
        result[self.result_name] = str(self.printer.to_string())
        num_children = self.child_count()
        if num_children is not None:
            if (
                hasattr(self.printer, "display_hint")
                and self.printer.display_hint() == "array"
            ):
                result["indexedVariables"] = num_children
            else:
                result["namedVariables"] = num_children
        if client_bool_capability("supportsMemoryReferences"):
            # https://github.com/microsoft/debug-adapter-protocol/issues/414
            # changed DAP to allow memory references for any of the
            # variable response requests, and to lift the restriction
            # to pointer-to-function from Variable.
            if self.value.type.strip_typedefs().code == gdb.TYPE_CODE_PTR:
                result["memoryReference"] = hex(int(self.value))
        if client_bool_capability("supportsVariableType"):
            result["type"] = str(self.value.type)
        return result

    @in_gdb_thread
    def fetch_one_child(self, idx):
        if isinstance(self.printer, gdb.ValuePrinter) and hasattr(
            self.printer, "child"
        ):
            (name, val) = self.printer.child(idx)
        else:
            (name, val) = self.cache_children()[idx]
        # A pretty-printer can return something other than a
        # gdb.Value, but it must be convertible.
        if not isinstance(val, gdb.Value):
            val = gdb.Value(val)
        return (name, val)


@in_gdb_thread
def find_variable(ref):
    """Given a variable reference, return the corresponding variable object."""
    global all_variables
    # Variable references are offset by 1.
    ref = ref - 1
    if ref < 0 or ref > len(all_variables):
        raise DAPException("invalid variablesReference")
    return all_variables[ref]
