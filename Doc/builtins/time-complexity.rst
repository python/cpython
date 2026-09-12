.. _time-complexity:

===============================================
Time complexity of operations on built-in types
===============================================

This page documents the time complexity of various operations on built-in types
in CPython. Other Python implementations may have different performance
characteristics. Additionally, the listed costs assume exact built-in types, as
instances of subclasses may have different costs.

We use |big O notation|_ to describe how the running time of an operation grows
with the size of its inputs. Unless stated otherwise, *n* denotes the number of
elements currently in the container, and *k* is the value of a numeric
parameter, such as an index or a repeat count.

.. |big O notation| replace:: Big *O* notation
.. _big O notation: https://en.wikipedia.org/wiki/Big_O_notation


:class:`!list`
==============

Lists are mutable sequences; for more detail on the implementation see
:ref:`how-are-lists-implemented`. The largest costs come from growing beyond the
current allocation size (because everything must move), or from inserting or
deleting somewhere near the beginning (because everything after that must move).
If you need to add or remove at both ends, consider using a
:class:`collections.deque` instead.

.. list-table::
   :header-rows: 1

   * - Operation
     - Complexity
   * - Copy (``l.copy()``)
     - *O*\ (*n*)
   * - Append (``l.append(x)``) [1]_
     - *O*\ (1)
   * - Pop (``l.pop(k)``) [1]_ [2]_
     - *O*\ (*n* - *k*)
   * - Insert (``l.insert(k, x)``) [1]_ [2]_
     - *O*\ (*n* - *k*)
   * - Get item (``l[k]``)
     - *O*\ (1)
   * - Set item (``l[k] = x``)
     - *O*\ (1)
   * - Delete item (``del l[k]``) [2]_
     - *O*\ (*n* - *k*)
   * - Iteration
     - *O*\ (*n*)
   * - Get slice (``l[i:j]``)
     - *O*\ (*j* - *i*)
   * - Set slice (``l[i:j] = t``) [1]_
     - *O*\ (*j* - *i*) if len(*t*) == *j* - *i*,
       otherwise *O*\ (*n* - *i* + len(*t*))
   * - Delete slice (``del l[i:j]``)
     - *O*\ (*n* - *i*)
   * - Extend (``l.extend(t)``) [1]_ [3]_
     - *O*\ (len(*t*))
   * - Sort (``l.sort()``) [4]_
     - *O*\ (*n* log *n*)
   * - Concatenate (``l1 + l2``)
     - *O*\ (len(*l1*) + len(*l2*))
   * - Multiply (``l * k``)
     - *O*\ (*nk*)
   * - ``x in l``
     - *O*\ (*n*)
   * - ``min(l)``, ``max(l)``
     - *O*\ (*n*)
   * - Get length (``len(l)``) [5]_
     - *O*\ (1)


:class:`!tuple`
===============

A :class:`tuple` is an :term:`immutable` sequence. Because a tuple can never
change, there are no insertion or deletion costs, and making a copy simply
returns the same object, so is constant time (*O*\ (1)).

.. list-table::
   :header-rows: 1

   * - Operation
     - Complexity
   * - Copy (``tuple(t)``)
     - *O*\ (1)
   * - Get item (``t[k]``)
     - *O*\ (1)
   * - Get slice (``t[i:j]``)
     - *O*\ (*j* - *i*)
   * - Concatenate (``t1 + t2``)
     - *O*\ (len(*t1*) + len(*t2*))
   * - Multiply (``t * k``)
     - *O*\ (*nk*)
   * - Iteration
     - *O*\ (*n*)
   * - ``x in t``
     - *O*\ (*n*)
   * - ``min(t)``, ``max(t)``
     - *O*\ (*n*)
   * - Get length (``len(t)``) [5]_
     - *O*\ (1)


:class:`!dict`, :class:`!frozendict`
====================================

The times listed for dict objects are average-case times, as they assume the
hash function for the objects is sufficiently robust to make collisions
uncommon. They also assume the keys are well-distributed among the set of
possible keys. In the worst case, when every key hashes to the same value,
each of the *O*\ (1) operations below instead takes *O*\ (*n*) time. They also
assume that hashing and comparing a key is *O*\ (1). For more detail on the
implementation, see :ref:`how-are-dictionaries-implemented`.

