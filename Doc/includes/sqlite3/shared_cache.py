import sqlite3

# The shared cache is only available in SQLite versions 3.3.3 or later
# See the SQLite documentation for details.

sqlite3.enable_shared_cache(True)
