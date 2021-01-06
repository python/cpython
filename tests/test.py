def fib(n):
    if n == 1 or n == 2:
        return 1
    a = fib(n - 1)
    b = fib(n - 2)
    return a + b

class Person(object):
    def __init__(self, name):
        self.name = name

    def hello(self):
        print("Hello, my name is " + self.name + "!")

my_list = [1, 2, 3, [321, 123]]
my_list.append(4)
my_list.remove(3)
my_list.extend([7, 8, 9])
my_list.insert(2, 42)
my_list[1] = 5
del my_list[0]
item = my_list.pop()

print('my_list', my_list)

my_list.reverse()
print('my_list', my_list)

my_list.clear()
print('my_list', my_list)

my_list2 = [1, 2, 3]

a = [1, 2]
b = [3, 4]
c = a + b
print(c)

my_dict = { 'a': 1 }
my_dict['b'] = 2
del my_dict['a']
my_dict.update({ 'd': 3, 'e': 5 })

print('my_dict', my_dict)

my_dict.pop('d')
print('my_dict', my_dict)

removed = my_dict.popitem()
print('removed', removed)
print('my_dict', my_dict)

my_dict.clear()
print('my_dict', my_dict)

fib(3)

s1 = "Hello"
s2 = "World"
s3 = s1 + s2
print(s3)

bob = Person("Robert")
bob.hello()

a_set = set([1, 2])
a_set.add(3)
a_set.update(set([4, 5]))
a_set.discard(3)
a_set.remove(2)

print(a_set)

a_set.clear()
print(a_set)

b_set = set([9, 8, 1, 2, 3])
c_set = a_set & b_set
print('c_set', c_set)

c_set = set([1])
c_set |= set([3])
print('c_set', c_set)