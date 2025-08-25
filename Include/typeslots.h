/* Do not renumber the file; these numbers are part of the stable ABI. */
#define Py_bf_getbuffer 1
#define Py_bf_releasebuffer 2
#define Py_mp_ass_subscript 3
#define Py_mp_length 4
#define Py_mp_subscript 5
#define Py_nb_absolute 6
#define Py_nb_add 7
#define Py_nb_and 8
#define Py_nb_bool 9
#define Py_nb_divmod 10
#define Py_nb_float 11
#define Py_nb_floor_divide 12
#define Py_nb_index 13
#define Py_nb_inplace_add 14
#define Py_nb_inplace_and 15
#define Py_nb_inplace_floor_divide 16
#define Py_nb_inplace_lshift 17
#define Py_nb_inplace_multiply 18
#define Py_nb_inplace_or 19
#define Py_nb_inplace_power 20
#define Py_nb_inplace_remainder 21
#define Py_nb_inplace_rshift 22
#define Py_nb_inplace_subtract 23
#define Py_nb_inplace_true_divide 24
#define Py_nb_inplace_xor 25
#define Py_nb_int 26
#define Py_nb_invert 27
#define Py_nb_lshift 28
#define Py_nb_multiply 29
#define Py_nb_negative 30
#define Py_nb_or 31
#define Py_nb_positive 32
#define Py_nb_power 33
#define Py_nb_remainder 34
#define Py_nb_rshift 35
#define Py_nb_subtract 36
#define Py_nb_true_divide 37
#define Py_nb_xor 38
#define Py_sq_ass_item 39
#define Py_sq_concat 40
#define Py_sq_contains 41
#define Py_sq_inplace_concat 42
#define Py_sq_inplace_repeat 43
#define Py_sq_item 44
#define Py_sq_length 45
#define Py_sq_repeat 46
#define Py_tp_alloc 47
#define Py_tp_base 48
#define Py_tp_bases 49
#define Py_tp_call 50
#define Py_tp_clear 51
#define Py_tp_dealloc 52
#define Py_tp_del 53
#define Py_tp_descr_get 54
#define Py_tp_descr_set 55
#define Py_tp_doc 56
#define Py_tp_getattr 57
#define Py_tp_getattro 58
#define Py_tp_hash 59
#define Py_tp_init 60
#define Py_tp_is_gc 61
#define Py_tp_iter 62
#define Py_tp_iternext 63
#define Py_tp_methods 64
#define Py_tp_new 65
#define Py_tp_repr 66
#define Py_tp_richcompare 67
#define Py_tp_setattr 68
#define Py_tp_setattro 69
#define Py_tp_str 70
#define Py_tp_traverse 71
#define Py_tp_members 72
#define Py_tp_getset 73
#define Py_tp_free 74
#define Py_nb_matrix_multiply 75
#define Py_nb_inplace_matrix_multiply 76
#define Py_am_await 77
#define Py_am_aiter 78
#define Py_am_anext 79
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03050000
/* New in 3.5 */
#define Py_tp_finalize 80
#endif
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030A0000
/* New in 3.10 */
#define Py_am_send 81
#endif
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030E0000
/* New in 3.14 */
#define Py_tp_vectorcall 82
#endif
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030E0000
/* New in 3.14 */
#define Py_tp_token 83
#endif
