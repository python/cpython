
class X:
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return str(self.value)
    def __format__(self, fmt):
        assert fmt[0] == '='
        self.value = eval(fmt[1:])
        return ''

x = X(3)
print(x)
f'{x:=4}'   # Behold!
print(x)
