#ifndef _Py_HAVE_SLOTS_H
#define _Py_HAVE_SLOTS_H

typedef void (*_Py_funcptr_t)(void);

struct PySlot {
    uint16_t sl_id;
    uint16_t sl_flags;
    _Py_ANONYMOUS union {
        uint32_t sl_reserved;
    };
    _Py_ANONYMOUS union {
        void *sl_ptr;
        _Py_funcptr_t sl_func;
        Py_ssize_t sl_size;
        int64_t sl_int64;
        uint64_t sl_uint64;
    };
};

#define PySlot_OPTIONAL 0x01
#define PySlot_STATIC 0x02
#define PySlot_INTPTR 0x04

#define Py_slot_invalid 0xffff

#define PySlot_DATA(NAME, VALUE) \
    {.sl_id=(NAME), .sl_flags=PySlot_INTPTR, .sl_ptr=(void*)(VALUE)}

#define PySlot_FUNC(NAME, VALUE) \
    {.sl_id=(NAME), .sl_func=(_Py_funcptr_t)(VALUE)}

#define PySlot_SIZE(NAME, VALUE) \
    {.sl_id=(NAME), .sl_size=(Py_ssize_t)(VALUE)}

#define PySlot_INT64(NAME, VALUE) \
    {.sl_id=(NAME), .sl_int64=(int64_t)(VALUE)}

#define PySlot_UINT64(NAME, VALUE) \
    {.sl_id=(NAME), .sl_uint64=(uint64_t)(VALUE)}

#define PySlot_STATIC_DATA(NAME, VALUE) \
    {.sl_id=(NAME), .sl_flags=PySlot_STATIC, .sl_ptr=(VALUE)}

#define PySlot_END {0}


#define PySlot_PTR(NAME, VALUE) \
    {(NAME), PySlot_INTPTR, {0}, {(void*)(VALUE)}}

#define PySlot_PTR_STATIC(NAME, VALUE) \
    {(NAME), PySlot_INTPTR | PySlot_STATIC, {0}, {(void*)(VALUE)}}

#endif  // _Py_HAVE_SLOTS_H
