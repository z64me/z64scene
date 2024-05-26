mkdir -p bin
gcc src/*.c z64viewer/src/*.c -o bin/z64scene -I z64viewer/include -lm -lglfw -ldl -Wall -Wno-unused-function -Wno-scalar-storage-order

