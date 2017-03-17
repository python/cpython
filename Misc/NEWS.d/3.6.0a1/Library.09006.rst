Issue #26915:  The __contains__ methods in the collections ABCs now check
for identity before checking equality.  This better matches the behavior
of the concrete classes, allows sensible handling of NaNs, and makes it
easier to reason about container invariants.