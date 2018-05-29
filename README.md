# cpython_r
Randomized the iterating order of cpython `dict` class.

The following functions are modified:
1. static PyObject * dictiter_iternextkey(dictiterobject *di)

2. static PyObject * dictiter_iternextvalue(dictiterobject *di)

3. static PyObject * dictiter_iternextitem(dictiterobject *di)

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
```
