
mkdir -p bin

# new method
mkdir -p bin/o/win32
i686-w64-mingw32.static-g++ -c src/gui.cpp -Iimgui -Iimgui/backends -Ijson/include
i686-w64-mingw32.static-gcc -DNOC_FILE_DIALOG_WIN32 src/*.c z64viewer/src/*.c -c -I z64viewer/include -I z64viewer/src -Wall -Wno-unused-function -Wno-scalar-storage-order
mv *.o bin/o/win32

# and link
i686-w64-mingw32.static-g++ -o bin/z64scene.exe bin/o/win32/*.o bin/o/win32/imgui/*.o -I z64viewer/include `i686-w64-mingw32.static-pkg-config --libs glfw3` -mconsole -mwindows -Wall -Wno-unused-function -Wno-scalar-storage-order

