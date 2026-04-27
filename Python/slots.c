/* Common handling of type/module slots
 */

#include "Python.h"

#include "pycore_slots.h"

#include <stdio.h>

// Iterating through a recursive structure doesn't look great in a debugger.
// Flip the #if to 1 to get a trace on stderr.
// (The messages can also serve as code comments.)
#if 0
#define MSG(...) { \
    fprintf(stderr, "slotiter: " __VA_ARGS__); fprintf(stderr, "\n");}
#else
#define MSG(...)
#endif

static char*
kind_name(_PySlot_KIND kind)
{
    switch (kind) {
        case _PySlot_KIND_TYPE: return "type";
        case _PySlot_KIND_MOD: return "module";
        case _PySlot_KIND_COMPAT: return "compat";
        case _PySlot_KIND_SLOT: return "generic slot";
    }
    Py_UNREACHABLE();
}

static void
init_with_kind(_PySlotIterator *it, const void *slots,
               _PySlot_KIND result_kind,
               _PySlot_KIND slot_struct_kind)
{
    MSG("");
    MSG("init (%s slot iterator)", kind_name(result_kind));
    it->state = it->states;
    it->state->any_slot = slots;
    it->state->slot_struct_kind = slot_struct_kind;
    it->kind = result_kind;
    it->name = NULL;
    it->recursion_level = 0;
    it->is_at_end = false;
    it->is_first_run = true;
    it->current.sl_id = 0;
    memset(it->seen, 0, sizeof(it->seen));
}

void
_PySlotIterator_Init(_PySlotIterator *it, const PySlot *slots,
                     _PySlot_KIND result_kind)
{
    init_with_kind(it, slots, result_kind, _PySlot_KIND_SLOT);
}

void
_PySlotIterator_InitLegacy(_PySlotIterator *it, const void *slots,
                           _PySlot_KIND kind)
{
    init_with_kind(it, slots, kind, kind);
}

void
_PySlotIterator_Rewind(_PySlotIterator *it, const void *slots)
{
    MSG("");
    MSG("rewind (%s slot iterator)", kind_name(it->kind));
    assert (it->is_at_end);
    assert (it->recursion_level == 0);
    assert (it->state == it->states);
    it->is_at_end = false;
    it->state->any_slot = slots;
    it->is_first_run = false;
}

static Py_ssize_t
seen_index(uint16_t id)
{
    return id / SEEN_ENTRY_BITS;
}

static unsigned int
seen_mask(uint16_t id)
{
    return ((unsigned int)1) << (id % SEEN_ENTRY_BITS);
}

bool
_PySlotIterator_SawSlot(_PySlotIterator *it, int id)
{
    assert (id > 0);
    assert (id < _Py_slot_COUNT);
    return it->seen[seen_index(id)] & seen_mask(id);
}

// Advance `it` to the next entry. Currently cannot fail.
static void
advance(_PySlotIterator *it)
{
    MSG("advance (at level %d)", (int)it->recursion_level);
    switch (it->state->slot_struct_kind) {
        case _PySlot_KIND_SLOT: it->state->slot++; break;
        case _PySlot_KIND_TYPE: it->state->tp_slot++; break;
        case _PySlot_KIND_MOD: it->state->mod_slot++; break;
        default:
            Py_UNREACHABLE();
    }
}

static int handle_first_run(_PySlotIterator *it);

