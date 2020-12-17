"""Fundamental matchers of objects and values, and composite matchers."""

from hamcrest.core.core.allof import all_of
from hamcrest.core.core.anyof import any_of
from hamcrest.core.core.described_as import described_as
from hamcrest.core.core.is_ import is_
from hamcrest.core.core.isanything import anything
from hamcrest.core.core.isequal import equal_to
from hamcrest.core.core.isinstanceof import instance_of
from hamcrest.core.core.isnone import none, not_none
from hamcrest.core.core.isnot import is_not, not_
from hamcrest.core.core.issame import same_instance
from hamcrest.core.core.raises import calling, raises

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"
