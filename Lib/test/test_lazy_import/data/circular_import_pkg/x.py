def X1():
    return "X"

from .y import Y1

def X2():
    return Y1()
