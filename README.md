c4 - C in four functions
========================

An exercise in minimalism.

Try the following:

    mkdir -p build
    gcc -o build/c4 c4.c
    ./build/c4 hello.c
    ./build/c4 -s hello.c
    
    ./build/c4 c4.c hello.c
    ./build/c4 c4.c c4.c hello.c

Command Line Options
--------------------

    -s      dump source and assembly
    -d      dump debug execution trace
    --      end of options (pass remaining arguments to script)

Run tests:

    ./build/c4 test/test.c

