#ifndef ASM_TRAMPOLINE_AARCH_64_H_
#define ASM_TRAMPOLINE_AARCH_64_H_

/*
 * References:
 *  - https://developer.arm.com/documentation/101028/0012/5--Feature-test-macros
 *  - https://github.com/ARM-software/abi-aa/blob/main/aaelf64/aaelf64.rst
 */

#if defined(__ARM_FEATURE_BTI_DEFAULT) && __ARM_FEATURE_BTI_DEFAULT == 1
  #define BTI_J hint 36 /* bti j: for jumps, IE br instructions */
  #define BTI_C hint 34  /* bti c: for calls, IE bl instructions */
  #define GNU_PROPERTY_AARCH64_BTI 1 /* bit 0 GNU Notes is for BTI support */
#else
  #define BTI_J
  #define BTI_C
  #define GNU_PROPERTY_AARCH64_BTI 0
#endif

#if defined(__ARM_FEATURE_PAC_DEFAULT)
  #if __ARM_FEATURE_PAC_DEFAULT & 1
    #define SIGN_LR hint 25 /* paciasp: sign with the A key */
    #define VERIFY_LR hint 29 /* autiasp: verify with the A key */
  #elif __ARM_FEATURE_PAC_DEFAULT & 2
    #define SIGN_LR hint 27 /* pacibsp: sign with the b key */
    #define VERIFY_LR hint 31 /* autibsp: verify with the b key */
  #endif
  #define GNU_PROPERTY_AARCH64_POINTER_AUTH 2 /* bit 1 GNU Notes is for PAC support */
#else
  #define SIGN_LR BTI_C
  #define VERIFY_LR
  #define GNU_PROPERTY_AARCH64_POINTER_AUTH 0
#endif

/* Add the BTI and PAC support to GNU Notes section */
#if GNU_PROPERTY_AARCH64_BTI != 0 || GNU_PROPERTY_AARCH64_POINTER_AUTH != 0
    .pushsection .note.gnu.property, "a"; /* Start a new allocatable section */
    .balign 8; /* align it on a byte boundry */
    .long 4; /* size of "GNU\0" */
    .long 0x10; /* size of descriptor */
    .long 0x5; /* NT_GNU_PROPERTY_TYPE_0 */
    .asciz "GNU";
    .long 0xc0000000; /* GNU_PROPERTY_AARCH64_FEATURE_1_AND */
    .long 4; /* Four bytes of data */
    .long (GNU_PROPERTY_AARCH64_BTI|GNU_PROPERTY_AARCH64_POINTER_AUTH); /* BTI or PAC is enabled */
    .long 0; /* padding for 8 byte alignment */
    .popsection; /* end the section */
#endif

#endif
