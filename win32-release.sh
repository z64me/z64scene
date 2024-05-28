
mkdir -p bin

# new method
mkdir -p bin/o/win32
i686-w64-mingw32.static-g++ -DNDEBUG -c src/gui.cpp -Iimgui -Iimgui/backends
i686-w64-mingw32.static-gcc -DNDEBUG src/*.c z64viewer/src/*.c -c -I z64viewer/include -I z64viewer/src -Wall -Wno-unused-function -Wno-scalar-storage-order
mv *.o bin/o/win32

# and link
i686-w64-mingw32.static-g++ -o bin/z64scene.exe -Os -s -flto bin/o/win32/*.o bin/o/win32/imgui/*.o -I z64viewer/include `i686-w64-mingw32.static-pkg-config --libs glfw3` -Wall -Wno-unused-function -Wno-scalar-storage-order

