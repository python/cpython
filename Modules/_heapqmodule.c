/* Drop in replacement for heapq.py

C implementation derived directly from heapq.py in Py2.3
which was written by Kevin O'Connor, augmented by Tim Peters,
annotated by François Pinard, and converted to C by Raymond Hettinger.

*/

#include "Python.h"

static int
siftdown(PyListObject *heap, Py_ssize_t startpos, Py_ssize_t pos)
{
    PyObject *newitem, *parent, **arr;
    Py_ssize_t parentpos, size;
    int cmp;

    assert(PyList_Check(heap));
    size = PyList_GET_SIZE(heap);
    if (pos >= size) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return -1;
    }

    /* Follow the path to the root, moving parents down until finding
       a place newitem fits. */
    arr = _PyList_ITEMS(heap);
    newitem = arr[pos];
    while (pos > startpos) {
        parentpos = (pos - 1) >> 1;
        parent = arr[parentpos];
        cmp = PyObject_RichCompareBool(newitem, parent, Py_LT);
        if (cmp < 0)
            return -1;
        if (size != PyList_GET_SIZE(heap)) {
            PyErr_SetString(PyExc_RuntimeError,
                            "list changed size during iteration");
            return -1;
        }
        if (cmp == 0)
            break;
        arr = _PyList_ITEMS(heap);
        parent = arr[parentpos];
        newitem = arr[pos];
        arr[parentpos] = newitem;
        arr[pos] = parent;
        pos = parentpos;
    }
    return 0;
}

static int
siftup(PyListObject *heap, Py_ssize_t pos)
{
    Py_ssize_t startpos, endpos, childpos, limit;
    PyObject *tmp1, *tmp2, **arr;
    int cmp;

    assert(PyList_Check(heap));
    endpos = PyList_GET_SIZE(heap);
    startpos = pos;
    if (pos >= endpos) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return -1;
    }

    /* Bubble up the smaller child until hitting a leaf. */
    arr = _PyList_ITEMS(heap);
    limit = endpos >> 1;         /* smallest pos that has no child */
    while (pos < limit) {
        /* Set childpos to index of smaller child.   */
        childpos = 2*pos + 1;    /* leftmost child position  */
        if (childpos + 1 < endpos) {
            cmp = PyObject_RichCompareBool(
                arr[childpos],
                arr[childpos + 1],
                Py_LT);
            if (cmp < 0)
                return -1;
            childpos += ((unsigned)cmp ^ 1);   /* increment when cmp==0 */
            arr = _PyList_ITEMS(heap);         /* arr may have changed */
            if (endpos != PyList_GET_SIZE(heap)) {
                PyErr_SetString(PyExc_RuntimeError,
                                "list changed size during iteration");
                return -1;
            }
        }
        /* Move the smaller child up. */
        tmp1 = arr[childpos];
        tmp2 = arr[pos];
        arr[childpos] = tmp2;
        arr[pos] = tmp1;
        pos = childpos;
    }
    /* Bubble it up to its final resting place (by sifting its parents down). */
    return siftdown(heap, startpos, pos);
}

static PyObject *
heappush(PyObject *self, PyObject *args)
{
    PyObject *heap, *item;

    if (!PyArg_UnpackTuple(args, "heappush", 2, 2, &heap, &item))
        return NULL;

    if (!PyList_Check(heap)) {
        PyErr_SetString(PyExc_TypeError, "heap argument must be a list");
        return NULL;
    }

    if (PyList_Append(heap, item))
        return NULL;

    if (siftdown((PyListObject *)heap, 0, PyList_GET_SIZE(heap)-1))
        return NULL;
    Py_RETURN_NONE;
}

PyDoc_STRVAR(heappush_doc,
"heappush(heap, item) -> None. Push item onto heap, maintaining the heap invariant.");

static PyObject *
heappop_internal(PyObject *heap, int siftup_func(PyListObject *, Py_ssize_t))
{
    PyObject *lastelt, *returnitem;
    Py_ssize_t n;

    if (!PyList_Check(heap)) {
        PyErr_SetString(PyExc_TypeError, "heap argument must be a list");
        return NULL;
    }

    /* raises IndexError if the heap is empty */
    n = PyList_GET_SIZE(heap);
    if (n == 0) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return NULL;
    }

    lastelt = PyList_GET_ITEM(heap, n-1) ;
    Py_INCREF(lastelt);
    if (PyList_SetSlice(heap, n-1, n, NULL)) {
        Py_DECREF(lastelt);
        return NULL;
    }
    n--;

    if (!n)
        return lastelt;
    returnitem = PyList_GET_ITEM(heap, 0);
    PyList_SET_ITEM(heap, 0, lastelt);
    if (siftup_func((PyListObject *)heap, 0)) {
        Py_DECREF(returnitem);
        return NULL;
    }
    return returnitem;
}

