from collections import deque, OrderedDict, Counter, defaultdict
t = (1, 2, 3)
l = [1, 2, 3]
s = {1, 2, 3}
d = {'a': 1, 'b': 2, 'c': 3}
dq = deque([1, 2, 3])
od = OrderedDict({'a': 1, 'b': 2, 'c': 3})
c = Counter('mississippi')
f = frozenset({1, 2})
intt = 1
mp = map
combined = {'tuple': t, 'list': l, 'set': s, 'dict': d, 'dict_keys': d, 'dict_values': d.values(), 'dict_items': d.items(), 'deque': dq,
            'ordered_dict': od, 'counter': c, 'frozenset': f, 'int': intt, 'map': mp}

names = ['t', 'l', 's', 'd', 'd.keys()', 'd.values()', 'd.items()', 'dq', 'od', 'c', 'f', 'intt', 'mp']

passed = defaultdict(lambda: defaultdict(set))
failed = defaultdict(set)

x = 0
for key, value in combined.items():
    for j in dir(value):
        try:
            qualname = eval(f'{names[x]}.{j}.__qualname__')
            qualname = qualname.split('.')
            passed[key][qualname[0]].add(qualname[1])
        except:
            failed[key].add(j)
    x = x + 1

print('passed: \n', passed)
print('failed: \n', failed)
