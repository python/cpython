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
# define LOG2(...)
# define LOG_STRING(s, n) if (n < 100) {                        \
    printf("\"%.*s\"", (int)(n), s);                            \
}
# define LOG_LINEUP()
#elif LOG_LEVEL == 2 && STRINGLIB_SIZEOF_CHAR == 1
# define LOG(...) printf(__VA_ARGS__)
# define LOG2(...) printf(__VA_ARGS__)
# define LOG_STRING(s, n) if (n < 100) {                        \
    printf("\"%.*s\"", (int)(n), s);                            \
}
# define LOG_LINEUP() do {                                      \
    if (n < 100) {                                              \
        LOG("> ");   LOG_STRING(s, n);                          \
        LOG("\n> "); LOG("%*s",(int)(ss - s + ip - p_end), ""); \
        LOG_STRING(p, m); LOG("\n");                            \
    }                                                           \
} while(0)
#else
# define LOG(...)
# define LOG2(...)
# define LOG_STRING(s, n)
# define LOG_LINEUP()
#endif


Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(_lex_search)(const STRINGLIB_CHAR *needle,
                       Py_ssize_t needle_len,
                       Py_ssize_t *return_period,
                       int invert_alphabet,
                       int reversed)
{
    /* Do a lexicographic search. Essentially this:
           >>> max(needle[i:] for i in range(len(needle)+1))
       Also find the period of the right half.
       Direction:
           reversed : {0, 1}
              if reversed == 1, then the problem is reverse
           In short:
              _lex_search(x, 1) == _lex_search(x[::-1], 0)

           Returned cut is the size of the cut towards chosen reversed. E.g.:
              >>> x = '1234'
              >>> cut, period = factorize(x, reversed=0)    # cut = 0
              >>> cut
              0
              >>> cut_idx = cut
              >>> x[:cut_idx], x[cut_idx:]
              '', '1234'
              >>> x = '4321'
              >>> cut, period = factorize(x, reversed=1)
              >>> cut
              0
              >>> cut_idx = len(x) - cut
              >>> x[:cut_idx], x[cut_idx:]
              '4321', ''
    */
    int dir = reversed ? -1 : 1;
    // starting position from chosen direction
    Py_ssize_t stt = reversed ? needle_len - 1 : 0;
    Py_ssize_t max_suffix = 0;
    Py_ssize_t candidate = 1;
    Py_ssize_t k = 0;
    // The period of the right half.
    Py_ssize_t period = 1;
    STRINGLIB_CHAR a, b;
    // directional indexers for candidate & max_suffix
    // It is more efficient way to achieve:
    //      a = needle[stt + dir * (candidate + k)];
    //      b = needle[stt + dir * (max_suffix + k)];
    Py_ssize_t cp, sp;
    cp = stt + dir * candidate;
    sp = stt + dir * max_suffix;
    while (candidate + k < needle_len) {
        // each loop increases (in chosen direction) candidate + k + max_suffix
        a = needle[cp];
        b = needle[sp];
        // check if the suffix at candidate is better than max_suffix
        if (invert_alphabet ? (b < a) : (a < b)) {
            // Fell short of max_suffix.
            // The next k + 1 characters are non-increasing
            // from candidate, so they won't start a maximal suffix.
            candidate += k + 1;
            cp += dir;
            sp -= dir * k;
            k = 0;
            // We've ruled out any period smaller than what's
            // been scanned since max_suffix.
            period = candidate - max_suffix;
        }
        else if (a == b) {
            if (k + 1 != period) {
                // Keep scanning the equal strings
                k++;
                cp += dir;
                sp += dir;
            }
            else {
                // Matched a whole period.
                // Start matching the next period.
                candidate += period;
                cp = stt + dir * candidate;
                sp -= dir * k;
                k = 0;
            }
        }
        else {
            // Did better than max_suffix, so replace it.
            max_suffix = candidate;
            candidate++;
            k = 0;
            period = 1;
            cp = stt + dir * candidate;
            sp = stt + dir * max_suffix;
        }
    }
    *return_period = period;
    return max_suffix;
}


Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(_factorize)(const STRINGLIB_CHAR *needle,
                      Py_ssize_t needle_len,
                      Py_ssize_t *return_period,
                      int reversed)
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
       period of the original string.

       Direction:
           reversed : {0, 1}
               if reversed == 1, then the problem is reverse
           In short:
               _factorize(x, 1) == _factorize(x[::-1], 0)
           See docstring of _lex_search if still unclear
    */
    Py_ssize_t cut1, period1, cut2, period2, cut, period;
    cut1 = STRINGLIB(_lex_search)(needle, needle_len, &period1, 0, reversed);
    cut2 = STRINGLIB(_lex_search)(needle, needle_len, &period2, 1, reversed);
    // Take the later cut.
    if (cut1 > cut2) {
        period = period1;
        cut = cut1;
    }
    else {
        period = period2;
        cut = cut2;
    }
    *return_period = period;
    return cut;
}


