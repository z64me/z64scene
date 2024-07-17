# z64scene

## build instructions

### linux dependencies

if you're producing a linux build on a debian machine, try:
```
sudo apt install gcc g++ make libglfw3-dev libgtk-3-dev
(as well as libgtk2.0-dev if you want to build for gtk2)
```

### windows dependencies

z64scene is built for windows using mxe ([https://mxe.cc/](https://mxe.cc/)) running
inside linux (tested on arch, and on debian running in wsl2).

when you go to set up mxe, don't build all the packages; you only need this:
```
make gcc glfw3
```

### and then clone and build z64scene

once you have the dependencies for your system (see above), you should be able to do this:
```sh
git clone --recurse-submodules https://github.com/z64me/z64scene.git
cd z64scene
make linux          # if building for windows, make win32
bin/z64scene        # if running on windows, bin/z64scene.exe
```

