
mkdir -p bin/o/imgui

# build the objects
(cd imgui/examples/example_glfw_opengl3 && make && rm main.o)

# move them to where the build script expects them to be
cp imgui/examples/example_glfw_opengl3/*.o bin/o/imgui

