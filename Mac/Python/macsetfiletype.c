
/*
 *  macsetfiletype - Set the mac's idea of file type
 *
 */
 
#include <Files.h>
#include <pascal.h>

int
setfiletype(name, creator, type)
char *name;
long creator, type;
{
	FInfo info;
	unsigned char *pname;
	
	pname = c2pstr(name);
	if ( GetFInfo(pname, 0, &info) < 0 )
		return -1;
	info.fdType = type;
	info.fdCreator = creator;
	return SetFInfo(pname, 0, &info);
}