// Bloom Setup
#if LONG_BIT >= 128
#  define STRINGLIB_BLOOM_MASK 127
#elif LONG_BIT >= 64
#  define STRINGLIB_BLOOM_MASK 63
#elif LONG_BIT >= 32
#  define STRINGLIB_BLOOM_MASK 31
#else
#  error "LONG_BIT is smaller than 32"
#endif

#define STRINGLIB_BLOOM_ADD(mask, ch) \
    ((mask |= (1UL << ((ch) & (STRINGLIB_BLOOM_MASK)))))
#define STRINGLIB_BLOOM(mask, ch)     \
    ((mask &  (1UL << ((ch) & (STRINGLIB_BLOOM_MASK)))))

// Boyer-Moore "Bad Character" table Setup
#define SHIFT_TYPE uint8_t
#define MAX_SHIFT UINT8_MAX

#define TABLE_SIZE 128U
#define TABLE_MASK 127U


typedef struct STRINGLIB(_pre) {
    // Data & Direction
    const STRINGLIB_CHAR *needle;   // needle
    Py_ssize_t needle_len;          // length of the needle
    int reversed;                   // Reverse Direction
    // Containment mask and Full "Good Suffix" gap for last character
    unsigned long mask;             // Containment bloom type mask
    Py_ssize_t true_gap;            // Actual Last character gap
    // "Bad Character" table and Filtered "Good Suffix" gap for last character
    SHIFT_TYPE table[TABLE_SIZE];   // Boyer-Moore "Bad Character" table
    Py_ssize_t gap;                 // Filterd Last Character Gap <= true_gap
    // Critical Factorization
    Py_ssize_t cut;                 // Critical Factorization Cut Length
    Py_ssize_t period;              // Global Period of the string
    int is_periodic;
} STRINGLIB(prework);


