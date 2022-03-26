def func():
    b = bytearray()
    b.translate(b'a'*256)

for i in range(10):
    func()