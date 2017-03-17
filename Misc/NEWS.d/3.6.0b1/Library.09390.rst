Issue #27776: The :func:`os.urandom` function does now block on Linux 3.17
and newer until the system urandom entropy pool is initialized to increase
the security. This change is part of the :pep:`524`.