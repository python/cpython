import sys

IS_PYREPL_SUPPORTED_PLATFORM = sys.platform != "win32"

# Note: This will be updated on REPL startup based on additional checks.
CAN_USE_PYREPL = IS_PYREPL_SUPPORTED_PLATFORM
