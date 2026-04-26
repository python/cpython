#include "expat_config.h"

#if defined(HAVE_ARC4RANDOM_BUF)
#  include "random_arc4random_buf.c"
#endif

#if defined(HAVE_ARC4RANDOM)
#  include "random_arc4random.c"
#endif

#if defined(_WIN32)
#  include "random_rand_s.c"
#endif

#if defined(HAVE_GETENTROPY)
#  include "random_getentropy.c"
#endif

#if defined(HAVE_GETRANDOM) || defined(HAVE_SYSCALL_GETRANDOM)
#  include "random_getrandom.c"
#endif

#if ! defined(_WIN32) && defined(XML_DEV_URANDOM)
#  include "random_dev_urandom.c"
#endif
