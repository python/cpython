/* These routines may be quite inefficient, but it's used only to emulate old
 * standards. */

#ifndef EMULATE_JISX0213_2000_ENCODE_INVALID
#define EMULATE_JISX0213_2000_ENCODE_INVALID 1
#endif

#define EMULATE_JISX0213_2000_ENCODE_BMP(assi, c)                       \
    if (config == (void *)2000 && (                                     \
                    (c) == 0x9B1C || (c) == 0x4FF1 ||                   \
                    (c) == 0x525D || (c) == 0x541E ||                   \
                    (c) == 0x5653 || (c) == 0x59F8 ||                   \
                    (c) == 0x5C5B || (c) == 0x5E77 ||                   \
                    (c) == 0x7626 || (c) == 0x7E6B))                    \
        return EMULATE_JISX0213_2000_ENCODE_INVALID;                    \
    else if (config == (void *)2000 && (c) == 0x9B1D)                   \
        (assi) = 0x8000 | 0x7d3b;                                       \

#define EMULATE_JISX0213_2000_ENCODE_EMP(assi, c)                       \
    if (config == (void *)2000 && (c) == 0x20B9F)                       \
        return EMULATE_JISX0213_2000_ENCODE_INVALID;

#ifndef EMULATE_JISX0213_2000_DECODE_INVALID
#define EMULATE_JISX0213_2000_DECODE_INVALID 2
#endif

#define EMULATE_JISX0213_2000_DECODE_PLANE1(assi, c1, c2)               \
    if (config == (void *)2000 &&                                       \
                    (((c1) == 0x2E && (c2) == 0x21) ||                  \
                     ((c1) == 0x2F && (c2) == 0x7E) ||                  \
                     ((c1) == 0x4F && (c2) == 0x54) ||                  \
                     ((c1) == 0x4F && (c2) == 0x7E) ||                  \
                     ((c1) == 0x74 && (c2) == 0x27) ||                  \
                     ((c1) == 0x7E && (c2) == 0x7A) ||                  \
                     ((c1) == 0x7E && (c2) == 0x7B) ||                  \
                     ((c1) == 0x7E && (c2) == 0x7C) ||                  \
                     ((c1) == 0x7E && (c2) == 0x7D) ||                  \
                     ((c1) == 0x7E && (c2) == 0x7E)))                   \
        return EMULATE_JISX0213_2000_DECODE_INVALID;

#define EMULATE_JISX0213_2000_DECODE_PLANE2(assi, c1, c2)               \
    if (config == (void *)2000 && (c1) == 0x7D && (c2) == 0x3B)         \
        (assi) = 0x9B1D;
