# -*- perl -*-
#
# This implements the Python manual class.  All it really needs to do it
# load the "python" style.  The style code is not moved into the class code
# at this time, since we expect additional document class to be developed
# for the Python documentation in the future.  Appropriate relocations will
# be made at that time.

package main;

do_require_package("report");
do_require_package("alltt");
do_require_package("python");

1;				# sheesh....
