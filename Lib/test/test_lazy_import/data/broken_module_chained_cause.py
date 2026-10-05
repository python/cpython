# Module that raises an exception with explicit cause during import
cause = ValueError("Cause of failure")
raise ValueError("This module always fails to import") from cause
