# How to build Bugdom for PS Vita
These instructions apply to Linux. I haven't tried other operating systems.

1. Install the Vita toolchain
2. Clone the repo **recursively**:
    ```
    git clone --recurse-submodules https://github.com/ywnico/bugdom-vita
    ```
3. Rebuild the vitaGL package in VitaSDK with `NO_DEBUG=1 CIRCULAR_VERTEX_POOL=2`
4. Build Bugdom:
    ```
    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DVITA=ON
    cmake --build .
    ```

## Notes

- This repository depends on ywnico's fork of Pomme (https://github.com/ywnico/Pomme-vita).
- The Pomme fork includes:
    - Updated data directory for Vita.
