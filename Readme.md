
## Build
```bash
cmake -S . -B build

cmake --build build
```
## Valgrind
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --log-file="valgrind.log" --vgdb=yes --vgdb-error=0 -s ./build/test 44.pdf