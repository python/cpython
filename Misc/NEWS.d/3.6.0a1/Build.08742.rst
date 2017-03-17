Issue #17603: Avoid error about nonexistant fileblocks.o file by using a
lower-level check for st_blocks in struct stat.