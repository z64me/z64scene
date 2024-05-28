
mkdir -p bin/o/win32/imgui

# build the objects
(cd imgui/examples/example_glfw_opengl3 && i686-w64-mingw32.static-g++ -c -DNDEBUG -Os -I../.. -I../../backends -I../libs/glfw/include main.cpp ../../backends/imgui_impl_glfw.cpp ../../backends/imgui_impl_opengl3.cpp ../../imgui*.cpp && rm main.o)

# move them to where the build script expects them to be
cp imgui/examples/example_glfw_opengl3/*.o bin/o/win32/imgui

