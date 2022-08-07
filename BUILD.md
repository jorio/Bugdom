# How to build Bugdom for PS Vita
These instructions apply to Linux. I haven't tried other operating systems. In the future, the vitaGL and vpk steps should be added to cmake to simplify the process.

1. Install the Vita toolchain
1. Clone the repo **recursively**:
    ```
    git clone --recurse-submodules https://github.com/ywnico/bugdom-vita
    ```
1. Build vitaGL:
    ```
    cd extern/vitaGL
    make
    cd ../..
    ```
1. Compile Bugdom (eboot.bin):
    ```
    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DVITA=ON
    cmake --build .
    cd ..
    ```
1. Package into vpk
    ```
    ./make_vpk.sh .
    ```

## Notes

- This repository depends on ywnico's forks of Pomme (https://github.com/ywnico/Pomme-vita) and VitaGL (the `ywnico_bugdom` branch of https://github.com/ywnico/vitaGL).
- The Pomme fork includes:
    - Updated data directory for Vita.
- The vitaGL fork includes:
    - An additional function, `glTexSubImage2DUnpackRow`, to simulate the OpenGL calls `glPixelStorei(GL_UNPACK_ROW_LENGTH, some_row_length); glTexSubImage2D(...);` for for unpacking subtextures.
    - Implementation of `GL_UNSIGNED_INT_8_8_8_8`, `GL_UNSIGNED_INT_8_8_8_8_REV`, and `GL_UNSIGNED_INT_1_5_5_5_REV`.
