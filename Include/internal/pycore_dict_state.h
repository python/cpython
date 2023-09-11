#ifndef Py_INTERNAL_DICT_STATE_H
#define Py_INTERNAL_DICT_STATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


#ifndef WITH_FREELISTS
// without freelists
#  define PyDict_MAXFREELIST 0
#endif

#ifndef PyDict_MAXFREELIST
#  define PyDict_MAXFREELIST 80
#endif

#define DICT_MAX_WATCHERS 8

struct _Py_dict_state {
    /*Global counter used to set ma_version_tag field of dictionary.
     * It is incremented each time that a dictionary is created and each
     * time that a dictionary is modified. */
    uint64_t global_version;
    uint32_t next_keys_version;

#if PyDict_MAXFREELIST > 0
    /* Dictionary reuse scheme to save calls to malloc and free */
    PyDictObject *free_list[PyDict_MAXFREELIST];
    PyDictKeysObject *keys_free_list[PyDict_MAXFREELIST];
    int numfree;
    int keys_numfree;
#endif

    PyDict_WatchCallback watchers[DICT_MAX_WATCHERS];
};

#define _dict_state_INIT \
    { \
        .next_keys_version = 2, \
    }


#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_DICT_STATE_H */
