from . import a

try:
    a.__annotations__
except Exception as e:
    exc = e
else:
    exc = None
