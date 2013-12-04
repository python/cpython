/* Parse tree node implementation */

#include "Python.h"
#include "node.h"
#include "errcode.h"

node *
PyNode_New(int type)
{
    node *n = (node *) PyObject_MALLOC(1 * sizeof(node));
    if (n == NULL)
        return NULL;
    n->n_type = type;
    n->n_str = NULL;
    n->n_lineno = 0;
    n->n_nchildren = 0;
    n->n_child = NULL;
    return n;
}

/* See comments at XXXROUNDUP below.  Returns -1 on overflow. */
static int
fancy_roundup(int n)
{
    /* Round up to the closest power of 2 >= n. */
    int result = 256;
    assert(n > 128);
    while (result < n) {
        result <<= 1;
        if (result <= 0)
            return -1;
    }
    return result;
}

/* A gimmick to make massive numbers of reallocs quicker.  The result is
 * a number >= the input.  In PyNode_AddChild, it's used like so, when
 * we're about to add child number current_size + 1:
 *
 *     if XXXROUNDUP(current_size) < XXXROUNDUP(current_size + 1):
 *         allocate space for XXXROUNDUP(current_size + 1) total children
 *     else:
 *         we already have enough space
 *
 * Since a node starts out empty, we must have
 *
 *     XXXROUNDUP(0) < XXXROUNDUP(1)
 *
 * so that we allocate space for the first child.  One-child nodes are very
 * common (presumably that would change if we used a more abstract form
 * of syntax tree), so to avoid wasting memory it's desirable that
 * XXXROUNDUP(1) == 1.  That in turn forces XXXROUNDUP(0) == 0.
 *
 * Else for 2 <= n <= 128, we round up to the closest multiple of 4.  Why 4?
 * Rounding up to a multiple of an exact power of 2 is very efficient, and
 * most nodes with more than one child have <= 4 kids.
 *
 * Else we call fancy_roundup() to grow proportionately to n.  We've got an
 * extreme case then (like test_longexp.py), and on many platforms doing
 * anything less than proportional growth leads to exorbitant runtime
 * (e.g., MacPython), or extreme fragmentation of user address space (e.g.,
 * Win98).
 *
 * In a run of compileall across the 2.3a0 Lib directory, Andrew MacIntyre
 * reported that, with this scheme, 89% of PyObject_REALLOC calls in
 * PyNode_AddChild passed 1 for the size, and 9% passed 4.  So this usually
 * wastes very little memory, but is very effective at sidestepping
 * platform-realloc disasters on vulnerable platforms.
 *
 * Note that this would be straightforward if a node stored its current
 * capacity.  The code is tricky to avoid that.
 */
#define XXXROUNDUP(n) ((n) <= 1 ? (n) :                 \
               (n) <= 128 ? _Py_SIZE_ROUND_UP((n), 4) : \
               fancy_roundup(n))


int
PyNode_AddChild(node *n1, int type, char *str, int lineno, int col_offset)
{
    const int nch = n1->n_nchildren;
    int current_capacity;
    int required_capacity;
    node *n;

    if (nch == INT_MAX || nch < 0)
        return E_OVERFLOW;

    current_capacity = XXXROUNDUP(nch);
    required_capacity = XXXROUNDUP(nch + 1);
    if (current_capacity < 0 || required_capacity < 0)
        return E_OVERFLOW;
    if (current_capacity < required_capacity) {
        if ((size_t)required_capacity > PY_SIZE_MAX / sizeof(node)) {
            return E_NOMEM;
        }
        n = n1->n_child;
        n = (node *) PyObject_REALLOC(n,
                                      required_capacity * sizeof(node));
        if (n == NULL)
            return E_NOMEM;
        n1->n_child = n;
    }

    n = &n1->n_child[n1->n_nchildren++];
    n->n_type = type;
    n->n_str = str;
    n->n_lineno = lineno;
    n->n_col_offset = col_offset;
    n->n_nchildren = 0;
    n->n_child = NULL;
    return 0;
}

/* Forward */
static void freechildren(node *);
static Py_ssize_t sizeofchildren(node *n);


void
PyNode_Free(node *n)
{
    if (n != NULL) {
        freechildren(n);
        PyObject_FREE(n);
    }
}

Py_ssize_t
_PyNode_SizeOf(node *n)
{
    Py_ssize_t res = 0;

    if (n != NULL)
        res = sizeof(node) + sizeofchildren(n);
    return res;
}

static void
freechildren(node *n)
{
    int i;
    for (i = NCH(n); --i >= 0; )
        freechildren(CHILD(n, i));
    if (n->n_child != NULL)
        PyObject_FREE(n->n_child);
    if (STR(n) != NULL)
        PyObject_FREE(STR(n));
}

static Py_ssize_t
sizeofchildren(node *n)
{
    Py_ssize_t res = 0;
    int i;
    for (i = NCH(n); --i >= 0; )
        res += sizeofchildren(CHILD(n, i));
    if (n->n_child != NULL)
        /* allocated size of n->n_child array */
        res += XXXROUNDUP(NCH(n)) * sizeof(node);
    if (STR(n) != NULL)
        res += strlen(STR(n)) + 1;
    return res;
}
