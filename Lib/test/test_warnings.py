import warnings

# The warnings module isn't easily tested, because it relies on module
# globals to store configuration information.  We need to extract the
# current settings to avoid bashing them while running tests.

_filters = []
_showwarning = None

def showwarning(message, category, filename, lineno, file=None):
    i = filename.find("Lib")
    filename = filename[i:]
    print "%s:%s: %s: %s" % (filename, lineno, category.__name__, message)

def monkey():
    global _filters, _showwarning
    _filters = warnings.filters[:]
    _showwarning = warnings.showwarning
    warnings.showwarning = showwarning

def unmonkey():
    warnings.filters = _filters[:]
    warnings.showwarning = _showwarning

def test():
    for item in warnings.filters:
        print (item[0], item[1] is None, item[2].__name__, item[3] is None,
               item[4])
    hello = "hello world"
    for i in range(4):
        warnings.warn(hello)
    warnings.warn(hello, UserWarning)
    warnings.warn(hello, DeprecationWarning)
    for i in range(3):
        warnings.warn(hello)
    warnings.filterwarnings("error", "", Warning, "", 0)
    try:
        warnings.warn(hello)
    except Exception, msg:
        print "Caught", msg.__class__.__name__ + ":", msg
    else:
        print "No exception"
    warnings.resetwarnings()
    try:
        warnings.filterwarnings("booh", "", Warning, "", 0)
    except Exception, msg:
        print "Caught", msg.__class__.__name__ + ":", msg
    else:
        print "No exception"

monkey()
test()
unmonkey()
