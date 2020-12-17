# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

from cryptography import utils


class ObjectIdentifier(object):
    def __init__(self, dotted_string):
        self._dotted_string = dotted_string

        nodes = self._dotted_string.split(".")
        intnodes = []

        # There must be at least 2 nodes, the first node must be 0..2, and
        # if less than 2, the second node cannot have a value outside the
        # range 0..39.  All nodes must be integers.
        for node in nodes:
            try:
                node_value = int(node, 10)
            except ValueError:
                raise ValueError(
                    "Malformed OID: %s (non-integer nodes)"
                    % (self._dotted_string)
                )
            if node_value < 0:
                raise ValueError(
                    "Malformed OID: %s (negative-integer nodes)"
                    % (self._dotted_string)
                )
            intnodes.append(node_value)

        if len(nodes) < 2:
            raise ValueError(
                "Malformed OID: %s (insufficient number of nodes)"
                % (self._dotted_string)
            )

        if intnodes[0] > 2:
            raise ValueError(
                "Malformed OID: %s (first node outside valid range)"
                % (self._dotted_string)
            )

        if intnodes[0] < 2 and intnodes[1] >= 40:
            raise ValueError(
                "Malformed OID: %s (second node outside valid range)"
                % (self._dotted_string)
            )

    def __eq__(self, other):
        if not isinstance(other, ObjectIdentifier):
            return NotImplemented

        return self.dotted_string == other.dotted_string

    def __ne__(self, other):
        return not self == other

    def __repr__(self):
        return "<ObjectIdentifier(oid={}, name={})>".format(
            self.dotted_string, self._name
        )

    def __hash__(self):
        return hash(self.dotted_string)

    @property
    def _name(self):
        # Lazy import to avoid an import cycle
        from cryptography.x509.oid import _OID_NAMES

        return _OID_NAMES.get(self, "Unknown OID")

    dotted_string = utils.read_only_property("_dotted_string")
