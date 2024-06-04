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
   deduce. See stringlib_find_two_way_notes.txt in this folder for a
   detailed explanation. */

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

#ifdef STRINGLIB_FAST_MEMCHR
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
#ifdef STRINGLIB_FAST_MEMCHR
        p = STRINGLIB_FAST_MEMCHR(s, ch, n);
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

#undef MEMCHR_CUT_OFF

#if STRINGLIB_SIZEOF_CHAR == 1
#  define MEMRCHR_CUT_OFF 15
#else
#  define MEMRCHR_CUT_OFF 40
#endif


Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(rfind_char)(const STRINGLIB_CHAR* s, Py_ssize_t n, STRINGLIB_CHAR ch)
{
    const STRINGLIB_CHAR *p;
#ifdef HAVE_MEMRCHR
    /* memrchr() is a GNU extension, available since glibc 2.1.91.  it
       doesn't seem as optimized as memchr(), but is still quite
       faster than our hand-written loop below. There is no wmemrchr
       for 4-byte chars. */

    if (n > MEMRCHR_CUT_OFF) {
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
                if (n1 - n > MEMRCHR_CUT_OFF)
                    continue;
                if (n <= MEMRCHR_CUT_OFF)
                    break;
                s1 = p - MEMRCHR_CUT_OFF;
                while (p > s1) {
                    p--;
                    if (*p == ch)
                        return (p - s);
                }
                n = p - s;
            }
            while (n > MEMRCHR_CUT_OFF);
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

#undef MEMRCHR_CUT_OFF


/* Change to 1 or 2 to see logging comments walk through the algorithm.
 * LOG_LEVEL == 1 print excludes input strings (useful for long inputs)
 * LOG_LEVEL == 2 print includes input alignments */
# define LOG_LEVEL 0
#if LOG_LEVEL == 1 && STRINGLIB_SIZEOF_CHAR == 1
# define LOG(...) printf(__VA_ARGS__)
# define LOG_STRING(s, n)
# define LOG_LINEUP()
# define LOG_LINEUP_REV()
#elif LOG_LEVEL == 2 && STRINGLIB_SIZEOF_CHAR == 1
# define LOG(...) printf(__VA_ARGS__)
# define LOG_STRING(s, n) printf("\"%.*s\"", (int)(n), s)
# define LOG_LINEUP() do {                                         \
    LOG("> "); LOG_STRING(haystack, len_haystack); LOG("\n> ");    \
    LOG("%*s",(int)(window_last - haystack + 1 - len_needle), ""); \
    LOG_STRING(needle, len_needle); LOG("\n");                     \
} while(0)
# define LOG_LINEUP_REV() do {                                     \
    LOG("> "); LOG_STRING(haystack, len_haystack); LOG("\n> ");    \
    LOG("%*s",(int)(window - haystack), "");                       \
    LOG_STRING(needle, len_needle); LOG("\n");                     \
} while(0)
#else
# define LOG(...)
# define LOG_STRING(s, n)
# define LOG_LINEUP()
# define LOG_LINEUP_REV()
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
STRINGLIB(_lex_search_rev)(const STRINGLIB_CHAR *needle, Py_ssize_t len_needle,
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
    Py_ssize_t offset = len_needle - 1;
    while (candidate + k < len_needle) {
        // each loop increases candidate + k + max_suffix
        STRINGLIB_CHAR a = needle[offset - candidate - k];
        STRINGLIB_CHAR b = needle[offset - max_suffix - k];
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
    return len_needle - max_suffix - 1;
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
       and (right startswith w or w startswith right).

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
    LOG("Cut: %ld & Period: %ld\n", cut, period);

    *return_period = period;
    return cut;
}


Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(_factorize_rev)(const STRINGLIB_CHAR *needle,
                          Py_ssize_t len_needle,
                          Py_ssize_t *return_period)
{
    Py_ssize_t cut1, period1, cut2, period2, cut, period;
    cut1 = STRINGLIB(_lex_search_rev)(needle, len_needle, &period1, 0);
    cut2 = STRINGLIB(_lex_search_rev)(needle, len_needle, &period2, 1);
    // Take the later cut.
    if (cut1 < cut2) {
        period = period1;
        cut = cut1;
    }
    else {
        period = period2;
        cut = cut2;
    }

    LOG("split: "); LOG_STRING(needle, cut + 1);
    LOG(" + "); LOG_STRING(needle + cut + 1, len_needle);
    LOG("\n");
    LOG("Cut: %ld & Period: %ld\n", cut, period);

    *return_period = period;
    return cut;
}


#define SHIFT_TYPE uint8_t
#define MAX_SHIFT UINT8_MAX

#define TABLE_SIZE_BITS 6u
#define TABLE_SIZE (1U << TABLE_SIZE_BITS)
#define TABLE_MASK (TABLE_SIZE - 1U)

typedef struct STRINGLIB(_pre) {
    const STRINGLIB_CHAR *needle;
    Py_ssize_t len_needle;
    Py_ssize_t cut;
    Py_ssize_t period;
    Py_ssize_t gap;
    int is_periodic;
    SHIFT_TYPE table[TABLE_SIZE];
} STRINGLIB(prework);


static void
STRINGLIB(_preprocess)(const STRINGLIB_CHAR *needle,
                       Py_ssize_t len_needle,
                       STRINGLIB(prework) *pw,
                       int direction,
                       int critical_fac,
                       int bc_table_gs_gap)
{
    // Set the Needle & Calculate Critical Factorization
    if (critical_fac) {
        pw->needle = needle;
        pw->len_needle = len_needle;
        if (direction == 1) {
            pw->cut = STRINGLIB(_factorize)(needle, len_needle, &(pw->period));
            assert(pw->period + pw->cut <= len_needle);
            //bbb c
            //   |
            //bbb
            // bb c
            //
            //c bbb
            // |
            pw->is_periodic = (0 == memcmp(needle,
                               needle + pw->period,
                               pw->cut * STRINGLIB_SIZEOF_CHAR));
            if (pw->is_periodic) {
                assert(pw->cut <= len_needle/2);
                assert(pw->cut < pw->period);
            }
            else {
                // A lower bound on the period
                pw->period = Py_MAX(pw->cut, len_needle - pw->cut) + 1;
            }
        }
        else {
            // [0, 1, 2, 3]
            pw->cut = STRINGLIB(_factorize_rev)(
                needle, len_needle, &(pw->period));
            Py_ssize_t inv_cut = len_needle - pw->cut - 1;
            assert(pw->cut - pw->period >= 0);
            pw->is_periodic = (0 == memcmp(needle + len_needle - inv_cut,
                               needle + len_needle - inv_cut - pw->period,
                               inv_cut * STRINGLIB_SIZEOF_CHAR));
            if (pw->is_periodic) {
                assert(pw->cut >= len_needle/2);
                assert(len_needle - pw->cut - 1 < pw->period);
            }
            else {
                // A lower bound on the period
                pw->period = Py_MIN(pw->cut, len_needle + pw->cut) + 1;
            }
        }
    }
    if (bc_table_gs_gap) {
        // Fill up a compressed Boyer-Moore "Bad Character" table
        Py_ssize_t not_found_shift = Py_MIN(len_needle, MAX_SHIFT);
        for (Py_ssize_t i = 0; i < (Py_ssize_t)TABLE_SIZE; i++) {
            pw->table[i] = Py_SAFE_DOWNCAST(not_found_shift,
                                            Py_ssize_t, SHIFT_TYPE);
        }
        if (direction == 1) {
            for (Py_ssize_t i = len_needle - not_found_shift; i < len_needle; i++) {
                SHIFT_TYPE shift = Py_SAFE_DOWNCAST(len_needle - 1 - i,
                                                    Py_ssize_t, SHIFT_TYPE);
                pw->table[needle[i] & TABLE_MASK] = shift;
            }

            // Initialize "Good Suffix" Last Character Gap
            // Note: gap("___aa") = 1
            pw->gap = len_needle;
            STRINGLIB_CHAR last = needle[len_needle - 1] & TABLE_MASK;
            for (Py_ssize_t i = len_needle - 2; i >= 0; i--) {
                STRINGLIB_CHAR x = needle[i] & TABLE_MASK;
                if (x == last) {
                    pw->gap = len_needle - 1 - i;
                    break;
                }
            }
        }
        else {
            for (Py_ssize_t i = not_found_shift - 1; i >= 0; i--) {
                SHIFT_TYPE shift = Py_SAFE_DOWNCAST(i, Py_ssize_t, SHIFT_TYPE);
                pw->table[needle[i] & TABLE_MASK] = shift;
            }
            pw->gap = len_needle;
            STRINGLIB_CHAR last = needle[0] & TABLE_MASK;
            for (Py_ssize_t i = 1; i < len_needle; i++) {
                STRINGLIB_CHAR x = needle[i] & TABLE_MASK;
                if (x == last) {
                    pw->gap = i;
                    break;
                }
            }
        }
        LOG("Good Suffix Gap: %ld\n", pw->gap);
    }
}


static Py_ssize_t
STRINGLIB(_two_way)(const STRINGLIB_CHAR *haystack,
                    Py_ssize_t len_haystack,
                    Py_ssize_t maxcount,
                    int mode,
                    STRINGLIB(prework) *pw)
{
    // Crochemore and Perrin's (1991) Two-Way algorithm.
    // See http://www-igm.univ-mlv.fr/~lecroq/string/node26.html#SECTION00260
    const Py_ssize_t len_needle = pw->len_needle;
    const Py_ssize_t cut = pw->cut;
    Py_ssize_t period = pw->period;
    const STRINGLIB_CHAR *const needle = pw->needle;
    const STRINGLIB_CHAR *window_last = haystack + len_needle - 1;
    const STRINGLIB_CHAR *const haystack_end = haystack + len_haystack;
    SHIFT_TYPE *table = pw->table;
    const STRINGLIB_CHAR *window;
    LOG("===== Two-way: \"%s\" in \"%s\". =====\n", needle, haystack);
    if (mode == FAST_COUNT){
        LOG("###### Counting \"%s\" in \"%s\".\n", needle, haystack);
    }
    else {
        LOG("###### Finding \"%s\" in \"%s\".\n", needle, haystack);
    }
    Py_ssize_t count = 0;
    Py_ssize_t gap = pw->gap;
    Py_ssize_t shift, i;
    Py_ssize_t iloop = 0;
    Py_ssize_t ihits = 0;
    Py_ssize_t gap_jump_end = Py_MIN(len_needle, cut + gap);
    int is_periodic = pw->is_periodic;
    Py_ssize_t memory = 0;
    if (is_periodic) {
        LOG("Needle is periodic.\n");
    }
    else {
        LOG("Needle is not periodic.\n");
        period = Py_MAX(gap, period);
    }
    while (window_last < haystack_end) {
        assert(memory == 0);
        LOG_LINEUP();
        iloop++;
        shift = table[(*window_last) & TABLE_MASK];
        window_last += shift;
        if (shift != 0){
            LOG("Horspool skip\n");
            continue;
        }
        if (window_last >= haystack_end){
            break;
        }
      no_shift:
        window = window_last - len_needle + 1;
        assert((window[len_needle - 1] & TABLE_MASK) ==
               (needle[len_needle - 1] & TABLE_MASK));
        if (is_periodic) {
            i = Py_MAX(cut, memory);
        } else {
            i = cut;
        }
        for (; i < len_needle; i++) {
            ihits++;
            if (needle[i] != window[i]) {
                if (i < gap_jump_end) {
                    LOG("Early right half mismatch: jump by gap.\n");
                    assert(gap >= i - cut + 1);
                    window_last += gap;
                }
                else {
                    LOG("Late right half mismatch: jump by n (>gap)\n");
                    assert(i - cut + 1 > gap);
                    window_last += i - cut + 1;
                }
                memory = 0;
                break;
            }
        }
        if (i != len_needle){
            continue;
        }
        if (is_periodic) {
            i = memory;
        } else {
            i = 0;
        }
        for (; i < cut; i++) {
            ihits++;
            if (needle[i] != window[i]) {
                LOG("Left half does not match.\n");
                window_last += period;
                if (!is_periodic){
                    break;
                }
                memory = len_needle - period;
                if (window_last >= haystack_end) {
                    break;
                }
                iloop++;
                Py_ssize_t shift = table[(*window_last) & TABLE_MASK];
                if (!shift){
                    goto no_shift;
                }
                // A mismatch has been identified to the right
                // of where i will next start, so we can jump
                // at least as far as if the mismatch occurred
                // on the first comparison.
                Py_ssize_t mem_jump = Py_MAX(cut, memory) - cut + 1;
                LOG("Skip with Memory.\n");
                memory = 0;
                window_last += Py_MAX(shift, mem_jump);
                break;
            }
        }
        if (i == cut) {
            LOG("Found a match!\n");
            if (mode != FAST_COUNT) {
                return window - haystack;
            }
            count++;
            if (count == maxcount) {
                return maxcount;
            }
            window_last += len_needle;
        }
    }
    // Loop Counter and Memory Access Counter Logging (Used for calibration)
    // In worst case scenario iloop == n - m
    // iloop == ihits indicates linear performance for quadratic problems
    LOG("iloop: %ld\n", iloop);
    LOG("ihits: %ld\n", ihits);
    if (mode == FAST_COUNT) {
        LOG("Counting finished.\n");
        return count;
    }
    else {
        LOG("Not found. Returning -1.\n");
        return -1;
    }
}


static Py_ssize_t
STRINGLIB(two_way_find)(const STRINGLIB_CHAR *haystack,
                        Py_ssize_t len_haystack,
                        const STRINGLIB_CHAR *needle,
                        Py_ssize_t len_needle,
                        Py_ssize_t maxcount,
                        int mode)
{
    int dir = 1;
    STRINGLIB(prework) pw;
    STRINGLIB(_preprocess)(needle, len_needle, &pw, dir, 1, 1);
    return STRINGLIB(_two_way)(haystack, len_haystack, maxcount, mode, &pw);
}


static Py_ssize_t
STRINGLIB(_two_way_rev)(const STRINGLIB_CHAR *haystack,
                        Py_ssize_t len_haystack,
                        Py_ssize_t maxcount,
                        int mode,
                        STRINGLIB(prework) *pw)
{
    // Reversed Crochemore and Perrin's (1991) Two-Way algorithm.
    // See http://www-igm.univ-mlv.fr/~lecroq/string/node26.html#SECTION00260
    const Py_ssize_t len_needle = pw->len_needle;
    const Py_ssize_t cut = pw->cut;
    Py_ssize_t period = pw->period;
    const STRINGLIB_CHAR *const needle = pw->needle;
    const STRINGLIB_CHAR *const haystack_end = haystack + len_haystack;
    const STRINGLIB_CHAR *window = haystack_end - len_needle;
    SHIFT_TYPE *table = pw->table;
    LOG("===== Two-way: \"%s\" in \"%s\". =====\n", needle, haystack);
    if (mode == FAST_COUNT){
        LOG("###### Counting \"%s\" in \"%s\".\n", needle, haystack);
    }
    else {
        LOG("###### Finding \"%s\" in \"%s\".\n", needle, haystack);
    }
    Py_ssize_t inv_cut;
    Py_ssize_t count = 0;
    Py_ssize_t gap = pw->gap;
    Py_ssize_t shift, i;
    Py_ssize_t iloop = 0;
    Py_ssize_t ihits = 0;
    Py_ssize_t gap_jump_end = Py_MAX(0, cut - gap);
    int is_periodic = pw->is_periodic;
    Py_ssize_t memory = 0;
    if (is_periodic) {
        LOG("Needle is periodic.\n");
    }
    else {
        LOG("Needle is not periodic.\n");
        period = Py_MAX(gap, period);
    }
    while (window >= haystack) {
        assert(memory == 0);
        LOG_LINEUP_REV();
        iloop++;
        shift = table[(*window) & TABLE_MASK];
        window -= shift;
        if (shift != 0){
            LOG("Horspool skip\n");
            continue;
        }
        if (window < haystack){
            break;
        }
      no_shift:
        assert((window[0] & TABLE_MASK) == (needle[0] & TABLE_MASK));
        if (is_periodic) {
            i = Py_MIN(cut, len_needle - memory);
        } else {
            i = cut;
        }
        for (; i >= 0; i--) {
            ihits++;
            if (needle[i] != window[i]) {
                if (i > gap_jump_end) {
                    LOG("Early left half mismatch: jump by gap.\n");
                    assert(gap >= cut - i + 1);
                    window -= gap;
                }
                else {
                    LOG("Late left half mismatch: jump by n (>gap)\n");
                    assert(cut - i + 1 > gap);
                    window -= cut - i + 1;
                }
                memory = 0;
                break;
            }
        }
        if (i != -1){
            continue;
        }
        i = len_needle - 1;
        if (is_periodic) {
            i -= memory;
        }
        for (; i > cut; i--) {
            ihits++;
            if (needle[i] != window[i]) {
                LOG("Right half does not match.\n");
                window -= period;
                if (!is_periodic){
                    break;
                }
                memory = len_needle - period;
                if (window < haystack) {
                    break;
                }
                iloop++;
                Py_ssize_t shift = table[(*window) & TABLE_MASK];
                if (!shift){
                    goto no_shift;
                }
                // A mismatch has been identified to the right
                // of where i will next start, so we can jump
                // at least as far as if the mismatch occurred
                // on the first comparison.
                // REF>
                //  0 m +
                // [0 1 2 3]
                //      m 0
                inv_cut = len_needle - cut - 1;
                Py_ssize_t mem_jump = Py_MAX(inv_cut, memory) - inv_cut + 1;
                LOG("Skip with Memory.\n");
                memory = 0;
                window -= Py_MAX(shift, mem_jump);
                break;
            }
        }
        if (i == cut) {
            LOG("Found a match!\n");
            if (mode != FAST_COUNT) {
                return window - haystack;
            }
            count++;
            if (count == maxcount) {
                return maxcount;
            }
            window -= len_needle;
        }
    }
    // Loop Counter and Memory Access Counter Logging (Used for calibration)
    // In worst case scenario iloop == n - m
    // iloop == ihits indicates linear performance for quadratic problems
    LOG("iloop: %ld\n", iloop);
    LOG("ihits: %ld\n", ihits);
    if (mode == FAST_COUNT) {
        LOG("Counting finished.\n");
        return count;
    }
    else {
        LOG("Not found. Returning -1.\n");
        return -1;
    }
}


static Py_ssize_t
STRINGLIB(two_way_rfind)(const STRINGLIB_CHAR *haystack,
                         Py_ssize_t len_haystack,
                         const STRINGLIB_CHAR *needle,
                         Py_ssize_t len_needle,
                         Py_ssize_t maxcount,
                         int mode)
{
    int dir = -1;
    STRINGLIB(prework) pw;
    STRINGLIB(_preprocess)(needle, len_needle, &pw, dir, 1, 1);
    return STRINGLIB(_two_way_rev)(haystack, len_haystack, maxcount, mode, &pw);
}


static Py_ssize_t
STRINGLIB(horspool_find)(const STRINGLIB_CHAR* s, Py_ssize_t n,
                         const STRINGLIB_CHAR* p, Py_ssize_t m,
                         Py_ssize_t maxcount, int mode, int dynamic)
{
    /* Boyer–Moore–Horspool algorithm
       with optional dynamic fallback to Two-Way algorithm */
    int dir = 1;
    Py_ssize_t shift, i, j;
    Py_ssize_t mlast = m - 1;
    const Py_ssize_t width = n - m;
    const STRINGLIB_CHAR p_last = p[mlast];
    const STRINGLIB_CHAR *ss = s + mlast;
    STRINGLIB_CHAR s_last;

    // Pre-Work
    STRINGLIB(prework) pw;
    STRINGLIB(_preprocess)(p, m, &pw, dir, 0, 1);
    Py_ssize_t gap = pw.gap;
    SHIFT_TYPE *table = pw.table;

    // Use Bloom for len(needle) <= 64
    // Initialization is costly for long needles
    // And this is not much beneficial for large set(needle)
    unsigned long mask = 0;
    Py_ssize_t bloom_gap = 0;
    Py_ssize_t j_stop = m;
    if (m <= 64) {
        LOG("Initializing Bloom\n");
        // Note: bloom_gap("___aa") = 1
        j_stop -= 1;
        bloom_gap = m;
        STRINGLIB_BLOOM_ADD(mask, p_last);
        for (i = 0; i < mlast; i++) {
            STRINGLIB_BLOOM_ADD(mask, p[i]);
            if (p[i] == p_last) {
                bloom_gap = mlast - i;
            }
        }
    }
    // Horspool Calibration
    const float hrs_lcost = 4.0f;       // average loop cost
    const float hrs_hcost = 0.4f;       // false positive hit cost
    const int ih_min = 100;             // minimum FP hits to fallback
    // Two-Way Calibration
    const float twy_icost = 3.0f * m;   // initialization cost
    const float twy_lcost = 3.0f;       // loop cost
    /* Running variables */
    float loops_left, total_time_hrs;
    float exp_hrs, exp_twy;
    Py_ssize_t count = 0;
    Py_ssize_t iloop=0;
    Py_ssize_t ihits=0;
    // TODO> synchronize vvariable names!!!
    // const Py_ssize_t len_needle = m;
    // const Py_ssize_t len_haystack = n;
    // const STRINGLIB_CHAR *const needle = p;
    // const STRINGLIB_CHAR *window_last;
    // const STRINGLIB_CHAR *const haystack = s;
    // const STRINGLIB_CHAR *const haystack_end = haystack + len_haystack;
    for (i = 0; i <= width;) {
        iloop++;
        s_last = ss[i];
        // window_last = s_last;
        // LOG_LINEUP();
        if (bloom_gap) {
            if (s_last != p_last){
                if (!STRINGLIB_BLOOM(mask, ss[i+1])){
                    i += m + 1;
                    LOG("Bloom skip\n");
                }
                else {
                    shift = table[s_last & TABLE_MASK];
                    i += shift;
                    if (shift == 0){
                        i += 1;
                    }
                    LOG("Modified Horspool skip\n");
                }
                continue;
            }
            assert(s_last == p_last);
        }
        else {
            shift = table[s_last & TABLE_MASK];
            i += shift;
            if (shift != 0){
                LOG("Horspool skip\n");
                continue;
            }
            assert((ss[i] & TABLE_MASK) == (p_last & TABLE_MASK));
        }
        if (i > width){
            break;
        }
        for (j = 0; j < j_stop; j++) {
            ihits++;
            if (s[i + j] != p[j]) {
                break;
            }
        }
        if (j == j_stop) {
            LOG("Found a match at %ld!\n", i);
            if (mode != FAST_COUNT) {
                return i;
            }
            count++;
            if (count == maxcount) {
                return maxcount;
            }
            i += m;
        }
        else if (bloom_gap && !STRINGLIB_BLOOM(mask, ss[i+1])) {
            LOG("move by (m + 1) = %ld\n", m + 1);
            i += m + 1;
        }
        else {
            if (bloom_gap) {
                LOG("move by bloom gap = %ld\n", gap);
                i += bloom_gap;
            } else {
                LOG("move by gap = %ld\n", gap);
                i += gap;
            }
        }
        if (dynamic && ihits > ih_min) {
            loops_left = width - i + 1;
            total_time_hrs = iloop * hrs_lcost + ihits * hrs_hcost;
            exp_hrs = total_time_hrs / i * loops_left;
            exp_twy = twy_icost + loops_left * twy_lcost;
            if (exp_twy < exp_hrs) {
                STRINGLIB(_preprocess)(p, m, &pw, dir, 1, 0);
                Py_ssize_t res = STRINGLIB(_two_way)(
                    s + i, n - i, maxcount - count, mode, &pw);
                if (mode == FAST_SEARCH) {
                    return res == -1 ? -1 : res + i;
                }
                else {
                    return res + count;
                }
            }
        }
    }
    // Loop Counter and False Hit Counter Logging
    // In worst case scenario iloop > n - m.
    // Used for calibration and fallback decision
    LOG("iloop: %ld\n", iloop);
    LOG("ihits: %ld\n", ihits);
    return mode == FAST_COUNT ? count : -1;
}


static Py_ssize_t
STRINGLIB(horspool_rfind)(const STRINGLIB_CHAR* s, Py_ssize_t n,
                          const STRINGLIB_CHAR* p, Py_ssize_t m,
                          Py_ssize_t maxcount, int mode, int dynamic)
{
    /* Reverse Boyer–Moore–Horspool algorithm
       with optional dynamic fallback to Two-Way algorithm */
    int dir = -1;
    Py_ssize_t shift, i, j;
    Py_ssize_t mlast = m - 1;
    const Py_ssize_t width = n - m;
    const STRINGLIB_CHAR p_first = p[0];
    STRINGLIB_CHAR s_first;

    // Pre-Work
    STRINGLIB(prework) pw;
    STRINGLIB(_preprocess)(p, m, &pw, dir, 0, 1);
    Py_ssize_t gap = pw.gap;
    SHIFT_TYPE *table = pw.table;

    // Use Bloom for len(needle) <= 64
    // Initialization is costly for long needles
    // And this is not much beneficial for large set(needle)
    unsigned long mask = 0;
    Py_ssize_t bloom_gap = 0;
    Py_ssize_t j_stop = -1;
    if (m <= 64) {
        LOG("Initializing Bloom\n");
        // Note: bloom_gap("___aa") = 1
        j_stop += 1;
        bloom_gap = m;
        STRINGLIB_BLOOM_ADD(mask, p_first);
        for (i = mlast; i > 0; i--) {
            STRINGLIB_BLOOM_ADD(mask, p[i]);
            if (p[i] == p_first) {
                bloom_gap = i;
            }
        }

    }
    // Horspool Calibration
    const float hrs_lcost = 4.0f;       // average loop cost
    const float hrs_hcost = 0.4f;       // false positive hit cost
    const int ih_min = 100;             // minimum FP hits to fallback
    // Two-Way Calibration
    const float twy_icost = 3.0f * m;   // initialization cost
    const float twy_lcost = 3.0f;       // loop cost
    /* Running variables */
    float loops_left, loops_past, total_time_hrs;
    float exp_hrs, exp_twy;
    Py_ssize_t count = 0;
    Py_ssize_t iloop=0;
    Py_ssize_t ihits=0;
    // TODO>
    // const Py_ssize_t len_needle = m;
    // const Py_ssize_t len_haystack = n;
    // const STRINGLIB_CHAR *const needle = p;
    // const STRINGLIB_CHAR *window_last = s + len_needle - 1;
    // const STRINGLIB_CHAR *const haystack = s;
    // const STRINGLIB_CHAR *const haystack_end = haystack + len_haystack;
    // const STRINGLIB_CHAR *window = haystack_end - len_needle;
    for (i = width; i >= 0;) {
        iloop++;
        s_first = s[i];
        // window = s_first;
        // LOG_LINEUP_REV();
        if (bloom_gap) {
            if (s_first != p_first){
                if (i > 0 && !STRINGLIB_BLOOM(mask, s[i - 1])){
                    i -= m + 1;
                    LOG("Bloom skip\n");
                }
                else {
                    shift = table[s_first & TABLE_MASK];
                    i -= shift;
                    if (shift == 0){
                        i -= 1;
                    }
                    LOG("Modified Horspool skip\n");
                }
                continue;
            }
            assert(s_first == p_first);
        }
        else {
            shift = table[s_first & TABLE_MASK];
            i -= shift;
            if (shift != 0){
                LOG("Horspool skip\n");
                continue;
            }
            assert((s[i] & TABLE_MASK) == (p_first & TABLE_MASK));
        }
        if (i < 0){
            break;
        }
        for (j = mlast; j > j_stop; j--) {
            ihits++;
            if (s[i + j] != p[j]) {
                break;
            }
        }
        if (j == j_stop) {
            LOG("Found a match at %ld!\n", i);
            if (mode != FAST_COUNT) {
                return i;
            }
            count++;
            if (count == maxcount) {
                return maxcount;
            }
            i -= m;
        }
        else if (bloom_gap && i > 0 && !STRINGLIB_BLOOM(mask, s[i - 1])) {
            LOG("move by (m + 1) = %ld\n", m + 1);
            i -= m + 1;
        }
        else {
            if (bloom_gap) {
                LOG("move by bloom gap = %ld\n", gap);
                i -= bloom_gap;
            } else {
                LOG("move by gap = %ld\n", gap);
                i -= gap;
            }
        }
        if (dynamic && ihits > ih_min) {
            loops_past = width - i;
            loops_left = i + 1;
            total_time_hrs = iloop * hrs_lcost + ihits * hrs_hcost;
            exp_hrs = total_time_hrs / loops_past * loops_left;
            exp_twy = twy_icost + loops_left * twy_lcost;
            if (exp_twy < exp_hrs) {
                STRINGLIB(_preprocess)(p, m, &pw, 1, 0, dir);
                Py_ssize_t res = STRINGLIB(_two_way_rev)(
                    s, n - loops_past, maxcount - count, mode, &pw);
                if (mode == FAST_SEARCH) {
                    return res == -1 ? -1 : res;
                }
                else {
                    return res + count;
                }
            }
        }
    }
    // Loop Counter and False Hit Counter Logging
    // In worst case scenario iloop > n - m.
    // Used for calibration and fallback decision
    LOG("iloop: %ld\n", iloop);
    LOG("ihits: %ld\n", ihits);
    return mode == FAST_COUNT ? count : -1;
}


#undef SHIFT_TYPE
#undef NOT_FOUND
#undef SHIFT_OVERFLOW
#undef TABLE_SIZE_BITS
#undef TABLE_SIZE
#undef TABLE_MASK

#undef LOG
#undef LOG_STRING
#undef LOG_LINEUP
#undef LOG_LEVEL


static inline Py_ssize_t
STRINGLIB(default_find)(const STRINGLIB_CHAR* s, Py_ssize_t n,
                        const STRINGLIB_CHAR* p, Py_ssize_t m,
                        Py_ssize_t maxcount, int mode)
{
    const Py_ssize_t w = n - m;
    Py_ssize_t mlast = m - 1, count = 0;
    const STRINGLIB_CHAR last = p[mlast];
    const STRINGLIB_CHAR *const ss = &s[mlast];

    // Initialize Bloom
    // Note: gap("___aa") = 0
    Py_ssize_t gap = mlast;
    unsigned long mask = 0;
    STRINGLIB_BLOOM_ADD(mask, last);
    for (Py_ssize_t i = 0; i < mlast; i++) {
        STRINGLIB_BLOOM_ADD(mask, p[i]);
        if (p[i] == last) {
            gap = mlast - i - 1;
        }
    }

    for (Py_ssize_t i = 0; i <= w; i++) {
        if (ss[i] == last) {
            /* candidate match */
            Py_ssize_t j;
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
                i = i + gap;
            }
        }
        else {
            /* skip: check if next character is part of pattern */
            if (!STRINGLIB_BLOOM(mask, ss[i+1])) {
                i = i + m;
            }
        }
    }
    return mode == FAST_COUNT ? count : -1;
}


