#include "Python.h"
#include "structmember.h"

#ifdef STDC_HEADERS
#include <stddef.h>
#else
#include <sys/types.h>          /* For size_t */
#endif

/*[clinic input]
module _collections
class _tuplegetter "_tuplegetterobject *" "&tuplegetter_type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=a8ece4ccad7e30ac]*/

static PyTypeObject tuplegetter_type;
#include "clinic/_collectionsmodule.c.h"

/* collections module implementation of a deque() datatype
   Written and maintained by Raymond D. Hettinger <python@rcn.com>
*/

/* The block length may be set to any number over 1.  Larger numbers
 * reduce the number of calls to the memory allocator, give faster
 * indexing and rotation, and reduce the link to data overhead ratio.
 * Making the block length a power of two speeds-up the modulo
 * and division calculations in deque_item() and deque_ass_item().
 */

#define BLOCKLEN 64
#define CENTER ((BLOCKLEN - 1) / 2)

/* Data for deque objects is stored in a doubly-linked list of fixed
 * length blocks.  This assures that appends or pops never move any
 * other data elements besides the one being appended or popped.
 *
 * Another advantage is that it completely avoids use of realloc(),
 * resulting in more predictable performance.
 *
 * Textbook implementations of doubly-linked lists store one datum
 * per link, but that gives them a 200% memory overhead (a prev and
 * next link for each datum) and it costs one malloc() call per data
 * element.  By using fixed-length blocks, the link to data ratio is
 * significantly improved and there are proportionally fewer calls
 * to malloc() and free().  The data blocks of consecutive pointers
 * also improve cache locality.
 *
 * The list of blocks is never empty, so d.leftblock and d.rightblock
 * are never equal to NULL.  The list is not circular.
 *
 * A deque d's first element is at d.leftblock[leftindex]
 * and its last element is at d.rightblock[rightindex].
 *
 * Unlike Python slice indices, these indices are inclusive on both
 * ends.  This makes the algorithms for left and right operations
 * more symmetrical and it simplifies the design.
 *
 * The indices, d.leftindex and d.rightindex are always in the range:
 *     0 <= index < BLOCKLEN
 *
 * And their exact relationship is:
 *     (d.leftindex + d.len - 1) % BLOCKLEN == d.rightindex
 *
 * Whenever d.leftblock == d.rightblock, then:
 *     d.leftindex + d.len - 1 == d.rightindex
 *
 * However, when d.leftblock != d.rightblock, the d.leftindex and
 * d.rightindex become indices into distinct blocks and either may
 * be larger than the other.
 *
 * Empty deques have:
 *     d.len == 0
 *     d.leftblock == d.rightblock
 *     d.leftindex == CENTER + 1
 *     d.rightindex == CENTER
 *
 * Checking for d.len == 0 is the intended way to see whether d is empty.
 */

typedef struct BLOCK {
    struct BLOCK *leftlink;
    PyObject *data[BLOCKLEN];
    struct BLOCK *rightlink;
} block;

typedef struct {
    PyObject_VAR_HEAD
    block *leftblock;
    block *rightblock;
    Py_ssize_t leftindex;       /* 0 <= leftindex < BLOCKLEN */
    Py_ssize_t rightindex;      /* 0 <= rightindex < BLOCKLEN */
    size_t state;               /* incremented whenever the indices move */
    Py_ssize_t maxlen;          /* maxlen is -1 for unbounded deques */
    PyObject *weakreflist;
} dequeobject;

static PyTypeObject deque_type;

/* For debug builds, add error checking to track the endpoints
 * in the chain of links.  The goal is to make sure that link
 * assignments only take place at endpoints so that links already
 * in use do not get overwritten.
 *
 * CHECK_END should happen before each assignment to a block's link field.
 * MARK_END should happen whenever a link field becomes a new endpoint.
 * This happens when new blocks are added or whenever an existing
 * block is freed leaving another existing block as the new endpoint.
 */

#ifndef NDEBUG
#define MARK_END(link)  link = NULL;
#define CHECK_END(link) assert(link == NULL);
#define CHECK_NOT_END(link) assert(link != NULL);
#else
#define MARK_END(link)
#define CHECK_END(link)
#define CHECK_NOT_END(link)
#endif

/* A simple freelisting scheme is used to minimize calls to the memory
   allocator.  It accommodates common use cases where new blocks are being
   added at about the same rate as old blocks are being freed.
 */

#define MAXFREEBLOCKS 16
static Py_ssize_t numfreeblocks = 0;
static block *freeblocks[MAXFREEBLOCKS];

static block *
newblock(void) {
    block *b;
    if (numfreeblocks) {
        numfreeblocks--;
        return freeblocks[numfreeblocks];
    }
    b = PyMem_Malloc(sizeof(block));
    if (b != NULL) {
        return b;
    }
    PyErr_NoMemory();
    return NULL;
}

static void
freeblock(block *b)
{
    if (numfreeblocks < MAXFREEBLOCKS) {
        freeblocks[numfreeblocks] = b;
        numfreeblocks++;
    } else {
        PyMem_Free(b);
    }
}

static PyObject *
deque_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    dequeobject *deque;
    block *b;

    /* create dequeobject structure */
    deque = (dequeobject *)type->tp_alloc(type, 0);
    if (deque == NULL)
        return NULL;

    b = newblock();
    if (b == NULL) {
        Py_DECREF(deque);
        return NULL;
    }
    MARK_END(b->leftlink);
    MARK_END(b->rightlink);

    assert(BLOCKLEN >= 2);
    Py_SET_SIZE(deque, 0);
    deque->leftblock = b;
    deque->rightblock = b;
    deque->leftindex = CENTER + 1;
    deque->rightindex = CENTER;
    deque->state = 0;
    deque->maxlen = -1;
    deque->weakreflist = NULL;

    return (PyObject *)deque;
}

static PyObject *
deque_pop(dequeobject *deque, PyObject *unused)
{
    PyObject *item;
    block *prevblock;

    if (Py_SIZE(deque) == 0) {
        PyErr_SetString(PyExc_IndexError, "pop from an empty deque");
        return NULL;
    }
    item = deque->rightblock->data[deque->rightindex];
    deque->rightindex--;
    Py_SET_SIZE(deque, Py_SIZE(deque) - 1);
    deque->state++;

    if (deque->rightindex < 0) {
        if (Py_SIZE(deque)) {
            prevblock = deque->rightblock->leftlink;
            assert(deque->leftblock != deque->rightblock);
            freeblock(deque->rightblock);
            CHECK_NOT_END(prevblock);
            MARK_END(prevblock->rightlink);
            deque->rightblock = prevblock;
            deque->rightindex = BLOCKLEN - 1;
        } else {
            assert(deque->leftblock == deque->rightblock);
            assert(deque->leftindex == deque->rightindex+1);
            /* re-center instead of freeing a block */
            deque->leftindex = CENTER + 1;
            deque->rightindex = CENTER;
        }
    }
    return item;
}

PyDoc_STRVAR(pop_doc, "Remove and return the rightmost element.");

static PyObject *
deque_popleft(dequeobject *deque, PyObject *unused)
{
    PyObject *item;
    block *prevblock;

    if (Py_SIZE(deque) == 0) {
        PyErr_SetString(PyExc_IndexError, "pop from an empty deque");
        return NULL;
    }
    assert(deque->leftblock != NULL);
    item = deque->leftblock->data[deque->leftindex];
    deque->leftindex++;
    Py_SET_SIZE(deque, Py_SIZE(deque) - 1);
    deque->state++;

    if (deque->leftindex == BLOCKLEN) {
        if (Py_SIZE(deque)) {
            assert(deque->leftblock != deque->rightblock);
            prevblock = deque->leftblock->rightlink;
            freeblock(deque->leftblock);
            CHECK_NOT_END(prevblock);
            MARK_END(prevblock->leftlink);
            deque->leftblock = prevblock;
            deque->leftindex = 0;
        } else {
            assert(deque->leftblock == deque->rightblock);
            assert(deque->leftindex == deque->rightindex+1);
            /* re-center instead of freeing a block */
            deque->leftindex = CENTER + 1;
            deque->rightindex = CENTER;
        }
    }
    return item;
}

PyDoc_STRVAR(popleft_doc, "Remove and return the leftmost element.");

/* The deque's size limit is d.maxlen.  The limit can be zero or positive.
 * If there is no limit, then d.maxlen == -1.
 *
 * After an item is added to a deque, we check to see if the size has
 * grown past the limit. If it has, we get the size back down to the limit
 * by popping an item off of the opposite end.  The methods that can
 * trigger this are append(), appendleft(), extend(), and extendleft().
 *
 * The macro to check whether a deque needs to be trimmed uses a single
 * unsigned test that returns true whenever 0 <= maxlen < Py_SIZE(deque).
 */

#define NEEDS_TRIM(deque, maxlen) ((size_t)(maxlen) < (size_t)(Py_SIZE(deque)))

static inline int
deque_append_internal(dequeobject *deque, PyObject *item, Py_ssize_t maxlen)
{
    if (deque->rightindex == BLOCKLEN - 1) {
        block *b = newblock();
        if (b == NULL)
            return -1;
        b->leftlink = deque->rightblock;
        CHECK_END(deque->rightblock->rightlink);
        deque->rightblock->rightlink = b;
        deque->rightblock = b;
        MARK_END(b->rightlink);
        deque->rightindex = -1;
    }
    Py_SET_SIZE(deque, Py_SIZE(deque) + 1);
    deque->rightindex++;
    deque->rightblock->data[deque->rightindex] = item;
    if (NEEDS_TRIM(deque, maxlen)) {
        PyObject *olditem = deque_popleft(deque, NULL);
        Py_DECREF(olditem);
    } else {
        deque->state++;
    }
    return 0;
}

