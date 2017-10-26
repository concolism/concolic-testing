Experiments in concolic testing with Klee
=========================================

## Parsing JSON strings (`aeson-cbits`)

A function originally from
[*json-strings*](https://hackage.haskell.org/package/json-stream), a Haskell
library for JSON. Some concerns have been raised about the current lack of
safety guarantees
(for instance, see
[this issue in *aeson*](https://github.com/bos/aeson/issues/535)).

## Testing noninterference, quickly (`noninterf`)

A port of an experiment in random testing for noninterference properties of a
small family of abstract machines. (In ICFP 2013, also on
[arxiv](https://arxiv.org/abs/1409.0393?context=cs.PL)).

---

# Build information

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

- `make klee` to launch Klee:
  + `TIMEOUT=N` to set a time limit of N seconds (default: `60`);
  + `NOLIMIT` to disable timeouts.
- `make` just builds the program for Klee (implied by `make klee`).
- `make replay` builds an executable to replay test cases.

These examples have various buggy versions.
See `Makefile` in each directory for corresponding options.

For example, in `aeson-cbits`, this enables the `BUG_DEST_TOO_LONG`.

```
make DEST_TOO_LONG=true klee
```

### Misc commands

- `make cpp` just preprocesses the files (for sanity checks).

### Reminders

To use STP, [Klee docs](https://klee.github.io/build-stp/) remind you to apply this.
(In some environments, STP may segfault without it.)

```
ulimit -s unlimited
```

To replay tests, add `libKleeRuntest` to the `LD_LIBRARY_PATH`, and set `KTEST_FILE` to
a `.ktest` file.

```
LD_LIBRARY_PATH=../klee/lib KTEST_FILE=klee-last/test000001.ktest ./noninterf.replay
```
