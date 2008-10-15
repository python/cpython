def hello():
    try:
        print "Hello, world"
    except IOError, e:
        print e.errno
