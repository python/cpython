Issue #25227: Optimize ASCII and latin1 encoders with the ``surrogateescape``
error handler: the encoders are now up to 3 times as fast. Initial patch
written by Serhiy Storchaka.