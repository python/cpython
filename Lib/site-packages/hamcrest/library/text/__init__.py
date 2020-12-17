"""Matchers that perform text comparisons."""

from .isequal_ignoring_case import equal_to_ignoring_case
from .isequal_ignoring_whitespace import equal_to_ignoring_whitespace
from .stringcontains import contains_string
from .stringcontainsinorder import string_contains_in_order
from .stringendswith import ends_with
from .stringmatches import matches_regexp
from .stringstartswith import starts_with

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"
