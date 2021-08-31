#include <Python.h>
#include <internal/pycore_gc.h>
#include <internal/pycore_code.h>

static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[305];
    }
toplevel_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 304,
    },
    .ob_shash = -1,
    .ob_sval = "\x64\x00\x5a\x00\x64\x01\x64\x02\x6c\x01\x5a\x02\x64\x01\x64\x03\x6c\x01\x6d\x03\x5a\x03\x6d\x04\x5a\x04\x01\x00\x64\x01\x64\x02\x6c\x05\x5a\x06\x64\x01\x64\x02\x6c\x07\x5a\x07\x64\x01\x64\x02\x6c\x08\x5a\x08\x64\x01\x64\x02\x6c\x09\x5a\x09\x64\x01\x64\x02\x6c\x0a\x5a\x0a\x64\x01\x64\x02\x6c\x0b\x5a\x0b\x64\x01\x64\x02\x6c\x0c\x5a\x0c\x64\x04\x64\x05\x67\x02\x5a\x0d\x65\x02\x6a\x0e\x5a\x0e\x65\x02\x6a\x0f\x64\x06\x64\x02\x85\x02\x19\x00\x5a\x10\x47\x00\x64\x07\x84\x00\x64\x04\x65\x11\x83\x03\x5a\x12\x69\x00\x5a\x13\x65\x14\x65\x0a\x83\x01\x5a\x15\x64\x08\x5a\x16\x64\x09\x5a\x17\x64\x0a\x5a\x18\x47\x00\x64\x0b\x84\x00\x64\x05\x65\x02\x6a\x19\x83\x03\x5a\x1a\x65\x0e\x64\x0c\x17\x00\x64\x0d\x64\x0d\x66\x03\x65\x0e\x64\x0e\x17\x00\x64\x0f\x64\x0d\x66\x03\x64\x10\x64\x11\x66\x04\x5a\x1b\x64\x12\x84\x00\x5a\x1c\x64\x13\x84\x00\x5a\x1d\x64\x14\x84\x00\x5a\x1e\x64\x15\x84\x00\x5a\x1f\x64\x16\x5a\x20\x64\x0f\x61\x21\x64\x17\x84\x00\x5a\x22\x64\x18\x84\x00\x5a\x23\x64\x19\x84\x00\x5a\x24\x64\x1a\x84\x00\x5a\x25\x65\x14\x65\x25\x6a\x26\x83\x01\x5a\x27\x64\x1b\x84\x00\x5a\x28\x64\x1c\x84\x00\x5a\x29\x64\x1d\x84\x00\x5a\x2a\x64\x1e\x84\x00\x5a\x2b\x64\x1f\x84\x00\x5a\x2c\x64\x20\x84\x00\x5a\x2d\x64\x02\x53\x00",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[593];
    }
toplevel_consts_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 592,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "\x7a\x69\x70\x69\x6d\x70\x6f\x72\x74\x20\x70\x72\x6f\x76\x69\x64\x65\x73\x20\x73\x75\x70\x70\x6f\x72\x74\x20\x66\x6f\x72\x20\x69\x6d\x70\x6f\x72\x74\x69\x6e\x67\x20\x50\x79\x74\x68\x6f\x6e\x20\x6d\x6f\x64\x75\x6c\x65\x73\x20\x66\x72\x6f\x6d\x20\x5a\x69\x70\x20\x61\x72\x63\x68\x69\x76\x65\x73\x2e\x0a\x0a\x54\x68\x69\x73\x20\x6d\x6f\x64\x75\x6c\x65\x20\x65\x78\x70\x6f\x72\x74\x73\x20\x74\x68\x72\x65\x65\x20\x6f\x62\x6a\x65\x63\x74\x73\x3a\x0a\x2d\x20\x7a\x69\x70\x69\x6d\x70\x6f\x72\x74\x65\x72\x3a\x20\x61\x20\x63\x6c\x61\x73\x73\x3b\x20\x69\x74\x73\x20\x63\x6f\x6e\x73\x74\x72\x75\x63\x74\x6f\x72\x20\x74\x61\x6b\x65\x73\x20\x61\x20\x70\x61\x74\x68\x20\x74\x6f\x20\x61\x20\x5a\x69\x70\x20\x61\x72\x63\x68\x69\x76\x65\x2e\x0a\x2d\x20\x5a\x69\x70\x49\x6d\x70\x6f\x72\x74\x45\x72\x72\x6f\x72\x3a\x20\x65\x78\x63\x65\x70\x74\x69\x6f\x6e\x20\x72\x61\x69\x73\x65\x64\x20\x62\x79\x20\x7a\x69\x70\x69\x6d\x70\x6f\x72\x74\x65\x72\x20\x6f\x62\x6a\x65\x63\x74\x73\x2e\x20\x49\x74\x27\x73\x20\x61\x0a\x20\x20\x73\x75\x62\x63\x6c\x61\x73\x73\x20\x6f\x66\x20\x49\x6d\x70\x6f\x72\x74\x45\x72\x72\x6f\x72\x2c\x20\x73\x6f\x20\x69\x74\x20\x63\x61\x6e\x20\x62\x65\x20\x63\x61\x75\x67\x68\x74\x20\x61\x73\x20\x49\x6d\x70\x6f\x72\x74\x45\x72\x72\x6f\x72\x2c\x20\x74\x6f\x6f\x2e\x0a\x2d\x20\x5f\x7a\x69\x70\x5f\x64\x69\x72\x65\x63\x74\x6f\x72\x79\x5f\x63\x61\x63\x68\x65\x3a\x20\x61\x20\x64\x69\x63\x74\x2c\x20\x6d\x61\x70\x70\x69\x6e\x67\x20\x61\x72\x63\x68\x69\x76\x65\x20\x70\x61\x74\x68\x73\x20\x74\x6f\x20\x7a\x69\x70\x20\x64\x69\x72\x65\x63\x74\x6f\x72\x79\x0a\x20\x20\x69\x6e\x66\x6f\x20\x64\x69\x63\x74\x73\x2c\x20\x61\x73\x20\x75\x73\x65\x64\x20\x69\x6e\x20\x7a\x69\x70\x69\x6d\x70\x6f\x72\x74\x65\x72\x2e\x5f\x66\x69\x6c\x65\x73\x2e\x0a\x0a\x49\x74\x20\x69\x73\x20\x75\x73\x75\x61\x6c\x6c\x79\x20\x6e\x6f\x74\x20\x6e\x65\x65\x64\x65\x64\x20\x74\x6f\x20\x75\x73\x65\x20\x74\x68\x65\x20\x7a\x69\x70\x69\x6d\x70\x6f\x72\x74\x20\x6d\x6f\x64\x75\x6c\x65\x20\x65\x78\x70\x6c\x69\x63\x69\x74\x6c\x79\x3b\x20\x69\x74\x20\x69\x73\x0a\x75\x73\x65\x64\x20\x62\x79\x20\x74\x68\x65\x20\x62\x75\x69\x6c\x74\x69\x6e\x20\x69\x6d\x70\x6f\x72\x74\x20\x6d\x65\x63\x68\x61\x6e\x69\x73\x6d\x20\x66\x6f\x72\x20\x73\x79\x73\x2e\x70\x61\x74\x68\x20\x69\x74\x65\x6d\x73\x20\x74\x68\x61\x74\x20\x61\x72\x65\x20\x70\x61\x74\x68\x73\x0a\x74\x6f\x20\x5a\x69\x70\x20\x61\x72\x63\x68\x69\x76\x65\x73\x2e\x0a",
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_1 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 0,
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[15];
    }
toplevel_consts_3_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 14,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_unpack_uint16",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[15];
    }
toplevel_consts_3_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 14,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_unpack_uint32",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[2];
        }_object;
    }
toplevel_consts_3 = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 2,
        },
        .ob_item = {
            & toplevel_consts_3_0._ascii.ob_base,
            & toplevel_consts_3_1._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[15];
    }
toplevel_consts_4 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 14,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "ZipImportError",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[12];
    }
toplevel_consts_5 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 11,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "zipimporter",
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_6 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 1 },
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[13];
    }
toplevel_consts_7_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 12,
    },
    .ob_shash = -1,
    .ob_sval = "\x65\x00\x5a\x01\x64\x00\x5a\x02\x64\x01\x53\x00",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[2];
        }_object;
    }
toplevel_consts_7_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 2,
        },
        .ob_item = {
            & toplevel_consts_4._ascii.ob_base,
            Py_None,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[9];
    }
toplevel_consts_7_names_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 8,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "__name__",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[11];
    }
toplevel_consts_7_names_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 10,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "__module__",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[13];
    }
toplevel_consts_7_names_2 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 12,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "__qualname__",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[3];
        }_object;
    }
toplevel_consts_7_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 3,
        },
        .ob_item = {
            & toplevel_consts_7_names_0._ascii.ob_base,
            & toplevel_consts_7_names_1._ascii.ob_base,
            & toplevel_consts_7_names_2._ascii.ob_base,
        },
    },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
        }_object;
    }
toplevel_consts_7_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 0,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[19];
    }
toplevel_consts_7_filename = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 18,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "./Lib/zipimport.py",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[5];
    }
toplevel_consts_7_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 4,
    },
    .ob_shash = -1,
    .ob_sval = "\x08\x00\x04\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[5];
    }
toplevel_consts_7_endlinetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 4,
    },
    .ob_shash = -1,
    .ob_sval = "\x08\xde\x04\x23",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[13];
    }
toplevel_consts_7_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 12,
    },
    .ob_shash = -1,
    .ob_sval = "\x01\x01\x01\x01\x01\x01\x01\x01\x05\x09\x05\x09",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[1];
    }
toplevel_consts_7_exceptiontable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 0,
    },
    .ob_shash = -1,
    .ob_sval = "",
};
static struct PyCodeObject toplevel_consts_7 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_7_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_7_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_7_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_7_exceptiontable.ob_base.ob_base,
    .co_flags = 0,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 0,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 1,
    .co_firstlineno = 34,
    .co_code = & toplevel_consts_7_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_7_exceptiontable.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_4._ascii.ob_base,
    .co_qualname = & toplevel_consts_4._ascii.ob_base,
    .co_linetable = & toplevel_consts_7_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_7_endlinetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_7_columntable.ob_base.ob_base,
    .co_nlocalsplus = 0,
    .co_nlocals = 0,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_8 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 22 },
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[5];
    }
toplevel_consts_9 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 4,
    },
    .ob_shash = -1,
    .ob_sval = "\x50\x4b\x05\x06",
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_10 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 65535 },
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[101];
    }
toplevel_consts_11_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 100,
    },
    .ob_shash = -1,
    .ob_sval = "\x65\x00\x5a\x01\x64\x00\x5a\x02\x64\x01\x5a\x03\x64\x02\x84\x00\x5a\x04\x64\x10\x64\x04\x84\x01\x5a\x05\x64\x10\x64\x05\x84\x01\x5a\x06\x64\x10\x64\x06\x84\x01\x5a\x07\x64\x07\x84\x00\x5a\x08\x64\x08\x84\x00\x5a\x09\x64\x09\x84\x00\x5a\x0a\x64\x0a\x84\x00\x5a\x0b\x64\x0b\x84\x00\x5a\x0c\x64\x0c\x84\x00\x5a\x0d\x64\x0d\x84\x00\x5a\x0e\x64\x0e\x84\x00\x5a\x0f\x64\x0f\x84\x00\x5a\x10\x64\x03\x53\x00",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[512];
    }
toplevel_consts_11_consts_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 511,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "\x7a\x69\x70\x69\x6d\x70\x6f\x72\x74\x65\x72\x28\x61\x72\x63\x68\x69\x76\x65\x70\x61\x74\x68\x29\x20\x2d\x3e\x20\x7a\x69\x70\x69\x6d\x70\x6f\x72\x74\x65\x72\x20\x6f\x62\x6a\x65\x63\x74\x0a\x0a\x20\x20\x20\x20\x43\x72\x65\x61\x74\x65\x20\x61\x20\x6e\x65\x77\x20\x7a\x69\x70\x69\x6d\x70\x6f\x72\x74\x65\x72\x20\x69\x6e\x73\x74\x61\x6e\x63\x65\x2e\x20\x27\x61\x72\x63\x68\x69\x76\x65\x70\x61\x74\x68\x27\x20\x6d\x75\x73\x74\x20\x62\x65\x20\x61\x20\x70\x61\x74\x68\x20\x74\x6f\x0a\x20\x20\x20\x20\x61\x20\x7a\x69\x70\x66\x69\x6c\x65\x2c\x20\x6f\x72\x20\x74\x6f\x20\x61\x20\x73\x70\x65\x63\x69\x66\x69\x63\x20\x70\x61\x74\x68\x20\x69\x6e\x73\x69\x64\x65\x20\x61\x20\x7a\x69\x70\x66\x69\x6c\x65\x2e\x20\x46\x6f\x72\x20\x65\x78\x61\x6d\x70\x6c\x65\x2c\x20\x69\x74\x20\x63\x61\x6e\x20\x62\x65\x0a\x20\x20\x20\x20\x27\x2f\x74\x6d\x70\x2f\x6d\x79\x69\x6d\x70\x6f\x72\x74\x2e\x7a\x69\x70\x27\x2c\x20\x6f\x72\x20\x27\x2f\x74\x6d\x70\x2f\x6d\x79\x69\x6d\x70\x6f\x72\x74\x2e\x7a\x69\x70\x2f\x6d\x79\x64\x69\x72\x65\x63\x74\x6f\x72\x79\x27\x2c\x20\x69\x66\x20\x6d\x79\x64\x69\x72\x65\x63\x74\x6f\x72\x79\x20\x69\x73\x20\x61\x0a\x20\x20\x20\x20\x76\x61\x6c\x69\x64\x20\x64\x69\x72\x65\x63\x74\x6f\x72\x79\x20\x69\x6e\x73\x69\x64\x65\x20\x74\x68\x65\x20\x61\x72\x63\x68\x69\x76\x65\x2e\x0a\x0a\x20\x20\x20\x20\x27\x5a\x69\x70\x49\x6d\x70\x6f\x72\x74\x45\x72\x72\x6f\x72\x20\x69\x73\x20\x72\x61\x69\x73\x65\x64\x20\x69\x66\x20\x27\x61\x72\x63\x68\x69\x76\x65\x70\x61\x74\x68\x27\x20\x64\x6f\x65\x73\x6e\x27\x74\x20\x70\x6f\x69\x6e\x74\x20\x74\x6f\x20\x61\x20\x76\x61\x6c\x69\x64\x20\x5a\x69\x70\x0a\x20\x20\x20\x20\x61\x72\x63\x68\x69\x76\x65\x2e\x0a\x0a\x20\x20\x20\x20\x54\x68\x65\x20\x27\x61\x72\x63\x68\x69\x76\x65\x27\x20\x61\x74\x74\x72\x69\x62\x75\x74\x65\x20\x6f\x66\x20\x7a\x69\x70\x69\x6d\x70\x6f\x72\x74\x65\x72\x20\x6f\x62\x6a\x65\x63\x74\x73\x20\x63\x6f\x6e\x74\x61\x69\x6e\x73\x20\x74\x68\x65\x20\x6e\x61\x6d\x65\x20\x6f\x66\x20\x74\x68\x65\x0a\x20\x20\x20\x20\x7a\x69\x70\x66\x69\x6c\x65\x20\x74\x61\x72\x67\x65\x74\x65\x64\x2e\x0a\x20\x20\x20\x20",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[297];
    }
toplevel_consts_11_consts_2_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 296,
    },
    .ob_shash = -1,
    .ob_sval = "\x74\x00\x7c\x01\x74\x01\x83\x02\x73\x0e\x64\x01\x64\x00\x6c\x02\x7d\x02\x7c\x02\xa0\x03\x7c\x01\xa1\x01\x7d\x01\x7c\x01\x73\x16\x74\x04\x64\x02\x7c\x01\x64\x03\x8d\x02\x82\x01\x74\x05\x72\x1e\x7c\x01\xa0\x06\x74\x05\x74\x07\xa1\x02\x7d\x01\x67\x00\x7d\x03\x09\x00\x09\x00\x74\x08\x6a\x09\x7c\x01\x83\x01\x7d\x04\x6e\x25\x23\x00\x04\x00\x74\x0a\x74\x0b\x66\x02\x79\x4b\x01\x00\x01\x00\x01\x00\x74\x08\x6a\x0c\x7c\x01\x83\x01\x5c\x02\x7d\x05\x7d\x06\x7c\x05\x7c\x01\x6b\x02\x72\x42\x74\x04\x64\x05\x7c\x01\x64\x03\x8d\x02\x82\x01\x7c\x05\x7d\x01\x7c\x03\xa0\x0d\x7c\x06\xa1\x01\x01\x00\x59\x00\x6e\x10\x77\x00\x25\x00\x7c\x04\x6a\x0e\x64\x06\x40\x00\x64\x07\x6b\x03\x72\x5a\x74\x04\x64\x05\x7c\x01\x64\x03\x8d\x02\x82\x01\x71\x5c\x71\x21\x09\x00\x74\x0f\x7c\x01\x19\x00\x7d\x07\x6e\x13\x23\x00\x04\x00\x74\x10\x79\x73\x01\x00\x01\x00\x01\x00\x74\x11\x7c\x01\x83\x01\x7d\x07\x7c\x07\x74\x0f\x7c\x01\x3c\x00\x59\x00\x6e\x02\x77\x00\x25\x00\x7c\x07\x7c\x00\x5f\x12\x7c\x01\x7c\x00\x5f\x13\x74\x08\x6a\x14\x7c\x03\x64\x00\x64\x00\x64\x08\x85\x03\x19\x00\x8e\x00\x7c\x00\x5f\x15\x7c\x00\x6a\x15\x72\x92\x7c\x00\x04\x00\x6a\x15\x74\x07\x37\x00\x02\x00\x5f\x15\x64\x00\x53\x00\x64\x00\x53\x00",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[22];
    }
toplevel_consts_11_consts_2_consts_2 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 21,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "archive path is empty",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[5];
    }
toplevel_consts_11_consts_2_consts_3_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 4,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "path",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[1];
        }_object;
    }
toplevel_consts_11_consts_2_consts_3 = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 1,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_consts_3_0._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[15];
    }
toplevel_consts_11_consts_2_consts_5 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 14,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "not a Zip file",
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_11_consts_2_consts_6 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 61440 },
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_11_consts_2_consts_7 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 32768 },
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_11_consts_2_consts_8 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = -1,
    },
    .ob_digit = { 1 },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[9];
        }_object;
    }
toplevel_consts_11_consts_2_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 9,
        },
        .ob_item = {
            Py_None,
            & toplevel_consts_1.ob_base.ob_base,
            & toplevel_consts_11_consts_2_consts_2._ascii.ob_base,
            & toplevel_consts_11_consts_2_consts_3._object.ob_base.ob_base,
            Py_True,
            & toplevel_consts_11_consts_2_consts_5._ascii.ob_base,
            & toplevel_consts_11_consts_2_consts_6.ob_base.ob_base,
            & toplevel_consts_11_consts_2_consts_7.ob_base.ob_base,
            & toplevel_consts_11_consts_2_consts_8.ob_base.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[11];
    }
toplevel_consts_11_consts_2_names_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 10,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "isinstance",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[4];
    }
toplevel_consts_11_consts_2_names_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 3,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "str",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[3];
    }
toplevel_consts_11_consts_2_names_2 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 2,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "os",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[9];
    }
toplevel_consts_11_consts_2_names_3 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 8,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "fsdecode",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[13];
    }
toplevel_consts_11_consts_2_names_5 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 12,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "alt_path_sep",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[8];
    }
toplevel_consts_11_consts_2_names_6 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 7,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "replace",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[9];
    }
toplevel_consts_11_consts_2_names_7 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 8,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "path_sep",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[20];
    }
toplevel_consts_11_consts_2_names_8 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 19,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_bootstrap_external",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[11];
    }
toplevel_consts_11_consts_2_names_9 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 10,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_path_stat",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[8];
    }
toplevel_consts_11_consts_2_names_10 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 7,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "OSError",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[11];
    }
toplevel_consts_11_consts_2_names_11 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 10,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "ValueError",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[12];
    }
toplevel_consts_11_consts_2_names_12 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 11,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_path_split",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[7];
    }
toplevel_consts_11_consts_2_names_13 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 6,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "append",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[8];
    }
toplevel_consts_11_consts_2_names_14 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 7,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "st_mode",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[21];
    }
toplevel_consts_11_consts_2_names_15 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 20,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_zip_directory_cache",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[9];
    }
toplevel_consts_11_consts_2_names_16 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 8,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "KeyError",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[16];
    }
toplevel_consts_11_consts_2_names_17 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 15,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_read_directory",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[7];
    }
toplevel_consts_11_consts_2_names_18 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 6,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_files",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[8];
    }
toplevel_consts_11_consts_2_names_19 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 7,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "archive",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[11];
    }
toplevel_consts_11_consts_2_names_20 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 10,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_path_join",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[7];
    }
toplevel_consts_11_consts_2_names_21 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 6,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "prefix",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[22];
        }_object;
    }
toplevel_consts_11_consts_2_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 22,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_names_0._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_1._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_2._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_3._ascii.ob_base,
            & toplevel_consts_4._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_5._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_6._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_7._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_8._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_9._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_10._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_11._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_12._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_13._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_14._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_15._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_16._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_17._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_18._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_19._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_20._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_21._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[5];
    }
toplevel_consts_11_consts_2_varnames_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 4,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "self",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[3];
    }
toplevel_consts_11_consts_2_varnames_4 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 2,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "st",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[8];
    }
toplevel_consts_11_consts_2_varnames_5 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 7,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "dirname",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[9];
    }
toplevel_consts_11_consts_2_varnames_6 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 8,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "basename",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[6];
    }
toplevel_consts_11_consts_2_varnames_7 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 5,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "files",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[8];
        }_object;
    }
toplevel_consts_11_consts_2_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 8,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_varnames_0._ascii.ob_base,
            & toplevel_consts_11_consts_2_consts_3_0._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_2._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_21._ascii.ob_base,
            & toplevel_consts_11_consts_2_varnames_4._ascii.ob_base,
            & toplevel_consts_11_consts_2_varnames_5._ascii.ob_base,
            & toplevel_consts_11_consts_2_varnames_6._ascii.ob_base,
            & toplevel_consts_11_consts_2_varnames_7._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[9];
    }
toplevel_consts_11_consts_2_name = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 8,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "__init__",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[21];
    }
toplevel_consts_11_consts_2_qualname = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 20,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "zipimporter.__init__",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[77];
    }
toplevel_consts_11_consts_2_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 76,
    },
    .ob_shash = -1,
    .ob_sval = "\x0a\x01\x08\x01\x0a\x01\x04\x01\x0c\x01\x04\x01\x0c\x01\x04\x02\x02\x01\x02\x01\x0c\x01\x02\x80\x10\x01\x0e\x03\x08\x01\x0c\x01\x04\x01\x0e\x01\x02\xf9\x02\x80\x0e\x0a\x0c\x02\x02\x01\x02\xf0\x02\x12\x0a\x01\x02\x80\x0c\x01\x08\x01\x0c\x01\x02\xfe\x02\x80\x06\x03\x06\x01\x16\x02\x06\x01\x12\x01\x04\xff",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[81];
    }
toplevel_consts_11_consts_2_endlinetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 80,
    },
    .ob_shash = -1,
    .ob_sval = "\x08\x01\x02\x02\x08\xff\x0a\x01\x02\x01\x0e\x01\x02\x01\x0e\x01\x04\x02\x02\x01\x02\x10\x0c\xf2\x02\x80\x02\x08\x06\xf9\x08\x07\x0e\xfc\x06\x01\x0e\x01\x04\x01\x10\x01\x02\x80\x0c\x03\x0e\x02\x02\x01\x02\xf0\x02\x16\x0a\xfd\x02\x80\x02\x03\x02\xfe\x08\x02\x08\xff\x0e\x01\x02\x80\x06\x01\x06\x01\x16\x02\x04\x01\x18\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[297];
    }
toplevel_consts_11_consts_2_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 296,
    },
    .ob_shash = -1,
    .ob_sval = "\x10\x1a\x1b\x1f\x21\x24\x10\x25\x09\x25\x0d\x16\x0d\x16\x0d\x16\x0d\x16\x14\x16\x14\x25\x20\x24\x14\x25\x0d\x11\x10\x14\x09\x45\x13\x21\x22\x39\x40\x44\x13\x45\x13\x45\x0d\x45\x0c\x18\x09\x38\x14\x18\x14\x38\x21\x2d\x2f\x37\x14\x38\x0d\x11\x12\x14\x09\x0f\x0f\x13\x0d\x16\x16\x29\x16\x34\x35\x39\x16\x3a\x11\x13\x11\x13\x00\x00\x0d\x28\x15\x1c\x1e\x28\x14\x29\x0d\x28\x0d\x28\x0d\x28\x0d\x28\x25\x38\x25\x44\x45\x49\x25\x4a\x11\x22\x11\x18\x1a\x22\x14\x1b\x1f\x23\x14\x23\x11\x46\x1b\x29\x2a\x3a\x41\x45\x1b\x46\x1b\x46\x15\x46\x18\x1f\x11\x15\x11\x17\x11\x28\x1f\x27\x11\x28\x11\x28\x11\x28\x11\x28\x0d\x28\x00\x00\x15\x17\x15\x1f\x22\x2a\x15\x2a\x2f\x37\x14\x37\x11\x46\x1b\x29\x2a\x3a\x41\x45\x1b\x46\x1b\x46\x15\x46\x11\x16\x0f\x13\x09\x2f\x15\x29\x2a\x2e\x15\x2f\x0d\x12\x0d\x12\x00\x00\x09\x2f\x10\x18\x09\x2f\x09\x2f\x09\x2f\x09\x2f\x15\x24\x25\x29\x15\x2a\x0d\x12\x2a\x2f\x0d\x21\x22\x26\x0d\x27\x0d\x27\x0d\x27\x09\x2f\x00\x00\x17\x1c\x09\x0d\x09\x14\x18\x1c\x09\x0d\x09\x15\x17\x2a\x17\x35\x37\x3d\x3e\x42\x3e\x42\x40\x42\x3e\x42\x37\x43\x17\x44\x09\x0d\x09\x14\x0c\x10\x0c\x17\x09\x24\x0d\x11\x0d\x18\x0d\x18\x1c\x24\x0d\x24\x0d\x18\x0d\x18\x0d\x18\x0d\x18\x09\x24\x09\x24",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[34];
    }
toplevel_consts_11_consts_2_exceptiontable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 33,
    },
    .ob_shash = -1,
    .ob_sval = "\xa2\x05\x28\x00\xa8\x21\x41\x0c\x07\xc1\x0b\x01\x41\x0c\x07\xc1\x1d\x04\x41\x22\x00\xc1\x22\x0f\x41\x34\x07\xc1\x33\x01\x41\x34\x07",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[9];
    }
toplevel_consts_11_consts_2_localspluskinds = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 8,
    },
    .ob_shash = -1,
    .ob_sval = "        ",
};
static struct PyCodeObject toplevel_consts_11_consts_2 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_11_consts_2_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_11_consts_2_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_11_consts_2_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_11_consts_2_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 2,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 9,
    .co_firstlineno = 64,
    .co_code = & toplevel_consts_11_consts_2_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_11_consts_2_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_11_consts_2_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_11_consts_2_name._ascii.ob_base,
    .co_qualname = & toplevel_consts_11_consts_2_qualname._ascii.ob_base,
    .co_linetable = & toplevel_consts_11_consts_2_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_11_consts_2_endlinetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_11_consts_2_columntable.ob_base.ob_base,
    .co_nlocalsplus = 8,
    .co_nlocals = 8,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_11_consts_2_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[91];
    }
toplevel_consts_11_consts_4_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 90,
    },
    .ob_shash = -1,
    .ob_sval = "\x74\x00\x6a\x01\x64\x01\x74\x02\x83\x02\x01\x00\x74\x03\x7c\x00\x7c\x01\x83\x02\x7d\x03\x7c\x03\x64\x02\x75\x01\x72\x13\x7c\x00\x67\x00\x66\x02\x53\x00\x74\x04\x7c\x00\x7c\x01\x83\x02\x7d\x04\x74\x05\x7c\x00\x7c\x04\x83\x02\x72\x29\x64\x02\x7c\x00\x6a\x06\x9b\x00\x74\x07\x9b\x00\x7c\x04\x9b\x00\x9d\x03\x67\x01\x66\x02\x53\x00\x64\x02\x67\x00\x66\x02\x53\x00",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[560];
    }
