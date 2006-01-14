
# http://python.org/sf/1303614

class Strange(object):
    def __hash__(self):
        return hash('hello')

    def __eq__(self, other):
        x.__dict__ = {}   # the old x.__dict__ is deallocated
        return False


class X(object):
    pass

if __name__ == '__main__':
    v = 123
    x = X()
    x.__dict__ = {Strange(): 42,
                  'hello': v+456}
    x.hello  # segfault: the above dict is accessed after it's deallocated
