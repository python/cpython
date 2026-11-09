# Imported by test_initializing_module_is_still_tracked.  The module it imports
# lazily imports this one back while this one is still initializing.
import test.test_lazy_import.data.lazy_on_init_fails
raise ValueError("initialization failed")
