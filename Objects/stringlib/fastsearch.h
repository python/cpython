/* stringlib: fastsearch implementation */

#define STRINGLIB_FASTSEARCH_H

/* fast search/count implementation, based on a mix between boyer-
   moore and horspool, with a few more bells and whistles on the top.
   for some more background, see:
   https://web.archive.org/web/20201107074620/http://effbot.org/zone/stringlib.htm */

/* note: fastsearch may access s[n], which isn't a problem when using
   Python's ordinary string types, but may cause problems if you're
   using this code in other contexts.  also, the count mode returns -1
   if there cannot possibly be a match in the target string, and 0 if
   it has actually checked for matches, but didn't find any.  callers
   beware! */

/* If the strings are long enough, use Crochemore and Perrin's Two-Way
   algorithm, which has worst-case O(n) runtime and best-case O(n/k).
   Also compute a table of shifts to achieve O(n/k) in more cases,
   and often (data dependent) deduce larger shifts than pure C&P can
   deduce. */

#define FAST_COUNT 0
#define FAST_SEARCH 1
#define FAST_RSEARCH 2

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

/* Change to a 1 to see logging comments walk through the algorithm. */
#if 0 && STRINGLIB_SIZEOF_CHAR == 1
# define LOG(...) printf(__VA_ARGS__)
# define LOG_STRING(s, n) printf("\"%.*s\"", n, s)
#else
# define LOG(...)
# define LOG_STRING(s, n)
#endif

Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(_lex_search)(const STRINGLIB_CHAR *needle, Py_ssize_t len_needle,
                       Py_ssize_t *return_period, int invert_alphabet)
{
    /* Do a lexicographic search. Essentially this:
           >>> max(needle[i:] for i in range(len(needle)+1))
       Also find the period of the right half.   */
    Py_ssize_t max_suffix = 0;
    Py_ssize_t candidate = 1;
    Py_ssize_t k = 0;
    // The period of the right half.
    Py_ssize_t period = 1;

    while (candidate + k < len_needle) {
        // each loop increases candidate + k + max_suffix
        STRINGLIB_CHAR a = needle[candidate + k];
        STRINGLIB_CHAR b = needle[max_suffix + k];
        // check if the suffix at candidate is better than max_suffix
        if (invert_alphabet ? (b < a) : (a < b)) {
            // Fell short of max_suffix.
            // The next k + 1 characters are non-increasing
            // from candidate, so they won't start a maximal suffix.
            candidate += k + 1;
            k = 0;
            // We've ruled out any period smaller than what's
            // been scanned since max_suffix.
            period = candidate - max_suffix;
        }
        else if (a == b) {
            if (k + 1 != period) {
                // Keep scanning the equal strings
                k++;
            }
            else {
                // Matched a whole period.
                // Start matching the next period.
                candidate += period;
                k = 0;
            }
        }
        else {
            // Did better than max_suffix, so replace it.
            max_suffix = candidate;
            candidate++;
            k = 0;
            period = 1;
        }
    }
    *return_period = period;
    return max_suffix;
}

Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(_factorize)(const STRINGLIB_CHAR *needle,
                      Py_ssize_t len_needle,
                      Py_ssize_t *return_period)
{
    /* Do a "critical factorization", making it so that:
       >>> needle = (left := needle[:cut]) + (right := needle[cut:])
       where the "local period" of the cut is maximal.

       The local period of the cut is the minimal length of a string w
       such that (left endswith w or w endswith left)
       and (right startswith w or w startswith left).

       The Critical Factorization Theorem says that this maximal local
       period is the global period of the string.

       Crochemore and Perrin (1991) show that this cut can be computed
       as the later of two cuts: one that gives a lexicographically
       maximal right half, and one that gives the same with the
       with respect to a reversed alphabet-ordering.

       This is what we want to happen:
           >>> x = "GCAGAGAG"
           >>> cut, period = factorize(x)
           >>> x[:cut], (right := x[cut:])
           ('GC', 'AGAGAG')
           >>> period  # right half period
           2
           >>> right[period:] == right[:-period]
           True

       This is how the local period lines up in the above example:
                GC | AGAGAG
           AGAGAGC = AGAGAGC
       The length of this minimal repetition is 7, which is indeed the
       period of the original string. */

    Py_ssize_t cut1, period1, cut2, period2, cut, period;
    cut1 = STRINGLIB(_lex_search)(needle, len_needle, &period1, 0);
    cut2 = STRINGLIB(_lex_search)(needle, len_needle, &period2, 1);

    // Take the later cut.
    if (cut1 > cut2) {
        period = period1;
        cut = cut1;
    }
    else {
        period = period2;
        cut = cut2;
    }

    LOG("split: "); LOG_STRING(needle, cut);
    LOG(" + "); LOG_STRING(needle + cut, len_needle - cut);
    LOG("\n");

    *return_period = period;
    return cut;
}

