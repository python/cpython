/* Error codes passed around between file input, tokenizer, parser and
   interpreter.  This was necessary so we can turn them into Python
   exceptions at a higher level. */

#define E_OK		10	/* No error */
#define E_EOF		11	/* (Unexpected) EOF read */
#define E_INTR		12	/* Interrupted */
#define E_TOKEN		13	/* Bad token */
#define E_SYNTAX	14	/* Syntax error */
#define E_NOMEM		15	/* Ran out of memory */
#define E_DONE		16	/* Parsing complete */
#define E_ERROR		17	/* Execution error */