bool
_PySlotIterator_Next(_PySlotIterator *it)
{
    MSG("next");
    assert(it);
    assert(!it->is_at_end);
    assert(!PyErr_Occurred());

    it->current.sl_id = -1;

    while (true) {
        if (it->state->slot == NULL) {
            if (it->recursion_level == 0) {
                MSG("end (initial nesting level done)");
                it->is_at_end = true;
                return 0;
            }
            MSG("pop nesting level %d", (int)it->recursion_level);
            it->recursion_level--;
            it->state = &it->states[it->recursion_level];
            advance(it);
            continue;
        }

        switch (it->state->slot_struct_kind) {
            case _PySlot_KIND_SLOT: {
                MSG("copying PySlot structure");
                it->current = *it->state->slot;
            } break;
            case _PySlot_KIND_TYPE: {
                MSG("converting PyType_Slot structure");
                memset(&it->current, 0, sizeof(it->current));
                it->current.sl_id = (uint16_t)it->state->tp_slot->slot;
                it->current.sl_flags = PySlot_INTPTR;
                it->current.sl_ptr = (void*)it->state->tp_slot->pfunc;
            } break;
            case _PySlot_KIND_MOD: {
                MSG("converting PyModuleDef_Slot structure");
                memset(&it->current, 0, sizeof(it->current));
                it->current.sl_id = (uint16_t)it->state->mod_slot->slot;
                it->current.sl_flags = PySlot_INTPTR;
                it->current.sl_ptr = (void*)it->state->mod_slot->value;
            } break;
            default: {
                Py_UNREACHABLE();
            } break;
        }

        /* shorter local names */
        PySlot *const result = &it->current;
        uint16_t flags = result->sl_flags;

        MSG("slot %d, flags 0x%x, from %p",
            (int)result->sl_id, (unsigned)flags, it->state->slot);

        uint16_t orig_id = result->sl_id;
        switch (it->kind) {
            case _PySlot_KIND_TYPE:
                result->sl_id = _PySlot_resolve_type_slot(result->sl_id);
                break;
            case _PySlot_KIND_MOD:
                result->sl_id = _PySlot_resolve_mod_slot(result->sl_id);
                break;
            default:
                Py_UNREACHABLE();
        }
        MSG("resolved to slot %d (%s)",
            (int)result->sl_id, _PySlot_GetName(result->sl_id));

        if (result->sl_id == Py_slot_invalid) {
            MSG("error (unknown/invalid slot)");
            if (flags & PySlot_OPTIONAL) {
                advance(it);
                continue;
            }
            _PySlot_err_bad_slot(kind_name(it->kind), orig_id);
            goto error;
        }
        if (result->sl_id == Py_slot_end) {
            MSG("sentinel slot, flags %x", (unsigned)flags);
            if (flags & PySlot_OPTIONAL) {
                MSG("error (bad flags on sentinel)");
                PyErr_Format(PyExc_SystemError,
                            "invalid flags for Py_slot_end: 0x%x",
                             (unsigned int)flags);
                goto error;
            }
            it->state->slot = NULL;
            continue;
        }

        if (result->sl_id == Py_slot_subslots
            || result->sl_id == Py_tp_slots
            || result->sl_id == Py_mod_slots
        ) {
            if (result->sl_ptr == NULL) {
                MSG("NULL subslots; skipping");
                advance(it);
                continue;
            }
            if ((it->states[0].slot_struct_kind == _PySlot_KIND_MOD)
                && (it->state->slot_struct_kind == _PySlot_KIND_SLOT)
                && !(result->sl_flags & PySlot_STATIC))
            {
                PyErr_Format(PyExc_SystemError,
                             "slots included from PyModuleDef must be static");
                goto error;
            }
            it->recursion_level++;
            MSG("recursing into level %d", it->recursion_level);
            if (it->recursion_level >= _PySlot_MAX_NESTING) {
                MSG("error (too much nesting)");
                PyErr_Format(PyExc_SystemError,
                            "%s (slot %d): too many levels of nested slots",
                            _PySlot_GetName(result->sl_id), orig_id);
                goto error;
            }
            it->state = &it->states[it->recursion_level];
            memset(it->state, 0, sizeof(_PySlotIterator_state));
            it->state->slot = result->sl_ptr;
            switch (result->sl_id) {
                case Py_slot_subslots:
                    it->state->slot_struct_kind = _PySlot_KIND_SLOT; break;
                case Py_tp_slots:
                    it->state->slot_struct_kind = _PySlot_KIND_TYPE; break;
                case Py_mod_slots:
                    it->state->slot_struct_kind = _PySlot_KIND_MOD; break;
            }
            continue;
        }

        if (flags & PySlot_INTPTR) {
            MSG("casting from intptr");
            /* this should compile to nothing on common architectures */
            switch (_PySlot_get_dtype(result->sl_id)) {
                case _PySlot_DTYPE_SIZE: {
                    result->sl_size = (Py_ssize_t)(intptr_t)result->sl_ptr;
                } break;
                case _PySlot_DTYPE_INT64: {
                    result->sl_int64 = (int64_t)(intptr_t)result->sl_ptr;
                } break;
                case _PySlot_DTYPE_UINT64: {
                    result->sl_uint64 = (uint64_t)(intptr_t)result->sl_ptr;
                } break;
                case _PySlot_DTYPE_PTR:
                case _PySlot_DTYPE_FUNC:
                case _PySlot_DTYPE_VOID:
                    break;
            }
        }

        advance(it);
        switch (_PySlot_get_dtype(result->sl_id)) {
            case _PySlot_DTYPE_VOID:
            case _PySlot_DTYPE_PTR:
                MSG("result: %d (%s): %p",
                    (int)result->sl_id, _PySlot_GetName(result->sl_id),
                    (void*)result->sl_ptr);
                break;
            case _PySlot_DTYPE_FUNC:
                MSG("result: %d (%s): %p",
                    (int)result->sl_id, _PySlot_GetName(result->sl_id),
                    (void*)result->sl_func);
                break;
            case _PySlot_DTYPE_SIZE:
                MSG("result: %d (%s): %zd",
                    (int)result->sl_id, _PySlot_GetName(result->sl_id),
                    (Py_ssize_t)result->sl_size);
                break;
            case _PySlot_DTYPE_INT64:
                MSG("result: %d (%s): %ld",
                    (int)result->sl_id,  _PySlot_GetName(result->sl_id),
                    (long)result->sl_int64);
                break;
            case _PySlot_DTYPE_UINT64:
                MSG("result: %d (%s): %lu (0x%lx)",
                    (int)result->sl_id, _PySlot_GetName(result->sl_id),
                    (unsigned long)result->sl_int64,
                    (unsigned long)result->sl_int64);
                break;
        }
        assert (result->sl_id > 0);
        assert (result->sl_id <= _Py_slot_COUNT);
        if (it->is_first_run && (handle_first_run(it) < 0)) {
            goto error;
        }
        return result->sl_id != Py_slot_end;
    }
    Py_UNREACHABLE();

error:
    it->current.sl_id = Py_slot_invalid;
    return true;
}

