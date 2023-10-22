# Original Algorithm:
# By Steve Hanov, 2011. Released to the public domain.
# Please see http://stevehanov.ca/blog/index.php?id=115 for the accompanying article.
#
# Adapted for PyPy/CPython by Carl Friedrich Bolz-Tereick
#
# Based on Daciuk, Jan, et al. "Incremental construction of minimal acyclic finite-state automata."
# Computational linguistics 26.1 (2000): 3-16.
#
# Updated 2014 to use DAWG as a mapping; see
# Kowaltowski, T.; CL. Lucchesi (1993), "Applications of finite automata representing large vocabularies",
# Software-Practice and Experience 1993

from collections import defaultdict
from functools import cached_property


# This class represents a node in the directed acyclic word graph (DAWG). It
# has a list of edges to other nodes. It has functions for testing whether it
# is equivalent to another node. Nodes are equivalent if they have identical
# edges, and each identical edge leads to identical states. The __hash__ and
# __eq__ functions allow it to be used as a key in a python dictionary.


class DawgNode:

    def __init__(self, dawg):
        self.id = dawg.next_id
        dawg.next_id += 1
        self.final = False
        self.edges = {}

        self.linear_edges = None # later: list of (string, next_state)

    def __str__(self):
        if self.final:
            arr = ["1"]
        else:
            arr = ["0"]

        for (label, node) in sorted(self.edges.items()):
            arr.append(label)
            arr.append(str(node.id))

        return "_".join(arr)
    __repr__ = __str__

    def _as_tuple(self):
        edges = sorted(self.edges.items())
        edge_tuple = tuple((label, node.id) for label, node in edges)
        return (self.final, edge_tuple)

    def __hash__(self):
        return hash(self._as_tuple())

    def __eq__(self, other):
        return self._as_tuple() == other._as_tuple()

    @cached_property
    def num_reachable_linear(self):
        # returns the number of different paths to final nodes reachable from
        # this one

        count = 0
        # staying at self counts as a path if self is final
        if self.final:
            count += 1
        for label, node in self.linear_edges:
            count += node.num_reachable_linear

        return count


