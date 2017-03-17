Issue #25498: Fix a crash when garbage-collecting ctypes objects created
by wrapping a memoryview.  This was a regression made in 3.5a1.  Based
on patch by Eryksun.