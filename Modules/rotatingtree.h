/* "Rotating trees" (Armin Rigo)
 *
 * Google "splay trees" for the general idea.
 *
 * It's a dict-like data structure that works best when accesses are not
 * random, but follow a strong pattern.  The one implemented here is for
 * access patterns where the same small set of keys is looked up over
 * and over again, and this set of keys evolves slowly over time.
 */

#include <stdlib.h>

#define EMPTY_ROTATING_TREE       ((rotating_node_t *)NULL)

typedef struct rotating_node_s rotating_node_t;
typedef int (*rotating_tree_enum_fn) (rotating_node_t *node, void *arg);

struct rotating_node_s {
	void *key;
	rotating_node_t *left;
	rotating_node_t *right;
};

void RotatingTree_Add(rotating_node_t **root, rotating_node_t *node);
rotating_node_t* RotatingTree_Get(rotating_node_t **root, void *key);
int RotatingTree_Enum(rotating_node_t *root, rotating_tree_enum_fn enumfn,
		      void *arg);
