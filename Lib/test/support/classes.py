# Common test classes.

class WithIndex:
    def __init__(self, value):
        self.value = value

    def __index__(self):
        return self.value


class IntSubclass(int):
    pass

class WithInt:
    def __init__(self, value):
        self.value = value

    def __int__(self):
        return self.value

class WithIntAndIndex:
    def __init__(self, value):
        self.value = value

    def __index__(self):
        return self.value

    def __int__(self):
        return self.value + 12


class FloatSubclass(float):
    pass

class OtherFloatSubclass(float):
    pass

class WithFloat:
    def __init__(self, value):
        self.value = value

    def __float__(self):
        return self.value

class FloatLikeSubclass(float):
    def __init__(self, value):
        self.value = value

    def __float__(self):
        return self.value.__class__(self.value*2)


class ComplexSubclass(complex):
    pass

class OtherComplexSubclass(complex):
    pass

class WithComplex:
    def __init__(self, value):
        self.value = value

    def __complex__(self):
        return self.value

class ComplexLikeSubclass(complex):
    def __init__(self, value):
        self.value = value

    def __complex__(self):
        return self.value.__class__(self.value*2)
