/* Do not renumber the file; these numbers are part of the stable ABI. */
#if defined(Py_LIMITED_API)
#undef Py_type_name
#undef Py_type_basicsize
#undef Py_type_itemsize
#undef Py_type_dealloc
#undef Py_type_vectorcall_offset
#undef Py_type_getattr
#undef Py_type_setattr
#undef Py_type_repr
#undef Py_type_hash
#undef Py_type_call
#undef Py_type_str
#undef Py_type_getattro
#undef Py_type_setattro
#undef Py_type_flags
#undef Py_type_doc
#undef Py_type_traverse
#undef Py_type_clear
#undef Py_type_richcompare
#undef Py_type_weaklistoffset
#undef Py_type_iter
#undef Py_type_iternext
#undef Py_type_methods
#undef Py_type_members
#undef Py_type_getset
#undef Py_type_base
#undef Py_type_dict
#undef Py_type_descr_get
#undef Py_type_descr_set
#undef Py_type_dictoffset
#undef Py_type_init
#undef Py_type_alloc
#undef Py_type_new
#undef Py_type_free
#undef Py_type_is_gc
#undef Py_type_bases
#undef Py_type_mro
#undef Py_type_cache
#undef Py_type_subclasses
#undef Py_type_weaklist
#undef Py_type_del
#undef Py_type_finalize
#undef Py_type_as_async
#undef Py_type_am_await
#undef Py_type_am_aiter
#undef Py_type_am_anext
#undef Py_type_as_number
#undef Py_type_nb_add
#undef Py_type_nb_subtract
#undef Py_type_nb_multiply
#undef Py_type_nb_remainder
#undef Py_type_nb_divmod
#undef Py_type_nb_power
#undef Py_type_nb_negative
#undef Py_type_nb_positive
#undef Py_type_nb_absolute
#undef Py_type_nb_bool
#undef Py_type_nb_invert
#undef Py_type_nb_lshift
#undef Py_type_nb_rshift
#undef Py_type_nb_and
#undef Py_type_nb_xor
#undef Py_type_nb_or
#undef Py_type_nb_int
#undef Py_type_nb_reserved
#undef Py_type_nb_float
#undef Py_type_nb_inplace_add
#undef Py_type_nb_inplace_subtract
#undef Py_type_nb_inplace_multiply
#undef Py_type_nb_inplace_remainder
#undef Py_type_nb_inplace_power
#undef Py_type_nb_inplace_lshift
#undef Py_type_nb_inplace_rshift
#undef Py_type_nb_inplace_and
#undef Py_type_nb_inplace_xor
#undef Py_type_nb_inplace_or
#undef Py_type_nb_floor_divide
#undef Py_type_nb_true_divide
#undef Py_type_nb_inplace_floor_divide
#undef Py_type_nb_inplace_true_divide
#undef Py_type_nb_index
#undef Py_type_nb_matrix_multiply
#undef Py_type_nb_inplace_matrix_multiply
#undef Py_type_as_sequence
#undef Py_type_sq_length
#undef Py_type_sq_concat
#undef Py_type_sq_repeat
#undef Py_type_sq_item
#undef Py_type_sq_ass_item
#undef Py_type_sq_contains
#undef Py_type_sq_inplace_concat
#undef Py_type_sq_inplace_repeat
#undef Py_type_as_mapping
#undef Py_type_mp_length
#undef Py_type_mp_subscript
#undef Py_type_mp_ass_subscript
#undef Py_type_as_buffer
#undef Py_type_bf_getbuffer
#undef Py_type_bf_releasebuffer
#else
/* Define STATIC_TYPE_OFFSET marco to avoid the duplicated number
 * with typeslots.h. */
