# Common test classes.

class MyIndex:
    def __init__(self, value):
        self.value = value

    def __index__(self):
        return self.value


class MyInt:
    def __init__(self, value):
        self.value = value

    def __int__(self):
        return self.value


class FloatSubclass(float):
    pass

class OtherFloatSubclass(float):
    pass

class FloatLike:
    def __init__(self, value):
        self.value = value

    def __float__(self):
        return self.value

class FloatLikeSubclass(float):
    def __init__(self, value):
        self.value = value

    def __float__(self):
        return self.value


class ComplexSubclass(complex):
    pass

class OtherComplexSubclass(complex):
    pass

class ComplexLike:
    def __init__(self, value):
        self.value = value

    def __complex__(self):
        return self.value

class ComplexLikeSubclass(complex):
    def __init__(self, value):
        self.value = value

    def __complex__(self):
        return self.value
