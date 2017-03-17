Issue #27570: Avoid zero-length memcpy() etc calls with null source
pointers in the "ctypes" and "array" modules.