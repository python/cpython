# Module 'statvfs'
#
# Defines constants for interpreting statvfs struct as returned
# by os.statvfs() and os.fstatvfs() (if they exist).
#
# Suggested usage: from statvfs import *
#

# Indices for statvfs struct members in the tuple returned by
# os.statvfs() and os.fstatvfs().

F_BSIZE   = 0		# Preferred file system block size
F_FRSIZE  = 1		# Fundamental file system block size
F_BLOCKS  = 2		# Total number of file system blocks (FRSIZE)
F_BFREE   = 3		# Total number of free blocks
F_BAVAIL  = 4		# Free blocks available to non-superuser
F_FILES   = 5		# Total number of file nodes
F_FFREE   = 6		# Total number of free file nodes
F_FAVAIL  = 7		# Free nodes available to non-superuser
F_FSID    = 8		# File system ID (meaning is system dependent)
F_FLAG    = 9		# Flags (see your local statvfs man page)
F_NAMEMAX = 10		# Maximum file name length
