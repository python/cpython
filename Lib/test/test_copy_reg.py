import copy_reg

class C:
    pass


try:
    copy_reg.pickle(C, None, None)
except TypeError, e:
    print "Caught expected TypeError:"
    print e
else:
    print "Failed to catch expected TypeError when registering a class type."


print
try:
    copy_reg.pickle(type(1), "not a callable")
except TypeError, e:
    print "Caught expected TypeError:"
    print e
else:
    print "Failed to catch TypeError " \
          "when registering a non-callable reduction function."


print
try:
    copy_reg.pickle(type(1), int, "not a callable")
except TypeError, e:
    print "Caught expected TypeError:"
    print e
else:
    print "Failed to catch TypeError " \
          "when registering a non-callable constructor."
