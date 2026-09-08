def function_1():
    function_3(1, 2)

# Check stacktrace
def function_2():
    function_1()

# CALL with positional args
def function_3(dummy, dummy2):
    pass

# CALL_KW (keyword arguments)
def function_4(**dummy):
    return 1
    return 2  # unreachable

# CALL_FUNCTION_EX (unpacking)
def function_5(dummy, dummy2, **dummy3):
    if False:
        return 7
    return 8

def start():
    function_1()
    function_2()
    function_3(1, 2)
    function_4(test=42)
    function_5(*(1, 2), **{"test": 42})

start()
