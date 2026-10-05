# Module that raises an exception with context during import
try:
    raise ValueError("Cause of failure")
except:
    raise ValueError("This module always fails to import")
