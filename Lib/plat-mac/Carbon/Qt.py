from _Qt import *
try:
    _ = AddFilePreview
except:
    raise ImportError, "Old (2.3) _Qt.so module loaded in stead of new (2.4) _Qt.so"
