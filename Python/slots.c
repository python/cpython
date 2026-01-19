/* Common handling of type/module slots
 */

#include "Python.h"

#include "pycore_slots.h"           // _PySlot_Info

#include <stdio.h>

// Iterating through a recursive structure doesn't look great in a debugger.
// Define this to get a trace on stderr.
// (The messages can also serve as code comments.)
#if 0
#define MSG(...) { \
    fprintf(stderr, "slotiter: " __VA_ARGS__); fprintf(stderr, "\n");}
#else
#define MSG(...)
#endif

static int validate_current_slot(_PySlotIterator *it);

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
    return "<thing>";
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
    it->state->ignoring_fallbacks = false;
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
    assert (!it->state->ignoring_fallbacks);
    it->is_at_end = false;
    it->state->any_slot = slots;
    it->is_first_run = false;
}

static Py_ssize_t
seen_index(uint16_t id)
{
    return id / sizeof(unsigned int);
}

static unsigned int
seen_mask(uint16_t id)
{
    return ((unsigned int)1) << (id % sizeof(unsigned int));
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

        /* Convert legacy structure */
        switch (it->state->slot_struct_kind) {
            case _PySlot_KIND_SLOT: {
                MSG("copying PySlot structure");
                it->current = *it->state->slot;  /* struct copy */
            } break;
            case _PySlot_KIND_TYPE: {
                MSG("converting PyType_Slot structure");
                memset(&it->current, 0, sizeof(it->current));
                it->current.sl_id = it->state->tp_slot->slot;
                it->current.sl_flags = PySlot_INTPTR;
                it->current.sl_ptr = (void*)it->state->tp_slot->pfunc;
            } break;
            case _PySlot_KIND_MOD: {
                MSG("converting PyModuleDef_Slot structure");
                memset(&it->current, 0, sizeof(it->current));
                it->current.sl_id = it->state->mod_slot->slot;
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

        if (it->state->ignoring_fallbacks) {
            if (!(flags & PySlot_HAS_FALLBACK)) {
                MSG("stopping to ignore fallbacks");
                it->state->ignoring_fallbacks = false;
            }
            MSG("skipped (ignoring fallbacks)");
            advance(it);
            continue;
        }
        if (result->sl_id >= _Py_slot_COUNT) {
            if (flags & (PySlot_OPTIONAL | PySlot_HAS_FALLBACK)) {
                MSG("skipped (unknown slot)");
                advance(it);
                continue;
            }
            MSG("error (unknown slot)");
            PyErr_Format(PyExc_SystemError,
                         "unknown slot ID %u", (unsigned int)result->sl_id);
            goto error;
        }
        if (result->sl_id == Py_slot_end) {
            flags &= ~PySlot_INTPTR;
            MSG("sentinel slot, flags %x", (unsigned)flags);
            if (flags == PySlot_OPTIONAL) {
                MSG("skipped (optional sentinel)");
                advance(it);
                continue;
            }
            const uint16_t bad_flags = PySlot_OPTIONAL | PySlot_HAS_FALLBACK;
            if (flags & bad_flags) {
                MSG("error (bad flags on sentinel)");
                PyErr_Format(PyExc_SystemError,
                            "invalid flags for Py_slot_end: 0x%x",
                             (unsigned int)flags);
                goto error;
            }
            it->state->slot = NULL;
            continue;
        }
        it->info = &_PySlot_InfoTable[result->sl_id];
        MSG("slot %d: %s", (int)result->sl_id, it->info->name);

        if (it->is_first_run && it->info->is_name) {
            MSG("setting name for error messages");
            assert(it->info->dtype == _PySlot_TYPE_PTR);
            it->name = result->sl_ptr;
        }

        // Resolve a legacy ambiguous slot number.
        // Save the original slot info for error messages.
        uint16_t orig_id = result->sl_id;
        _PySlot_Info *orig_info = &_PySlot_InfoTable[result->sl_id];
        if (it->info->kind == _PySlot_KIND_COMPAT) {
            MSG("resolving compat slot");
            switch (it->kind) {
                case _PySlot_KIND_TYPE: {
                    result->sl_id = it->info->compat_info.type_id;
                } break;
                case _PySlot_KIND_MOD: {
                    result->sl_id = it->info->compat_info.mod_id;
                } break;
                default: {
                    Py_UNREACHABLE();
                } break;
            }
            it->info = &_PySlot_InfoTable[result->sl_id];
            MSG("slot %d: %s", (int)result->sl_id, it->info->name);
        }

        if (it->info->is_subslots) {
            if (result->sl_ptr == NULL) {
                MSG("NULL subslots; skipping");
                advance(it);
                continue;
            }
            it->recursion_level++;
            MSG("recursing into level %d", it->recursion_level);
            if (it->recursion_level >= _PySlot_MAX_NESTING) {
                MSG("error (too much nesting)");
                PyErr_Format(PyExc_SystemError,
                            "%s (slot %d): too many levels of nested slots",
                            orig_info->name, orig_id);
                goto error;
            }
            it->state = &it->states[it->recursion_level];
            memset(it->state, 0, sizeof(_PySlotIterator_state));
            it->state->slot = result->sl_ptr;
            it->state->slot_struct_kind = it->info->kind;
            continue;
        }

        if (flags & PySlot_INTPTR) {
            MSG("casting from intptr");
            switch (it->info->dtype) {
                case _PySlot_TYPE_SIZE: {
                    result->sl_size = (Py_ssize_t)(intptr_t)result->sl_ptr;
                } break;
                case _PySlot_TYPE_INT64: {
                    result->sl_int64 = (int64_t)(intptr_t)result->sl_ptr;
                } break;
                case _PySlot_TYPE_UINT64: {
                    result->sl_uint64 = (uint64_t)(intptr_t)result->sl_ptr;
                } break;
                case _PySlot_TYPE_PTR:
                case _PySlot_TYPE_FUNC:
                case _PySlot_TYPE_VOID:
                    break;
            }
        }

        if (flags & PySlot_HAS_FALLBACK) {
            MSG("starting to ignore fallbacks");
            it->state->ignoring_fallbacks = true;
        }

        advance(it);
        MSG("result: %d (%s)", (int)result->sl_id, it->info->name);
        assert (result->sl_id > 0);
        assert (result->sl_id <= _Py_slot_COUNT);
        assert (result->sl_id <= INT_MAX);
        if (it->is_first_run && validate_current_slot(it) < 0) {
            goto error;
        }
        return result->sl_id != Py_slot_end;
    }
    Py_UNREACHABLE();

error:
    it->current.sl_id = Py_slot_invalid;
    return true;
}

static int
validate_current_slot(_PySlotIterator *it)
{
    const _PySlot_Info *info = it->info;
    int id = it->current.sl_id;

    if (it->info->kind != it->kind) {
        MSG("error (bad slot kind)");
        PyErr_Format(PyExc_SystemError,
                        "%s (slot %d) is not compatible with %ss",
                        info->name,
                        id,
                        kind_name(it->kind));
        return -1;
    }

    if (it->info->null_handling != _PySlot_PROBLEM_ALLOW) {
        bool is_null = false;
        switch (it->info->dtype) {
            case _PySlot_TYPE_PTR: {
                is_null = it->current.sl_ptr == NULL;
            } break;
            case _PySlot_TYPE_FUNC: {
                is_null = it->current.sl_func == NULL;
            } break;
            default: {
                Py_UNREACHABLE();
            } break;
        }
        if (is_null) {
            MSG("slot is NULL but shouldn't");
            if (it->info->null_handling == _PySlot_PROBLEM_REJECT) {
                MSG("error (NULL rejected)");
                PyErr_Format(PyExc_SystemError,
                             "NULL not allowed for slot Py_%s",
                             it->info->name);
                return -1;
            }
            MSG("deprecated NULL");
            if (PyErr_WarnFormat(
                PyExc_DeprecationWarning,
                1,
                "NULL value in slot %s is deprecated",
                it->info->name) < 0)
            {
                return -1;
            }
        }
    }

    if (info->duplicate_handling != _PySlot_PROBLEM_ALLOW) {
        if (_PySlotIterator_SawSlot(it, id)) {
            MSG("slot was seen before but shouldn't be duplicated");
            if (info->duplicate_handling == _PySlot_PROBLEM_REJECT) {
                MSG("error (duplicate rejected)");
                PyErr_Format(
                    PyExc_SystemError,
                    "%s%s%s has multiple %s (%d) slots",
                    kind_name(it->kind),
                    it->name ? " " : "",
                    it->name ? it->name : "",
                    info->name,
                    (int)it->current.sl_id);
                return -1;
            }
            MSG("deprecated duplicate");
            if (PyErr_WarnFormat(
                    PyExc_DeprecationWarning,
                    0,
                    "%s%s%s has multiple %s (%d) slots. This is deprecated.",
                    kind_name(it->kind),
                    it->name ? " " : "",
                    it->name ? it->name : "",
                    info->name,
                    (int)it->current.sl_id) < 0) {
                return -1;
            }
        }
    }
    it->seen[seen_index(id)] |= seen_mask(id);
    return 0;
}
