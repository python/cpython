import rotor

r = rotor.newrotor("you'll never guess this")
r = rotor.newrotor("you'll never guess this", 12)

A = 'spam and eggs'
B = 'cheese shop'

a = r.encrypt(A)
print `a`
b = r.encryptmore(B)
print `b`

A1 = r.decrypt(a)
print A1
if A1 <> A:
    print 'decrypt failed'

B1 = r.decryptmore(b)
print B1
if B1 <> B:
    print 'decryptmore failed'

try:
    r.setkey()
except TypeError:
    pass
r.setkey('you guessed it!')
