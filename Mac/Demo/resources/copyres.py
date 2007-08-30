from Carbon.Res import *
from Carbon.Resources import *
import MacOS

READ = 1
WRITE = 2
smAllScripts = -3

def raw_input(prompt):
    import sys
    sys.stdout.write(prompt)
    sys.stdout.flush()
    return sys.stdin.readline()

def copyres(src, dst):
    """Copy resource from src file to dst file."""

    cur = CurResFile()
    ctor, type = MacOS.GetCreatorAndType(src)
    input = FSpOpenResFile(src, READ)
    try:
        FSpCreateResFile(dst, ctor, type, smAllScripts)
    except:
        raw_input("%s already exists...  CR to write anyway! " % dst)
    output = FSpOpenResFile(dst, WRITE)
    UseResFile(input)
    ntypes = Count1Types()
    for itype in range(1, 1+ntypes):
        type = Get1IndType(itype)
        nresources = Count1Resources(type)
        for ires in range(1, 1+nresources):
            res = Get1IndResource(type, ires)
            res.LoadResource()
            id, type, name = res.GetResInfo()
            size = res.SizeResource()
            attrs = res.GetResAttrs()
            print(id, type, name, size, hex(attrs))
            res.DetachResource()
            UseResFile(output)
            try:
                res2 = Get1Resource(type, id)
            except (RuntimeError, Res.Error) as msg:
                res2 = None
            if res2:
                print("Duplicate type+id, not copied")
                print (res2.size, res2.data)
                print(res2.GetResInfo())
                if res2.HomeResFile() == output:
                    'OK'
                elif res2.HomeResFile() == input:
                    'BAD!'
                else:
                    print('Home:', res2.HomeResFile())
            else:
                res.AddResource(type, id, name)
                #res.SetResAttrs(attrs)
                res.WriteResource()
            UseResFile(input)
    UseResFile(cur)
    CloseResFile(output)
    CloseResFile(input)

copyres('::python.¹.rsrc', '::foo.rsrc')
