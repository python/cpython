
/* List a node on a file */

#include "Python.h"
#include "pycore_pystate.h"
#include "token.h"
#include "node.h"

/* Forward */
static void list1node(FILE *, node *);
static void listnode(FILE *, node *);

void
PyNode_ListTree(node *n)
{
    listnode(stdout, n);
}

static void
listnode(FILE *fp, node *n)
{
    PyInterpreterState *interp = _PyInterpreterState_GET_UNSAFE();

    interp->parser.listnode.level = 0;
    interp->parser.listnode.atbol = 1;
    list1node(fp, n);
}

static void
list1node(FILE *fp, node *n)
{
    PyInterpreterState *interp;

    if (n == NULL)
        return;
    if (ISNONTERMINAL(TYPE(n))) {
        int i;
        for (i = 0; i < NCH(n); i++)
            list1node(fp, CHILD(n, i));
    }
    else if (ISTERMINAL(TYPE(n))) {
        interp = _PyInterpreterState_GET_UNSAFE();
        switch (TYPE(n)) {
        case INDENT:
            interp->parser.listnode.level++;
            break;
        case DEDENT:
            interp->parser.listnode.level--;
            break;
        default:
            if (interp->parser.listnode.atbol) {
                int i;
                for (i = 0; i < interp->parser.listnode.level; ++i)
                    fprintf(fp, "\t");
                interp->parser.listnode.atbol = 0;
            }
            if (TYPE(n) == NEWLINE) {
                if (STR(n) != NULL)
                    fprintf(fp, "%s", STR(n));
                fprintf(fp, "\n");
                interp->parser.listnode.atbol = 1;
            }
            else
                fprintf(fp, "%s ", STR(n));
            break;
        }
    }
    else
        fprintf(fp, "? ");
}
