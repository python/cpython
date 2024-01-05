// A doubly-linked list that can be embedded in a struct.
//
// Usage:
//  struct llist_node head = LLIST_INIT(head);
//  typedef struct {
//      ...
//      struct llist_node node;
//      ...
//  } MyObj;
//
//  llist_insert_tail(&head, &obj->node);
//  llist_remove(&obj->node);
//
//  struct llist_node *node;
//  llist_for_each(node, &head) {
//      MyObj *obj = llist_data(node, MyObj, node);
//      ...
//  }
//

#ifndef Py_INTERNAL_LLIST_H
#define Py_INTERNAL_LLIST_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "Py_BUILD_CORE must be defined to include this header"
#endif

struct llist_node {
    struct llist_node *next;
    struct llist_node *prev;
};

// Get the struct containing a node.
#define llist_data(node, type, member) \
    (type*)((char*)node - offsetof(type, member))

// Iterate over a list.
#define llist_for_each(node, head) \
    for (node = (head)->next; node != (head); node = node->next)

// Iterate over a list, but allow removal of the current node.
#define llist_for_each_safe(node, head) \
    for (struct llist_node *_next = (node = (head)->next, node->next); \
         node != (head); node = _next, _next = node->next)

#define LLIST_INIT(head) { &head, &head }

static inline void
llist_init(struct llist_node *head)
{
    head->next = head;
    head->prev = head;
}

// Returns 1 if the list is empty, 0 otherwise.
static inline int
llist_empty(struct llist_node *head)
{
    return head->next == head;
}

// Appends to the tail of the list.
static inline void
llist_insert_tail(struct llist_node *head, struct llist_node *node)
{
    node->prev = head->prev;
    node->next = head;
    head->prev->next = node;
    head->prev = node;
}

// Remove a node from the list.
static inline void
llist_remove(struct llist_node *node)
{
    struct llist_node *prev = node->prev;
    struct llist_node *next = node->next;
    prev->next = next;
    next->prev = prev;
    node->prev = NULL;
    node->next = NULL;
}

// Append all nodes from head2 onto head1. head2 is left empty.
static inline void
llist_concat(struct llist_node *head1, struct llist_node *head2)
{
    if (!llist_empty(head2)) {
        head1->prev->next = head2->next;
        head2->next->prev = head1->prev;

        head1->prev = head2->prev;
        head2->prev->next = head1;
        llist_init(head2);
    }
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_LLIST_H */