static PyObject *
deque_append(dequeobject *deque, PyObject *item)
{
    Py_INCREF(item);
    if (deque_append_internal(deque, item, deque->maxlen) < 0)
        return NULL;
    Py_RETURN_NONE;
}

PyDoc_STRVAR(append_doc, "Add an element to the right side of the deque.");

static inline int
deque_appendleft_internal(dequeobject *deque, PyObject *item, Py_ssize_t maxlen)
{
    if (deque->leftindex == 0) {
        block *b = newblock();
        if (b == NULL)
            return -1;
        b->rightlink = deque->leftblock;
        CHECK_END(deque->leftblock->leftlink);
        deque->leftblock->leftlink = b;
        deque->leftblock = b;
        MARK_END(b->leftlink);
        deque->leftindex = BLOCKLEN;
    }
    Py_SET_SIZE(deque, Py_SIZE(deque) + 1);
    deque->leftindex--;
    deque->leftblock->data[deque->leftindex] = item;
    if (NEEDS_TRIM(deque, deque->maxlen)) {
        PyObject *olditem = deque_pop(deque, NULL);
        Py_DECREF(olditem);
    } else {
        deque->state++;
    }
    return 0;
}

static PyObject *
deque_appendleft(dequeobject *deque, PyObject *item)
{
    Py_INCREF(item);
    if (deque_appendleft_internal(deque, item, deque->maxlen) < 0)
        return NULL;
    Py_RETURN_NONE;
}

PyDoc_STRVAR(appendleft_doc, "Add an element to the left side of the deque.");

static PyObject*
finalize_iterator(PyObject *it)
{
    if (PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_StopIteration))
            PyErr_Clear();
        else {
            Py_DECREF(it);
            return NULL;
        }
    }
    Py_DECREF(it);
    Py_RETURN_NONE;
}

/* Run an iterator to exhaustion.  Shortcut for
   the extend/extendleft methods when maxlen == 0. */
static PyObject*
consume_iterator(PyObject *it)
{
    PyObject *(*iternext)(PyObject *);
    PyObject *item;

    iternext = *Py_TYPE(it)->tp_iternext;
    while ((item = iternext(it)) != NULL) {
        Py_DECREF(item);
    }
    return finalize_iterator(it);
}

static PyObject *
deque_extend(dequeobject *deque, PyObject *iterable)
{
    PyObject *it, *item;
    PyObject *(*iternext)(PyObject *);
    Py_ssize_t maxlen = deque->maxlen;

    /* Handle case where id(deque) == id(iterable) */
    if ((PyObject *)deque == iterable) {
        PyObject *result;
        PyObject *s = PySequence_List(iterable);
        if (s == NULL)
            return NULL;
        result = deque_extend(deque, s);
        Py_DECREF(s);
        return result;
    }

    it = PyObject_GetIter(iterable);
    if (it == NULL)
        return NULL;

    if (maxlen == 0)
        return consume_iterator(it);

    /* Space saving heuristic.  Start filling from the left */
    if (Py_SIZE(deque) == 0) {
        assert(deque->leftblock == deque->rightblock);
        assert(deque->leftindex == deque->rightindex+1);
        deque->leftindex = 1;
        deque->rightindex = 0;
    }

    iternext = *Py_TYPE(it)->tp_iternext;
    while ((item = iternext(it)) != NULL) {
        if (deque_append_internal(deque, item, maxlen) == -1) {
            Py_DECREF(item);
            Py_DECREF(it);
            return NULL;
        }
    }
    return finalize_iterator(it);
}

PyDoc_STRVAR(extend_doc,
"Extend the right side of the deque with elements from the iterable");

static PyObject *
deque_extendleft(dequeobject *deque, PyObject *iterable)
{
    PyObject *it, *item;
    PyObject *(*iternext)(PyObject *);
    Py_ssize_t maxlen = deque->maxlen;

    /* Handle case where id(deque) == id(iterable) */
    if ((PyObject *)deque == iterable) {
        PyObject *result;
        PyObject *s = PySequence_List(iterable);
        if (s == NULL)
            return NULL;
        result = deque_extendleft(deque, s);
        Py_DECREF(s);
        return result;
    }

    it = PyObject_GetIter(iterable);
    if (it == NULL)
        return NULL;

    if (maxlen == 0)
        return consume_iterator(it);

    /* Space saving heuristic.  Start filling from the right */
    if (Py_SIZE(deque) == 0) {
        assert(deque->leftblock == deque->rightblock);
        assert(deque->leftindex == deque->rightindex+1);
        deque->leftindex = BLOCKLEN - 1;
        deque->rightindex = BLOCKLEN - 2;
    }

    iternext = *Py_TYPE(it)->tp_iternext;
    while ((item = iternext(it)) != NULL) {
        if (deque_appendleft_internal(deque, item, maxlen) == -1) {
            Py_DECREF(item);
            Py_DECREF(it);
            return NULL;
        }
    }
    return finalize_iterator(it);
}

PyDoc_STRVAR(extendleft_doc,
"Extend the left side of the deque with elements from the iterable");

static PyObject *
deque_inplace_concat(dequeobject *deque, PyObject *other)
{
    PyObject *result;

    result = deque_extend(deque, other);
    if (result == NULL)
        return result;
    Py_INCREF(deque);
    Py_DECREF(result);
    return (PyObject *)deque;
}

static PyObject *
deque_copy(PyObject *deque, PyObject *Py_UNUSED(ignored))
{
    PyObject *result;
    dequeobject *old_deque = (dequeobject *)deque;
    if (Py_TYPE(deque) == &deque_type) {
        dequeobject *new_deque;
        PyObject *rv;

        new_deque = (dequeobject *)deque_new(&deque_type, (PyObject *)NULL, (PyObject *)NULL);
        if (new_deque == NULL)
            return NULL;
        new_deque->maxlen = old_deque->maxlen;
        /* Fast path for the deque_repeat() common case where len(deque) == 1 */
        if (Py_SIZE(deque) == 1) {
            PyObject *item = old_deque->leftblock->data[old_deque->leftindex];
            rv = deque_append(new_deque, item);
        } else {
            rv = deque_extend(new_deque, deque);
        }
        if (rv != NULL) {
            Py_DECREF(rv);
            return (PyObject *)new_deque;
        }
        Py_DECREF(new_deque);
        return NULL;
    }
    if (old_deque->maxlen < 0)
        result = PyObject_CallOneArg((PyObject *)(Py_TYPE(deque)), deque);
    else
        result = PyObject_CallFunction((PyObject *)(Py_TYPE(deque)), "Oi",
                                       deque, old_deque->maxlen, NULL);
    if (result != NULL && !PyObject_TypeCheck(result, &deque_type)) {
        PyErr_Format(PyExc_TypeError,
                     "%.200s() must return a deque, not %.200s",
                     Py_TYPE(deque)->tp_name, Py_TYPE(result)->tp_name);
        Py_DECREF(result);
        return NULL;
    }
    return result;
}

PyDoc_STRVAR(copy_doc, "Return a shallow copy of a deque.");

static PyObject *
deque_concat(dequeobject *deque, PyObject *other)
{
    PyObject *new_deque, *result;
    int rv;

    rv = PyObject_IsInstance(other, (PyObject *)&deque_type);
    if (rv <= 0) {
        if (rv == 0) {
            PyErr_Format(PyExc_TypeError,
                         "can only concatenate deque (not \"%.200s\") to deque",
                         Py_TYPE(other)->tp_name);
        }
        return NULL;
    }

    new_deque = deque_copy((PyObject *)deque, NULL);
    if (new_deque == NULL)
        return NULL;
    result = deque_extend((dequeobject *)new_deque, other);
    if (result == NULL) {
        Py_DECREF(new_deque);
        return NULL;
    }
    Py_DECREF(result);
    return new_deque;
}

static int
deque_clear(dequeobject *deque)
{
    block *b;
    block *prevblock;
    block *leftblock;
    Py_ssize_t leftindex;
    Py_ssize_t n, m;
    PyObject *item;
    PyObject **itemptr, **limit;

    if (Py_SIZE(deque) == 0)
        return 0;

    /* During the process of clearing a deque, decrefs can cause the
       deque to mutate.  To avoid fatal confusion, we have to make the
       deque empty before clearing the blocks and never refer to
       anything via deque->ref while clearing.  (This is the same
       technique used for clearing lists, sets, and dicts.)

       Making the deque empty requires allocating a new empty block.  In
       the unlikely event that memory is full, we fall back to an
       alternate method that doesn't require a new block.  Repeating
       pops in a while-loop is slower, possibly re-entrant (and a clever
       adversary could cause it to never terminate).
    */

    b = newblock();
    if (b == NULL) {
        PyErr_Clear();
        goto alternate_method;
    }

    /* Remember the old size, leftblock, and leftindex */
    n = Py_SIZE(deque);
    leftblock = deque->leftblock;
    leftindex = deque->leftindex;

    /* Set the deque to be empty using the newly allocated block */
    MARK_END(b->leftlink);
    MARK_END(b->rightlink);
    Py_SET_SIZE(deque, 0);
    deque->leftblock = b;
    deque->rightblock = b;
    deque->leftindex = CENTER + 1;
    deque->rightindex = CENTER;
    deque->state++;

    /* Now the old size, leftblock, and leftindex are disconnected from
       the empty deque and we can use them to decref the pointers.
    */
    m = (BLOCKLEN - leftindex > n) ? n : BLOCKLEN - leftindex;
    itemptr = &leftblock->data[leftindex];
    limit = itemptr + m;
    n -= m;
    while (1) {
        if (itemptr == limit) {
            if (n == 0)
                break;
            CHECK_NOT_END(leftblock->rightlink);
            prevblock = leftblock;
            leftblock = leftblock->rightlink;
            m = (n > BLOCKLEN) ? BLOCKLEN : n;
            itemptr = leftblock->data;
            limit = itemptr + m;
            n -= m;
            freeblock(prevblock);
        }
        item = *(itemptr++);
        Py_DECREF(item);
    }
    CHECK_END(leftblock->rightlink);
    freeblock(leftblock);
    return 0;

  alternate_method:
    while (Py_SIZE(deque)) {
        item = deque_pop(deque, NULL);
        assert (item != NULL);
        Py_DECREF(item);
    }
    return 0;
}

