import sys

class _BaseLayout:
    def __init__(self, cls, fields, is_struct):
        self.cls = cls
        self.align = getattr(cls, '_align_', 1);

class WindowsLayout(_BaseLayout):
    pass

class GCCSysVLayout(_BaseLayout):
    pass

if sys.platform == 'win32':
    NativeLayout = WindowsLayout
else:
    NativeLayout = GCCSysVLayout
