/* Parser-tokenizer link interface */

#if 0
extern int parsetok PROTO((struct tok_state *, grammar *, int start,
							node **n_ret));
#endif
extern int parsestring PROTO((char *, grammar *, int start, node **n_ret));
extern int parsefile PROTO((FILE *, grammar *, int start,
					char *ps1, char *ps2, node **n_ret));
