#define JISX0201_R_ENCODE(c, assi)                      \
    if ((c) < 0x80 && (c) != 0x5c && (c) != 0x7e) {     \
        (assi) = (c);                                   \
    }                                                   \
    else if ((c) == 0x00a5) {                           \
        (assi) = 0x5c;                                  \
    }                                                   \
    else if ((c) == 0x203e) {                           \
        (assi) = 0x7e;                                  \
    }

#define JISX0201_K_ENCODE(c, assi)                      \
    if ((c) >= 0xff61 && (c) <= 0xff9f) {               \
        (assi) = (c) - 0xfec0;                          \
    }

#define JISX0201_ENCODE(c, assi)                        \
    JISX0201_R_ENCODE(c, assi)                          \
    else JISX0201_K_ENCODE(c, assi)

#define JISX0201_R_DECODE_CHAR(c, assi)                 \
    if ((c) < 0x5c) {                                   \
        (assi) = (c);                                   \
    }                                                   \
    else if ((c) == 0x5c) {                             \
        (assi) = 0x00a5;                                \
    }                                                   \
    else if ((c) < 0x7e) {                              \
        (assi) = (c);                                   \
    }                                                   \
    else if ((c) == 0x7e) {                             \
        (assi) = 0x203e;                                \
    }                                                   \
    else if ((c) == 0x7f) {                             \
        (assi) = 0x7f;                                  \
    }

#define JISX0201_R_DECODE(c, writer)                    \
    if ((c) < 0x5c) {                                   \
        OUTCHAR(c);                                     \
    }                                                   \
    else if ((c) == 0x5c) {                             \
        OUTCHAR(0x00a5);                                \
    }                                                   \
    else if ((c) < 0x7e) {                              \
        OUTCHAR(c);                                     \
    }                                                   \
    else if ((c) == 0x7e) {                             \
        OUTCHAR(0x203e);                                \
    }                                                   \
    else if ((c) == 0x7f) {                             \
        OUTCHAR(0x7f);                                  \
    }

#define JISX0201_K_DECODE(c, writer)                    \
    if ((c) >= 0xa1 && (c) <= 0xdf) {                   \
        OUTCHAR(0xfec0 + (c));                          \
    }
#define JISX0201_K_DECODE_CHAR(c, assi)                 \
    if ((c) >= 0xa1 && (c) <= 0xdf) {                   \
        (assi) = 0xfec0 + (c);                          \
    }
#define JISX0201_DECODE(c, writer)                      \
    JISX0201_R_DECODE(c, writer)                        \
    else JISX0201_K_DECODE(c, writer)
