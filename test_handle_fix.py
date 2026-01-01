import ctypes
import sys
import os

print(f"OS name: {os.name}")
print(f"Platform: {sys.platform}")

if os.name == "nt":
    print("\nTesting Windows WinDLL...")
    try:
        # Get a handle from kernel32
        handle = ctypes.windll.kernel32._handle
        print(f"Got kernel32 handle: {handle}")
        
        # Try to create a new WinDLL with that handle
        print("Creating WinDLL with name=None and handle...")
        lib = ctypes.WinDLL(name=None, handle=handle)
        print(f"Success! Created lib with handle: {lib._handle}")
        print(f"Handles match: {handle == lib._handle}")
    except Exception as e:
        print(f"Error: {type(e).__name__}: {e}")
        import traceback
        traceback.print_exc()
else:
    print("Not on Windows, skipping test")
