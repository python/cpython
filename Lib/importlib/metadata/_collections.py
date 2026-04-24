import typing


class Pair(typing.NamedTuple):
    name: str
    value: str

    @classmethod
    def parse(cls, text):
        return cls(*map(str.strip, text.split("=", 1)))
