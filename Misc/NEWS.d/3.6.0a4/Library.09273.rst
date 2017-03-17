Issue #27130: In the "zlib" module, fix handling of large buffers
(typically 4 GiB) when compressing and decompressing.  Previously, inputs
were limited to 4 GiB, and compression and decompression operations did not
properly handle results of 4 GiB.