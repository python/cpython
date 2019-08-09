class SwagList(list):
    def __init__(self, *args, **kwargs):
        list.__init__(self, *args, **kwargs)

    def __getitem__(self, index):
        if index == 69:
            print('python: Nice!')
        elif index == 420:
            print('python: Blaze it, homie')
        return list.__getitem__(self, index)


a = SwagList(range(1000))
a[69]
