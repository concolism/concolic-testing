Experiments in concolic testing with Klee
=========================================

Scripts will assume that `klee` is in the root of this directory
and that `klee`'s include headers are in `../klee/include/`.

    ln -s path/to/klee klee

Otherwise copy scripts before making changes to avoid checking them in git

    cp aeson-cbits/build{-script,}.sh
    cp aeson-cbits/run-klee{-script,}.sh

    # edit aeson-cbits/build.sh and aeson-cbits/run-klee.sh to customize paths
