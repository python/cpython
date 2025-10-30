#ifndef _PY_TOKENIZER_MODE_H_
#define _PY_TOKENIZER_MODE_H_

/* Maximum nesting level for f-strings and t-strings */
#define MAXFSTRINGLEVEL 150

/* Macro to check if currently inside an f-string */
#define INSIDE_FSTRING(tok) ((tok)->tok_mode_stack_index > 0)
#define INSIDE_FSTRING_EXPR(tok) ((tok)->curly_bracket_expr_start_depth >= 0)
#define INSIDE_FSTRING_EXPR_AT_TOP(tok) \
    ((tok)->curly_bracket_depth - (tok)->curly_bracket_expr_start_depth == 1)

/* Tokenizer mode kinds */
enum tokenizer_mode_kind_t {
    TOK_REGULAR_MODE,
    TOK_FSTRING_MODE,
};

/* String kind (f-string or t-string) */
enum string_kind_t {
    FSTRING,
    TSTRING,
};

#define MAX_EXPR_NESTING 3

/* Tokenizer mode structure for tracking f-string/t-string state.
 * This structure is used in a stack (tok_mode_stack) to handle
 * nested f-strings and t-strings, allowing proper parsing of
 * expressions within string literals. */
typedef struct _tokenizer_mode {
    enum tokenizer_mode_kind_t kind;

    /* Curly bracket depth tracking for expressions in f-strings */
    int curly_bracket_depth;
    int curly_bracket_expr_start_depth;

    /* Quote character and size (1 for ', ", 3 for ''', """) */
    char quote;
    int quote_size;
    
    /* Whether string has 'r' prefix (raw string) */
    int raw;
    
    /* Position tracking for error reporting */
    const char* start;
    const char* multi_line_start;
    int first_line;

    /* Byte offsets for string positioning */
    Py_ssize_t start_offset;
    Py_ssize_t multi_line_start_offset;

    /* Buffer for tracking last expression in f-string */
    Py_ssize_t last_expr_size;
    Py_ssize_t last_expr_end;
    char* last_expr_buffer;
    
    /* State flags for format specs and debug expressions */
    int in_debug;
    int in_format_spec;

    /* Whether this is an f-string or t-string */
    enum string_kind_t string_kind;
} tokenizer_mode;

#endif /* !_PY_TOKENIZER_MODE_H_ */