static void
STRINGLIB(prepare_search)(STRINGLIB(prework) *pw,
                          int bloom_mask_and_true_gap,
                          int bc_table_and_gap,
                          int critical_factorization)
{
    /*
       This function prepares different search algorithm methods

          horspool_find:    bc_table_and_gap = 1
                            bloom_mask_and_true_gap = 1 (optional)

          two_way_find:     bc_table_and_gap = 1
                            critical_factorization = 1
    */
    const STRINGLIB_CHAR *p = pw->needle;
    Py_ssize_t m = pw->needle_len;
    int reversed = pw->reversed;
    int dir = reversed ? -1 : 1;
    Py_ssize_t stt = reversed ? m - 1 : 0;
    Py_ssize_t end = reversed ? 0 : m - 1;
    Py_ssize_t j, jp;
    // 1. Containment mask and Full "Good Suffix" gap for last character
    if (bloom_mask_and_true_gap) {
        const STRINGLIB_CHAR p_last = p[end];
        pw->mask = 0;
        pw->true_gap = m;
        // Note: true_gap("___aa") = 1
        STRINGLIB_CHAR p_tmp;
        STRINGLIB_BLOOM_ADD(pw->mask, p_last);
        j = 1;
        jp = end - dir * j;
        for (; j < m; j++) {
            p_tmp = p[jp];
            STRINGLIB_BLOOM_ADD(pw->mask, p_tmp);
            if (pw->true_gap == m && p_tmp == p_last) {
                pw->true_gap = j;
            }
            jp -= dir;
        }
        LOG("Good Suffix Full Gap: %ld\n", pw->true_gap);
    }

    if (bc_table_and_gap) {
        // 2.1. Fill a compressed Boyer-Moore "Bad Character" table
        Py_ssize_t not_found_shift = Py_MIN(m, MAX_SHIFT);
        for (Py_ssize_t j = 0; j < (Py_ssize_t)TABLE_SIZE; j++) {
            pw->table[j] = Py_SAFE_DOWNCAST(not_found_shift,
                                            Py_ssize_t, SHIFT_TYPE);
        }
        j = m - not_found_shift;
        jp = stt + dir * j;
        for (; j < m; j++) {
            SHIFT_TYPE shift = Py_SAFE_DOWNCAST(m - 1 - j,
                                                Py_ssize_t, SHIFT_TYPE);
            pw->table[p[jp] & TABLE_MASK] = shift;
            jp += dir;
        }
        // 2.2. Initialize "Good Suffix" Last Character Gap
        // Note: gap("___aa") = 1
        pw->gap = m;
        STRINGLIB_CHAR last = p[end] & TABLE_MASK;
        j = 1;
        jp = end - dir * j;
        for (; j < m; j++) {
            STRINGLIB_CHAR x = p[jp] & TABLE_MASK;
            if (x == last) {
                pw->gap = j;
                break;
            }
            jp -= dir;
        }
        LOG("Good Suffix Partial Gap: %ld\n", pw->gap);
    }

    // 3. Calculate Critical Factorization
    if (critical_factorization) {
        Py_ssize_t period;
        Py_ssize_t cut = STRINGLIB(_factorize)(p, m, &period, reversed);
        assert(cut + period <= m);
        int cmp;
        if (reversed) {
            Py_ssize_t cut_idx = m - cut;
            cmp = memcmp(p + cut_idx, p + cut_idx - period,
                         cut * STRINGLIB_SIZEOF_CHAR);
        }
        else {
            cmp = memcmp(p, p + period, cut * STRINGLIB_SIZEOF_CHAR);
        }
        int is_periodic = cmp == 0;
        if (is_periodic) {
            assert(cut <= m/2);
            assert(cut < period);
            LOG("Needle is periodic.\n");
        }
        else {
            // Upper bound on the period
            period = Py_MAX(cut, m - cut) + 1;
            LOG("Needle is not periodic.\n");
        }
        pw->cut = cut;
        pw->period = period;
        pw->is_periodic = is_periodic;

        LOG("Cut: %ld & Period: %ld\n", cut, period);
        LOG("split: ");
        LOG_STRING(p, cut);
        LOG(" + ");
        LOG_STRING(p + cut, m - cut);
        LOG("\n");
    }
}


