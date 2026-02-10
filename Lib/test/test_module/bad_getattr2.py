def __getattr__():
    "Bad one"

x = 1

def __dir__(bad_sig):
    return []
