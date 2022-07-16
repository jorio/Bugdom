# How to build Bugdom for PS Vita
These instructions apply to Linux. I haven't tried other operating systems.

1. Install the Vita toolchain
1. Clone the repo **recursively**:
    ```
    git clone --recurse-submodules https://github.com/ywnico/bugdom-vita
    ```
1. Build the game:
    ```
    # Compile eboot.bin
    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DVITA=ON
    cmake --build .

    # Package into vpk
    cd ..
    ./make_vpk.sh .
    ```

## Notes

- This repository depends on ywnico's forks of Pomme (https://github.com/ywnico/Pomme-vita) and VitaGL (the `unpack_row_length` branch of https://github.com/ywnico/vitaGL).
- The Pomme fork includes file access and ARGB -> RGBA pixel conversion routines needed for the Vita.
- The vitaGL fork includes an additional function, `glTexSubImage2DUnpackRow`, to simulate the OpenGL calls `glPixelStorei(GL_UNPACK_ROW_LENGTH, some_row_length); glTexSubImage2D(...);` for for unpacking subtextures.