static PyObject *
heappop(PyObject *self, PyObject *heap)
{
    return heappop_internal(heap, siftup);
}

PyDoc_STRVAR(heappop_doc,
"Pop the smallest item off the heap, maintaining the heap invariant.");

static PyObject *
heapreplace_internal(PyObject *args, int siftup_func(PyListObject *, Py_ssize_t))
{
    PyObject *heap, *item, *returnitem;

    if (!PyArg_UnpackTuple(args, "heapreplace", 2, 2, &heap, &item))
        return NULL;

    if (!PyList_Check(heap)) {
        PyErr_SetString(PyExc_TypeError, "heap argument must be a list");
        return NULL;
    }

    if (PyList_GET_SIZE(heap) == 0) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return NULL;
    }

    returnitem = PyList_GET_ITEM(heap, 0);
    Py_INCREF(item);
    PyList_SET_ITEM(heap, 0, item);
    if (siftup_func((PyListObject *)heap, 0)) {
        Py_DECREF(returnitem);
        return NULL;
    }
    return returnitem;
}

static PyObject *
heapreplace(PyObject *self, PyObject *args)
{
    return heapreplace_internal(args, siftup);
}

PyDoc_STRVAR(heapreplace_doc,
"heapreplace(heap, item) -> value. Pop and return the current smallest value, and add the new item.\n\
\n\
This is more efficient than heappop() followed by heappush(), and can be\n\
more appropriate when using a fixed-size heap.  Note that the value\n\
returned may be larger than item!  That constrains reasonable uses of\n\
this routine unless written as part of a conditional replacement:\n\n\
    if item > heap[0]:\n\
        item = heapreplace(heap, item)\n");

static PyObject *
heappushpop(PyObject *self, PyObject *args)
{
    PyObject *heap, *item, *returnitem;
    int cmp;

    if (!PyArg_UnpackTuple(args, "heappushpop", 2, 2, &heap, &item))
        return NULL;

    if (!PyList_Check(heap)) {
        PyErr_SetString(PyExc_TypeError, "heap argument must be a list");
        return NULL;
    }

    if (PyList_GET_SIZE(heap) == 0) {
        Py_INCREF(item);
        return item;
    }

    cmp = PyObject_RichCompareBool(PyList_GET_ITEM(heap, 0), item, Py_LT);
    if (cmp < 0)
        return NULL;
    if (cmp == 0) {
        Py_INCREF(item);
        return item;
    }

    if (PyList_GET_SIZE(heap) == 0) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return NULL;
    }

    returnitem = PyList_GET_ITEM(heap, 0);
    Py_INCREF(item);
    PyList_SET_ITEM(heap, 0, item);
    if (siftup((PyListObject *)heap, 0)) {
        Py_DECREF(returnitem);
        return NULL;
    }
    return returnitem;
}

PyDoc_STRVAR(heappushpop_doc,
"heappushpop(heap, item) -> value. Push item on the heap, then pop and return the smallest item\n\
from the heap. The combined action runs more efficiently than\n\
heappush() followed by a separate call to heappop().");

static Py_ssize_t
keep_top_bit(Py_ssize_t n)
{
    int i = 0;

    while (n > 1) {
        n >>= 1;
        i++;
    }
    return n << i;
}

/* Cache friendly version of heapify()
   -----------------------------------

   Build-up a heap in O(n) time by performing siftup() operations
   on nodes whose children are already heaps.

   The simplest way is to sift the nodes in reverse order from
   n//2-1 to 0 inclusive.  The downside is that children may be
   out of cache by the time their parent is reached.

   A better way is to not wait for the children to go out of cache.
   Once a sibling pair of child nodes have been sifted, immediately
   sift their parent node (while the children are still in cache).

   Both ways build child heaps before their parents, so both ways
   do the exact same number of comparisons and produce exactly
   the same heap.  The only difference is that the traversal
   order is optimized for cache efficiency.
*/

