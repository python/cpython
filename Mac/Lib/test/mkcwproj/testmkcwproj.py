import mkcwproj
import sys

dict = {
	"sysprefix": sys.prefix,
	"sources": ["mkcwtestmodule.c"],
	"extrasearchdirs": [],
}
	
	
mkcwproj.mkproject("mkcwtest.prj", "mkcwtest", dict)
mkcwproj.buildproject("mkcwtest.prj")
