/* Fudge unix isatty and fileno for RISCOS */

#include "h.unixstuff"
#include <math.h>
#include "h.osfile"

int fileno(FILE *f)
{ return (int)f;
}

int isatty(int fn)
{ return (fn==fileno(stdin));
}

bits unixtime(bits ld,bits ex)
{ ld&=0xFF;
  ld-=51;
  if(ex<1855548004U) ld--;
  ex-=1855548004U;
  return ex/100+42949672*ld+(95*ld)/100;
}

int unlink(char *fname)
{ remove(fname);
  return 0;
}


/*#define RET(k) {printf(" %d\n",k);return k;}*/
#define RET(k) return k

int isdir(char *fn)
{ int ob;
/*  printf("isdir %s",fn);*/
  if(xosfile_read_stamped_no_path(fn,&ob,0,0,0,0,0)) RET(0);
  switch (ob)
  {   case osfile_IS_DIR:RET(1);
    case osfile_IS_IMAGE:RET(1);
  }
  RET(0);
}

int isfile(char *fn)
{ int ob;  /*printf("isfile %s",fn);*/
  if(xosfile_read_stamped_no_path(fn,&ob,0,0,0,0,0)) RET(0);
  switch (ob)
  {  case osfile_IS_FILE:RET(1);
    case osfile_IS_IMAGE:RET(1);
  }
  RET(0);
}

int exists(char *fn)
{ int ob;  /*printf("exists %s",fn);*/
  if(xosfile_read_stamped_no_path(fn,&ob,0,0,0,0,0)) RET(0);
  switch (ob)
  {  case osfile_IS_FILE:RET(1);
      case osfile_IS_DIR:RET(1);
    case osfile_IS_IMAGE:RET(1);
  }
  RET(0);
}
