
# http://python.org/sf/1303614

class Y(object):
    pass

class type_with_modifiable_dict(Y, type):
    pass

class MyClass(object, metaclass=type_with_modifiable_dict):
    """This class has its __dict__ attribute completely exposed:
    user code can read, reassign and even delete it.
    """


if __name__ == '__main__':
    del MyClass.__dict__  # if we set tp_dict to NULL,
    print(MyClass)         # doing anything with MyClass segfaults
