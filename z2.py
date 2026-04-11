import sys


def testfunc(n):
    def gen():
        for i in range(10):
            yield i
    x = 0
    for i in range(4002):
        for i in gen():
            x += i
    return x
testfunc(4002)

