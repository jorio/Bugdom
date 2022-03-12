# How to build Bugdom

## TL;DR: Automated build script

Clone the repo **recursively**, then run `python3 build.py` to execute the build steps described in this document and package up the game.

build.py is the script that is used by the CI setup to produce builds. If you want to build the game manually instead, read on.

## How to build the game on macOS

1. Install the prerequisites:
    - Xcode 12+
    - [CMake](https://formulae.brew.sh/formula/cmake) 3.16+
1. Clone the repo **recursively**:
    ```
    git clone --recurse-submodules https://github.com/jorio/Bugdom
    ```
1. Download [SDL2-2.0.20.dmg](https://libsdl.org/release/SDL2-2.0.20.dmg), open it, and copy **SDL2.framework** to the **extern** folder
1. Prep the Xcode project:
    ```
    cmake -G Xcode -S . -B build
    ```
1. Now you can open `build/Bugdom.xcodeproj` in Xcode, or you can just go ahead and build the game:
    ```
    cmake --build build --config Release
    ```
1. The game gets built in `build/Release/Bugdom.app`. Enjoy!

## How to build the game on Windows

1. Install the prerequisites:
    - Visual Studio 2022 with the C++ toolchain
    - [CMake](https://cmake.org/download/) 3.16+
1. Clone the repo **recursively**:
    ```
    git clone --recurse-submodules https://github.com/jorio/Bugdom
    ```
1. Download [SDL2-devel-2.0.20-VC.zip](https://libsdl.org/release/SDL2-devel-2.0.20-VC.zip) and extract the contents into the **extern** folder
1. Prep the Visual Studio solution:
    ```
    cmake -G "Visual Studio 17 2022" -A x64 -S . -B build
    ```
1. Now you can open `build/Bugdom.sln` in Visual Studio, or you can just go ahead and build the game:
    ```
    cmake --build build --config Release
    ```
1. The game gets built in `build/Release/Bugdom.exe`. Enjoy!

## How to build the game on Linux et al.

1. Install the prerequisites from your package manager:
    - Any C++20 compiler
    - CMake 3.16+
    - SDL2 development library (e.g. "libsdl2-dev" on Ubuntu, "sdl2" on Arch, "SDL-devel" on Fedora)
    - OpenGL development libraries (e.g. "libgl1-mesa-dev" on Ubuntu)
1. Clone the repo **recursively**:
    ```
    git clone --recurse-submodules https://github.com/jorio/Bugdom
    ```
1. Build the game:
    ```
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build
    ```
1. The game gets built in `build/Release/Bugdom`. Enjoy!

