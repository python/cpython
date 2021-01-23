class Person(object):
    def __init__(self, name):
        self.name = name
    
    def greet(self, other):
        return "Hello, " + other.name + "!"
    
bob = Person("Robert")
james = Person("Jimmy")

bob.greet(james)