static Py_ssize_t
STRINGLIB(adaptive_find)(const STRINGLIB_CHAR* s, Py_ssize_t n,
                         const STRINGLIB_CHAR* p, Py_ssize_t m,
                         Py_ssize_t maxcount, int mode)
{
    const Py_ssize_t w = n - m;
    Py_ssize_t mlast = m - 1, count = 0;
    Py_ssize_t gap = mlast;
    Py_ssize_t hits = 0, res;
    const STRINGLIB_CHAR last = p[mlast];
    const STRINGLIB_CHAR *const ss = &s[mlast];

    unsigned long mask = 0;
    for (Py_ssize_t i = 0; i < mlast; i++) {
        STRINGLIB_BLOOM_ADD(mask, p[i]);
        if (p[i] == last) {
            gap = mlast - i - 1;
        }
    }
    STRINGLIB_BLOOM_ADD(mask, last);

    for (Py_ssize_t i = 0; i <= w; i++) {
        if (ss[i] == last) {
            /* candidate match */
            Py_ssize_t j;
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
            hits += j + 1;
            if (hits > m / 4 && w - i > 2000) {
                res = STRINGLIB(two_way_find)(
                    s + i, n - i, p, m, maxcount, mode);
                if (mode == FAST_SEARCH) {
                    return res == -1 ? -1 : res + i;
                }
                else {
                    return res + count;
                }
            }
            /* miss: check if next character is part of pattern */
            if (!STRINGLIB_BLOOM(mask, ss[i+1])) {
                i = i + m;
            }
            else {
                i = i + gap;
            }
        }
        else {
            /* skip: check if next character is part of pattern */
            if (!STRINGLIB_BLOOM(mask, ss[i+1])) {
                i = i + m;
            }
        }
    }
    return mode == FAST_COUNT ? count : -1;
}