static PyObject *
deque_clearmethod(dequeobject *deque, PyObject *Py_UNUSED(ignored))
{
    deque_clear(deque);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(clear_doc, "Remove all elements from the deque.");

static PyObject *
deque_inplace_repeat(dequeobject *deque, Py_ssize_t n)
{
    Py_ssize_t i, m, size;
    PyObject *seq;
    PyObject *rv;

    size = Py_SIZE(deque);
    if (size == 0 || n == 1) {
        Py_INCREF(deque);
        return (PyObject *)deque;
    }

    if (n <= 0) {
        deque_clear(deque);
        Py_INCREF(deque);
        return (PyObject *)deque;
    }

    if (size == 1) {
        /* common case, repeating a single element */
        PyObject *item = deque->leftblock->data[deque->leftindex];

        if (deque->maxlen >= 0 && n > deque->maxlen)
            n = deque->maxlen;

        deque->state++;
        for (i = 0 ; i < n-1 ; ) {
            if (deque->rightindex == BLOCKLEN - 1) {
                block *b = newblock();
                if (b == NULL) {
                    Py_SET_SIZE(deque, Py_SIZE(deque) + i);
                    return NULL;
                }
                b->leftlink = deque->rightblock;
                CHECK_END(deque->rightblock->rightlink);
                deque->rightblock->rightlink = b;
                deque->rightblock = b;
                MARK_END(b->rightlink);
                deque->rightindex = -1;
            }
            m = n - 1 - i;
            if (m > BLOCKLEN - 1 - deque->rightindex)
                m = BLOCKLEN - 1 - deque->rightindex;
            i += m;
            while (m--) {
                deque->rightindex++;
                Py_INCREF(item);
                deque->rightblock->data[deque->rightindex] = item;
            }
        }
        Py_SET_SIZE(deque, Py_SIZE(deque) + i);
        Py_INCREF(deque);
        return (PyObject *)deque;
    }

    if ((size_t)size > PY_SSIZE_T_MAX / (size_t)n) {
        return PyErr_NoMemory();
    }

    seq = PySequence_List((PyObject *)deque);
    if (seq == NULL)
        return seq;

    /* Reduce the number of repetitions when maxlen would be exceeded */
    if (deque->maxlen >= 0 && n * size > deque->maxlen)
        n = (deque->maxlen + size - 1) / size;

    for (i = 0 ; i < n-1 ; i++) {
        rv = deque_extend(deque, seq);
        if (rv == NULL) {
            Py_DECREF(seq);
            return NULL;
        }
        Py_DECREF(rv);
    }
    Py_INCREF(deque);
    Py_DECREF(seq);
    return (PyObject *)deque;
}

static PyObject *
deque_repeat(dequeobject *deque, Py_ssize_t n)
{
    dequeobject *new_deque;
    PyObject *rv;

    new_deque = (dequeobject *)deque_copy((PyObject *) deque, NULL);
    if (new_deque == NULL)
        return NULL;
    rv = deque_inplace_repeat(new_deque, n);
    Py_DECREF(new_deque);
    return rv;
}

/* The rotate() method is part of the public API and is used internally
as a primitive for other methods.

Rotation by 1 or -1 is a common case, so any optimizations for high
volume rotations should take care not to penalize the common case.

Conceptually, a rotate by one is equivalent to a pop on one side and an
append on the other.  However, a pop/append pair is unnecessarily slow
because it requires an incref/decref pair for an object located randomly
in memory.  It is better to just move the object pointer from one block
to the next without changing the reference count.

When moving batches of pointers, it is tempting to use memcpy() but that
proved to be slower than a simple loop for a variety of reasons.
Memcpy() cannot know in advance that we're copying pointers instead of
bytes, that the source and destination are pointer aligned and
non-overlapping, that moving just one pointer is a common case, that we
never need to move more than BLOCKLEN pointers, and that at least one
pointer is always moved.

For high volume rotations, newblock() and freeblock() are never called
more than once.  Previously emptied blocks are immediately reused as a
destination block.  If a block is left-over at the end, it is freed.
*/

static int
_deque_rotate(dequeobject *deque, Py_ssize_t n)
{
    block *b = NULL;
    block *leftblock = deque->leftblock;
    block *rightblock = deque->rightblock;
    Py_ssize_t leftindex = deque->leftindex;
    Py_ssize_t rightindex = deque->rightindex;
    Py_ssize_t len=Py_SIZE(deque), halflen=len>>1;
    int rv = -1;

    if (len <= 1)
        return 0;
    if (n > halflen || n < -halflen) {
        n %= len;
        if (n > halflen)
            n -= len;
        else if (n < -halflen)
            n += len;
    }
    assert(len > 1);
    assert(-halflen <= n && n <= halflen);

    deque->state++;
    while (n > 0) {
        if (leftindex == 0) {
            if (b == NULL) {
                b = newblock();
                if (b == NULL)
                    goto done;
            }
            b->rightlink = leftblock;
            CHECK_END(leftblock->leftlink);
            leftblock->leftlink = b;
            leftblock = b;
            MARK_END(b->leftlink);
            leftindex = BLOCKLEN;
            b = NULL;
        }
        assert(leftindex > 0);
        {
            PyObject **src, **dest;
            Py_ssize_t m = n;

            if (m > rightindex + 1)
                m = rightindex + 1;
            if (m > leftindex)
                m = leftindex;
            assert (m > 0 && m <= len);
            rightindex -= m;
            leftindex -= m;
            src = &rightblock->data[rightindex + 1];
            dest = &leftblock->data[leftindex];
            n -= m;
            do {
                *(dest++) = *(src++);
            } while (--m);
        }
        if (rightindex < 0) {
            assert(leftblock != rightblock);
            assert(b == NULL);
            b = rightblock;
            CHECK_NOT_END(rightblock->leftlink);
            rightblock = rightblock->leftlink;
            MARK_END(rightblock->rightlink);
            rightindex = BLOCKLEN - 1;
        }
    }
    while (n < 0) {
        if (rightindex == BLOCKLEN - 1) {
            if (b == NULL) {
                b = newblock();
                if (b == NULL)
                    goto done;
            }
            b->leftlink = rightblock;
            CHECK_END(rightblock->rightlink);
            rightblock->rightlink = b;
            rightblock = b;
            MARK_END(b->rightlink);
            rightindex = -1;
            b = NULL;
        }
        assert (rightindex < BLOCKLEN - 1);
        {
            PyObject **src, **dest;
            Py_ssize_t m = -n;

            if (m > BLOCKLEN - leftindex)
                m = BLOCKLEN - leftindex;
            if (m > BLOCKLEN - 1 - rightindex)
                m = BLOCKLEN - 1 - rightindex;
            assert (m > 0 && m <= len);
            src = &leftblock->data[leftindex];
            dest = &rightblock->data[rightindex + 1];
            leftindex += m;
            rightindex += m;
            n += m;
            do {
                *(dest++) = *(src++);
            } while (--m);
        }
        if (leftindex == BLOCKLEN) {
            assert(leftblock != rightblock);
            assert(b == NULL);
            b = leftblock;
            CHECK_NOT_END(leftblock->rightlink);
            leftblock = leftblock->rightlink;
            MARK_END(leftblock->leftlink);
            leftindex = 0;
        }
    }
    rv = 0;
done:
    if (b != NULL)
        freeblock(b);
    deque->leftblock = leftblock;
    deque->rightblock = rightblock;
    deque->leftindex = leftindex;
    deque->rightindex = rightindex;

    return rv;
}

static PyObject *
deque_rotate(dequeobject *deque, PyObject *const *args, Py_ssize_t nargs)
{
    Py_ssize_t n=1;

    if (!_PyArg_ParseStack(args, nargs, "|n:rotate", &n)) {
        return NULL;
    }

    if (!_deque_rotate(deque, n))
        Py_RETURN_NONE;
    return NULL;
}

PyDoc_STRVAR(rotate_doc,
"Rotate the deque n steps to the right (default n=1).  If n is negative, rotates left.");

static PyObject *
deque_reverse(dequeobject *deque, PyObject *unused)
{
    block *leftblock = deque->leftblock;
    block *rightblock = deque->rightblock;
    Py_ssize_t leftindex = deque->leftindex;
    Py_ssize_t rightindex = deque->rightindex;
    Py_ssize_t n = Py_SIZE(deque) >> 1;
    PyObject *tmp;

    while (--n >= 0) {
        /* Validate that pointers haven't met in the middle */
        assert(leftblock != rightblock || leftindex < rightindex);
        CHECK_NOT_END(leftblock);
        CHECK_NOT_END(rightblock);

        /* Swap */
        tmp = leftblock->data[leftindex];
        leftblock->data[leftindex] = rightblock->data[rightindex];
        rightblock->data[rightindex] = tmp;

        /* Advance left block/index pair */
        leftindex++;
        if (leftindex == BLOCKLEN) {
            leftblock = leftblock->rightlink;
            leftindex = 0;
        }

        /* Step backwards with the right block/index pair */
        rightindex--;
        if (rightindex < 0) {
            rightblock = rightblock->leftlink;
            rightindex = BLOCKLEN - 1;
        }
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(reverse_doc,
"D.reverse() -- reverse *IN PLACE*");

static PyObject *
deque_count(dequeobject *deque, PyObject *v)
{
    block *b = deque->leftblock;
    Py_ssize_t index = deque->leftindex;
    Py_ssize_t n = Py_SIZE(deque);
    Py_ssize_t count = 0;
    size_t start_state = deque->state;
    PyObject *item;
    int cmp;

    while (--n >= 0) {
        CHECK_NOT_END(b);
        item = b->data[index];
        Py_INCREF(item);
        cmp = PyObject_RichCompareBool(item, v, Py_EQ);
        Py_DECREF(item);
        if (cmp < 0)
            return NULL;
        count += cmp;

        if (start_state != deque->state) {
            PyErr_SetString(PyExc_RuntimeError,
                            "deque mutated during iteration");
            return NULL;
        }

        /* Advance left block/index pair */
        index++;
        if (index == BLOCKLEN) {
            b = b->rightlink;
            index = 0;
        }
    }
    return PyLong_FromSsize_t(count);
}

PyDoc_STRVAR(count_doc,
"D.count(value) -> integer -- return number of occurrences of value");

static int
deque_contains(dequeobject *deque, PyObject *v)
{
    block *b = deque->leftblock;
    Py_ssize_t index = deque->leftindex;
    Py_ssize_t n = Py_SIZE(deque);
    size_t start_state = deque->state;
    PyObject *item;
    int cmp;

    while (--n >= 0) {
        CHECK_NOT_END(b);
        item = b->data[index];
        Py_INCREF(item);
        cmp = PyObject_RichCompareBool(item, v, Py_EQ);
        Py_DECREF(item);
        if (cmp) {
            return cmp;
        }
        if (start_state != deque->state) {
            PyErr_SetString(PyExc_RuntimeError,
                            "deque mutated during iteration");
            return -1;
        }
        index++;
        if (index == BLOCKLEN) {
            b = b->rightlink;
            index = 0;
        }
    }
    return 0;
}

static Py_ssize_t
deque_len(dequeobject *deque)
{
    return Py_SIZE(deque);
}

static PyObject *
deque_index(dequeobject *deque, PyObject *const *args, Py_ssize_t nargs)
{
    Py_ssize_t i, n, start=0, stop=Py_SIZE(deque);
    PyObject *v, *item;
    block *b = deque->leftblock;
    Py_ssize_t index = deque->leftindex;
    size_t start_state = deque->state;
    int cmp;

    if (!_PyArg_ParseStack(args, nargs, "O|O&O&:index", &v,
                           _PyEval_SliceIndexNotNone, &start,
                           _PyEval_SliceIndexNotNone, &stop)) {
        return NULL;
    }

    if (start < 0) {
        start += Py_SIZE(deque);
        if (start < 0)
            start = 0;
    }
    if (stop < 0) {
        stop += Py_SIZE(deque);
        if (stop < 0)
            stop = 0;
    }
    if (stop > Py_SIZE(deque))
        stop = Py_SIZE(deque);
    if (start > stop)
        start = stop;
    assert(0 <= start && start <= stop && stop <= Py_SIZE(deque));

    for (i=0 ; i < start - BLOCKLEN ; i += BLOCKLEN) {
        b = b->rightlink;
    }
    for ( ; i < start ; i++) {
        index++;
        if (index == BLOCKLEN) {
            b = b->rightlink;
            index = 0;
        }
    }

    n = stop - i;
    while (--n >= 0) {
        CHECK_NOT_END(b);
        item = b->data[index];
        cmp = PyObject_RichCompareBool(item, v, Py_EQ);
        if (cmp > 0)
            return PyLong_FromSsize_t(stop - n - 1);
        if (cmp < 0)
            return NULL;
        if (start_state != deque->state) {
            PyErr_SetString(PyExc_RuntimeError,
                            "deque mutated during iteration");
            return NULL;
        }
        index++;
        if (index == BLOCKLEN) {
            b = b->rightlink;
            index = 0;
        }
    }
    PyErr_Format(PyExc_ValueError, "%R is not in deque", v);
    return NULL;
}

PyDoc_STRVAR(index_doc,
"D.index(value, [start, [stop]]) -> integer -- return first index of value.\n"
"Raises ValueError if the value is not present.");

/* insert(), remove(), and delitem() are implemented in terms of
   rotate() for simplicity and reasonable performance near the end
   points.  If for some reason these methods become popular, it is not
   hard to re-implement this using direct data movement (similar to
   the code used in list slice assignments) and achieve a performance
   boost (by moving each pointer only once instead of twice).
*/

static PyObject *
deque_insert(dequeobject *deque, PyObject *const *args, Py_ssize_t nargs)
{
    Py_ssize_t index;
    Py_ssize_t n = Py_SIZE(deque);
    PyObject *value;
    PyObject *rv;

    if (!_PyArg_ParseStack(args, nargs, "nO:insert", &index, &value)) {
        return NULL;
    }

    if (deque->maxlen == Py_SIZE(deque)) {
        PyErr_SetString(PyExc_IndexError, "deque already at its maximum size");
        return NULL;
    }
    if (index >= n)
        return deque_append(deque, value);
    if (index <= -n || index == 0)
        return deque_appendleft(deque, value);
    if (_deque_rotate(deque, -index))
        return NULL;
    if (index < 0)
        rv = deque_append(deque, value);
    else
        rv = deque_appendleft(deque, value);
    if (rv == NULL)
        return NULL;
    Py_DECREF(rv);
    if (_deque_rotate(deque, index))
        return NULL;
    Py_RETURN_NONE;
}

PyDoc_STRVAR(insert_doc,
"D.insert(index, object) -- insert object before index");

static PyObject *
deque_remove(dequeobject *deque, PyObject *value)
{
    Py_ssize_t i, n=Py_SIZE(deque);

    for (i=0 ; i<n ; i++) {
        PyObject *item = deque->leftblock->data[deque->leftindex];
        int cmp = PyObject_RichCompareBool(item, value, Py_EQ);

        if (Py_SIZE(deque) != n) {
            PyErr_SetString(PyExc_IndexError,
                "deque mutated during remove().");
            return NULL;
        }
        if (cmp > 0) {
            PyObject *tgt = deque_popleft(deque, NULL);
            assert (tgt != NULL);
            if (_deque_rotate(deque, i))
                return NULL;
            Py_DECREF(tgt);
            Py_RETURN_NONE;
        }
        else if (cmp < 0) {
            _deque_rotate(deque, i);
            return NULL;
        }
        _deque_rotate(deque, -1);
    }
    PyErr_SetString(PyExc_ValueError, "deque.remove(x): x not in deque");
    return NULL;
}

PyDoc_STRVAR(remove_doc,
"D.remove(value) -- remove first occurrence of value.");

static int
valid_index(Py_ssize_t i, Py_ssize_t limit)
{
    /* The cast to size_t lets us use just a single comparison
       to check whether i is in the range: 0 <= i < limit */
    return (size_t) i < (size_t) limit;
}

static PyObject *
deque_item(dequeobject *deque, Py_ssize_t i)
{
    block *b;
    PyObject *item;
    Py_ssize_t n, index=i;

    if (!valid_index(i, Py_SIZE(deque))) {
        PyErr_SetString(PyExc_IndexError, "deque index out of range");
        return NULL;
    }

    if (i == 0) {
        i = deque->leftindex;
        b = deque->leftblock;
    } else if (i == Py_SIZE(deque) - 1) {
        i = deque->rightindex;
        b = deque->rightblock;
    } else {
        i += deque->leftindex;
        n = (Py_ssize_t)((size_t) i / BLOCKLEN);
        i = (Py_ssize_t)((size_t) i % BLOCKLEN);
        if (index < (Py_SIZE(deque) >> 1)) {
            b = deque->leftblock;
            while (--n >= 0)
                b = b->rightlink;
        } else {
            n = (Py_ssize_t)(
                    ((size_t)(deque->leftindex + Py_SIZE(deque) - 1))
                    / BLOCKLEN - n);
            b = deque->rightblock;
            while (--n >= 0)
                b = b->leftlink;
        }
    }
    item = b->data[i];
    Py_INCREF(item);
    return item;
}

static int
deque_del_item(dequeobject *deque, Py_ssize_t i)
{
    PyObject *item;
    int rv;

    assert (i >= 0 && i < Py_SIZE(deque));
    if (_deque_rotate(deque, -i))
        return -1;
    item = deque_popleft(deque, NULL);
    rv = _deque_rotate(deque, i);
    assert (item != NULL);
    Py_DECREF(item);
    return rv;
}

static int
deque_ass_item(dequeobject *deque, Py_ssize_t i, PyObject *v)
{
    PyObject *old_value;
    block *b;
    Py_ssize_t n, len=Py_SIZE(deque), halflen=(len+1)>>1, index=i;

    if (!valid_index(i, len)) {
        PyErr_SetString(PyExc_IndexError, "deque index out of range");
        return -1;
    }
    if (v == NULL)
        return deque_del_item(deque, i);

    i += deque->leftindex;
    n = (Py_ssize_t)((size_t) i / BLOCKLEN);
    i = (Py_ssize_t)((size_t) i % BLOCKLEN);
    if (index <= halflen) {
        b = deque->leftblock;
        while (--n >= 0)
            b = b->rightlink;
    } else {
        n = (Py_ssize_t)(
                ((size_t)(deque->leftindex + Py_SIZE(deque) - 1))
                / BLOCKLEN - n);
        b = deque->rightblock;
        while (--n >= 0)
            b = b->leftlink;
    }
    Py_INCREF(v);
    old_value = b->data[i];
    b->data[i] = v;
    Py_DECREF(old_value);
    return 0;
}

static void
deque_dealloc(dequeobject *deque)
{
    PyObject_GC_UnTrack(deque);
    if (deque->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) deque);
    if (deque->leftblock != NULL) {
        deque_clear(deque);
        assert(deque->leftblock != NULL);
        freeblock(deque->leftblock);
    }
    deque->leftblock = NULL;
    deque->rightblock = NULL;
    Py_TYPE(deque)->tp_free(deque);
}

static int
deque_traverse(dequeobject *deque, visitproc visit, void *arg)
{
    block *b;
    PyObject *item;
    Py_ssize_t index;
    Py_ssize_t indexlo = deque->leftindex;
    Py_ssize_t indexhigh;

    for (b = deque->leftblock; b != deque->rightblock; b = b->rightlink) {
        for (index = indexlo; index < BLOCKLEN ; index++) {
            item = b->data[index];
            Py_VISIT(item);
        }
        indexlo = 0;
    }
    indexhigh = deque->rightindex;
    for (index = indexlo; index <= indexhigh; index++) {
        item = b->data[index];
        Py_VISIT(item);
    }
    return 0;
}

static PyObject *
deque_reduce(dequeobject *deque, PyObject *Py_UNUSED(ignored))
{
    PyObject *dict, *it;
    _Py_IDENTIFIER(__dict__);

    if (_PyObject_LookupAttrId((PyObject *)deque, &PyId___dict__, &dict) < 0) {
        return NULL;
    }
    if (dict == NULL) {
        dict = Py_None;
        Py_INCREF(dict);
    }

    it = PyObject_GetIter((PyObject *)deque);
    if (it == NULL) {
        Py_DECREF(dict);
        return NULL;
    }

    if (deque->maxlen < 0) {
        return Py_BuildValue("O()NN", Py_TYPE(deque), dict, it);
    }
    else {
        return Py_BuildValue("O(()n)NN", Py_TYPE(deque), deque->maxlen, dict, it);
    }
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static PyObject *
deque_repr(PyObject *deque)
{
    PyObject *aslist, *result;
    int i;

    i = Py_ReprEnter(deque);
    if (i != 0) {
        if (i < 0)
            return NULL;
        return PyUnicode_FromString("[...]");
    }

    aslist = PySequence_List(deque);
    if (aslist == NULL) {
        Py_ReprLeave(deque);
        return NULL;
    }
    if (((dequeobject *)deque)->maxlen >= 0)
        result = PyUnicode_FromFormat("%s(%R, maxlen=%zd)",
                                      _PyType_Name(Py_TYPE(deque)), aslist,
                                      ((dequeobject *)deque)->maxlen);
    else
        result = PyUnicode_FromFormat("%s(%R)",
                                      _PyType_Name(Py_TYPE(deque)), aslist);
    Py_ReprLeave(deque);
    Py_DECREF(aslist);
    return result;
}

static PyObject *
deque_richcompare(PyObject *v, PyObject *w, int op)
{
    PyObject *it1=NULL, *it2=NULL, *x, *y;
    Py_ssize_t vs, ws;
    int b, cmp=-1;

    if (!PyObject_TypeCheck(v, &deque_type) ||
        !PyObject_TypeCheck(w, &deque_type)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    /* Shortcuts */
    vs = Py_SIZE((dequeobject *)v);
    ws = Py_SIZE((dequeobject *)w);
    if (op == Py_EQ) {
        if (v == w)
            Py_RETURN_TRUE;
        if (vs != ws)
            Py_RETURN_FALSE;
    }
    if (op == Py_NE) {
        if (v == w)
            Py_RETURN_FALSE;
        if (vs != ws)
            Py_RETURN_TRUE;
    }

    /* Search for the first index where items are different */
    it1 = PyObject_GetIter(v);
    if (it1 == NULL)
        goto done;
    it2 = PyObject_GetIter(w);
    if (it2 == NULL)
        goto done;
    for (;;) {
        x = PyIter_Next(it1);
        if (x == NULL && PyErr_Occurred())
            goto done;
        y = PyIter_Next(it2);
        if (x == NULL || y == NULL)
            break;
        b = PyObject_RichCompareBool(x, y, Py_EQ);
        if (b == 0) {
            cmp = PyObject_RichCompareBool(x, y, op);
            Py_DECREF(x);
            Py_DECREF(y);
            goto done;
        }
        Py_DECREF(x);
        Py_DECREF(y);
        if (b < 0)
            goto done;
    }
    /* We reached the end of one deque or both */
    Py_XDECREF(x);
    Py_XDECREF(y);
    if (PyErr_Occurred())
        goto done;
    switch (op) {
    case Py_LT: cmp = y != NULL; break;  /* if w was longer */
    case Py_LE: cmp = x == NULL; break;  /* if v was not longer */
    case Py_EQ: cmp = x == y;    break;  /* if we reached the end of both */
    case Py_NE: cmp = x != y;    break;  /* if one deque continues */
    case Py_GT: cmp = x != NULL; break;  /* if v was longer */
    case Py_GE: cmp = y == NULL; break;  /* if w was not longer */
    }

done:
    Py_XDECREF(it1);
    Py_XDECREF(it2);
    if (cmp == 1)
        Py_RETURN_TRUE;
    if (cmp == 0)
        Py_RETURN_FALSE;
    return NULL;
}

static int
deque_init(dequeobject *deque, PyObject *args, PyObject *kwdargs)
{
    PyObject *iterable = NULL;
    PyObject *maxlenobj = NULL;
    Py_ssize_t maxlen = -1;
    char *kwlist[] = {"iterable", "maxlen", 0};

    if (kwdargs == NULL && PyTuple_GET_SIZE(args) <= 2) {
        if (PyTuple_GET_SIZE(args) > 0) {
            iterable = PyTuple_GET_ITEM(args, 0);
        }
        if (PyTuple_GET_SIZE(args) > 1) {
            maxlenobj = PyTuple_GET_ITEM(args, 1);
        }
    } else {
        if (!PyArg_ParseTupleAndKeywords(args, kwdargs, "|OO:deque", kwlist,
                                         &iterable, &maxlenobj))
            return -1;
    }
    if (maxlenobj != NULL && maxlenobj != Py_None) {
        maxlen = PyLong_AsSsize_t(maxlenobj);
        if (maxlen == -1 && PyErr_Occurred())
            return -1;
        if (maxlen < 0) {
            PyErr_SetString(PyExc_ValueError, "maxlen must be non-negative");
            return -1;
        }
    }
    deque->maxlen = maxlen;
    if (Py_SIZE(deque) > 0)
        deque_clear(deque);
    if (iterable != NULL) {
        PyObject *rv = deque_extend(deque, iterable);
        if (rv == NULL)
            return -1;
        Py_DECREF(rv);
    }
    return 0;
}

static PyObject *
deque_sizeof(dequeobject *deque, void *unused)
{
    Py_ssize_t res;
    Py_ssize_t blocks;

    res = _PyObject_SIZE(Py_TYPE(deque));
    blocks = (size_t)(deque->leftindex + Py_SIZE(deque) + BLOCKLEN - 1) / BLOCKLEN;
    assert(deque->leftindex + Py_SIZE(deque) - 1 ==
           (blocks - 1) * BLOCKLEN + deque->rightindex);
    res += blocks * sizeof(block);
    return PyLong_FromSsize_t(res);
}

PyDoc_STRVAR(sizeof_doc,
"D.__sizeof__() -- size of D in memory, in bytes");

static int
deque_bool(dequeobject *deque)
{
    return Py_SIZE(deque) != 0;
}

static PyObject *
deque_get_maxlen(dequeobject *deque, void *Py_UNUSED(ignored))
{
    if (deque->maxlen < 0)
        Py_RETURN_NONE;
    return PyLong_FromSsize_t(deque->maxlen);
}


/* deque object ********************************************************/

static PyGetSetDef deque_getset[] = {
    {"maxlen", (getter)deque_get_maxlen, (setter)NULL,
     "maximum size of a deque or None if unbounded"},
    {0}
};

static PySequenceMethods deque_as_sequence = {
    (lenfunc)deque_len,                 /* sq_length */
    (binaryfunc)deque_concat,           /* sq_concat */
    (ssizeargfunc)deque_repeat,         /* sq_repeat */
    (ssizeargfunc)deque_item,           /* sq_item */
    0,                                  /* sq_slice */
    (ssizeobjargproc)deque_ass_item,    /* sq_ass_item */
    0,                                  /* sq_ass_slice */
    (objobjproc)deque_contains,         /* sq_contains */
    (binaryfunc)deque_inplace_concat,   /* sq_inplace_concat */
    (ssizeargfunc)deque_inplace_repeat, /* sq_inplace_repeat */
};

static PyNumberMethods deque_as_number = {
    0,                                  /* nb_add */
    0,                                  /* nb_subtract */
    0,                                  /* nb_multiply */
    0,                                  /* nb_remainder */
    0,                                  /* nb_divmod */
    0,                                  /* nb_power */
    0,                                  /* nb_negative */
    0,                                  /* nb_positive */
    0,                                  /* nb_absolute */
    (inquiry)deque_bool,                /* nb_bool */
    0,                                  /* nb_invert */
 };

static PyObject *deque_iter(dequeobject *deque);
static PyObject *deque_reviter(dequeobject *deque, PyObject *Py_UNUSED(ignored));
PyDoc_STRVAR(reversed_doc,
    "D.__reversed__() -- return a reverse iterator over the deque");

static PyMethodDef deque_methods[] = {
    {"append",                  (PyCFunction)deque_append,
        METH_O,                  append_doc},
    {"appendleft",              (PyCFunction)deque_appendleft,
        METH_O,                  appendleft_doc},
    {"clear",                   (PyCFunction)deque_clearmethod,
        METH_NOARGS,             clear_doc},
    {"__copy__",                deque_copy,
        METH_NOARGS,             copy_doc},
    {"copy",                    deque_copy,
        METH_NOARGS,             copy_doc},
    {"count",                   (PyCFunction)deque_count,
        METH_O,                  count_doc},
    {"extend",                  (PyCFunction)deque_extend,
        METH_O,                  extend_doc},
    {"extendleft",              (PyCFunction)deque_extendleft,
        METH_O,                  extendleft_doc},
    {"index",                   (PyCFunction)(void(*)(void))deque_index,
        METH_FASTCALL,            index_doc},
    {"insert",                  (PyCFunction)(void(*)(void))deque_insert,
        METH_FASTCALL,            insert_doc},
    {"pop",                     (PyCFunction)deque_pop,
        METH_NOARGS,             pop_doc},
    {"popleft",                 (PyCFunction)deque_popleft,
        METH_NOARGS,             popleft_doc},
    {"__reduce__",              (PyCFunction)deque_reduce,
        METH_NOARGS,             reduce_doc},
    {"remove",                  (PyCFunction)deque_remove,
        METH_O,                  remove_doc},
    {"__reversed__",            (PyCFunction)deque_reviter,
        METH_NOARGS,             reversed_doc},
    {"reverse",                 (PyCFunction)deque_reverse,
        METH_NOARGS,             reverse_doc},
    {"rotate",                  (PyCFunction)(void(*)(void))deque_rotate,
        METH_FASTCALL,            rotate_doc},
    {"__sizeof__",              (PyCFunction)deque_sizeof,
        METH_NOARGS,             sizeof_doc},
    {NULL,              NULL}   /* sentinel */
};

PyDoc_STRVAR(deque_doc,
"deque([iterable[, maxlen]]) --> deque object\n\
\n\
A list-like sequence optimized for data accesses near its endpoints.");

static PyTypeObject deque_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "collections.deque",                /* tp_name */
    sizeof(dequeobject),                /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)deque_dealloc,          /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    deque_repr,                         /* tp_repr */
    &deque_as_number,                   /* tp_as_number */
    &deque_as_sequence,                 /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    PyObject_HashNotImplemented,        /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
                                        /* tp_flags */
    deque_doc,                          /* tp_doc */
    (traverseproc)deque_traverse,       /* tp_traverse */
    (inquiry)deque_clear,               /* tp_clear */
    (richcmpfunc)deque_richcompare,     /* tp_richcompare */
    offsetof(dequeobject, weakreflist), /* tp_weaklistoffset*/
    (getiterfunc)deque_iter,            /* tp_iter */
    0,                                  /* tp_iternext */
    deque_methods,                      /* tp_methods */
    0,                                  /* tp_members */
    deque_getset,                       /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)deque_init,               /* tp_init */
    PyType_GenericAlloc,                /* tp_alloc */
    deque_new,                          /* tp_new */
    PyObject_GC_Del,                    /* tp_free */
};

/*********************** Deque Iterator **************************/

typedef struct {
    PyObject_HEAD
    block *b;
    Py_ssize_t index;
    dequeobject *deque;
    size_t state;          /* state when the iterator is created */
    Py_ssize_t counter;    /* number of items remaining for iteration */
} dequeiterobject;

static PyTypeObject dequeiter_type;

static PyObject *
deque_iter(dequeobject *deque)
{
    dequeiterobject *it;

    it = PyObject_GC_New(dequeiterobject, &dequeiter_type);
    if (it == NULL)
        return NULL;
    it->b = deque->leftblock;
    it->index = deque->leftindex;
    Py_INCREF(deque);
    it->deque = deque;
    it->state = deque->state;
    it->counter = Py_SIZE(deque);
    PyObject_GC_Track(it);
    return (PyObject *)it;
}

static int
dequeiter_traverse(dequeiterobject *dio, visitproc visit, void *arg)
{
    Py_VISIT(dio->deque);
    return 0;
}

static void
dequeiter_dealloc(dequeiterobject *dio)
{
    /* bpo-31095: UnTrack is needed before calling any callbacks */
    PyObject_GC_UnTrack(dio);
    Py_XDECREF(dio->deque);
    PyObject_GC_Del(dio);
}

static PyObject *
dequeiter_next(dequeiterobject *it)
{
    PyObject *item;

    if (it->deque->state != it->state) {
        it->counter = 0;
        PyErr_SetString(PyExc_RuntimeError,
                        "deque mutated during iteration");
        return NULL;
    }
    if (it->counter == 0)
        return NULL;
    assert (!(it->b == it->deque->rightblock &&
              it->index > it->deque->rightindex));

    item = it->b->data[it->index];
    it->index++;
    it->counter--;
    if (it->index == BLOCKLEN && it->counter > 0) {
        CHECK_NOT_END(it->b->rightlink);
        it->b = it->b->rightlink;
        it->index = 0;
    }
    Py_INCREF(item);
    return item;
}

static PyObject *
dequeiter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Py_ssize_t i, index=0;
    PyObject *deque;
    dequeiterobject *it;
    if (!PyArg_ParseTuple(args, "O!|n", &deque_type, &deque, &index))
        return NULL;
    assert(type == &dequeiter_type);

    it = (dequeiterobject*)deque_iter((dequeobject *)deque);
    if (!it)
        return NULL;
    /* consume items from the queue */
    for(i=0; i<index; i++) {
        PyObject *item = dequeiter_next(it);
        if (item) {
            Py_DECREF(item);
        } else {
            if (it->counter) {
                Py_DECREF(it);
                return NULL;
            } else
                break;
        }
    }
    return (PyObject*)it;
}

