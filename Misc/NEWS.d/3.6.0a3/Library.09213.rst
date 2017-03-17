Issue #16182: Fix various functions in the "readline" module to use the
locale encoding, and fix get_begidx() and get_endidx() to return code point
indexes.