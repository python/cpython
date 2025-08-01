"""
Benchmark for b-tree workload.  This is intended to exercise the cyclic
garbage collector by presenting it with a large and interconnected
object graph.
"""

import gc
import random
import sys

# Total number of b-tree nodes to create.  We would like this to be
# large enough so that the working set of data doesn't fit into the CPU
# cache.  This benchmark is supposed to be similar to a real application
# that hold a large number of Python objects in RAM and does some
# processing on them.
NUM_NODES = 40_000

# Fraction of tree to re-create after initial creation.  Set to zero to
# disable re-creation.
RECREATE_FRACTION = 0.2

# Seed value for random generator
RANDOM_SEED = 0


class BNode:
    """
    Instance attributes:
      items: list
      nodes: [BNode]
    """

    __slots__ = ['items', 'nodes']

    minimum_degree = 16  # a.k.a. t

    def __init__(self):
        self.items = []
        self.nodes = None

    def is_leaf(self):
        return self.nodes is None

    def __iter__(self):
        if self.is_leaf():
            yield from self.items
        else:
            for position, item in enumerate(self.items):
                yield from self.nodes[position]
                yield item
            yield from self.nodes[-1]

    def is_full(self):
        return len(self.items) == 2 * self.minimum_degree - 1

    def get_position(self, key):
        for position, item in enumerate(self.items):
            if item[0] >= key:
                return position
        return len(self.items)

    def search(self, key):
        """(key:anything) -> None | (key:anything, value:anything)
        Return the matching pair, or None.
        """
        position = self.get_position(key)
        if position < len(self.items) and self.items[position][0] == key:
            return self.items[position]
        elif self.is_leaf():
            return None
        else:
            return self.nodes[position].search(key)

    def insert_item(self, item):
        """(item:(key:anything, value:anything))"""
        assert not self.is_full()
        key = item[0]
        position = self.get_position(key)
        if position < len(self.items) and self.items[position][0] == key:
            self.items[position] = item
        elif self.is_leaf():
            self.items.insert(position, item)
        else:
            child = self.nodes[position]
            if child.is_full():
                self.split_child(position, child)
                if key == self.items[position][0]:
                    self.items[position] = item
                else:
                    if key > self.items[position][0]:
                        position += 1
                    self.nodes[position].insert_item(item)
            else:
                self.nodes[position].insert_item(item)

    def split_child(self, position, child):
        """(position:int, child:BNode)"""
        assert not self.is_full()
        assert not self.is_leaf()
        assert self.nodes[position] is child
        assert child.is_full()
        bigger = self.__class__()
        middle = self.minimum_degree - 1
        splitting_key = child.items[middle]
        bigger.items = child.items[middle + 1 :]
        child.items = child.items[:middle]
        assert len(bigger.items) == len(child.items)
        if not child.is_leaf():
            bigger.nodes = child.nodes[middle + 1 :]
            child.nodes = child.nodes[: middle + 1]
            assert len(bigger.nodes) == len(child.nodes)
        self.items.insert(position, splitting_key)
        self.nodes.insert(position + 1, bigger)

    def get_count(self):
        """() -> int
        How many items are stored in this node and descendants?
        """
        result = len(self.items)
        for node in self.nodes or []:
            result += node.get_count()
        return result

    def get_level(self):
        """() -> int
        How many levels of nodes are there between this node
        and descendant leaf nodes?
        """
        if self.is_leaf():
            return 0
        else:
            return 1 + self.nodes[0].get_level()

    def delete(self, key):
        """(key:anything)
        Delete the item with this key.
        This is intended to follow the description in 19.3 of
        'Introduction to Algorithms' by Cormen, Lieserson, and Rivest.
        """

        def is_big(node):
            # Precondition for recursively calling node.delete(key).
            return node and len(node.items) >= node.minimum_degree

        p = self.get_position(key)
        matches = p < len(self.items) and self.items[p][0] == key
        if self.is_leaf():
            if matches:
                # Case 1.
                del self.items[p]
            else:
                raise KeyError(key)
        else:
            node = self.nodes[p]
            lower_sibling = p > 0 and self.nodes[p - 1]
            upper_sibling = p < len(self.nodes) - 1 and self.nodes[p + 1]
            if matches:
                # Case 2.
                if is_big(node):
                    # Case 2a.
                    extreme = node.get_max_item()
                    node.delete(extreme[0])
                    self.items[p] = extreme
                elif is_big(upper_sibling):
                    # Case 2b.
                    extreme = upper_sibling.get_min_item()
                    upper_sibling.delete(extreme[0])
                    self.items[p] = extreme
                else:
                    # Case 2c.
                    extreme = upper_sibling.get_min_item()
                    upper_sibling.delete(extreme[0])
                    node.items = node.items + [extreme] + upper_sibling.items
                    if not node.is_leaf():
                        node.nodes = node.nodes + upper_sibling.nodes
                    del self.items[p]
                    del self.nodes[p + 1]
            else:
                if not is_big(node):
                    if is_big(lower_sibling):
                        # Case 3a1: Shift an item from lower_sibling.
                        node.items.insert(0, self.items[p - 1])
                        self.items[p - 1] = lower_sibling.items[-1]
                        del lower_sibling.items[-1]
                        if not node.is_leaf():
                            node.nodes.insert(0, lower_sibling.nodes[-1])
                            del lower_sibling.nodes[-1]
                    elif is_big(upper_sibling):
                        # Case 3a2: Shift an item from upper_sibling.
                        node.items.append(self.items[p])
                        self.items[p] = upper_sibling.items[0]
                        del upper_sibling.items[0]
                        if not node.is_leaf():
                            node.nodes.append(upper_sibling.nodes[0])
                            del upper_sibling.nodes[0]
                    elif lower_sibling:
                        # Case 3b1: Merge with lower_sibling
                        node.items = (
                            lower_sibling.items
                            + [self.items[p - 1]]
                            + node.items
                        )
                        if not node.is_leaf():
                            node.nodes = lower_sibling.nodes + node.nodes
                        del self.items[p - 1]
                        del self.nodes[p - 1]
                    else:
                        # Case 3b2: Merge with upper_sibling
                        node.items = (
                            node.items + [self.items[p]] + upper_sibling.items
                        )
                        if not node.is_leaf():
                            node.nodes = node.nodes + upper_sibling.nodes
                        del self.items[p]
                        del self.nodes[p + 1]
                assert is_big(node)
                node.delete(key)
            if not self.items:
                # This can happen when self is the root node.
                self.items = self.nodes[0].items
                self.nodes = self.nodes[0].nodes


