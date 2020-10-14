/* stringlib: fastsearch implementation */

#define STRINGLIB_FASTSEARCH_H


/* FAST_SEARCH and FAST_COUNT use the two-way algorithm. See:
      http://www-igm.univ-mlv.fr/~lecroq/string/node26.html#SECTION00260
       https://en.wikipedia.org/wiki/Two-way_string-matching_algorithm
   Largely influenced by glibc:
       https://code.woboq.org/userspace/glibc/string/str-two-way.h.html
       https://code.woboq.org/userspace/glibc/string/memmem.c.html

   FAST_RSEARCH uses another algorithm, based on a mix between boyer-
   moore and horspool, with a few more bells and whistles on the top.
   for some more background, see: http://effbot.org/zone/stringlib.htm */

#define FAST_COUNT 0
#define FAST_SEARCH 1
#define FAST_RSEARCH 2

/* Change to a 1 to see logging comments walk through the algorithm. */
#if 0 && STRINGLIB_SIZEOF_CHAR == 1
#define LOG(...) printf(__VA_ARGS__)
#define LOG_STRING(s, n) printf("\"%.*s\"", n, s)
#else
#define LOG(...)
#define LOG_STRING(s, n)
#endif

#if LONG_BIT >= 128
#define STRINGLIB_BLOOM_WIDTH 128
#elif LONG_BIT >= 64
#define STRINGLIB_BLOOM_WIDTH 64
#elif LONG_BIT >= 32
#define STRINGLIB_BLOOM_WIDTH 32
#else
#error "LONG_BIT is smaller than 32"
#endif

#define STRINGLIB_BLOOM_ADD(mask, ch) \
    ((mask |= (1UL << ((ch) & (STRINGLIB_BLOOM_WIDTH -1)))))
#define STRINGLIB_BLOOM(mask, ch)     \
    ((mask &  (1UL << ((ch) & (STRINGLIB_BLOOM_WIDTH -1)))))

#if STRINGLIB_SIZEOF_CHAR == 1
#  define MEMCHR_CUT_OFF 15
#else
#  define MEMCHR_CUT_OFF 40
#endif

Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(find_char)(const STRINGLIB_CHAR* s, Py_ssize_t n, STRINGLIB_CHAR ch)
{
    const STRINGLIB_CHAR *p, *e;

    p = s;
    e = s + n;
    if (n > MEMCHR_CUT_OFF) {
#if STRINGLIB_SIZEOF_CHAR == 1
        p = memchr(s, ch, n);
        if (p != NULL)
            return (p - s);
        return -1;
#else
        /* use memchr if we can choose a needle without too many likely
           false positives */
        const STRINGLIB_CHAR *s1, *e1;
        unsigned char needle = ch & 0xff;
        /* If looking for a multiple of 256, we'd have too
           many false positives looking for the '\0' byte in UCS2
           and UCS4 representations. */
        if (needle != 0) {
            do {
                void *candidate = memchr(p, needle,
                                         (e - p) * sizeof(STRINGLIB_CHAR));
                if (candidate == NULL)
                    return -1;
                s1 = p;
                p = (const STRINGLIB_CHAR *)
                        _Py_ALIGN_DOWN(candidate, sizeof(STRINGLIB_CHAR));
                if (*p == ch)
                    return (p - s);
                /* False positive */
                p++;
                if (p - s1 > MEMCHR_CUT_OFF)
                    continue;
                if (e - p <= MEMCHR_CUT_OFF)
                    break;
                e1 = p + MEMCHR_CUT_OFF;
                while (p != e1) {
                    if (*p == ch)
                        return (p - s);
                    p++;
                }
            }
            while (e - p > MEMCHR_CUT_OFF);
        }
#endif
    }
    while (p < e) {
        if (*p == ch)
            return (p - s);
        p++;
    }
    return -1;
}

Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(rfind_char)(const STRINGLIB_CHAR* s, Py_ssize_t n, STRINGLIB_CHAR ch)
{
    const STRINGLIB_CHAR *p;
#ifdef HAVE_MEMRCHR
    /* memrchr() is a GNU extension, available since glibc 2.1.91.
       it doesn't seem as optimized as memchr(), but is still quite
       faster than our hand-written loop below */

    if (n > MEMCHR_CUT_OFF) {
#if STRINGLIB_SIZEOF_CHAR == 1
        p = memrchr(s, ch, n);
        if (p != NULL)
            return (p - s);
        return -1;
#else
        /* use memrchr if we can choose a needle without too many likely
           false positives */
        const STRINGLIB_CHAR *s1;
        Py_ssize_t n1;
        unsigned char needle = ch & 0xff;
        /* If looking for a multiple of 256, we'd have too
           many false positives looking for the '\0' byte in UCS2
           and UCS4 representations. */
        if (needle != 0) {
            do {
                void *candidate = memrchr(s, needle,
                                          n * sizeof(STRINGLIB_CHAR));
                if (candidate == NULL)
                    return -1;
                n1 = n;
                p = (const STRINGLIB_CHAR *)
                        _Py_ALIGN_DOWN(candidate, sizeof(STRINGLIB_CHAR));
                n = p - s;
                if (*p == ch)
                    return n;
                /* False positive */
                if (n1 - n > MEMCHR_CUT_OFF)
                    continue;
                if (n <= MEMCHR_CUT_OFF)
                    break;
                s1 = p - MEMCHR_CUT_OFF;
                while (p > s1) {
                    p--;
                    if (*p == ch)
                        return (p - s);
                }
                n = p - s;
            }
            while (n > MEMCHR_CUT_OFF);
        }
#endif
    }
#endif  /* HAVE_MEMRCHR */
    p = s + n;
    while (p > s) {
        p--;
        if (*p == ch)
            return (p - s);
    }
    return -1;
}

