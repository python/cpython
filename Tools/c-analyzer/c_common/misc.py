
class Labeled:
    __slots__ = ('_label',)
    def __init__(self, label):
        self._label = label
    def __repr__(self):
        return f'<{self._label}>'
