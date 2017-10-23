Experiments in concolic testing with Klee
=========================================

Scripts will assume that the root of this directory contains a `klee/` folder
with:

- `klee/bin/`: `klee` executables
- `klee/include/`: `klee` headers

    mkdir klee/
    ln -s /path/to/klee/include klee/include
    ln -s /path/to/klee/build/bin klee/bin

In each directory,

- `make` builds the files;
- `make klee` starts testing.
