cd build
cd src
./gsm "$(cat ../../input.txt)" > gsm.ll
llc --filetype=obj -o=gsm.o gsm.ll
clang -o gsmbin gsm.o ../../rtGSM.c
./gsmbin
