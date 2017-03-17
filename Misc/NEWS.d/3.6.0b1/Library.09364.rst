Issue #10513: Fix a regression in Connection.commit().  Statements should
not be reset after a commit.