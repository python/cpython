import global_test_module

def g():
    global name
    name = "Robert"

name = "Bobby"

g()

print(name)

global_test_module.g()
answer = global_test_module.h()
print(answer)