toplevel_consts_11_consts_4_consts_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 559,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "\x66\x69\x6e\x64\x5f\x6c\x6f\x61\x64\x65\x72\x28\x66\x75\x6c\x6c\x6e\x61\x6d\x65\x2c\x20\x70\x61\x74\x68\x3d\x4e\x6f\x6e\x65\x29\x20\x2d\x3e\x20\x73\x65\x6c\x66\x2c\x20\x73\x74\x72\x20\x6f\x72\x20\x4e\x6f\x6e\x65\x2e\x0a\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x53\x65\x61\x72\x63\x68\x20\x66\x6f\x72\x20\x61\x20\x6d\x6f\x64\x75\x6c\x65\x20\x73\x70\x65\x63\x69\x66\x69\x65\x64\x20\x62\x79\x20\x27\x66\x75\x6c\x6c\x6e\x61\x6d\x65\x27\x2e\x20\x27\x66\x75\x6c\x6c\x6e\x61\x6d\x65\x27\x20\x6d\x75\x73\x74\x20\x62\x65\x20\x74\x68\x65\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x66\x75\x6c\x6c\x79\x20\x71\x75\x61\x6c\x69\x66\x69\x65\x64\x20\x28\x64\x6f\x74\x74\x65\x64\x29\x20\x6d\x6f\x64\x75\x6c\x65\x20\x6e\x61\x6d\x65\x2e\x20\x49\x74\x20\x72\x65\x74\x75\x72\x6e\x73\x20\x74\x68\x65\x20\x7a\x69\x70\x69\x6d\x70\x6f\x72\x74\x65\x72\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x69\x6e\x73\x74\x61\x6e\x63\x65\x20\x69\x74\x73\x65\x6c\x66\x20\x69\x66\x20\x74\x68\x65\x20\x6d\x6f\x64\x75\x6c\x65\x20\x77\x61\x73\x20\x66\x6f\x75\x6e\x64\x2c\x20\x61\x20\x73\x74\x72\x69\x6e\x67\x20\x63\x6f\x6e\x74\x61\x69\x6e\x69\x6e\x67\x20\x74\x68\x65\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x66\x75\x6c\x6c\x20\x70\x61\x74\x68\x20\x6e\x61\x6d\x65\x20\x69\x66\x20\x69\x74\x27\x73\x20\x70\x6f\x73\x73\x69\x62\x6c\x79\x20\x61\x20\x70\x6f\x72\x74\x69\x6f\x6e\x20\x6f\x66\x20\x61\x20\x6e\x61\x6d\x65\x73\x70\x61\x63\x65\x20\x70\x61\x63\x6b\x61\x67\x65\x2c\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x6f\x72\x20\x4e\x6f\x6e\x65\x20\x6f\x74\x68\x65\x72\x77\x69\x73\x65\x2e\x20\x54\x68\x65\x20\x6f\x70\x74\x69\x6f\x6e\x61\x6c\x20\x27\x70\x61\x74\x68\x27\x20\x61\x72\x67\x75\x6d\x65\x6e\x74\x20\x69\x73\x20\x69\x67\x6e\x6f\x72\x65\x64\x20\x2d\x2d\x20\x69\x74\x27\x73\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x74\x68\x65\x72\x65\x20\x66\x6f\x72\x20\x63\x6f\x6d\x70\x61\x74\x69\x62\x69\x6c\x69\x74\x79\x20\x77\x69\x74\x68\x20\x74\x68\x65\x20\x69\x6d\x70\x6f\x72\x74\x65\x72\x20\x70\x72\x6f\x74\x6f\x63\x6f\x6c\x2e\x0a\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x44\x65\x70\x72\x65\x63\x61\x74\x65\x64\x20\x73\x69\x6e\x63\x65\x20\x50\x79\x74\x68\x6f\x6e\x20\x33\x2e\x31\x30\x2e\x20\x55\x73\x65\x20\x66\x69\x6e\x64\x5f\x73\x70\x65\x63\x28\x29\x20\x69\x6e\x73\x74\x65\x61\x64\x2e\x0a\x20\x20\x20\x20\x20\x20\x20\x20",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[103];
    }
toplevel_consts_11_consts_4_consts_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 102,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "zipimporter.find_loader() is deprecated and slated for removal in Python 3.12; use find_spec() instead",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[3];
        }_object;
    }
toplevel_consts_11_consts_4_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 3,
        },
        .ob_item = {
            & toplevel_consts_11_consts_4_consts_0._ascii.ob_base,
            & toplevel_consts_11_consts_4_consts_1._ascii.ob_base,
            Py_None,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[10];
    }
toplevel_consts_11_consts_4_names_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 9,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_warnings",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[5];
    }
toplevel_consts_11_consts_4_names_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 4,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "warn",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[19];
    }
toplevel_consts_11_consts_4_names_2 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 18,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "DeprecationWarning",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[17];
    }
toplevel_consts_11_consts_4_names_3 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 16,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_get_module_info",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[17];
    }
toplevel_consts_11_consts_4_names_4 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 16,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_get_module_path",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[8];
    }
toplevel_consts_11_consts_4_names_5 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 7,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_is_dir",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[8];
        }_object;
    }
toplevel_consts_11_consts_4_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 8,
        },
        .ob_item = {
            & toplevel_consts_11_consts_4_names_0._ascii.ob_base,
            & toplevel_consts_11_consts_4_names_1._ascii.ob_base,
            & toplevel_consts_11_consts_4_names_2._ascii.ob_base,
            & toplevel_consts_11_consts_4_names_3._ascii.ob_base,
            & toplevel_consts_11_consts_4_names_4._ascii.ob_base,
            & toplevel_consts_11_consts_4_names_5._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_19._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_7._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[9];
    }
toplevel_consts_11_consts_4_varnames_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 8,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "fullname",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[3];
    }
toplevel_consts_11_consts_4_varnames_3 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 2,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "mi",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[8];
    }
toplevel_consts_11_consts_4_varnames_4 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 7,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "modpath",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[5];
        }_object;
    }
toplevel_consts_11_consts_4_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 5,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_varnames_0._ascii.ob_base,
            & toplevel_consts_11_consts_4_varnames_1._ascii.ob_base,
            & toplevel_consts_11_consts_2_consts_3_0._ascii.ob_base,
            & toplevel_consts_11_consts_4_varnames_3._ascii.ob_base,
            & toplevel_consts_11_consts_4_varnames_4._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[12];
    }
toplevel_consts_11_consts_4_name = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 11,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "find_loader",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[24];
    }
toplevel_consts_11_consts_4_qualname = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 23,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "zipimporter.find_loader",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[21];
    }
toplevel_consts_11_consts_4_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 20,
    },
    .ob_shash = -1,
    .ob_sval = "\x06\x0c\x02\x02\x04\xfe\x0a\x03\x08\x01\x08\x02\x0a\x07\x0a\x01\x18\x04\x08\x02",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[21];
    }
toplevel_consts_11_consts_4_endlinetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 20,
    },
    .ob_shash = -1,
    .ob_sval = "\x04\x0c\x02\x01\x06\x01\x0a\x01\x06\x01\x0a\x02\x0a\x07\x08\x01\x1a\x04\x08\x02",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[91];
    }
toplevel_consts_11_consts_4_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 90,
    },
    .ob_shash = -1,
    .ob_sval = "\x09\x12\x09\x17\x18\x49\x18\x2a\x09\x2b\x09\x2b\x0e\x1e\x1f\x23\x25\x2d\x0e\x2e\x09\x0b\x0c\x0e\x16\x1a\x0c\x1a\x09\x1c\x14\x18\x1a\x1c\x14\x1c\x0d\x1c\x13\x23\x24\x28\x2a\x32\x13\x33\x09\x10\x0c\x13\x14\x18\x1a\x21\x0c\x22\x09\x40\x14\x18\x1e\x22\x1e\x2a\x1b\x3f\x2c\x34\x1b\x3f\x36\x3d\x1b\x3f\x1b\x3f\x1a\x40\x14\x40\x0d\x40\x10\x14\x16\x18\x10\x18\x09\x18",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[6];
    }
toplevel_consts_11_consts_4_localspluskinds = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 5,
    },
    .ob_shash = -1,
    .ob_sval = "     ",
};
static struct PyCodeObject toplevel_consts_11_consts_4 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_11_consts_4_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_11_consts_4_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_11_consts_4_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_7_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 3,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 4,
    .co_firstlineno = 110,
    .co_code = & toplevel_consts_11_consts_4_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_11_consts_4_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_11_consts_4_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_11_consts_4_name._ascii.ob_base,
    .co_qualname = & toplevel_consts_11_consts_4_qualname._ascii.ob_base,
    .co_linetable = & toplevel_consts_11_consts_4_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_11_consts_4_endlinetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_11_consts_4_columntable.ob_base.ob_base,
    .co_nlocalsplus = 5,
    .co_nlocals = 5,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_11_consts_4_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[29];
    }
toplevel_consts_11_consts_5_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 28,
    },
    .ob_shash = -1,
    .ob_sval = "\x74\x00\x6a\x01\x64\x01\x74\x02\x83\x02\x01\x00\x7c\x00\xa0\x03\x7c\x01\x7c\x02\xa1\x02\x64\x02\x19\x00\x53\x00",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[460];
    }
toplevel_consts_11_consts_5_consts_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 459,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "\x66\x69\x6e\x64\x5f\x6d\x6f\x64\x75\x6c\x65\x28\x66\x75\x6c\x6c\x6e\x61\x6d\x65\x2c\x20\x70\x61\x74\x68\x3d\x4e\x6f\x6e\x65\x29\x20\x2d\x3e\x20\x73\x65\x6c\x66\x20\x6f\x72\x20\x4e\x6f\x6e\x65\x2e\x0a\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x53\x65\x61\x72\x63\x68\x20\x66\x6f\x72\x20\x61\x20\x6d\x6f\x64\x75\x6c\x65\x20\x73\x70\x65\x63\x69\x66\x69\x65\x64\x20\x62\x79\x20\x27\x66\x75\x6c\x6c\x6e\x61\x6d\x65\x27\x2e\x20\x27\x66\x75\x6c\x6c\x6e\x61\x6d\x65\x27\x20\x6d\x75\x73\x74\x20\x62\x65\x20\x74\x68\x65\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x66\x75\x6c\x6c\x79\x20\x71\x75\x61\x6c\x69\x66\x69\x65\x64\x20\x28\x64\x6f\x74\x74\x65\x64\x29\x20\x6d\x6f\x64\x75\x6c\x65\x20\x6e\x61\x6d\x65\x2e\x20\x49\x74\x20\x72\x65\x74\x75\x72\x6e\x73\x20\x74\x68\x65\x20\x7a\x69\x70\x69\x6d\x70\x6f\x72\x74\x65\x72\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x69\x6e\x73\x74\x61\x6e\x63\x65\x20\x69\x74\x73\x65\x6c\x66\x20\x69\x66\x20\x74\x68\x65\x20\x6d\x6f\x64\x75\x6c\x65\x20\x77\x61\x73\x20\x66\x6f\x75\x6e\x64\x2c\x20\x6f\x72\x20\x4e\x6f\x6e\x65\x20\x69\x66\x20\x69\x74\x20\x77\x61\x73\x6e\x27\x74\x2e\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x54\x68\x65\x20\x6f\x70\x74\x69\x6f\x6e\x61\x6c\x20\x27\x70\x61\x74\x68\x27\x20\x61\x72\x67\x75\x6d\x65\x6e\x74\x20\x69\x73\x20\x69\x67\x6e\x6f\x72\x65\x64\x20\x2d\x2d\x20\x69\x74\x27\x73\x20\x74\x68\x65\x72\x65\x20\x66\x6f\x72\x20\x63\x6f\x6d\x70\x61\x74\x69\x62\x69\x6c\x69\x74\x79\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x77\x69\x74\x68\x20\x74\x68\x65\x20\x69\x6d\x70\x6f\x72\x74\x65\x72\x20\x70\x72\x6f\x74\x6f\x63\x6f\x6c\x2e\x0a\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x44\x65\x70\x72\x65\x63\x61\x74\x65\x64\x20\x73\x69\x6e\x63\x65\x20\x50\x79\x74\x68\x6f\x6e\x20\x33\x2e\x31\x30\x2e\x20\x55\x73\x65\x20\x66\x69\x6e\x64\x5f\x73\x70\x65\x63\x28\x29\x20\x69\x6e\x73\x74\x65\x61\x64\x2e\x0a\x20\x20\x20\x20\x20\x20\x20\x20",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[103];
    }
toplevel_consts_11_consts_5_consts_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 102,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "zipimporter.find_module() is deprecated and slated for removal in Python 3.12; use find_spec() instead",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[4];
        }_object;
    }
toplevel_consts_11_consts_5_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 4,
        },
        .ob_item = {
            & toplevel_consts_11_consts_5_consts_0._ascii.ob_base,
            & toplevel_consts_11_consts_5_consts_1._ascii.ob_base,
            & toplevel_consts_1.ob_base.ob_base,
            Py_None,
        },
    },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[4];
        }_object;
    }
toplevel_consts_11_consts_5_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 4,
        },
        .ob_item = {
            & toplevel_consts_11_consts_4_names_0._ascii.ob_base,
            & toplevel_consts_11_consts_4_names_1._ascii.ob_base,
            & toplevel_consts_11_consts_4_names_2._ascii.ob_base,
            & toplevel_consts_11_consts_4_name._ascii.ob_base,
        },
    },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[3];
        }_object;
    }
toplevel_consts_11_consts_5_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 3,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_varnames_0._ascii.ob_base,
            & toplevel_consts_11_consts_4_varnames_1._ascii.ob_base,
            & toplevel_consts_11_consts_2_consts_3_0._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[12];
    }
toplevel_consts_11_consts_5_name = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 11,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "find_module",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[24];
    }
toplevel_consts_11_consts_5_qualname = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 23,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "zipimporter.find_module",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[9];
    }
toplevel_consts_11_consts_5_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 8,
    },
    .ob_shash = -1,
    .ob_sval = "\x06\x0b\x02\x02\x04\xfe\x10\x03",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[9];
    }
toplevel_consts_11_consts_5_endlinetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 8,
    },
    .ob_shash = -1,
    .ob_sval = "\x04\x0b\x02\x01\x06\x01\x10\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[29];
    }
toplevel_consts_11_consts_5_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 28,
    },
    .ob_shash = -1,
    .ob_sval = "\x09\x12\x09\x17\x18\x49\x18\x2a\x09\x2b\x09\x2b\x10\x14\x10\x30\x21\x29\x2b\x2f\x10\x30\x31\x32\x10\x33\x09\x33",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[4];
    }
toplevel_consts_11_consts_5_localspluskinds = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 3,
    },
    .ob_shash = -1,
    .ob_sval = "   ",
};
static struct PyCodeObject toplevel_consts_11_consts_5 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_11_consts_5_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_11_consts_5_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_11_consts_5_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_7_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 3,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 4,
    .co_firstlineno = 147,
    .co_code = & toplevel_consts_11_consts_5_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_11_consts_5_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_11_consts_5_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_11_consts_5_name._ascii.ob_base,
    .co_qualname = & toplevel_consts_11_consts_5_qualname._ascii.ob_base,
    .co_linetable = & toplevel_consts_11_consts_5_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_11_consts_5_endlinetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_11_consts_5_columntable.ob_base.ob_base,
    .co_nlocalsplus = 3,
    .co_nlocals = 3,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_11_consts_5_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[109];
    }
toplevel_consts_11_consts_6_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 108,
    },
    .ob_shash = -1,
    .ob_sval = "\x74\x00\x7c\x00\x7c\x01\x83\x02\x7d\x03\x7c\x03\x64\x01\x75\x01\x72\x11\x74\x01\x6a\x02\x7c\x01\x7c\x00\x7c\x03\x64\x02\x8d\x03\x53\x00\x74\x03\x7c\x00\x7c\x01\x83\x02\x7d\x04\x74\x04\x7c\x00\x7c\x04\x83\x02\x72\x34\x7c\x00\x6a\x05\x9b\x00\x74\x06\x9b\x00\x7c\x04\x9b\x00\x9d\x03\x7d\x05\x74\x01\x6a\x07\x7c\x01\x64\x01\x64\x03\x64\x04\x8d\x03\x7d\x06\x7c\x06\x6a\x08\xa0\x09\x7c\x05\xa1\x01\x01\x00\x7c\x06\x53\x00\x64\x01\x53\x00",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[108];
    }
toplevel_consts_11_consts_6_consts_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 107,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "\x43\x72\x65\x61\x74\x65\x20\x61\x20\x4d\x6f\x64\x75\x6c\x65\x53\x70\x65\x63\x20\x66\x6f\x72\x20\x74\x68\x65\x20\x73\x70\x65\x63\x69\x66\x69\x65\x64\x20\x6d\x6f\x64\x75\x6c\x65\x2e\x0a\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x52\x65\x74\x75\x72\x6e\x73\x20\x4e\x6f\x6e\x65\x20\x69\x66\x20\x74\x68\x65\x20\x6d\x6f\x64\x75\x6c\x65\x20\x63\x61\x6e\x6e\x6f\x74\x20\x62\x65\x20\x66\x6f\x75\x6e\x64\x2e\x0a\x20\x20\x20\x20\x20\x20\x20\x20",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[11];
    }
toplevel_consts_11_consts_6_consts_2_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 10,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "is_package",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[1];
        }_object;
    }
toplevel_consts_11_consts_6_consts_2 = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 1,
        },
        .ob_item = {
            & toplevel_consts_11_consts_6_consts_2_0._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[5];
    }
toplevel_consts_11_consts_6_consts_4_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 4,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "name",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[7];
    }
toplevel_consts_11_consts_6_consts_4_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 6,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "loader",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[3];
        }_object;
    }
toplevel_consts_11_consts_6_consts_4 = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 3,
        },
        .ob_item = {
            & toplevel_consts_11_consts_6_consts_4_0._ascii.ob_base,
            & toplevel_consts_11_consts_6_consts_4_1._ascii.ob_base,
            & toplevel_consts_11_consts_6_consts_2_0._ascii.ob_base,
        },
    },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[5];
        }_object;
    }
toplevel_consts_11_consts_6_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 5,
        },
        .ob_item = {
            & toplevel_consts_11_consts_6_consts_0._ascii.ob_base,
            Py_None,
            & toplevel_consts_11_consts_6_consts_2._object.ob_base.ob_base,
            Py_True,
            & toplevel_consts_11_consts_6_consts_4._object.ob_base.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[11];
    }
toplevel_consts_11_consts_6_names_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 10,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_bootstrap",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[17];
    }
toplevel_consts_11_consts_6_names_2 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 16,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "spec_from_loader",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[11];
    }
toplevel_consts_11_consts_6_names_7 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 10,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "ModuleSpec",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[27];
    }
toplevel_consts_11_consts_6_names_8 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 26,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "submodule_search_locations",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[10];
        }_object;
    }
toplevel_consts_11_consts_6_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 10,
        },
        .ob_item = {
            & toplevel_consts_11_consts_4_names_3._ascii.ob_base,
            & toplevel_consts_11_consts_6_names_1._ascii.ob_base,
            & toplevel_consts_11_consts_6_names_2._ascii.ob_base,
            & toplevel_consts_11_consts_4_names_4._ascii.ob_base,
            & toplevel_consts_11_consts_4_names_5._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_19._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_7._ascii.ob_base,
            & toplevel_consts_11_consts_6_names_7._ascii.ob_base,
            & toplevel_consts_11_consts_6_names_8._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_13._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[7];
    }
toplevel_consts_11_consts_6_varnames_2 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 6,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "target",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[12];
    }
toplevel_consts_11_consts_6_varnames_3 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 11,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "module_info",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[5];
    }
toplevel_consts_11_consts_6_varnames_6 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 4,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "spec",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[7];
        }_object;
    }
toplevel_consts_11_consts_6_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 7,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_varnames_0._ascii.ob_base,
            & toplevel_consts_11_consts_4_varnames_1._ascii.ob_base,
            & toplevel_consts_11_consts_6_varnames_2._ascii.ob_base,
            & toplevel_consts_11_consts_6_varnames_3._ascii.ob_base,
            & toplevel_consts_11_consts_4_varnames_4._ascii.ob_base,
            & toplevel_consts_11_consts_2_consts_3_0._ascii.ob_base,
            & toplevel_consts_11_consts_6_varnames_6._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[10];
    }
toplevel_consts_11_consts_6_name = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 9,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "find_spec",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[22];
    }
toplevel_consts_11_consts_6_qualname = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 21,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "zipimporter.find_spec",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[25];
    }
toplevel_consts_11_consts_6_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 24,
    },
    .ob_shash = -1,
    .ob_sval = "\x0a\x05\x08\x01\x10\x01\x0a\x07\x0a\x01\x12\x04\x08\x01\x02\x01\x06\xff\x0c\x02\x04\x01\x04\x02",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[29];
    }
toplevel_consts_11_consts_6_endlinetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 28,
    },
    .ob_shash = -1,
    .ob_sval = "\x0a\x05\x06\x01\x02\x13\x10\xee\x0a\x07\x08\x01\x02\x0a\x12\xfa\x08\x01\x06\x01\x02\xff\x0c\x02\x04\x01\x04\x02",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[109];
    }
toplevel_consts_11_consts_6_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 108,
    },
    .ob_shash = -1,
    .ob_sval = "\x17\x27\x28\x2c\x2e\x36\x17\x37\x09\x14\x0c\x17\x1f\x23\x0c\x23\x09\x1c\x14\x1e\x14\x2f\x30\x38\x3a\x3e\x4b\x56\x14\x57\x14\x57\x0d\x57\x17\x27\x28\x2c\x2e\x36\x17\x37\x0d\x14\x10\x17\x18\x1c\x1e\x25\x10\x26\x0d\x1c\x1b\x1f\x1b\x27\x18\x3c\x29\x31\x18\x3c\x33\x3a\x18\x3c\x18\x3c\x11\x15\x18\x22\x18\x2d\x33\x3b\x44\x48\x39\x3d\x18\x3e\x18\x3e\x11\x15\x11\x15\x11\x30\x11\x3d\x38\x3c\x11\x3d\x11\x3d\x18\x1c\x11\x1c\x18\x1c\x18\x1c",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[8];
    }
toplevel_consts_11_consts_6_localspluskinds = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 7,
    },
    .ob_shash = -1,
    .ob_sval = "       ",
};
static struct PyCodeObject toplevel_consts_11_consts_6 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_11_consts_6_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_11_consts_6_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_11_consts_6_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_7_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 3,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 5,
    .co_firstlineno = 163,
    .co_code = & toplevel_consts_11_consts_6_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_11_consts_6_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_11_consts_6_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_11_consts_6_name._ascii.ob_base,
    .co_qualname = & toplevel_consts_11_consts_6_qualname._ascii.ob_base,
    .co_linetable = & toplevel_consts_11_consts_6_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_11_consts_6_endlinetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_11_consts_6_columntable.ob_base.ob_base,
    .co_nlocalsplus = 7,
    .co_nlocals = 7,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_11_consts_6_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[21];
    }
toplevel_consts_11_consts_7_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 20,
    },
    .ob_shash = -1,
    .ob_sval = "\x74\x00\x7c\x00\x7c\x01\x83\x02\x5c\x03\x7d\x02\x7d\x03\x7d\x04\x7c\x02\x53\x00",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[167];
    }
toplevel_consts_11_consts_7_consts_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 166,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "\x67\x65\x74\x5f\x63\x6f\x64\x65\x28\x66\x75\x6c\x6c\x6e\x61\x6d\x65\x29\x20\x2d\x3e\x20\x63\x6f\x64\x65\x20\x6f\x62\x6a\x65\x63\x74\x2e\x0a\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x52\x65\x74\x75\x72\x6e\x20\x74\x68\x65\x20\x63\x6f\x64\x65\x20\x6f\x62\x6a\x65\x63\x74\x20\x66\x6f\x72\x20\x74\x68\x65\x20\x73\x70\x65\x63\x69\x66\x69\x65\x64\x20\x6d\x6f\x64\x75\x6c\x65\x2e\x20\x52\x61\x69\x73\x65\x20\x5a\x69\x70\x49\x6d\x70\x6f\x72\x74\x45\x72\x72\x6f\x72\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x69\x66\x20\x74\x68\x65\x20\x6d\x6f\x64\x75\x6c\x65\x20\x63\x6f\x75\x6c\x64\x6e\x27\x74\x20\x62\x65\x20\x69\x6d\x70\x6f\x72\x74\x65\x64\x2e\x0a\x20\x20\x20\x20\x20\x20\x20\x20",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[2];
        }_object;
    }
toplevel_consts_11_consts_7_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 2,
        },
        .ob_item = {
            & toplevel_consts_11_consts_7_consts_0._ascii.ob_base,
            Py_None,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[17];
    }
toplevel_consts_11_consts_7_names_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 16,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_get_module_code",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[1];
        }_object;
    }
toplevel_consts_11_consts_7_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 1,
        },
        .ob_item = {
            & toplevel_consts_11_consts_7_names_0._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[5];
    }
toplevel_consts_11_consts_7_varnames_2 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 4,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "code",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[10];
    }
toplevel_consts_11_consts_7_varnames_3 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 9,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "ispackage",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[5];
        }_object;
    }
toplevel_consts_11_consts_7_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 5,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_varnames_0._ascii.ob_base,
            & toplevel_consts_11_consts_4_varnames_1._ascii.ob_base,
            & toplevel_consts_11_consts_7_varnames_2._ascii.ob_base,
            & toplevel_consts_11_consts_7_varnames_3._ascii.ob_base,
            & toplevel_consts_11_consts_4_varnames_4._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[9];
    }
toplevel_consts_11_consts_7_name = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 8,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "get_code",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[21];
    }
toplevel_consts_11_consts_7_qualname = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 20,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "zipimporter.get_code",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[5];
    }
toplevel_consts_11_consts_7_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 4,
    },
    .ob_shash = -1,
    .ob_sval = "\x10\x06\x04\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[21];
    }
toplevel_consts_11_consts_7_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 20,
    },
    .ob_shash = -1,
    .ob_sval = "\x24\x34\x35\x39\x3b\x43\x24\x44\x09\x21\x09\x0d\x0f\x18\x1a\x21\x10\x14\x09\x14",
};
static struct PyCodeObject toplevel_consts_11_consts_7 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_11_consts_7_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_11_consts_7_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_11_consts_7_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_7_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 2,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 3,
    .co_firstlineno = 190,
    .co_code = & toplevel_consts_11_consts_7_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_11_consts_7_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_11_consts_4_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_11_consts_7_name._ascii.ob_base,
    .co_qualname = & toplevel_consts_11_consts_7_qualname._ascii.ob_base,
    .co_linetable = & toplevel_consts_11_consts_7_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_11_consts_7_linetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_11_consts_7_columntable.ob_base.ob_base,
    .co_nlocalsplus = 5,
    .co_nlocals = 5,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_11_consts_7_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[115];
    }
toplevel_consts_11_consts_8_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 114,
    },
    .ob_shash = -1,
    .ob_sval = "\x74\x00\x72\x08\x7c\x01\xa0\x01\x74\x00\x74\x02\xa1\x02\x7d\x01\x7c\x01\x7d\x02\x7c\x01\xa0\x03\x7c\x00\x6a\x04\x74\x02\x17\x00\xa1\x01\x72\x1d\x7c\x01\x74\x05\x7c\x00\x6a\x04\x74\x02\x17\x00\x83\x01\x64\x01\x85\x02\x19\x00\x7d\x02\x09\x00\x7c\x00\x6a\x06\x7c\x02\x19\x00\x7d\x03\x6e\x0f\x23\x00\x04\x00\x74\x07\x79\x31\x01\x00\x01\x00\x01\x00\x74\x08\x64\x02\x64\x03\x7c\x02\x83\x03\x82\x01\x77\x00\x25\x00\x74\x09\x7c\x00\x6a\x04\x7c\x03\x83\x02\x53\x00",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[155];
    }
toplevel_consts_11_consts_8_consts_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 154,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "\x67\x65\x74\x5f\x64\x61\x74\x61\x28\x70\x61\x74\x68\x6e\x61\x6d\x65\x29\x20\x2d\x3e\x20\x73\x74\x72\x69\x6e\x67\x20\x77\x69\x74\x68\x20\x66\x69\x6c\x65\x20\x64\x61\x74\x61\x2e\x0a\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x52\x65\x74\x75\x72\x6e\x20\x74\x68\x65\x20\x64\x61\x74\x61\x20\x61\x73\x73\x6f\x63\x69\x61\x74\x65\x64\x20\x77\x69\x74\x68\x20\x27\x70\x61\x74\x68\x6e\x61\x6d\x65\x27\x2e\x20\x52\x61\x69\x73\x65\x20\x4f\x53\x45\x72\x72\x6f\x72\x20\x69\x66\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x74\x68\x65\x20\x66\x69\x6c\x65\x20\x77\x61\x73\x6e\x27\x74\x20\x66\x6f\x75\x6e\x64\x2e\x0a\x20\x20\x20\x20\x20\x20\x20\x20",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[1];
    }
