a_list = [1]
for i in range(10):
    a_list.append(i)

b_list = [1, 2, 3]

b_list.extend(a_list)

del b_list[0]
del b_list[1]

b_list[2] = 123

c_list = [4, 5, 6]
c_list.insert(2, 7)

c_list.remove(4)
item = c_list.pop()

c_list.clear()

b_list.reverse()

d_list = [[1, 2], [3, 4], [5, 6]]
e_list = []
for sub_list in d_list:
    e_list.append(sub_list[0] + sub_list[1])