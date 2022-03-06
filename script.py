import faulthandler
import ctypes
faulthandler.enable()
faulthandler.set_context("foo234")

ctypes.string_at(0)
