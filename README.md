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
