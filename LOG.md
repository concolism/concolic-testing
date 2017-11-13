2017-11-13
==========

aeson
-----

After extending the length of the input, we finally get 100% coverage!
Klee also no longer terminates within 5 minutes, suggesting lots more
different behaviors are possible with two or more hex-encoded characters.

Actually, if we open the state machine a bit, some transitions/states seem to
be missing.  In particular, transitions 4 and 6 in the first half are missing.
Transitions 1 and 3 are reached but somehow inputs reaching those are not
considered as "reaching new states" so they are discarded (unless we ask klee
to output all states). States 60 and 84 are also missing.

2017-11-06
==========

Added gcov integration. Coverage is 93.68% (of 95 Executable LOC) for
aeson and 92.31% (of 130 ELOC) for noninterf.

aeson
-----

Klee terminates, but misses two branches: the true branch of `if (surrogate)`
and the block following the `surrogate2` label.

Next:

- Confirm that inputs to reach those branches exist.

noninterf
---------

Here Klee was interrupted after one minute, so some invalid tests were written
by Klee. 6 tests ended successfully (indistinguishable initial states
taken to indist. final states).

Some lines are reported as not covered, but that is most likely because
stuck states lead to klee_silent_exit which forgets these test cases.

2017-10-26
==========

aeson
-----

No bugs were found, but coverage seems low.
It is uncertain whether there are any bugs in this function.

The executable wrapper to get Klee running is very simple.
To evaluate the efficiency of testing, we deliberately introduce bugs
("manual mutation testing") to see whether Klee catches them.

A simple bug in the testing code is to make the input buffer too short, thus
causing a buffer overflow if the input is sufficiently well-structured.
An initial attempt with Klee+Z3 was unsuccessful, taking over 10min to
overflow a buffer of two codepoints. Klee+STP finds the bug with a buffer of
six codepoints in a minute or so.

Interestingly, with the unbugged version, Klee+STP terminates.
But the coverage of 37% reported by `klee-stats` seems suspiciously low.
Klee+Z3 struggles at around 25%.

```
------------------------------------------------------------------------
|  Path   |  Instrs|  Time(s)|  ICov(%)|  BCov(%)|  ICount|  TSolver(%)|
------------------------------------------------------------------------
|klee-last|  316462|    93.93|    37.25|    31.87|    1173|       94.63|
------------------------------------------------------------------------
```

We are also wondering whether Klee is disturbed by the state machine at the
beginning of the main loop.

Next:

- Take out the state machine from the loop. Would that help?
- Create more bugs at various places.

Noninterference
---------------

This program is written from scratch, based on the paper.
It is still in development, which is deliberately sloppy with the expectation
that Klee will catch the bugs.
Li-yao is already familiar with both the paper and the original Haskell code.

We start with a simple step function for the abstract machine.
One bug that was caught during development is the integer overflow for
an ADD instruction. The code assumes for simplicity that all numbers are
non-negative, whereas integer overflow (which is technically undefined
behavior) causes the apparition of a negative number, which results in an
unchecked out-of-bounds access to memory. The fix was to handle integer
overflow as an error.

We note that the error was only detected in a subsequent
memory operation: it was not found when testing a single step.
One possible though minor improvement may be for Klee to catch this
undefined behavior as soon as it happens. (Does LLVM, the level at which
Klee operates, preserve enough of these (non-)semantics for that to be
possible?)

Coverage stagnates at 56%, which sounds low, though more investigation is
necessary to interpret this number accurately.

```
-------------------------------------------------------------------------
|   Path   |  Instrs|  Time(s)|  ICov(%)|  BCov(%)|  ICount|  TSolver(%)|
-------------------------------------------------------------------------
|klee-out-4|  379112|   159.57|    56.37|    36.96|    1492|       90.84|
-------------------------------------------------------------------------
```

Based on an informal conception of concolic testing, the program is
set up in this order with the hope to improve performance:

- first run `machine1` to completion (without errors);
- assume indistinguishability between the initial states of `machine1` and `machine2`;
- run `machine2` to completion;
- check indistinguishability between their final states.

Next:

- Does this order matter to Klee? Try other orders.
- There are still bugs left in the programs. Can Klee find them?
- We are currently testing the property of "end-to-end noninterference",
  which is informally the most natural, "highest level" noninterference (NI)
  property given by the paper. Two stronger variants to try, that should be
  more convenient both to test and verify,
  are "low lock-step NI" and "single-step NI".
- What interesting criteria are there to compare this approach, concolic
  testing, with random testing, which was the subject of the paper?
  + Time to find bugs
  + Speed of test generation
  + Number of test cases to find bugs
  + Complexity of the setup and tooling
  + ???
