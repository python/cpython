#include <Python.h>
#include <errcode.h>
#include "../tokenizer.h"

#include "pegen.h"
#include "parse_string.h"

#include "vm.h"
#include "vmparse.h"  // Generated parser tables

#undef D
#define DEBUG 0

#if DEBUG
#define D(x) x
#else
#define D(x)
#endif

#define MAXFRAMES 100

typedef struct _stack {
    Parser *p;
    int top;
    Frame frames[MAXFRAMES];
} Stack;

static inline Frame *
push_frame(Stack *stack, Rule *rule)
{
    D(printf("               push %s\n", rule->name));
    assert(stack->top < MAXFRAMES);
    Frame *f = &stack->frames[stack->top++];
    f->rule = rule;
    f->mark = stack->p->mark;
    f->ialt = 0;
    f->iop = 0;
    f->cut = 0;
    f->ncollected = 0;
    f->capacity = 0;
    f->collection = NULL;
    f->ival = 0;
    return f;
}

static inline Frame *
pop_frame(Stack *stack, void *v)
{
    assert(stack->top > 1);
    Frame *f = &stack->frames[--stack->top];  // Frame being popped
    if (f->collection) {
        PyMem_Free(f->collection);
    }
    // TODO: Selective memoization
    _PyPegen_insert_memo(stack->p, f->mark, f->rule->type + 1000, v);
    f = &stack->frames[stack->top - 1];  // New top of stack
    D(printf("               pop %s\n", f->rule->name));
    return f;
}

static inline asdl_seq *
make_asdl_seq(Parser *p, void *collection[], int ncollected)
{
    asdl_seq *seq = _Py_asdl_seq_new(ncollected, p->arena);
    if (!seq) {
        return NULL;
    }
    for (int i = 0; i < ncollected; i++) {
        asdl_seq_SET(seq, i, collection[i]);
    }
    return seq;
}

