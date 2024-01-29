Protect :mod:`zipfile` from "quoted-overlap" zipbomb. It now raises
BadZipFile when try to read an entry that overlaps with other entry or
central directory.