A :class:`frozendict` is immutable, so it does not support setting, deleting,
or updating items. The other operations below apply to it at the same costs.

.. list-table::
   :header-rows: 1

   * - Operation
     - Complexity
   * - ``key in d``
     - *O*\ (1)
   * - Copy (``d.copy()``) [6]_ [7]_
     - *O*\ (*n*)
   * - Get item (``d[key]``, ``d.get(key)``)
     - *O*\ (1)
   * - Set item (``d[key] = value``) [1]_
     - *O*\ (1)
   * - Delete item (``del d[key]``, ``d.pop(key)``)
     - *O*\ (1)
   * - Update (``d.update(t)``, ``d |= t``) [1]_ [3]_ [7]_
     - *O*\ (len(*t*))
   * - Iteration [7]_
     - *O*\ (*n*)
   * - Get length (``len(d)``) [5]_
     - *O*\ (1)


:class:`!set`, :class:`!frozenset`
==================================

See :class:`dict` as the :class:`set` and :class:`frozenset` implementations are
similar, and the same caveats apply.
In the worst case, *O*\ (1) operations instead take *O*\ (*n*) time,
and operations that look up every element degrade accordingly.

A :class:`frozenset` is :term:`immutable`, so it does not support adding,
discarding, or the in-place update operations. The others below apply to it at
the same costs.

.. list-table::
   :header-rows: 1

   * - Operation
     - Complexity
   * - ``x in s``
     - *O*\ (1)
   * - Copy (``s.copy()``) [6]_ [7]_
     - *O*\ (*n*)
   * - Add (``s.add(x)``) [1]_
     - *O*\ (1)
   * - Discard (``s.discard(x)``, ``s.remove(x)``)
     - *O*\ (1)
   * - Union (``s1 | s2``, ``s1.union(s2)``) [7]_
     - *O*\ (len(*s1*) + len(*s2*))
   * - Update (``s1 |= s2``, ``s1.update(s2)``) [1]_ [7]_
     - *O*\ (len(*s2*))
   * - Intersection (``s1 & s2``, ``s1.intersection(s2)``) [7]_ [8]_
     - *O*\ (min(len(*s1*), len(*s2*)))
   * - Intersection update (``s1 &= s2``, ``s1.intersection_update(s2)``) [1]_ [7]_ [8]_
     - *O*\ (min(len(*s1*), len(*s2*)))
   * - Difference (``s1 - s2``, ``s1.difference(s2)``) [7]_ [9]_
     - *O*\ (len(*s1*))
   * - Difference update (``s1 -= s2``, ``s1.difference_update(s2)``) [1]_ [7]_ [8]_
     - *O*\ (min(len(*s1*), len(*s2*)))
   * - Symmetric difference (``s1 ^ s2``, ``s1.symmetric_difference(s2)``) [7]_
     - *O*\ (len(*s1*) + len(*s2*))
   * - Symmetric difference update (``s1 ^= s2``, ``s1.symmetric_difference_update(s2)``) [1]_ [7]_
     - *O*\ (len(*s2*))
   * - Get length (``len(s)``) [5]_
     - *O*\ (1)


:class:`!str`, :class:`!bytes`, :class:`!bytearray`
===================================================

:class:`str` and :class:`bytes` objects are immutable sequences of characters and
bytes, respectively. As with tuples, copying one returns the original object.
A :class:`bytearray` is mutable, and additionally supports the mutating operations
of :class:`list` (except :meth:`!sort`), at the same costs. However, deleting at
the front with ``del`` (``del b[0]``, ``del b[:k]``) only advances the start of
the buffer instead of moving the remaining bytes, and is amortized *O*\ (1).

.. list-table::
   :header-rows: 1

   * - Operation
     - Complexity
   * - Get item (``s[k]``)
     - *O*\ (1)
   * - Get slice (``s[i:j]``)
     - *O*\ (*j* - *i*)
   * - Concatenate (``s + t``) [10]_
     - *O*\ (len(*s*) + len(*t*))
   * - Multiply (``s * k``)
     - *O*\ (*nk*)
   * - Substring search (``x in s``, ``s.find(x)``, ``s.index(x)``) [11]_
     - *O*\ (*n*)
   * - Reverse substring search (``s.rfind(x)``, ``s.rindex(x)``) [11]_ [12]_
     - *O*\ (*n* × len(*x*))
   * - Encode or decode [13]_
     - *O*\ (*n*)
   * - Iteration
     - *O*\ (*n*)
   * - Get length (``len(s)``) [5]_
     - *O*\ (1)


