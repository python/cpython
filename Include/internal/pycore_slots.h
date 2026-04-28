#ifndef _Py_PYCORE_SLOTS_H
#define _Py_PYCORE_SLOTS_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include <stdbool.h>

/* Slot data type */
typedef enum _PySlot_DTYPE {
    _PySlot_DTYPE_VOID,
    _PySlot_DTYPE_FUNC,
    _PySlot_DTYPE_PTR,
    _PySlot_DTYPE_SIZE,
    _PySlot_DTYPE_INT64,
    _PySlot_DTYPE_UINT64,
}_PySlot_DTYPE;

/* Slot kind, used to identify:
 * - the thing the slot initializes (type/module/special)
 * - the struct type (PySlot/PyType_Slot/PyModuleDef_Slot)
 */
typedef enum _PySlot_KIND {
    _PySlot_KIND_TYPE,
    _PySlot_KIND_MOD,
    _PySlot_KIND_COMPAT,
    _PySlot_KIND_SLOT,
} _PySlot_KIND;

typedef enum _PySlot_PROBLEM_HANDLING {
    _PySlot_PROBLEM_ALLOW = 0,
    _PySlot_PROBLEM_DEPRECATED,
    _PySlot_PROBLEM_REJECT,
} _PySlot_PROBLEM_HANDLING;

PyAPI_DATA(const char *const) _PySlot_names[];

#define _PySlot_MAX_NESTING 5

/* State for one nesting level of a slots iterator */
typedef struct _PySlotIterator_state {
    union {
        // tagged by slot_struct_kind:
        const PySlot *slot;               // with _PySlot_KIND_SLOT
        const PyType_Slot *tp_slot;       // with _PySlot_KIND_TYPE
        const PyModuleDef_Slot *mod_slot; // with _PySlot_KIND_MOD
        const void *any_slot;
    };
    _PySlot_KIND slot_struct_kind;
} _PySlotIterator_state;

#define SEEN_ENTRY_BITS (8 * sizeof(unsigned int))

/* State for a slots iterator */
typedef struct {
    _PySlotIterator_state *state;
    _PySlotIterator_state states[_PySlot_MAX_NESTING];
    unsigned int seen[_Py_slot_COUNT / SEEN_ENTRY_BITS + 1];
    _PySlot_KIND kind;
    uint8_t recursion_level;
    bool is_at_end :1;
    bool is_first_run :1;

    // Name of the object (type/module) being defined, NULL if unknown.
    // Must be set by the callers as soon as it's known.
    const char *name;

    /* Output information: */

    // The slot. Always a copy; may be modified by caller of the iterator.
    PySlot current;

} _PySlotIterator;

/* Initialize an iterator using a Py_Slot array */
PyAPI_FUNC(void)
_PySlotIterator_Init(_PySlotIterator *it, const PySlot *slots,
                     _PySlot_KIND result_kind);

/* Initialize an iterator using a legacy slot array */
PyAPI_FUNC(void)
_PySlotIterator_InitLegacy(_PySlotIterator *it, const void *slots,
                           _PySlot_KIND kind);

/* Reset a *successfully exhausted* iterator to the beginning.
 * The *slots* must be the same as for the previous
 * `_PySlotIterator_InitWithKind` call.
 * (Unlike creating a new iterator, we can skip some validation after Rewind.)
 */
PyAPI_FUNC(void) _PySlotIterator_Rewind(_PySlotIterator *it, const void *slots);

/* Iteration function.
 *
 * Return false at the end (when successfully exhausted).
 * Otherwise (even on error), fill output information in `it` and return true.
 *
 * On error, set an exception and set `it->current.sl_id` to `Py_slot_invalid`.
 */
PyAPI_FUNC(bool) _PySlotIterator_Next(_PySlotIterator *it);

/* Return 1 if given slot was "seen" by an earlier _PySlotIterator_Next call.
 * (This state is not reset by rewinding.)
 */
PyAPI_FUNC(bool) _PySlotIterator_SawSlot(_PySlotIterator *, int);

static inline const char *
_PySlot_GetName(uint16_t id)
{
    if (id >= _Py_slot_COUNT) {
        return "<unknown_slot>";
    }
    if (id == Py_slot_invalid) {
        return "Py_slot_invalid";
    }
    return _PySlot_names[id];
}

static inline void
_PySlot_err_bad_slot(char *kind, uint16_t id)
{
    if (id < _Py_slot_COUNT) {
        PyErr_Format(PyExc_SystemError, "invalid %s slot %d (%s)",
                    kind, (unsigned int)id, _PySlot_names[id]);
    }
    else if (id == Py_slot_invalid) {
        PyErr_Format(PyExc_SystemError, "invalid slot (Py_slot_invalid, %u)",
                     (unsigned int)id);
    }
    else {
        PyErr_Format(PyExc_SystemError, "unknown %s slot ID %u",
                     kind, (unsigned int)id);
    }
}

#include "internal/pycore_slots_generated.h"

#endif // _Py_PYCORE_SLOTS_H
