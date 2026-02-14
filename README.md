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

