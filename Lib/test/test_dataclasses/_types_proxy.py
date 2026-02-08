# We need this to test a case when a type
# is imported via some other package,
# like ClassVar from typing_extensions instead of typing.
# https://github.com/python/cpython/issues/133956
from typing import ClassVar
from dataclasses import InitVar
