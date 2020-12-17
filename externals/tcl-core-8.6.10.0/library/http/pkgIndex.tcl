if {![package vsatisfies [package provide Tcl] 8.6-]} {return}
package ifneeded http 2.9.1 [list tclPkgSetup $dir http 2.9.1 {{http.tcl source {::http::config ::http::formatQuery ::http::geturl ::http::reset ::http::wait ::http::register ::http::unregister ::http::mapReply}}}]
