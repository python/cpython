#include <stdio.h>

#define __swi
#include "oslib/osfile.h"

long PyOS_GetLastModificationTime(char *path, FILE *fp)
{
  int obj;
  bits load, exec, ftype;

  if (xosfile_read_stamped_no_path(path, &obj, &load, &exec, 0, 0, &ftype)) return -1;
  if (obj != osfile_IS_FILE) return -1;
  if (ftype == osfile_TYPE_UNTYPED) return -1;

  load &= 0xFF;
  load -= 51;
  if (exec < 1855548004U) load--;
  exec -= 1855548004U;
  return exec/100+42949672*load+(95*load)/100;
}