static PyObject *
dequeiter_len(dequeiterobject *it, PyObject *Py_UNUSED(ignored))
{
    return PyLong_FromSsize_t(it->counter);
}

PyDoc_STRVAR(length_hint_doc, "Private method returning an estimate of len(list(it)).");

static PyObject *
dequeiter_reduce(dequeiterobject *it, PyObject *Py_UNUSED(ignored))
{
    return Py_BuildValue("O(On)", Py_TYPE(it), it->deque, Py_SIZE(it->deque) - it->counter);
}

static PyMethodDef dequeiter_methods[] = {
    {"__length_hint__", (PyCFunction)dequeiter_len, METH_NOARGS, length_hint_doc},
    {"__reduce__", (PyCFunction)dequeiter_reduce, METH_NOARGS, reduce_doc},
    {NULL,              NULL}           /* sentinel */
};

static PyTypeObject dequeiter_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_collections._deque_iterator",             /* tp_name */
    sizeof(dequeiterobject),                    /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)dequeiter_dealloc,              /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,    /* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)dequeiter_traverse,           /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)dequeiter_next,               /* tp_iternext */
    dequeiter_methods,                          /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    dequeiter_new,                              /* tp_new */
    0,
};

/*********************** Deque Reverse Iterator **************************/

static PyTypeObject dequereviter_type;

