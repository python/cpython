Issue #20120: Use RawConfigParser for .pypirc parsing,
removing support for interpolation unintentionally added
with move to Python 3. Behavior no longer does any
interpolation in .pypirc files, matching behavior in Python
2.7 and Setuptools 19.0.