# cpython_r
Randomized the iterating order of cpython `dict` class.

The following functions are modified:
1. static PyObject * dictiter_iternextkey(dictiterobject *di)

2. static PyObject * dictiter_iternextvalue(dictiterobject *di)

3. static PyObject * dictiter_iternextitem(dictiterobject *di)

# setup
You can refer to this page for setup instructions.

https://devguide.python.org/setup/

It is recommended that to install cpython_r in another directory other than your default python3. You can use the `--prefix` option to set your installation directory during configuration.

```
./configure --prefix=/foo/bar
```

# example code
```python
d = {"a":1, "b":2, "c":3, "d":4, "e":5, "f":7, "g":6}
a = [{}, {}, {}, {}, {}, {}, {}]
for k in range(70000):
    for i,e in enumerate(d):
        if e in a[i]:
            a[i][e] += 1
        else:
            a[i][e] = 1 
for i in a:
    print(i)
''' 
cpython_r result:
{'d': 10173, 'a': 10035, 'b': 9867, 'f': 10082, 'e': 9885, 'c': 10011, 'g': 9947}
{'c': 9925, 'g': 9941, 'a': 10093, 'e': 10120, 'd': 9917, 'f': 10031, 'b': 9973}
{'g': 10076, 'c': 10003, 'e': 9897, 'a': 10102, 'f': 10021, 'd': 9947, 'b': 9954}
{'a': 9860, 'b': 10017, 'e': 9931, 'd': 10117, 'f': 9958, 'g': 10154, 'c': 9963}
{'e': 10123, 'f': 10004, 'd': 9842, 'a': 9837, 'c': 10039, 'g': 9966, 'b': 10189}
{'b': 10065, 'd': 10049, 'c': 10043, 'g': 9836, 'f': 9903, 'a': 10059, 'e': 10045}
{'f': 10001, 'e': 9999, 'b': 9935, 'a': 10014, 'g': 10080, 'd': 9955, 'c': 10016}
cpython result:
{'a': 70000}
{'b': 70000}
{'c': 70000}
{'d': 70000}
{'e': 70000}
{'f': 70000}
{'g': 70000}
'''
```
```python
d = {"a":1, "b":2, "c":3, "d":4, "e":5, "f":7, "g":6}
a = [{}, {}, {}, {}, {}, {}, {}]
for k in range(70000):
    for i,e in enumerate(d.values()):
        if e in a[i]:
            a[i][e] += 1
        else:
            a[i][e] = 1 
for i in a:
    print(i)
'''
cpython_r result:
{2: 9974, 5: 10137, 3: 9996, 7: 9791, 6: 10053, 4: 9962, 1: 10087}
{4: 10162, 3: 9904, 6: 9858, 5: 9869, 2: 10117, 1: 9871, 7: 10219}
{5: 10182, 6: 9915, 2: 10017, 7: 9990, 4: 9983, 3: 9952, 1: 9961}
{3: 10293, 1: 10060, 7: 9891, 4: 10023, 2: 9817, 5: 9946, 6: 9970}
{6: 10009, 7: 10003, 4: 10010, 2: 10151, 3: 9931, 1: 9914, 5: 9982}
{7: 10097, 4: 9828, 1: 10087, 6: 10042, 3: 9997, 5: 10027, 2: 9922}
{1: 10020, 5: 9857, 3: 9927, 4: 10032, 2: 10002, 6: 10153, 7: 10009}
cpython result:
{1: 70000}
{2: 70000}
{3: 70000}
{4: 70000}
{5: 70000}
{7: 70000}
{6: 70000}
'''
```
```python
d = {"a":1, "b":2, "c":3, "d":4, "e":5, "f":7, "g":6}
a = [{}, {}, {}, {}, {}, {}, {}]
for k in range(70000):
    for i,e in enumerate(d.items()):
        if e in a[i]:
            a[i][e] += 1
        else:
            a[i][e] = 1 
for i in a:
    print(i)
'''
cpython_r result:
{('d', 4): 9940, ('e', 5): 10036, ('f', 7): 9924, ('c', 3): 10033, ('b', 2): 9964, ('a', 1): 10045, ('g', 6): 10058}
{('c', 3): 10109, ('d', 4): 9884, ('g', 6): 10031, ('a', 1): 10065, ('b', 2): 9888, ('f', 7): 10019, ('e', 5): 10004}
{('f', 7): 10173, ('c', 3): 9923, ('b', 2): 9926, ('g', 6): 9789, ('a', 1): 10104, ('e', 5): 10110, ('d', 4): 9975}
{('e', 5): 9932, ('a', 1): 9881, ('c', 3): 9938, ('g', 6): 10053, ('d', 4): 10033, ('b', 2): 10183, ('f', 7): 9980}
{('g', 6): 10056, ('a', 1): 10100, ('d', 4): 10050, ('f', 7): 9981, ('e', 5): 9928, ('c', 3): 9916, ('b', 2): 9969}
{('a', 1): 9849, ('c', 3): 9971, ('d', 4): 10185, ('e', 5): 9907, ('g', 6): 9935, ('b', 2): 10087, ('f', 7): 10066}
{('b', 2): 9983, ('f', 7): 9857, ('a', 1): 9956, ('g', 6): 10078, ('d', 4): 9933, ('e', 5): 10083, ('c', 3): 10110}
cpython result:
{('a', 1): 70000}
{('b', 2): 70000}
{('c', 3): 70000}
{('d', 4): 70000}
{('e', 5): 70000}
{('f', 7): 70000}
{('g', 6): 70000}
'''
```
