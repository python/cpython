# Filter out warnings about signed/unsigned constants
import warnings
warnings.filterwarnings("ignore", "", FutureWarning, ".*Controls")
warnings.filterwarnings("ignore", "", FutureWarning, ".*MacTextEditor")
