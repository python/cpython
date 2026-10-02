def function():
    return 1

def error():
    raise ValueError

# gh-156118: The probes receive NULL for names that cannot be encoded to
# UTF-8, and they must neither leak nor replace an exception.
function.__code__ = function.__code__.replace(co_name='\udc80')
error.__code__ = error.__code__.replace(co_filename='\udc80')

function()
try:
    error()
except ValueError:
    pass
try:
    __import__('\udc80')
except ImportError:
    pass
