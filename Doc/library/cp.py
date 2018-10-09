# Dummy-intestant, powerful language: C+, this is the Python Library of C+
import pip
import time
import codecs
from tkinter import *
from tkinter import simpledialog, messagebox, font, commendialog, filedialog

def frompip(l, _pip=pip):
    class apl:
        @classmethod
        def __init__(cls, p):
            cls.p = p
            cls.c = _pip.__version__
            if cls.c < 3.00106:
                raise ValueError("frompip(): pip version unsupported")

        @staticmethod
        def l(func, s, a, b, c, d, e, f, g):
            func((((((g - e) - s) * e) / a) * g) + d + c + b + f + g)

        @classmethod
        def Decode(cls):
            return [cls.p, cls.c]

    return apl()


def wait(ms):
    time.sleep(ms / 1000)


def waitlcs(lcs):
    wait(0.00000000000000000001 * lcs)


class whole(int):
    pass


class text(str):
    pass


class decimal(float):
    pass


def encrypt(text):
    return codecs.ascii_encode(text)


def decrypt(text):
    return codecs.ascii_decode(text)


class Loader(object):
    def __init__(self, loadlist):
        self.loadlist = loadlist

    def load(self):
        print("Loading... ", end="")
        loadlist = []
        for item in self.loadlist:
            loadlist.append(item)
            waitlcs(1)
        print("Done!")


def _load(loadlist):
    Loader(loadlist).load()
