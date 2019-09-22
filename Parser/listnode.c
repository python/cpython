
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

    interp->parser_data.listnode_data.level = 0;
    interp->parser_data.listnode_data.atbol = 1;
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
            interp->parser_data.listnode_data.level++;
            break;
        case DEDENT:
            interp->parser_data.listnode_data.level--;
            break;
        default:
            if (interp->parser_data.listnode_data.atbol) {
                int i;
                for (i = 0; i < interp->parser_data.listnode_data.level; ++i)
                    fprintf(fp, "\t");
                interp->parser_data.listnode_data.atbol = 0;
            }
            if (TYPE(n) == NEWLINE) {
                if (STR(n) != NULL)
                    fprintf(fp, "%s", STR(n));
                fprintf(fp, "\n");
                interp->parser_data.listnode_data.atbol = 1;
            }
            else
                fprintf(fp, "%s ", STR(n));
            break;
        }
    }
    else
        fprintf(fp, "? ");
}
