from warnings import deprecated

@deprecated("Test")
class A:
    def __init_subclass__(self, **kwargs):
        pass

class B(A):
    pass
