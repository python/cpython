
def f(): a = b = c = d = e = 0; g()

def g(): h()

def h(): i()

def i(): j()

def j(): k()

def k(): l()

l = lambda: test()

def test():
    exec "import string; string.capwords(None)" in {}

k()
