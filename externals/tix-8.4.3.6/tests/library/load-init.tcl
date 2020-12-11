# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: load-init.tcl,v 1.2 2002/11/13 21:12:18 idiscovery Exp $
#
# 
#
#

puts -nonewline "trying to load the Tix dynamic library ... "
load ../../unix-tk4.1/libtix.so Tix
puts "done"