toplevel_consts_11_consts_8_consts_3 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 0,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[4];
        }_object;
    }
toplevel_consts_11_consts_8_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 4,
        },
        .ob_item = {
            & toplevel_consts_11_consts_8_consts_0._ascii.ob_base,
            Py_None,
            & toplevel_consts_1.ob_base.ob_base,
            & toplevel_consts_11_consts_8_consts_3._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[11];
    }
toplevel_consts_11_consts_8_names_3 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 10,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "startswith",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[4];
    }
toplevel_consts_11_consts_8_names_5 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 3,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "len",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[10];
    }
toplevel_consts_11_consts_8_names_9 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 9,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_get_data",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[10];
        }_object;
    }
toplevel_consts_11_consts_8_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 10,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_names_5._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_6._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_7._ascii.ob_base,
            & toplevel_consts_11_consts_8_names_3._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_19._ascii.ob_base,
            & toplevel_consts_11_consts_8_names_5._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_18._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_16._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_10._ascii.ob_base,
            & toplevel_consts_11_consts_8_names_9._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[9];
    }
toplevel_consts_11_consts_8_varnames_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 8,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "pathname",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[4];
    }
toplevel_consts_11_consts_8_varnames_2 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 3,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "key",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[10];
    }
toplevel_consts_11_consts_8_varnames_3 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 9,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "toc_entry",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[4];
        }_object;
    }
toplevel_consts_11_consts_8_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 4,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_varnames_0._ascii.ob_base,
            & toplevel_consts_11_consts_8_varnames_1._ascii.ob_base,
            & toplevel_consts_11_consts_8_varnames_2._ascii.ob_base,
            & toplevel_consts_11_consts_8_varnames_3._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[9];
    }
toplevel_consts_11_consts_8_name = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 8,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "get_data",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[21];
    }
toplevel_consts_11_consts_8_qualname = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 20,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "zipimporter.get_data",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[27];
    }
toplevel_consts_11_consts_8_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 26,
    },
    .ob_shash = -1,
    .ob_sval = "\x04\x06\x0c\x01\x04\x02\x10\x01\x16\x01\x02\x02\x0c\x01\x02\x80\x0c\x01\x0c\x01\x02\xff\x02\x80\x0c\x02",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[27];
    }
toplevel_consts_11_consts_8_endlinetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 26,
    },
    .ob_shash = -1,
    .ob_sval = "\x02\x06\x0e\x01\x04\x02\x0e\x01\x18\x01\x02\x05\x0c\xfe\x02\x80\x02\x02\x02\xff\x16\x01\x02\x80\x0c\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[115];
    }
toplevel_consts_11_consts_8_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 114,
    },
    .ob_shash = -1,
    .ob_sval = "\x0c\x18\x09\x40\x18\x20\x18\x40\x29\x35\x37\x3f\x18\x40\x0d\x15\x0f\x17\x09\x0c\x0c\x14\x0c\x38\x20\x24\x20\x2c\x2f\x37\x20\x37\x0c\x38\x09\x3a\x13\x1b\x1c\x1f\x20\x24\x20\x2c\x2f\x37\x20\x37\x1c\x38\x1c\x39\x1c\x39\x13\x3a\x0d\x10\x09\x26\x19\x1d\x19\x24\x25\x28\x19\x29\x0d\x16\x0d\x16\x00\x00\x09\x26\x10\x18\x09\x26\x09\x26\x09\x26\x09\x26\x13\x1a\x1b\x1c\x1e\x20\x22\x25\x13\x26\x0d\x26\x09\x26\x00\x00\x10\x19\x1a\x1e\x1a\x26\x28\x31\x10\x32\x09\x32",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[9];
    }
toplevel_consts_11_consts_8_exceptiontable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 8,
    },
    .ob_shash = -1,
    .ob_sval = "\x9e\x05\x24\x00\xa4\x0e\x32\x07",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[5];
    }
toplevel_consts_11_consts_8_localspluskinds = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 4,
    },
    .ob_shash = -1,
    .ob_sval = "    ",
};
static struct PyCodeObject toplevel_consts_11_consts_8 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_11_consts_8_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_11_consts_8_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_11_consts_8_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_11_consts_8_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 2,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 8,
    .co_firstlineno = 200,
    .co_code = & toplevel_consts_11_consts_8_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_11_consts_8_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_11_consts_8_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_11_consts_8_name._ascii.ob_base,
    .co_qualname = & toplevel_consts_11_consts_8_qualname._ascii.ob_base,
    .co_linetable = & toplevel_consts_11_consts_8_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_11_consts_8_endlinetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_11_consts_8_columntable.ob_base.ob_base,
    .co_nlocalsplus = 4,
    .co_nlocals = 4,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_11_consts_8_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[21];
    }
toplevel_consts_11_consts_9_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 20,
    },
    .ob_shash = -1,
    .ob_sval = "\x74\x00\x7c\x00\x7c\x01\x83\x02\x5c\x03\x7d\x02\x7d\x03\x7d\x04\x7c\x04\x53\x00",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[166];
    }
toplevel_consts_11_consts_9_consts_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 165,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "\x67\x65\x74\x5f\x66\x69\x6c\x65\x6e\x61\x6d\x65\x28\x66\x75\x6c\x6c\x6e\x61\x6d\x65\x29\x20\x2d\x3e\x20\x66\x69\x6c\x65\x6e\x61\x6d\x65\x20\x73\x74\x72\x69\x6e\x67\x2e\x0a\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x52\x65\x74\x75\x72\x6e\x20\x74\x68\x65\x20\x66\x69\x6c\x65\x6e\x61\x6d\x65\x20\x66\x6f\x72\x20\x74\x68\x65\x20\x73\x70\x65\x63\x69\x66\x69\x65\x64\x20\x6d\x6f\x64\x75\x6c\x65\x20\x6f\x72\x20\x72\x61\x69\x73\x65\x20\x5a\x69\x70\x49\x6d\x70\x6f\x72\x74\x45\x72\x72\x6f\x72\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x69\x66\x20\x69\x74\x20\x63\x6f\x75\x6c\x64\x6e\x27\x74\x20\x62\x65\x20\x69\x6d\x70\x6f\x72\x74\x65\x64\x2e\x0a\x20\x20\x20\x20\x20\x20\x20\x20",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[2];
        }_object;
    }
toplevel_consts_11_consts_9_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 2,
        },
        .ob_item = {
            & toplevel_consts_11_consts_9_consts_0._ascii.ob_base,
            Py_None,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[13];
    }
toplevel_consts_11_consts_9_name = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 12,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "get_filename",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[25];
    }
toplevel_consts_11_consts_9_qualname = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 24,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "zipimporter.get_filename",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[5];
    }
toplevel_consts_11_consts_9_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 4,
    },
    .ob_shash = -1,
    .ob_sval = "\x10\x08\x04\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[21];
    }
toplevel_consts_11_consts_9_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 20,
    },
    .ob_shash = -1,
    .ob_sval = "\x24\x34\x35\x39\x3b\x43\x24\x44\x09\x21\x09\x0d\x0f\x18\x1a\x21\x10\x17\x09\x17",
};
static struct PyCodeObject toplevel_consts_11_consts_9 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_11_consts_9_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_11_consts_7_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_11_consts_9_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_7_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 2,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 3,
    .co_firstlineno = 221,
    .co_code = & toplevel_consts_11_consts_9_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_11_consts_7_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_11_consts_4_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_11_consts_9_name._ascii.ob_base,
    .co_qualname = & toplevel_consts_11_consts_9_qualname._ascii.ob_base,
    .co_linetable = & toplevel_consts_11_consts_9_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_11_consts_9_linetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_11_consts_9_columntable.ob_base.ob_base,
    .co_nlocalsplus = 5,
    .co_nlocals = 5,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_11_consts_7_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[129];
    }
toplevel_consts_11_consts_10_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 128,
    },
    .ob_shash = -1,
    .ob_sval = "\x74\x00\x7c\x00\x7c\x01\x83\x02\x7d\x02\x7c\x02\x64\x01\x75\x00\x72\x12\x74\x01\x64\x02\x7c\x01\x9b\x02\x9d\x02\x7c\x01\x64\x03\x8d\x02\x82\x01\x74\x02\x7c\x00\x7c\x01\x83\x02\x7d\x03\x7c\x02\x72\x20\x74\x03\x6a\x04\x7c\x03\x64\x04\x83\x02\x7d\x04\x6e\x05\x7c\x03\x9b\x00\x64\x05\x9d\x02\x7d\x04\x09\x00\x7c\x00\x6a\x05\x7c\x04\x19\x00\x7d\x05\x6e\x0c\x23\x00\x04\x00\x74\x06\x79\x36\x01\x00\x01\x00\x01\x00\x59\x00\x64\x01\x53\x00\x77\x00\x25\x00\x74\x07\x7c\x00\x6a\x08\x7c\x05\x83\x02\xa0\x09\xa1\x00\x53\x00",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[254];
    }
toplevel_consts_11_consts_10_consts_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 253,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "\x67\x65\x74\x5f\x73\x6f\x75\x72\x63\x65\x28\x66\x75\x6c\x6c\x6e\x61\x6d\x65\x29\x20\x2d\x3e\x20\x73\x6f\x75\x72\x63\x65\x20\x73\x74\x72\x69\x6e\x67\x2e\x0a\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x52\x65\x74\x75\x72\x6e\x20\x74\x68\x65\x20\x73\x6f\x75\x72\x63\x65\x20\x63\x6f\x64\x65\x20\x66\x6f\x72\x20\x74\x68\x65\x20\x73\x70\x65\x63\x69\x66\x69\x65\x64\x20\x6d\x6f\x64\x75\x6c\x65\x2e\x20\x52\x61\x69\x73\x65\x20\x5a\x69\x70\x49\x6d\x70\x6f\x72\x74\x45\x72\x72\x6f\x72\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x69\x66\x20\x74\x68\x65\x20\x6d\x6f\x64\x75\x6c\x65\x20\x63\x6f\x75\x6c\x64\x6e\x27\x74\x20\x62\x65\x20\x66\x6f\x75\x6e\x64\x2c\x20\x72\x65\x74\x75\x72\x6e\x20\x4e\x6f\x6e\x65\x20\x69\x66\x20\x74\x68\x65\x20\x61\x72\x63\x68\x69\x76\x65\x20\x64\x6f\x65\x73\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x63\x6f\x6e\x74\x61\x69\x6e\x20\x74\x68\x65\x20\x6d\x6f\x64\x75\x6c\x65\x2c\x20\x62\x75\x74\x20\x68\x61\x73\x20\x6e\x6f\x20\x73\x6f\x75\x72\x63\x65\x20\x66\x6f\x72\x20\x69\x74\x2e\x0a\x20\x20\x20\x20\x20\x20\x20\x20",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[19];
    }
toplevel_consts_11_consts_10_consts_2 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 18,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "can't find module ",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[1];
        }_object;
    }
toplevel_consts_11_consts_10_consts_3 = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 1,
        },
        .ob_item = {
            & toplevel_consts_11_consts_6_consts_4_0._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[12];
    }
toplevel_consts_11_consts_10_consts_4 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 11,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "__init__.py",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[4];
    }
toplevel_consts_11_consts_10_consts_5 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 3,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = ".py",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[6];
        }_object;
    }
toplevel_consts_11_consts_10_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 6,
        },
        .ob_item = {
            & toplevel_consts_11_consts_10_consts_0._ascii.ob_base,
            Py_None,
            & toplevel_consts_11_consts_10_consts_2._ascii.ob_base,
            & toplevel_consts_11_consts_10_consts_3._object.ob_base.ob_base,
            & toplevel_consts_11_consts_10_consts_4._ascii.ob_base,
            & toplevel_consts_11_consts_10_consts_5._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[7];
    }
toplevel_consts_11_consts_10_names_9 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 6,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "decode",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[10];
        }_object;
    }
toplevel_consts_11_consts_10_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 10,
        },
        .ob_item = {
            & toplevel_consts_11_consts_4_names_3._ascii.ob_base,
            & toplevel_consts_4._ascii.ob_base,
            & toplevel_consts_11_consts_4_names_4._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_8._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_20._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_18._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_16._ascii.ob_base,
            & toplevel_consts_11_consts_8_names_9._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_19._ascii.ob_base,
            & toplevel_consts_11_consts_10_names_9._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[9];
    }
toplevel_consts_11_consts_10_varnames_4 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 8,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "fullpath",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[6];
        }_object;
    }
toplevel_consts_11_consts_10_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 6,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_varnames_0._ascii.ob_base,
            & toplevel_consts_11_consts_4_varnames_1._ascii.ob_base,
            & toplevel_consts_11_consts_4_varnames_3._ascii.ob_base,
            & toplevel_consts_11_consts_2_consts_3_0._ascii.ob_base,
            & toplevel_consts_11_consts_10_varnames_4._ascii.ob_base,
            & toplevel_consts_11_consts_8_varnames_3._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[11];
    }
toplevel_consts_11_consts_10_name = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 10,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "get_source",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[23];
    }
toplevel_consts_11_consts_10_qualname = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 22,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "zipimporter.get_source",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[31];
    }
toplevel_consts_11_consts_10_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 30,
    },
    .ob_shash = -1,
    .ob_sval = "\x0a\x07\x08\x01\x12\x01\x0a\x02\x04\x01\x0e\x01\x0a\x02\x02\x02\x0c\x01\x02\x80\x0c\x01\x06\x02\x02\xfe\x02\x80\x10\x03",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[33];
    }
toplevel_consts_11_consts_10_endlinetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 32,
    },
    .ob_shash = -1,
    .ob_sval = "\x0a\x07\x06\x01\x14\x01\x0a\x02\x02\x01\x02\x03\x0e\xfe\x0a\x02\x02\x06\x0c\xfd\x02\x80\x02\x03\x02\xfe\x10\x02\x02\x80\x10\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[129];
    }
toplevel_consts_11_consts_10_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 128,
    },
    .ob_shash = -1,
    .ob_sval = "\x0e\x1e\x1f\x23\x25\x2d\x0e\x2e\x09\x0b\x0c\x0e\x12\x16\x0c\x16\x09\x53\x13\x21\x22\x43\x37\x3f\x22\x43\x22\x43\x4a\x52\x13\x53\x13\x53\x0d\x53\x10\x20\x21\x25\x27\x2f\x10\x30\x09\x0d\x0c\x0e\x09\x24\x18\x2b\x18\x36\x37\x3b\x3d\x4a\x18\x4b\x0d\x15\x0d\x15\x1b\x1f\x18\x24\x18\x24\x18\x24\x0d\x15\x09\x18\x19\x1d\x19\x24\x25\x2d\x19\x2e\x0d\x16\x0d\x16\x00\x00\x09\x18\x10\x18\x09\x18\x09\x18\x09\x18\x09\x18\x14\x18\x14\x18\x14\x18\x09\x18\x00\x00\x10\x19\x1a\x1e\x1a\x26\x28\x31\x10\x32\x10\x3b\x10\x3b\x09\x3b",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[13];
    }
toplevel_consts_11_consts_10_exceptiontable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 12,
    },
    .ob_shash = -1,
    .ob_sval = "\xa6\x05\x2c\x00\xac\x07\x37\x07\xb6\x01\x37\x07",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[7];
    }
toplevel_consts_11_consts_10_localspluskinds = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 6,
    },
    .ob_shash = -1,
    .ob_sval = "      ",
};
static struct PyCodeObject toplevel_consts_11_consts_10 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_11_consts_10_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_11_consts_10_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_11_consts_10_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_11_consts_10_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 2,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 8,
    .co_firstlineno = 233,
    .co_code = & toplevel_consts_11_consts_10_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_11_consts_10_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_11_consts_10_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_11_consts_10_name._ascii.ob_base,
    .co_qualname = & toplevel_consts_11_consts_10_qualname._ascii.ob_base,
    .co_linetable = & toplevel_consts_11_consts_10_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_11_consts_10_endlinetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_11_consts_10_columntable.ob_base.ob_base,
    .co_nlocalsplus = 6,
    .co_nlocals = 6,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_11_consts_10_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[41];
    }
toplevel_consts_11_consts_11_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 40,
    },
    .ob_shash = -1,
    .ob_sval = "\x74\x00\x7c\x00\x7c\x01\x83\x02\x7d\x02\x7c\x02\x64\x01\x75\x00\x72\x12\x74\x01\x64\x02\x7c\x01\x9b\x02\x9d\x02\x7c\x01\x64\x03\x8d\x02\x82\x01\x7c\x02\x53\x00",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[172];
    }
toplevel_consts_11_consts_11_consts_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 171,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "\x69\x73\x5f\x70\x61\x63\x6b\x61\x67\x65\x28\x66\x75\x6c\x6c\x6e\x61\x6d\x65\x29\x20\x2d\x3e\x20\x62\x6f\x6f\x6c\x2e\x0a\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x52\x65\x74\x75\x72\x6e\x20\x54\x72\x75\x65\x20\x69\x66\x20\x74\x68\x65\x20\x6d\x6f\x64\x75\x6c\x65\x20\x73\x70\x65\x63\x69\x66\x69\x65\x64\x20\x62\x79\x20\x66\x75\x6c\x6c\x6e\x61\x6d\x65\x20\x69\x73\x20\x61\x20\x70\x61\x63\x6b\x61\x67\x65\x2e\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x52\x61\x69\x73\x65\x20\x5a\x69\x70\x49\x6d\x70\x6f\x72\x74\x45\x72\x72\x6f\x72\x20\x69\x66\x20\x74\x68\x65\x20\x6d\x6f\x64\x75\x6c\x65\x20\x63\x6f\x75\x6c\x64\x6e\x27\x74\x20\x62\x65\x20\x66\x6f\x75\x6e\x64\x2e\x0a\x20\x20\x20\x20\x20\x20\x20\x20",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[4];
        }_object;
    }
toplevel_consts_11_consts_11_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 4,
        },
        .ob_item = {
            & toplevel_consts_11_consts_11_consts_0._ascii.ob_base,
            Py_None,
            & toplevel_consts_11_consts_10_consts_2._ascii.ob_base,
            & toplevel_consts_11_consts_10_consts_3._object.ob_base.ob_base,
        },
    },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[2];
        }_object;
    }
toplevel_consts_11_consts_11_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 2,
        },
        .ob_item = {
            & toplevel_consts_11_consts_4_names_3._ascii.ob_base,
            & toplevel_consts_4._ascii.ob_base,
        },
    },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[3];
        }_object;
    }
toplevel_consts_11_consts_11_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 3,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_varnames_0._ascii.ob_base,
            & toplevel_consts_11_consts_4_varnames_1._ascii.ob_base,
            & toplevel_consts_11_consts_4_varnames_3._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[23];
    }
toplevel_consts_11_consts_11_qualname = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 22,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "zipimporter.is_package",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[9];
    }
toplevel_consts_11_consts_11_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 8,
    },
    .ob_shash = -1,
    .ob_sval = "\x0a\x06\x08\x01\x12\x01\x04\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[9];
    }
toplevel_consts_11_consts_11_endlinetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 8,
    },
    .ob_shash = -1,
    .ob_sval = "\x0a\x06\x06\x01\x14\x01\x04\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[41];
    }
toplevel_consts_11_consts_11_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 40,
    },
    .ob_shash = -1,
    .ob_sval = "\x0e\x1e\x1f\x23\x25\x2d\x0e\x2e\x09\x0b\x0c\x0e\x12\x16\x0c\x16\x09\x53\x13\x21\x22\x43\x37\x3f\x22\x43\x22\x43\x4a\x52\x13\x53\x13\x53\x0d\x53\x10\x12\x09\x12",
};
static struct PyCodeObject toplevel_consts_11_consts_11 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_11_consts_11_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_11_consts_11_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_11_consts_11_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_7_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 2,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 4,
    .co_firstlineno = 259,
    .co_code = & toplevel_consts_11_consts_11_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_11_consts_11_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_11_consts_5_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_11_consts_6_consts_2_0._ascii.ob_base,
    .co_qualname = & toplevel_consts_11_consts_11_qualname._ascii.ob_base,
    .co_linetable = & toplevel_consts_11_consts_11_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_11_consts_11_endlinetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_11_consts_11_columntable.ob_base.ob_base,
    .co_nlocalsplus = 3,
    .co_nlocals = 3,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_11_consts_11_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[257];
    }
toplevel_consts_11_consts_12_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 256,
    },
    .ob_shash = -1,
    .ob_sval = "\x64\x01\x7d\x02\x74\x00\x6a\x01\x7c\x02\x74\x02\x83\x02\x01\x00\x74\x03\x7c\x00\x7c\x01\x83\x02\x5c\x03\x7d\x03\x7d\x04\x7d\x05\x74\x04\x6a\x05\xa0\x06\x7c\x01\xa1\x01\x7d\x06\x7c\x06\x64\x02\x75\x00\x73\x1f\x74\x07\x7c\x06\x74\x08\x83\x02\x73\x28\x74\x08\x7c\x01\x83\x01\x7d\x06\x7c\x06\x74\x04\x6a\x05\x7c\x01\x3c\x00\x7c\x00\x7c\x06\x5f\x09\x09\x00\x7c\x04\x72\x3e\x74\x0a\x7c\x00\x7c\x01\x83\x02\x7d\x07\x74\x0b\x6a\x0c\x7c\x00\x6a\x0d\x7c\x07\x83\x02\x7d\x08\x7c\x08\x67\x01\x7c\x06\x5f\x0e\x74\x0f\x7c\x06\x64\x03\x83\x02\x73\x46\x74\x10\x7c\x06\x5f\x10\x74\x0b\x6a\x11\x7c\x06\x6a\x12\x7c\x01\x7c\x05\x83\x03\x01\x00\x74\x13\x7c\x03\x7c\x06\x6a\x12\x83\x02\x01\x00\x6e\x0a\x23\x00\x01\x00\x01\x00\x01\x00\x74\x04\x6a\x05\x7c\x01\x3d\x00\x82\x00\x25\x00\x09\x00\x74\x04\x6a\x05\x7c\x01\x19\x00\x7d\x06\x6e\x11\x23\x00\x04\x00\x74\x14\x79\x75\x01\x00\x01\x00\x01\x00\x74\x15\x64\x04\x7c\x01\x9b\x02\x64\x05\x9d\x03\x83\x01\x82\x01\x77\x00\x25\x00\x74\x16\x6a\x17\x64\x06\x7c\x01\x7c\x05\x83\x03\x01\x00\x7c\x06\x53\x00",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[321];
    }
toplevel_consts_11_consts_12_consts_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 320,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "\x6c\x6f\x61\x64\x5f\x6d\x6f\x64\x75\x6c\x65\x28\x66\x75\x6c\x6c\x6e\x61\x6d\x65\x29\x20\x2d\x3e\x20\x6d\x6f\x64\x75\x6c\x65\x2e\x0a\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x4c\x6f\x61\x64\x20\x74\x68\x65\x20\x6d\x6f\x64\x75\x6c\x65\x20\x73\x70\x65\x63\x69\x66\x69\x65\x64\x20\x62\x79\x20\x27\x66\x75\x6c\x6c\x6e\x61\x6d\x65\x27\x2e\x20\x27\x66\x75\x6c\x6c\x6e\x61\x6d\x65\x27\x20\x6d\x75\x73\x74\x20\x62\x65\x20\x74\x68\x65\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x66\x75\x6c\x6c\x79\x20\x71\x75\x61\x6c\x69\x66\x69\x65\x64\x20\x28\x64\x6f\x74\x74\x65\x64\x29\x20\x6d\x6f\x64\x75\x6c\x65\x20\x6e\x61\x6d\x65\x2e\x20\x49\x74\x20\x72\x65\x74\x75\x72\x6e\x73\x20\x74\x68\x65\x20\x69\x6d\x70\x6f\x72\x74\x65\x64\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x6d\x6f\x64\x75\x6c\x65\x2c\x20\x6f\x72\x20\x72\x61\x69\x73\x65\x73\x20\x5a\x69\x70\x49\x6d\x70\x6f\x72\x74\x45\x72\x72\x6f\x72\x20\x69\x66\x20\x69\x74\x20\x63\x6f\x75\x6c\x64\x20\x6e\x6f\x74\x20\x62\x65\x20\x69\x6d\x70\x6f\x72\x74\x65\x64\x2e\x0a\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x44\x65\x70\x72\x65\x63\x61\x74\x65\x64\x20\x73\x69\x6e\x63\x65\x20\x50\x79\x74\x68\x6f\x6e\x20\x33\x2e\x31\x30\x2e\x20\x55\x73\x65\x20\x65\x78\x65\x63\x5f\x6d\x6f\x64\x75\x6c\x65\x28\x29\x20\x69\x6e\x73\x74\x65\x61\x64\x2e\x0a\x20\x20\x20\x20\x20\x20\x20\x20",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[115];
    }
toplevel_consts_11_consts_12_consts_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 114,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "zipimport.zipimporter.load_module() is deprecated and slated for removal in Python 3.12; use exec_module() instead",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[13];
    }
toplevel_consts_11_consts_12_consts_3 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 12,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "__builtins__",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[15];
    }
toplevel_consts_11_consts_12_consts_4 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 14,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "Loaded module ",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[26];
    }
toplevel_consts_11_consts_12_consts_5 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 25,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = " not found in sys.modules",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[31];
    }
toplevel_consts_11_consts_12_consts_6 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 30,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "import {} # loaded from Zip {}",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[7];
        }_object;
    }
toplevel_consts_11_consts_12_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 7,
        },
        .ob_item = {
            & toplevel_consts_11_consts_12_consts_0._ascii.ob_base,
            & toplevel_consts_11_consts_12_consts_1._ascii.ob_base,
            Py_None,
            & toplevel_consts_11_consts_12_consts_3._ascii.ob_base,
            & toplevel_consts_11_consts_12_consts_4._ascii.ob_base,
            & toplevel_consts_11_consts_12_consts_5._ascii.ob_base,
            & toplevel_consts_11_consts_12_consts_6._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[4];
    }
toplevel_consts_11_consts_12_names_4 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 3,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "sys",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[8];
    }
toplevel_consts_11_consts_12_names_5 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 7,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "modules",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[4];
    }
toplevel_consts_11_consts_12_names_6 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 3,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "get",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[13];
    }
toplevel_consts_11_consts_12_names_8 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 12,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_module_type",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[11];
    }
toplevel_consts_11_consts_12_names_9 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 10,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "__loader__",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[9];
    }
toplevel_consts_11_consts_12_names_14 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 8,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "__path__",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[8];
    }
toplevel_consts_11_consts_12_names_15 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 7,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "hasattr",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[15];
    }
toplevel_consts_11_consts_12_names_17 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 14,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_fix_up_module",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[9];
    }
toplevel_consts_11_consts_12_names_18 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 8,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "__dict__",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[5];
    }
toplevel_consts_11_consts_12_names_19 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 4,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "exec",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[12];
    }
toplevel_consts_11_consts_12_names_21 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 11,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "ImportError",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[17];
    }
toplevel_consts_11_consts_12_names_23 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 16,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_verbose_message",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[24];
        }_object;
    }
toplevel_consts_11_consts_12_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 24,
        },
        .ob_item = {
            & toplevel_consts_11_consts_4_names_0._ascii.ob_base,
            & toplevel_consts_11_consts_4_names_1._ascii.ob_base,
            & toplevel_consts_11_consts_4_names_2._ascii.ob_base,
            & toplevel_consts_11_consts_7_names_0._ascii.ob_base,
            & toplevel_consts_11_consts_12_names_4._ascii.ob_base,
            & toplevel_consts_11_consts_12_names_5._ascii.ob_base,
            & toplevel_consts_11_consts_12_names_6._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_0._ascii.ob_base,
            & toplevel_consts_11_consts_12_names_8._ascii.ob_base,
            & toplevel_consts_11_consts_12_names_9._ascii.ob_base,
            & toplevel_consts_11_consts_4_names_4._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_8._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_20._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_19._ascii.ob_base,
            & toplevel_consts_11_consts_12_names_14._ascii.ob_base,
            & toplevel_consts_11_consts_12_names_15._ascii.ob_base,
            & toplevel_consts_11_consts_12_consts_3._ascii.ob_base,
            & toplevel_consts_11_consts_12_names_17._ascii.ob_base,
            & toplevel_consts_11_consts_12_names_18._ascii.ob_base,
            & toplevel_consts_11_consts_12_names_19._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_16._ascii.ob_base,
            & toplevel_consts_11_consts_12_names_21._ascii.ob_base,
            & toplevel_consts_11_consts_6_names_1._ascii.ob_base,
            & toplevel_consts_11_consts_12_names_23._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[4];
    }
