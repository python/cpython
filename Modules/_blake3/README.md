The "Modules/\_blake3/impl" directory contains an unmodified copy of the "c"
directory from the BLAKE3 source code distribution (with the unnecessary-to-us
Rust bindings removed), along with a copy of BLAKE3's "LICENSE".
There's also a copy of BLAKE3's "test_vectors.json" in the "Lib/test" directory,
although it's been renamed to "blake3.test_vectors.json".

I got all of that from the official BLAKE3 repo:

	https://github.com/BLAKE3-team/BLAKE3/

--larry 2022/03/03