class Dawg:
    def __init__(self):
        self.previous_word = ""
        self.next_id = 0
        self.root = DawgNode(self)

        # Here is a list of nodes that have not been checked for duplication.
        self.unchecked_nodes = []

        # Here is a list of unique nodes that have been checked for
        # duplication.
        self.minimized_nodes = {}

        # Here is the data associated with all the nodes
        self.data = {}
        self.inverse = {}

    def insert(self, word, data):
        if not all(0 <= ord(c) < 128 for c in word):
            raise ValueError("Use 7-bit ASCII characters only")
        if word <= self.previous_word:
            raise ValueError("Error: Words must be inserted in alphabetical order.")
        if data in self.inverse:
            raise ValueError(f"data {data} is duplicate, got it for word {self.inverse[data]} and now {word}")

        # find common prefix between word and previous word
        common_prefix = 0
        for i in range(min(len(word), len(self.previous_word))):
            if word[i] != self.previous_word[i]:
                break
            common_prefix += 1

        # Check the unchecked_nodes for redundant nodes, proceeding from last
        # one down to the common prefix size. Then truncate the list at that
        # point.
        self._minimize(common_prefix)

        self.data[word] = data
        self.inverse[data] = word

        # add the suffix, starting from the correct node mid-way through the
        # graph
        if len(self.unchecked_nodes) == 0:
            node = self.root
        else:
            node = self.unchecked_nodes[-1][2]

        for letter in word[common_prefix:]:
            next_node = DawgNode(self)
            node.edges[letter] = next_node
            self.unchecked_nodes.append((node, letter, next_node))
            node = next_node

        node.final = True
        self.previous_word = word

    def finish(self):
        # minimize all unchecked_nodes
        self._minimize(0)

        self._linearize_edges()

        topoorder, linear_data, inverse = self._topological_order()
        return self.compute_packed(topoorder), linear_data, inverse

    def _minimize(self, down_to):
        # proceed from the leaf up to a certain point
        for i in range(len(self.unchecked_nodes) - 1, down_to - 1, -1):
            (parent, letter, child) = self.unchecked_nodes[i]
            if child in self.minimized_nodes:
                # replace the child with the previously encountered one
                parent.edges[letter] = self.minimized_nodes[child]
            else:
                # add the state to the minimized nodes.
                self.minimized_nodes[child] = child
            self.unchecked_nodes.pop()

    def _lookup(self, word):
        node = self.root
        skipped = 0  # keep track of number of final nodes that we skipped
        index = 0
        while index < len(word):
            for label, child in node.linear_edges:
                if word[index] == label[0]:
                    if word[index:index + len(label)] == label:
                        if node.final:
                            skipped += 1
                        index += len(label)
                        node = child
                        break
                    else:
                        return None
                skipped += child.num_reachable_linear
            else:
                return None
        return skipped

    def enum_all_nodes(self):
        stack = [self.root]
        done = set()
        while stack:
            node = stack.pop()
            if node.id in done:
                continue
            yield node
            done.add(node.id)
            for label, child in sorted(node.edges.items()):
                stack.append(child)

    def prettyprint(self):
        for node in sorted(self.enum_all_nodes(), key=lambda e: e.id):
            s_final = " final" if node.final else ""
            print(f"{node.id}: ({node}) {s_final}")
            for label, child in sorted(node.edges.items()):
                print(f"    {label} goto {child.id}")

    def _inverse_lookup(self, number):
        assert 0, "not working in the current form, but keep it as the pure python version of compact lookup"
        result = []
        node = self.root
        while 1:
            if node.final:
                if pos == 0:
                    return "".join(result)
                pos -= 1
            for label, child in sorted(node.edges.items()):
                nextpos = pos - child.num_reachable_linear
                if nextpos < 0:
                    result.append(label)
                    node = child
                    break
                else:
                    pos = nextpos
            else:
                assert 0

    def _linearize_edges(self):
        # compute "linear" edges. the idea is that long chains of edges without
        # any of the intermediate states being final or any extra incoming or
        # outgoing edges can be represented by having removing them, and
        # instead using longer strings as edge labels (instead of single
        # characters)
        incoming = defaultdict(list)
        nodes = sorted(self.enum_all_nodes(), key=lambda e: e.id)
        for node in nodes:
            for label, child in sorted(node.edges.items()):
                incoming[child].append(node)
        for node in nodes:
            node.linear_edges = []
            for label, child in sorted(node.edges.items()):
                s = [label]
                while len(child.edges) == 1 and len(incoming[child]) == 1 and not child.final:
                    (c, child), = child.edges.items()
                    s.append(c)
                node.linear_edges.append((''.join(s), child))

    def _topological_order(self):
        # compute reachable linear nodes, and the set of incoming edges for each node
        order = []
        stack = [self.root]
        seen = set()
        while stack:
            # depth first traversal
            node = stack.pop()
            if node.id in seen:
                continue
            seen.add(node.id)
            order.append(node)
            for label, child in node.linear_edges:
                stack.append(child)

        # do a (slightly bad) topological sort
        incoming = defaultdict(set)
        for node in order:
            for label, child in node.linear_edges:
                incoming[child].add((label, node))
        no_incoming = [order[0]]
        topoorder = []
        positions = {}
        while no_incoming:
            node = no_incoming.pop()
            topoorder.append(node)
            positions[node] = len(topoorder)
            # use "reversed" to make sure that the linear_edges get reorderd
            # from their alphabetical order as little as necessary (no_incoming
            # is LIFO)
            for label, child in reversed(node.linear_edges):
                incoming[child].discard((label, node))
                if not incoming[child]:
                    no_incoming.append(child)
                    del incoming[child]
        # check result
        assert set(topoorder) == set(order)
        assert len(set(topoorder)) == len(topoorder)

        for node in order:
            node.linear_edges.sort(key=lambda element: positions[element[1]])

        for node in order:
            for label, child in node.linear_edges:
                assert positions[child] > positions[node]
        # number the nodes. afterwards every input string in the set has a
        # unique number in the 0 <= number < len(data). We then put the data in
        # self.data into a linear list using these numbers as indexes.
        topoorder[0].num_reachable_linear
        linear_data = [None] * len(self.data)
        inverse = {} # maps value back to index
        for word, value in self.data.items():
            index = self._lookup(word)
            linear_data[index] = value
            inverse[value] = index

        return topoorder, linear_data, inverse

    def compute_packed(self, order):
        # assign offsets to every node
        for i, node in enumerate(order):
            # we don't know position of the edge yet, just use something big as
            # the starting position. we'll have to do further iterations anyway,
            # but the size is at least a lower limit then
            node.packed_offset = 2 ** 30 + i * 2 ** 10

        # due to the varint encoding of edge targets we need to run this to
        # fixpoint
        last_result = None
        while 1:
            result = bytearray()
            for node in order:
                offset = node.packed_offset = len(result)
                encode_varint_unsigned(
                    number_add_bits(node.num_reachable_linear, node.final),
                    result
                )
                if len(node.linear_edges) == 0:
                    assert node.final
                    encode_varint_unsigned(0, result) # add a 0 saying "done"
                prev_printed = len(result)
                prev_child_offset = len(result)
                for edgeindex, (label, targetnode) in enumerate(node.linear_edges):
                    label = label.encode('ascii')
                    child_offset = targetnode.packed_offset
                    child_offset_difference = child_offset - prev_child_offset

                    info = number_add_bits(child_offset_difference, len(label) == 1, edgeindex == len(node.linear_edges) - 1)
                    if edgeindex == 0:
                        assert info != 0
                    encode_varint_unsigned(info, result)
                    prev_child_offset = child_offset
                    if len(label) > 1:
                        result.append(len(label))
                    result.extend(label)
                    prev_printed = len(result)
                node.packed_size = len(result) - node.packed_offset
            if result == last_result:
                break
            last_result = result
        self.packed = result
        return bytes(result)