toplevel_consts_11_consts_12_varnames_2 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 3,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "msg",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[4];
    }
toplevel_consts_11_consts_12_varnames_6 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 3,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "mod",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[9];
        }_object;
    }
toplevel_consts_11_consts_12_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 9,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_varnames_0._ascii.ob_base,
            & toplevel_consts_11_consts_4_varnames_1._ascii.ob_base,
            & toplevel_consts_11_consts_12_varnames_2._ascii.ob_base,
            & toplevel_consts_11_consts_7_varnames_2._ascii.ob_base,
            & toplevel_consts_11_consts_7_varnames_3._ascii.ob_base,
            & toplevel_consts_11_consts_4_varnames_4._ascii.ob_base,
            & toplevel_consts_11_consts_12_varnames_6._ascii.ob_base,
            & toplevel_consts_11_consts_2_consts_3_0._ascii.ob_base,
            & toplevel_consts_11_consts_10_varnames_4._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[12];
    }
toplevel_consts_11_consts_12_name = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 11,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "load_module",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[24];
    }
toplevel_consts_11_consts_12_qualname = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 23,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "zipimporter.load_module",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[63];
    }
toplevel_consts_11_consts_12_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 62,
    },
    .ob_shash = -1,
    .ob_sval = "\x04\x09\x0c\x02\x10\x01\x0c\x01\x12\x01\x08\x01\x0a\x01\x06\x01\x02\x02\x04\x01\x0a\x03\x0e\x01\x08\x01\x0a\x02\x06\x01\x10\x01\x0e\x01\x02\x80\x06\x01\x08\x01\x02\x01\x02\x80\x02\x02\x0c\x01\x02\x80\x0c\x01\x10\x01\x02\xff\x02\x80\x0e\x02\x04\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[73];
    }
toplevel_consts_11_consts_12_endlinetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 72,
    },
    .ob_shash = -1,
    .ob_sval = "\x02\x0a\x02\xff\x0c\x02\x10\x01\x0c\x01\x06\x01\x02\x02\x08\xfe\x02\x02\x08\xff\x0a\x01\x06\x01\x02\x10\x02\xf3\x02\x05\x0a\xfe\x0e\x01\x08\x01\x08\x02\x08\x01\x10\x01\x0e\x01\x02\x80\x06\x03\x08\xff\x02\x01\x02\x80\x02\x05\x0c\xfe\x02\x80\x02\x02\x02\xff\x1a\x01\x02\x80\x0e\x01\x04\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[257];
    }
toplevel_consts_11_consts_12_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 256,
    },
    .ob_shash = -1,
    .ob_sval = "\x10\x43\x09\x0c\x09\x12\x09\x17\x18\x1b\x1d\x2f\x09\x30\x09\x30\x24\x34\x35\x39\x3b\x43\x24\x44\x09\x21\x09\x0d\x0f\x18\x1a\x21\x0f\x12\x0f\x1a\x0f\x28\x1f\x27\x0f\x28\x09\x0c\x0c\x0f\x13\x17\x0c\x17\x09\x28\x1f\x29\x2a\x2d\x2f\x3b\x1f\x3c\x09\x28\x13\x1f\x20\x28\x13\x29\x0d\x10\x25\x28\x0d\x10\x0d\x18\x19\x21\x0d\x22\x1a\x1e\x09\x0c\x09\x17\x09\x12\x10\x19\x0d\x2a\x18\x28\x29\x2d\x2f\x37\x18\x38\x11\x15\x1c\x2f\x1c\x3a\x3b\x3f\x3b\x47\x49\x4d\x1c\x4e\x11\x19\x21\x29\x20\x2a\x11\x14\x11\x1d\x14\x1b\x1c\x1f\x21\x2f\x14\x30\x0d\x30\x24\x30\x11\x14\x11\x21\x0d\x20\x0d\x2f\x30\x33\x30\x3c\x3e\x46\x48\x4f\x0d\x50\x0d\x50\x0d\x11\x12\x16\x18\x1b\x18\x24\x0d\x25\x0d\x25\x0d\x25\x00\x00\x09\x12\x09\x12\x09\x12\x11\x14\x11\x1c\x1d\x25\x11\x26\x0d\x12\x00\x00\x09\x56\x13\x16\x13\x1e\x1f\x27\x13\x28\x0d\x10\x0d\x10\x00\x00\x09\x56\x10\x18\x09\x56\x09\x56\x09\x56\x09\x56\x13\x1e\x1f\x55\x30\x38\x1f\x55\x1f\x55\x1f\x55\x13\x56\x0d\x56\x09\x56\x00\x00\x09\x13\x09\x24\x25\x45\x47\x4f\x51\x58\x09\x59\x09\x59\x10\x13\x09\x13",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[24];
    }
toplevel_consts_11_consts_12_exceptiontable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 23,
    },
    .ob_shash = -1,
    .ob_sval = "\xac\x28\x41\x15\x00\xc1\x15\x09\x41\x1e\x07\xc1\x20\x05\x41\x26\x00\xc1\x26\x10\x41\x36\x07",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[10];
    }
toplevel_consts_11_consts_12_localspluskinds = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 9,
    },
    .ob_shash = -1,
    .ob_sval = "         ",
};
static struct PyCodeObject toplevel_consts_11_consts_12 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_11_consts_12_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_11_consts_12_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_11_consts_12_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_11_consts_12_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 2,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 8,
    .co_firstlineno = 272,
    .co_code = & toplevel_consts_11_consts_12_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_11_consts_12_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_11_consts_12_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_11_consts_12_name._ascii.ob_base,
    .co_qualname = & toplevel_consts_11_consts_12_qualname._ascii.ob_base,
    .co_linetable = & toplevel_consts_11_consts_12_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_11_consts_12_endlinetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_11_consts_12_columntable.ob_base.ob_base,
    .co_nlocalsplus = 9,
    .co_nlocals = 9,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_11_consts_12_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[65];
    }
toplevel_consts_11_consts_13_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 64,
    },
    .ob_shash = -1,
    .ob_sval = "\x09\x00\x7c\x00\xa0\x00\x7c\x01\xa1\x01\x73\x08\x64\x01\x53\x00\x6e\x0c\x23\x00\x04\x00\x74\x01\x79\x13\x01\x00\x01\x00\x01\x00\x59\x00\x64\x01\x53\x00\x77\x00\x25\x00\x64\x02\x64\x03\x6c\x02\x6d\x03\x7d\x02\x01\x00\x7c\x02\x7c\x00\x7c\x01\x83\x02\x53\x00",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[205];
    }
toplevel_consts_11_consts_13_consts_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 204,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "\x52\x65\x74\x75\x72\x6e\x20\x74\x68\x65\x20\x52\x65\x73\x6f\x75\x72\x63\x65\x52\x65\x61\x64\x65\x72\x20\x66\x6f\x72\x20\x61\x20\x70\x61\x63\x6b\x61\x67\x65\x20\x69\x6e\x20\x61\x20\x7a\x69\x70\x20\x66\x69\x6c\x65\x2e\x0a\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x49\x66\x20\x27\x66\x75\x6c\x6c\x6e\x61\x6d\x65\x27\x20\x69\x73\x20\x61\x20\x70\x61\x63\x6b\x61\x67\x65\x20\x77\x69\x74\x68\x69\x6e\x20\x74\x68\x65\x20\x7a\x69\x70\x20\x66\x69\x6c\x65\x2c\x20\x72\x65\x74\x75\x72\x6e\x20\x74\x68\x65\x0a\x20\x20\x20\x20\x20\x20\x20\x20\x27\x52\x65\x73\x6f\x75\x72\x63\x65\x52\x65\x61\x64\x65\x72\x27\x20\x6f\x62\x6a\x65\x63\x74\x20\x66\x6f\x72\x20\x74\x68\x65\x20\x70\x61\x63\x6b\x61\x67\x65\x2e\x20\x20\x4f\x74\x68\x65\x72\x77\x69\x73\x65\x20\x72\x65\x74\x75\x72\x6e\x20\x4e\x6f\x6e\x65\x2e\x0a\x20\x20\x20\x20\x20\x20\x20\x20",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[10];
    }
toplevel_consts_11_consts_13_consts_3_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 9,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "ZipReader",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[1];
        }_object;
    }
toplevel_consts_11_consts_13_consts_3 = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 1,
        },
        .ob_item = {
            & toplevel_consts_11_consts_13_consts_3_0._ascii.ob_base,
        },
    },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[4];
        }_object;
    }
toplevel_consts_11_consts_13_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 4,
        },
        .ob_item = {
            & toplevel_consts_11_consts_13_consts_0._ascii.ob_base,
            Py_None,
            & toplevel_consts_1.ob_base.ob_base,
            & toplevel_consts_11_consts_13_consts_3._object.ob_base.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[18];
    }
toplevel_consts_11_consts_13_names_2 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 17,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "importlib.readers",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[4];
        }_object;
    }
toplevel_consts_11_consts_13_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 4,
        },
        .ob_item = {
            & toplevel_consts_11_consts_6_consts_2_0._ascii.ob_base,
            & toplevel_consts_4._ascii.ob_base,
            & toplevel_consts_11_consts_13_names_2._ascii.ob_base,
            & toplevel_consts_11_consts_13_consts_3_0._ascii.ob_base,
        },
    },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[3];
        }_object;
    }
toplevel_consts_11_consts_13_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 3,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_varnames_0._ascii.ob_base,
            & toplevel_consts_11_consts_4_varnames_1._ascii.ob_base,
            & toplevel_consts_11_consts_13_consts_3_0._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[20];
    }
toplevel_consts_11_consts_13_name = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 19,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "get_resource_reader",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[32];
    }
toplevel_consts_11_consts_13_qualname = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 31,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "zipimporter.get_resource_reader",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[23];
    }
toplevel_consts_11_consts_13_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 22,
    },
    .ob_shash = -1,
    .ob_sval = "\x02\x06\x0a\x01\x04\x01\x02\xff\x02\x80\x0c\x02\x06\x01\x02\xff\x02\x80\x0c\x02\x0a\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[21];
    }
toplevel_consts_11_consts_13_endlinetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 20,
    },
    .ob_shash = -1,
    .ob_sval = "\x02\x0a\x08\xfd\x08\x01\x02\x80\x02\x02\x02\xff\x10\x01\x02\x80\x0c\x01\x0a\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[65];
    }
toplevel_consts_11_consts_13_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 64,
    },
    .ob_shash = -1,
    .ob_sval = "\x09\x18\x14\x18\x14\x2d\x24\x2c\x14\x2d\x0d\x1c\x18\x1c\x18\x1c\x0d\x1c\x00\x00\x09\x18\x10\x1e\x09\x18\x09\x18\x09\x18\x09\x18\x14\x18\x14\x18\x14\x18\x09\x18\x00\x00\x09\x30\x09\x30\x09\x30\x09\x30\x09\x30\x09\x30\x10\x19\x1a\x1e\x20\x28\x10\x29\x09\x29",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[13];
    }
toplevel_consts_11_consts_13_exceptiontable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 12,
    },
    .ob_shash = -1,
    .ob_sval = "\x81\x05\x09\x00\x89\x07\x14\x07\x93\x01\x14\x07",
};
static struct PyCodeObject toplevel_consts_11_consts_13 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_11_consts_13_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_11_consts_13_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_11_consts_13_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_11_consts_13_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 2,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 8,
    .co_firstlineno = 315,
    .co_code = & toplevel_consts_11_consts_13_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_11_consts_13_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_11_consts_5_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_11_consts_13_name._ascii.ob_base,
    .co_qualname = & toplevel_consts_11_consts_13_qualname._ascii.ob_base,
    .co_linetable = & toplevel_consts_11_consts_13_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_11_consts_13_endlinetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_11_consts_13_columntable.ob_base.ob_base,
    .co_nlocalsplus = 3,
    .co_nlocals = 3,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_11_consts_13_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[75];
    }
toplevel_consts_11_consts_14_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 74,
    },
    .ob_shash = -1,
    .ob_sval = "\x09\x00\x74\x00\x7c\x00\x6a\x01\x83\x01\x7c\x00\x5f\x02\x7c\x00\x6a\x02\x74\x03\x7c\x00\x6a\x01\x3c\x00\x64\x01\x53\x00\x23\x00\x04\x00\x74\x04\x79\x23\x01\x00\x01\x00\x01\x00\x74\x03\xa0\x05\x7c\x00\x6a\x01\x64\x01\xa1\x02\x01\x00\x64\x01\x7c\x00\x5f\x02\x59\x00\x64\x01\x53\x00\x77\x00\x25\x00",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[42];
    }
toplevel_consts_11_consts_14_consts_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 41,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "Reload the file data of the archive path.",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[2];
        }_object;
    }
toplevel_consts_11_consts_14_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 2,
        },
        .ob_item = {
            & toplevel_consts_11_consts_14_consts_0._ascii.ob_base,
            Py_None,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[4];
    }
toplevel_consts_11_consts_14_names_5 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 3,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "pop",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[6];
        }_object;
    }
toplevel_consts_11_consts_14_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 6,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_names_17._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_19._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_18._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_15._ascii.ob_base,
            & toplevel_consts_4._ascii.ob_base,
            & toplevel_consts_11_consts_14_names_5._ascii.ob_base,
        },
    },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[1];
        }_object;
    }
toplevel_consts_11_consts_14_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 1,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_varnames_0._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[18];
    }
toplevel_consts_11_consts_14_name = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 17,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "invalidate_caches",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[30];
    }
toplevel_consts_11_consts_14_qualname = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 29,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "zipimporter.invalidate_caches",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[19];
    }
toplevel_consts_11_consts_14_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 18,
    },
    .ob_shash = -1,
    .ob_sval = "\x02\x02\x0c\x01\x10\x01\x02\x80\x0c\x01\x0e\x01\x0c\x01\x02\xfe\x02\x80",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[21];
    }
toplevel_consts_11_consts_14_endlinetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 20,
    },
    .ob_shash = -1,
    .ob_sval = "\x02\x07\x0c\xfc\x10\x01\x02\x80\x02\x03\x02\xfe\x08\x02\x0e\xff\x0e\x01\x02\x80",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[75];
    }
toplevel_consts_11_consts_14_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 74,
    },
    .ob_shash = -1,
    .ob_sval = "\x09\x1f\x1b\x2a\x2b\x2f\x2b\x37\x1b\x38\x0d\x11\x0d\x18\x32\x36\x32\x3d\x0d\x21\x22\x26\x22\x2e\x0d\x2f\x0d\x2f\x0d\x2f\x00\x00\x09\x1f\x10\x1e\x09\x1f\x09\x1f\x09\x1f\x09\x1f\x0d\x21\x0d\x39\x26\x2a\x26\x32\x34\x38\x0d\x39\x0d\x39\x1b\x1f\x0d\x11\x0d\x18\x0d\x18\x0d\x18\x0d\x18\x09\x1f\x00\x00",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[13];
    }
toplevel_consts_11_consts_14_exceptiontable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 12,
    },
    .ob_shash = -1,
    .ob_sval = "\x81\x0c\x0f\x00\x8f\x11\x24\x07\xa3\x01\x24\x07",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[2];
    }
toplevel_consts_11_consts_14_localspluskinds = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 1,
    },
    .ob_shash = -1,
    .ob_sval = " ",
};
static struct PyCodeObject toplevel_consts_11_consts_14 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_11_consts_14_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_11_consts_14_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_11_consts_14_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_11_consts_14_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 1,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 8,
    .co_firstlineno = 330,
    .co_code = & toplevel_consts_11_consts_14_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_11_consts_14_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_11_consts_14_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_11_consts_14_name._ascii.ob_base,
    .co_qualname = & toplevel_consts_11_consts_14_qualname._ascii.ob_base,
    .co_linetable = & toplevel_consts_11_consts_14_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_11_consts_14_endlinetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_11_consts_14_columntable.ob_base.ob_base,
    .co_nlocalsplus = 1,
    .co_nlocals = 1,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_11_consts_14_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[25];
    }
toplevel_consts_11_consts_15_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 24,
    },
    .ob_shash = -1,
    .ob_sval = "\x64\x01\x7c\x00\x6a\x00\x9b\x00\x74\x01\x9b\x00\x7c\x00\x6a\x02\x9b\x00\x64\x02\x9d\x05\x53\x00",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[22];
    }
toplevel_consts_11_consts_15_consts_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 21,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "<zipimporter object \"",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[3];
    }
toplevel_consts_11_consts_15_consts_2 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 2,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "\">",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[3];
        }_object;
    }
toplevel_consts_11_consts_15_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 3,
        },
        .ob_item = {
            Py_None,
            & toplevel_consts_11_consts_15_consts_1._ascii.ob_base,
            & toplevel_consts_11_consts_15_consts_2._ascii.ob_base,
        },
    },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[3];
        }_object;
    }
toplevel_consts_11_consts_15_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 3,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_names_19._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_7._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_21._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[9];
    }
toplevel_consts_11_consts_15_name = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 8,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "__repr__",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[21];
    }
toplevel_consts_11_consts_15_qualname = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 20,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "zipimporter.__repr__",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[3];
    }
toplevel_consts_11_consts_15_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 2,
    },
    .ob_shash = -1,
    .ob_sval = "\x18\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[25];
    }
toplevel_consts_11_consts_15_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 24,
    },
    .ob_shash = -1,
    .ob_sval = "\x10\x4f\x28\x2c\x28\x34\x10\x4f\x36\x3e\x10\x4f\x40\x44\x40\x4b\x10\x4f\x10\x4f\x10\x4f\x09\x4f",
};
static struct PyCodeObject toplevel_consts_11_consts_15 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_11_consts_15_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_11_consts_15_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_11_consts_15_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_7_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 1,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 5,
    .co_firstlineno = 340,
    .co_code = & toplevel_consts_11_consts_15_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_11_consts_14_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_11_consts_14_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_11_consts_15_name._ascii.ob_base,
    .co_qualname = & toplevel_consts_11_consts_15_qualname._ascii.ob_base,
    .co_linetable = & toplevel_consts_11_consts_15_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_11_consts_15_linetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_11_consts_15_columntable.ob_base.ob_base,
    .co_nlocalsplus = 1,
    .co_nlocals = 1,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_11_consts_14_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[1];
        }_object;
    }
toplevel_consts_11_consts_16 = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 1,
        },
        .ob_item = {
            Py_None,
        },
    },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[17];
        }_object;
    }
toplevel_consts_11_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 17,
        },
        .ob_item = {
            & toplevel_consts_5._ascii.ob_base,
            & toplevel_consts_11_consts_1._ascii.ob_base,
            & toplevel_consts_11_consts_2.ob_base,
            Py_None,
            & toplevel_consts_11_consts_4.ob_base,
            & toplevel_consts_11_consts_5.ob_base,
            & toplevel_consts_11_consts_6.ob_base,
            & toplevel_consts_11_consts_7.ob_base,
            & toplevel_consts_11_consts_8.ob_base,
            & toplevel_consts_11_consts_9.ob_base,
            & toplevel_consts_11_consts_10.ob_base,
            & toplevel_consts_11_consts_11.ob_base,
            & toplevel_consts_11_consts_12.ob_base,
            & toplevel_consts_11_consts_13.ob_base,
            & toplevel_consts_11_consts_14.ob_base,
            & toplevel_consts_11_consts_15.ob_base,
            & toplevel_consts_11_consts_16._object.ob_base.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[8];
    }
toplevel_consts_11_names_3 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 7,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "__doc__",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[17];
        }_object;
    }
toplevel_consts_11_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 17,
        },
        .ob_item = {
            & toplevel_consts_7_names_0._ascii.ob_base,
            & toplevel_consts_7_names_1._ascii.ob_base,
            & toplevel_consts_7_names_2._ascii.ob_base,
            & toplevel_consts_11_names_3._ascii.ob_base,
            & toplevel_consts_11_consts_2_name._ascii.ob_base,
            & toplevel_consts_11_consts_4_name._ascii.ob_base,
            & toplevel_consts_11_consts_5_name._ascii.ob_base,
            & toplevel_consts_11_consts_6_name._ascii.ob_base,
            & toplevel_consts_11_consts_7_name._ascii.ob_base,
            & toplevel_consts_11_consts_8_name._ascii.ob_base,
            & toplevel_consts_11_consts_9_name._ascii.ob_base,
            & toplevel_consts_11_consts_10_name._ascii.ob_base,
            & toplevel_consts_11_consts_6_consts_2_0._ascii.ob_base,
            & toplevel_consts_11_consts_12_name._ascii.ob_base,
            & toplevel_consts_11_consts_13_name._ascii.ob_base,
            & toplevel_consts_11_consts_14_name._ascii.ob_base,
            & toplevel_consts_11_consts_15_name._ascii.ob_base,
        },
    },
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[31];
    }
toplevel_consts_11_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 30,
    },
    .ob_shash = -1,
    .ob_sval = "\x08\x00\x04\x01\x06\x11\x08\x2e\x08\x25\x08\x10\x06\x1b\x06\x0a\x06\x15\x06\x0c\x06\x1a\x06\x0d\x06\x2b\x06\x0f\x0a\x0a",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[39];
    }
toplevel_consts_11_endlinetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 38,
    },
    .ob_shash = -1,
    .ob_sval = "\x08\xd2\x02\x3b\x02\xc5\x06\x66\x02\x08\x06\x20\x02\x05\x06\x0e\x02\x02\x06\x19\x06\x09\x06\x14\x06\x0d\x06\x19\x06\x0d\x06\x2c\x06\x0f\x06\x0a\x0a\x04",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[101];
    }
toplevel_consts_11_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 100,
    },
    .ob_shash = -1,
    .ob_sval = "\x01\x01\x01\x01\x01\x01\x01\x01\x05\x08\x01\x01\x05\x24\x05\x24\x05\x24\x2a\x2e\x05\x18\x05\x18\x05\x18\x2a\x2e\x05\x33\x05\x33\x05\x33\x2a\x2e\x05\x1c\x05\x1c\x05\x1c\x05\x14\x05\x14\x05\x14\x05\x32\x05\x32\x05\x32\x05\x17\x05\x17\x05\x17\x05\x3b\x05\x3b\x05\x3b\x05\x12\x05\x12\x05\x12\x05\x13\x05\x13\x05\x13\x05\x29\x05\x29\x05\x29\x05\x1f\x05\x1f\x05\x1f\x05\x4f\x05\x4f\x05\x4f\x05\x4f\x05\x4f",
};
static struct PyCodeObject toplevel_consts_11 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_11_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_11_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_11_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_7_exceptiontable.ob_base.ob_base,
    .co_flags = 0,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 0,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 2,
    .co_firstlineno = 46,
    .co_code = & toplevel_consts_11_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_7_exceptiontable.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_5._ascii.ob_base,
    .co_qualname = & toplevel_consts_5._ascii.ob_base,
    .co_linetable = & toplevel_consts_11_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_11_endlinetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_11_columntable.ob_base.ob_base,
    .co_nlocalsplus = 0,
    .co_nlocals = 0,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[13];
    }
toplevel_consts_12 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 12,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "__init__.pyc",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[5];
    }
toplevel_consts_16_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 4,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = ".pyc",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[3];
        }_object;
    }
toplevel_consts_16 = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 3,
        },
        .ob_item = {
            & toplevel_consts_16_0._ascii.ob_base,
            Py_True,
            Py_False,
        },
    },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[3];
        }_object;
    }
toplevel_consts_17 = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 3,
        },
        .ob_item = {
            & toplevel_consts_11_consts_10_consts_5._ascii.ob_base,
            Py_False,
            Py_False,
        },
    },
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[21];
    }
toplevel_consts_18_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 20,
    },
    .ob_shash = -1,
    .ob_sval = "\x7c\x00\x6a\x00\x7c\x01\xa0\x01\x64\x01\xa1\x01\x64\x02\x19\x00\x17\x00\x53\x00",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[2];
    }
toplevel_consts_18_consts_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 1,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = ".",
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_18_consts_2 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 2 },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[3];
        }_object;
    }
toplevel_consts_18_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 3,
        },
        .ob_item = {
            Py_None,
            & toplevel_consts_18_consts_1._ascii.ob_base,
            & toplevel_consts_18_consts_2.ob_base.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[11];
    }
toplevel_consts_18_names_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 10,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "rpartition",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[2];
        }_object;
    }
toplevel_consts_18_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 2,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_names_21._ascii.ob_base,
            & toplevel_consts_18_names_1._ascii.ob_base,
        },
    },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[2];
        }_object;
    }
toplevel_consts_18_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 2,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_varnames_0._ascii.ob_base,
            & toplevel_consts_11_consts_4_varnames_1._ascii.ob_base,
        },
    },
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[3];
    }
toplevel_consts_18_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 2,
    },
    .ob_shash = -1,
    .ob_sval = "\x14\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[21];
    }
toplevel_consts_18_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 20,
    },
    .ob_shash = -1,
    .ob_sval = "\x0c\x10\x0c\x17\x1a\x22\x1a\x32\x2e\x31\x1a\x32\x33\x34\x1a\x35\x0c\x35\x05\x35",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[3];
    }
toplevel_consts_18_localspluskinds = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 2,
    },
    .ob_shash = -1,
    .ob_sval = "  ",
};
static struct PyCodeObject toplevel_consts_18 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_18_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_18_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_18_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_7_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 2,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 4,
    .co_firstlineno = 358,
    .co_code = & toplevel_consts_18_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_18_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_18_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_11_consts_4_names_4._ascii.ob_base,
    .co_qualname = & toplevel_consts_11_consts_4_names_4._ascii.ob_base,
    .co_linetable = & toplevel_consts_18_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_18_linetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_18_columntable.ob_base.ob_base,
    .co_nlocalsplus = 2,
    .co_nlocals = 2,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_18_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[19];
    }
toplevel_consts_19_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 18,
    },
    .ob_shash = -1,
    .ob_sval = "\x7c\x01\x74\x00\x17\x00\x7d\x02\x7c\x02\x7c\x00\x6a\x01\x76\x00\x53\x00",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[2];
        }_object;
    }
toplevel_consts_19_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 2,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_names_7._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_18._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[8];
    }
toplevel_consts_19_varnames_2 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 7,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "dirpath",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[3];
        }_object;
    }
toplevel_consts_19_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 3,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_varnames_0._ascii.ob_base,
            & toplevel_consts_11_consts_2_consts_3_0._ascii.ob_base,
            & toplevel_consts_19_varnames_2._ascii.ob_base,
        },
    },
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[5];
    }
toplevel_consts_19_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 4,
    },
    .ob_shash = -1,
    .ob_sval = "\x08\x04\x0a\x02",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[19];
    }
toplevel_consts_19_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 18,
    },
    .ob_shash = -1,
    .ob_sval = "\x0f\x13\x16\x1e\x0f\x1e\x05\x0c\x0c\x13\x17\x1b\x17\x22\x0c\x22\x05\x22",
};
static struct PyCodeObject toplevel_consts_19 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_11_consts_16._object.ob_base.ob_base,
    .co_names = & toplevel_consts_19_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_19_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_7_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 2,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 2,
    .co_firstlineno = 362,
    .co_code = & toplevel_consts_19_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_19_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_11_consts_5_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_11_consts_4_names_5._ascii.ob_base,
    .co_qualname = & toplevel_consts_11_consts_4_names_5._ascii.ob_base,
    .co_linetable = & toplevel_consts_19_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_19_linetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_19_columntable.ob_base.ob_base,
    .co_nlocalsplus = 3,
    .co_nlocals = 3,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_19_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[57];
    }
toplevel_consts_20_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 56,
    },
    .ob_shash = -1,
    .ob_sval = "\x74\x00\x7c\x00\x7c\x01\x83\x02\x7d\x02\x74\x01\x44\x00\x5d\x12\x5c\x03\x7d\x03\x7d\x04\x7d\x05\x7c\x02\x7c\x03\x17\x00\x7d\x06\x7c\x06\x7c\x00\x6a\x02\x76\x00\x72\x19\x7c\x05\x02\x00\x01\x00\x53\x00\x71\x07\x64\x00\x53\x00",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[17];
    }
toplevel_consts_20_names_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 16,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_zip_searchorder",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[3];
        }_object;
    }
toplevel_consts_20_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 3,
        },
        .ob_item = {
            & toplevel_consts_11_consts_4_names_4._ascii.ob_base,
            & toplevel_consts_20_names_1._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_18._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[7];
    }
