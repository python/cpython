import weakref

# http://python.org/sf/1377858
# Fixed for new-style classes in 2.5c1.

ref = None

def test_weakref_in_del():
    class Target():
        def __del__(self):
            global ref
            ref = weakref.ref(self)

    w = Target()

if __name__ == '__main__':
    test_weakref_in_del()
