# module 'poly' -- Polynomials

# A polynomial is represented by a list of coefficients, e.g.,
# [1, 10, 5] represents 1*x**0 + 10*x**1 + 5*x**2 (or 1 + 10x + 5x**2).
# There is no way to suppress internal zeros; trailing zeros are
# taken out by normalize().

def normalize(p): # Strip unnecessary zero coefficients
    n = len(p)
    while n:
        if p[n-1]: return p[:n]
        n = n-1
    return []

def plus(a, b):
    if len(a) < len(b): a, b = b, a # make sure a is the longest
    res = a[:] # make a copy
    for i in range(len(b)):
        res[i] = res[i] + b[i]
    return normalize(res)

def minus(a, b):
    neg_b = map(lambda x: -x, b[:])
    return plus(a, neg_b)

def one(power, coeff): # Representation of coeff * x**power
    res = []
    for i in range(power): res.append(0)
    return res + [coeff]

def times(a, b):
    res = []
    for i in range(len(a)):
        for j in range(len(b)):
            res = plus(res, one(i+j, a[i]*b[j]))
    return res

def power(a, n): # Raise polynomial a to the positive integral power n
    if n == 0: return [1]
    if n == 1: return a
    if n/2*2 == n:
        b = power(a, n/2)
        return times(b, b)
    return times(power(a, n-1), a)

def der(a): # First derivative
    res = a[1:]
    for i in range(len(res)):
        res[i] = res[i] * (i+1)
    return res

# Computing a primitive function would require rational arithmetic...
