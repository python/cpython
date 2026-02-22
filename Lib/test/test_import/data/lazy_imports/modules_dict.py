lazy import test.test_import.data.lazy_imports.basic2 as basic2

import sys
mod = sys.modules[__name__]
x = mod.__dict__
