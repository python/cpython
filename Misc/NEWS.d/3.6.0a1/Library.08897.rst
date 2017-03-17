Issue #25447: Copying the lru_cache() wrapper object now always works,
independently from the type of the wrapped object (by returning the original
object unchanged).