# from http://mail.python.org/pipermail/python-dev/2001-June/015239.html

# if you keep changing a dictionary while looking up a key, you can
# provoke an infinite recursion in C

# At the time neither Tim nor Michael could be bothered to think of a
# way to fix it.

class Yuck:
    def __init__(self):
        self.i = 0

    def make_dangerous(self):
        self.i = 1

    def __hash__(self):
        # direct to slot 4 in table of size 8; slot 12 when size 16
        return 4 + 8

    def __eq__(self, other):
        if self.i == 0:
            # leave dict alone
            pass
        elif self.i == 1:
            # fiddle to 16 slots
            self.__fill_dict(6)
            self.i = 2
        else:
            # fiddle to 8 slots
            self.__fill_dict(4)
            self.i = 1

        return 1

    def __fill_dict(self, n):
        self.i = 0
        dict.clear()
        for i in range(n):
            dict[i] = i
        dict[self] = "OK!"

y = Yuck()
dict = {y: "OK!"}

z = Yuck()
y.make_dangerous()
print(dict[z])
