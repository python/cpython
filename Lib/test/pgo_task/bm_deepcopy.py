"""
Benchmark to measure performance of the python builtin method copy.deepcopy

Performance is tested on a nested dictionary and a dataclass

Author: Pieter Eendebak

"""
import copy
from dataclasses import dataclass


@dataclass
class A:
    string: str
    lst: list
    boolean: bool


def benchmark_reduce(n):
    """ Benchmark where the __reduce__ functionality is used """
    class C(object):
        def __init__(self):
            self.a = 1
            self.b = 2

        def __reduce__(self):
            return (C, (), self.__dict__)

        def __setstate__(self, state):
            self.__dict__.update(state)
    c = C()

    for ii in range(n):
        _ = copy.deepcopy(c)


def benchmark_memo(n):
    """ Benchmark where the memo functionality is used """
    A = [1] * 100
    data = {'a': (A, A, A), 'b': [A] * 100}

    for ii in range(n):
        _ = copy.deepcopy(data)


def benchmark(n):
    """ Benchmark on some standard data types """
    a = {
        'list': [1, 2, 3, 43],
        't': (1 ,2, 3),
        'str': 'hello',
        'subdict': {'a': True}
    }
    dc = A('hello', [1, 2, 3], True)

    dt = 0
    for ii in range(n):
        for jj in range(30):
            _ = copy.deepcopy(a)
        for s in ['red', 'blue', 'green']:
            dc.string = s
            for kk in range(5):
                dc.lst[0] = kk
                for b in [True, False]:
                    dc.boolean = b
                    _ = copy.deepcopy(dc)


def run_pgo():
    loops = 1000
    benchmark(loops)
    benchmark_reduce(loops)
    benchmark_memo(loops)
