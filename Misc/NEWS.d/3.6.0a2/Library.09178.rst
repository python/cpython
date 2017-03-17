Comment out socket (SO_REUSEPORT) and posix (O_SHLOCK, O_EXLOCK) constants
exposed on the API which are not implemented on GNU/Hurd. They would not
work at runtime anyway.