# Demonstration of the fix for gh-143304
# This shows the before and after behavior

print("=" * 60)
print("BEFORE THE FIX (main branch):")
print("=" * 60)
print("""
def _load_library(self, name, mode, handle, winmode):
    # ... iOS/tvOS/watchOS .fwork handling ...
    # ... AIX archive handling ...
    self._name = name
    return _dlopen(name, mode)  # ❌ handle parameter is IGNORED!
""")

print("\n" + "=" * 60)
print("AFTER THE FIX (our branch):")
print("=" * 60)
print("""
def _load_library(self, name, mode, handle, winmode):
    # ... iOS/tvOS/watchOS .fwork handling ...
    # ... AIX archive handling ...
    self._name = name
    if handle is not None:      # ✅ Check if handle was provided
        return handle           # ✅ Use the provided handle
    return _dlopen(name, mode)  # Only call _dlopen if no handle
""")

print("\n" + "=" * 60)
print("COMPARISON WITH WINDOWS VERSION:")
print("=" * 60)
print("""
Windows _load_library (already correct):
    self._name = name
    if handle is not None:
        return handle
    return _LoadLibrary(self._name, winmode)

POSIX _load_library (now fixed to match):
    self._name = name
    if handle is not None:      # ← This was missing!
        return handle           # ← This was missing!
    return _dlopen(name, mode)
""")

print("\n" + "=" * 60)
print("VERIFICATION:")
print("=" * 60)
print("✓ The fix adds 2 lines to the POSIX implementation")
print("✓ The fix matches the Windows implementation pattern")
print("✓ The fix allows users to pass an existing handle to CDLL")
print("✓ The fix prevents unnecessary _dlopen() calls when handle is provided")
