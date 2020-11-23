"""
This execute a payload code object which crashes the interpeter for different
python versions. Currently there is no bytecode verifier to prevent those
crashes related to malformed bytecode
"""

def crash():
    exec((lambda f: f).__code__.__class__(
        0,0,0,0,0,0,b"\x01",(),(),(),"","",0,b""))


if __name__ == '__main__':
    crash()
