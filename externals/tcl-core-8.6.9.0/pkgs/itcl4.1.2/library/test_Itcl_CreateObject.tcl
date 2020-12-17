# this is a program for testing the stubs interface ItclCreateObject.
# it uses itclTestRegisterC.c with the call C function functionality,
# so it also tests that feature.
# you need to define in Makefile CFLAGS: -DITCL_DEBUG_C_INTERFACE 
# for makeing that work.
package require itcl

::itcl::class ::c1 {
   public method c0 {args} @cArgFunc
   public method m1 { args } { puts "Hello Tcl $args" }
}

set obj1 [::c1 #auto ]
$obj1 m1 World

# C method cargFunc implements a call to Itcl_CreateObject!
#
# args for method c0 of class ::c1
# arg1 does not matter
# arg2 is the class name
# arg3 is the full class name (full path name)
# arg4 is the object name of the created Itcl object
set obj2 [$obj1 c0 ::itcl::parser::handleClass ::c1 ::c1 ::c1::c11]
# test, if it is working!
$obj2 m1 Folks

