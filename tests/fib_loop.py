def fib(n):
    i = 3
    a = 1
    b = 1
    while i < n:
        a, b = b, a + b
        i += 1
    return b

result = fib(8)
print(result)