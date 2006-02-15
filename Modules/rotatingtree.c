#include "rotatingtree.h"

#define KEY_LOWER_THAN(key1, key2)  ((char*)(key1) < (char*)(key2))

/* The randombits() function below is a fast-and-dirty generator that
 * is probably irregular enough for our purposes.  Note that it's biased:
 * I think that ones are slightly more probable than zeroes.  It's not
 * important here, though.
 */

static unsigned int random_value = 1;
static unsigned int random_stream = 0;

static int
randombits(int bits)
{
	int result;
	if (random_stream < (1U << bits)) {
		random_value *= 1082527;
		random_stream = random_value;
	}
	result = random_stream & ((1<<bits)-1);
	random_stream >>= bits;
	return result;
}


/* Insert a new node into the tree.
   (*root) is modified to point to the new root. */
void
RotatingTree_Add(rotating_node_t **root, rotating_node_t *node)
{
	while (*root != NULL) {
		if (KEY_LOWER_THAN(node->key, (*root)->key))
			root = &((*root)->left);
		else
			root = &((*root)->right);
	}
	node->left = NULL;
	node->right = NULL;
	*root = node;
}

/* Locate the node with the given key.  This is the most complicated
   function because it occasionally rebalances the tree to move the
   resulting node closer to the root. */
rotating_node_t *
RotatingTree_Get(rotating_node_t **root, void *key)
{
	if (randombits(3) != 4) {
		/* Fast path, no rebalancing */
		rotating_node_t *node = *root;
		while (node != NULL) {
			if (node->key == key)
				return node;
			if (KEY_LOWER_THAN(key, node->key))
				node = node->left;
			else
				node = node->right;
		}
		return NULL;
	}
	else {
		rotating_node_t **pnode = root;
		rotating_node_t *node = *pnode;
		rotating_node_t *next;
		int rotate;
		if (node == NULL)
			return NULL;
		while (1) {
			if (node->key == key)
				return node;
			rotate = !randombits(1);
			if (KEY_LOWER_THAN(key, node->key)) {
				next = node->left;
				if (next == NULL)
					return NULL;
				if (rotate) {
					node->left = next->right;
					next->right = node;
					*pnode = next;
				}
				else
					pnode = &(node->left);
			}
			else {
				next = node->right;
				if (next == NULL)
					return NULL;
				if (rotate) {
					node->right = next->left;
					next->left = node;
					*pnode = next;
				}
				else
					pnode = &(node->right);
			}
			node = next;
		}
	}
}

/* Enumerate all nodes in the tree.  The callback enumfn() should return
   zero to continue the enumeration, or non-zero to interrupt it.
   A non-zero value is directly returned by RotatingTree_Enum(). */
int
RotatingTree_Enum(rotating_node_t *root, rotating_tree_enum_fn enumfn,
		  void *arg)
{
	int result;
	rotating_node_t *node;
	while (root != NULL) {
		result = RotatingTree_Enum(root->left, enumfn, arg);
		if (result != 0) return result;
		node = root->right;
		result = enumfn(root, arg);
		if (result != 0) return result;
		root = node;
	}
	return 0;
}