class BTree:
    """
    Instance attributes:
      root: BNode
    """

    __slots__ = ['root']

    def __init__(self):
        self.root = BNode()

    def __bool__(self):
        return bool(self.root.items)

    def iteritems(self):
        yield from self.root

    def iterkeys(self):
        for item in self.root:
            yield item[0]

    def __iter__(self):
        yield from self.iterkeys()

    def __contains__(self, key):
        return self.root.search(key) is not None

    def __setitem__(self, key, value):
        self.add(key, value)

    def __getitem__(self, key):
        item = self.root.search(key)
        if item is None:
            raise KeyError(key)
        return item[1]

    def __delitem__(self, key):
        self.root.delete(key)

    def get(self, key, default=None):
        """(key:anything, default:anything=None) -> anything"""
        try:
            return self[key]
        except KeyError:
            return default

    def add(self, key, value=True):
        """(key:anything, value:anything=True)
        Make self[key] == val.
        """
        if self.root.is_full():
            # replace and split.
            node = self.root.__class__()
            node.nodes = [self.root]
            node.split_child(0, node.nodes[0])
            self.root = node
        self.root.insert_item((key, value))

    def __len__(self):
        """() -> int
        Compute and return the total number of items."""
        return self.root.get_count()


class Record:
    def __init__(self, a, b, c, d, e, f):
        self.a = a
        self.b = b
        self.c = c
        self.d = d
        self.e = e
        self.f = f


def make_records(num_nodes):
    rnd = random.Random(RANDOM_SEED)
    for node_id in range(num_nodes):
        a = node_id
        b = f'node {node_id}'
        c = rnd.randbytes(node_id % 100)
        d = rnd.random()
        e = sys.intern(str(rnd.randint(0, 30)))
        f = rnd.choice([None, True, False])
        yield Record(a, b, c, d, e, f)


def make_tree(num_nodes, records):
    ids = list(range(num_nodes))
    # Create the tree with randomized key order.
    random.shuffle(ids)

    tree = BTree()
    for node_id in ids:
        tree[node_id] = records[node_id]

    if RECREATE_FRACTION > 0:
        # Re-create part of the tree.  This can cause objects in memory
        # to become more fragmented or shuffled since they are not allocated
        # in sequence.  Since we created nodes with keys in random order, we
        # can delete the lowest numbered ones and re-make those.
        remake_ids = range(int(num_nodes * RECREATE_FRACTION))
        for node_id in remake_ids:
            del tree[node_id]
        for node_id in remake_ids:
            tree[node_id] = records[node_id]

    return tree


def run_once(gc_only, records):
    obj = make_tree(NUM_NODES, records)

    gc.collect()

    if not gc_only:
        # Iterate over all nodes and add up the value of the 'd' attribute.
        d_total = 0.0
        for key in obj:
            node = obj[key]
            d_total += node.d

        # Lookup a random subset of nodes, add up value of 'd'
        num_lookup = max(200, NUM_NODES // 20)
        d_total = 0
        rnd = random.Random(RANDOM_SEED)
        for i in range(num_lookup):
            node_id = rnd.randint(0, NUM_NODES)
            node = obj.get(node_id)
            if node is not None:
                d_total += node.d


def run_bench(loops, gc_only):
    # Create the set of records outside the timed section.  In a real
    # application, the data would likely come from a file, a database or
    # from some other network service.  We don't want to benchmark the
    # 'random' module.
    records = list(make_records(NUM_NODES))
    total_time = 0
    for i in range(loops):
        random.seed(RANDOM_SEED)
        run_once(gc_only, records)
    return total_time


def run_pgo():
    run_bench(1, False)
