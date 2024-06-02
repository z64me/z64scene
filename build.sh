
mkdir -p bin

# old method
#gcc src/*.c z64viewer/src/*.c -o bin/z64scene -I z64viewer/include -lm -lglfw -ldl -Wall -Wno-unused-function -Wno-scalar-storage-order

# new method
mkdir -p bin/o
g++ -c src/gui.cpp -Iimgui -Iimgui/backends
gcc src/*.c z64viewer/src/*.c -c -DNOC_FILE_DIALOG_GTK `pkg-config --cflags gtk+-3.0` -I z64viewer/include -I z64viewer/src -lm -lglfw -ldl -Wall -Wno-unused-function -Wno-scalar-storage-order
mv *.o bin/o

# use `pkg-config --cflags gtk+-2.0` for gtk2

# and link
#g++ -o bin/z64scene bin/o/*.o -I z64viewer/include -lm -lglfw -ldl -Wall -Wno-unused-function -Wno-scalar-storage-order
g++ -o bin/z64scene bin/o/*.o bin/o/imgui/*.o -I z64viewer/include `pkg-config --libs gtk+-3.0` -lm -lglfw -ldl -Wall -Wno-unused-function -Wno-scalar-storage-order

# use `pkg-config --libs gtk+-2.0` for gtk2

