#ifndef Py_OPCODE_CACHE_H
#define Py_OPCODE_CACHE_H

#ifdef Py_DEBUG
// --with-pydebug is used to find memory leak.  opcache makes it harder.
// So we disable opcache when Py_DEBUG is defined.
// See bpo-37146
#define OPCACHE_MIN_RUNS 0  /* disable opcache */
#else
#define OPCACHE_MIN_RUNS 1024  /* create opcache when code executed this time */
#endif
#define OPCODE_CACHE_MAX_TRIES 20
#define OPCACHE_STATS 0  /* Enable stats */

#if OPCACHE_STATS
extern size_t _Py_opcache_code_objects;
extern size_t _Py_opcache_code_objects_extra_mem;

extern size_t _Py_opcache_global_opts;
extern size_t _Py_opcache_global_hits;
extern size_t _Py_opcache_global_misses;

extern size_t _Py_opcache_attr_opts;
extern size_t _Py_opcache_attr_hits;
extern size_t _Py_opcache_attr_misses;
extern size_t _Py_opcache_attr_deopts;
extern size_t _Py_opcache_attr_total;
#endif

#if OPCACHE_STATS

static inline void
OPCACHE_STAT_GLOBAL_HIT()
{
    if (co->co_opcache != NULL) _Py_opcache_global_hits++;
}

static inline void
OPCACHE_STAT_GLOBAL_MISS() {
    if (co->co_opcache != NULL) _Py_opcache_global_misses++;
}

static inline void
OPCACHE_STAT_GLOBAL_OPT() {
    if (co->co_opcache != NULL) _Py_opcache_global_opts++;
}

static inline void
OPCACHE_STAT_ATTR_HIT() {
    if (co->co_opcache != NULL) _Py_opcache_attr_hits++;
}

static inline void
OPCACHE_STAT_ATTR_MISS() {
    if (co->co_opcache != NULL) _Py_opcache_attr_misses++;
}

static inline void
OPCACHE_STAT_ATTR_OPT() {
    if (co->co_opcache!= NULL) _Py_opcache_attr_opts++;
}

static inline void
OPCACHE_STAT_ATTR_DEOPT() {
    if (co->co_opcache != NULL) _Py_opcache_attr_deopts++;
}

static inline void
OPCACHE_STAT_ATTR_TOTAL() {
    if (co->co_opcache != NULL) _Py_opcache_attr_total++;
}

#else /* OPCACHE_STATS */

#define OPCACHE_STAT_GLOBAL_HIT()
#define OPCACHE_STAT_GLOBAL_MISS()
#define OPCACHE_STAT_GLOBAL_OPT()

#define OPCACHE_STAT_ATTR_HIT()
#define OPCACHE_STAT_ATTR_MISS()
#define OPCACHE_STAT_ATTR_OPT()
#define OPCACHE_STAT_ATTR_DEOPT()
#define OPCACHE_STAT_ATTR_TOTAL()

#endif

/* macros for opcode cache */
static inline void
OPCACHE_CHECK(_PyOpcache **co_opcache, PyCodeObject *co,
              size_t current_instruction) {
    *co_opcache = NULL;
    if (co->co_opcache == NULL) {
        return;
    }
    unsigned char co_opcache_offset =
        co->co_opcache_map[current_instruction];
    if (co_opcache_offset > 0) {
        assert(co_opcache_offset <= co->co_opcache_size);
        *co_opcache = co->co_opcache + co_opcache_offset - 1;
        assert(*co_opcache != NULL);
        (*co_opcache)->current_instruction = current_instruction;
    }
}

static inline void
OPCACHE_DEOPT(_PyOpcache **co_opcache, PyCodeObject *co) {
    if (*co_opcache == NULL) {
        return;
    }
    (*co_opcache)->optimized = -1;
    unsigned char co_opcache_offset =
        co->co_opcache_map[(*co_opcache)->current_instruction];
    assert(co_opcache_offset <= co->co_opcache_size);
    co->co_opcache_map[co_opcache_offset] = 0;
    *co_opcache = NULL;
}

static inline void
OPCACHE_DEOPT_LOAD_ATTR(_PyOpcache **co_opcache, PyCodeObject *co) {
    if (co_opcache == NULL) {
        return;
    }
    OPCACHE_STAT_ATTR_DEOPT();
    OPCACHE_DEOPT(co_opcache, co);
}

static inline void
OPCACHE_MAYBE_DEOPT_LOAD_ATTR(_PyOpcache **co_opcache, PyCodeObject *co) {
    if (*co_opcache != NULL && --(*co_opcache)->optimized <= 0) {
        OPCACHE_DEOPT_LOAD_ATTR(co_opcache, co);
    }
}

void _PyOpcodeCache_PrintStats();

int _PyOpcodeCache_LoadGlobal(PyCodeObject *co, _PyOpcache **co_opcache_ptr,
                              PyFrameObject *f, PyObject* name, PyObject** result);
int _PyOpcodeCache_LoadAttr(PyCodeObject *co, _PyOpcache **co_opcache,
                            PyTypeObject* type, PyObject* name,
                            PyObject* owner, PyObject**result);

#endif
