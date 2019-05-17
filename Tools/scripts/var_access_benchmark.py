'Show relative speeds of local, nonlocal, global, and built-in access.'

# Please leave this code so that it runs under older versions of
# Python 3 (no f-strings).  That will allow benchmarking for
# cross-version comparisons.  To run the benchmark on Python 2,
# comment-out the nonlocal reads and writes.

from collections import deque, namedtuple

trials = [None] * 500
steps_per_trial = 25

class A(object):
    def m(self):
        pass

class B(object):
    __slots__ = 'x'
    def __init__(self, x):
        self.x = x

class C(object):
    def __init__(self, x):
        self.x = x

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

def read_classvar_from_class(trials=trials, A=A):
    A.x = 1
    for t in trials:
        A.x;    A.x;    A.x;    A.x;    A.x
        A.x;    A.x;    A.x;    A.x;    A.x
        A.x;    A.x;    A.x;    A.x;    A.x
        A.x;    A.x;    A.x;    A.x;    A.x
        A.x;    A.x;    A.x;    A.x;    A.x

def read_classvar_from_instance(trials=trials, A=A):
    A.x = 1
    a = A()
    for t in trials:
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x

def read_instancevar(trials=trials, a=C(1)):
    for t in trials:
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x

def read_instancevar_slots(trials=trials, a=B(1)):
    for t in trials:
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x
        a.x;    a.x;    a.x;    a.x;    a.x

def read_namedtuple(trials=trials, D=namedtuple('D', ['x'])):
    a = D(1)
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

def write_instancevar(trials=trials, a=C(1)):
    for t in trials:
        a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1
        a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1
        a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1
        a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1
        a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1

def write_instancevar_slots(trials=trials, a=B(1)):
    for t in trials:
        a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1
        a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1
        a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1
        a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1
        a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1;    a.x = 1

def read_list(trials=trials, a=[1]):
    for t in trials:
        a[0];   a[0];   a[0];   a[0];   a[0]
        a[0];   a[0];   a[0];   a[0];   a[0]
        a[0];   a[0];   a[0];   a[0];   a[0]
        a[0];   a[0];   a[0];   a[0];   a[0]
        a[0];   a[0];   a[0];   a[0];   a[0]

def read_deque(trials=trials, a=deque([1])):
    for t in trials:
        a[0];   a[0];   a[0];   a[0];   a[0]
        a[0];   a[0];   a[0];   a[0];   a[0]
        a[0];   a[0];   a[0];   a[0];   a[0]
        a[0];   a[0];   a[0];   a[0];   a[0]
        a[0];   a[0];   a[0];   a[0];   a[0]

def read_dict(trials=trials, a={0: 1}):
    for t in trials:
        a[0];   a[0];   a[0];   a[0];   a[0]
        a[0];   a[0];   a[0];   a[0];   a[0]
        a[0];   a[0];   a[0];   a[0];   a[0]
        a[0];   a[0];   a[0];   a[0];   a[0]
        a[0];   a[0];   a[0];   a[0];   a[0]

def read_strdict(trials=trials, a={'key': 1}):
    for t in trials:
        a['key'];   a['key'];   a['key'];   a['key'];   a['key']
        a['key'];   a['key'];   a['key'];   a['key'];   a['key']
        a['key'];   a['key'];   a['key'];   a['key'];   a['key']
        a['key'];   a['key'];   a['key'];   a['key'];   a['key']
        a['key'];   a['key'];   a['key'];   a['key'];   a['key']

def list_append_pop(trials=trials, a=[1]):
    ap, pop = a.append, a.pop
    for t in trials:
        ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop()
        ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop()
        ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop()
        ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop()
        ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop()

def deque_append_pop(trials=trials, a=deque([1])):
    ap, pop = a.append, a.pop
    for t in trials:
        ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop()
        ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop()
        ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop()
        ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop()
        ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop()

def deque_append_popleft(trials=trials, a=deque([1])):
    ap, pop = a.append, a.popleft
    for t in trials:
        ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop();
        ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop();
        ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop();
        ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop();
        ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop(); ap(1); pop();

def write_list(trials=trials, a=[1]):
    for t in trials:
        a[0]=1; a[0]=1; a[0]=1; a[0]=1; a[0]=1
        a[0]=1; a[0]=1; a[0]=1; a[0]=1; a[0]=1
        a[0]=1; a[0]=1; a[0]=1; a[0]=1; a[0]=1
        a[0]=1; a[0]=1; a[0]=1; a[0]=1; a[0]=1
        a[0]=1; a[0]=1; a[0]=1; a[0]=1; a[0]=1

def write_deque(trials=trials, a=deque([1])):
    for t in trials:
        a[0]=1; a[0]=1; a[0]=1; a[0]=1; a[0]=1
        a[0]=1; a[0]=1; a[0]=1; a[0]=1; a[0]=1
        a[0]=1; a[0]=1; a[0]=1; a[0]=1; a[0]=1
        a[0]=1; a[0]=1; a[0]=1; a[0]=1; a[0]=1
        a[0]=1; a[0]=1; a[0]=1; a[0]=1; a[0]=1

def write_dict(trials=trials, a={0: 1}):
    for t in trials:
        a[0]=1; a[0]=1; a[0]=1; a[0]=1; a[0]=1
        a[0]=1; a[0]=1; a[0]=1; a[0]=1; a[0]=1
        a[0]=1; a[0]=1; a[0]=1; a[0]=1; a[0]=1
        a[0]=1; a[0]=1; a[0]=1; a[0]=1; a[0]=1
        a[0]=1; a[0]=1; a[0]=1; a[0]=1; a[0]=1

def write_strdict(trials=trials, a={'key': 1}):
    for t in trials:
        a['key']=1; a['key']=1; a['key']=1; a['key']=1; a['key']=1
        a['key']=1; a['key']=1; a['key']=1; a['key']=1; a['key']=1
        a['key']=1; a['key']=1; a['key']=1; a['key']=1; a['key']=1
        a['key']=1; a['key']=1; a['key']=1; a['key']=1; a['key']=1
        a['key']=1; a['key']=1; a['key']=1; a['key']=1; a['key']=1

def loop_overhead(trials=trials):
    for t in trials:
        pass


if __name__=='__main__':

    from timeit import Timer

    for f in [
            'Variable and attribute read access:',
            read_local, read_nonlocal, read_global, read_builtin,
            read_classvar_from_class, read_classvar_from_instance,
            read_instancevar, read_instancevar_slots,
            read_namedtuple, read_boundmethod,
            '\nVariable and attribute write access:',
            write_local, write_nonlocal, write_global,
            write_classvar, write_instancevar, write_instancevar_slots,
            '\nData structure read access:',
            read_list, read_deque, read_dict, read_strdict,
            '\nData structure write access:',
            write_list, write_deque, write_dict, write_strdict,
            '\nStack (or queue) operations:',
            list_append_pop, deque_append_pop, deque_append_popleft,
            '\nTiming loop overhead:',
            loop_overhead]:
        if isinstance(f, str):
            print(f)
            continue
        timing = min(Timer(f).repeat(7, 1000))
        timing *= 1000000 / (len(trials) * steps_per_trial)
        print('{:6.1f} ns\t{}'.format(timing, f.__name__))