toplevel_consts_20_varnames_3 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 6,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "suffix",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[11];
    }
toplevel_consts_20_varnames_4 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 10,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "isbytecode",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[7];
        }_object;
    }
toplevel_consts_20_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 7,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_varnames_0._ascii.ob_base,
            & toplevel_consts_11_consts_4_varnames_1._ascii.ob_base,
            & toplevel_consts_11_consts_2_consts_3_0._ascii.ob_base,
            & toplevel_consts_20_varnames_3._ascii.ob_base,
            & toplevel_consts_20_varnames_4._ascii.ob_base,
            & toplevel_consts_11_consts_7_varnames_3._ascii.ob_base,
            & toplevel_consts_11_consts_10_varnames_4._ascii.ob_base,
        },
    },
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[15];
    }
toplevel_consts_20_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 14,
    },
    .ob_shash = -1,
    .ob_sval = "\x0a\x01\x0e\x01\x08\x01\x0a\x01\x08\x01\x02\xff\x04\x02",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[17];
    }
toplevel_consts_20_endlinetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 16,
    },
    .ob_shash = -1,
    .ob_sval = "\x0a\x01\x02\x01\x04\x03\x08\xfd\x08\x01\x08\x01\x0c\x01\x04\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[57];
    }
toplevel_consts_20_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 56,
    },
    .ob_shash = -1,
    .ob_sval = "\x0c\x1c\x1d\x21\x23\x2b\x0c\x2c\x05\x09\x2a\x3a\x05\x1d\x05\x1d\x09\x26\x09\x0f\x11\x1b\x1d\x26\x14\x18\x1b\x21\x14\x21\x09\x11\x0c\x14\x18\x1c\x18\x23\x0c\x23\x09\x1d\x14\x1d\x0d\x1d\x0d\x1d\x0d\x1d\x09\x1d\x0c\x10\x0c\x10",
};
static struct PyCodeObject toplevel_consts_20 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_11_consts_16._object.ob_base.ob_base,
    .co_names = & toplevel_consts_20_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_20_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_7_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 2,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 4,
    .co_firstlineno = 371,
    .co_code = & toplevel_consts_20_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_20_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_11_consts_6_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_11_consts_4_names_3._ascii.ob_base,
    .co_qualname = & toplevel_consts_11_consts_4_names_3._ascii.ob_base,
    .co_linetable = & toplevel_consts_20_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_20_endlinetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_20_columntable.ob_base.ob_base,
    .co_nlocalsplus = 7,
    .co_nlocals = 7,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_20_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[1265];
    }
toplevel_consts_21_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 1264,
    },
    .ob_shash = -1,
    .ob_sval = "\x09\x00\x74\x00\x6a\x01\x7c\x00\x83\x01\x7d\x01\x6e\x12\x23\x00\x04\x00\x74\x02\x79\x17\x01\x00\x01\x00\x01\x00\x74\x03\x64\x01\x7c\x00\x9b\x02\x9d\x02\x7c\x00\x64\x02\x8d\x02\x82\x01\x77\x00\x25\x00\x7c\x01\x35\x00\x01\x00\x09\x00\x7c\x01\xa0\x04\x74\x05\x0b\x00\x64\x03\xa1\x02\x01\x00\x7c\x01\xa0\x06\xa1\x00\x7d\x02\x7c\x01\xa0\x07\x74\x05\xa1\x01\x7d\x03\x6e\x12\x23\x00\x04\x00\x74\x02\x79\x3e\x01\x00\x01\x00\x01\x00\x74\x03\x64\x04\x7c\x00\x9b\x02\x9d\x02\x7c\x00\x64\x02\x8d\x02\x82\x01\x77\x00\x25\x00\x74\x08\x7c\x03\x83\x01\x74\x05\x6b\x03\x72\x4f\x74\x03\x64\x04\x7c\x00\x9b\x02\x9d\x02\x7c\x00\x64\x02\x8d\x02\x82\x01\x7c\x03\x64\x00\x64\x05\x85\x02\x19\x00\x74\x09\x6b\x03\x72\xcc\x09\x00\x7c\x01\xa0\x04\x64\x06\x64\x03\xa1\x02\x01\x00\x7c\x01\xa0\x06\xa1\x00\x7d\x04\x6e\x12\x23\x00\x04\x00\x74\x02\x79\x73\x01\x00\x01\x00\x01\x00\x74\x03\x64\x04\x7c\x00\x9b\x02\x9d\x02\x7c\x00\x64\x02\x8d\x02\x82\x01\x77\x00\x25\x00\x74\x0a\x7c\x04\x74\x0b\x18\x00\x74\x05\x18\x00\x64\x06\x83\x02\x7d\x05\x09\x00\x7c\x01\xa0\x04\x7c\x05\xa1\x01\x01\x00\x7c\x01\xa0\x07\xa1\x00\x7d\x06\x6e\x12\x23\x00\x04\x00\x74\x02\x79\x99\x01\x00\x01\x00\x01\x00\x74\x03\x64\x04\x7c\x00\x9b\x02\x9d\x02\x7c\x00\x64\x02\x8d\x02\x82\x01\x77\x00\x25\x00\x7c\x06\xa0\x0c\x74\x09\xa1\x01\x7d\x07\x7c\x07\x64\x06\x6b\x00\x72\xad\x74\x03\x64\x07\x7c\x00\x9b\x02\x9d\x02\x7c\x00\x64\x02\x8d\x02\x82\x01\x7c\x06\x7c\x07\x7c\x07\x74\x05\x17\x00\x85\x02\x19\x00\x7d\x03\x74\x08\x7c\x03\x83\x01\x74\x05\x6b\x03\x72\xc4\x74\x03\x64\x08\x7c\x00\x9b\x02\x9d\x02\x7c\x00\x64\x02\x8d\x02\x82\x01\x7c\x04\x74\x08\x7c\x06\x83\x01\x18\x00\x7c\x07\x17\x00\x7d\x02\x74\x0d\x7c\x03\x64\x09\x64\x0a\x85\x02\x19\x00\x83\x01\x7d\x08\x74\x0d\x7c\x03\x64\x0a\x64\x0b\x85\x02\x19\x00\x83\x01\x7d\x09\x7c\x02\x7c\x08\x6b\x00\x72\xe9\x74\x03\x64\x0c\x7c\x00\x9b\x02\x9d\x02\x7c\x00\x64\x02\x8d\x02\x82\x01\x7c\x02\x7c\x09\x6b\x00\x72\xf6\x74\x03\x64\x0d\x7c\x00\x9b\x02\x9d\x02\x7c\x00\x64\x02\x8d\x02\x82\x01\x7c\x02\x7c\x08\x38\x00\x7d\x02\x7c\x02\x7c\x09\x18\x00\x7d\x0a\x7c\x0a\x64\x06\x6b\x00\x90\x01\x72\x0c\x74\x03\x64\x0e\x7c\x00\x9b\x02\x9d\x02\x7c\x00\x64\x02\x8d\x02\x82\x01\x69\x00\x7d\x0b\x64\x06\x7d\x0c\x09\x00\x7c\x01\xa0\x04\x7c\x02\xa1\x01\x01\x00\x6e\x13\x23\x00\x04\x00\x74\x02\x90\x01\x79\x28\x01\x00\x01\x00\x01\x00\x74\x03\x64\x04\x7c\x00\x9b\x02\x9d\x02\x7c\x00\x64\x02\x8d\x02\x82\x01\x77\x00\x25\x00\x09\x00\x7c\x01\xa0\x07\x64\x10\xa1\x01\x7d\x03\x74\x08\x7c\x03\x83\x01\x64\x05\x6b\x00\x90\x01\x72\x3b\x74\x0e\x64\x11\x83\x01\x82\x01\x7c\x03\x64\x00\x64\x05\x85\x02\x19\x00\x64\x12\x6b\x03\x90\x01\x72\x46\x90\x02\x71\x5c\x74\x08\x7c\x03\x83\x01\x64\x10\x6b\x03\x90\x01\x72\x51\x74\x0e\x64\x11\x83\x01\x82\x01\x74\x0f\x7c\x03\x64\x13\x64\x14\x85\x02\x19\x00\x83\x01\x7d\x0d\x74\x0f\x7c\x03\x64\x14\x64\x09\x85\x02\x19\x00\x83\x01\x7d\x0e\x74\x0f\x7c\x03\x64\x09\x64\x15\x85\x02\x19\x00\x83\x01\x7d\x0f\x74\x0f\x7c\x03\x64\x15\x64\x0a\x85\x02\x19\x00\x83\x01\x7d\x10\x74\x0d\x7c\x03\x64\x0a\x64\x0b\x85\x02\x19\x00\x83\x01\x7d\x11\x74\x0d\x7c\x03\x64\x0b\x64\x16\x85\x02\x19\x00\x83\x01\x7d\x12\x74\x0d\x7c\x03\x64\x16\x64\x17\x85\x02\x19\x00\x83\x01\x7d\x04\x74\x0f\x7c\x03\x64\x17\x64\x18\x85\x02\x19\x00\x83\x01\x7d\x13\x74\x0f\x7c\x03\x64\x18\x64\x19\x85\x02\x19\x00\x83\x01\x7d\x14\x74\x0f\x7c\x03\x64\x19\x64\x1a\x85\x02\x19\x00\x83\x01\x7d\x15\x74\x0d\x7c\x03\x64\x1b\x64\x10\x85\x02\x19\x00\x83\x01\x7d\x16\x7c\x13\x7c\x14\x17\x00\x7c\x15\x17\x00\x7d\x08\x7c\x16\x7c\x09\x6b\x04\x90\x01\x72\xbd\x74\x03\x64\x1c\x7c\x00\x9b\x02\x9d\x02\x7c\x00\x64\x02\x8d\x02\x82\x01\x7c\x16\x7c\x0a\x37\x00\x7d\x16\x09\x00\x7c\x01\xa0\x07\x7c\x13\xa1\x01\x7d\x17\x6e\x13\x23\x00\x04\x00\x74\x02\x90\x01\x79\xd9\x01\x00\x01\x00\x01\x00\x74\x03\x64\x04\x7c\x00\x9b\x02\x9d\x02\x7c\x00\x64\x02\x8d\x02\x82\x01\x77\x00\x25\x00\x74\x08\x7c\x17\x83\x01\x7c\x13\x6b\x03\x90\x01\x72\xeb\x74\x03\x64\x04\x7c\x00\x9b\x02\x9d\x02\x7c\x00\x64\x02\x8d\x02\x82\x01\x09\x00\x74\x08\x7c\x01\xa0\x07\x7c\x08\x7c\x13\x18\x00\xa1\x01\x83\x01\x7c\x08\x7c\x13\x18\x00\x6b\x03\x90\x02\x72\x03\x74\x03\x64\x04\x7c\x00\x9b\x02\x9d\x02\x7c\x00\x64\x02\x8d\x02\x82\x01\x6e\x13\x23\x00\x04\x00\x74\x02\x90\x02\x79\x15\x01\x00\x01\x00\x01\x00\x74\x03\x64\x04\x7c\x00\x9b\x02\x9d\x02\x7c\x00\x64\x02\x8d\x02\x82\x01\x77\x00\x25\x00\x7c\x0d\x64\x1d\x40\x00\x90\x02\x72\x21\x7c\x17\xa0\x10\xa1\x00\x7d\x17\x6e\x1b\x09\x00\x7c\x17\xa0\x10\x64\x1e\xa1\x01\x7d\x17\x6e\x14\x23\x00\x04\x00\x74\x11\x90\x02\x79\x3a\x01\x00\x01\x00\x01\x00\x7c\x17\xa0\x10\x64\x1f\xa1\x01\xa0\x12\x74\x13\xa1\x01\x7d\x17\x59\x00\x6e\x02\x77\x00\x25\x00\x7c\x17\xa0\x14\x64\x20\x74\x15\xa1\x02\x7d\x17\x74\x16\x6a\x17\x7c\x00\x7c\x17\x83\x02\x7d\x18\x7c\x18\x7c\x0e\x7c\x12\x7c\x04\x7c\x16\x7c\x0f\x7c\x10\x7c\x11\x66\x08\x7d\x19\x7c\x19\x7c\x0b\x7c\x17\x3c\x00\x7c\x0c\x64\x21\x37\x00\x7d\x0c\x90\x01\x71\x2b\x09\x00\x64\x00\x04\x00\x04\x00\x83\x03\x01\x00\x6e\x0c\x23\x00\x31\x00\x90\x02\x73\x69\x77\x04\x25\x00\x01\x00\x01\x00\x01\x00\x59\x00\x01\x00\x01\x00\x74\x18\x6a\x19\x64\x22\x7c\x0c\x7c\x00\x83\x03\x01\x00\x7c\x0b\x53\x00",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[22];
    }
toplevel_consts_21_consts_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 21,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "can't open Zip file: ",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[22];
    }
toplevel_consts_21_consts_4 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 21,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "can't read Zip file: ",
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_21_consts_5 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 4 },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[17];
    }
toplevel_consts_21_consts_7 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 16,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "not a Zip file: ",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[19];
    }
toplevel_consts_21_consts_8 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 18,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "corrupt Zip file: ",
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_21_consts_9 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 12 },
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_21_consts_10 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 16 },
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_21_consts_11 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 20 },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[29];
    }
toplevel_consts_21_consts_12 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 28,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "bad central directory size: ",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[31];
    }
toplevel_consts_21_consts_13 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 30,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "bad central directory offset: ",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[39];
    }
toplevel_consts_21_consts_14 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 38,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "bad central directory size or offset: ",
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_21_consts_16 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 46 },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[28];
    }
toplevel_consts_21_consts_17 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 27,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "EOF read where not expected",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[5];
    }
toplevel_consts_21_consts_18 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 4,
    },
    .ob_shash = -1,
    .ob_sval = "\x50\x4b\x01\x02",
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_21_consts_19 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 8 },
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_21_consts_20 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 10 },
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_21_consts_21 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 14 },
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_21_consts_22 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 24 },
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_21_consts_23 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 28 },
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_21_consts_24 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 30 },
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_21_consts_25 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 32 },
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_21_consts_26 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 34 },
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_21_consts_27 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 42 },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[26];
    }
toplevel_consts_21_consts_28 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 25,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "bad local header offset: ",
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_21_consts_29 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 2048 },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[6];
    }
toplevel_consts_21_consts_30 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 5,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "ascii",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[7];
    }
toplevel_consts_21_consts_31 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 6,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "latin1",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[2];
    }
toplevel_consts_21_consts_32 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 1,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "/",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[34];
    }
toplevel_consts_21_consts_34 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 33,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "zipimport: found {} names in {!r}",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[35];
        }_object;
    }
toplevel_consts_21_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 35,
        },
        .ob_item = {
            Py_None,
            & toplevel_consts_21_consts_1._ascii.ob_base,
            & toplevel_consts_11_consts_2_consts_3._object.ob_base.ob_base,
            & toplevel_consts_18_consts_2.ob_base.ob_base,
            & toplevel_consts_21_consts_4._ascii.ob_base,
            & toplevel_consts_21_consts_5.ob_base.ob_base,
            & toplevel_consts_1.ob_base.ob_base,
            & toplevel_consts_21_consts_7._ascii.ob_base,
            & toplevel_consts_21_consts_8._ascii.ob_base,
            & toplevel_consts_21_consts_9.ob_base.ob_base,
            & toplevel_consts_21_consts_10.ob_base.ob_base,
            & toplevel_consts_21_consts_11.ob_base.ob_base,
            & toplevel_consts_21_consts_12._ascii.ob_base,
            & toplevel_consts_21_consts_13._ascii.ob_base,
            & toplevel_consts_21_consts_14._ascii.ob_base,
            Py_True,
            & toplevel_consts_21_consts_16.ob_base.ob_base,
            & toplevel_consts_21_consts_17._ascii.ob_base,
            & toplevel_consts_21_consts_18.ob_base.ob_base,
            & toplevel_consts_21_consts_19.ob_base.ob_base,
            & toplevel_consts_21_consts_20.ob_base.ob_base,
            & toplevel_consts_21_consts_21.ob_base.ob_base,
            & toplevel_consts_21_consts_22.ob_base.ob_base,
            & toplevel_consts_21_consts_23.ob_base.ob_base,
            & toplevel_consts_21_consts_24.ob_base.ob_base,
            & toplevel_consts_21_consts_25.ob_base.ob_base,
            & toplevel_consts_21_consts_26.ob_base.ob_base,
            & toplevel_consts_21_consts_27.ob_base.ob_base,
            & toplevel_consts_21_consts_28._ascii.ob_base,
            & toplevel_consts_21_consts_29.ob_base.ob_base,
            & toplevel_consts_21_consts_30._ascii.ob_base,
            & toplevel_consts_21_consts_31._ascii.ob_base,
            & toplevel_consts_21_consts_32._ascii.ob_base,
            & toplevel_consts_6.ob_base.ob_base,
            & toplevel_consts_21_consts_34._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[4];
    }
toplevel_consts_21_names_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 3,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_io",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[10];
    }
toplevel_consts_21_names_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 9,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "open_code",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[5];
    }
toplevel_consts_21_names_4 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 4,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "seek",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[21];
    }
toplevel_consts_21_names_5 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 20,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "END_CENTRAL_DIR_SIZE",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[5];
    }
toplevel_consts_21_names_6 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 4,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "tell",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[5];
    }
toplevel_consts_21_names_7 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 4,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "read",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[19];
    }
toplevel_consts_21_names_9 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 18,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "STRING_END_ARCHIVE",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[4];
    }
toplevel_consts_21_names_10 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 3,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "max",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[16];
    }
toplevel_consts_21_names_11 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 15,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "MAX_COMMENT_LEN",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[6];
    }
toplevel_consts_21_names_12 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 5,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "rfind",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[9];
    }
toplevel_consts_21_names_14 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 8,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "EOFError",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[19];
    }
toplevel_consts_21_names_17 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 18,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "UnicodeDecodeError",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[10];
    }
toplevel_consts_21_names_18 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 9,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "translate",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[12];
    }
toplevel_consts_21_names_19 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 11,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "cp437_table",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[26];
        }_object;
    }
toplevel_consts_21_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 26,
        },
        .ob_item = {
            & toplevel_consts_21_names_0._ascii.ob_base,
            & toplevel_consts_21_names_1._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_10._ascii.ob_base,
            & toplevel_consts_4._ascii.ob_base,
            & toplevel_consts_21_names_4._ascii.ob_base,
            & toplevel_consts_21_names_5._ascii.ob_base,
            & toplevel_consts_21_names_6._ascii.ob_base,
            & toplevel_consts_21_names_7._ascii.ob_base,
            & toplevel_consts_11_consts_8_names_5._ascii.ob_base,
            & toplevel_consts_21_names_9._ascii.ob_base,
            & toplevel_consts_21_names_10._ascii.ob_base,
            & toplevel_consts_21_names_11._ascii.ob_base,
            & toplevel_consts_21_names_12._ascii.ob_base,
            & toplevel_consts_3_1._ascii.ob_base,
            & toplevel_consts_21_names_14._ascii.ob_base,
            & toplevel_consts_3_0._ascii.ob_base,
            & toplevel_consts_11_consts_10_names_9._ascii.ob_base,
            & toplevel_consts_21_names_17._ascii.ob_base,
            & toplevel_consts_21_names_18._ascii.ob_base,
            & toplevel_consts_21_names_19._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_6._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_7._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_8._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_20._ascii.ob_base,
            & toplevel_consts_11_consts_6_names_1._ascii.ob_base,
            & toplevel_consts_11_consts_12_names_23._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[3];
    }
toplevel_consts_21_varnames_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 2,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "fp",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[16];
    }
toplevel_consts_21_varnames_2 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 15,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "header_position",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[7];
    }
toplevel_consts_21_varnames_3 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 6,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "buffer",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[10];
    }
toplevel_consts_21_varnames_4 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 9,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "file_size",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[18];
    }
toplevel_consts_21_varnames_5 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 17,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "max_comment_start",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[5];
    }
toplevel_consts_21_varnames_6 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 4,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "data",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[4];
    }
toplevel_consts_21_varnames_7 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 3,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "pos",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[12];
    }
toplevel_consts_21_varnames_8 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 11,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "header_size",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[14];
    }
toplevel_consts_21_varnames_9 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 13,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "header_offset",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[11];
    }
toplevel_consts_21_varnames_10 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 10,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "arc_offset",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[6];
    }
toplevel_consts_21_varnames_12 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 5,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "count",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[6];
    }
toplevel_consts_21_varnames_13 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 5,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "flags",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[9];
    }
toplevel_consts_21_varnames_14 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 8,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "compress",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[5];
    }
toplevel_consts_21_varnames_15 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 4,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "time",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[5];
    }
toplevel_consts_21_varnames_16 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 4,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "date",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[4];
    }
toplevel_consts_21_varnames_17 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 3,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "crc",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[10];
    }
toplevel_consts_21_varnames_18 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 9,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "data_size",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[10];
    }
toplevel_consts_21_varnames_19 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 9,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "name_size",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[11];
    }
toplevel_consts_21_varnames_20 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 10,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "extra_size",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[13];
    }
toplevel_consts_21_varnames_21 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 12,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "comment_size",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[12];
    }
toplevel_consts_21_varnames_22 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 11,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "file_offset",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[2];
    }
toplevel_consts_21_varnames_25 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 1,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "t",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[26];
        }_object;
    }
toplevel_consts_21_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 26,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_names_19._ascii.ob_base,
            & toplevel_consts_21_varnames_1._ascii.ob_base,
            & toplevel_consts_21_varnames_2._ascii.ob_base,
            & toplevel_consts_21_varnames_3._ascii.ob_base,
            & toplevel_consts_21_varnames_4._ascii.ob_base,
            & toplevel_consts_21_varnames_5._ascii.ob_base,
            & toplevel_consts_21_varnames_6._ascii.ob_base,
            & toplevel_consts_21_varnames_7._ascii.ob_base,
            & toplevel_consts_21_varnames_8._ascii.ob_base,
            & toplevel_consts_21_varnames_9._ascii.ob_base,
            & toplevel_consts_21_varnames_10._ascii.ob_base,
            & toplevel_consts_11_consts_2_varnames_7._ascii.ob_base,
            & toplevel_consts_21_varnames_12._ascii.ob_base,
            & toplevel_consts_21_varnames_13._ascii.ob_base,
            & toplevel_consts_21_varnames_14._ascii.ob_base,
            & toplevel_consts_21_varnames_15._ascii.ob_base,
            & toplevel_consts_21_varnames_16._ascii.ob_base,
            & toplevel_consts_21_varnames_17._ascii.ob_base,
            & toplevel_consts_21_varnames_18._ascii.ob_base,
            & toplevel_consts_21_varnames_19._ascii.ob_base,
            & toplevel_consts_21_varnames_20._ascii.ob_base,
            & toplevel_consts_21_varnames_21._ascii.ob_base,
            & toplevel_consts_21_varnames_22._ascii.ob_base,
            & toplevel_consts_11_consts_6_consts_4_0._ascii.ob_base,
            & toplevel_consts_11_consts_2_consts_3_0._ascii.ob_base,
            & toplevel_consts_21_varnames_25._ascii.ob_base,
        },
    },
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[275];
    }
toplevel_consts_21_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 274,
    },
    .ob_shash = -1,
    .ob_sval = "\x02\x01\x0c\x01\x02\x80\x0c\x01\x12\x01\x02\xff\x02\x80\x06\x03\x02\x01\x0e\x01\x08\x01\x0c\x01\x02\x80\x0c\x01\x12\x01\x02\xff\x02\x80\x0c\x02\x12\x01\x10\x01\x02\x03\x0c\x01\x0a\x01\x02\x80\x0c\x01\x0a\x01\x02\x01\x06\xff\x02\xff\x02\x80\x08\x03\x02\x01\x02\xff\x02\x01\x04\xff\x02\x02\x0a\x01\x0a\x01\x02\x80\x0c\x01\x0a\x01\x02\x01\x06\xff\x02\xff\x02\x80\x0a\x03\x08\x01\x0a\x01\x02\x01\x06\xff\x10\x02\x0c\x01\x0a\x01\x02\x01\x06\xff\x10\x02\x10\x02\x10\x01\x08\x01\x12\x01\x08\x01\x12\x01\x08\x01\x08\x01\x0a\x01\x12\x01\x04\x02\x04\x02\x02\x01\x0c\x01\x02\x80\x0e\x01\x12\x01\x02\xff\x02\x80\x02\x02\x0a\x01\x0e\x01\x08\x01\x12\x02\x04\x01\x0e\x01\x08\x01\x10\x01\x10\x01\x10\x01\x10\x01\x10\x01\x10\x01\x10\x01\x10\x01\x10\x01\x10\x01\x10\x01\x0c\x01\x0a\x01\x12\x01\x08\x01\x02\x02\x0c\x01\x02\x80\x0e\x01\x12\x01\x02\xff\x02\x80\x0e\x02\x12\x01\x02\x04\x1c\x01\x12\x01\x02\xff\x02\x80\x0e\x02\x12\x01\x02\xff\x02\x80\x0a\x03\x0a\x02\x02\x03\x0c\x01\x02\x80\x0e\x01\x14\x01\x02\xff\x02\x80\x0c\x03\x0c\x01\x14\x01\x08\x01\x08\x01\x04\xca\x02\x06\x16\xc4\x02\x80\x0c\x00\x0e\x6d\x04\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[275];
    }
toplevel_consts_21_endlinetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 274,
    },
    .ob_shash = -1,
    .ob_sval = "\x02\x04\x0c\xfe\x02\x80\x02\x02\x02\xff\x1c\x01\x02\x80\x02\x02\x04\x6c\x02\x9a\x0e\xfc\x08\x01\x0c\x01\x02\x80\x02\x02\x02\xff\x1c\x01\x02\x80\x0a\x01\x14\x01\x0e\x01\x02\x19\x02\xef\x0c\xfc\x0a\x01\x02\x80\x02\x03\x02\xfe\x08\x02\x0a\xff\x0a\x01\x02\x80\x08\x01\x08\x01\x02\xff\x02\x07\x0a\xfc\x0a\x01\x02\x80\x02\x03\x02\xfe\x08\x02\x0a\xff\x0a\x01\x02\x80\x0a\x01\x06\x01\x02\x02\x0a\xff\x08\x01\x10\x01\x0a\x01\x02\x02\x0a\xff\x08\x01\x10\x01\x10\x02\x10\x01\x06\x01\x14\x01\x06\x01\x14\x01\x08\x01\x08\x01\x06\x01\x16\x01\x04\x02\x04\x02\x02\x04\x0c\xfe\x02\x80\x02\x02\x02\xff\x1e\x01\x02\x80\x02\x01\x0a\x01\x0a\x01\x0c\x01\x0e\x02\x08\x01\x0a\x01\x0c\x01\x10\x01\x10\x01\x10\x01\x10\x01\x10\x01\x10\x01\x10\x01\x10\x01\x10\x01\x10\x01\x10\x01\x0c\x01\x06\x01\x16\x01\x08\x01\x02\x05\x0c\xfe\x02\x80\x02\x02\x02\xff\x1e\x01\x02\x80\x0a\x01\x16\x01\x02\x08\x18\xfd\x18\x01\x02\x80\x02\x02\x02\xff\x1e\x01\x02\x80\x06\x02\x04\x08\x0a\xfa\x02\x06\x0c\xfe\x02\x80\x02\x02\x02\xff\x20\x01\x02\x80\x0c\x02\x0c\x01\x14\x01\x08\x01\x08\x01\x04\xca\x02\x06\x16\x30\x02\x80\x0c\x00\x0e\x01\x04\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[1265];
    }
