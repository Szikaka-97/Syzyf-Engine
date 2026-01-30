# Syzyf

(Hopefully) A very simple game engine, coming to you straight from Boat City!

## Prerequisites
The Syzyf project requires the following *stuff* in order to build

* A G++ compatible compiler installed and configured (MSVC doesn't work as of 9ff4904)
* CMAKE installed and working
* Git
* A build tool of your choice (make, ninja, etc)

### Windows

To build the application on Windows, you will need to enable developer mode in your system's settings

> Settings -> Update & Security -> For developers -> Developer Mode ON

## Building

To build a project, you must first generate the required make files

```
cmake -B build
```

If you're developing on Windows, you will need to explicitly specify the generator, otherwise the command will generate an MSVC solution

```
cmake -B build -G MinGW Makefiles
```
or
```
cmake -B build -G Ninja
```

The result should be a `build` directory created in the root directory of the project

After navigating into it, you can run your preferred build tool to compile the project

The actual built application will reside in `build/src` directory, along with the symlink to the development `res` folder, containing all the runtime resources needed to run the application

**NOTE** If you're on Windows and your build fails because of a failure to create a symlink to `res`, refer to `Prerequisites/Windows` for instructions on how to enable developer mode and allow for users to create symbolic links

To run the project, navigate to the `build/src` directory (the app will fail if started from any other) and launch the application

### Windows

One of the dependencies of the assimp library automatically assumes that every compiler running on Windows is MSVC. This will lead to errors when it'll try to use the macros and intrinsic functions usually defined by MSVC. To remedy this problem you will have to navigate to `build/_deps/assimp-src/contrib/poly2tri/poly2tri/common` and change all mentions of `_WIN32` symbol in the `dll_symbols.h` and `shapes.h` to `_MSC_VER`, as well as in `DefaultIOSteram.cpp` in `build/_deps/assimp-src/code/Common`

## Development

CMake will glob all source files from the `src` directory. All files contained there will be compiled

The project has the `src/include` folder configured as a include directory, so all files contained there can be included by using angle brackets

To add new libraries, or change the version of an existing one, edit the CMakeLists.txt file in `thirdparty` folder. <br>
Note that to link any new library into the project, you will need to add an appropriate entry in `src/CMakeLists.txt` as well

The default target uses the -g gcc flag, so it will compile with debug symbols for use in an external debugger like GDB. Unfortunately, even with -O0 the compiler tends to heavily optimize out the local variables, so be wary.

There is currently no option provided for generating .pdb files, so no RenderDoc debugging