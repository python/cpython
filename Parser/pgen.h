/* Parser generator interface */

extern grammar gram;

extern grammar *meta_grammar PROTO((void));
extern grammar *pgen PROTO((struct _node *));