toplevel_consts_21_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 1264,
    },
    .ob_shash = -1,
    .ob_sval = "\x05\x50\x0e\x11\x0e\x1b\x1c\x23\x0e\x24\x09\x0b\x09\x0b\x00\x00\x05\x50\x0c\x13\x05\x50\x05\x50\x05\x50\x05\x50\x0f\x1d\x1e\x41\x36\x3d\x1e\x41\x1e\x41\x48\x4f\x0f\x50\x0f\x50\x09\x50\x05\x50\x00\x00\x0a\x0c\x05\x17\x05\x17\x09\x54\x0d\x0f\x0d\x2e\x16\x2a\x15\x2a\x2c\x2d\x0d\x2e\x0d\x2e\x1f\x21\x1f\x28\x1f\x28\x0d\x1c\x16\x18\x16\x33\x1e\x32\x16\x33\x0d\x13\x0d\x13\x00\x00\x09\x54\x10\x17\x09\x54\x09\x54\x09\x54\x09\x54\x13\x21\x22\x45\x3a\x41\x22\x45\x22\x45\x4c\x53\x13\x54\x13\x54\x0d\x54\x09\x54\x00\x00\x0c\x0f\x10\x16\x0c\x17\x1b\x2f\x0c\x2f\x09\x54\x13\x21\x22\x45\x3a\x41\x22\x45\x22\x45\x4c\x53\x13\x54\x13\x54\x0d\x54\x0c\x12\x13\x15\x14\x15\x13\x15\x0c\x16\x1a\x2c\x0c\x2c\x09\x3a\x0d\x33\x11\x13\x11\x1e\x19\x1a\x1c\x1d\x11\x1e\x11\x1e\x1d\x1f\x1d\x26\x1d\x26\x11\x1a\x11\x1a\x00\x00\x0d\x33\x14\x1b\x0d\x33\x0d\x33\x0d\x33\x0d\x33\x17\x25\x26\x49\x3e\x45\x26\x49\x26\x49\x2b\x32\x17\x33\x17\x33\x11\x33\x0d\x33\x00\x00\x21\x24\x25\x2e\x31\x40\x25\x40\x25\x39\x25\x39\x3b\x3c\x21\x3d\x0d\x1e\x0d\x33\x11\x13\x11\x2b\x19\x2a\x11\x2b\x11\x2b\x18\x1a\x18\x21\x18\x21\x11\x15\x11\x15\x00\x00\x0d\x33\x14\x1b\x0d\x33\x0d\x33\x0d\x33\x0d\x33\x17\x25\x26\x49\x3e\x45\x26\x49\x26\x49\x2b\x32\x17\x33\x17\x33\x11\x33\x0d\x33\x00\x00\x13\x17\x13\x31\x1e\x30\x13\x31\x0d\x10\x10\x13\x16\x17\x10\x17\x0d\x33\x17\x25\x26\x44\x39\x40\x26\x44\x26\x44\x2b\x32\x17\x33\x17\x33\x11\x33\x16\x1a\x1b\x1e\x1f\x22\x23\x37\x1f\x37\x1b\x37\x16\x38\x0d\x13\x10\x13\x14\x1a\x10\x1b\x1f\x33\x10\x33\x0d\x33\x17\x25\x26\x46\x3b\x42\x26\x46\x26\x46\x2b\x32\x17\x33\x17\x33\x11\x33\x1f\x28\x2b\x2e\x2f\x33\x2b\x34\x1f\x34\x37\x3a\x1f\x3a\x0d\x1c\x17\x25\x26\x2c\x2d\x2f\x30\x32\x2d\x32\x26\x33\x17\x34\x09\x14\x19\x27\x28\x2e\x2f\x31\x32\x34\x2f\x34\x28\x35\x19\x36\x09\x16\x0c\x1b\x1e\x29\x0c\x29\x09\x5b\x13\x21\x22\x4c\x41\x48\x22\x4c\x22\x4c\x53\x5a\x13\x5b\x13\x5b\x0d\x5b\x0c\x1b\x1e\x2b\x0c\x2b\x09\x5d\x13\x21\x22\x4e\x43\x4a\x22\x4e\x22\x4e\x55\x5c\x13\x5d\x13\x5d\x0d\x5d\x09\x18\x1c\x27\x09\x27\x09\x18\x16\x25\x28\x35\x16\x35\x09\x13\x0c\x16\x19\x1a\x0c\x1a\x09\x65\x09\x65\x13\x21\x22\x56\x4b\x52\x22\x56\x22\x56\x5d\x64\x13\x65\x13\x65\x0d\x65\x11\x13\x09\x0e\x11\x12\x09\x0e\x09\x54\x0d\x0f\x0d\x25\x15\x24\x0d\x25\x0d\x25\x0d\x25\x00\x00\x09\x54\x10\x17\x09\x54\x09\x54\x09\x54\x09\x54\x09\x54\x13\x21\x22\x45\x3a\x41\x22\x45\x22\x45\x4c\x53\x13\x54\x13\x54\x0d\x54\x09\x54\x00\x00\x0f\x13\x16\x18\x16\x21\x1e\x20\x16\x21\x0d\x13\x10\x13\x14\x1a\x10\x1b\x1e\x1f\x10\x1f\x0d\x3e\x0d\x3e\x17\x1f\x20\x3d\x17\x3e\x11\x3e\x10\x16\x17\x19\x18\x19\x17\x19\x10\x1a\x1e\x2b\x10\x2b\x0d\x16\x0d\x16\x11\x16\x11\x16\x10\x13\x14\x1a\x10\x1b\x1f\x21\x10\x21\x0d\x3e\x0d\x3e\x17\x1f\x20\x3d\x17\x3e\x11\x3e\x15\x23\x24\x2a\x2b\x2c\x2d\x2f\x2b\x2f\x24\x30\x15\x31\x0d\x12\x18\x26\x27\x2d\x2e\x30\x31\x33\x2e\x33\x27\x34\x18\x35\x0d\x15\x14\x22\x23\x29\x2a\x2c\x2d\x2f\x2a\x2f\x23\x30\x14\x31\x0d\x11\x14\x22\x23\x29\x2a\x2c\x2d\x2f\x2a\x2f\x23\x30\x14\x31\x0d\x11\x13\x21\x22\x28\x29\x2b\x2c\x2e\x29\x2e\x22\x2f\x13\x30\x0d\x10\x19\x27\x28\x2e\x2f\x31\x32\x34\x2f\x34\x28\x35\x19\x36\x0d\x16\x19\x27\x28\x2e\x2f\x31\x32\x34\x2f\x34\x28\x35\x19\x36\x0d\x16\x19\x27\x28\x2e\x2f\x31\x32\x34\x2f\x34\x28\x35\x19\x36\x0d\x16\x1a\x28\x29\x2f\x30\x32\x33\x35\x30\x35\x29\x36\x1a\x37\x0d\x17\x1c\x2a\x2b\x31\x32\x34\x35\x37\x32\x37\x2b\x38\x1c\x39\x0d\x19\x1b\x29\x2a\x30\x31\x33\x34\x36\x31\x36\x2a\x37\x1b\x38\x0d\x18\x1b\x24\x27\x31\x1b\x31\x34\x40\x1b\x40\x0d\x18\x10\x1b\x1e\x2b\x10\x2b\x0d\x5c\x0d\x5c\x17\x25\x26\x4d\x42\x49\x26\x4d\x26\x4d\x54\x5b\x17\x5c\x17\x5c\x11\x5c\x0d\x18\x1c\x26\x0d\x26\x0d\x18\x0d\x58\x18\x1a\x18\x2a\x20\x29\x18\x2a\x11\x15\x11\x15\x00\x00\x0d\x58\x14\x1b\x0d\x58\x0d\x58\x0d\x58\x0d\x58\x0d\x58\x17\x25\x26\x49\x3e\x45\x26\x49\x26\x49\x50\x57\x17\x58\x17\x58\x11\x58\x0d\x58\x00\x00\x10\x13\x14\x18\x10\x19\x1d\x26\x10\x26\x0d\x58\x0d\x58\x17\x25\x26\x49\x3e\x45\x26\x49\x26\x49\x50\x57\x17\x58\x17\x58\x11\x58\x0d\x58\x14\x17\x18\x1a\x18\x38\x20\x2b\x2e\x37\x20\x37\x18\x38\x14\x39\x3d\x48\x4b\x54\x3d\x54\x14\x54\x11\x5c\x11\x5c\x1b\x29\x2a\x4d\x42\x49\x2a\x4d\x2a\x4d\x54\x5b\x1b\x5c\x1b\x5c\x15\x5c\x11\x5c\x00\x00\x0d\x58\x14\x1b\x0d\x58\x0d\x58\x0d\x58\x0d\x58\x0d\x58\x17\x25\x26\x49\x3e\x45\x26\x49\x26\x49\x50\x57\x17\x58\x17\x58\x11\x58\x0d\x58\x00\x00\x10\x15\x18\x1d\x10\x1d\x0d\x48\x0d\x48\x18\x1c\x18\x25\x18\x25\x11\x15\x11\x15\x11\x48\x1c\x20\x1c\x30\x28\x2f\x1c\x30\x15\x19\x15\x19\x00\x00\x11\x48\x18\x2a\x11\x48\x11\x48\x11\x48\x11\x48\x11\x48\x1c\x20\x1c\x31\x28\x30\x1c\x31\x1c\x48\x3c\x47\x1c\x48\x15\x19\x15\x19\x15\x19\x11\x48\x00\x00\x14\x18\x14\x2f\x21\x24\x26\x2e\x14\x2f\x0d\x11\x14\x27\x14\x32\x33\x3a\x3c\x40\x14\x41\x0d\x11\x12\x16\x18\x20\x22\x2b\x2d\x36\x38\x43\x45\x49\x4b\x4f\x51\x54\x11\x55\x0d\x0e\x1b\x1c\x0d\x12\x13\x17\x0d\x18\x0d\x12\x16\x17\x0d\x17\x0d\x12\x0f\x13\x0f\x13\x11\x16\x05\x17\x05\x17\x05\x17\x05\x17\x05\x17\x05\x17\x05\x17\x05\x17\x05\x17\x05\x17\x05\x17\x00\x00\x05\x17\x05\x17\x05\x17\x05\x17\x05\x17\x05\x17\x05\x0f\x05\x20\x21\x44\x46\x4b\x4d\x54\x05\x55\x05\x55\x0c\x11\x05\x11",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[202];
    }
toplevel_consts_21_exceptiontable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 201,
    },
    .ob_shash = -1,
    .ob_sval = "\x81\x05\x07\x00\x87\x11\x18\x07\x9b\x01\x49\x23\x03\x9d\x10\x2e\x02\xad\x01\x49\x23\x03\xae\x11\x3f\x09\xbf\x18\x49\x23\x03\xc1\x18\x0a\x41\x23\x02\xc1\x22\x01\x49\x23\x03\xc1\x23\x11\x41\x34\x09\xc1\x34\x0a\x49\x23\x03\xc1\x3f\x09\x42\x09\x02\xc2\x08\x01\x49\x23\x03\xc2\x09\x11\x42\x1a\x09\xc2\x1a\x41\x36\x49\x23\x03\xc4\x11\x05\x44\x17\x02\xc4\x16\x01\x49\x23\x03\xc4\x17\x12\x44\x29\x09\xc4\x29\x42\x18\x49\x23\x03\xc7\x02\x05\x47\x08\x02\xc7\x07\x01\x49\x23\x03\xc7\x08\x12\x47\x1a\x09\xc7\x1a\x11\x49\x23\x03\xc7\x2c\x17\x48\x04\x02\xc8\x03\x01\x49\x23\x03\xc8\x04\x12\x48\x16\x09\xc8\x16\x0b\x49\x23\x03\xc8\x22\x05\x48\x28\x02\xc8\x27\x01\x49\x23\x03\xc8\x28\x10\x48\x3b\x09\xc8\x38\x02\x49\x23\x03\xc8\x3a\x01\x48\x3b\x09\xc8\x3b\x21\x49\x23\x03\xc9\x23\x05\x49\x28\x0b\xc9\x29\x03\x49\x28\x0b",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[27];
    }
toplevel_consts_21_localspluskinds = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 26,
    },
    .ob_shash = -1,
    .ob_sval = "                          ",
};
static struct PyCodeObject toplevel_consts_21 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_21_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_21_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_21_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_21_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 1,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 9,
    .co_firstlineno = 402,
    .co_code = & toplevel_consts_21_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_21_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_21_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_11_consts_2_names_17._ascii.ob_base,
    .co_qualname = & toplevel_consts_11_consts_2_names_17._ascii.ob_base,
    .co_linetable = & toplevel_consts_21_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_21_endlinetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_21_columntable.ob_base.ob_base,
    .co_nlocalsplus = 26,
    .co_nlocals = 26,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_21_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyCompactUnicodeObject _compact;
        uint16_t _data[257];
    }
toplevel_consts_22 = {
    ._compact = {
        ._base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyUnicode_Type,
            },
            .length = 256,
            .hash = -1,
            .state = {
                .kind = 2,
                .compact = 1,
                .ascii = 0,
                .ready = 1,
            },
        },
    },
    ._data = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
        32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
        64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
        80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
        96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
        112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
        199, 252, 233, 226, 228, 224, 229, 231, 234, 235, 232, 239, 238, 236, 196, 197,
        201, 230, 198, 244, 246, 242, 251, 249, 255, 214, 220, 162, 163, 165, 8359, 402,
        225, 237, 243, 250, 241, 209, 170, 186, 191, 8976, 172, 189, 188, 161, 171, 187,
        9617, 9618, 9619, 9474, 9508, 9569, 9570, 9558, 9557, 9571, 9553, 9559, 9565, 9564, 9563, 9488,
        9492, 9524, 9516, 9500, 9472, 9532, 9566, 9567, 9562, 9556, 9577, 9574, 9568, 9552, 9580, 9575,
        9576, 9572, 9573, 9561, 9560, 9554, 9555, 9579, 9578, 9496, 9484, 9608, 9604, 9612, 9616, 9600,
        945, 223, 915, 960, 931, 963, 181, 964, 934, 920, 937, 948, 8734, 966, 949, 8745,
        8801, 177, 8805, 8804, 8992, 8993, 247, 8776, 176, 8729, 183, 8730, 8319, 178, 9632, 160,
    },
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[111];
    }
toplevel_consts_23_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 110,
    },
    .ob_shash = -1,
    .ob_sval = "\x74\x00\x72\x0b\x74\x01\x6a\x02\x64\x01\x83\x01\x01\x00\x74\x03\x64\x02\x83\x01\x82\x01\x64\x03\x61\x00\x09\x00\x64\x04\x64\x05\x6c\x04\x6d\x05\x7d\x00\x01\x00\x6e\x12\x23\x00\x04\x00\x74\x06\x79\x25\x01\x00\x01\x00\x01\x00\x74\x01\x6a\x02\x64\x01\x83\x01\x01\x00\x74\x03\x64\x02\x83\x01\x82\x01\x77\x00\x25\x00\x09\x00\x64\x06\x61\x00\x6e\x05\x23\x00\x64\x06\x61\x00\x77\x00\x25\x00\x74\x01\x6a\x02\x64\x07\x83\x01\x01\x00\x7c\x00\x53\x00",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[28];
    }
toplevel_consts_23_consts_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 27,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "zipimport: zlib UNAVAILABLE",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[42];
    }
toplevel_consts_23_consts_2 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 41,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "can't decompress data; zlib not available",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[11];
    }
toplevel_consts_23_consts_5_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 10,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "decompress",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[1];
        }_object;
    }
toplevel_consts_23_consts_5 = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 1,
        },
        .ob_item = {
            & toplevel_consts_23_consts_5_0._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[26];
    }
toplevel_consts_23_consts_7 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 25,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "zipimport: zlib available",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[8];
        }_object;
    }
toplevel_consts_23_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 8,
        },
        .ob_item = {
            Py_None,
            & toplevel_consts_23_consts_1._ascii.ob_base,
            & toplevel_consts_23_consts_2._ascii.ob_base,
            Py_True,
            & toplevel_consts_1.ob_base.ob_base,
            & toplevel_consts_23_consts_5._object.ob_base.ob_base,
            Py_False,
            & toplevel_consts_23_consts_7._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[16];
    }
toplevel_consts_23_names_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 15,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_importing_zlib",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[5];
    }
toplevel_consts_23_names_4 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 4,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "zlib",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[10];
    }
toplevel_consts_23_names_6 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 9,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "Exception",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[7];
        }_object;
    }
toplevel_consts_23_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 7,
        },
        .ob_item = {
            & toplevel_consts_23_names_0._ascii.ob_base,
            & toplevel_consts_11_consts_6_names_1._ascii.ob_base,
            & toplevel_consts_11_consts_12_names_23._ascii.ob_base,
            & toplevel_consts_4._ascii.ob_base,
            & toplevel_consts_23_names_4._ascii.ob_base,
            & toplevel_consts_23_consts_5_0._ascii.ob_base,
            & toplevel_consts_23_names_6._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[21];
    }
toplevel_consts_23_name = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 20,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_get_decompress_func",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[37];
    }
toplevel_consts_23_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 36,
    },
    .ob_shash = -1,
    .ob_sval = "\x04\x02\x0a\x03\x08\x01\x04\x02\x02\x01\x0e\x01\x02\x80\x0c\x01\x0a\x01\x08\x01\x02\xfe\x02\x80\x02\xff\x06\x05\x02\x80\x08\x00\x0a\x02\x04\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[41];
    }
toplevel_consts_23_endlinetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 40,
    },
    .ob_shash = -1,
    .ob_sval = "\x02\x02\x02\x04\x0a\xff\x08\x01\x04\x02\x02\x07\x0e\xfb\x02\x80\x02\x03\x02\xfe\x08\x02\x0a\xff\x0a\x01\x02\x80\x02\xfd\x06\x05\x02\x80\x08\x00\x0a\x02\x04\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[111];
    }
toplevel_consts_23_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 110,
    },
    .ob_shash = -1,
    .ob_sval = "\x08\x17\x05\x4a\x09\x13\x09\x24\x25\x42\x09\x43\x09\x43\x0f\x1d\x1e\x49\x0f\x4a\x09\x4a\x17\x1b\x05\x14\x05\x20\x09\x24\x09\x24\x09\x24\x09\x24\x09\x24\x09\x24\x09\x24\x00\x00\x05\x4a\x0c\x15\x05\x4a\x05\x4a\x05\x4a\x05\x4a\x09\x13\x09\x24\x25\x42\x09\x43\x09\x43\x0f\x1d\x1e\x49\x0f\x4a\x09\x4a\x05\x4a\x00\x00\x09\x24\x1b\x20\x09\x18\x09\x18\x00\x00\x1b\x20\x09\x18\x09\x20\x09\x20\x05\x0f\x05\x20\x21\x3c\x05\x3d\x05\x3d\x0c\x16\x05\x16",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[21];
    }
toplevel_consts_23_exceptiontable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 20,
    },
    .ob_shash = -1,
    .ob_sval = "\x8e\x06\x15\x00\x94\x01\x2b\x00\x95\x11\x26\x07\xa6\x01\x2b\x00\xab\x04\x2f\x07",
};
static struct PyCodeObject toplevel_consts_23 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_23_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_23_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_23_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_23_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 0,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 8,
    .co_firstlineno = 560,
    .co_code = & toplevel_consts_23_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_23_consts_5._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_11_consts_14_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_23_name._ascii.ob_base,
    .co_qualname = & toplevel_consts_23_name._ascii.ob_base,
    .co_linetable = & toplevel_consts_23_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_23_endlinetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_23_columntable.ob_base.ob_base,
    .co_nlocalsplus = 1,
    .co_nlocals = 1,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_23_consts_5._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[389];
    }
toplevel_consts_24_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 388,
    },
    .ob_shash = -1,
    .ob_sval = "\x7c\x01\x5c\x08\x7d\x02\x7d\x03\x7d\x04\x7d\x05\x7d\x06\x7d\x07\x7d\x08\x7d\x09\x7c\x04\x64\x01\x6b\x00\x72\x12\x74\x00\x64\x02\x83\x01\x82\x01\x74\x01\x6a\x02\x7c\x00\x83\x01\x35\x00\x7d\x0a\x09\x00\x7c\x0a\xa0\x03\x7c\x06\xa1\x01\x01\x00\x6e\x12\x23\x00\x04\x00\x74\x04\x79\x2f\x01\x00\x01\x00\x01\x00\x74\x00\x64\x03\x7c\x00\x9b\x02\x9d\x02\x7c\x00\x64\x04\x8d\x02\x82\x01\x77\x00\x25\x00\x7c\x0a\xa0\x05\x64\x05\xa1\x01\x7d\x0b\x74\x06\x7c\x0b\x83\x01\x64\x05\x6b\x03\x72\x40\x74\x07\x64\x06\x83\x01\x82\x01\x7c\x0b\x64\x00\x64\x07\x85\x02\x19\x00\x64\x08\x6b\x03\x72\x51\x74\x00\x64\x09\x7c\x00\x9b\x02\x9d\x02\x7c\x00\x64\x04\x8d\x02\x82\x01\x74\x08\x7c\x0b\x64\x0a\x64\x0b\x85\x02\x19\x00\x83\x01\x7d\x0c\x74\x08\x7c\x0b\x64\x0b\x64\x05\x85\x02\x19\x00\x83\x01\x7d\x0d\x64\x05\x7c\x0c\x17\x00\x7c\x0d\x17\x00\x7d\x0e\x7c\x06\x7c\x0e\x37\x00\x7d\x06\x09\x00\x7c\x0a\xa0\x03\x7c\x06\xa1\x01\x01\x00\x6e\x12\x23\x00\x04\x00\x74\x04\x79\x82\x01\x00\x01\x00\x01\x00\x74\x00\x64\x03\x7c\x00\x9b\x02\x9d\x02\x7c\x00\x64\x04\x8d\x02\x82\x01\x77\x00\x25\x00\x7c\x0a\xa0\x05\x7c\x04\xa1\x01\x7d\x0f\x74\x06\x7c\x0f\x83\x01\x7c\x04\x6b\x03\x72\x93\x74\x04\x64\x0c\x83\x01\x82\x01\x09\x00\x64\x00\x04\x00\x04\x00\x83\x03\x01\x00\x6e\x0b\x23\x00\x31\x00\x73\x9f\x77\x04\x25\x00\x01\x00\x01\x00\x01\x00\x59\x00\x01\x00\x01\x00\x7c\x03\x64\x01\x6b\x02\x72\xab\x7c\x0f\x53\x00\x09\x00\x74\x09\x83\x00\x7d\x10\x6e\x0d\x23\x00\x04\x00\x74\x0a\x79\xbb\x01\x00\x01\x00\x01\x00\x74\x00\x64\x0d\x83\x01\x82\x01\x77\x00\x25\x00\x7c\x10\x7c\x0f\x64\x0e\x83\x02\x53\x00",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[19];
    }
toplevel_consts_24_consts_2 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 18,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "negative data size",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[5];
    }
toplevel_consts_24_consts_8 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 4,
    },
    .ob_shash = -1,
    .ob_sval = "\x50\x4b\x03\x04",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[24];
    }
toplevel_consts_24_consts_9 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 23,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "bad local file header: ",
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_24_consts_10 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 26 },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[27];
    }
toplevel_consts_24_consts_12 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 26,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "zipimport: can't read data",
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_24_consts_14 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = -1,
    },
    .ob_digit = { 15 },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[15];
        }_object;
    }
toplevel_consts_24_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 15,
        },
        .ob_item = {
            Py_None,
            & toplevel_consts_1.ob_base.ob_base,
            & toplevel_consts_24_consts_2._ascii.ob_base,
            & toplevel_consts_21_consts_4._ascii.ob_base,
            & toplevel_consts_11_consts_2_consts_3._object.ob_base.ob_base,
            & toplevel_consts_21_consts_24.ob_base.ob_base,
            & toplevel_consts_21_consts_17._ascii.ob_base,
            & toplevel_consts_21_consts_5.ob_base.ob_base,
            & toplevel_consts_24_consts_8.ob_base.ob_base,
            & toplevel_consts_24_consts_9._ascii.ob_base,
            & toplevel_consts_24_consts_10.ob_base.ob_base,
            & toplevel_consts_21_consts_23.ob_base.ob_base,
            & toplevel_consts_24_consts_12._ascii.ob_base,
            & toplevel_consts_23_consts_2._ascii.ob_base,
            & toplevel_consts_24_consts_14.ob_base.ob_base,
        },
    },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[11];
        }_object;
    }
toplevel_consts_24_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 11,
        },
        .ob_item = {
            & toplevel_consts_4._ascii.ob_base,
            & toplevel_consts_21_names_0._ascii.ob_base,
            & toplevel_consts_21_names_1._ascii.ob_base,
            & toplevel_consts_21_names_4._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_10._ascii.ob_base,
            & toplevel_consts_21_names_7._ascii.ob_base,
            & toplevel_consts_11_consts_8_names_5._ascii.ob_base,
            & toplevel_consts_21_names_14._ascii.ob_base,
            & toplevel_consts_3_0._ascii.ob_base,
            & toplevel_consts_23_name._ascii.ob_base,
            & toplevel_consts_23_names_6._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[9];
    }
toplevel_consts_24_varnames_2 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 8,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "datapath",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[9];
    }
toplevel_consts_24_varnames_15 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 8,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "raw_data",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[17];
        }_object;
    }
toplevel_consts_24_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 17,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_names_19._ascii.ob_base,
            & toplevel_consts_11_consts_8_varnames_3._ascii.ob_base,
            & toplevel_consts_24_varnames_2._ascii.ob_base,
            & toplevel_consts_21_varnames_14._ascii.ob_base,
            & toplevel_consts_21_varnames_18._ascii.ob_base,
            & toplevel_consts_21_varnames_4._ascii.ob_base,
            & toplevel_consts_21_varnames_22._ascii.ob_base,
            & toplevel_consts_21_varnames_15._ascii.ob_base,
            & toplevel_consts_21_varnames_16._ascii.ob_base,
            & toplevel_consts_21_varnames_17._ascii.ob_base,
            & toplevel_consts_21_varnames_1._ascii.ob_base,
            & toplevel_consts_21_varnames_3._ascii.ob_base,
            & toplevel_consts_21_varnames_19._ascii.ob_base,
            & toplevel_consts_21_varnames_20._ascii.ob_base,
            & toplevel_consts_21_varnames_8._ascii.ob_base,
            & toplevel_consts_24_varnames_15._ascii.ob_base,
            & toplevel_consts_23_consts_5_0._ascii.ob_base,
        },
    },
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[89];
    }
toplevel_consts_24_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 88,
    },
    .ob_shash = -1,
    .ob_sval = "\x14\x01\x08\x01\x08\x01\x0c\x02\x02\x02\x0c\x01\x02\x80\x0c\x01\x12\x01\x02\xff\x02\x80\x0a\x02\x0c\x01\x08\x01\x10\x02\x12\x02\x10\x02\x10\x01\x0c\x01\x08\x01\x02\x01\x0c\x01\x02\x80\x0c\x01\x12\x01\x02\xff\x02\x80\x0a\x02\x0c\x01\x08\x01\x02\xff\x14\xe9\x02\x80\x0c\x00\x08\x1a\x04\x02\x02\x03\x08\x01\x02\x80\x0c\x01\x08\x01\x02\xff\x02\x80\x0a\x02",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[89];
    }
toplevel_consts_24_endlinetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 88,
    },
    .ob_shash = -1,
    .ob_sval = "\x14\x01\x06\x01\x0a\x01\x08\x02\x02\x18\x02\xe8\x02\x05\x0c\xfe\x02\x80\x02\x02\x02\xff\x1c\x01\x02\x80\x0a\x01\x0a\x01\x0a\x01\x0e\x02\x14\x02\x10\x02\x10\x01\x0c\x01\x08\x01\x02\x04\x0c\xfe\x02\x80\x02\x02\x02\xff\x1c\x01\x02\x80\x0a\x01\x0a\x01\x20\x01\x02\x80\x0c\x00\x06\x02\x06\x02\x02\x06\x08\xfe\x02\x80\x02\x02\x02\xff\x12\x01\x02\x80\x0a\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[389];
    }