:class:`!memoryview`
====================

:class:`memoryview` objects allow Python code to access the internal data
of an object that supports the :ref:`buffer protocol <bufferobjects>` without
copying. In particular, slicing a memory view returns a new view onto the same
buffer.

.. list-table::
   :header-rows: 1

   * - Operation
     - Complexity
   * - Create (``memoryview(obj)``)
     - *O*\ (1)
   * - Get item (``v[k]``)
     - *O*\ (1)
   * - Get slice (``v[i:j]``)
     - *O*\ (1)
   * - Index (``v.index(x)``) [11]_ [14]_
     - *O*\ (*n*)
   * - Count (``v.count(x)``) [14]_
     - *O*\ (*n*)
   * - Convert to bytes (``v.tobytes()``, ``bytes(v)``)
     - *O*\ (*n*)
   * - Get length (``len(v)``) [5]_
     - *O*\ (1)


:class:`!range`
===============

A :class:`range` object computes its items on demand from its *start*, *stop* and
*step* values, so most operations do not depend on the length of the range.

.. list-table::
   :header-rows: 1

   * - Operation
     - Complexity
   * - Get item (``r[k]``)
     - *O*\ (1)
   * - Get slice (``r[i:j]``)
     - *O*\ (1)
   * - ``x in r`` [15]_
     - *O*\ (1)
   * - Index and count (``r.index(x)``, ``r.count(x)``) [15]_
     - *O*\ (1)
   * - Iteration
     - *O*\ (*n*)
   * - ``min(r)``, ``max(r)``
     - *O*\ (*n*)
   * - Get length (``len(r)``) [5]_
     - *O*\ (1)


Notes
=====

.. [1] Amortized. An individual operation may occasionally be *O*\ (*n*)
   when the underlying storage is resized, but this cost is spread over
   many operations, depending on the history of the container.

.. [2] Popping or deleting the element at index *k* of a list of size *n*
   shifts all elements after *k* one slot to the left, moving *n* - *k* - 1
   elements; inserting at index *k* shifts the elements from *k* onwards one
   slot to the right, moving *n* - *k* elements. The worst case is index 0,
   where the whole rest of the list has to be moved; the average case, an
   index in the middle of the list, takes *O*\ (*n*/2) = *O*\ (*n*)
   operations; and operating at the end of the list moves nothing and is
   *O*\ (1).

.. [3] Plus the cost of iterating over *t*, which may be expensive for an
   arbitrary iterable.

.. [4] This is the worst case scenario. Sorting is adaptive and input that is
   already sorted or reverse-sorted takes only *O*\ (*n*) comparisons.
   See :source:`Objects/listsort.txt` for more information.

.. [5] The number of elements is stored in the object, so ``len()`` does
   not need to count them.

.. [6] Copying a :class:`frozendict` or a :class:`frozenset` is *O*\ (1) as it
   returns the original object.

.. [7] These operations scan the container's internal hash table, which is
   not shrunk when elements are removed. After removing most elements, they
   still take time proportional to the container's former size, until a
   later insertion triggers a resize.

.. [8] *O*\ (len(*t*)) if *t* is not a set.

.. [9] *O*\ (len(*s*) + len(*t*)) if *t* is not a set.

.. [10] Each concatenation builds a new object, so building a string by
   concatenating many pieces in a loop is quadratic in the total length.
   See the :ref:`note on concatenating immutable sequences
   <typesseq-repeated-concatenation>` for alternatives.

.. [11] With *start* and *end* arguments, *n* is the length of the region
   searched rather than of *s*, and unlike slicing nothing is copied.

.. [12] This is the worst case. Reverse searches are *O*\ (*n*) on typical
   input. Forward searches instead use a more elaborate algorithm with a
   linear worst case, described in
   :source:`Objects/stringlib/stringlib_find_two_way_notes.txt`.

.. [13] This assumes a codec that does a constant amount of work per character.

.. [14] These unpack and compare each element individually, so they are much
   slower than the equivalent :class:`bytes` methods.

.. [15] Assuming :class:`int` or :class:`bool` arguments. For other types,
   the range is searched like any other sequence in *O*\ (*n*) time.
