import sys

class _BaseLayout:
    pass

class WindowsLayout(_BaseLayout):
    pass

class GCCSysVLayout(_BaseLayout):
    pass

if sys.platform == 'win32':
    NativeLayout = WindowsLayout
else:
    NativeLayout = GCCSysVLayout
