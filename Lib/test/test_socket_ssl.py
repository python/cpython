# Test just the SSL support in the socket module, in a moderately bogus way.

from test import test_support
import socket

# Optionally test SSL support.  This requires the 'network' resource as given
# on the regrtest command line.
skip_expected = not (test_support.is_resource_enabled('network') and
                     hasattr(socket, "ssl"))

def test_main():
    test_support.requires('network')
    if not hasattr(socket, "ssl"):
        raise test_support.TestSkipped("socket module has no ssl support")

    import urllib

    socket.RAND_status()
    try:
        socket.RAND_egd(1)
    except TypeError:
        pass
    else:
        print "didn't raise TypeError"
    socket.RAND_add("this is a random string", 75.0)

    f = urllib.urlopen('https://sf.net')
    buf = f.read()
    f.close()

if __name__ == "__main__":
    test_main()
