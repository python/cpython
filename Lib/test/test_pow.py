import sys
import test_support

def powtest(type):
    if type != float: 
        print "    Testing 2-argument pow() function..."
        for i in range(-1000, 1000):
            if pow(type(i), 0) != 1: 
                raise ValueError, 'pow('+str(i)+',0) != 1'
            if pow(type(i), 1) != type(i):
                raise ValueError, 'pow('+str(i)+',1) != '+str(i)
            if pow(type(0), 1) != type(0):
                raise ValueError, 'pow(0,'+str(i)+') != 0'
            if pow(type(1), 1) != type(1):
                raise ValueError, 'pow(1,'+str(i)+') != 1'

        for i in range(-100, 100):
            if pow(type(i), 3) != i*i*i:
                raise ValueError, 'pow('+str(i)+',3) != '+str(i*i*i)
    
        pow2 = 1
        for i in range(0,31):
            if pow(2, i) != pow2:
                raise ValueError, 'pow(2,'+str(i)+') != '+str(pow2)
            if i != 30 : pow2 = pow2*2

        for othertype in int, long:
            for i in range(-10, 0) + range(1, 10):
                ii = type(i)
                for j in range(1, 11):
                    jj = -othertype(j)
                    try:
                        pow(ii, jj)
                    except ValueError:
                        pass # taking an int to a neg int power should fail
                    else:
                        raise ValueError, "pow(%s, %s) did not fail" % (ii, jj)

    for othertype in int, long, float:
        for i in range(1, 100):
            zero = type(0)
            exp = -othertype(i/10.0)
            if exp == 0:
                continue
            try:
                pow(zero, exp)
            except ZeroDivisionError:
                pass # taking zero to any negative exponent should fail
            else:
                raise ValueError, "pow(%s, %s) did not fail" % (zero, exp)

    print "    Testing 3-argument pow() function..."
    il, ih = -20, 20
    jl, jh = -5,   5
    kl, kh = -10, 10
    compare = cmp
    if type == float:
        il = 1
        compare = test_support.fcmp
    elif type == int:
        jl = 0
    elif type == long:
        jl, jh = 0, 15
    for i in range(il, ih+1):
         for j in range(jl, jh+1):
             for k in range(kl, kh+1):
                 if k != 0:
                     if compare(pow(type(i),j,k), pow(type(i),j)% type(k)):
                         raise ValueError, "pow(" +str(i)+ "," +str(j)+ \
                              "," +str(k)+ ") != pow(" +str(i)+ "," + \
                              str(j)+ ") % " +str(k)


print 'Testing integer mode...'
powtest(int)
print 'Testing long integer mode...'
powtest(long)
print 'Testing floating point mode...'
powtest(float)

# Other tests-- not very systematic

print 'The number in both columns should match.'
print `pow(3,3) % 8`, `pow(3,3,8)`
print `pow(3,3) % -8`, `pow(3,3,-8)`
print `pow(3,2) % -2`, `pow(3,2,-2)`
print `pow(-3,3) % 8`, `pow(-3,3,8)`
print `pow(-3,3) % -8`, `pow(-3,3,-8)`
print `pow(5,2) % -8`, `pow(5,2,-8)`
print

print `pow(3L,3L) % 8`, `pow(3L,3L,8)`
print `pow(3L,3L) % -8`, `pow(3L,3L,-8)`
print `pow(3L,2) % -2`, `pow(3L,2,-2)`
print `pow(-3L,3L) % 8`, `pow(-3L,3L,8)`
print `pow(-3L,3L) % -8`, `pow(-3L,3L,-8)`
print `pow(5L,2) % -8`, `pow(5L,2,-8)`
print

print pow(3.0,3.0) % 8, pow(3.0,3.0,8)
print pow(3.0,3.0) % -8, pow(3.0,3.0,-8)
print pow(3.0,2) % -2, pow(3.0,2,-2)
print pow(5.0,2) % -8, pow(5.0,2,-8)
print

for i in range(-10, 11):
 for j in range(0, 6):
  for k in range(-7, 11):
   if j >= 0 and k != 0:
    o = pow(i,j) % k
    n = pow(i,j,k)
    if o != n: print 'Integer mismatch:', i,j,k
   if j >= 0 and k <> 0:
    o = pow(long(i),j) % k
    n = pow(long(i),j,k)
    if o != n: print 'Long mismatch:', i,j,k
   if i >= 0 and k <> 0:
     o = pow(float(i),j) % k
     n = pow(float(i),j,k)
     if o != n: print 'Float mismatch:', i,j,k
