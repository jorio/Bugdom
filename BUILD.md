# How to build Bugdom

## The easy way: build.py (automated build script)

`build.py` can produce a game executable from a fresh clone of the repo in a single command. It will work on macOS, Windows and Linux, provided that your system has Python 3, CMake, and an adequate C++ compiler.

```
git clone --recurse-submodules https://github.com/jorio/Bugdom
cd Bugdom
python3 build.py
```

If you want to build the game **manually** instead, the rest of this document describes how to do just that on each of the big 3 desktop OSes.

## How to build the game manually on macOS

1. Install the prerequisites:
    - Xcode 12+
    - [CMake](https://formulae.brew.sh/formula/cmake) 3.16+ (installing via Homebrew is recommended)
1. Clone the repo **recursively**:
    ```
    git clone --recurse-submodules https://github.com/jorio/Bugdom
    cd Bugdom
    ```
1. Download [SDL2-2.26.2.dmg](https://libsdl.org/release/SDL2-2.26.2.dmg), open it, and copy **SDL2.framework** to the **extern** folder
1. Prep the Xcode project:
    ```
    cmake -G Xcode -S . -B build
    ```
1. Now you can open `build/Bugdom.xcodeproj` in Xcode, or you can just go ahead and build the game:
    ```
    cmake --build build --config RelWithDebInfo
    ```
1. The game gets built in `build/RelWithDebInfo/Bugdom.app`. Enjoy!

## How to build the game manually on Windows

1. Install the prerequisites:
    - Visual Studio 2022 with the C++ toolchain
    - [CMake](https://cmake.org/download/) 3.16+
1. Clone the repo **recursively**:
    ```
    git clone --recurse-submodules https://github.com/jorio/Bugdom
    cd Bugdom
    ```
1. Download [SDL2-devel-2.26.2-VC.zip](https://libsdl.org/release/SDL2-devel-2.26.2-VC.zip), extract it, and copy **SDL2-2.26.2** to the **extern** folder
1. Prep the Visual Studio solution:
    ```
    cmake -G "Visual Studio 17 2022" -A x64 -S . -B build
    ```
1. Now you can open `build/Bugdom.sln` in Visual Studio, or you can just go ahead and build the game:
    ```
    cmake --build build --config Release
    ```
1. The game gets built in `build/Release/Bugdom.exe`. Enjoy!

## How to build the game manually on Linux et al.

1. Install the prerequisites from your package manager:
    - Any C++20 compiler
    - CMake 3.16+
    - SDL2 development library (e.g. "libsdl2-dev" on Ubuntu, "sdl2" on Arch, "SDL-devel" on Fedora)
    - OpenGL development libraries (e.g. "libgl1-mesa-dev" on Ubuntu)
1. Clone the repo **recursively**:
    ```
    git clone --recurse-submodules https://github.com/jorio/Bugdom
    cd Bugdom
    ```
1. Build the game:
    ```
    cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
    cmake --build build
    ```
    If you'd like to enable runtime sanitizers, append `-DSANITIZE=1` to the **first** `cmake` call above.
1. The game gets built in `build/Bugdom`. Enjoy!

