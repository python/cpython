- Issue #23441: rcompleter now prints a tab character instead of displaying
  possible completions for an empty word.  Initial patch by Martin Sekera.

- Issue #24683: Fixed crashes in _json functions called with arguments of
  inappropriate type.

- Issue #21697: shutil.copytree() now correctly handles symbolic links that
  point to directories.  Patch by Eduardo Seabra and Thomas Kluyver.

- Issue #14373: Fixed segmentation fault when gc.collect() is called during
  constructing lru_cache (C implementation).

- Issue #24695: Fix a regression in traceback.print_exception().  If
  exc_traceback is None we shouldn't print a traceback header like described
  in the documentation.

- Issue #24620: Random.setstate() now validates the value of state last element.

- Issue #22485: Fixed an issue that caused `inspect.getsource` to return
  incorrect results on nested functions.

- Issue #22153: Improve unittest docs. Patch from Martin Panter and evilzero.

- Issue #24580: Symbolic group references to open group in re patterns now are
  explicitly forbidden as well as numeric group references.

- Issue #24206: Fixed __eq__ and __ne__ methods of inspect classes.

- Issue #24631: Fixed regression in the timeit module with multiline setup.

- Issue #18622: unittest.mock.mock_open().reset_mock would recurse infinitely.
  Patch from Nicola Palumbo and Laurent De Buyst.

- Issue #23661: unittest.mock side_effects can now be exceptions again. This
  was a regression vs Python 3.4. Patch from Ignacio Rossi

- Issue #24608: chunk.Chunk.read() now always returns bytes, not str.

- Issue #18684: Fixed reading out of the buffer in the re module.

- Issue #24259: tarfile now raises a ReadError if an archive is truncated
  inside a data segment.

- Issue #15014: SMTP.auth() and SMTP.login() now support RFC 4954's optional
  initial-response argument to the SMTP AUTH command.

- Issue #24669: Fix inspect.getsource() for 'async def' functions.
  Patch by Kai Groner.

- Issue #24688: ast.get_docstring() for 'async def' functions.