static Py_ssize_t
STRINGLIB(default_rfind)(const STRINGLIB_CHAR* s, Py_ssize_t n,
                         const STRINGLIB_CHAR* p, Py_ssize_t m,
                         Py_ssize_t maxcount, int mode)
{
    /* create compressed boyer-moore delta 1 table */
    unsigned long mask = 0;
    Py_ssize_t i, j, mlast = m - 1, skip = m - 1, w = n - m;

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
    return -1;
}


static inline Py_ssize_t
STRINGLIB(count_char)(const STRINGLIB_CHAR *s, Py_ssize_t n,
                      const STRINGLIB_CHAR p0, Py_ssize_t maxcount)
{
    Py_ssize_t i, count = 0;
    for (i = 0; i < n; i++) {
        if (s[i] == p0) {
            count++;
            if (count == maxcount) {
                return maxcount;
            }
        }
    }
    return count;
}


Py_LOCAL_INLINE(Py_ssize_t)
FASTSEARCH(const STRINGLIB_CHAR* s, Py_ssize_t n,
           const STRINGLIB_CHAR* p, Py_ssize_t m,
           Py_ssize_t maxcount, int mode)
{
    if (n < m || (mode == FAST_COUNT && maxcount == 0)) {
        return -1;
    }
    /* look for special cases */
    if (m <= 1) {
        if (m <= 0) {
            return -1;
        }
        /* use special case for 1-character strings */
        if (mode == FAST_SEARCH)
            return STRINGLIB(find_char)(s, n, p[0]);
        else if (mode == FAST_RSEARCH)
            return STRINGLIB(rfind_char)(s, n, p[0]);
        else {
            return STRINGLIB(count_char)(s, n, p[0], maxcount);
        }
    }
    else if (n == m) {
         /* use special case when both strings are of equal length */
         int res = memcmp(s, p, m * sizeof(STRINGLIB_CHAR));
         if (mode == FAST_COUNT){
             return res == 0 ? 1 : 0;
         } else {
             return res == 0 ? 0 : -1;
         }
    }
    if (mode != FAST_RSEARCH) {
        return STRINGLIB(horspool_find)(s, n, p, m, maxcount, mode, 1);
        // return STRINGLIB(two_way_find)(s, n, p, m, maxcount, mode);
    }
    else {
        /* FAST_RSEARCH */
        return STRINGLIB(horspool_rfind)(s, n, p, m, maxcount, mode, 1);
        // return STRINGLIB(two_way_rfind)(s, n, p, m, maxcount, mode);
    }
}
