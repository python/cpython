Issue #23034: The output of a special Python build with defined COUNT_ALLOCS,
SHOW_ALLOC_COUNT or SHOW_TRACK_COUNT macros is now off by  default.  It can
be re-enabled using the "-X showalloccount" option.  It now outputs to stderr
instead of stdout.