:mod:`graphlib` --- Functionality to operate with graph-like structures
=========================================================================

.. module:: graphlib
   :synopsis: Functionality to operate with graph-like structures


**Source code:** :source:`Lib/graphlib.py`

.. testsetup:: default

   import graphlib
   from graphlib import *

--------------


.. class:: TopologicalSorter(graph=None)

   Provides functionality to topologically sort a graph of hashable nodes.

   A topological order is a linear ordering of the nodes in a graph such that
   for every directed edge u -> v from node u to node v, node v comes
   before node u in the ordering. For instance, the nodes of the graph may
   represent tasks to be performed, and the edges may represent constraints that
   one task must be performed before another; in this example, a topological
   ordering is just a valid sequence for the tasks. A complete topological
   ordering is possible if and only if the graph is a directed acyclic graph,
   that is, if it has no circular dependencies among the tasks to be performed.

   If the optional *graph* argument is provided it must be a dictionary where
   the keys are nodes and the values are iterables of nodes that the key has
   edges to. The edges are directed from the keys to the values. Additional
   nodes can be added to the graph using the :meth:`~TopologicalSorter.add`
   method.

   In the general case, the steps required to perform the sorting of a given
   graph are as follows:

         * Create an instance of the :class:`TopologicalSorter` with an optional
           initial graph.
         * Add additional nodes to the graph.
         * Call :meth:`~TopologicalSorter.prepare` on the graph.
         * While :meth:`~TopologicalSorter.is_active` is ``True``, iterate over
           the nodes returned by :meth:`~TopologicalSorter.get_ready` and
           process them. Call :meth:`~TopologicalSorter.done` on each node as it
           finishes processing.

   In case just an immediate sorting of the nodes in the graph is required and
   no parallelism is involved, the convenience method
   :meth:`TopologicalSorter.static_order` can be used directly:

   .. doctest::

       >>> graph = {"D": {"B", "C"}, "C": {"A"}, "B": {"A"}}
       >>> ts = TopologicalSorter(graph)
       >>> tuple(ts.static_order())
       ('A', 'C', 'B', 'D')

   The class is designed to easily support parallel processing of the nodes as
   they become ready. For instance::

       topological_sorter = TopologicalSorter()

       # Add nodes to 'topological_sorter'...

       topological_sorter.prepare()
       while topological_sorter.is_active():
           for node in topological_sorter.get_ready():
               # Worker threads or processes take nodes to work on off the
               # 'task_queue' queue.
               task_queue.put(node)

           # When the work for a node is done, workers put the node in
           # 'finalized_tasks_queue' so we can get more nodes to work on.
           # The definition of 'is_active()' guarantees that, at this point, at
           # least one node has been placed on 'task_queue' that hasn't yet
           # been passed to 'done()', so this blocking 'get()' must (eventually)
           # succeed.  After calling 'done()', we loop back to call 'get_ready()'
           # again, so put newly freed nodes on 'task_queue' as soon as
           # logically possible.
           node = finalized_tasks_queue.get()
           topological_sorter.done(node)

   .. method:: add(node, *end_nodes)

      Add nodes and/or edges to the graph.

      All of the arguments are nodes and must be hashable.

      First: this adds new nodes to the graph for all of the arguments
      which were not already part of the graph.

      Second: this adds new edges from the *node* to each of the *end_nodes*.

      Raises :exc:`ValueError` if called after :meth:`~TopologicalSorter.prepare`.

   .. method:: prepare()

      Mark the graph as finished and check for cycles in the graph.

      After calling this method the graph cannot be modified using
      the :meth:`~TopologicalSorter.add` method.

      Raises :exc:`CycleError` if any cycles are detected,
      but :meth:`~TopologicalSorter.get_ready` can still be used to obtain as
      many nodes as possible until cycles block more progress.

   .. method:: is_active()

      Returns ``True`` if more progress can be made and ``False`` otherwise.
      Progress can be made if cycles do not block the resolution and either
      there are still nodes ready to be returned by
      :meth:`~TopologicalSorter.get_ready` or there are nodes which were
      returned by :meth:`~TopologicalSorter.get_ready` and have not yet been
      marked as :meth:`~TopologicalSorter.done`.

      The :meth:`~TopologicalSorter.__bool__` method of this class defers to
      this function, so instead of::

          if ts.is_active():
              ...

      it is possible to simply do::

          if ts:
              ...

      Raises :exc:`ValueError` if this method is called before
      :meth:`~TopologicalSorter.prepare` is called.

   .. method:: done(*nodes)

      Marks a set of nodes returned by :meth:`TopologicalSorter.get_ready` as
      processed, unblocking any successor of each node in *nodes* for being
      returned in the future by a call to :meth:`TopologicalSorter.get_ready`.

      Raises :exc:`ValueError` if: any node in *nodes* is not part of this
      graph, has not yet been returned by :meth:`~TopologicalSorter.get_ready`,
      has already been marked as processed by a previous call
      to :meth:`~TopologicalSorter.done`, or if this method is called without
      first calling :meth:`~TopologicalSorter.prepare`.

   .. method:: get_ready()

      Returns a ``tuple`` with all the nodes that are ready. Initially it
      returns all nodes with no outgoing edges, and once those are marked as
      processed by calling :meth:`TopologicalSorter.done`, further calls will
      return all new nodes that have all their outgoing edges already processed.
      Once no more progress can be made, empty tuples are returned.

      Raises :exc:`ValueError` if this method is called before
      :meth:`~TopologicalSorter.prepare` is called.

   .. method:: static_order()

      Returns an iterator object which will iterate over nodes in a topological
      order. When using this method, :meth:`~TopologicalSorter.prepare` and
      :meth:`~TopologicalSorter.done` should not be called. This method is
      equivalent to::

          def static_order(self):
              self.prepare()
              while self.is_active():
                  node_group = self.get_ready()
                  yield from node_group
                  self.done(*node_group)

      The particular order that is returned may depend on the specific order in
      which the items were inserted in the graph. For example:

      .. doctest::

          >>> ts = TopologicalSorter()
          >>> ts.add(3, 2, 1)
          >>> ts.add(1, 0)
          >>> print([*ts.static_order()])
          [2, 0, 1, 3]

          >>> ts2 = TopologicalSorter()
          >>> ts2.add(1, 0)
          >>> ts2.add(3, 2, 1)
          >>> print([*ts2.static_order()])
          [0, 2, 1, 3]

      This is due to the fact that "0" and "2" are in the same level in the
      graph (they would have been returned in the same call to
      :meth:`~TopologicalSorter.get_ready`) and the order between them is
      determined by the order of insertion.

      Raises :exc:`CycleError` if any cycles are detected.

   .. versionadded:: 3.9


Exceptions
----------
The :mod:`graphlib` module defines the following exception classes:

.. exception:: CycleError

   Subclass of :exc:`ValueError` raised by :meth:`TopologicalSorter.prepare` if
   cycles exist in the graph.

   The detected cycle can be accessed via the second element in
   the :attr:`~CycleError.args` attribute of the exception instance:
   ``cycle_error.args[1]``. It consists of a list of nodes where each node has
   an edge to the previous node in the list. The first and last nodes in the
   list are the same node and so it forms a complete cycle.

   If multiple cycles exist, only one of them will be detected and reported in
   the exception.