#define SHIFT_TYPE uint8_t
#define NOT_FOUND ((1U<<(8*sizeof(SHIFT_TYPE))) - 1U)
#define SHIFT_OVERFLOW (NOT_FOUND - 1U)

#define TABLE_SIZE_BITS 6
#define TABLE_SIZE (1U << TABLE_SIZE_BITS)
#define TABLE_MASK (TABLE_SIZE - 1U)

typedef struct STRINGLIB(_pre) {
    const STRINGLIB_CHAR *needle;
    Py_ssize_t len_needle;
    Py_ssize_t cut;
    Py_ssize_t period;
    int is_periodic;
    SHIFT_TYPE table[TABLE_SIZE];
} STRINGLIB(prework);


Py_LOCAL_INLINE(void)
STRINGLIB(_preprocess)(const STRINGLIB_CHAR *needle, Py_ssize_t len_needle,
                       STRINGLIB(prework) *p)
{
    p->needle = needle;
    p->len_needle = len_needle;
    p->cut = STRINGLIB(_factorize)(needle, len_needle, &(p->period));
    assert(p->period + p->cut <= len_needle);
    p->is_periodic = (0 == memcmp(needle,
                                  needle + p->period,
                                  p->cut * STRINGLIB_SIZEOF_CHAR));
    if (p->is_periodic) {
        assert(p->cut <= len_needle/2);
        assert(p->cut < p->period);
    }
    else {
        // A lower bound on the period
        p->period = Py_MAX(p->cut, len_needle - p->cut) + 1;
    }
    // Now fill up a table
    memset(&(p->table[0]), 0xff, TABLE_SIZE*sizeof(SHIFT_TYPE));
    assert(p->table[0] == NOT_FOUND);
    assert(p->table[TABLE_MASK] == NOT_FOUND);
    for (Py_ssize_t i = 0; i < len_needle; i++) {
        Py_ssize_t shift = len_needle - i;
        if (shift > SHIFT_OVERFLOW) {
            shift = SHIFT_OVERFLOW;
        }
        p->table[needle[i] & TABLE_MASK] = Py_SAFE_DOWNCAST(shift,
                                                            Py_ssize_t,
                                                            SHIFT_TYPE);
    }
}

Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(_two_way)(const STRINGLIB_CHAR *haystack, Py_ssize_t len_haystack,
                    STRINGLIB(prework) *p)
{
    // Crochemore and Perrin's (1991) Two-Way algorithm.
    // See http://www-igm.univ-mlv.fr/~lecroq/string/node26.html#SECTION00260
    Py_ssize_t len_needle = p->len_needle;
    Py_ssize_t cut = p->cut;
    Py_ssize_t period = p->period;
    const STRINGLIB_CHAR *needle = p->needle;
    const STRINGLIB_CHAR *window = haystack;
    const STRINGLIB_CHAR *last_window = haystack + len_haystack - len_needle;
    SHIFT_TYPE *table = p->table;
    LOG("===== Two-way: \"%s\" in \"%s\". =====\n", needle, haystack);

    if (p->is_periodic) {
        LOG("Needle is periodic.\n");
        Py_ssize_t memory = 0;
      periodicwindowloop:
        while (window <= last_window) {
            Py_ssize_t i = Py_MAX(cut, memory);

            // Visualize the line-up:
            LOG("> "); LOG_STRING(haystack, len_haystack);
            LOG("\n> "); LOG("%*s", window - haystack, "");
            LOG_STRING(needle, len_needle);
            LOG("\n> "); LOG("%*s", window - haystack + i, "");
            LOG(" ^ <-- cut\n");

            if (window[i] != needle[i]) {
                // Sunday's trick: if we're going to jump, we might
                // as well jump to line up the character *after* the
                // current window.
                STRINGLIB_CHAR first_outside = window[len_needle];
                SHIFT_TYPE shift = table[first_outside & TABLE_MASK];
                if (shift == NOT_FOUND) {
                    LOG("\"%c\" not found. Skipping entirely.\n",
                        first_outside);
                    window += len_needle + 1;
                }
                else {
                    LOG("Shifting to line up \"%c\".\n", first_outside);
                    Py_ssize_t memory_shift = i - cut + 1;
                    window += Py_MAX(shift, memory_shift);
                }
                memory = 0;
                goto periodicwindowloop;
            }
            for (i = i + 1; i < len_needle; i++) {
                if (needle[i] != window[i]) {
                    LOG("Right half does not match. Jump ahead by %d.\n",
                        i - cut + 1);
                    window += i - cut + 1;
                    memory = 0;
                    goto periodicwindowloop;
                }
            }
            for (i = memory; i < cut; i++) {
                if (needle[i] != window[i]) {
                    LOG("Left half does not match. Jump ahead by period %d.\n",
                        period);
                    window += period;
                    memory = len_needle - period;
                    goto periodicwindowloop;
                }
            }
            LOG("Left half matches. Returning %d.\n",
                window - haystack);
            return window - haystack;
        }
    }
    else {
        LOG("Needle is not periodic.\n");
        assert(cut < len_needle);
        STRINGLIB_CHAR needle_cut = needle[cut];
      windowloop:
        while (window <= last_window) {

            // Visualize the line-up:
            LOG("> "); LOG_STRING(haystack, len_haystack);
            LOG("\n> "); LOG("%*s", window - haystack, "");
            LOG_STRING(needle, len_needle);
            LOG("\n> "); LOG("%*s", window - haystack + cut, "");
            LOG(" ^ <-- cut\n");

            if (window[cut] != needle_cut) {
                // Sunday's trick: if we're going to jump, we might
                // as well jump to line up the character *after* the
                // current window.
                STRINGLIB_CHAR first_outside = window[len_needle];
                SHIFT_TYPE shift = table[first_outside & TABLE_MASK];
                if (shift == NOT_FOUND) {
                    LOG("\"%c\" not found. Skipping entirely.\n",
                        first_outside);
                    window += len_needle + 1;
                }
                else {
                    LOG("Shifting to line up \"%c\".\n", first_outside);
                    window += shift;
                }
                goto windowloop;
            }
            for (Py_ssize_t i = cut + 1; i < len_needle; i++) {
                if (needle[i] != window[i]) {
                    LOG("Right half does not match. Advance by %d.\n",
                        i - cut + 1);
                    window += i - cut + 1;
                    goto windowloop;
                }
            }
            for (Py_ssize_t i = 0; i < cut; i++) {
                if (needle[i] != window[i]) {
                    LOG("Left half does not match. Advance by period %d.\n",
                        period);
                    window += period;
                    goto windowloop;
                }
            }
            LOG("Left half matches. Returning %d.\n", window - haystack);
            return window - haystack;
        }
    }
    LOG("Not found. Returning -1.\n");
    return -1;
}

Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(_two_way_find)(const STRINGLIB_CHAR *haystack,
                         Py_ssize_t len_haystack,
                         const STRINGLIB_CHAR *needle,
                         Py_ssize_t len_needle)
{
    LOG("###### Finding \"%s\" in \"%s\".\n", needle, haystack);
    STRINGLIB(prework) p;
    STRINGLIB(_preprocess)(needle, len_needle, &p);
    return STRINGLIB(_two_way)(haystack, len_haystack, &p);
}

Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(_two_way_count)(const STRINGLIB_CHAR *haystack,
                          Py_ssize_t len_haystack,
                          const STRINGLIB_CHAR *needle,
                          Py_ssize_t len_needle,
                          Py_ssize_t maxcount)
{
    LOG("###### Counting \"%s\" in \"%s\".\n", needle, haystack);
    STRINGLIB(prework) p;
    STRINGLIB(_preprocess)(needle, len_needle, &p);
    Py_ssize_t index = 0, count = 0;
    while (1) {
        Py_ssize_t result;
        result = STRINGLIB(_two_way)(haystack + index,
                                     len_haystack - index, &p);
        if (result == -1) {
            return count;
        }
        count++;
        if (count == maxcount) {
            return maxcount;
        }
        index += result + len_needle;
    }
    return count;
}

