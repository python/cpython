/* stringlib: fastsearch implementation */

#define STRINGLIB_FASTSEARCH_H

/* fast search/count implementation, based on a mix between boyer-
   moore and horspool, with a few more bells and whistles on the top.
   for some more background, see: http://effbot.org/zone/stringlib.htm */

/* When the needle/pattern is long enough during the a forward search
   or count, use the more complex Two-Way algorithm, which leverages
   patterns in the string to ensure no worse than linear time.
   Additionally, a Boyer-Moore bad-character shift table is computed
   so that sublinear (as in O(n/m)) time is achieved in more cases.
   References:
       http://www-igm.univ-mlv.fr/~lecroq/string/node26.html#SECTION00260
       https://en.wikipedia.org/wiki/Two-way_string-matching_algorithm
   This implementation was largely influenced by glibc:
       https://code.woboq.org/userspace/glibc/string/str-two-way.h.html
       https://code.woboq.org/userspace/glibc/string/memmem.c.html
   Discussion here:
       https://bugs.python.org/issue41972
   */

/* note: fastsearch may access s[n], which isn't a problem when using
   Python's ordinary string types, but may cause problems if you're
   using this code in other contexts.  also, the count mode returns -1
   if there cannot possibly be a match in the target string, and 0 if
   it has actually checked for matches, but didn't find any.  callers
   beware! */

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