static PyObject *
cache_friendly_heapify(PyObject *heap, int siftup_func(PyListObject *, Py_ssize_t))
{
    Py_ssize_t i, j, m, mhalf, leftmost;

    m = PyList_GET_SIZE(heap) >> 1;         /* index of first childless node */
    leftmost = keep_top_bit(m + 1) - 1;     /* leftmost node in row of m */
    mhalf = m >> 1;                         /* parent of first childless node */

    for (i = leftmost - 1 ; i >= mhalf ; i--) {
        j = i;
        while (1) {
            if (siftup_func((PyListObject *)heap, j))
                return NULL;
            if (!(j & 1))
                break;
            j >>= 1;
        }
    }

    for (i = m - 1 ; i >= leftmost ; i--) {
        j = i;
        while (1) {
            if (siftup_func((PyListObject *)heap, j))
                return NULL;
            if (!(j & 1))
                break;
            j >>= 1;
        }
    }
    Py_RETURN_NONE;
}

static PyObject *
heapify_internal(PyObject *heap, int siftup_func(PyListObject *, Py_ssize_t))
{
    Py_ssize_t i, n;

    if (!PyList_Check(heap)) {
        PyErr_SetString(PyExc_TypeError, "heap argument must be a list");
        return NULL;
    }

    /* For heaps likely to be bigger than L1 cache, we use the cache
       friendly heapify function.  For smaller heaps that fit entirely
       in cache, we prefer the simpler algorithm with less branching.
    */
    n = PyList_GET_SIZE(heap);
    if (n > 2500)
        return cache_friendly_heapify(heap, siftup_func);

    /* Transform bottom-up.  The largest index there's any point to
       looking at is the largest with a child index in-range, so must
       have 2*i + 1 < n, or i < (n-1)/2.  If n is even = 2*j, this is
       (2*j-1)/2 = j-1/2 so j-1 is the largest, which is n//2 - 1.  If
       n is odd = 2*j+1, this is (2*j+1-1)/2 = j so j-1 is the largest,
       and that's again n//2-1.
    */
    for (i = (n >> 1) - 1 ; i >= 0 ; i--)
        if (siftup_func((PyListObject *)heap, i))
            return NULL;
    Py_RETURN_NONE;
}

static PyObject *
heapify(PyObject *self, PyObject *heap)
{
    return heapify_internal(heap, siftup);
}

PyDoc_STRVAR(heapify_doc,
"Transform list into a heap, in-place, in O(len(heap)) time.");

static int
siftdown_max(PyListObject *heap, Py_ssize_t startpos, Py_ssize_t pos)
{
    PyObject *newitem, *parent, **arr;
    Py_ssize_t parentpos, size;
    int cmp;

    assert(PyList_Check(heap));
    size = PyList_GET_SIZE(heap);
    if (pos >= size) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return -1;
    }

    /* Follow the path to the root, moving parents down until finding
       a place newitem fits. */
    arr = _PyList_ITEMS(heap);
    newitem = arr[pos];
    while (pos > startpos) {
        parentpos = (pos - 1) >> 1;
        parent = arr[parentpos];
        cmp = PyObject_RichCompareBool(parent, newitem, Py_LT);
        if (cmp < 0)
            return -1;
        if (size != PyList_GET_SIZE(heap)) {
            PyErr_SetString(PyExc_RuntimeError,
                            "list changed size during iteration");
            return -1;
        }
        if (cmp == 0)
            break;
        arr = _PyList_ITEMS(heap);
        parent = arr[parentpos];
        newitem = arr[pos];
        arr[parentpos] = newitem;
        arr[pos] = parent;
        pos = parentpos;
    }
    return 0;
}

static int
siftup_max(PyListObject *heap, Py_ssize_t pos)
{
    Py_ssize_t startpos, endpos, childpos, limit;
    PyObject *tmp1, *tmp2, **arr;
    int cmp;

    assert(PyList_Check(heap));
    endpos = PyList_GET_SIZE(heap);
    startpos = pos;
    if (pos >= endpos) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return -1;
    }

    /* Bubble up the smaller child until hitting a leaf. */
    arr = _PyList_ITEMS(heap);
    limit = endpos >> 1;         /* smallest pos that has no child */
    while (pos < limit) {
        /* Set childpos to index of smaller child.   */
        childpos = 2*pos + 1;    /* leftmost child position  */
        if (childpos + 1 < endpos) {
            cmp = PyObject_RichCompareBool(
                arr[childpos + 1],
                arr[childpos],
                Py_LT);
            if (cmp < 0)
                return -1;
            childpos += ((unsigned)cmp ^ 1);   /* increment when cmp==0 */
            arr = _PyList_ITEMS(heap);         /* arr may have changed */
            if (endpos != PyList_GET_SIZE(heap)) {
                PyErr_SetString(PyExc_RuntimeError,
                                "list changed size during iteration");
                return -1;
            }
        }
        /* Move the smaller child up. */
        tmp1 = arr[childpos];
        tmp2 = arr[pos];
        arr[childpos] = tmp2;
        arr[pos] = tmp1;
        pos = childpos;
    }
    /* Bubble it up to its final resting place (by sifting its parents down). */
    return siftdown_max(heap, startpos, pos);
}