static PyObject *
deque_reviter(dequeobject *deque, PyObject *Py_UNUSED(ignored))
{
    dequeiterobject *it;

    it = PyObject_GC_New(dequeiterobject, &dequereviter_type);
    if (it == NULL)
        return NULL;
    it->b = deque->rightblock;
    it->index = deque->rightindex;
    Py_INCREF(deque);
    it->deque = deque;
    it->state = deque->state;
    it->counter = Py_SIZE(deque);
    PyObject_GC_Track(it);
    return (PyObject *)it;
}

static PyObject *
dequereviter_next(dequeiterobject *it)
{
    PyObject *item;
    if (it->counter == 0)
        return NULL;

    if (it->deque->state != it->state) {
        it->counter = 0;
        PyErr_SetString(PyExc_RuntimeError,
                        "deque mutated during iteration");
        return NULL;
    }
    assert (!(it->b == it->deque->leftblock &&
              it->index < it->deque->leftindex));

    item = it->b->data[it->index];
    it->index--;
    it->counter--;
    if (it->index < 0 && it->counter > 0) {
        CHECK_NOT_END(it->b->leftlink);
        it->b = it->b->leftlink;
        it->index = BLOCKLEN - 1;
    }
    Py_INCREF(item);
    return item;
}

static PyObject *
dequereviter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Py_ssize_t i, index=0;
    PyObject *deque;
    dequeiterobject *it;
    if (!PyArg_ParseTuple(args, "O!|n", &deque_type, &deque, &index))
        return NULL;
    assert(type == &dequereviter_type);

    it = (dequeiterobject*)deque_reviter((dequeobject *)deque, NULL);
    if (!it)
        return NULL;
    /* consume items from the queue */
    for(i=0; i<index; i++) {
        PyObject *item = dequereviter_next(it);
        if (item) {
            Py_DECREF(item);
        } else {
            if (it->counter) {
                Py_DECREF(it);
                return NULL;
            } else
                break;
        }
    }
    return (PyObject*)it;
}