#undef SHIFT_TYPE
#undef NOT_FOUND
#undef SHIFT_OVERFLOW
#undef TABLE_SIZE_BITS
#undef TABLE_SIZE
#undef TABLE_MASK

#undef LOG
#undef LOG_STRING

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
        if (m >= 100 && w >= 2000 && w / m >= 5) {
            /* For larger problems where the needle isn't a huge
               percentage of the size of the haystack, the relatively
               expensive O(m) startup cost of the two-way algorithm
               will surely pay off. */
            if (mode == FAST_SEARCH) {
                return STRINGLIB(_two_way_find)(s, n, p, m);
            }
            else {
                return STRINGLIB(_two_way_count)(s, n, p, m, maxcount);
            }
        }
        const STRINGLIB_CHAR *ss = s + m - 1;
        const STRINGLIB_CHAR *pp = p + m - 1;

        /* create compressed boyer-moore delta 1 table */

        /* process pattern[:-1] */
        for (i = 0; i < mlast; i++) {
            STRINGLIB_BLOOM_ADD(mask, p[i]);
            if (p[i] == p[mlast]) {
                skip = mlast - i - 1;
            }
        }
        /* process pattern[-1] outside the loop */
        STRINGLIB_BLOOM_ADD(mask, p[mlast]);

        if (m >= 100 && w >= 8000) {
            /* To ensure that we have good worst-case behavior,
               here's an adaptive version of the algorithm, where if
               we match O(m) characters without any matches of the
               entire needle, then we predict that the startup cost of
               the two-way algorithm will probably be worth it. */
            Py_ssize_t hits = 0;
            for (i = 0; i <= w; i++) {
                if (ss[i] == pp[0]) {
                    /* candidate match */
                    for (j = 0; j < mlast; j++) {
                        if (s[i+j] != p[j]) {
                            break;
                        }
                    }
                    if (j == mlast) {
                        /* got a match! */
                        if (mode != FAST_COUNT) {
                            return i;
                        }
                        count++;
                        if (count == maxcount) {
                            return maxcount;
                        }
                        i = i + mlast;
                        continue;
                    }
                    /* miss: check if next character is part of pattern */
                    if (!STRINGLIB_BLOOM(mask, ss[i+1])) {
                        i = i + m;
                    }
                    else {
                        i = i + skip;
                    }
                    hits += j + 1;
                    if (hits >= m / 4 && i < w - 1000) {
                        /* We've done O(m) fruitless comparisons
                           anyway, so spend the O(m) cost on the
                           setup for the two-way algorithm. */
                        Py_ssize_t res;
                        if (mode == FAST_COUNT) {
                            res = STRINGLIB(_two_way_count)(
                                s+i, n-i, p, m, maxcount-count);
                            return count + res;
                        }
                        else {
                            res = STRINGLIB(_two_way_find)(s+i, n-i, p, m);
                            if (res == -1) {
                                return -1;
                            }
                            return i + res;
                        }
                    }
                }
                else {
                    /* skip: check if next character is part of pattern */
                    if (!STRINGLIB_BLOOM(mask, ss[i+1])) {
                        i = i + m;
                    }
                }
            }
            if (mode != FAST_COUNT) {
                return -1;
            }
            return count;
        }
        /* The standard, non-adaptive version of the algorithm. */
        for (i = 0; i <= w; i++) {
            /* note: using mlast in the skip path slows things down on x86 */
            if (ss[i] == pp[0]) {
                /* candidate match */
                for (j = 0; j < mlast; j++) {
                    if (s[i+j] != p[j]) {
                        break;
                    }
                }
                if (j == mlast) {
                    /* got a match! */
                    if (mode != FAST_COUNT) {
                        return i;
                    }
                    count++;
                    if (count == maxcount) {
                        return maxcount;
                    }
                    i = i + mlast;
                    continue;
                }
                /* miss: check if next character is part of pattern */
                if (!STRINGLIB_BLOOM(mask, ss[i+1])) {
                    i = i + m;
                }
                else {
                    i = i + skip;
                }
            }
            else {
                /* skip: check if next character is part of pattern */
                if (!STRINGLIB_BLOOM(mask, ss[i+1])) {
                    i = i + m;
                }
            }
        }
    }
    else {    /* FAST_RSEARCH */

        /* create compressed boyer-moore delta 1 table */

        /* process pattern[0] outside the loop */
        STRINGLIB_BLOOM_ADD(mask, p[0]);
        /* process pattern[:0:-1] */
        for (i = mlast; i > 0; i--) {
            STRINGLIB_BLOOM_ADD(mask, p[i]);
            if (p[i] == p[0]) {
                skip = i - 1;
            }
        }

        for (i = w; i >= 0; i--) {
            if (s[i] == p[0]) {
                /* candidate match */
                for (j = mlast; j > 0; j--) {
                    if (s[i+j] != p[j]) {
                        break;
                    }
                }
                if (j == 0) {
                    /* got a match! */
                    return i;
                }
                /* miss: check if previous character is part of pattern */
                if (i > 0 && !STRINGLIB_BLOOM(mask, s[i-1])) {
                    i = i - m;
                }
                else {
                    i = i - skip;
                }
            }
            else {
                /* skip: check if previous character is part of pattern */
                if (i > 0 && !STRINGLIB_BLOOM(mask, s[i-1])) {
                    i = i - m;
                }
            }
        }
    }

    if (mode != FAST_COUNT)
        return -1;
    return count;
}