toplevel_consts_24_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 388,
    },
    .ob_shash = -1,
    .ob_sval = "\x4e\x57\x05\x4b\x05\x0d\x0f\x17\x19\x22\x24\x2d\x2f\x3a\x3c\x40\x42\x46\x48\x4b\x08\x11\x14\x15\x08\x15\x05\x33\x0f\x1d\x1e\x32\x0f\x33\x09\x33\x0a\x0d\x0a\x17\x18\x1f\x0a\x20\x05\x38\x24\x26\x09\x54\x0d\x0f\x0d\x21\x15\x20\x0d\x21\x0d\x21\x0d\x21\x00\x00\x09\x54\x10\x17\x09\x54\x09\x54\x09\x54\x09\x54\x13\x21\x22\x45\x3a\x41\x22\x45\x22\x45\x4c\x53\x13\x54\x13\x54\x0d\x54\x09\x54\x00\x00\x12\x14\x12\x1d\x1a\x1c\x12\x1d\x09\x0f\x0c\x0f\x10\x16\x0c\x17\x1b\x1d\x0c\x1d\x09\x3a\x13\x1b\x1c\x39\x13\x3a\x0d\x3a\x0c\x12\x13\x15\x14\x15\x13\x15\x0c\x16\x1a\x27\x0c\x27\x09\x56\x13\x21\x22\x47\x3c\x43\x22\x47\x22\x47\x4e\x55\x13\x56\x13\x56\x0d\x56\x15\x23\x24\x2a\x2b\x2d\x2e\x30\x2b\x30\x24\x31\x15\x32\x09\x12\x16\x24\x25\x2b\x2c\x2e\x2f\x31\x2c\x31\x25\x32\x16\x33\x09\x13\x17\x19\x1c\x25\x17\x25\x28\x32\x17\x32\x09\x14\x09\x14\x18\x23\x09\x23\x09\x14\x09\x54\x0d\x0f\x0d\x21\x15\x20\x0d\x21\x0d\x21\x0d\x21\x00\x00\x09\x54\x10\x17\x09\x54\x09\x54\x09\x54\x09\x54\x13\x21\x22\x45\x3a\x41\x22\x45\x22\x45\x4c\x53\x13\x54\x13\x54\x0d\x54\x09\x54\x00\x00\x14\x16\x14\x26\x1c\x25\x14\x26\x09\x11\x0c\x0f\x10\x18\x0c\x19\x1d\x26\x0c\x26\x09\x38\x13\x1a\x1b\x37\x13\x38\x0d\x38\x09\x38\x05\x38\x05\x38\x05\x38\x05\x38\x05\x38\x05\x38\x05\x38\x05\x38\x05\x38\x05\x38\x00\x00\x05\x38\x05\x38\x05\x38\x05\x38\x05\x38\x05\x38\x08\x10\x14\x15\x08\x15\x05\x18\x10\x18\x09\x18\x05\x4a\x16\x2a\x16\x2c\x09\x13\x09\x13\x00\x00\x05\x4a\x0c\x15\x05\x4a\x05\x4a\x05\x4a\x05\x4a\x0f\x1d\x1e\x49\x0f\x4a\x09\x4a\x05\x4a\x00\x00\x0c\x16\x17\x1f\x21\x24\x0c\x25\x05\x25",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[72];
    }
toplevel_consts_24_exceptiontable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 71,
    },
    .ob_shash = -1,
    .ob_sval = "\x97\x01\x42\x1a\x03\x99\x05\x1f\x02\x9e\x01\x42\x1a\x03\x9f\x11\x30\x09\xb0\x3b\x42\x1a\x03\xc1\x2c\x05\x41\x32\x02\xc1\x31\x01\x42\x1a\x03\xc1\x32\x11\x42\x03\x09\xc2\x03\x10\x42\x1a\x03\xc2\x1a\x04\x42\x1e\x0b\xc2\x1f\x03\x42\x1e\x0b\xc2\x2c\x03\x42\x30\x00\xc2\x30\x0c\x42\x3c\x07",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[18];
    }
toplevel_consts_24_localspluskinds = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 17,
    },
    .ob_shash = -1,
    .ob_sval = "                 ",
};
static struct PyCodeObject toplevel_consts_24 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_24_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_24_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_24_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_24_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 2,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 9,
    .co_firstlineno = 581,
    .co_code = & toplevel_consts_24_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_24_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_24_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_11_consts_8_names_9._ascii.ob_base,
    .co_qualname = & toplevel_consts_11_consts_8_names_9._ascii.ob_base,
    .co_linetable = & toplevel_consts_24_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_24_endlinetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_24_columntable.ob_base.ob_base,
    .co_nlocalsplus = 17,
    .co_nlocals = 17,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_24_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[17];
    }
toplevel_consts_25_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 16,
    },
    .ob_shash = -1,
    .ob_sval = "\x74\x00\x7c\x00\x7c\x01\x18\x00\x83\x01\x64\x01\x6b\x01\x53\x00",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[2];
        }_object;
    }
toplevel_consts_25_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 2,
        },
        .ob_item = {
            Py_None,
            & toplevel_consts_6.ob_base.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[4];
    }
toplevel_consts_25_names_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 3,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "abs",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[1];
        }_object;
    }
toplevel_consts_25_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 1,
        },
        .ob_item = {
            & toplevel_consts_25_names_0._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[3];
    }
toplevel_consts_25_varnames_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 2,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "t1",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[3];
    }
toplevel_consts_25_varnames_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 2,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "t2",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[2];
        }_object;
    }
toplevel_consts_25_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 2,
        },
        .ob_item = {
            & toplevel_consts_25_varnames_0._ascii.ob_base,
            & toplevel_consts_25_varnames_1._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[10];
    }
toplevel_consts_25_name = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 9,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_eq_mtime",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[3];
    }
toplevel_consts_25_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 2,
    },
    .ob_shash = -1,
    .ob_sval = "\x10\x02",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[17];
    }
toplevel_consts_25_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 16,
    },
    .ob_shash = -1,
    .ob_sval = "\x0c\x0f\x10\x12\x15\x17\x10\x17\x0c\x18\x1c\x1d\x0c\x1d\x05\x1d",
};
static struct PyCodeObject toplevel_consts_25 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_25_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_25_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_25_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_7_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 2,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 3,
    .co_firstlineno = 627,
    .co_code = & toplevel_consts_25_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_25_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_18_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_25_name._ascii.ob_base,
    .co_qualname = & toplevel_consts_25_name._ascii.ob_base,
    .co_linetable = & toplevel_consts_25_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_25_linetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_25_columntable.ob_base.ob_base,
    .co_nlocalsplus = 2,
    .co_nlocals = 2,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_25_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[255];
    }
toplevel_consts_26_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 254,
    },
    .ob_shash = -1,
    .ob_sval = "\x7c\x03\x7c\x02\x64\x01\x9c\x02\x7d\x05\x74\x00\x6a\x01\x7c\x04\x7c\x03\x7c\x05\x83\x03\x7d\x06\x7c\x06\x64\x02\x40\x00\x64\x03\x6b\x03\x7d\x07\x7c\x07\x72\x3f\x7c\x06\x64\x04\x40\x00\x64\x03\x6b\x03\x7d\x08\x74\x02\x6a\x03\x64\x05\x6b\x03\x72\x3e\x7c\x08\x73\x26\x74\x02\x6a\x03\x64\x06\x6b\x02\x72\x3e\x74\x04\x7c\x00\x7c\x02\x83\x02\x7d\x09\x7c\x09\x64\x00\x75\x01\x72\x3e\x74\x02\x6a\x05\x74\x00\x6a\x06\x7c\x09\x83\x02\x7d\x0a\x74\x00\x6a\x07\x7c\x04\x7c\x0a\x7c\x03\x7c\x05\x83\x04\x01\x00\x6e\x28\x74\x08\x7c\x00\x7c\x02\x83\x02\x5c\x02\x7d\x0b\x7d\x0c\x7c\x0b\x72\x67\x74\x09\x74\x0a\x7c\x04\x64\x07\x64\x08\x85\x02\x19\x00\x83\x01\x7c\x0b\x83\x02\x72\x5d\x74\x0a\x7c\x04\x64\x08\x64\x09\x85\x02\x19\x00\x83\x01\x7c\x0c\x6b\x03\x72\x67\x74\x0b\x6a\x0c\x64\x0a\x7c\x03\x9b\x02\x9d\x02\x83\x01\x01\x00\x64\x00\x53\x00\x74\x0d\x6a\x0e\x7c\x04\x64\x09\x64\x00\x85\x02\x19\x00\x83\x01\x7d\x0d\x74\x0f\x7c\x0d\x74\x10\x83\x02\x73\x7d\x74\x11\x64\x0b\x7c\x01\x9b\x02\x64\x0c\x9d\x03\x83\x01\x82\x01\x7c\x0d\x53\x00",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[2];
        }_object;
    }
toplevel_consts_26_consts_1 = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 2,
        },
        .ob_item = {
            & toplevel_consts_11_consts_6_consts_4_0._ascii.ob_base,
            & toplevel_consts_11_consts_2_consts_3_0._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[6];
    }
toplevel_consts_26_consts_5 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 5,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "never",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[7];
    }
toplevel_consts_26_consts_6 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 6,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "always",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[23];
    }
toplevel_consts_26_consts_10 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 22,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "bytecode is stale for ",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[17];
    }
toplevel_consts_26_consts_11 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 16,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "compiled module ",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[22];
    }
toplevel_consts_26_consts_12 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 21,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = " is not a code object",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[13];
        }_object;
    }
toplevel_consts_26_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 13,
        },
        .ob_item = {
            Py_None,
            & toplevel_consts_26_consts_1._object.ob_base.ob_base,
            & toplevel_consts_6.ob_base.ob_base,
            & toplevel_consts_1.ob_base.ob_base,
            & toplevel_consts_18_consts_2.ob_base.ob_base,
            & toplevel_consts_26_consts_5._ascii.ob_base,
            & toplevel_consts_26_consts_6._ascii.ob_base,
            & toplevel_consts_21_consts_19.ob_base.ob_base,
            & toplevel_consts_21_consts_9.ob_base.ob_base,
            & toplevel_consts_21_consts_10.ob_base.ob_base,
            & toplevel_consts_26_consts_10._ascii.ob_base,
            & toplevel_consts_26_consts_11._ascii.ob_base,
            & toplevel_consts_26_consts_12._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[14];
    }
toplevel_consts_26_names_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 13,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_classify_pyc",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[5];
    }
toplevel_consts_26_names_2 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 4,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_imp",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[22];
    }
toplevel_consts_26_names_3 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 21,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "check_hash_based_pycs",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[16];
    }
toplevel_consts_26_names_4 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 15,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_get_pyc_source",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[12];
    }
toplevel_consts_26_names_5 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 11,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "source_hash",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[18];
    }
toplevel_consts_26_names_6 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 17,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_RAW_MAGIC_NUMBER",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[19];
    }
toplevel_consts_26_names_7 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 18,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_validate_hash_pyc",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[30];
    }
toplevel_consts_26_names_8 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 29,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_get_mtime_and_size_of_source",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[8];
    }
toplevel_consts_26_names_13 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 7,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "marshal",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[6];
    }
toplevel_consts_26_names_14 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 5,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "loads",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[11];
    }
toplevel_consts_26_names_16 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 10,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_code_type",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[10];
    }
toplevel_consts_26_names_17 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 9,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "TypeError",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[18];
        }_object;
    }
toplevel_consts_26_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 18,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_names_8._ascii.ob_base,
            & toplevel_consts_26_names_1._ascii.ob_base,
            & toplevel_consts_26_names_2._ascii.ob_base,
            & toplevel_consts_26_names_3._ascii.ob_base,
            & toplevel_consts_26_names_4._ascii.ob_base,
            & toplevel_consts_26_names_5._ascii.ob_base,
            & toplevel_consts_26_names_6._ascii.ob_base,
            & toplevel_consts_26_names_7._ascii.ob_base,
            & toplevel_consts_26_names_8._ascii.ob_base,
            & toplevel_consts_25_name._ascii.ob_base,
            & toplevel_consts_3_1._ascii.ob_base,
            & toplevel_consts_11_consts_6_names_1._ascii.ob_base,
            & toplevel_consts_11_consts_12_names_23._ascii.ob_base,
            & toplevel_consts_26_names_13._ascii.ob_base,
            & toplevel_consts_26_names_14._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_0._ascii.ob_base,
            & toplevel_consts_26_names_16._ascii.ob_base,
            & toplevel_consts_26_names_17._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[12];
    }
toplevel_consts_26_varnames_5 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 11,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "exc_details",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[11];
    }
toplevel_consts_26_varnames_7 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 10,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "hash_based",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[13];
    }
toplevel_consts_26_varnames_8 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 12,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "check_source",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[13];
    }
toplevel_consts_26_varnames_9 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 12,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "source_bytes",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[13];
    }
toplevel_consts_26_varnames_11 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 12,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "source_mtime",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[12];
    }
toplevel_consts_26_varnames_12 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 11,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "source_size",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[14];
        }_object;
    }
toplevel_consts_26_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 14,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_varnames_0._ascii.ob_base,
            & toplevel_consts_11_consts_8_varnames_1._ascii.ob_base,
            & toplevel_consts_11_consts_10_varnames_4._ascii.ob_base,
            & toplevel_consts_11_consts_4_varnames_1._ascii.ob_base,
            & toplevel_consts_21_varnames_6._ascii.ob_base,
            & toplevel_consts_26_varnames_5._ascii.ob_base,
            & toplevel_consts_21_varnames_13._ascii.ob_base,
            & toplevel_consts_26_varnames_7._ascii.ob_base,
            & toplevel_consts_26_varnames_8._ascii.ob_base,
            & toplevel_consts_26_varnames_9._ascii.ob_base,
            & toplevel_consts_26_names_5._ascii.ob_base,
            & toplevel_consts_26_varnames_11._ascii.ob_base,
            & toplevel_consts_26_varnames_12._ascii.ob_base,
            & toplevel_consts_11_consts_7_varnames_2._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[16];
    }
toplevel_consts_26_name = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 15,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_unmarshal_code",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[73];
    }
toplevel_consts_26_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 72,
    },
    .ob_shash = -1,
    .ob_sval = "\x02\x02\x02\x01\x06\xfe\x0e\x05\x0c\x02\x04\x01\x0c\x01\x0a\x01\x02\x01\x02\xff\x08\x01\x02\xff\x0a\x02\x08\x01\x04\x01\x04\x01\x02\x01\x04\xfe\x04\x05\x08\x01\x04\xff\x02\x80\x08\x04\x06\xff\x04\x03\x16\x03\x12\x01\x02\xff\x04\x02\x08\x01\x04\xff\x04\x02\x12\x02\x0a\x01\x10\x01\x04\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[83];
    }
toplevel_consts_26_endlinetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 82,
    },
    .ob_shash = -1,
    .ob_sval = "\x02\x02\x02\x01\x04\x01\x02\xfd\x0e\x05\x0c\x02\x02\x01\x02\x18\x0c\xe9\x08\x01\x02\x0a\x02\xf7\x02\x09\x08\xf7\x02\x09\x0a\xf8\x06\x01\x02\x07\x04\xfa\x04\x01\x02\x01\x02\x01\x02\xfd\x04\x05\x0c\x01\x02\x80\x08\x03\x06\xff\x02\x03\x02\x07\x14\xfc\x02\x04\x12\xfd\x02\x03\x04\xfe\x0c\x01\x04\x01\x12\x02\x08\x01\x12\x01\x04\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[255];
    }
toplevel_consts_26_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 254,
    },
    .ob_shash = -1,
    .ob_sval = "\x11\x19\x11\x19\x13\x06\x13\x06\x05\x10\x0d\x20\x0d\x2e\x2f\x33\x35\x3d\x3f\x4a\x0d\x4b\x05\x0a\x12\x17\x1a\x1d\x12\x1d\x21\x22\x12\x22\x05\x0f\x08\x12\x05\x1c\x18\x1d\x20\x24\x18\x24\x28\x29\x18\x29\x09\x15\x0d\x11\x0d\x27\x2b\x32\x0d\x32\x09\x3e\x12\x1e\x09\x3e\x22\x26\x22\x3c\x40\x48\x22\x48\x09\x3e\x1c\x2b\x2c\x30\x32\x3a\x1c\x3b\x0d\x19\x10\x1c\x24\x28\x10\x28\x0d\x3e\x1f\x23\x1f\x2f\x15\x28\x15\x3a\x15\x21\x1f\x12\x11\x1c\x11\x24\x11\x37\x15\x19\x1b\x26\x28\x30\x32\x3d\x11\x3e\x11\x3e\x00\x00\x0d\x2a\x2b\x2f\x31\x39\x0d\x3a\x09\x22\x09\x15\x17\x22\x0c\x18\x09\x1c\x15\x1e\x1f\x2d\x2e\x32\x33\x34\x35\x37\x33\x37\x2e\x38\x1f\x39\x3b\x47\x15\x48\x0d\x1c\x15\x23\x24\x28\x29\x2b\x2c\x2e\x29\x2e\x24\x2f\x15\x30\x34\x3f\x15\x3f\x0d\x1c\x11\x1b\x11\x2c\x15\x3a\x2e\x36\x15\x3a\x15\x3a\x11\x3b\x11\x3b\x18\x1c\x18\x1c\x0c\x13\x0c\x19\x1a\x1e\x1f\x21\x1f\x22\x1f\x22\x1a\x23\x0c\x24\x05\x09\x0c\x16\x17\x1b\x1d\x27\x0c\x28\x05\x4e\x0f\x18\x19\x4d\x2c\x34\x19\x4d\x19\x4d\x19\x4d\x0f\x4e\x09\x4e\x0c\x10\x05\x10",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[15];
    }
toplevel_consts_26_localspluskinds = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 14,
    },
    .ob_shash = -1,
    .ob_sval = "              ",
};
static struct PyCodeObject toplevel_consts_26 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_26_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_26_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_26_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_7_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 5,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 5,
    .co_firstlineno = 635,
    .co_code = & toplevel_consts_26_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_26_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_26_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_26_name._ascii.ob_base,
    .co_qualname = & toplevel_consts_26_name._ascii.ob_base,
    .co_linetable = & toplevel_consts_26_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_26_endlinetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_26_columntable.ob_base.ob_base,
    .co_nlocalsplus = 14,
    .co_nlocals = 14,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_26_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[29];
    }
toplevel_consts_27_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 28,
    },
    .ob_shash = -1,
    .ob_sval = "\x7c\x00\xa0\x00\x64\x01\x64\x02\xa1\x02\x7d\x00\x7c\x00\xa0\x00\x64\x03\x64\x02\xa1\x02\x7d\x00\x7c\x00\x53\x00",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[3];
    }
toplevel_consts_27_consts_1 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 2,
    },
    .ob_shash = -1,
    .ob_sval = "\x0d\x0a",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[2];
    }
toplevel_consts_27_consts_2 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 1,
    },
    .ob_shash = -1,
    .ob_sval = "\x0a",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[2];
    }
toplevel_consts_27_consts_3 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 1,
    },
    .ob_shash = -1,
    .ob_sval = "\x0d",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[4];
        }_object;
    }
toplevel_consts_27_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 4,
        },
        .ob_item = {
            Py_None,
            & toplevel_consts_27_consts_1.ob_base.ob_base,
            & toplevel_consts_27_consts_2.ob_base.ob_base,
            & toplevel_consts_27_consts_3.ob_base.ob_base,
        },
    },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[1];
        }_object;
    }
toplevel_consts_27_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 1,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_names_6._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[7];
    }
toplevel_consts_27_varnames_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 6,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "source",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[1];
        }_object;
    }
toplevel_consts_27_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 1,
        },
        .ob_item = {
            & toplevel_consts_27_varnames_0._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[24];
    }
toplevel_consts_27_name = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 23,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_normalize_line_endings",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[7];
    }
toplevel_consts_27_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 6,
    },
    .ob_shash = -1,
    .ob_sval = "\x0c\x01\x0c\x01\x04\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[29];
    }
toplevel_consts_27_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 28,
    },
    .ob_shash = -1,
    .ob_sval = "\x0e\x14\x0e\x2c\x1d\x24\x26\x2b\x0e\x2c\x05\x0b\x0e\x14\x0e\x2a\x1d\x22\x24\x29\x0e\x2a\x05\x0b\x0c\x12\x05\x12",
};
static struct PyCodeObject toplevel_consts_27 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_27_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_27_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_27_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_7_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 1,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 4,
    .co_firstlineno = 680,
    .co_code = & toplevel_consts_27_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_27_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_11_consts_14_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_27_name._ascii.ob_base,
    .co_qualname = & toplevel_consts_27_name._ascii.ob_base,
    .co_linetable = & toplevel_consts_27_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_27_linetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_27_columntable.ob_base.ob_base,
    .co_nlocalsplus = 1,
    .co_nlocals = 1,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_27_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[25];
    }
toplevel_consts_28_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 24,
    },
    .ob_shash = -1,
    .ob_sval = "\x74\x00\x7c\x01\x83\x01\x7d\x01\x74\x01\x7c\x01\x7c\x00\x64\x01\x64\x02\x64\x03\x8d\x04\x53\x00",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[13];
    }
toplevel_consts_28_consts_3_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 12,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "dont_inherit",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[1];
        }_object;
    }
toplevel_consts_28_consts_3 = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 1,
        },
        .ob_item = {
            & toplevel_consts_28_consts_3_0._ascii.ob_base,
        },
    },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[4];
        }_object;
    }
toplevel_consts_28_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 4,
        },
        .ob_item = {
            Py_None,
            & toplevel_consts_11_consts_12_names_19._ascii.ob_base,
            Py_True,
            & toplevel_consts_28_consts_3._object.ob_base.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[8];
    }
toplevel_consts_28_names_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 7,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "compile",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[2];
        }_object;
    }
toplevel_consts_28_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 2,
        },
        .ob_item = {
            & toplevel_consts_27_name._ascii.ob_base,
            & toplevel_consts_28_names_1._ascii.ob_base,
        },
    },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[2];
        }_object;
    }
toplevel_consts_28_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 2,
        },
        .ob_item = {
            & toplevel_consts_11_consts_8_varnames_1._ascii.ob_base,
            & toplevel_consts_27_varnames_0._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[16];
    }
toplevel_consts_28_name = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 15,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_compile_source",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[5];
    }
toplevel_consts_28_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 4,
    },
    .ob_shash = -1,
    .ob_sval = "\x08\x01\x10\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[25];
    }
toplevel_consts_28_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 24,
    },
    .ob_shash = -1,
    .ob_sval = "\x0e\x25\x26\x2c\x0e\x2d\x05\x0b\x0c\x13\x14\x1a\x1c\x24\x26\x2c\x3b\x3f\x0c\x40\x0c\x40\x05\x40",
};
static struct PyCodeObject toplevel_consts_28 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_28_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_28_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_28_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_7_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 2,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 6,
    .co_firstlineno = 687,
    .co_code = & toplevel_consts_28_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_28_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_18_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_28_name._ascii.ob_base,
    .co_qualname = & toplevel_consts_28_name._ascii.ob_base,
    .co_linetable = & toplevel_consts_28_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_28_linetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_28_columntable.ob_base.ob_base,
    .co_nlocalsplus = 2,
    .co_nlocals = 2,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_28_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[69];
    }
toplevel_consts_29_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 68,
    },
    .ob_shash = -1,
    .ob_sval = "\x74\x00\x6a\x01\x7c\x00\x64\x01\x3f\x00\x64\x02\x17\x00\x7c\x00\x64\x03\x3f\x00\x64\x04\x40\x00\x7c\x00\x64\x05\x40\x00\x7c\x01\x64\x06\x3f\x00\x7c\x01\x64\x03\x3f\x00\x64\x07\x40\x00\x7c\x01\x64\x05\x40\x00\x64\x08\x14\x00\x64\x09\x64\x09\x64\x09\x66\x09\x83\x01\x53\x00",
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_29_consts_1 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 9 },
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_29_consts_2 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 1980 },
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_29_consts_3 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 5 },
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_29_consts_4 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 15 },
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_29_consts_5 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 31 },
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_29_consts_6 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 11 },
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_29_consts_7 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 63 },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[10];
        }_object;
    }
toplevel_consts_29_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 10,
        },
        .ob_item = {
            Py_None,
            & toplevel_consts_29_consts_1.ob_base.ob_base,
            & toplevel_consts_29_consts_2.ob_base.ob_base,
            & toplevel_consts_29_consts_3.ob_base.ob_base,
            & toplevel_consts_29_consts_4.ob_base.ob_base,
            & toplevel_consts_29_consts_5.ob_base.ob_base,
            & toplevel_consts_29_consts_6.ob_base.ob_base,
            & toplevel_consts_29_consts_7.ob_base.ob_base,
            & toplevel_consts_18_consts_2.ob_base.ob_base,
            & toplevel_consts_11_consts_2_consts_8.ob_base.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[7];
    }
toplevel_consts_29_names_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 6,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "mktime",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[2];
        }_object;
    }
toplevel_consts_29_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 2,
        },
        .ob_item = {
            & toplevel_consts_21_varnames_15._ascii.ob_base,
            & toplevel_consts_29_names_1._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[2];
    }
toplevel_consts_29_varnames_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 1,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "d",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[2];
        }_object;
    }
toplevel_consts_29_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 2,
        },
        .ob_item = {
            & toplevel_consts_29_varnames_0._ascii.ob_base,
            & toplevel_consts_21_varnames_25._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[15];
    }
toplevel_consts_29_name = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 14,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_parse_dostime",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[19];
    }
toplevel_consts_29_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 18,
    },
    .ob_shash = -1,
    .ob_sval = "\x04\x01\x0a\x01\x0a\x01\x06\x01\x06\x01\x0a\x01\x0a\x01\x06\x01\x06\xf9",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[17];
    }
toplevel_consts_29_endlinetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 16,
    },
    .ob_shash = -1,
    .ob_sval = "\x04\x01\x0a\x01\x0a\x01\x06\x01\x06\x01\x0a\x01\x0a\x01\x0c\x01",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[69];
    }
toplevel_consts_29_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 68,
    },
    .ob_shash = -1,
    .ob_sval = "\x0c\x10\x0c\x17\x0a\x0b\x0f\x10\x0a\x10\x14\x18\x09\x18\x0a\x0b\x0f\x10\x0a\x10\x14\x17\x09\x17\x09\x0a\x0d\x11\x09\x11\x09\x0a\x0e\x10\x09\x10\x0a\x0b\x0f\x10\x0a\x10\x14\x18\x09\x18\x0a\x0b\x0e\x12\x0a\x12\x16\x17\x09\x17\x09\x0b\x0d\x0f\x11\x13\x18\x14\x0c\x15\x05\x15",
};
static struct PyCodeObject toplevel_consts_29 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_29_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_29_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_29_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_7_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 2,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 10,
    .co_firstlineno = 693,
    .co_code = & toplevel_consts_29_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_29_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_18_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_29_name._ascii.ob_base,
    .co_qualname = & toplevel_consts_29_name._ascii.ob_base,
    .co_linetable = & toplevel_consts_29_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_29_endlinetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_29_columntable.ob_base.ob_base,
    .co_nlocalsplus = 2,
    .co_nlocals = 2,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_29_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[113];
    }
toplevel_consts_30_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 112,
    },
    .ob_shash = -1,
    .ob_sval = "\x09\x00\x7c\x01\x64\x01\x64\x00\x85\x02\x19\x00\x64\x02\x76\x00\x73\x0b\x4a\x00\x82\x01\x7c\x01\x64\x00\x64\x01\x85\x02\x19\x00\x7d\x01\x7c\x00\x6a\x00\x7c\x01\x19\x00\x7d\x02\x7c\x02\x64\x03\x19\x00\x7d\x03\x7c\x02\x64\x04\x19\x00\x7d\x04\x7c\x02\x64\x05\x19\x00\x7d\x05\x74\x01\x7c\x04\x7c\x03\x83\x02\x7c\x05\x66\x02\x53\x00\x23\x00\x04\x00\x74\x02\x74\x03\x74\x04\x66\x03\x79\x36\x01\x00\x01\x00\x01\x00\x59\x00\x64\x06\x53\x00\x77\x00\x25\x00",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[2];
    }
toplevel_consts_30_consts_2_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 1,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "c",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[2];
    }
toplevel_consts_30_consts_2_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 1,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "o",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[2];
        }_object;
    }
toplevel_consts_30_consts_2 = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 2,
        },
        .ob_item = {
            & toplevel_consts_30_consts_2_0._ascii.ob_base,
            & toplevel_consts_30_consts_2_1._ascii.ob_base,
        },
    },
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_30_consts_4 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 6 },
};
static
    struct {
        PyObject_VAR_HEAD
        digit ob_digit[1];
    }
toplevel_consts_30_consts_5 = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyLong_Type,
        },
        .ob_size = 1,
    },
    .ob_digit = { 3 },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[2];
        }_object;
    }
toplevel_consts_30_consts_6 = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 2,
        },
        .ob_item = {
            & toplevel_consts_1.ob_base.ob_base,
            & toplevel_consts_1.ob_base.ob_base,
        },
    },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[7];
        }_object;
    }
toplevel_consts_30_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 7,
        },
        .ob_item = {
            Py_None,
            & toplevel_consts_11_consts_2_consts_8.ob_base.ob_base,
            & toplevel_consts_30_consts_2._object.ob_base.ob_base,
            & toplevel_consts_29_consts_3.ob_base.ob_base,
            & toplevel_consts_30_consts_4.ob_base.ob_base,
            & toplevel_consts_30_consts_5.ob_base.ob_base,
            & toplevel_consts_30_consts_6._object.ob_base.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[11];
    }
toplevel_consts_30_names_3 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 10,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "IndexError",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[5];
        }_object;
    }
toplevel_consts_30_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 5,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_names_18._ascii.ob_base,
            & toplevel_consts_29_name._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_16._ascii.ob_base,
            & toplevel_consts_30_names_3._ascii.ob_base,
            & toplevel_consts_26_names_17._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[18];
    }
toplevel_consts_30_varnames_5 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 17,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "uncompressed_size",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[6];
        }_object;
    }
