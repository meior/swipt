# swipt
A network routing algorithm for simultaneous wireless information and power transfer.The goal is to obtain maximum channel transmission capacity.This is a GUI program based on OpenGL.

## Before
I used `glut.h` and third-party dependent library `freeglut.h` in this project.You should install the dependent files which can be found in `bin`, `lib`, `include` folder.

## Compile
I provide a single c++ source file `swipt.cpp` which include all the source code.So that, you can debug and run this project at one-click in Visual Studio Code because i have written all setting into files under `.vscode` folder.But you need `C/C++` and `Code Runner` extensions to achieve that.

You can also compile swipt.cpp to get executable file by command line.Here is an example of gcc 7.1.0 compiler and you need to change the executable file name(swipt.exe) for other platforms.
```bash
$ g++ -c swipt.cpp -o swipt.o -D FREEGLUT_STATIC
$ g++ -o swipt.exe swipt.o -lfreeglut_static -lopengl32 -lglu32 -lwinmm -lgdi32 -static-libgcc -Wall
```

There is another way to get executable file more conveniently.Make is the best way certainly.I use MinGW on Windows and you need to modify `Makefile` for other platforms.
```bash
$ mingw32-make
$ mingw32-make clean    // clean temp files
```

## Usage
Using command line parameters to switch mode between 'INTERFERENCE MODE' and 'NO-INTERFERENCE MODE'.Click on two nodes with the mouse and it will start working.

### Interference mode(default)
```bash
$ swipt.exe 1
```
![swipt_interference](http://7xs1tt.com1.z0.glb.clouddn.com/swipt/interference.jpg)

### No-Interference mode
```bash
$ swipt.exe 0
```
![swipt_no-interference](http://7xs1tt.com1.z0.glb.clouddn.com/swipt/no-interference.jpg)
