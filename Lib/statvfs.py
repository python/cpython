# Module 'statvfs'
#
# Defines constants for interpreting statvfs struct as returned
# by os.statvfs() and os.fstatvfs() (if they exist).
#

# Indices for statvfs struct members in tuple returned by
# os.statvfs() and os.fstatvfs()

F_BSIZE   = 0
F_FRSIZE  = 1
F_BLOCKS  = 2
F_BFREE   = 3
F_BAVAIL  = 4
F_FILES   = 5
F_FFREE   = 6
F_FAVAIL  = 7
F_FSID    = 8
F_FLAG    = 9
F_NAMEMAX = 10
