def evens():
    i = 0
    while True:
        i += 1
        if i % 2 == 0:
            yield i
