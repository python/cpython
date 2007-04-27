
# http://python.org/sf/1303614

class Y(object):
    pass

class type_with_modifiable_dict(type, Y):
    pass

class MyClass(object, metaclass=type_with_modifiable_dict):
    """This class has its __dict__ attribute indirectly
    exposed via the __dict__ getter/setter of Y.
    """


if __name__ == '__main__':
    dictattr = Y.__dict__['__dict__']
    dictattr.__delete__(MyClass)  # if we set tp_dict to NULL,
    print(MyClass)         # doing anything with MyClass segfaults
