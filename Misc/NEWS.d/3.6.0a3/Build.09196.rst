Issue #23968: Rename the platform directory from plat-$(MACHDEP) to
plat-$(PLATFORM_TRIPLET).
Rename the config directory (LIBPL) from config-$(LDVERSION) to
config-$(LDVERSION)-$(PLATFORM_TRIPLET).
Install the platform specifc _sysconfigdata module into the platform
directory and rename it to include the ABIFLAGS.