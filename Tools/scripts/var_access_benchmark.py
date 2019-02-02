'Show relative speeds of local, nonlocal, global, and built-in access.'

# Please leave this code so that it runs under older versions of
# Python 3 (no f-strings).  That will allow benchmarking for
# cross-version comparisons.

from collections import namedtuple

trials = [None] * 500
steps_per_trial = 25

def read_local(trials=trials):
    v_local = 1
    for t in trials:
        v_local;    v_local;    v_local;    v_local;    v_local
        v_local;    v_local;    v_local;    v_local;    v_local
        v_local;    v_local;    v_local;    v_local;    v_local
        v_local;    v_local;    v_local;    v_local;    v_local
        v_local;    v_local;    v_local;    v_local;    v_local

def make_nonlocal_reader():
    v_nonlocal = 1
    def inner(trials=trials):
        for t in trials:
            v_nonlocal; v_nonlocal; v_nonlocal; v_nonlocal; v_nonlocal
            v_nonlocal; v_nonlocal; v_nonlocal; v_nonlocal; v_nonlocal
            v_nonlocal; v_nonlocal; v_nonlocal; v_nonlocal; v_nonlocal
            v_nonlocal; v_nonlocal; v_nonlocal; v_nonlocal; v_nonlocal
            v_nonlocal; v_nonlocal; v_nonlocal; v_nonlocal; v_nonlocal
    inner.__name__ = 'read_nonlocal'
    return inner

read_nonlocal = make_nonlocal_reader()

v_global = 1
def read_global(trials=trials):
    for t in trials:
        v_global; v_global; v_global; v_global; v_global
        v_global; v_global; v_global; v_global; v_global
        v_global; v_global; v_global; v_global; v_global
        v_global; v_global; v_global; v_global; v_global
        v_global; v_global; v_global; v_global; v_global

def read_builtin(trials=trials):
    for t in trials:
        oct; oct; oct; oct; oct
        oct; oct; oct; oct; oct
        oct; oct; oct; oct; oct
        oct; oct; oct; oct; oct
        oct; oct; oct; oct; oct

class A:
    def m(self):
        pass

class B(object):
    __slots__ = 'x'

def read_classvar(trials=trials, A=A):
    A.x = 1
    for t in trials:
        A.x;    A.x;    A.x;    A.x;    A.x
        A.x;    A.x;    A.x;    A.x;    A.x
        A.x;    A.x;    A.x;    A.x;    A.x
        A.x;    A.x;    A.x;    A.x;    A.x
        A.x;    A.x;    A.x;    A.x;    A.x

def read_instancevar(trials=trials, a=A()):
    a.x = 1
    for t in trials:
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x

def read_instancevar_slots(trials=trials, a=B()):
    a.x = 1
    for t in trials:
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x

def read_namedtuple(trials=trials, C=namedtuple('C', ['x'])):
    a = C(1)
    for t in trials:
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x

def read_boundmethod(trials=trials, a=A()):
    for t in trials:
        a.m;    a.m;    a.m;    a.m;    a.m
        a.m;    a.m;    a.m;    a.m;    a.m
        a.m;    a.m;    a.m;    a.m;    a.m
        a.m;    a.m;    a.m;    a.m;    a.m
        a.m;    a.m;    a.m;    a.m;    a.m

def write_local(trials=trials):
    v_local = 1
    for t in trials:
        v_local = 1; v_local = 1; v_local = 1; v_local = 1; v_local = 1
        v_local = 1; v_local = 1; v_local = 1; v_local = 1; v_local = 1
        v_local = 1; v_local = 1; v_local = 1; v_local = 1; v_local = 1
        v_local = 1; v_local = 1; v_local = 1; v_local = 1; v_local = 1
        v_local = 1; v_local = 1; v_local = 1; v_local = 1; v_local = 1

def make_nonlocal_writer():
    v_nonlocal = 1
    def inner(trials=trials):
        nonlocal v_nonlocal
        for t in trials:
            v_nonlocal = 1; v_nonlocal = 1; v_nonlocal = 1; v_nonlocal = 1; v_nonlocal = 1
            v_nonlocal = 1; v_nonlocal = 1; v_nonlocal = 1; v_nonlocal = 1; v_nonlocal = 1
            v_nonlocal = 1; v_nonlocal = 1; v_nonlocal = 1; v_nonlocal = 1; v_nonlocal = 1
            v_nonlocal = 1; v_nonlocal = 1; v_nonlocal = 1; v_nonlocal = 1; v_nonlocal = 1
            v_nonlocal = 1; v_nonlocal = 1; v_nonlocal = 1; v_nonlocal = 1; v_nonlocal = 1
    inner.__name__ = 'write_nonlocal'
    return inner

write_nonlocal = make_nonlocal_writer()

def write_global(trials=trials):
    global v_global
    for t in trials:
        v_global = 1; v_global = 1; v_global = 1; v_global = 1; v_global = 1
        v_global = 1; v_global = 1; v_global = 1; v_global = 1; v_global = 1
        v_global = 1; v_global = 1; v_global = 1; v_global = 1; v_global = 1
        v_global = 1; v_global = 1; v_global = 1; v_global = 1; v_global = 1
        v_global = 1; v_global = 1; v_global = 1; v_global = 1; v_global = 1

def write_classvar(trials=trials, A=A):
    for t in trials:
        A.x = 1;    A.x = 1;    A.x = 1;    A.x = 1;    A.x = 1
        A.x = 1;    A.x = 1;    A.x = 1;    A.x = 1;    A.x = 1
        A.x = 1;    A.x = 1;    A.x = 1;    A.x = 1;    A.x = 1
        A.x = 1;    A.x = 1;    A.x = 1;    A.x = 1;    A.x = 1
        A.x = 1;    A.x = 1;    A.x = 1;    A.x = 1;    A.x = 1

def write_instancevar(trials=trials, a=A()):
    for t in trials:
        a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1
        a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1
        a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1
        a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1
        a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1

def write_instancevar_slots(trials=trials, a=B()):
    for t in trials:
        a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1
        a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1
        a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1
        a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1
        a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1

def loop_overhead(trials=trials):
    for t in trials:
        pass


if __name__=='__main__':

    from timeit import Timer

    print('Speed of different kinds of variable accesses:')
    for f in [read_local, read_nonlocal, read_global, read_builtin,
              read_classvar, read_instancevar, read_instancevar_slots,
              read_namedtuple, read_boundmethod,
              write_local, write_nonlocal, write_global,
              write_classvar, write_instancevar, write_instancevar_slots,
              loop_overhead]:
        timing = min(Timer(f).repeat(7, 1000))
        timing *= 1000000 / (len(trials) * steps_per_trial)
        print('{:6.1f} \N{greek small letter mu}s\t{}'.format(timing, f.__name__))
