# Coroutine example:  controlling multiple instances of a single function

from Coroutine import *

# fringe visits a nested list in inorder, and detaches for each non-list
# element; raises EarlyExit after the list is exhausted
def fringe( co, list ):
    for x in list:
        if type(x) is type([]):
            fringe(co, x)
        else:
            co.back(x)

def printinorder( list ):
    co = Coroutine()
    f = co.create(fringe, co, list)
    try:
        while 1:
            print co.tran(f),
    except EarlyExit:
        pass
    print

printinorder([1,2,3])  # 1 2 3
printinorder([[[[1,[2]]],3]]) # ditto
x = [0, 1, [2, [3]], [4,5], [[[6]]] ]
printinorder(x) # 0 1 2 3 4 5 6

# fcmp lexicographically compares the fringes of two nested lists
def fcmp( l1, l2 ):
    co1 = Coroutine(); f1 = co1.create(fringe, co1, l1)
    co2 = Coroutine(); f2 = co2.create(fringe, co2, l2)
    while 1:
        try:
            v1 = co1.tran(f1)
        except EarlyExit:
            try:
                v2 = co2.tran(f2)
            except EarlyExit:
                return 0
            co2.kill()
            return -1
        try:
            v2 = co2.tran(f2)
        except EarlyExit:
            co1.kill()
            return 1
        if v1 != v2:
            co1.kill(); co2.kill()
            return cmp(v1,v2)

print fcmp(range(7), x)  #  0; fringes are equal
print fcmp(range(6), x)  # -1; 1st list ends early
print fcmp(x, range(6))  #  1; 2nd list ends early
print fcmp(range(8), x)  #  1; 2nd list ends early
print fcmp(x, range(8))  # -1; 1st list ends early
print fcmp([1,[[2],8]],
           [[[1],2],8])  #  0
print fcmp([1,[[3],8]],
           [[[1],2],8])  #  1
print fcmp([1,[[2],8]],
           [[[1],2],9])  # -1

# end of example
