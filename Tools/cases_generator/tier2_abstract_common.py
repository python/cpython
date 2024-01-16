# We have to keep this here instead of tier2_abstract_generator.py
# to avoid a circular import.
SPECIALLY_HANDLED_ABSTRACT_INSTR = {
    "LOAD_FAST",
    "LOAD_FAST_CHECK",
    "LOAD_FAST_AND_CLEAR",
    "LOAD_CONST",
    "STORE_FAST",
    "STORE_FAST_MAYBE_NULL",
    "COPY",
    "POP_TOP",
    "PUSH_NULL",
    "SWAP",
    # Frame stuff
    "_PUSH_FRAME",
    "_POP_FRAME",
    # Bookkeeping
    "_SET_IP",
    "_CHECK_VALIDITY",
}
