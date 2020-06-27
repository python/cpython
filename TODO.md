# Things remaining to do for final submission

## Mentor
* Check with python mentor on infinite loop

## Functionality
* Allow `distutils.copytree` to update if there are symlinks to be overwritten.
* Check POSIX and GNU `ln` behaviour regarding `os.link`'s `follow_symlinks=True`

## Code
* Replace [`distutils.copytree` usage of os.symlink](https://github.com/python/cpython/blob/master/Lib/distutils/dir_util.py#L152) with `shutil.symlink`
* Comment and write complete tests for `shutil.link`
* Investigate other possible standard library usages of `os.{sym,}link`
* Check max number of columns to use

## Non-code
* Complete tests for `link`
* Move Doc/whatsnew entry to latest version
* Implement https://devguide.python.org/committing/#what-s-new-and-news-entries


## Documentation
* Add function documentation for shutil.link
* Add user-facing documentation for new methods
* Add suggestion to use shutil.{sym,}link rather than the `os.` versions

## Before submission
* Review https://devguide.python.org/committing/
* Run `shutil` tests:  `git sh ./python Lib/test/test_shutil.py`
* Rebase on latest upstream
* Run all tests
  * all
  * build?

## Resources relating to the PR
* [Pull Request itself](https://github.com/python/cpython/pull/14464)
* [StackOverflow issue with `distutils.dir_util.copy_tree` and overwriting symlinks](https://stackoverflow.com/questions/53090360/python-distutils-copy-tree-fails-to-update-if-there-are-symlinks)
* [Positive support from 2 core devs](https://code.activestate.com/lists/python-ideas/56421/)
* [Initial post to python-ideas](https://code.activestate.com/lists/python-ideas/55992/)
* [Python bpo-36656](https://bugs.python.org/issue36656)