/* Validate current slot, and do bookkeeping */
static int
handle_first_run(_PySlotIterator *it)
{
    int id = it->current.sl_id;

    if (_PySlot_get_must_be_static(id)) {
        if (!(it->current.sl_flags & PySlot_STATIC)
            && (it->state->slot_struct_kind == _PySlot_KIND_SLOT))
        {
            PyErr_Format(
                PyExc_SystemError,
                "%s requires PySlot_STATIC",
                _PySlot_GetName(id));
            return -1;
        }
    }

    _PySlot_PROBLEM_HANDLING null_handling = _PySlot_get_null_handling(id);
    if (null_handling != _PySlot_PROBLEM_ALLOW) {
        bool is_null = false;
        switch (_PySlot_get_dtype(id)) {
            case _PySlot_DTYPE_PTR: {
                is_null = it->current.sl_ptr == NULL;
            } break;
            case _PySlot_DTYPE_FUNC: {
                is_null = it->current.sl_func == NULL;
            } break;
            default: {
                //Py_UNREACHABLE();
            } break;
        }
        if (is_null) {
            MSG("slot is NULL but shouldn't");
            if (null_handling == _PySlot_PROBLEM_REJECT) {
                MSG("error (NULL rejected)");
                PyErr_Format(PyExc_SystemError,
                             "NULL not allowed for slot %s",
                             _PySlot_GetName(id));
                return -1;
            }
            if (it->states[0].slot_struct_kind == _PySlot_KIND_SLOT) {
                MSG("deprecated NULL");
                if (PyErr_WarnFormat(
                    PyExc_DeprecationWarning,
                    1,
                    "NULL value in slot %s is deprecated",
                    _PySlot_GetName(id)) < 0)
                {
                    return -1;
                }
            }
            else {
                MSG("unwanted NULL in legacy struct");
            }
        }
    }

    _PySlot_PROBLEM_HANDLING duplicate_handling = _PySlot_get_duplicate_handling(id);
    if (duplicate_handling != _PySlot_PROBLEM_ALLOW) {
        if (_PySlotIterator_SawSlot(it, id)) {
            MSG("slot was seen before but shouldn't be duplicated");
            if (duplicate_handling == _PySlot_PROBLEM_REJECT) {
                MSG("error (duplicate rejected)");
                PyErr_Format(
                    PyExc_SystemError,
                    "%s%s%s has multiple %s (%d) slots",
                    kind_name(it->kind),
                    it->name ? " " : "",
                    it->name ? it->name : "",
                    _PySlot_GetName(id),
                    (int)it->current.sl_id);
                return -1;
            }
            if (it->states[0].slot_struct_kind == _PySlot_KIND_SLOT) {
                MSG("deprecated duplicate");
                if (PyErr_WarnFormat(
                        PyExc_DeprecationWarning,
                        0,
                        "%s%s%s has multiple %s (%d) slots. This is deprecated.",
                        kind_name(it->kind),
                        it->name ? " " : "",
                        it->name ? it->name : "",
                        _PySlot_GetName(id),
                        (int)it->current.sl_id) < 0) {
                    return -1;
                }
            }
            else {
                MSG("unwanted duplicate in legacy struct");
            }
        }
    }
    it->seen[seen_index(id)] |= seen_mask(id);
    return 0;
}
