Issue #25003: On Solaris 11.3 or newer, os.urandom() now uses the
getrandom() function instead of the getentropy() function. The getentropy()
function is blocking to generate very good quality entropy, os.urandom()
doesn't need such high-quality entropy.