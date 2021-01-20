class MyKey(object):
    def __init__(self, id, name):
        self.id = id
        self.name = name

    def __eq__(self, other):
        return self.id == other.id
    
    def __hash__(self):
        return self.id
    
    def __repr__(self):
        return "key(" + str(self.id) + ", " + self.name + ")"

class MyKeyB(object):
    def __init__(self, id, name):
        self.id = id
        self.name = name

    def __eq__(self, other):
        return self.id == other.id and self.name == other.name
    
    def __hash__(self):
        return hash((self.id, self.name))
    
    def __repr__(self):
        return "key(" + str(self.id) + ", " + self.name + ")"

def test1():
    key1 = MyKey(1, "baby")
    key2 = MyKey(1, "mommy")
    
    e = { key1: 2 }
    e[key2] = 3
    print("test1", e)

def test2():
    key1 = MyKeyB(1, "baby")
    key2 = MyKeyB(1, "mommy")
    
    e = { key1: 2 }
    e[key2] = 3
    print("test2", e)


def test3():
    tel = {'jack': 4098, 'sape': 4139}
    tel['guido'] = 4127
    del tel['sape']
    tel['irv'] = 4127
    guido = tel.pop('guido')
    item = tel.popitem()
    tel.update({ 'babara': 3323, 'jerry': 9483 })
    tel.update([('mary', 2934)])

    tel2 = dict([('sape', 4139), ('guido', 4127), ('jack', 4098)])
    tel3 = dict(sape=4139, guido=4127, jack=4098)
    d2 = {x: x**2 for x in (2, 4, 6)}

    tel2.clear()

test1()
test2()
test3()