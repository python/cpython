/* Windows mpdecimal.h shim
 *
 * Generally, the mpdecimal library build will copy the correct header into
 * place named "mpdecimal.h", but since we're building it ourselves directly
 * into _decimal.pyd, we need to pick the right one.
 *
 * */

#if defined(_MSC_VER)
  #if defined(CONFIG_64)
    #include <mpdecimal64vc.h>
  #elif defined(CONFIG_32)
    #include <mpdecimal32vc.h>
  #else
    #error "Unknown configuration!"
  #endif
#endif
