def fib(n):
    if n == 1 or n == 2:
        return 1
    a = fib(n - 1)
    b = fib(n - 2)
    return a + b


def main():
    result = fib(5)
    print(result)