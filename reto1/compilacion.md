gcc -O2 needles.c -o needles -lm
gcc -O2 dartboard.c -o dartboard -lm

./needles 1000000
./dartboard 1000000