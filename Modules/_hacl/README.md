# Algorithm implementations used by the `hashlib` module.

This code comes from the
[HACL\*](https://github.com/hacl-star/hacl-star/) project.

HACL\* is a cryptographic library that has been formally verified for memory
safety, functional correctness, and secret independence.

## Updating HACL*

Use the `refresh.sh` script in this directory to pull in a new upstream code
version.  The upstream git hash used for the most recent code pull is recorded
in the script.  Modify the script as needed to bring in more if changes are
needed based on upstream code refactoring.

Never manually edit HACL\* files. Always add transformation shell code to the
`refresh.sh` script to perform any necessary edits. If there are serious code
changes needed, work with the upstream repository.

## Local files

1. `./include/python_hacl_namespaces.h`
1. `./README.md`
1. `./refresh.sh`

## ACKS

* Jonathan Protzenko aka [@msprotz on Github](https://github.com/msprotz)
contributed our HACL\* based builtin code.
