class WeirdList(list):
    def __init__(self, *args, **kwargs):
        list.__init__(self, *args, **kwargs)

    def __getitem__(self, index):
        if isinstance(index, slice):
            index = slice(index.start+1,
                              index.stop+1,
                              index.step)
        elif isinstance(index, int):
            index += 1
        else:
            raise NotImplementedError
        return list.__getitem__(self, index)


a = WeirdList(range(1000))
a[-1]