static void *
run_vm(Parser *p, Rule rules[], int root)
{
    Stack stack = {p, 0, {{0}}};
    Frame *f = push_frame(&stack, &rules[root]);
    void *v;
    int oparg;
    int opc;

 top:
    opc = f->rule->opcodes[f->iop];
    if (DEBUG) {
        if (p->mark == p->fill)
            _PyPegen_fill_token(p);
        for (int i = 0; i < stack.top; i++) printf(" ");
        printf("Rule: %s; ialt=%d; iop=%d; op=%s; arg=%d, mark=%d; token=%d; p^='%s'\n",
               f->rule->name, f->ialt, f->iop, opcode_names[opc],
               opc >= OP_TOKEN ? f->rule->opcodes[f->iop + 1] : -1,
               p->mark, p->tokens[p->mark]->type,
               p->fill > p-> mark ? PyBytes_AsString(p->tokens[p->mark]->bytes) : "<UNSEEN>");
    }
    f->iop++;
    switch (opc) {
    case OP_NOOP:
        goto top;
    case OP_CUT:
        f->cut = 1;
        goto top;
    case OP_OPTIONAL:
        goto top;

    case OP_NAME:
        v = _PyPegen_name_token(p);
        break;
    case OP_NUMBER:
        v = _PyPegen_number_token(p);
        break;
    case OP_STRING:
        v = _PyPegen_string_token(p);
        break;

    case OP_LOOP_ITERATE:
        f->mark = p->mark;
        assert(f->ival == 1 || f->ival == 2);
        v = f->vals[0];
        assert(v);
        if (f->ncollected >= f->capacity) {
            f->capacity = (f->ncollected + 1) * 2;  // 2, 6, 14, 30, 62, ... (2**i - 2)
            f->collection = PyMem_Realloc(f->collection, (f->capacity) * sizeof(void *));
            if (!f->collection) {
                return PyErr_NoMemory();
            }
        }
        f->collection[f->ncollected++] = v;
        f->iop = f->rule->alts[f->ialt];
        f->ival = 0;
        goto top;
    case OP_LOOP_COLLECT_DELIMITED:
        /* Collect one item */
        assert(f->ival == 1);
        if (f->ncollected >= f->capacity) {
            f->capacity = f->ncollected + 1;  // We know there won't be any more
            f->collection = PyMem_Realloc(f->collection, (f->capacity) * sizeof(void *));
            if (!f->collection) {
                return PyErr_NoMemory();
            }
        }
        f->collection[f->ncollected++] = v;
        // Fallthrough!
    case OP_LOOP_COLLECT_NONEMPTY:
        if (!f->ncollected) {
            printf("Nothing collected for %s\n", f->rule->name);
            v = NULL;
            f = pop_frame(&stack, v);
            break;
        }
        // Fallthrough!
    case OP_LOOP_COLLECT:
        v = make_asdl_seq(p, f->collection, f->ncollected);
        f = pop_frame(&stack, v);
        if (!v) {
            return PyErr_NoMemory();
        }
        break;

    case OP_SAVE_MARK:
        f->savemark = p->mark;
        goto top;
    case OP_POS_LOOKAHEAD:
        assert(f->ival > 0);
        f->ival--;  /* Back out last added value */
        p->mark = f->savemark;
        goto top;
    case OP_NEG_LOOKAHEAD:
        v = NULL;
        break;

    case OP_SETUP_LEFT_REC:
        assert(p->mark == f->mark);
        assert (_PyPegen_is_memoized(p, f->rule->type + 1000, &v) == 0);
        if (_PyPegen_insert_memo(p, f->mark, f->rule->type + 1000, NULL) == -1)  {
            return NULL;
        }
        goto top;

    case OP_SUCCESS:
        v = f->vals[0];
        return v;
    case OP_FAILURE:
        return RAISE_SYNTAX_ERROR("A syntax error");

    case OP_TOKEN:
        oparg = f->rule->opcodes[f->iop++];
        v = _PyPegen_expect_token(p, oparg);
        break;
    case OP_SOFT_KEYWORD:
        oparg = f->rule->opcodes[f->iop++];
        v = _PyPegen_expect_soft_keyword(p, soft_keywords[oparg]);
        break;
    case OP_RULE:
        oparg = f->rule->opcodes[f->iop++];
        Rule *rule = &rules[oparg];
        // TODO: Selective memoization
        v = NULL;  // In case is_memoized ran into an error
        int memo = _PyPegen_is_memoized(p, rule->type + 1000, &v);
        if (memo) {
            // The result is v; if v != NULL, p->mark has been updated
            break;
        }
        f = push_frame(&stack, rule);
        goto top;
    case OP_RETURN:
        oparg = f->rule->opcodes[f->iop++];
        v = call_action(p, f, oparg);
        f = pop_frame(&stack, v);
        break;

    case OP_RETURN_LEFT_REC:
        oparg = f->rule->opcodes[f->iop++];
        v = call_action(p, f, oparg);
        if (v) {
            // It's a little tricky to recover the cached position:
            // we must call is_memoized() for the start position, and then
            // is_memoized() updates p->mark to the cached end position
            int newmark = p->mark;
            p->mark = f->mark;
            void *w;
            if (_PyPegen_is_memoized(p, f->rule->type + 1000, &w) == -1) {
                return NULL;
            }
            if (newmark > p->mark) {
                D(printf("        newmark wins\n"));
                // The new result is better than the cached value; update memo
                p->mark = newmark;
                int ok = _PyPegen_update_memo(p, f->mark, f->rule->type + 1000, v);
                if (ok == -1) {
                    return NULL;
                }
                // Restart parsing this rule from the top
                p->mark = f->mark;
                f->ival = 0;
                f->ialt = 0;
                f->iop = f->rule->alts[0];
                assert(f->rule->opcodes[f->iop] == OP_SETUP_LEFT_REC);
                f->iop++;  // Skip over OP_SETUP_LEFT_REC
                goto top;
            }
            // End the recursion loop, popping the frame
            v = w;
            f = pop_frame(&stack, v);
        }
        break;

    default:
        printf("opc=%d\n", opc);
        assert(0);
    }

    if (v) {
        D(printf("            OK\n"));
        assert(f->ival < MAXVALS);
        f->vals[f->ival++] = v;
        goto top;
    }
    if (PyErr_Occurred()) {
        D(printf("            PyErr\n"));
        p->error_indicator = 1;
        return NULL;
    }

 fail:
    opc = f->rule->opcodes[f->iop];
    if (opc == OP_OPTIONAL) {
        D(printf("            OP_OPTIONAL\n"));
        assert(f->ival < MAXVALS);
        f->vals[f->ival++] = NULL;
        f->iop++;  // Skip over the OP_OPTIONAL opcode
        goto top;
    }
    if (opc == OP_NEG_LOOKAHEAD) {
        D(printf("            OP_NEG_LOOKAHEAD\n"));
        p->mark = f->savemark;
        f->iop++;  // Skip over the OP_NEG_LOOKAHEAD opcode
        goto top;
    }

    D(printf("            fail\n"));
    p->mark = f->mark;
    if (f->cut)
        goto pop;
    f->iop = f->rule->alts[++f->ialt];
    if (f->iop == -1)
        goto pop;
    f->ival = 0;
    goto top;

 pop:
    f = pop_frame(&stack, NULL);
    goto fail;
}

void *
_PyPegen_vmparser(Parser *p)
{
    p->keywords = reserved_keywords;
    p->n_keyword_lists = n_keyword_lists;

    return run_vm(p, all_rules, R_ROOT);
}
