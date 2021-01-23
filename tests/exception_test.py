def h():
    print("called h")
    raise Exception("oof")

def g():
    print("called g")
    h()

def f():
    print("called f")
    g()

try:
    f()
except:
    print("caught exception")