/* "allobjects.c" -- Source for precompiled header "allobjects.h" */

#include <stdio.h>
#include "string.h"

#include "PROTO.h"

#include "object.h"
#include "objimpl.h"

#include "intobject.h"
#include "floatobject.h"
#include "stringobject.h"
#include "tupleobject.h"
#include "listobject.h"
#include "dictobject.h"
#include "methodobject.h"
#include "moduleobject.h"
#include "funcobject.h"
#include "classobject.h"
#include "fileobject.h"

#include "errors.h"
#include "malloc.h"

extern char *strdup PROTO((const char *));
