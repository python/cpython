# Module that raises an exception with suppressed context during import
raise ValueError("This module always fails to import") from None
