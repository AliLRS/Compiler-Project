mkdir build
cd build
cmake ..
make
cd src
./gsm "int a;" > gsm.ll
llc --filetype=obj -o=gsm.o gsm.ll
clang -o gsmbin gsm.o ../../rtGSM.c
