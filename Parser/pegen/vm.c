#include <Python.h>
#include <errcode.h>
#include "../tokenizer.h"

#include "pegen.h"
#include "parse_string.h"

#include "vm.h"

static Frame *
push_frame(Stack *stack, Rule *rule)
{
    printf("               push %s\n", rule->name);
    assert(stack->top < 100);
    Frame *f = &stack->frames[stack->top++];
    f->rule = rule;
    f->mark = stack->p->mark;
    f->ialt = 0;
    f->iop = 0;
    f->cut = 0;
    f->ncollected = 0;
    f->collection = NULL;
    f->ival = 0;
    return f;
}

static Frame *
pop_frame(Stack *stack)
{
    assert(stack->top > 1);
    Frame *f = &stack->frames[--stack->top];  // Frame being popped
    if (f->collection) {
        PyMem_Free(f->collection);
    }
    f = &stack->frames[stack->top - 1];  // New top of stack
    printf("               pop %s\n", f->rule->name);
    return f;
}

static asdl_seq *
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
run_vm(Parser *p, Rule rules[], int start)
{
    Stack stack = {p, 0, {{0}}};
    Frame *f = push_frame(&stack, &rules[start]);
    void *v;
    int oparg;

 top:
    if (p->mark == p->fill)
        _PyPegen_fill_token(p);
    for (int i = 0; i < stack.top; i++) printf(" ");
    printf("Rule: %s; ialt=%d; iop=%d; op=%s; mark=%d; token=%d; p^='%s'\n",
           f->rule->name, f->ialt, f->iop, opcode_names[f->rule->opcodes[f->iop]], p->mark, p->tokens[p->mark]->type,
           p->fill > p-> mark ? PyBytes_AsString(p->tokens[p->mark]->bytes) : "<UNSEEN>");
    switch (f->rule->opcodes[f->iop++]) {
    case OP_NOOP:
        goto top;
    case OP_NAME:
        v = _PyPegen_name_token(p);
        break;
    case OP_NUMBER:
        v = _PyPegen_number_token(p);
        break;
    case OP_CUT:
        f->cut = 1;
        goto top;
    case OP_OPTIONAL:
        goto top;
    case OP_LOOP_START:
        f->ncollected = 0;
        f->collection = PyMem_Malloc(0);
        if (!f->collection) {
            return PyErr_NoMemory();
        }
        goto top;
    case OP_LOOP_ITERATE:
        f->mark = p->mark;
        assert(f->ival == 1);
        v = f->vals[0];
        assert(v);
        f->collection = PyMem_Realloc(f->collection, (f->ncollected + 1) * sizeof(void *));
        if (!f->collection) {
            return NULL;
        }
        f->collection[f->ncollected++] = v;
        f->iop = f->rule->alts[f->ialt] + 1;  // Skip OP_LOOP_START operation
        f->ival = 0;
        goto top;
    case OP_LOOP_COLLECT:
        v = make_asdl_seq(p, f->collection, f->ncollected);
        f = pop_frame(&stack);
        if (!v) {
            return PyErr_NoMemory();
        }
        break;
    case OP_TOKEN:
        oparg = f->rule->opcodes[f->iop++];
        v = _PyPegen_expect_token(p, oparg);
        break;
    case OP_RULE:
        oparg = f->rule->opcodes[f->iop++];
        f = push_frame(&stack, &rules[oparg]);
        goto top;
    case OP_RETURN:
        oparg = f->rule->opcodes[f->iop++];
        v = call_action(p, f, oparg);
        f = pop_frame(&stack);
        break;
    case OP_SUCCESS:
        oparg = f->rule->opcodes[f->iop++];
        v = call_action(p, f, oparg);
        if (v || PyErr_Occurred()) {
            return v;
        }
        return RAISE_SYNTAX_ERROR("Final action failed");
    case OP_FAILURE:
        return RAISE_SYNTAX_ERROR("A syntax error");
    default:
        abort();
    }

    if (v) {
        printf("            OK\n");
        f->vals[f->ival++] = v;
        goto top;
    }
    if (PyErr_Occurred()) {
        printf("            PyErr\n");
        p->error_indicator = 1;
        return NULL;
    }

 fail:
    if (f->rule->opcodes[f->iop] == OP_OPTIONAL) {
        printf("            GA!\n");
        f->vals[f->ival++] = NULL;
        f->iop++;  // Skip over the OP_OPTIONAL opcode
        goto top;
    }

    printf("            fail\n");
    p->mark = f->mark;
    if (f->cut)
        goto pop;
    f->iop = f->rule->alts[++f->ialt];
    if (f->iop == -1)
        goto pop;
    goto top;

 pop:
    f = pop_frame(&stack);
    goto fail;
}

void *
_PyPegen_vmparser(Parser *p)
{
    p->keywords = reserved_keywords;
    p->n_keyword_lists = n_keyword_lists;

    return run_vm(p, all_rules, 0);
}