# ______________________________________________________________________
# the following functions operate on the packed representation

def number_add_bits(x, *bits):
    for bit in bits:
        assert bit == 0 or bit == 1
        x = (x << 1) | bit
    return x

def encode_varint_unsigned(i, res):
    # https://en.wikipedia.org/wiki/LEB128 unsigned variant
    more = True
    startlen = len(res)
    if i < 0:
        raise ValueError("only positive numbers supported", i)
    while more:
        lowest7bits = i & 0b1111111
        i >>= 7
        if i == 0:
            more = False
        else:
            lowest7bits |= 0b10000000
        res.append(lowest7bits)
    return len(res) - startlen

def number_split_bits(x, n, acc=()):
    if n == 1:
        return x >> 1, x & 1
    if n == 2:
        return x >> 2, (x >> 1) & 1, x & 1
    assert 0, "implement me!"

def decode_varint_unsigned(b, index=0):
    res = 0
    shift = 0
    while True:
        byte = b[index]
        res = res | ((byte & 0b1111111) << shift)
        index += 1
        shift += 7
        if not (byte & 0b10000000):
            return res, index

def decode_node(packed, node):
    x, node = decode_varint_unsigned(packed, node)
    node_count, final = number_split_bits(x, 1)
    return node_count, final, node

def decode_edge(packed, edgeindex, prev_child_offset, offset):
    x, offset = decode_varint_unsigned(packed, offset)
    if x == 0 and edgeindex == 0:
        raise KeyError # trying to decode past a final node
    child_offset_difference, len1, final_edge = number_split_bits(x, 2)
    child_offset = prev_child_offset + child_offset_difference
    if len1:
        size = 1
    else:
        size, offset = decode_varint_unsigned(packed, offset)
    return child_offset, final_edge, size, offset

def _match_edge(packed, s, size, node_offset, stringpos):
    if size > 1 and stringpos + size > len(s):
        # past the end of the string, can't match
        return False
    for i in range(size):
        if packed[node_offset + i] != s[stringpos + i]:
            # if a subsequent char of an edge doesn't match, the word isn't in
            # the dawg
            if i > 0:
                raise KeyError
            return False
    return True

def lookup(packed, data, s):
    return data[_lookup(packed, s)]

def _lookup(packed, s):
    stringpos = 0
    node_offset = 0
    skipped = 0  # keep track of number of final nodes that we skipped
    false = False
    while stringpos < len(s):
        #print(f"{node_offset=} {stringpos=}")
        _, final, edge_offset = decode_node(packed, node_offset)
        prev_child_offset = edge_offset
        edgeindex = 0
        while 1:
            child_offset, final_edge, size, edgelabel_chars_offset = decode_edge(packed, edgeindex, prev_child_offset, edge_offset)
            #print(f"    {edge_offset=} {child_offset=} {final_edge=} {size=} {edgelabel_chars_offset=}")
            edgeindex += 1
            prev_child_offset = child_offset
            if _match_edge(packed, s, size, edgelabel_chars_offset, stringpos):
                # match
                if final:
                    skipped += 1
                stringpos += size
                node_offset = child_offset
                break
            if final_edge:
                raise KeyError
            child_count, _, _ = decode_node(packed, child_offset)
            skipped += child_count
            edge_offset = edgelabel_chars_offset + size
    _, final, _ = decode_node(packed, node_offset)
    if final:
        return skipped
    raise KeyError

def inverse_lookup(packed, inverse, x):
    pos = inverse[x]
    return _inverse_lookup(packed, pos)

def _inverse_lookup(packed, pos):
    result = bytearray()
    node_offset = 0
    while 1:
        node_count, final, edge_offset = decode_node(packed, node_offset)
        if final:
            if pos == 0:
                return bytes(result)
            pos -= 1
        prev_child_offset = edge_offset
        edgeindex = 0
        while 1:
            child_offset, final_edge, size, edgelabel_chars_offset = decode_edge(packed, edgeindex, prev_child_offset, edge_offset)
            edgeindex += 1
            prev_child_offset = child_offset
            child_count, _, _ = decode_node(packed, child_offset)
            nextpos = pos - child_count
            if nextpos < 0:
                assert edgelabel_chars_offset >= 0
                result.extend(packed[edgelabel_chars_offset: edgelabel_chars_offset + size])
                node_offset = child_offset
                break
            elif not final_edge:
                pos = nextpos
                edge_offset = edgelabel_chars_offset + size
            else:
                raise KeyError
        else:
            raise KeyError


def build_compression_dawg(ucdata):
    d = Dawg()
    ucdata.sort()
    for name, value in ucdata:
        d.insert(name, value)
    packed, pos_to_code, reversedict = d.finish()
    print("size of dawg [KiB]", round(len(packed) / 1024, 2))
    # check that lookup and inverse_lookup work correctly on the input data
    for name, value in ucdata:
        assert lookup(packed, pos_to_code, name.encode('ascii')) == value
        assert inverse_lookup(packed, reversedict, value) == name.encode('ascii')
    return packed, pos_to_code
