Issue #25399: Optimize bytearray % args using the new private _PyBytesWriter
API. Formatting is now between 2.5 and 5 times faster.