def fib(n):
    if n == 1 or n == 2:
        return 1
    a = fib(n - 1)
    b = fib(n - 2)
    return a + b

class MyClass(object):
    def hello(self):
        print("Hello")

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

print('my_dict', my_dict)

fib(3)

