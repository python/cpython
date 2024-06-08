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
STRINGLIB(_lex_search)(const STRINGLIB_CHAR *p,
                       Py_ssize_t m,
                       Py_ssize_t *return_period,
                       int invert_alphabet,
                       int dir)
{
    /* Do a lexicographic search. Essentially this:
           >>> max(needle[i:] for i in range(len(needle)+1))
       Also find the period of the right half.
       Direction:
           dir : {-1, 1}
              if dir == -1, then the problem is reverse
           In short:
              _lex_search(x, -1) == _lex_search(x[::-1], 1)

           Returned cut is the size of the cut towards chosen direction. E.g.:
              >>> x = '1234'
              >>> cut, period = factorize(x, dir=1)    # cut = 0
              >>> cut
              0
              >>> cut_idx = cut
              >>> x[:cut_idx], x[cut_idx:]
              '', '1234'
              >>> x = '4321'
              >>> cut, period = factorize(x, dir=-1)
              >>> cut
              0
              >>> cut_idx = len(x) - cut
              >>> x[:cut_idx], x[cut_idx:]
              '4321', ''
    */
    Py_ssize_t max_suffix = 0;
    Py_ssize_t candidate = 1;
    Py_ssize_t k = 0;
    // The period of the right half.
    Py_ssize_t period = 1;
    // stt is starting position from chosen direction
    Py_ssize_t stt = dir == 1 ? 0 : m - 1;
    STRINGLIB_CHAR a, b;
    while (candidate + k < m) {
        // each loop increases (in chosen direction) candidate + k + max_suffix
        a = p[stt + dir*(candidate + k)];
        b = p[stt + dir*(max_suffix + k)];
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
STRINGLIB(_factorize)(const STRINGLIB_CHAR *p,
                      Py_ssize_t m,
                      Py_ssize_t *return_period,
                      int dir)
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
           dir : {-1, 1}
               if dir == -1, then the problem is reverse
           In short:
               _factorize(x, -1) == _factorize(x[::-1], 1)
           See docstring of _lex_search if still unclear
    */
    Py_ssize_t cut1, period1, cut2, period2, cut, period;
    cut1 = STRINGLIB(_lex_search)(p, m, &period1, 0, dir);
    cut2 = STRINGLIB(_lex_search)(p, m, &period2, 1, dir);
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


#define SHIFT_TYPE uint8_t
#define MAX_SHIFT UINT8_MAX

#define TABLE_SIZE_BITS 6u
#define TABLE_SIZE (1U << TABLE_SIZE_BITS)
#define TABLE_MASK (TABLE_SIZE - 1U)

typedef struct STRINGLIB(_pre) {
    const STRINGLIB_CHAR *p;        // needle
    Py_ssize_t m;                   // length of the needle
    Py_ssize_t cut;                 // Critical Factorization Cut
    Py_ssize_t period;              // Global Period of the string
    Py_ssize_t gap;                 // "Good Suffix" Last Character Gap
    int is_periodic;
    SHIFT_TYPE table[TABLE_SIZE];   // Boyer-Moore "Bad Character" table
} STRINGLIB(prework);


static void
STRINGLIB(_init_bc_table_gs_gap)(STRINGLIB(prework) *pw, int dir)
{
    // 1. Fill up a compressed Boyer-Moore "Bad Character" table
    const STRINGLIB_CHAR *p = pw->p;
    Py_ssize_t m = pw->m;
    Py_ssize_t stt = dir == 1 ? 0 : m - 1;
    Py_ssize_t end = dir == 1 ? m - 1 : 0;
    Py_ssize_t not_found_shift = Py_MIN(m, MAX_SHIFT);
    for (Py_ssize_t i = 0; i < (Py_ssize_t)TABLE_SIZE; i++) {
        pw->table[i] = Py_SAFE_DOWNCAST(not_found_shift,
                                        Py_ssize_t, SHIFT_TYPE);
    }
    for (Py_ssize_t i = m - not_found_shift; i < m; i++) {
        SHIFT_TYPE shift = Py_SAFE_DOWNCAST(m - 1 - i,
                                            Py_ssize_t, SHIFT_TYPE);
        pw->table[p[stt + dir*i] & TABLE_MASK] = shift;
    }
    // 2. Initialize "Good Suffix" Last Character Gap
    // Note: gap("___aa") = 1
    pw->gap = m;
    STRINGLIB_CHAR last = p[end] & TABLE_MASK;
    for (Py_ssize_t i = 1; i < m; i++) {
        STRINGLIB_CHAR x = p[end - dir*i] & TABLE_MASK;
        if (x == last) {
            pw->gap = i;
            break;
        }
    }
    LOG("Good Suffix Gap: %ld\n", pw->gap);
}


static void
STRINGLIB(_init_critical_fac)(STRINGLIB(prework) *pw, int dir)
{
    // Calculate Critical Factorization
    const STRINGLIB_CHAR *p = pw->p;
    Py_ssize_t m = pw->m;
    Py_ssize_t cut, period;
    int is_periodic;
    cut = STRINGLIB(_factorize)(p, m, &period, dir);
    assert(cut + period <= m);
    if (dir == 1) {
        is_periodic = memcmp(p, p + period, cut * STRINGLIB_SIZEOF_CHAR) == 0;
    }
    else {
        Py_ssize_t cut_idx = m - cut;
        is_periodic = (memcmp(p + cut_idx, p + cut_idx - period,
                              cut * STRINGLIB_SIZEOF_CHAR) == 0);
    }
    if (is_periodic) {
        assert(cut <= m/2);
        assert(cut < period);
        LOG("Needle is periodic.\n");
    }
    else {
        // A lower bound on the period
        // CLARIFY> An upper bound?
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


static Py_ssize_t
STRINGLIB(_two_way)(const STRINGLIB_CHAR *s, Py_ssize_t n,
                    Py_ssize_t maxcount, int mode,
                    STRINGLIB(prework) *pw, int dir)
{
    /* Crochemore and Perrin's (1991) Two-Way algorithm.
        See http://www-igm.univ-mlv.fr/~lecroq/string/node26.html#SECTION00260

        Main Inputs
           s - haystack
           p - needle
           n - len(haystack)
           m - len(needle)

        Bi-Directional Conventions:
           See docstring of horspool_find

        Critical factorization reversion:
           See docstring of _factorize
    */
    if (mode == FAST_COUNT) {
        LOG("Two-way Count.\n");
    }
    else {
        LOG("Two-way Find.\n");
    }
    LOG("haystack: "); LOG_STRING(s, n); LOG("\n");
    LOG("needle  : "); LOG_STRING(pw->p, pw->m); LOG("\n");
    dir = dir < 0 ? -1 : 1;     // This could help compiler a bit
    int reversed = dir < 0;

    // Prepare
    const STRINGLIB_CHAR *const p = pw->p;
    const Py_ssize_t m = pw->m;
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
        for (; j < m; j++) {
            ihits++;
            jp = p_stt + (reversed ? -j : j);
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
        }
        if (j != m) {
            continue;
        }

        j = Py_MIN(memory, cut);    // Needed for j == cut below to be correct
        for (; j < cut; j++) {
            ihits++;
            jp = p_stt + (reversed ? -j : j);
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
        }
        if (j == cut) {
            LOG("Found a match!\n");
            if (mode != FAST_COUNT) {
                return reversed ? n - m - i : i;
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
    return mode == FAST_COUNT ? count : -1;
}


static Py_ssize_t
STRINGLIB(two_way_find)(const STRINGLIB_CHAR *s, Py_ssize_t n,
                        const STRINGLIB_CHAR *p, Py_ssize_t m,
                        Py_ssize_t maxcount, int mode, int dir)
{
    STRINGLIB(prework) pw;
    (&pw)->p = p;
    (&pw)->m = m;
    STRINGLIB(_init_bc_table_gs_gap)(&pw, dir);
    STRINGLIB(_init_critical_fac)(&pw, dir);
    return STRINGLIB(_two_way)(s, n, maxcount, mode, &pw, dir);
}


static Py_ssize_t
STRINGLIB(horspool_find)(const STRINGLIB_CHAR* s, Py_ssize_t n,
                         const STRINGLIB_CHAR* p, Py_ssize_t m,
                         Py_ssize_t maxcount, int mode,
                         int dir, int dynamic)
{
    /* Boyer–Moore–Horspool algorithm
       with optional dynamic fallback to Two-Way algorithm

       Main Inputs
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

          >>> m = len(needle)
          >>> run_time = m * m + n_loops * (loop_cost + hit_cost * avg_hit)

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

             8. It works well, but is not perfect.
                See if you can come up with more accurate model.
    */
    if (mode == FAST_COUNT) {
        LOG("Horspool Count.\n");
    }
    else {
        LOG("Horspool Find\n");
    }
    LOG("haystack: "); LOG_STRING(s, n); LOG("\n");
    LOG("needle  : "); LOG_STRING(p, m); LOG("\n");
    dir = dir < 0 ? -1 : 1;     // This could help compiler a bit
    int reversed = dir < 0;

    // Prepare
    STRINGLIB(prework) pw;
    (&pw)->p = p;
    (&pw)->m = m;
    STRINGLIB(_init_bc_table_gs_gap)(&pw, dir);
    Py_ssize_t gap = (&pw)->gap;
    SHIFT_TYPE *table = (&pw)->table;

    // Direction Independent
    const Py_ssize_t m_m1 = m - 1;
    const Py_ssize_t m_p1 = m + 1;
    const Py_ssize_t w = n - m;
    // Direction Dependent
    const Py_ssize_t s_stt = dir == 1 ? 0 : n - 1;
    const Py_ssize_t p_stt = dir == 1 ? 0 : m - 1;
    const Py_ssize_t p_end = dir == 1 ? m - 1 : 0;
    const Py_ssize_t dir_m_m1 = reversed ? -m_m1 : m_m1;
    const STRINGLIB_CHAR *const ss = s + s_stt + dir_m_m1;
    const STRINGLIB_CHAR p_last = p[p_end];

    // Indexers (ip and jp are directional indices. e.g. ip = pos * i)
    Py_ssize_t i, j, ip, jp;

    // Use Bloom for len(haystack) >= 30 * len(needle)
    unsigned long mask = 0;
    Py_ssize_t true_gap = 0;
    Py_ssize_t j_stop = m;
    if (n >= 30 * m) {
        j_stop = m_m1;
        true_gap = m;
        // Note: true_gap("___aa") = 1
        STRINGLIB_CHAR p_tmp;
        STRINGLIB_BLOOM_ADD(mask, p_last);
        for (j = 1; j < m; j++) {
            jp = p_end + (reversed ? j : -j);
            p_tmp = p[jp];
            STRINGLIB_BLOOM_ADD(mask, p_tmp);
            if (true_gap == m && p_tmp == p_last) {
                true_gap = j;
            }
        }
        LOG("Good Suffix True Gap: %ld\n", true_gap);
    }

    // Total cost of two-way initialization
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
    Py_ssize_t iloop = 0, ihits_last = 0;
    Py_ssize_t ihits = 0, iloop_last = 0;
    // Loop
    for (i = 0; i <= w;) {
        iloop++;
        ip = reversed ? -i : i;
        s_last = ss[ip];
        LOG2("Last window ch: %c\n", s_last);
        if (true_gap) {
            shift = 0;
            if (s_last != p_last) {
                shift = (i < w && !STRINGLIB_BLOOM(mask, ss[ip+dir]))
                                ? m_p1
                                : Py_MAX(table[s_last & TABLE_MASK], 1);
            }
        }
        else {
            shift = table[s_last & TABLE_MASK];
        }
        if (shift) {
            LOG("Shift: %ld\n", shift);
            i += shift;
            continue;
        }

        // assert(s_last == p_last);                               // true_gap
        // assert((s_last & TABLE_MASK) == (p_last & TABLE_MASK)); // else
        j_off = ip - p_end;
        for (j = 0; j < j_stop; j++) {
            ihits++;
            jp = p_stt + (reversed ? -j : j);
            LOG2("Checking j=%ld: %c vs %c\n", j, ss[j_off + jp], p[jp]);
            if (ss[j_off + jp] != p[jp]) {
                break;
            }
        }
        if (j == j_stop) {
            LOG("Found a match!\n");
            if (mode != FAST_COUNT) {
                return reversed ? n - m - i : i;
            }
            if (++count == maxcount) {
                return maxcount;
            }
            i += m;
        }
        else if (true_gap) {
            if (i < w && !STRINGLIB_BLOOM(mask, ss[ip+dir])) {
                LOG("Move by (m + 1) = %ld\n", m_p1);
                i += m_p1;
            }
            else {
                LOG("Move by true gap = %ld\n", gap);
                i += true_gap;
            }
        }
        else {
            LOG("Move by table gap = %ld\n", gap);
            i += gap;
        }

        if (dynamic) {
            if (ihits - ihits_last < 100 && iloop - iloop_last < 100) {
                continue;
            }
            ll = (double)(w - i + 1);
            exp_hrs = ((double)iloop * hrs_lcost +
                       (double)ihits * hrs_hcost) / (double)i * ll;
            exp_twy = twy_icost + ll * twy_lcost;
            if (exp_twy < exp_hrs) {
                LOG("switching to two-way algorithm: n=%ld, m=%ld\n", n, m);
                STRINGLIB(_init_critical_fac)(&pw, dir);
                Py_ssize_t res = STRINGLIB(_two_way)(
                        reversed ? s : s + i, n - i,
                        maxcount - count, mode, &pw, dir);
                if (mode == FAST_SEARCH) {
                    return res == -1 ? -1 : (reversed ? res : res + i);
                }
                else {
                    return res + count;
                }
            }
            ihits_last = ihits;
            iloop_last = iloop;
        }
    }
    // Loop Counter and False Hit Counter Logging
    // In worst case scenario: ihits == (n - m) * m
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
                    s + i, n - i, p, m, maxcount, mode, 1);
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
    int dynamic = 1;    // dynamic fallback to two-way algorithm flag
    int dir = mode != FAST_RSEARCH ? 1 : -1;
    return STRINGLIB(horspool_find)(s, n, p, m, maxcount, mode, dir, dynamic);
}