static PyObject *
heappop_max(PyObject *self, PyObject *heap)
{
    return heappop_internal(heap, siftup_max);
}

PyDoc_STRVAR(heappop_max_doc, "Maxheap variant of heappop.");

static PyObject *
heapreplace_max(PyObject *self, PyObject *args)
{
    return heapreplace_internal(args, siftup_max);
}

PyDoc_STRVAR(heapreplace_max_doc, "Maxheap variant of heapreplace");

static PyObject *
heapify_max(PyObject *self, PyObject *heap)
{
    return heapify_internal(heap, siftup_max);
}

PyDoc_STRVAR(heapify_max_doc, "Maxheap variant of heapify.");

static PyMethodDef heapq_methods[] = {
    {"heappush",        (PyCFunction)heappush,
        METH_VARARGS,           heappush_doc},
    {"heappushpop",     (PyCFunction)heappushpop,
        METH_VARARGS,           heappushpop_doc},
    {"heappop",         (PyCFunction)heappop,
        METH_O,                 heappop_doc},
    {"heapreplace",     (PyCFunction)heapreplace,
        METH_VARARGS,           heapreplace_doc},
    {"heapify",         (PyCFunction)heapify,
        METH_O,                 heapify_doc},
    {"_heappop_max",    (PyCFunction)heappop_max,
        METH_O,                 heappop_max_doc},
    {"_heapreplace_max",(PyCFunction)heapreplace_max,
        METH_VARARGS,           heapreplace_max_doc},
    {"_heapify_max",    (PyCFunction)heapify_max,
        METH_O,                 heapify_max_doc},
    {NULL,              NULL}           /* sentinel */
};

PyDoc_STRVAR(module_doc,
"Heap queue algorithm (a.k.a. priority queue).\n\
\n\
Heaps are arrays for which a[k] <= a[2*k+1] and a[k] <= a[2*k+2] for\n\
all k, counting elements from 0.  For the sake of comparison,\n\
non-existing elements are considered to be infinite.  The interesting\n\
property of a heap is that a[0] is always its smallest element.\n\
\n\
Usage:\n\
\n\
heap = []            # creates an empty heap\n\
heappush(heap, item) # pushes a new item on the heap\n\
item = heappop(heap) # pops the smallest item from the heap\n\
item = heap[0]       # smallest item on the heap without popping it\n\
heapify(x)           # transforms list into a heap, in-place, in linear time\n\
item = heapreplace(heap, item) # pops and returns smallest item, and adds\n\
                               # new item; the heap size is unchanged\n\
\n\
Our API differs from textbook heap algorithms as follows:\n\
\n\
- We use 0-based indexing.  This makes the relationship between the\n\
  index for a node and the indexes for its children slightly less\n\
  obvious, but is more suitable since Python uses 0-based indexing.\n\
\n\
- Our heappop() method returns the smallest item, not the largest.\n\
\n\
These two make it possible to view the heap as a regular Python list\n\
without surprises: heap[0] is the smallest item, and heap.sort()\n\
maintains the heap invariant!\n");