static PyTypeObject dequereviter_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_collections._deque_reverse_iterator",     /* tp_name */
    sizeof(dequeiterobject),                    /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)dequeiter_dealloc,              /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,    /* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)dequeiter_traverse,           /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)dequereviter_next,            /* tp_iternext */
    dequeiter_methods,                          /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    dequereviter_new,                           /* tp_new */
    0,
};

/* defaultdict type *********************************************************/

typedef struct {
    PyDictObject dict;
    PyObject *default_factory;
} defdictobject;

static PyTypeObject defdict_type; /* Forward */

PyDoc_STRVAR(defdict_missing_doc,
"__missing__(key) # Called by __getitem__ for missing key; pseudo-code:\n\
  if self.default_factory is None: raise KeyError((key,))\n\
  self[key] = value = self.default_factory()\n\
  return value\n\
");

static PyObject *
defdict_missing(defdictobject *dd, PyObject *key)
{
    PyObject *factory = dd->default_factory;
    PyObject *value;
    if (factory == NULL || factory == Py_None) {
        /* XXX Call dict.__missing__(key) */
        PyObject *tup;
        tup = PyTuple_Pack(1, key);
        if (!tup) return NULL;
        PyErr_SetObject(PyExc_KeyError, tup);
        Py_DECREF(tup);
        return NULL;
    }
    value = _PyObject_CallNoArg(factory);
    if (value == NULL)
        return value;
    if (PyObject_SetItem((PyObject *)dd, key, value) < 0) {
        Py_DECREF(value);
        return NULL;
    }
    return value;
}