toplevel_consts_30_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 6,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_varnames_0._ascii.ob_base,
            & toplevel_consts_11_consts_2_consts_3_0._ascii.ob_base,
            & toplevel_consts_11_consts_8_varnames_3._ascii.ob_base,
            & toplevel_consts_21_varnames_15._ascii.ob_base,
            & toplevel_consts_21_varnames_16._ascii.ob_base,
            & toplevel_consts_30_varnames_5._ascii.ob_base,
        },
    },
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[27];
    }
toplevel_consts_30_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 26,
    },
    .ob_shash = -1,
    .ob_sval = "\x02\x01\x14\x02\x0c\x01\x0a\x01\x08\x03\x08\x01\x08\x01\x0e\x01\x02\x80\x12\x01\x06\x01\x02\xff\x02\x80",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[27];
    }
toplevel_consts_30_endlinetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 26,
    },
    .ob_shash = -1,
    .ob_sval = "\x02\x0d\x14\xf6\x0c\x01\x0a\x01\x08\x03\x08\x01\x08\x01\x0e\x01\x02\x80\x02\x02\x08\xff\x10\x01\x02\x80",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[113];
    }
toplevel_consts_30_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 112,
    },
    .ob_shash = -1,
    .ob_sval = "\x05\x14\x10\x14\x15\x17\x15\x18\x15\x18\x10\x19\x1d\x27\x10\x27\x09\x27\x09\x27\x09\x27\x10\x14\x15\x18\x16\x18\x15\x18\x10\x19\x09\x0d\x15\x19\x15\x20\x21\x25\x15\x26\x09\x12\x10\x19\x1a\x1b\x10\x1c\x09\x0d\x10\x19\x1a\x1b\x10\x1c\x09\x0d\x1d\x26\x27\x28\x1d\x29\x09\x1a\x10\x1e\x1f\x23\x25\x29\x10\x2a\x2c\x3d\x10\x3d\x09\x3d\x00\x00\x05\x14\x0d\x15\x17\x21\x23\x2c\x0c\x2d\x05\x14\x05\x14\x05\x14\x05\x14\x10\x14\x10\x14\x10\x14\x05\x14\x00\x00",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[13];
    }
toplevel_consts_30_exceptiontable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 12,
    },
    .ob_shash = -1,
    .ob_sval = "\x81\x27\x29\x00\xa9\x0a\x37\x07\xb6\x01\x37\x07",
};
static struct PyCodeObject toplevel_consts_30 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_30_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_30_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_30_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_30_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 2,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 10,
    .co_firstlineno = 706,
    .co_code = & toplevel_consts_30_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_30_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_11_consts_10_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_26_names_8._ascii.ob_base,
    .co_qualname = & toplevel_consts_26_names_8._ascii.ob_base,
    .co_linetable = & toplevel_consts_30_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_30_endlinetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_30_columntable.ob_base.ob_base,
    .co_nlocalsplus = 6,
    .co_nlocals = 6,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_30_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[83];
    }
toplevel_consts_31_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 82,
    },
    .ob_shash = -1,
    .ob_sval = "\x7c\x01\x64\x01\x64\x00\x85\x02\x19\x00\x64\x02\x76\x00\x73\x0a\x4a\x00\x82\x01\x7c\x01\x64\x00\x64\x01\x85\x02\x19\x00\x7d\x01\x09\x00\x7c\x00\x6a\x00\x7c\x01\x19\x00\x7d\x02\x6e\x0c\x23\x00\x04\x00\x74\x01\x79\x21\x01\x00\x01\x00\x01\x00\x59\x00\x64\x00\x53\x00\x77\x00\x25\x00\x74\x02\x7c\x00\x6a\x03\x7c\x02\x83\x02\x53\x00",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[3];
        }_object;
    }
toplevel_consts_31_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 3,
        },
        .ob_item = {
            Py_None,
            & toplevel_consts_11_consts_2_consts_8.ob_base.ob_base,
            & toplevel_consts_30_consts_2._object.ob_base.ob_base,
        },
    },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[4];
        }_object;
    }
toplevel_consts_31_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 4,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_names_18._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_16._ascii.ob_base,
            & toplevel_consts_11_consts_8_names_9._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_19._ascii.ob_base,
        },
    },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[3];
        }_object;
    }
toplevel_consts_31_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 3,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_varnames_0._ascii.ob_base,
            & toplevel_consts_11_consts_2_consts_3_0._ascii.ob_base,
            & toplevel_consts_11_consts_8_varnames_3._ascii.ob_base,
        },
    },
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[21];
    }
toplevel_consts_31_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 20,
    },
    .ob_shash = -1,
    .ob_sval = "\x14\x02\x0c\x01\x02\x02\x0c\x01\x02\x80\x0c\x01\x06\x01\x02\xff\x02\x80\x0c\x03",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[21];
    }
toplevel_consts_31_endlinetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 20,
    },
    .ob_shash = -1,
    .ob_sval = "\x14\x02\x0c\x01\x02\x07\x0c\xfc\x02\x80\x02\x02\x02\xff\x10\x01\x02\x80\x0c\x02",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[83];
    }
toplevel_consts_31_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 82,
    },
    .ob_shash = -1,
    .ob_sval = "\x0c\x10\x11\x13\x11\x14\x11\x14\x0c\x15\x19\x23\x0c\x23\x05\x23\x05\x23\x05\x23\x0c\x10\x11\x14\x12\x14\x11\x14\x0c\x15\x05\x09\x05\x32\x15\x19\x15\x20\x21\x25\x15\x26\x09\x12\x09\x12\x00\x00\x05\x14\x0c\x14\x05\x14\x05\x14\x05\x14\x05\x14\x10\x14\x10\x14\x10\x14\x05\x14\x00\x00\x10\x19\x1a\x1e\x1a\x26\x28\x31\x10\x32\x09\x32",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[13];
    }
toplevel_consts_31_exceptiontable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 12,
    },
    .ob_shash = -1,
    .ob_sval = "\x91\x05\x17\x00\x97\x07\x22\x07\xa1\x01\x22\x07",
};
static struct PyCodeObject toplevel_consts_31 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_31_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_31_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_31_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_31_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 2,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 8,
    .co_firstlineno = 725,
    .co_code = & toplevel_consts_31_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_31_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_11_consts_5_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_26_names_4._ascii.ob_base,
    .co_qualname = & toplevel_consts_26_names_4._ascii.ob_base,
    .co_linetable = & toplevel_consts_31_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_31_endlinetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_31_columntable.ob_base.ob_base,
    .co_nlocalsplus = 3,
    .co_nlocals = 3,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_31_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[271];
    }
toplevel_consts_32_code = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 270,
    },
    .ob_shash = -1,
    .ob_sval = "\x74\x00\x7c\x00\x7c\x01\x83\x02\x7d\x02\x64\x00\x7d\x03\x74\x01\x44\x00\x5d\x66\x5c\x03\x7d\x04\x7d\x05\x7d\x06\x7c\x02\x7c\x04\x17\x00\x7d\x07\x74\x02\x6a\x03\x64\x01\x7c\x00\x6a\x04\x74\x05\x7c\x07\x64\x02\x64\x03\x8d\x05\x01\x00\x09\x00\x7c\x00\x6a\x06\x7c\x07\x19\x00\x7d\x08\x6e\x0b\x23\x00\x04\x00\x74\x07\x79\x2d\x01\x00\x01\x00\x01\x00\x59\x00\x71\x09\x77\x00\x25\x00\x7c\x08\x64\x04\x19\x00\x7d\x09\x74\x08\x7c\x00\x6a\x04\x7c\x08\x83\x02\x7d\x0a\x64\x00\x7d\x0b\x7c\x05\x72\x5b\x09\x00\x74\x09\x7c\x00\x7c\x09\x7c\x07\x7c\x01\x7c\x0a\x83\x05\x7d\x0b\x6e\x19\x23\x00\x04\x00\x74\x0a\x79\x59\x01\x00\x7d\x0c\x01\x00\x7c\x0c\x7d\x03\x59\x00\x64\x00\x7d\x0c\x7e\x0c\x6e\x0b\x64\x00\x7d\x0c\x7e\x0c\x77\x01\x77\x00\x25\x00\x74\x0b\x7c\x09\x7c\x0a\x83\x02\x7d\x0b\x7c\x0b\x64\x00\x75\x00\x72\x65\x71\x09\x7c\x08\x64\x04\x19\x00\x7d\x09\x7c\x0b\x7c\x06\x7c\x09\x66\x03\x02\x00\x01\x00\x53\x00\x7c\x03\x72\x7e\x64\x05\x7c\x03\x9b\x00\x9d\x02\x7d\x0d\x74\x0c\x7c\x0d\x7c\x01\x64\x06\x8d\x02\x7c\x03\x82\x02\x74\x0c\x64\x07\x7c\x01\x9b\x02\x9d\x02\x7c\x01\x64\x06\x8d\x02\x82\x01",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[14];
    }
toplevel_consts_32_consts_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 13,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "trying {}{}{}",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[10];
    }
toplevel_consts_32_consts_3_0 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 9,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "verbosity",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[1];
        }_object;
    }
toplevel_consts_32_consts_3 = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 1,
        },
        .ob_item = {
            & toplevel_consts_32_consts_3_0._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[21];
    }
toplevel_consts_32_consts_5 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 20,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "module load failed: ",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[8];
        }_object;
    }
toplevel_consts_32_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 8,
        },
        .ob_item = {
            Py_None,
            & toplevel_consts_32_consts_1._ascii.ob_base,
            & toplevel_consts_18_consts_2.ob_base.ob_base,
            & toplevel_consts_32_consts_3._object.ob_base.ob_base,
            & toplevel_consts_1.ob_base.ob_base,
            & toplevel_consts_32_consts_5._ascii.ob_base,
            & toplevel_consts_11_consts_10_consts_3._object.ob_base.ob_base,
            & toplevel_consts_11_consts_10_consts_2._ascii.ob_base,
        },
    },
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[13];
        }_object;
    }
toplevel_consts_32_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 13,
        },
        .ob_item = {
            & toplevel_consts_11_consts_4_names_4._ascii.ob_base,
            & toplevel_consts_20_names_1._ascii.ob_base,
            & toplevel_consts_11_consts_6_names_1._ascii.ob_base,
            & toplevel_consts_11_consts_12_names_23._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_19._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_7._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_18._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_16._ascii.ob_base,
            & toplevel_consts_11_consts_8_names_9._ascii.ob_base,
            & toplevel_consts_26_name._ascii.ob_base,
            & toplevel_consts_11_consts_12_names_21._ascii.ob_base,
            & toplevel_consts_28_name._ascii.ob_base,
            & toplevel_consts_4._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[13];
    }
toplevel_consts_32_varnames_3 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 12,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "import_error",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[4];
    }
toplevel_consts_32_varnames_12 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 3,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "exc",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[14];
        }_object;
    }
toplevel_consts_32_varnames = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 14,
        },
        .ob_item = {
            & toplevel_consts_11_consts_2_varnames_0._ascii.ob_base,
            & toplevel_consts_11_consts_4_varnames_1._ascii.ob_base,
            & toplevel_consts_11_consts_2_consts_3_0._ascii.ob_base,
            & toplevel_consts_32_varnames_3._ascii.ob_base,
            & toplevel_consts_20_varnames_3._ascii.ob_base,
            & toplevel_consts_20_varnames_4._ascii.ob_base,
            & toplevel_consts_11_consts_7_varnames_3._ascii.ob_base,
            & toplevel_consts_11_consts_10_varnames_4._ascii.ob_base,
            & toplevel_consts_11_consts_8_varnames_3._ascii.ob_base,
            & toplevel_consts_11_consts_4_varnames_4._ascii.ob_base,
            & toplevel_consts_21_varnames_6._ascii.ob_base,
            & toplevel_consts_11_consts_7_varnames_2._ascii.ob_base,
            & toplevel_consts_32_varnames_12._ascii.ob_base,
            & toplevel_consts_11_consts_12_varnames_2._ascii.ob_base,
        },
    },
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[67];
    }
toplevel_consts_32_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 66,
    },
    .ob_shash = -1,
    .ob_sval = "\x0a\x01\x04\x01\x0e\x01\x08\x01\x16\x01\x02\x01\x0c\x01\x02\x80\x0c\x01\x04\x01\x02\xff\x02\x80\x08\x03\x0c\x01\x04\x01\x04\x01\x02\x01\x12\x01\x02\x80\x0c\x01\x0e\x01\x08\x80\x02\xff\x02\x80\x0a\x03\x08\x01\x02\x03\x08\x01\x0e\x01\x04\x02\x0a\x01\x0e\x01\x12\x02",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[77];
    }
toplevel_consts_32_endlinetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 76,
    },
    .ob_shash = -1,
    .ob_sval = "\x0a\x01\x04\x01\x02\x01\x04\x1d\x08\xe3\x08\x01\x16\x01\x02\x15\x0c\xed\x02\x80\x02\x02\x02\xff\x0e\x01\x02\x80\x08\x02\x0c\x01\x04\x01\x02\x01\x02\x06\x02\xfe\x12\xfe\x02\x80\x02\x02\x02\xff\x16\x01\x08\x80\x02\x00\x02\x80\x0a\x02\x06\x01\x04\x03\x08\x01\x0e\x01\x02\x02\x02\x04\x0a\xfd\x0e\x01\x12\x02",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[271];
    }
toplevel_consts_32_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 270,
    },
    .ob_shash = -1,
    .ob_sval = "\x0c\x1c\x1d\x21\x23\x2b\x0c\x2c\x05\x09\x14\x18\x05\x11\x2a\x3a\x05\x53\x05\x53\x09\x26\x09\x0f\x11\x1b\x1d\x26\x14\x18\x1b\x21\x14\x21\x09\x11\x09\x13\x09\x24\x25\x34\x36\x3a\x36\x42\x44\x4c\x4e\x56\x62\x63\x09\x64\x09\x64\x09\x64\x09\x2c\x19\x1d\x19\x24\x25\x2d\x19\x2e\x0d\x16\x0d\x16\x00\x00\x09\x11\x10\x18\x09\x11\x09\x11\x09\x11\x09\x11\x0d\x11\x0d\x11\x09\x11\x00\x00\x17\x20\x21\x22\x17\x23\x0d\x14\x14\x1d\x1e\x22\x1e\x2a\x2c\x35\x14\x36\x0d\x11\x14\x18\x0d\x11\x10\x1a\x0d\x36\x11\x27\x1c\x2b\x2c\x30\x32\x39\x3b\x43\x45\x4d\x4f\x53\x1c\x54\x15\x19\x15\x19\x00\x00\x11\x27\x18\x23\x11\x27\x11\x27\x11\x27\x11\x27\x24\x27\x15\x21\x15\x21\x15\x21\x15\x21\x15\x21\x15\x21\x00\x00\x00\x00\x00\x00\x00\x00\x11\x27\x00\x00\x18\x27\x28\x2f\x31\x35\x18\x36\x11\x15\x10\x14\x18\x1c\x10\x1c\x0d\x19\x11\x19\x17\x20\x21\x22\x17\x23\x0d\x14\x14\x18\x1a\x23\x25\x2c\x14\x2c\x0d\x2c\x0d\x2c\x0d\x2c\x0c\x18\x09\x53\x13\x38\x2a\x36\x13\x38\x13\x38\x0d\x10\x13\x21\x22\x25\x2c\x34\x13\x35\x13\x35\x3b\x47\x0d\x47\x13\x21\x22\x43\x37\x3f\x22\x43\x22\x43\x4a\x52\x13\x53\x13\x53\x0d\x53",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[36];
    }
toplevel_consts_32_exceptiontable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 35,
    },
    .ob_shash = -1,
    .ob_sval = "\x9e\x05\x24\x02\xa4\x07\x2e\x09\xad\x01\x2e\x09\xbe\x08\x41\x07\x02\xc1\x07\x07\x41\x1a\x09\xc1\x0e\x02\x41\x15\x09\xc1\x15\x05\x41\x1a\x09",
};
static struct PyCodeObject toplevel_consts_32 = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts_32_consts._object.ob_base.ob_base,
    .co_names = & toplevel_consts_32_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_consts_32_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_32_exceptiontable.ob_base.ob_base,
    .co_flags = 3,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 2,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 9,
    .co_firstlineno = 740,
    .co_code = & toplevel_consts_32_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_32_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_26_localspluskinds.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_consts_11_consts_7_names_0._ascii.ob_base,
    .co_qualname = & toplevel_consts_11_consts_7_names_0._ascii.ob_base,
    .co_linetable = & toplevel_consts_32_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_consts_32_endlinetable.ob_base.ob_base,
    .co_columntable = & toplevel_consts_32_columntable.ob_base.ob_base,
    .co_nlocalsplus = 14,
    .co_nlocals = 14,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_32_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[33];
        }_object;
    }
toplevel_consts = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 33,
        },
        .ob_item = {
            & toplevel_consts_0._ascii.ob_base,
            & toplevel_consts_1.ob_base.ob_base,
            Py_None,
            & toplevel_consts_3._object.ob_base.ob_base,
            & toplevel_consts_4._ascii.ob_base,
            & toplevel_consts_5._ascii.ob_base,
            & toplevel_consts_6.ob_base.ob_base,
            & toplevel_consts_7.ob_base,
            & toplevel_consts_8.ob_base.ob_base,
            & toplevel_consts_9.ob_base.ob_base,
            & toplevel_consts_10.ob_base.ob_base,
            & toplevel_consts_11.ob_base,
            & toplevel_consts_12._ascii.ob_base,
            Py_True,
            & toplevel_consts_11_consts_10_consts_4._ascii.ob_base,
            Py_False,
            & toplevel_consts_16._object.ob_base.ob_base,
            & toplevel_consts_17._object.ob_base.ob_base,
            & toplevel_consts_18.ob_base,
            & toplevel_consts_19.ob_base,
            & toplevel_consts_20.ob_base,
            & toplevel_consts_21.ob_base,
            & toplevel_consts_22._compact._base.ob_base,
            & toplevel_consts_23.ob_base,
            & toplevel_consts_24.ob_base,
            & toplevel_consts_25.ob_base,
            & toplevel_consts_26.ob_base,
            & toplevel_consts_27.ob_base,
            & toplevel_consts_28.ob_base,
            & toplevel_consts_29.ob_base,
            & toplevel_consts_30.ob_base,
            & toplevel_consts_31.ob_base,
            & toplevel_consts_32.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[27];
    }
toplevel_names_1 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 26,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_frozen_importlib_external",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[18];
    }
toplevel_names_5 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 17,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_frozen_importlib",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[8];
    }
toplevel_names_13 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 7,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "__all__",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[16];
    }
toplevel_names_15 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 15,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "path_separators",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[5];
    }
toplevel_names_20 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 4,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "type",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[14];
    }
toplevel_names_25 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 13,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "_LoaderBasics",
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[9];
    }
toplevel_names_38 = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 8,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "__code__",
};
static
    struct {
        PyGC_Head _gc_head;
        struct {
            PyObject_VAR_HEAD
            PyObject *ob_item[46];
        }_object;
    }
toplevel_names = {
    ._object = {
        .ob_base = {
            .ob_base = {
                .ob_refcnt = 999999999,
                .ob_type = &PyTuple_Type,
            },
            .ob_size = 46,
        },
        .ob_item = {
            & toplevel_consts_11_names_3._ascii.ob_base,
            & toplevel_names_1._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_8._ascii.ob_base,
            & toplevel_consts_3_0._ascii.ob_base,
            & toplevel_consts_3_1._ascii.ob_base,
            & toplevel_names_5._ascii.ob_base,
            & toplevel_consts_11_consts_6_names_1._ascii.ob_base,
            & toplevel_consts_26_names_2._ascii.ob_base,
            & toplevel_consts_21_names_0._ascii.ob_base,
            & toplevel_consts_26_names_13._ascii.ob_base,
            & toplevel_consts_11_consts_12_names_4._ascii.ob_base,
            & toplevel_consts_21_varnames_15._ascii.ob_base,
            & toplevel_consts_11_consts_4_names_0._ascii.ob_base,
            & toplevel_names_13._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_7._ascii.ob_base,
            & toplevel_names_15._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_5._ascii.ob_base,
            & toplevel_consts_11_consts_12_names_21._ascii.ob_base,
            & toplevel_consts_4._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_15._ascii.ob_base,
            & toplevel_names_20._ascii.ob_base,
            & toplevel_consts_11_consts_12_names_8._ascii.ob_base,
            & toplevel_consts_21_names_5._ascii.ob_base,
            & toplevel_consts_21_names_9._ascii.ob_base,
            & toplevel_consts_21_names_11._ascii.ob_base,
            & toplevel_names_25._ascii.ob_base,
            & toplevel_consts_5._ascii.ob_base,
            & toplevel_consts_20_names_1._ascii.ob_base,
            & toplevel_consts_11_consts_4_names_4._ascii.ob_base,
            & toplevel_consts_11_consts_4_names_5._ascii.ob_base,
            & toplevel_consts_11_consts_4_names_3._ascii.ob_base,
            & toplevel_consts_11_consts_2_names_17._ascii.ob_base,
            & toplevel_consts_21_names_19._ascii.ob_base,
            & toplevel_consts_23_names_0._ascii.ob_base,
            & toplevel_consts_23_name._ascii.ob_base,
            & toplevel_consts_11_consts_8_names_9._ascii.ob_base,
            & toplevel_consts_25_name._ascii.ob_base,
            & toplevel_consts_26_name._ascii.ob_base,
            & toplevel_names_38._ascii.ob_base,
            & toplevel_consts_26_names_16._ascii.ob_base,
            & toplevel_consts_27_name._ascii.ob_base,
            & toplevel_consts_28_name._ascii.ob_base,
            & toplevel_consts_29_name._ascii.ob_base,
            & toplevel_consts_26_names_8._ascii.ob_base,
            & toplevel_consts_26_names_4._ascii.ob_base,
            & toplevel_consts_11_consts_7_names_0._ascii.ob_base,
        },
    },
};
static
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[9];
    }
toplevel_name = {
    ._ascii = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyUnicode_Type,
        },
        .length = 8,
        .hash = -1,
        .state = {
            .kind = 1,
            .compact = 1,
            .ascii = 1,
            .ready = 1,
        },
    },
    ._data = "<module>",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[91];
    }
toplevel_linetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 90,
    },
    .ob_shash = -1,
    .ob_sval = "\x04\x00\x08\x10\x10\x01\x08\x01\x08\x01\x08\x01\x08\x01\x08\x01\x08\x01\x08\x01\x08\x02\x06\x03\x0e\x01\x0e\x03\x04\x04\x08\x02\x04\x02\x04\x01\x04\x01\x10\x02\x00\x7f\x00\x7f\x0c\x32\x0c\x01\x02\x01\x02\x01\x04\xfc\x06\x09\x06\x04\x06\x09\x06\x1f\x02\x7e\x02\xfe\x04\x1d\x06\x05\x06\x15\x06\x2e\x06\x08\x0a\x28\x06\x05\x06\x07\x06\x06\x06\x0d\x06\x13\x0a\x0f",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[111];
    }
toplevel_endlinetable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 110,
    },
    .ob_shash = -1,
    .ob_sval = "\x04\x0c\x08\x04\x10\x01\x08\x01\x08\x01\x08\x01\x08\x01\x08\x01\x08\x01\x08\x01\x08\x02\x06\x03\x0e\x01\x08\x04\x02\xff\x04\x01\x04\x03\x08\x02\x04\x02\x04\x01\x04\x01\x00\x7f\x00\x7f\x08\x2b\x00\x81\x00\x81\x04\xd7\x00\x7f\x00\x7f\x04\x29\x0c\x09\x0c\x01\x02\x01\x02\x01\x02\x01\x02\xfb\x06\x0a\x06\x09\x06\x09\x00\x7f\x06\x0e\x02\x22\x02\xe6\x04\x1d\x06\x17\x06\x2b\x06\x08\x06\x2c\x0a\x02\x06\x08\x06\x06\x06\x0c\x06\x12\x06\x10\x0a\x25",
};
static
    struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_shash;
        char ob_sval[305];
    }
toplevel_columntable = {
    .ob_base = {
        .ob_base = {
            .ob_refcnt = 999999999,
            .ob_type = &PyBytes_Type,
        },
        .ob_size = 304,
    },
    .ob_shash = -1,
    .ob_sval = "\x01\x04\x01\x04\x01\x39\x01\x39\x01\x39\x01\x39\x01\x46\x01\x46\x01\x46\x01\x46\x01\x46\x01\x46\x01\x46\x01\x46\x01\x27\x01\x27\x01\x27\x01\x27\x01\x0c\x01\x0c\x01\x0c\x01\x0c\x01\x0b\x01\x0b\x01\x0b\x01\x0b\x01\x0f\x01\x0f\x01\x0f\x01\x0f\x01\x0b\x01\x0b\x01\x0b\x01\x0b\x01\x0c\x01\x0c\x01\x0c\x01\x0c\x01\x11\x01\x11\x01\x11\x01\x11\x0c\x1c\x1e\x2b\x0b\x2c\x01\x08\x0c\x1f\x0c\x28\x01\x09\x10\x23\x10\x33\x34\x35\x34\x36\x34\x36\x10\x37\x01\x0d\x01\x09\x01\x09\x01\x09\x01\x09\x16\x21\x01\x09\x01\x09\x18\x1a\x01\x15\x10\x14\x15\x18\x10\x19\x01\x0d\x18\x1a\x01\x15\x16\x23\x01\x13\x13\x20\x01\x10\x01\x4f\x01\x4f\x01\x4f\x01\x4f\x13\x26\x13\x34\x01\x4f\x01\x4f\x06\x0e\x11\x1f\x06\x1f\x21\x25\x27\x2b\x05\x2c\x06\x0e\x11\x1e\x06\x1e\x20\x25\x27\x2b\x05\x2c\x05\x1a\x05\x1a\x14\x02\x01\x11\x01\x35\x01\x35\x01\x35\x01\x22\x01\x22\x01\x22\x01\x10\x01\x10\x01\x10\x01\x11\x01\x11\x01\x11\x05\x2f\x01\x0c\x13\x18\x01\x10\x01\x16\x01\x16\x01\x16\x01\x25\x01\x25\x01\x25\x01\x1d\x01\x1d\x01\x1d\x01\x10\x01\x10\x01\x10\x0e\x12\x13\x22\x13\x2b\x0e\x2c\x01\x0b\x01\x12\x01\x12\x01\x12\x01\x40\x01\x40\x01\x40\x01\x15\x01\x15\x01\x15\x01\x14\x01\x14\x01\x14\x01\x32\x01\x32\x01\x32\x01\x53\x01\x53\x01\x53\x01\x53\x01\x53",
};
static struct PyCodeObject toplevel = {
    .ob_base = {
        .ob_refcnt = 999999999,
        .ob_type = &PyCode_Type,
    },
    .co_consts = & toplevel_consts._object.ob_base.ob_base,
    .co_names = & toplevel_names._object.ob_base.ob_base,
    .co_firstinstr = (_Py_CODEUNIT *) & toplevel_code.ob_sval,
    .co_exceptiontable = & toplevel_consts_7_exceptiontable.ob_base.ob_base,
    .co_flags = 0,
    .co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,
    .co_argcount = 0,
    .co_posonlyargcount = 0,
    .co_kwonlyargcount = 0,
    .co_stacksize = 4,
    .co_firstlineno = 1,
    .co_code = & toplevel_code.ob_base.ob_base,
    .co_localsplusnames = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_localspluskinds = & toplevel_consts_7_exceptiontable.ob_base.ob_base,
    .co_filename = & toplevel_consts_7_filename._ascii.ob_base,
    .co_name = & toplevel_name._ascii.ob_base,
    .co_qualname = & toplevel_name._ascii.ob_base,
    .co_linetable = & toplevel_linetable.ob_base.ob_base,
    .co_endlinetable = & toplevel_endlinetable.ob_base.ob_base,
    .co_columntable = & toplevel_columntable.ob_base.ob_base,
    .co_nlocalsplus = 0,
    .co_nlocals = 0,
    .co_nplaincellvars = 0,
    .co_ncellvars = 0,
    .co_nfreevars = 0,
    .co_varnames = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_cellvars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
    .co_freevars = & toplevel_consts_7_varnames._object.ob_base.ob_base,
};

static void do_patchups() {
    if (sizeof(wchar_t) == 2) {
        toplevel_consts_22._compact._base.wstr = (wchar_t *) toplevel_consts_22._data;
        toplevel_consts_22._compact.wstr_length = 256;
    }
}

PyObject *
_Py_get_zipimport_toplevel(void)
{
    do_patchups();
    return (PyObject *) &toplevel;
}

