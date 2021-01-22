a_set = set()
b_set = {1, 2, 3}
a_set.add("A")
a_set.add("B")
a_set.add("B")
b_set.discard(2)
b_set.discard(4)
b_set.remove(3)
a_set.clear()
c_set = {1, 2, 3}
d_set = {3, 4, 5}
e_set = c_set - d_set
d_set -= c_set
c_set = {1, 2, 3}
d_set = {3, 4, 5}
d_set |= c_set
c_set = {1, 2, 3}
d_set = {3, 4, 5}
d_set ^= c_set
c_set = {1, 2, 3}
d_set = {3, 4, 5}
d_set &= c_set
e_set = {1, 2, 3}
d_set = {3, 4, 5}
e_set.update(d_set)
f_set = set()
d_set = {3, 4, 5}
f_set.update(d_set)
g_set = set()
g_set.update({1, 2, 3})