PyDoc_STRVAR(defdict_copy_doc, "D.copy() -> a shallow copy of D.");

static PyObject *
defdict_copy(defdictobject *dd, PyObject *Py_UNUSED(ignored))
{
    /* This calls the object's class.  That only works for subclasses
       whose class constructor has the same signature.  Subclasses that
       define a different constructor signature must override copy().
    */

    if (dd->default_factory == NULL)
        return PyObject_CallFunctionObjArgs((PyObject*)Py_TYPE(dd), Py_None, dd, NULL);
    return PyObject_CallFunctionObjArgs((PyObject*)Py_TYPE(dd),
                                        dd->default_factory, dd, NULL);
}

static PyObject *
defdict_reduce(defdictobject *dd, PyObject *Py_UNUSED(ignored))
{
    /* __reduce__ must return a 5-tuple as follows:

       - factory function
       - tuple of args for the factory function
       - additional state (here None)
       - sequence iterator (here None)
       - dictionary iterator (yielding successive (key, value) pairs

       This API is used by pickle.py and copy.py.

       For this to be useful with pickle.py, the default_factory
       must be picklable; e.g., None, a built-in, or a global
       function in a module or package.

       Both shallow and deep copying are supported, but for deep
       copying, the default_factory must be deep-copyable; e.g. None,
       or a built-in (functions are not copyable at this time).

       This only works for subclasses as long as their constructor
       signature is compatible; the first argument must be the
       optional default_factory, defaulting to None.
    */
    PyObject *args;
    PyObject *items;
    PyObject *iter;
    PyObject *result;
    _Py_IDENTIFIER(items);

    if (dd->default_factory == NULL || dd->default_factory == Py_None)
        args = PyTuple_New(0);
    else
        args = PyTuple_Pack(1, dd->default_factory);
    if (args == NULL)
        return NULL;
    items = _PyObject_CallMethodIdNoArgs((PyObject *)dd, &PyId_items);
    if (items == NULL) {
        Py_DECREF(args);
        return NULL;
    }
    iter = PyObject_GetIter(items);
    if (iter == NULL) {
        Py_DECREF(items);
        Py_DECREF(args);
        return NULL;
    }
    result = PyTuple_Pack(5, Py_TYPE(dd), args,
                          Py_None, Py_None, iter);
    Py_DECREF(iter);
    Py_DECREF(items);
    Py_DECREF(args);
    return result;
}

static PyMethodDef defdict_methods[] = {
    {"__missing__", (PyCFunction)defdict_missing, METH_O,
     defdict_missing_doc},
    {"copy", (PyCFunction)defdict_copy, METH_NOARGS,
     defdict_copy_doc},
    {"__copy__", (PyCFunction)defdict_copy, METH_NOARGS,
     defdict_copy_doc},
    {"__reduce__", (PyCFunction)defdict_reduce, METH_NOARGS,
     reduce_doc},
    {NULL}
};

static PyMemberDef defdict_members[] = {
    {"default_factory", T_OBJECT,
     offsetof(defdictobject, default_factory), 0,
     PyDoc_STR("Factory for default value called by __missing__().")},
    {NULL}
};

static void
defdict_dealloc(defdictobject *dd)
{
    /* bpo-31095: UnTrack is needed before calling any callbacks */
    PyObject_GC_UnTrack(dd);
    Py_CLEAR(dd->default_factory);
    PyDict_Type.tp_dealloc((PyObject *)dd);
}

static PyObject *
defdict_repr(defdictobject *dd)
{
    PyObject *baserepr;
    PyObject *defrepr;
    PyObject *result;
    baserepr = PyDict_Type.tp_repr((PyObject *)dd);
    if (baserepr == NULL)
        return NULL;
    if (dd->default_factory == NULL)
        defrepr = PyUnicode_FromString("None");
    else
    {
        int status = Py_ReprEnter(dd->default_factory);
        if (status != 0) {
            if (status < 0) {
                Py_DECREF(baserepr);
                return NULL;
            }
            defrepr = PyUnicode_FromString("...");
        }
        else
            defrepr = PyObject_Repr(dd->default_factory);
        Py_ReprLeave(dd->default_factory);
    }
    if (defrepr == NULL) {
        Py_DECREF(baserepr);
        return NULL;
    }
    result = PyUnicode_FromFormat("%s(%U, %U)",
                                  _PyType_Name(Py_TYPE(dd)),
                                  defrepr, baserepr);
    Py_DECREF(defrepr);
    Py_DECREF(baserepr);
    return result;
}

static int
defdict_traverse(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(((defdictobject *)self)->default_factory);
    return PyDict_Type.tp_traverse(self, visit, arg);
}

static int
defdict_tp_clear(defdictobject *dd)
{
    Py_CLEAR(dd->default_factory);
    return PyDict_Type.tp_clear((PyObject *)dd);
}

static int
defdict_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    defdictobject *dd = (defdictobject *)self;
    PyObject *olddefault = dd->default_factory;
    PyObject *newdefault = NULL;
    PyObject *newargs;
    int result;
    if (args == NULL || !PyTuple_Check(args))
        newargs = PyTuple_New(0);
    else {
        Py_ssize_t n = PyTuple_GET_SIZE(args);
        if (n > 0) {
            newdefault = PyTuple_GET_ITEM(args, 0);
            if (!PyCallable_Check(newdefault) && newdefault != Py_None) {
                PyErr_SetString(PyExc_TypeError,
                    "first argument must be callable or None");
                return -1;
            }
        }
        newargs = PySequence_GetSlice(args, 1, n);
    }
    if (newargs == NULL)
        return -1;
    Py_XINCREF(newdefault);
    dd->default_factory = newdefault;
    result = PyDict_Type.tp_init(self, newargs, kwds);
    Py_DECREF(newargs);
    Py_XDECREF(olddefault);
    return result;
}

PyDoc_STRVAR(defdict_doc,
"defaultdict(default_factory[, ...]) --> dict with default factory\n\
\n\
The default factory is called without arguments to produce\n\
a new value when a key is not present, in __getitem__ only.\n\
A defaultdict compares equal to a dict with the same items.\n\
All remaining arguments are treated the same as if they were\n\
passed to the dict constructor, including keyword arguments.\n\
");

/* See comment in xxsubtype.c */
#define DEFERRED_ADDRESS(ADDR) 0

static PyTypeObject defdict_type = {
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
    "collections.defaultdict",          /* tp_name */
    sizeof(defdictobject),              /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)defdict_dealloc,        /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    (reprfunc)defdict_repr,             /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
                                    /* tp_flags */
    defdict_doc,                        /* tp_doc */
    defdict_traverse,                   /* tp_traverse */
    (inquiry)defdict_tp_clear,          /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset*/
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    defdict_methods,                    /* tp_methods */
    defdict_members,                    /* tp_members */
    0,                                  /* tp_getset */
    DEFERRED_ADDRESS(&PyDict_Type),     /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    defdict_init,                       /* tp_init */
    PyType_GenericAlloc,                /* tp_alloc */
    0,                                  /* tp_new */
    PyObject_GC_Del,                    /* tp_free */
};

