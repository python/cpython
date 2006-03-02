import sys

if sys.platform != 'darwin':
    raise ValueError, "This test only leaks on Mac OS X"

def leak():
    # taken from platform._mac_ver_lookup()
    from gestalt import gestalt
    import MacOS

    try:
        gestalt('sysu')
    except MacOS.Error:
        pass