#undef MEMCHR_CUT_OFF


Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(_lex_search)(const STRINGLIB_CHAR *needle, Py_ssize_t needle_len,
                       int inverted, Py_ssize_t *return_period)
{
    // We'll eventually partition needle into
    // needle[:max_suffix + 1] + needle[max_suffix + 1:]
    Py_ssize_t max_suffix = -1;

    Py_ssize_t suffix = 0; // candidate for max_suffix
    Py_ssize_t period = 1; // candidate for return_period
    Py_ssize_t k = 1; // working index

    while (suffix + k < needle_len) {
        STRINGLIB_CHAR a = needle[suffix + k];
        STRINGLIB_CHAR b = needle[max_suffix + k];
        if (inverted ? (a < b) : (b < a)) {
            // Suffix is smaller, period is entire prefix so far.
            suffix += k;
            k = 1;
            period = suffix - max_suffix;
        }
        else if (a == b) {
            // Advance through the repitition of the current period.
            if (k != period) {
                k++;
            }
            else {
                suffix += period;
                k = 1;
            }
        }
        else {
            // Found a bigger suffix.
            max_suffix = suffix;
            suffix += 1;
            k = 1;
            period = 1;
        }
    }
    *return_period = period;
    return max_suffix + 1;
}


Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(_critical_factorization)(const STRINGLIB_CHAR *needle, Py_ssize_t needle_len,
                                   Py_ssize_t *return_period)
{
    /* Morally, this is what we want to happen:
        >>> x = "GCAGAGAG"
        >>> suf, period = _critical_factorization(x)
        >>> x[:suf], x[suf:]
        ('GC', 'AGAGAG')
        >>> period
        2               */
    Py_ssize_t period1, period2, max_suf1, max_suf2;

    // Search using both forward and inverted character-orderings
    max_suf1 = STRINGLIB(_lex_search)(needle, needle_len, 0, &period1);
    max_suf2 = STRINGLIB(_lex_search)(needle, needle_len, 1, &period2);

    // Choose the later suffix
    if (max_suf2 < max_suf1) {
        *return_period = period1;
        return max_suf1;
    }
    else {
        *return_period = period2;
        return max_suf2;
    }
}


Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(_two_way)(const STRINGLIB_CHAR *needle, Py_ssize_t needle_len,
                    const STRINGLIB_CHAR *haystack, Py_ssize_t haystack_len,
                    Py_ssize_t suffix, Py_ssize_t period)
{
    LOG("========================\n");
    LOG("Two-way with needle="); LOG_STRING(needle, needle_len);
    LOG(" and haystack="); LOG_STRING(haystack, haystack_len);
    LOG("\nSplit "); LOG_STRING(needle, needle_len);
    LOG(" into "); LOG_STRING(needle, suffix);
    LOG(" and "); LOG_STRING(needle + suffix, needle_len - suffix);
    LOG(".\n");
    unsigned long mask = 0;

    /* Get the set of characters (mod 2^k) in the needle. */
    for (Py_ssize_t i = 0; i < needle_len; i++) {
        STRINGLIB_BLOOM_ADD(mask, needle[i]);
    }

    if (memcmp(needle, needle+period, suffix * STRINGLIB_SIZEOF_CHAR) == 0) {
        LOG("needle is completely periodic.\n");
        // a mismatch can only advance by the period.
        // use memory to avoid re-scanning known occurrences of the period.
        Py_ssize_t memory = 0;
        Py_ssize_t j = 0; // index into haystack
        while (j <= haystack_len - needle_len) {

            // Visualize the line-up:
            LOG("> "); LOG_STRING(haystack, haystack_len);
            LOG("\n> "); LOG("%*s", j, ""); LOG_STRING(needle, needle_len);
            LOG("\n");

            if (!STRINGLIB_BLOOM(mask, haystack[j + needle_len - 1])) {
                LOG("'%c' not in needle; skipping ahead!\n", haystack[j + needle_len - 1]);
                memory = 0;
                j += needle_len;
                continue;
            }

            LOG("Scanning right half.\n");
            Py_ssize_t i = Py_MAX(suffix, memory);
            while (i < needle_len && needle[i] == haystack[j+i]) {
                i++;
            }
            if (i >= needle_len) {
                LOG("Right half matched. Scanning left half.\n");
                i = suffix - 1;
                while (memory < i + 1 && needle[i] == haystack[j+i]) {
                    i--;
                }
                if (i + 1 < memory + 1) {
                    LOG("Left half matches. Returning %d.\n", j);
                    return j;
                }
                LOG("No match.\n");
                // Remember how many periods were scanned on the right
                j += period;
                memory = needle_len - period;
            }
            else {
                LOG("Skip without checking left half.\n");
                j += i - suffix + 1;
                memory = 0;
            }
        }
    }
    else {
        LOG("needle is NOT completely periodic.\n");

        // The two halves are distinct;
        // no extra memory is required,
        // and a mismatch results in a maximal shift.
        period = 1 + Py_MAX(suffix, needle_len - suffix);
        STRINGLIB_CHAR suffix_start = needle[suffix];
        STRINGLIB_CHAR suffix_end = needle[needle_len - 1];

        LOG("Using period %d.\n", period);
        LOG("Right half starts with %c\n", suffix_start);
        LOG("Right half endswith %c\n", suffix_end);

        // Compute the distance between suffix_start and the pervious
        // occurrence of suffix_start.
        Py_ssize_t middle_shift = suffix;
        for (Py_ssize_t k = suffix - 1; k >= 0; k--) {
            if (needle[k] == suffix_start) {
                middle_shift = suffix - k;
                break;
            }
        }

        Py_ssize_t end_shift = needle_len;
        for (Py_ssize_t k = needle_len - 1; k >= 0; k--) {
            if (needle[k] == suffix_end) {
                end_shift = needle_len - k;
                break;
            }
        }

        Py_ssize_t both_shift = suffix;
        for (Py_ssize_t k = 1; suffix - k >= 0; k++) {
            if (needle[suffix - k] == suffix_start
                && suffix_start == needle[needle_len - 1 - k])
            {
                both_shift = k;
                break;
            }
        }

        Py_ssize_t j = 0;
        while (j <= haystack_len - needle_len) {
            while (1) {
                // scan until middle matches
                Py_ssize_t find;
                find = STRINGLIB(find_char)(haystack + j + suffix,
                                            haystack_len - j - needle_len + 1,
                                            suffix_start);
                if (find == -1) {
                    return -1;
                }
                else {
                    j += find;
                    if (j > haystack_len - needle_len) {
                        assert(j <= haystack_len - needle_len);
                    }
                }
                if (haystack[j + suffix] != suffix_start) {
                    assert(haystack[j + suffix] == suffix_start);
                }

                STRINGLIB_CHAR end = haystack[j+needle_len-1];
                if (end == suffix_end) {
                    break;
                }
                else if (!STRINGLIB_BLOOM(mask, end)) {
                    j += needle_len;
                }
                else {
                    j += middle_shift;
                }
                if (j > haystack_len - needle_len) {
                    return -1;
                }
                find = STRINGLIB(find_char)(haystack + j + needle_len - 1,
                                            haystack_len - j - needle_len + 1,
                                            suffix_end);
                if (find == -1) {
                    return -1;
                }
                else {
                    j += find;
                    assert(j <= haystack_len - needle_len);
                }
                assert(haystack[j + needle_len - 1] == suffix_end);
                if (haystack[j+suffix] == suffix_start) {
                    break;
                }
                else {
                    j += end_shift;
                }
                if (j > haystack_len - needle_len) {
                    return -1;
                }
            }

            assert(haystack[j + suffix] == suffix_start);
            assert(haystack[j + needle_len - 1] == suffix_end);

            LOG("> "); LOG_STRING(haystack, haystack_len);
            LOG("\n> "); LOG("%*s", j, ""); LOG_STRING(needle, needle_len);
            LOG("\n");

            LOG("Checking the right half.\n");
            Py_ssize_t i = suffix + 1;
            for (; i < needle_len; i++) {
                if (needle[i] != haystack[j + i]){
                    LOG("No match.\n");
                    break;
                }
            }

            if (i >= needle_len) {
                LOG("Matches. Checking the left half.\n");
                i = suffix - 1;
                for (i = suffix - 1; i >= 0; i--) {
                    if (needle[i] != haystack[j + i]) {
                        break;
                    }
                }
                if (i == -1) {
                    LOG("Match! (at %d)\n", j);
                    return j;
                }
                j += period;
            }
            else {
                LOG("Jump forward without checking left half.\n");
                // Note: In common cases, "both_shift" wins.
                j += Py_MAX(both_shift, i - suffix + 1);
            }
        }

    }
    LOG("Reached end. Returning -1.\n");
    return -1;
}


Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(_fastsearch)(const STRINGLIB_CHAR *needle, Py_ssize_t needle_len,
                       const STRINGLIB_CHAR *haystack, Py_ssize_t haystack_len)
{
    if (needle_len == 0) {
        return -1;
    }
    STRINGLIB_CHAR first = needle[0];

    Py_ssize_t index = STRINGLIB(find_char)(haystack, haystack_len, first);
    if (index == -1) {
        return -1;
    }

    if (haystack_len < needle_len + index) {
        return -1;
    }

    // Do a fast compare to maybe avoid the initialization overhead
    if (memcmp(haystack+index, needle, needle_len*STRINGLIB_SIZEOF_CHAR) == 0) {
        return index;
    }

    // Start later.
    index++;
    Py_ssize_t period, suffix;
    suffix = STRINGLIB(_critical_factorization)(needle, needle_len, &period);
    Py_ssize_t result = STRINGLIB(_two_way)(needle, needle_len,
                                            haystack + index,
                                            haystack_len - index,
                                            suffix, period);

    if (result == -1) {
        return -1;
    }
    return index + result;
}


Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(_fastcount)(const STRINGLIB_CHAR *needle, Py_ssize_t needle_len,
                      const STRINGLIB_CHAR *haystack, Py_ssize_t haystack_len,
                      Py_ssize_t maxcount)
{
    if (maxcount == 0) {
        return 0;
    }
    if (needle_len == 1) {
        Py_ssize_t count = 0;
        for (Py_ssize_t i = 0; i < haystack_len; i++) {
            if (haystack[i] == needle[0]) {
                count++;
                if (count == maxcount) {
                    return maxcount;
                }
            }
        }
        return count;
    }
    if (needle_len == 0) {
        return haystack_len + 1;
    }
    STRINGLIB_CHAR first = needle[0];
    Py_ssize_t index = STRINGLIB(find_char)(haystack, haystack_len, first);
    if (index == -1) {
        return 0;
    }
    if (haystack_len < needle_len + index) {
        return -1;
    }
    Py_ssize_t suffix, period;
    suffix = STRINGLIB(_critical_factorization)(needle, needle_len, &period);
    Py_ssize_t count = 0;
    while (1) {
        Py_ssize_t result = STRINGLIB(_two_way)(needle, needle_len,
                                                haystack + index,
                                                haystack_len - index,
                                                suffix, period);
        if (result == -1) {
            return count;
        }
        else {
            count++;
            if (count == maxcount) {
                return maxcount;
            }
            index += result + needle_len;
        }
    }

}