PyDoc_STRVAR(__about__,
"Heap queues\n\
\n\
[explanation by Fran\xc3\xa7ois Pinard]\n\
\n\
Heaps are arrays for which a[k] <= a[2*k+1] and a[k] <= a[2*k+2] for\n\
all k, counting elements from 0.  For the sake of comparison,\n\
non-existing elements are considered to be infinite.  The interesting\n\
property of a heap is that a[0] is always its smallest element.\n"
"\n\
The strange invariant above is meant to be an efficient memory\n\
representation for a tournament.  The numbers below are `k', not a[k]:\n\
\n\
                                   0\n\
\n\
                  1                                 2\n\
\n\
          3               4                5               6\n\
\n\
      7       8       9       10      11      12      13      14\n\
\n\
    15 16   17 18   19 20   21 22   23 24   25 26   27 28   29 30\n\
\n\
\n\
In the tree above, each cell `k' is topping `2*k+1' and `2*k+2'.  In\n\
a usual binary tournament we see in sports, each cell is the winner\n\
over the two cells it tops, and we can trace the winner down the tree\n\
to see all opponents s/he had.  However, in many computer applications\n\
of such tournaments, we do not need to trace the history of a winner.\n\
To be more memory efficient, when a winner is promoted, we try to\n\
replace it by something else at a lower level, and the rule becomes\n\
that a cell and the two cells it tops contain three different items,\n\
but the top cell \"wins\" over the two topped cells.\n"
"\n\
If this heap invariant is protected at all time, index 0 is clearly\n\
the overall winner.  The simplest algorithmic way to remove it and\n\
find the \"next\" winner is to move some loser (let's say cell 30 in the\n\
diagram above) into the 0 position, and then percolate this new 0 down\n\
the tree, exchanging values, until the invariant is re-established.\n\
This is clearly logarithmic on the total number of items in the tree.\n\
By iterating over all items, you get an O(n ln n) sort.\n"
"\n\
A nice feature of this sort is that you can efficiently insert new\n\
items while the sort is going on, provided that the inserted items are\n\
not \"better\" than the last 0'th element you extracted.  This is\n\
especially useful in simulation contexts, where the tree holds all\n\
incoming events, and the \"win\" condition means the smallest scheduled\n\
time.  When an event schedule other events for execution, they are\n\
scheduled into the future, so they can easily go into the heap.  So, a\n\
heap is a good structure for implementing schedulers (this is what I\n\
used for my MIDI sequencer :-).\n"
"\n\
Various structures for implementing schedulers have been extensively\n\
studied, and heaps are good for this, as they are reasonably speedy,\n\
the speed is almost constant, and the worst case is not much different\n\
than the average case.  However, there are other representations which\n\
are more efficient overall, yet the worst cases might be terrible.\n"
"\n\
Heaps are also very useful in big disk sorts.  You most probably all\n\
know that a big sort implies producing \"runs\" (which are pre-sorted\n\
sequences, which size is usually related to the amount of CPU memory),\n\
followed by a merging passes for these runs, which merging is often\n\
very cleverly organised[1].  It is very important that the initial\n\
sort produces the longest runs possible.  Tournaments are a good way\n\
to that.  If, using all the memory available to hold a tournament, you\n\
replace and percolate items that happen to fit the current run, you'll\n\
produce runs which are twice the size of the memory for random input,\n\
and much better for input fuzzily ordered.\n"
"\n\
Moreover, if you output the 0'th item on disk and get an input which\n\
may not fit in the current tournament (because the value \"wins\" over\n\
the last output value), it cannot fit in the heap, so the size of the\n\
heap decreases.  The freed memory could be cleverly reused immediately\n\
for progressively building a second heap, which grows at exactly the\n\
same rate the first heap is melting.  When the first heap completely\n\
vanishes, you switch heaps and start a new run.  Clever and quite\n\
effective!\n\
\n\
In a word, heaps are useful memory structures to know.  I use them in\n\
a few applications, and I think it is good to keep a `heap' module\n\
around. :-)\n"
"\n\
--------------------\n\
[1] The disk balancing algorithms which are current, nowadays, are\n\
more annoying than clever, and this is a consequence of the seeking\n\
capabilities of the disks.  On devices which cannot seek, like big\n\
tape drives, the story was quite different, and one had to be very\n\
clever to ensure (far in advance) that each tape movement will be the\n\
most effective possible (that is, will best participate at\n\
\"progressing\" the merge).  Some tapes were even able to read\n\
backwards, and this was also used to avoid the rewinding time.\n\
Believe me, real good tape sorts were quite spectacular to watch!\n\
From all times, sorting has always been a Great Art! :-)\n");


static struct PyModuleDef _heapqmodule = {
    PyModuleDef_HEAD_INIT,
    "_heapq",
    module_doc,
    -1,
    heapq_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__heapq(void)
{
    PyObject *m, *about;

    m = PyModule_Create(&_heapqmodule);
    if (m == NULL)
        return NULL;
    about = PyUnicode_DecodeUTF8(__about__, strlen(__about__), NULL);
    PyModule_AddObject(m, "__about__", about);
    return m;
}

