# Build

1. build boost, rose
2. checkout repo, cd
3. `cmake -DROSEHOME=<ROSEHOME> -DBOOSTHOME=<BOOSTHOME> -H. -Bbuild`
4. `cmake --build build -- -j10`

# Doc

* CHiLL.pdf
* `cmake --build build -- doc`

