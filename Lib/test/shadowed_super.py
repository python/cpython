class super:
    msg = "truly super"


class C:
    def method(self):
        return super().msg
