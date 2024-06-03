# z64scene

## build instructions

make sure you install the following dependencies:
- gcc
- g++
- make
- glfw3
- gtk3 (only if building for linux)

if you're producing a linux build on a debian machine, try:
```
sudo apt install gcc g++ make libglfw3-dev libgtk-3-dev
(as well as libgtk2.0-dev if you want to build for gtk2)
```

and then you should be able to do this:
```sh
git clone --recurse-submodules https://github.com/z64me/z64scene.git
cd z64scene
make linux          # if building for windows, make win32
bin/z64scene        # if running on windows, bin/z64scene.exe
```

