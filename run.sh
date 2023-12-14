mkdir build
cd build
cmake ..
make
cd src
# get input from a file named input
./gsm "$(cat ../input.txt)" > gsm.ll
llc --filetype=obj -o=gsm.o gsm.ll
clang -o gsmbin gsm.o ../../rtGSM.c
