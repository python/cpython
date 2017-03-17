Issue #27861: Fixed a crash in sqlite3.Connection.cursor() when a factory
creates not a cursor.  Patch by Xiang Zhang.