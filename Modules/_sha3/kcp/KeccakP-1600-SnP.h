#if KeccakOpt == 64
  #include "KeccakP-1600-SnP-opt64.h"
#elif KeccakOpt == 32
  #include "KeccakP-1600-SnP-opt32.h"
#else
  #error "No KeccakOpt"
#endif
