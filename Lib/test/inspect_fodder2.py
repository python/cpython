# line 1
def wrap(foo=None):
  def wrapper(func):
    return func
  return wrapper

# line 7
def replace(func):
  def insteadfunc():
    print 'hello'
  return insteadfunc

# line 13
@wrap()
@wrap(wrap)
def wrapped():
  pass

# line 19
@replace
def gone():
  pass
