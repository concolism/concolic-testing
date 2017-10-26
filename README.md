Experiments in concolic testing with Klee
=========================================

## Initial setup

Scripts will assume that the root of this directory contains a `klee/` folder
with:

- `klee/bin/`: `klee` executables
- `klee/lib/`: `klee` libraries
- `klee/include/`: `klee` headers

A typical sequence of commands to get to a working setup:

```
mkdir klee/
ln -s /path/to/klee/include klee/include
ln -s /path/to/klee/build/bin klee/bin
ln -s /path/to/klee/build/lib klee/lib
```

We work with `clang-5.0`. The programs are probably simple enough to run on
older versions though.

## Running the examples

In each directory,

- `make` builds the program for Klee;
- `make klee` starts testing;
- `make replay` builds an executable to replay test cases.

These examples have various buggy versions.
See `Makefile` in each directory for options.

For example, in `aeson-cbits`, this enables the `BUG_DEST_TOO_LONG`.

```
make DEST_TOO_LONG=true klee
```

### Misc commands

- `make cpp` just preprocesses the files (for sanity checks).
