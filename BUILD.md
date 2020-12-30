# How to build Bugdom

# Quick rundown of the project before we start

The current version of Bugdom is built from the following components.

| Component | What is it? | Where does the build system expect to find it? |
| --------- | ----------- | ------------ |
| [Bugdom](https://github.com/jorio/bugdom) | The game itself | It's this very repository. |
| [Pomme](https://github.com/jorio/pomme) | my "Carbon-lite" compatibility layer for Mac OS 8/9-era code. | Stored as a git submodule in extern/Pomme. |
| [Quesa](https://github.com/jorio/quesa) | an independent implementation of QuickDraw 3D. For now, Bugdom will only work with my own fork of Quesa, as the "official" version lacks extensions required by the game. | Stored as a git submodule in extern/Quesa. |
| [SDL](https://libsdl.org) | Cross-platform input/windowing/sound library. | Fetched separately, stored in extern |

This guide will walk you through setting up these components and building the game.

# Prep

Prep your dev tools:

| Mac | Win | Linux |
|-----|-----|-------|
| Xcode 10+ | MS Visual Studio 2019 | Any C++20 compiler |
| CMake 3.17+ ([`brew install cmake`](https://formulae.brew.sh/formula/cmake)) | CMake 3.17+ ([.msi/.zip](https://github.com/Kitware/CMake/releases)) | CMake 3.17+ |

Now, clone the game repo **recursively**. It's important because its dependencies ([Pomme](https://github.com/jorio/pomme), [Quesa](https://github.com/jorio/quesa)) are stored as git submodules.

```git clone --recurse-submodules https://github.com/jorio/Bugdom```

# Instructions for automated builds

To fully (re)build the game, go into the Bugdom directory then run this:

```python3 setup_project.py```

This script will fetch the right version of SDL2 for you† and it will invoke CMake to create one of these projects depending on your platform:

| Mac        | Win           | Linux† |
| ------------- | ------------- | ----- |
| Xcode project: | Visual Studio solution: | Standard makefiles:  |
| build-xcode/Bugdom.xcodeproj | build-msvc/Bugdom.sln |  build-release/, build-debug/ |

When that's done, the script will ask you if you want to build the game straight away. Hit Y or N. Done!

You can also open the project in your IDE to modify the game.

**!!!WARNING!!! You must think of the xcodeproj/sln files as transient. Running setup_project.py will nuke the project files and regenerate them every time. Any permanent change to the build process should be committed to CMakeLists.txt rather than the xcodeproj/sln files.**

> † On Linux, the script won't fetch SDL2. It relies on it being installed on your system already. The package may be called "libsdl2-dev" (Debian, Ubuntu) or simply "sdl2" (Arch).

# Instructions for manual builds

If you don't want to run setup_project.py, here's what you need to do:

## On Mac:
- Get [SDL2-2.0.14.dmg](http://libsdl.org/release/SDL2-2.0.14.dmg), open it, and copy "SDL2.framework" to the "extern" folder
- Prep Xcode project: `cmake -G Xcode -S . -B build-xcode`
- Now you can open "build-xcode/Bugdom.xcodeproj" in Xcode,
- or you can just go ahead and build the game:
`cmake --build build-xcode --config Release`
    - the game gets built as "build-xcode/Release/Bugdom.app"

## On Windows:
- Get [SDL2-devel-2.0.14-VC.zip](http://libsdl.org/release/SDL2-devel-2.0.12-VC.zip) and extract its contents into the "extern" folder
- Prep Visual Studio solution: `cmake -G "Visual Studio 16 2019" -A x64 -S . -B build-msvc`
- Now you can open "build-msvc/Bugdom.sln" in Visual Studio,
- or you can just go ahead and build the game:
``cmake --build build-msvc --config Release``
    - the game gets built as "build-msvc/Release/Bugdom.exe"

## On Linux:
- `cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release`
- `cmake --build build-release`
    - the game gets built as "build/Release/Bugdom"

# Instructions for CLion

If you have JetBrains CLion, you can point it at the repo's root directory and start working on the game straight away. CLion doesn't need pre-generated project files like Xcode or Visual Studio do.
