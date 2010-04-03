"""codefragments.py -- wrapper to modify code fragments."""

# (c) 1998, Just van Rossum, Letterror

__version__ = "0.8b3"
__author__ = "jvr"

import warnings
warnings.warnpy3k("the cfmfile module is deprecated and is removed in 3,0",
              stacklevel=2)

import Carbon.File
import struct
from Carbon import Res
import os
import sys

DEBUG = 0

error = "cfm.error"

BUFSIZE = 0x80000

def mergecfmfiles(srclist, dst, architecture = 'fat'):
    """Merge all files in srclist into a new file dst.

    If architecture is given, only code fragments of that type will be used:
    "pwpc" for PPC, "m68k" for cfm68k. This does not work for "classic"
    68k code, since it does not use code fragments to begin with.
    If architecture is None, all fragments will be used, enabling FAT binaries.
    """

    srclist = list(srclist)
    for i in range(len(srclist)):
        srclist[i] = Carbon.File.pathname(srclist[i])
    dst = Carbon.File.pathname(dst)

    dstfile = open(dst, "wb")
    rf = Res.FSpOpenResFile(dst, 3)
    try:
        dstcfrg = CfrgResource()
        for src in srclist:
            srccfrg = CfrgResource(src)
            for frag in srccfrg.fragments:
                if frag.architecture == 'pwpc' and architecture == 'm68k':
                    continue
                if frag.architecture == 'm68k' and architecture == 'pwpc':
                    continue
                dstcfrg.append(frag)

                frag.copydata(dstfile)

        cfrgres = Res.Resource(dstcfrg.build())
        Res.UseResFile(rf)
        cfrgres.AddResource('cfrg', 0, "")
    finally:
        dstfile.close()
        rf = Res.CloseResFile(rf)


class CfrgResource:

    def __init__(self, path = None):
        self.version = 1
        self.fragments = []
        self.path = path
        if path is not None and os.path.exists(path):
            currentresref = Res.CurResFile()
            resref = Res.FSpOpenResFile(path, 1)
            Res.UseResFile(resref)
            try:
                try:
                    data = Res.Get1Resource('cfrg', 0).data
                except Res.Error:
                    raise Res.Error, "no 'cfrg' resource found", sys.exc_traceback
            finally:
                Res.CloseResFile(resref)
                Res.UseResFile(currentresref)
            self.parse(data)
            if self.version != 1:
                raise error, "unknown 'cfrg' resource format"

    def parse(self, data):
        (res1, res2, self.version,
            res3, res4, res5, res6,
            self.memberCount) = struct.unpack("8l", data[:32])
        data = data[32:]
        while data:
            frag = FragmentDescriptor(self.path, data)
            data = data[frag.memberSize:]
            self.fragments.append(frag)

    def build(self):
        self.memberCount = len(self.fragments)
        data = struct.pack("8l", 0, 0, self.version, 0, 0, 0, 0, self.memberCount)
        for frag in self.fragments:
            data = data + frag.build()
        return data

    def append(self, frag):
        self.fragments.append(frag)


class FragmentDescriptor:

    def __init__(self, path, data = None):
        self.path = path
        if data is not None:
            self.parse(data)

    def parse(self, data):
        self.architecture = data[:4]
        (   self.updatelevel,
            self.currentVersion,
            self.oldDefVersion,
            self.stacksize,
            self.applibdir,
            self.fragtype,
            self.where,
            self.offset,
            self.length,
            self.res1, self.res2,
            self.memberSize,) = struct.unpack("4lhBB4lh", data[4:42])
        pname = data[42:self.memberSize]
        self.name = pname[1:1+ord(pname[0])]

    def build(self):
        data = self.architecture
        data = data + struct.pack("4lhBB4l",
                self.updatelevel,
                self.currentVersion,
                self.oldDefVersion,
                self.stacksize,
                self.applibdir,
                self.fragtype,
                self.where,
                self.offset,
                self.length,
                self.res1, self.res2)
        self.memberSize = len(data) + 2 + 1 + len(self.name)
        # pad to 4 byte boundaries
        if self.memberSize % 4:
            self.memberSize = self.memberSize + 4 - (self.memberSize % 4)
        data = data + struct.pack("hb", self.memberSize, len(self.name))
        data = data + self.name
        data = data + '\000' * (self.memberSize - len(data))
        return data

    def getfragment(self):
        if self.where != 1:
            raise error, "can't read fragment, unsupported location"
        f = open(self.path, "rb")
        f.seek(self.offset)
        if self.length:
            frag = f.read(self.length)
        else:
            frag = f.read()
        f.close()
        return frag

    def copydata(self, outfile):
        if self.where != 1:
            raise error, "can't read fragment, unsupported location"
        infile = open(self.path, "rb")
        if self.length == 0:
            infile.seek(0, 2)
            self.length = infile.tell()

        # Position input file and record new offset from output file
        infile.seek(self.offset)

        # pad to 16 byte boundaries
        offset = outfile.tell()
        if offset % 16:
            offset = offset + 16 - (offset % 16)
        outfile.seek(offset)
        self.offset = offset

        l = self.length
        while l:
            if l > BUFSIZE:
                outfile.write(infile.read(BUFSIZE))
                l = l - BUFSIZE
            else:
                outfile.write(infile.read(l))
                l = 0
        infile.close()
