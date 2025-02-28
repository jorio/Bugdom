# Bugdom for PowerPC Mac OS X Tiger

Got a PowerPC Mac? Tired of the degraded experience of running Bugdom under the Classic environment? Here's a version of Bugdom that runs natively in Mac OS X Tiger!

Special thanks to Rudy V. Pancaro for sponsoring this work!

You can download a prebuilt version of the game here:
https://github.com/jorio/Bugdom/releases/tag/1.3.3

Have fun!

## Caveats

Bugdom on OSX/PPC is mostly identical to my port of the game for modern computers, with a few caveats:

- Only keyboard input is supported.

- Seamless terrain texturing isn't supported because the GPUs in PPC Macs typically don't support NPOT textures.

- The game will start in 640x480, but you can change the resolution in the game's built-in Video Settings. Do note that you must restart the game for the resolution change to be effective.

- If you're getting a choppy framerate:
    - quit all background applications before running the game
    - try to use a lower screen resolution in "video settings" (restart the game to apply)
    - try "low detail" mode in "video settings"
    - set "aspect ratio" to "4:3" in "video settings" so the GPU has less geometry to draw

## How to build it from source

1. Get your PowerPC machine with Mac OS X 10.4 ready.

1. Install **Xcode 2.5**. We're not going to use the compilers that ship with Xcode 2.5 — they're too old and they'll fail to build Pomme, the support library used in the modern version of Bugdom. But Xcode does ship essential tools such as `make`, `ld`, the OpenGL development lib, etc.

1. Install a recent enough version of `git` and `python3`. We'll need those to manage the source tree and run the build scripts. I recommend using [Tigerbrew](https://github.com/mistydemeo/tigerbrew) for this. At the time of writing, Tigerbrew offers Git 2.10.2 and Python 3.7.0, which are more than enough for our purposes.

1. Install **GCC 7.3.0** manually:

    - Download Tigerbrew's prebuilt gcc 7.3.0 package: **[gcc-7.3.0.tiger_g3.bottle.tar.gz](https://ia904500.us.archive.org/24/items/tigerbrew/gcc-7.3.0.tiger_g3.bottle.tar.gz)**. This will save you days of building gcc on a clunky old machine. (Note: we're using a G3 build as the "lowest common denominator" to produce an executable that's compatible with the widest range of PowerPC CPUs.)

    - Extract the .tar.gz file, and move the `gcc` directory to the root of your boot drive so that you can execute `/gcc/7.3.0/bin/gcc-7`. The game's build scripts expect to find gcc-7 at this exact location. If you want to install gcc-7 elsewhere, you'll have to edit the scripts.

1. Clone the repositories for the game itself and SDL 2.0.3:

    ```sh
    git clone --recurse-submodules https://github.com/jorio/Bugdom
    git clone --branch release-2.0.3 https://github.com/libsdl-org/SDL Bugdom/osxppc/sdl-2.0.3
    ```

1. `cd` into `Bugdom/osxppc`

1. Build SDL 2.0.3:
    
    ```sh
    ./mksdl.sh
    ```
    
    When it's done, it should have produced a file named `libSDL2.a` in the `osxppc` folder.

    (Under the hood, this script patches SDL 2.0.3 with `sdl-tiger.patch`, then builds SDL as a static library. The patch is a slightly modified version of [this patch by Thomas Bernard](https://gist.github.com/miniupnp/26d6e967570e5729a757).)

1. Generate the makefile for the game:
    
    ```sh
    python3 mkppcmakefile.py
    ```
    
    When it's done, it should have produced a file named `Makefile` in the `osxppc` folder.

1. Build the game:

    ```sh
    make
    ```

    When it's done, it should have produced a file named `Bugdom_G3_release`.

1. Take the game for a test drive:

    ```sh
    ln -s ../Data Data
    ./Bugdom_G3_release
    ```

1. If you want to make an app bundle, run:
    ```bash
    python3 mkppcapp.py --exe Bugdom_G3_release
    ```
    This will produce a double-clickable app.


## Notes on making a 3-architecture “fat” binary that will run on x86_64, arm64, and PPC

- Build the PPC executable (Bugdom_G3_release) per instructions above and move it to your modern Mac.

- Build the full app bundle for x86_64 and arm64 on the modern Mac.

- Set `LSMinimumSystemVersion` to `10.4` in Info.plist so the game will run on Tiger.

- Optional: You can keep a separate OS version floor for the modern archs in Info.plist, e.g.:
    ```xml
    <key>LSMinimumSystemVersionByArchitecture</key>
    <dict>
        <key>x86_64</key>
        <string>10.11</string>
    </dict> 
    ```

- Create the fat binary:
    ```
    lipo -create -o FatBugdom Bugdom.app/Contents/MacOS/Bugdom Bugdom_G3_release
    ```
    You can check that all 3 archs are in the executable: `lipo -info FatBugdom`

- Move FatBugdom to Bugdom.app/Contents/MacOS/Bugdom

- Re-sign the executable, preserving runtime hardening for notarization:
    ```
    codesign --force --sign $APPLE_DEVTEAM --options runtime Bugdom.app
    ```