static Py_ssize_t
STRINGLIB(two_way_find)(const STRINGLIB_CHAR *haystack,
                        const Py_ssize_t haystack_len,
                        int find_mode, Py_ssize_t maxcount,
                        STRINGLIB(prework) *pw)
{
    /* Crochemore and Perrin's (1991) Two-Way algorithm.
        See http://www-igm.univ-mlv.fr/~lecroq/string/node26.html#SECTION00260

       Initialization
          pw needs to be initialized using prepare_search with
             bc_table_and_gap = 1 and critical_factorization = 1

       Variable Names
          s - haystack
          p - needle
          n - len(haystack)
          m - len(needle)

       Bi-Directional Conventions:
          See docstring of horspool_find

       Critical factorization reversion:
          See docstring of _factorize
    */
    LOG(find_mode ? "Two-way Find.\n" : "Two-way Count.\n");

    // Collect Data
    int reversed = pw->reversed;
    int dir = reversed ? -1 : 1;
    const STRINGLIB_CHAR *const p = pw->needle;
    const STRINGLIB_CHAR *const s = haystack;
    const Py_ssize_t m = pw->needle_len;
    const Py_ssize_t n = haystack_len;
    LOG("haystack: "); LOG_STRING(s, n); LOG("\n");
    LOG("needle  : "); LOG_STRING(p, m); LOG("\n");

    // Retrieve Preparation
    SHIFT_TYPE *table = pw->table;
    const Py_ssize_t gap = pw->gap;
    const Py_ssize_t cut = pw->cut;
    int is_periodic = pw->is_periodic;
    Py_ssize_t period = is_periodic ? pw->period : Py_MAX(gap, pw->period);
    Py_ssize_t gap_jump_end = Py_MIN(m, cut + gap);

    // Direction Independent
    const Py_ssize_t w = n - m;
    const Py_ssize_t m_m1 = m - 1;
    // Direction Dependent
    const Py_ssize_t p_stt = dir == 1 ? 0 : m - 1;
    const Py_ssize_t s_stt = dir == 1 ? 0 : n - 1;
    const Py_ssize_t p_end = dir == 1 ? m - 1 : 0;
    const Py_ssize_t dir_m_m1 = reversed ? -m_m1 : m_m1;
    const STRINGLIB_CHAR *const ss = s + s_stt + dir_m_m1;

    // Indexers (ip and jp are directional indices. e.g. ip = pos * i)
    Py_ssize_t i, j, ip, jp;
    // Temporary
    Py_ssize_t j_off;   // offset from last to leftmost window index
    Py_ssize_t shift;
    int do_mem_jump = 0;
    // Counters
    Py_ssize_t iloop = 0;
    Py_ssize_t ihits = 0;
    Py_ssize_t count = 0;
    Py_ssize_t memory = 0;
    // Loop
    for (i = 0; i <= w;) {
        iloop++;
        ip = reversed ? -i : i;
        LOG2("Last window ch: %c\n", ss[ip]);
        LOG_LINEUP();
        shift = table[ss[ip] & TABLE_MASK];
        if (shift){
            if (do_mem_jump) {
                // A mismatch has been identified to the right
                // of where i will next start, so we can jump
                // at least as far as if the mismatch occurred
                // on the first comparison.
                shift = Py_MAX(shift, Py_MAX(1, memory - cut + 1));
                memory = 0;
                LOG("Skip with Memory: %ld\n", shift);
            }
            assert(memory == 0);
            LOG("Shift: %ld\n", shift);
            i += shift;
            do_mem_jump = 0;
            continue;
        }
        assert((ss[ip] & TABLE_MASK) == (p[m - 1] & TABLE_MASK));
        j_off = ip - p_end;
        j = is_periodic ? Py_MAX(cut, memory) : cut;
        jp = p_stt + dir * j;
        for (; j < m; j++) {
            ihits++;
            LOG2("Checking j=%ld: %c vs %c\n", j, ss[j_off + jp], p[jp]);
            if (ss[j_off + jp] != p[jp]) {
                if (j < gap_jump_end) {
                    LOG("Early later half mismatch: jump by gap.\n");
                    assert(gap >= j - cut + 1);
                    i += gap;
                }
                else {
                    LOG("Late later half mismatch: jump by n (>gap)\n");
                    assert(j - cut + 1 > gap);
                    i += j - cut + 1;
                }
                memory = 0;
                break;
            }
            jp += dir;
        }
        if (j != m) {
            continue;
        }

        j = Py_MIN(memory, cut);    // Needed for j == cut below to be correct
        jp = p_stt + dir * j;
        for (; j < cut; j++) {
            ihits++;
            LOG2("Checking j=%ld: %c vs %c\n", j, ss[j_off + jp], p[jp]);
            if (ss[j_off + jp] != p[jp]) {
                LOG("First half does not match.\n");
                if (is_periodic) {
                    memory = m - period;
                    do_mem_jump = 1;
                }
                i += period;
                break;
            }
            jp += dir;
        }
        if (j == cut) {
            LOG("Found a match!\n");
            if (find_mode) {
                return reversed ? w - i : i;
            }
            if (++count == maxcount) {
                return maxcount;
            }
            memory = 0;
            i += m;
        }

    }
    // Loop Counter and Memory Access Counter Logging (Used for calibration)
    // In worst case scenario iloop == n - m
    // iloop == ihits indicates linear performance for quadratic problems
    LOG("iloop: %ld\n", iloop);
    LOG("ihits: %ld\n", ihits);
    return find_mode ? -1 : count;
}


