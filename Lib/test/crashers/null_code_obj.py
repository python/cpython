import platform

"""
This execute a payload code object which crashes the interpeter for different
python versions. Currently there is no bytecode verifier to prevent those
crashes related to malformed bytecode
"""


def crash_27():
    exec((lambda f:f).func_code.__class__(0,0,0,0,"\x01",(),(),(),"","",0,""))


def crash_36():
    exec((lambda f:f).__code__.__class__(0,0,0,0,0,b"\x01",(),(),(),"","",0,b""))


def crash_38():
    exec((lambda f: f).__code__.__class__(0,0,0,0,0,0,b"\x01",(),(),(),"","",0,b""))


def crash():
    locals()['crash_27'] = crash_27
    locals()['crash_36'] = crash_36
    locals()['crash_37'] = crash_36
    locals()['crash_38'] = crash_38
    locals()['crash_39'] = crash_38
    locals()['crash_310'] = crash_38

    py_ver = "".join(platform.python_version_tuple()[0:2])
    crash_func = "crash_{}".format(py_ver)
    locals()[crash_func]()


if __name__ == '__main__':
    crash()
