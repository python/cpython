Issue #18018: Import raises ImportError instead of SystemError if a relative
import is attempted without a known parent package.