Py_LOCAL_INLINE(Py_ssize_t)
FASTSEARCH(const STRINGLIB_CHAR* s, Py_ssize_t n,
           const STRINGLIB_CHAR* p, Py_ssize_t m,
           Py_ssize_t maxcount, int mode)
{
    if (m > n) {
        return -1;
    }

    if (mode == FAST_SEARCH) {
        return STRINGLIB(_fastsearch)(p, m, s, n);
    }
    else if (mode == FAST_COUNT) {
        return STRINGLIB(_fastcount)(p, m, s, n, maxcount);
    }
    else {    /* FAST_RSEARCH */
        if (m == 1) {
            return STRINGLIB(rfind_char)(s, n, p[0]);
        }

        Py_ssize_t w = n - m;
        Py_ssize_t mlast = m - 1;
        Py_ssize_t skip = mlast - 1;
        Py_ssize_t mask = 0;
        Py_ssize_t i, j;

        /* create compressed boyer-moore delta 1 table */

        /* process pattern[0] outside the loop */
        STRINGLIB_BLOOM_ADD(mask, p[0]);
        /* process pattern[:0:-1] */
        for (i = mlast; i > 0; i--) {
            STRINGLIB_BLOOM_ADD(mask, p[i]);
            if (p[i] == p[0])
                skip = i - 1;
        }

        for (i = w; i >= 0; i--) {
            if (s[i] == p[0]) {
                /* candidate match */
                for (j = mlast; j > 0; j--)
                    if (s[i+j] != p[j])
                        break;
                if (j == 0)
                    /* got a match! */
                    return i;
                /* miss: check if previous character is part of pattern */
                if (i > 0 && !STRINGLIB_BLOOM(mask, s[i-1]))
                    i = i - m;
                else
                    i = i - skip;
            } else {
                /* skip: check if previous character is part of pattern */
                if (i > 0 && !STRINGLIB_BLOOM(mask, s[i-1]))
                    i = i - m;
            }
        }
        return -1;
    }
}

#undef LOG
#undef LOG_STRING