/* helper function for Counter  *********************************************/

/*[clinic input]
_collections._count_elements

    mapping: object
    iterable: object
    /

Count elements in the iterable, updating the mapping
[clinic start generated code]*/

static PyObject *
_collections__count_elements_impl(PyObject *module, PyObject *mapping,
                                  PyObject *iterable)
/*[clinic end generated code: output=7e0c1789636b3d8f input=e79fad04534a0b45]*/
{
    _Py_IDENTIFIER(get);
    _Py_IDENTIFIER(__setitem__);
    PyObject *it, *oldval;
    PyObject *newval = NULL;
    PyObject *key = NULL;
    PyObject *bound_get = NULL;
    PyObject *mapping_get;
    PyObject *dict_get;
    PyObject *mapping_setitem;
    PyObject *dict_setitem;

    it = PyObject_GetIter(iterable);
    if (it == NULL)
        return NULL;

    /* Only take the fast path when get() and __setitem__()
     * have not been overridden.
     */
    mapping_get = _PyType_LookupId(Py_TYPE(mapping), &PyId_get);
    dict_get = _PyType_LookupId(&PyDict_Type, &PyId_get);
    mapping_setitem = _PyType_LookupId(Py_TYPE(mapping), &PyId___setitem__);
    dict_setitem = _PyType_LookupId(&PyDict_Type, &PyId___setitem__);

    if (mapping_get != NULL && mapping_get == dict_get &&
        mapping_setitem != NULL && mapping_setitem == dict_setitem &&
        PyDict_Check(mapping))
    {
        while (1) {
            /* Fast path advantages:
                   1. Eliminate double hashing
                      (by re-using the same hash for both the get and set)
                   2. Avoid argument overhead of PyObject_CallFunctionObjArgs
                      (argument tuple creation and parsing)
                   3. Avoid indirection through a bound method object
                      (creates another argument tuple)
                   4. Avoid initial increment from zero
                      (reuse an existing one-object instead)
            */
            Py_hash_t hash;

            key = PyIter_Next(it);
            if (key == NULL)
                break;

            if (!PyUnicode_CheckExact(key) ||
                (hash = ((PyASCIIObject *) key)->hash) == -1)
            {
                hash = PyObject_Hash(key);
                if (hash == -1)
                    goto done;
            }

            oldval = _PyDict_GetItem_KnownHash(mapping, key, hash);
            if (oldval == NULL) {
                if (PyErr_Occurred())
                    goto done;
                if (_PyDict_SetItem_KnownHash(mapping, key, _PyLong_One, hash) < 0)
                    goto done;
            } else {
                newval = PyNumber_Add(oldval, _PyLong_One);
                if (newval == NULL)
                    goto done;
                if (_PyDict_SetItem_KnownHash(mapping, key, newval, hash) < 0)
                    goto done;
                Py_CLEAR(newval);
            }
            Py_DECREF(key);
        }
    } else {
        bound_get = _PyObject_GetAttrId(mapping, &PyId_get);
        if (bound_get == NULL)
            goto done;

        while (1) {
            key = PyIter_Next(it);
            if (key == NULL)
                break;
            oldval = PyObject_CallFunctionObjArgs(bound_get, key, _PyLong_Zero, NULL);
            if (oldval == NULL)
                break;
            newval = PyNumber_Add(oldval, _PyLong_One);
            Py_DECREF(oldval);
            if (newval == NULL)
                break;
            if (PyObject_SetItem(mapping, key, newval) < 0)
                break;
            Py_CLEAR(newval);
            Py_DECREF(key);
        }
    }

done:
    Py_DECREF(it);
    Py_XDECREF(key);
    Py_XDECREF(newval);
    Py_XDECREF(bound_get);
    if (PyErr_Occurred())
        return NULL;
    Py_RETURN_NONE;
}

/* Helper function for namedtuple() ************************************/

typedef struct {
    PyObject_HEAD
    Py_ssize_t index;
    PyObject* doc;
} _tuplegetterobject;

/*[clinic input]
@classmethod
_tuplegetter.__new__ as tuplegetter_new

    index: Py_ssize_t
    doc: object
    /
[clinic start generated code]*/

static PyObject *
tuplegetter_new_impl(PyTypeObject *type, Py_ssize_t index, PyObject *doc)
/*[clinic end generated code: output=014be444ad80263f input=87c576a5bdbc0bbb]*/
{
    _tuplegetterobject* self;
    self = (_tuplegetterobject *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }
    self->index = index;
    Py_INCREF(doc);
    self->doc = doc;
    return (PyObject *)self;
}

static PyObject *
tuplegetter_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
    Py_ssize_t index = ((_tuplegetterobject*)self)->index;
    PyObject *result;

    if (obj == NULL) {
        Py_INCREF(self);
        return self;
    }
    if (!PyTuple_Check(obj)) {
        if (obj == Py_None) {
            Py_INCREF(self);
            return self;
        }
        PyErr_Format(PyExc_TypeError,
                     "descriptor for index '%zd' for tuple subclasses "
                     "doesn't apply to '%s' object",
                     index,
                     Py_TYPE(obj)->tp_name);
        return NULL;
    }

    if (!valid_index(index, PyTuple_GET_SIZE(obj))) {
        PyErr_SetString(PyExc_IndexError, "tuple index out of range");
        return NULL;
    }

    result = PyTuple_GET_ITEM(obj, index);
    Py_INCREF(result);
    return result;
}

static int
tuplegetter_descr_set(PyObject *self, PyObject *obj, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_AttributeError, "can't delete attribute");
    } else {
        PyErr_SetString(PyExc_AttributeError, "can't set attribute");
    }
    return -1;
}

static int
tuplegetter_traverse(PyObject *self, visitproc visit, void *arg)
{
    _tuplegetterobject *tuplegetter = (_tuplegetterobject *)self;
    Py_VISIT(tuplegetter->doc);
    return 0;
}

static int
tuplegetter_clear(PyObject *self)
{
    _tuplegetterobject *tuplegetter = (_tuplegetterobject *)self;
    Py_CLEAR(tuplegetter->doc);
    return 0;
}

static void
tuplegetter_dealloc(_tuplegetterobject *self)
{
    PyObject_GC_UnTrack(self);
    tuplegetter_clear((PyObject*)self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject*
tuplegetter_reduce(_tuplegetterobject *self, PyObject *Py_UNUSED(ignored))
{
    return Py_BuildValue("(O(nO))", (PyObject*) Py_TYPE(self), self->index, self->doc);
}


static PyMemberDef tuplegetter_members[] = {
    {"__doc__",  T_OBJECT, offsetof(_tuplegetterobject, doc), 0},
    {0}
};

static PyMethodDef tuplegetter_methods[] = {
    {"__reduce__", (PyCFunction)tuplegetter_reduce, METH_NOARGS, NULL},
    {NULL},
};

static PyTypeObject tuplegetter_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_collections._tuplegetter",                /* tp_name */
    sizeof(_tuplegetterobject),                 /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)tuplegetter_dealloc,            /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,    /* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)tuplegetter_traverse,         /* tp_traverse */
    (inquiry)tuplegetter_clear,                 /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    tuplegetter_methods,                        /* tp_methods */
    tuplegetter_members,                        /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    tuplegetter_descr_get,                      /* tp_descr_get */
    tuplegetter_descr_set,                      /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    tuplegetter_new,                            /* tp_new */
    0,
};


/* module level code ********************************************************/

PyDoc_STRVAR(module_doc,
"High performance data structures.\n\
- deque:        ordered collection accessible from endpoints only\n\
- defaultdict:  dict subclass with a default value factory\n\
");

static struct PyMethodDef module_functions[] = {
    _COLLECTIONS__COUNT_ELEMENTS_METHODDEF
    {NULL,       NULL}          /* sentinel */
};

static struct PyModuleDef _collectionsmodule = {
    PyModuleDef_HEAD_INIT,
    "_collections",
    module_doc,
    -1,
    module_functions,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__collections(void)
{
    PyObject *m;

    m = PyModule_Create(&_collectionsmodule);
    if (m == NULL)
        return NULL;

    if (PyType_Ready(&deque_type) < 0)
        return NULL;
    Py_INCREF(&deque_type);
    PyModule_AddObject(m, "deque", (PyObject *)&deque_type);

    defdict_type.tp_base = &PyDict_Type;
    if (PyType_Ready(&defdict_type) < 0)
        return NULL;
    Py_INCREF(&defdict_type);
    PyModule_AddObject(m, "defaultdict", (PyObject *)&defdict_type);

    Py_INCREF(&PyODict_Type);
    PyModule_AddObject(m, "OrderedDict", (PyObject *)&PyODict_Type);

    if (PyType_Ready(&dequeiter_type) < 0)
        return NULL;
    Py_INCREF(&dequeiter_type);
    PyModule_AddObject(m, "_deque_iterator", (PyObject *)&dequeiter_type);

    if (PyType_Ready(&dequereviter_type) < 0)
        return NULL;
    Py_INCREF(&dequereviter_type);
    PyModule_AddObject(m, "_deque_reverse_iterator", (PyObject *)&dequereviter_type);

    if (PyType_Ready(&tuplegetter_type) < 0)
        return NULL;
    Py_INCREF(&tuplegetter_type);
    PyModule_AddObject(m, "_tuplegetter", (PyObject *)&tuplegetter_type);

    return m;
}