// Preprocessing steps for the two-way algorithm.
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
            // Advance through the repetition of the current period.
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
STRINGLIB(_critical_factorization)(const STRINGLIB_CHAR *needle,
                                   Py_ssize_t needle_len,
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


#define SHIFT_TYPE uint16_t
#define NOT_FOUND ((1U<<(8*sizeof(SHIFT_TYPE))) - 1U)
#define SHIFT_OVERFLOW (NOT_FOUND - 1U)

#define TABLE_SIZE_BITS 7
#define TABLE_SIZE (1U << TABLE_SIZE_BITS)
#define TABLE_MASK (TABLE_SIZE - 1U)

Py_LOCAL_INLINE(void)
STRINGLIB(_init_table)(const STRINGLIB_CHAR *needle, Py_ssize_t needle_len,
                       SHIFT_TYPE *table)
{
    // Fill the table with NOT_FOUND
    memset(table, 0xff, TABLE_SIZE * sizeof(SHIFT_TYPE));
    assert(table[0] == NOT_FOUND);
    assert(table[TABLE_SIZE - 1] == NOT_FOUND);
    for (Py_ssize_t j = 0; j < needle_len; j++) {
        // TABLE_MASK means not in string
        // SHIFT_OVERFLOW means shift at least SHIFT_OVERFLOW
        Py_ssize_t shift = needle_len - j - 1;
        if (shift > SHIFT_OVERFLOW) {
            shift = SHIFT_OVERFLOW;
        }
        table[needle[j] & TABLE_MASK] = (SHIFT_TYPE)shift;
    }
}


Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(_two_way)(const STRINGLIB_CHAR *needle, Py_ssize_t needle_len,
                    const STRINGLIB_CHAR *haystack, Py_ssize_t haystack_len,
                    Py_ssize_t suffix, Py_ssize_t period,
                    SHIFT_TYPE *shift_table)
{
    LOG("========================\n");
    LOG("Two-way with needle="); LOG_STRING(needle, needle_len);
    LOG(" and haystack="); LOG_STRING(haystack, haystack_len);
    LOG("\nSplit "); LOG_STRING(needle, needle_len);
    LOG(" into "); LOG_STRING(needle, suffix);
    LOG(" and "); LOG_STRING(needle + suffix, needle_len - suffix);
    LOG(".\n");

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

            STRINGLIB_CHAR last = haystack[j + needle_len - 1];
            int index = last & TABLE_MASK;
            SHIFT_TYPE shift = shift_table[index];

            switch (shift)
            {
                case 0: {
                    break;
                }
                case NOT_FOUND: {
                    LOG("Last character not found in string.\n");
                    memory = 0;
                    j += needle_len;
                    continue;
                }
                case SHIFT_OVERFLOW: {
                    LOG("Shift overflowed.\n");
                    memory = 0;
                    j += SHIFT_OVERFLOW;
                    continue;
                }
                default: {
                    if (memory && shift < period) {
                        LOG("Shifting through multiple periods.\n");
                        j += needle_len - period;
                    } else {
                        LOG("Table says shift by %d.\n", shift);
                        j += shift;
                    }
                    memory = 0;
                    continue;
                }
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
        LOG("Using period %d.\n", period);

        Py_ssize_t j = 0;
        while (j <= haystack_len - needle_len) {
            LOG("> "); LOG_STRING(haystack, haystack_len);
            LOG("\n> "); LOG("%*s", j, ""); LOG_STRING(needle, needle_len);
            LOG("\n");

            STRINGLIB_CHAR last = haystack[j + needle_len - 1];
            int index = last & TABLE_MASK;
            SHIFT_TYPE shift = shift_table[index];
            switch (shift)
            {
                case 0: {
                    break;
                }
                case NOT_FOUND: {
                    LOG("Last character not found in string.\n");
                    j += needle_len;
                    continue;
                }
                default: {
                    LOG("Table says shift by %d.\n", shift);
                    j += shift;
                    continue;
                }
            }

            assert((haystack[j + needle_len - 1] & TABLE_MASK)
                   == (needle[needle_len - 1] & TABLE_MASK));

            LOG("Checking the right half.\n");
            Py_ssize_t i = suffix;
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
                j += i - suffix + 1;
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

    Py_ssize_t index = STRINGLIB(find_char)(haystack,
                                            haystack_len - needle_len + 1,
                                            first);
    if (index == -1) {
        return -1;
    }

    // Do a fast compare in all cases to maybe avoid the initialization overhead
    if (memcmp(haystack+index, needle, needle_len*STRINGLIB_SIZEOF_CHAR) == 0) {
        return index;
    }
    else {
        // Start later.
        index++;
    }

    // Prework: factorize.
    Py_ssize_t period, suffix;
    suffix = STRINGLIB(_critical_factorization)(needle, needle_len, &period);

    // Prework: make a skip table.
    SHIFT_TYPE shift_table[TABLE_SIZE];
    STRINGLIB(_init_table)(needle, needle_len, shift_table);

    Py_ssize_t result = STRINGLIB(_two_way)(needle, needle_len,
                                            haystack + index,
                                            haystack_len - index,
                                            suffix, period,
                                            shift_table);

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
    STRINGLIB_CHAR first = needle[0];
    Py_ssize_t index = STRINGLIB(find_char)(haystack,
                                            haystack_len - needle_len + 1,
                                            first);
    if (index == -1) {
        return 0;
    }
    Py_ssize_t suffix, period;
    suffix = STRINGLIB(_critical_factorization)(needle, needle_len, &period);
    SHIFT_TYPE shift_table[TABLE_SIZE];
    STRINGLIB(_init_table)(needle, needle_len, shift_table);
    Py_ssize_t count = 0;
    while (1) {
        Py_ssize_t result = STRINGLIB(_two_way)(needle, needle_len,
                                                haystack + index,
                                                haystack_len - index,
                                                suffix, period,
                                                shift_table);
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

#undef SHIFT_TYPE
#undef NOT_FOUND
#undef SHIFT_OVERFLOW
#undef TABLE_SIZE_BITS
#undef TABLE_SIZE
#undef TABLE_MASK


Py_LOCAL_INLINE(Py_ssize_t)
FASTSEARCH(const STRINGLIB_CHAR* s, Py_ssize_t n,
           const STRINGLIB_CHAR* p, Py_ssize_t m,
           Py_ssize_t maxcount, int mode)
{
    unsigned long mask;
    Py_ssize_t skip, count = 0;
    Py_ssize_t i, j, mlast, w;

    w = n - m;

    if (w < 0 || (mode == FAST_COUNT && maxcount == 0))
        return -1;

    /* look for special cases */
    if (m <= 1) {
        if (m <= 0)
            return -1;
        /* use special case for 1-character strings */
        if (mode == FAST_SEARCH)
            return STRINGLIB(find_char)(s, n, p[0]);
        else if (mode == FAST_RSEARCH)
            return STRINGLIB(rfind_char)(s, n, p[0]);
        else {  /* FAST_COUNT */
            for (i = 0; i < n; i++)
                if (s[i] == p[0]) {
                    count++;
                    if (count == maxcount)
                        return maxcount;
                }
            return count;
        }
    }

    mlast = m - 1;
    skip = mlast;
    mask = 0;

    if (mode != FAST_RSEARCH) {
        if (n >= 500 && m >= 10) {
            /* long needles/haystacks get the two-way algorithm. */
            if (mode == FAST_SEARCH) {
                return STRINGLIB(_fastsearch)(p, m, s, n);
            }
            else {
                return STRINGLIB(_fastcount)(p, m, s, n, maxcount);
            }
        }

        /* Short needles use Fredrik Lundh's Horspool/Sunday hybrid
           algorithm for less overhead. */
        const STRINGLIB_CHAR *ss = s + m - 1;
        const STRINGLIB_CHAR *pp = p + m - 1;

        /* create compressed boyer-moore delta 1 table */

        /* process pattern[:-1] */
        for (i = 0; i < mlast; i++) {
            STRINGLIB_BLOOM_ADD(mask, p[i]);
            if (p[i] == p[mlast])
                skip = mlast - i - 1;
        }
        /* process pattern[-1] outside the loop */
        STRINGLIB_BLOOM_ADD(mask, p[mlast]);

        for (i = 0; i <= w; i++) {
            /* note: using mlast in the skip path slows things down on x86 */
            if (ss[i] == pp[0]) {
                /* candidate match */
                for (j = 0; j < mlast; j++)
                    if (s[i+j] != p[j])
                        break;
                if (j == mlast) {
                    /* got a match! */
                    if (mode != FAST_COUNT)
                        return i;
                    count++;
                    if (count == maxcount)
                        return maxcount;
                    i = i + mlast;
                    continue;
                }
                /* miss: check if next character is part of pattern */
                if (!STRINGLIB_BLOOM(mask, ss[i+1]))
                    i = i + m;
                else
                    i = i + skip;
            } else {
                /* skip: check if next character is part of pattern */
                if (!STRINGLIB_BLOOM(mask, ss[i+1]))
                    i = i + m;
            }
        }
    } else {    /* FAST_RSEARCH */

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
    }

    if (mode != FAST_COUNT)
        return -1;
    return count;
}

#undef LOG
#undef LOG_STRING
