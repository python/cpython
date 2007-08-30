# List all resources

from Carbon import Res
from Carbon.Resources import *

def list1resources():
    ntypes = Res.Count1Types()
    for itype in range(1, 1+ntypes):
        type = Res.Get1IndType(itype)
        print("Type:", repr(type))
        nresources = Res.Count1Resources(type)
        for i in range(1, 1 + nresources):
            Res.SetResLoad(0)
            res = Res.Get1IndResource(type, i)
            Res.SetResLoad(1)
            info(res)

def listresources():
    ntypes = Res.CountTypes()
    for itype in range(1, 1+ntypes):
        type = Res.GetIndType(itype)
        print("Type:", repr(type))
        nresources = Res.CountResources(type)
        for i in range(1, 1 + nresources):
            Res.SetResLoad(0)
            res = Res.GetIndResource(type, i)
            Res.SetResLoad(1)
            info(res)

def info(res):
    print(res.GetResInfo(), res.SizeResource(), decodeattrs(res.GetResAttrs()))

attrnames = {
        resChanged:     'Changed',
        resPreload:     'Preload',
        resProtected:   'Protected',
        resLocked:      'Locked',
        resPurgeable:   'Purgeable',
        resSysHeap:     'SysHeap',
}

def decodeattrs(attrs):
    names = []
    for bit in range(16):
        mask = 1<<bit
        if attrs & mask:
            if attrnames.has_key(mask):
                names.append(attrnames[mask])
            else:
                names.append(hex(mask))
    return names

def test():
    print("=== Local resourcess ===")
    list1resources()
    print("=== All resources ===")
    listresources()

if __name__ == '__main__':
    test()