static Py_ssize_t
STRINGLIB(horspool_find)(const STRINGLIB_CHAR* haystack,
                         const Py_ssize_t haystack_len,
                         int find_mode, Py_ssize_t maxcount,
                         int dynamic, STRINGLIB(prework) *pw)
{
    /* Boyer–Moore–Horspool algorithm
       with optional dynamic fallback to Two-Way algorithm

       Initialization
          pw needs to be initialized using prepare_search with
             bc_table_and_gap = 1 and optionally bloom_mask_and_true_gap = 1

       Variable Names
          s - haystack
          p - needle
          n - len(haystack)
          m - len(needle)

       Bi-Directional Conventions:
          stt - start index
          end - end index
          ss - pointer to last window index that matches last needle character
          >>> dir_fwd, dir_rev = 1, -1
          >>> s = [0, 1, 2, 3, 4, 5]
          >>> s_stt_fwd, s_stt_rev = 0, 5
          >>> s_end_fwd, s_end_rev = 5, 0
          >>> p = [0, 1]
          >>> m = len(p)
          >>> s = 0
          >>> ss_fwd = s + s_stt_fwd + dir_fwd * (m - 1)
          >>> ss_rev = s + s_stt_rev + dir_rev * (m - 1)
          >>> ss_fwd, ss_rev
          (1, 4)

          There is one more important variable: j_off
          It brings ss in alignment with a needle.
          So that it stands at the first absolute index of the window

          >>> i = 0       # first step
          >>> p_stt_fwd, p_stt_rev = 0, 1
          >>> p_end_fwd, p_end_rev = 1, 0
          >>> j_off_fwd = dir_fwd * i - p_end_fwd
          >>> ss_fwd + j_off_fwd
          0

          such that [0, 1, 2, 3, 4, 5]
                    [0, 1]
                     * - both indices are at 0

          >>> j_off_rev = dir_rev * i - p_end_rev
          >>> ss_rev + j_off_rev
          4

          such that [0, 1, 2, 3, 4, 5]
                                [0, 1]
                                 * - both indices are at 0
          Finally, which side it iterates from is determined by:
             jp = p_stt + (reversed ? -j : j);
          , where j is an increasing needle-size counter in both cases

          With this transformation the problem becomes direction agnostic

       Dynamic mode
          'Horspool' algorithm will switch to `two_way_find` if it predicts
              that it can solve the problem faster.

       Calibration
          The simple model for run time of search algorithm is as follows:
          loop - actual loop that happens (not theoretical)
          init_cost - initialization cost per 1 needle character in ns
          loop_cost - cost of 1 main loop
          hit_cost  - cost of 1 false positive character check
          avg_hit - average number of false positive hits per 1 loop

          m = len(needle)
          run_time = init_cost * m + n_loops * (loop_cost + hit_cost * avg_hit)

             Note, n_loops * avg_hit is what causes quadratic complexity.

          Calibrate:
             1. expose function to run without handling special cases first.
             2. set dynamic = 0
             3. Enable counter printing to know how many hits and loops
                happened. see printf(iloop|ihits) at the end of the function

             4. init_cost = run_time(horspool_find(s='', p='*' * m)) / m

             5. `two_way` only has loop cost.
                            run_time(two_way_find(s='*' * 1000)) - init_cost
                loop_cost = ------------------------------------------------
                                       n_loops (from stdout)
                Note, iloop & ihits of `two_way` should be the same.

             6. To get loop_cost and hit_cost of `horspool_find` solve
                   equation system representing 2 different runs
                n_loops1 * loop_cost + n_hits1 * hit_cost = run_time(problem_1)
                n_loops2 * loop_cost + n_hits2 * hit_cost = run_time(problem_2)

                init_cost of `horspool` for larger problems is negligible
                Furthermore, it is not used as it has already happened

             7. Run above for different problems. Take averages.
                Compare with current calibration constants.

             8. Current calibration works well, but is not perfect.
                Maybe you can come up with more accurate model.
    */
    LOG(find_mode ? "Horspool Find\n" : "Horspool Count.\n");

    // Collect Data
    int reversed = pw->reversed;
    int dir = reversed ? -1 : 1;
    const STRINGLIB_CHAR *const p = pw->needle;
    const STRINGLIB_CHAR *const s = haystack;
    const Py_ssize_t m = pw->needle_len;
    const Py_ssize_t n = haystack_len;
    LOG("haystack: "); LOG_STRING(s, n); LOG("\n");
    LOG("needle  : "); LOG_STRING(p, m); LOG("\n");

    // Direction Independent
    const Py_ssize_t m_m1 = m - 1;
    const Py_ssize_t m_p1 = m + 1;
    const Py_ssize_t w = n - m;

    // Retrieve Preparation
    int bloom_on = pw->true_gap != 0;
    unsigned long mask = pw->mask;
    // If bloom_on last character always matches so no need to check it
    Py_ssize_t j_stop = bloom_on ? m_m1 : m;
    Py_ssize_t gap = bloom_on ? pw->true_gap : pw->gap;
    SHIFT_TYPE *table = pw->table;

    // Direction Dependent
    const Py_ssize_t s_stt = reversed ? n - 1 : 0;
    const Py_ssize_t p_stt = reversed ? m - 1 : 0;
    const Py_ssize_t p_end = reversed ? 0 : m - 1;
    const Py_ssize_t dir_m_m1 = reversed ? -m_m1 : m_m1;
    const STRINGLIB_CHAR *const ss = s + s_stt + dir_m_m1;
    const STRINGLIB_CHAR p_last = p[p_end];

    // Indexers (ip and jp are directional indices. e.g. ip = pos * i)
    Py_ssize_t i, j, ip, jp;

    // Horspool Calibration
    const double hrs_lcost = 4.0;               // average loop cost
    const double hrs_hcost = 0.4;               // false positive hit cost
    // Two-Way Calibration
    const double twy_icost = 3.5 * (double)m;   // total initialization cost
    const double twy_lcost = 3.0;               // loop cost
    // Temporary
    double exp_hrs, exp_twy, ll;   // expected run times & loops left
    Py_ssize_t j_off;              // offset from last to leftmost window index
    Py_ssize_t shift;
    STRINGLIB_CHAR s_last;
    // Counters
    Py_ssize_t count = 0;
    Py_ssize_t iloop = 0;
    Py_ssize_t ihits = 0, ihits_last = 0;
    // Loop
    for (i = 0; i <= w;) {
        iloop++;
        ip = reversed ? -i : i;
        s_last = ss[ip];
        LOG2("Last window ch: %c\n", s_last);
        if (bloom_on) {
            shift = 0;
            if (s_last != p_last) {
                shift = i < w && !STRINGLIB_BLOOM(mask, ss[ip+dir]) ?
                        m_p1 : Py_MAX(table[s_last & TABLE_MASK], 1);
            }
            if (shift) {
                LOG("Shift: %ld\n", shift);
                i += shift;
                continue;
            }
            assert(s_last == p_last);
        }
        else {
            shift = table[s_last & TABLE_MASK];
            if (shift) {
                LOG("Shift: %ld\n", shift);
                i += shift;
                continue;
            }
            assert((s_last & TABLE_MASK) == (p_last & TABLE_MASK));
        }
        j_off = ip - p_end;
        jp = p_stt;
        for (j = 0; j < j_stop; j++) {
            ihits++;
            LOG2("Checking j=%ld: %c vs %c\n", j, ss[j_off + jp], p[jp]);
            if (ss[j_off + jp] != p[jp]) {
                break;
            }
            jp += dir;
        }

        if (j == j_stop) {
            LOG("Found a match!\n");
            if (find_mode) {
                return reversed ? w - i : i;
            }
            if (++count == maxcount) {
                return maxcount;
            }
            i += m;
        }
        else if (bloom_on && i < w && !STRINGLIB_BLOOM(mask, ss[ip+dir])) {
            // full skip: check if next character is part of pattern
            LOG("Move by (m + 1) = %ld\n", m_p1);
            i += m_p1;
        }
        else {
            LOG("Move by gap = %ld\n", gap);
            i += gap;
        }

        if (dynamic && ihits - ihits_last >= 100) {
            ll = (double)(w - i + 1);
            exp_hrs = ((double)iloop * hrs_lcost +
                       (double)ihits * hrs_hcost) / (double)i * ll;
            exp_twy = twy_icost + ll * twy_lcost;
            if (exp_twy < exp_hrs) {
                LOG("switching to two-way algorithm: n=%ld, m=%ld\n", n, m);
                STRINGLIB(prepare_search)(pw, 0, 0, 1);
                Py_ssize_t res = STRINGLIB(two_way_find)(
                        reversed ? s : s + i, n - i,
                        find_mode, maxcount - count, pw);
                if (find_mode) {
                    return res == -1 ? -1 : (reversed ? res : res + i);
                }
                else {
                    return res + count;
                }
            }
            ihits_last = ihits;
        }
    }
    // Loop Counter and False Hit Counter Logging
    // In worst case scenario: ihits == (n - m) * m
    // Used for calibration and fallback decision
    LOG("iloop: %ld\n", iloop);
    LOG("ihits: %ld\n", ihits);
    return find_mode ? -1 : count;
}

