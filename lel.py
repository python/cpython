import traceback

try:
    compile("f(x,     y for y in range(30))", "", "exec")
    # compile("f(x,     1 1232323     , z)", "", "exec")
    # compile("def fact(x):\n\treturn x!\n", "?", "exec")
    # compile("1+\n", "?", "exec")
except Exception as e:
    f = e

traceback.print_exception(f)
