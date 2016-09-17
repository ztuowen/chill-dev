# Build

1. build boost, rose
2. checkout repo, cd
3. `cmake -DROSEHOME=<ROSEHOME> -DBOOSTHOME=<BOOSTHOME> -H. -Bbuild`
4. `cmake --build build -- -j10`
5. If no errors during build, executables are in `build`

# Install

1. Optionally set `CMAKE_INSTALL_PREFIX`
2. Optionally `cmake --build build -- install`

# Doc

* CHiLL.pdf
* `cmake --build build -- doc`

