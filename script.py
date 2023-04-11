import faulthandler
import ctypes

faulthandler.enable()
faulthandler.set_context("foo234")
# faulthandler.clear_context()

ctypes.string_at(0)
