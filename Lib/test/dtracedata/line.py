def test_line():
    a = 1
    print('# Preamble', a)
    for i in range(2):
        a = i
        b = i+2
        c = i+3
        if c < 4:
            a = c
        d = a + b +c
        print('#', a, b, c, d)
    a = 1
    print('# Epilogue', a)


if __name__ == '__main__':
    test_line()
