typedef enum _opcodes {
    OP_NOOP,
    OP_CUT,
    OP_OPTIONAL,
    OP_NAME,
    OP_NUMBER,
    OP_STRING,
    OP_LOOP_ITERATE,
    OP_LOOP_COLLECT,
    OP_LOOP_COLLECT_NONEMPTY,
    OP_LOOP_COLLECT_DELIMITED,
    OP_SAVE_MARK,
    OP_POS_LOOKAHEAD,
    OP_NEG_LOOKAHEAD,
    OP_SUCCESS,
    OP_FAILURE,
    // The rest have an argument
    OP_SOFT_KEYWORD,
    OP_TOKEN,
    OP_RULE,
    OP_RETURN,
} Opcode;

static char *opcode_names[] = {
    "OP_NOOP",
    "OP_CUT",
    "OP_OPTIONAL",
    "OP_NAME",
    "OP_NUMBER",
    "OP_STRING",
    "OP_LOOP_ITERATE",
    "OP_LOOP_COLLECT",
    "OP_LOOP_COLLECT_NONEMPTY",
    "OP_LOOP_COLLECT_DELIMITED",
    "OP_SAVE_MARK",
    "OP_POS_LOOKAHEAD",
    "OP_NEG_LOOKAHEAD",
    "OP_SUCCESS",
    "OP_FAILURE",
    // The rest have an argument
    "OP_SOFT_KEYWORD",
    "OP_TOKEN",
    "OP_RULE",
    "OP_RETURN",
};

#define MAXALTS 15
#define MAXOPCODES 100

typedef struct _rule {
    char *name;
    short type;
    short memo;  // memoized rule (not left-recursive)
    short leftrec;  // left-recursive rule (needs memo lookup)
    short alts[MAXALTS];
    short opcodes[MAXOPCODES];
} Rule;

#define MAXVALS 10

typedef struct _frame {
    Rule *rule;
    int mark;
    int savemark;
    int lastmark;
    short ialt;
    short iop;
    short ival;
    short cut;
    int ncollected;
    int capacity;
    void *lastval;
    void **collection;
    void *vals[MAXVALS];
} Frame;
