import mkcwproject
import sys

dict = {
	"sysprefix": sys.prefix,
	"sources": ["mkcwtestmodule.c"],
	"extrasearchdirs": [],
}
	
	
mkcwproject.mkproject("mkcwtest.prj", "mkcwtest", dict)
mkcwproject.buildproject("mkcwtest.prj")