#undef STRINGLIB_BLOOM_MASK

#undef SHIFT_TYPE
#undef TABLE_SIZE
#undef TABLE_MASK

#undef LOG
#undef LOG_STRING
#undef LOG_LINEUP
#undef LOG_LEVEL


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


static inline Py_ssize_t
STRINGLIB(count_char_no_maxcount)(const STRINGLIB_CHAR *s, Py_ssize_t n,
                                  const STRINGLIB_CHAR p0)
/* A specialized function of count_char that does not cut off at a maximum.
   As a result, the compiler is able to vectorize the loop. */
{
    Py_ssize_t count = 0;
    for (Py_ssize_t i = 0; i < n; i++) {
        if (s[i] == p0) {
            count++;
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
            if (maxcount == PY_SSIZE_T_MAX) {
                return STRINGLIB(count_char_no_maxcount)(s, n, p[0]);
            }
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

    STRINGLIB(prework) pw = {
        .needle = p,
        .needle_len = m,
        .reversed = mode == FAST_RSEARCH,
        .true_gap = 0,
    };
    // Use Bloom for len(haystack) >= 30 * len(needle)
    int bloom_mask_and_true_gap = n >= 30 * m;
    STRINGLIB(prepare_search)(&pw, bloom_mask_and_true_gap, 1, 0);
    int find_mode = mode != FAST_COUNT;
    int dynamic = 1;
    return STRINGLIB(horspool_find)(s, n, find_mode, maxcount, dynamic, &pw);
}
