/*
The Keccak sponge function, designed by Guido Bertoni, Joan Daemen,
Michaël Peeters and Gilles Van Assche. For more information, feedback or
questions, please refer to our website: http://keccak.noekeon.org/

Implementation by the designers,
hereby denoted as "the implementer".

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#ifdef UseSchedule
    #if (UseSchedule == 1)
        #include "KeccakF-1600-32-s1.macros"
    #elif (UseSchedule == 2)
        #include "KeccakF-1600-32-s2.macros"
    #elif (UseSchedule == 3)
        #include "KeccakF-1600-32-rvk.macros"
    #else
        #error "This schedule is not supported."
    #endif
#else
    #include "KeccakF-1600-32-s1.macros"
#endif