#define STATIC_TYPE_OFFSET 500
#define Py_type_name (1 + STATIC_TYPE_OFFSET)
#define Py_type_basicsize (2 + STATIC_TYPE_OFFSET)
#define Py_type_itemsize (3 + STATIC_TYPE_OFFSET)
#define Py_type_dealloc (4 + STATIC_TYPE_OFFSET)
#define Py_type_vectorcall_offset (5 + STATIC_TYPE_OFFSET)
#define Py_type_getattr (6 + STATIC_TYPE_OFFSET)
#define Py_type_setattr (7 + STATIC_TYPE_OFFSET)
#define Py_type_repr (8 + STATIC_TYPE_OFFSET)
#define Py_type_hash (9 + STATIC_TYPE_OFFSET)
#define Py_type_call (10 + STATIC_TYPE_OFFSET)
#define Py_type_str (11 + STATIC_TYPE_OFFSET)
#define Py_type_getattro (12 + STATIC_TYPE_OFFSET)
#define Py_type_setattro (13 + STATIC_TYPE_OFFSET)
#define Py_type_flags (14 + STATIC_TYPE_OFFSET)
#define Py_type_doc (15 + STATIC_TYPE_OFFSET)
#define Py_type_traverse (16 + STATIC_TYPE_OFFSET)
#define Py_type_clear (17 + STATIC_TYPE_OFFSET)
#define Py_type_richcompare (18 + STATIC_TYPE_OFFSET)
#define Py_type_weaklistoffset (19 + STATIC_TYPE_OFFSET)
#define Py_type_iter (20 + STATIC_TYPE_OFFSET)
#define Py_type_iternext (21 + STATIC_TYPE_OFFSET)
#define Py_type_methods (22 + STATIC_TYPE_OFFSET)
#define Py_type_members (23 + STATIC_TYPE_OFFSET)
#define Py_type_getset (24 + STATIC_TYPE_OFFSET)
#define Py_type_base (25 + STATIC_TYPE_OFFSET)
#define Py_type_dict (26 + STATIC_TYPE_OFFSET)
#define Py_type_descr_get (27 + STATIC_TYPE_OFFSET)
#define Py_type_descr_set (28 + STATIC_TYPE_OFFSET)
#define Py_type_dictoffset (29 + STATIC_TYPE_OFFSET)
#define Py_type_init (30 + STATIC_TYPE_OFFSET)
#define Py_type_alloc (31 + STATIC_TYPE_OFFSET)
#define Py_type_new (32 + STATIC_TYPE_OFFSET)
#define Py_type_free (33 + STATIC_TYPE_OFFSET)
#define Py_type_is_gc (34 + STATIC_TYPE_OFFSET)
#define Py_type_bases (35 + STATIC_TYPE_OFFSET)
#define Py_type_mro (36 + STATIC_TYPE_OFFSET)
#define Py_type_cache (37 + STATIC_TYPE_OFFSET)
#define Py_type_subclasses (38 + STATIC_TYPE_OFFSET)
#define Py_type_weaklist (39 + STATIC_TYPE_OFFSET)
#define Py_type_del (40 + STATIC_TYPE_OFFSET)
#define Py_type_finalize (41 + STATIC_TYPE_OFFSET)
#define Py_type_as_async (42 + STATIC_TYPE_OFFSET)
#define Py_type_am_await (43 + STATIC_TYPE_OFFSET)
#define Py_type_am_aiter (44 + STATIC_TYPE_OFFSET)
#define Py_type_am_anext (45 + STATIC_TYPE_OFFSET)
#define Py_type_as_number (46 + STATIC_TYPE_OFFSET)
#define Py_type_nb_add (47 + STATIC_TYPE_OFFSET)
#define Py_type_nb_subtract (48 + STATIC_TYPE_OFFSET)
#define Py_type_nb_multiply (49 + STATIC_TYPE_OFFSET)
#define Py_type_nb_remainder (50 + STATIC_TYPE_OFFSET)
#define Py_type_nb_divmod (51 + STATIC_TYPE_OFFSET)
#define Py_type_nb_power (52 + STATIC_TYPE_OFFSET)
#define Py_type_nb_negative (53 + STATIC_TYPE_OFFSET)
#define Py_type_nb_positive (54 + STATIC_TYPE_OFFSET)
#define Py_type_nb_absolute (55 + STATIC_TYPE_OFFSET)
#define Py_type_nb_bool (56 + STATIC_TYPE_OFFSET)
#define Py_type_nb_invert (57 + STATIC_TYPE_OFFSET)
#define Py_type_nb_lshift (58 + STATIC_TYPE_OFFSET)
#define Py_type_nb_rshift (59 + STATIC_TYPE_OFFSET)
#define Py_type_nb_and (60 + STATIC_TYPE_OFFSET)
#define Py_type_nb_xor (61 + STATIC_TYPE_OFFSET)
#define Py_type_nb_or (62 + STATIC_TYPE_OFFSET)
#define Py_type_nb_int (63 + STATIC_TYPE_OFFSET)
#define Py_type_nb_reserved (64 + STATIC_TYPE_OFFSET)
#define Py_type_nb_float (65 + STATIC_TYPE_OFFSET)
#define Py_type_nb_inplace_add (66 + STATIC_TYPE_OFFSET)
#define Py_type_nb_inplace_subtract (67 + STATIC_TYPE_OFFSET)
#define Py_type_nb_inplace_multiply (68 + STATIC_TYPE_OFFSET)
#define Py_type_nb_inplace_remainder (69 + STATIC_TYPE_OFFSET)
#define Py_type_nb_inplace_power (70 + STATIC_TYPE_OFFSET)
#define Py_type_nb_inplace_lshift (71 + STATIC_TYPE_OFFSET)
#define Py_type_nb_inplace_rshift (72 + STATIC_TYPE_OFFSET)
#define Py_type_nb_inplace_and (73 + STATIC_TYPE_OFFSET)
#define Py_type_nb_inplace_xor (74 + STATIC_TYPE_OFFSET)
#define Py_type_nb_inplace_or (75 + STATIC_TYPE_OFFSET)
#define Py_type_nb_floor_divide (76 + STATIC_TYPE_OFFSET)
#define Py_type_nb_true_divide (77 + STATIC_TYPE_OFFSET)
#define Py_type_nb_inplace_floor_divide (78 + STATIC_TYPE_OFFSET)
#define Py_type_nb_inplace_true_divide (79 + STATIC_TYPE_OFFSET)
#define Py_type_nb_index (80 + STATIC_TYPE_OFFSET)
#define Py_type_nb_matrix_multiply (81 + STATIC_TYPE_OFFSET)
#define Py_type_nb_inplace_matrix_multiply (82 + STATIC_TYPE_OFFSET)
#define Py_type_as_sequence (83 + STATIC_TYPE_OFFSET)
#define Py_type_sq_length (84 + STATIC_TYPE_OFFSET)
#define Py_type_sq_concat (85 + STATIC_TYPE_OFFSET)
#define Py_type_sq_repeat (86 + STATIC_TYPE_OFFSET)
#define Py_type_sq_item (87 + STATIC_TYPE_OFFSET)
#define Py_type_sq_ass_item (88 + STATIC_TYPE_OFFSET)
#define Py_type_sq_contains (89 + STATIC_TYPE_OFFSET)
#define Py_type_sq_inplace_concat (90 + STATIC_TYPE_OFFSET)
#define Py_type_sq_inplace_repeat (91 + STATIC_TYPE_OFFSET)
#define Py_type_as_mapping (92 + STATIC_TYPE_OFFSET)
#define Py_type_mp_length (93 + STATIC_TYPE_OFFSET)
#define Py_type_mp_subscript (94 + STATIC_TYPE_OFFSET)
#define Py_type_mp_ass_subscript (95 + STATIC_TYPE_OFFSET)
#define Py_type_as_buffer (96 + STATIC_TYPE_OFFSET)
#define Py_type_bf_getbuffer (97 + STATIC_TYPE_OFFSET)
#define Py_type_bf_releasebuffer (98 + STATIC_TYPE_OFFSET)
#endif
