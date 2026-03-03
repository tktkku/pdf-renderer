## AFL++
```bash
export LLVM_CONFIG="llvm-config-11"
export CC=$HOME/AFLplusplus/afl-gcc-fast 
export CXX=$HOME/AFLplusplus/afl-g++-fast
cmake -S . -B build1 -D CMAKE_BUILD_TYPE=Debug
afl-fuzz -i ./input -o ./out -s 123 -t 30000 -- ./build1/test @@
#If you receive a message like "Hmm, your system is configured to send core dump notifications to an external utility...", just do:
sudo su
echo core >/proc/sys/kernel/core_pattern
exit
gdb --args ./build/test ./output/default/crashes/<your_filename>
```
## fix git error
```bash
git fsck --full
rm .git/object/xxxxx
git fsck --full
tail -n 2 .git/logs/refs/heads/${BRANCH_NAME}
git show ${hash}
git update-ref HEAD ${hash}

```
## delete submodule
```
rm -rf mod
vim .gitmodules
vim .git/config
rm -rf .git/module/mode
```
## Valgrind
valgrind --leak-check=full --track-origins=yes --log-file="valgrind.log" ./build/test ./test.pdf
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --log-file="valgrind.log" --vgdb=yes --vgdb-error=0 -s ./build/test test.pdf
valgrind --tool=callgrind ./build/test test.pdf
callgrind_annotate callgrind.out.<PID> --inclusive=yes
kcachegrind callgrind.out.<PID>
gprof2dot -f callgrind callgrind.out.<PID> | dot -Tpng -o profile.png
