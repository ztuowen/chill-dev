# CHiLL

***The Composable High Level Loop Source-to-Source Translator***

## How to use

### Build

1. checkout repo, cd
2. `cmake -DROSEHOME=<ROSEHOME> -DBOOSTHOME=<BOOSTHOME> -H. -Bbuild`
3. `cmake --build build -- -j10`
4. If no errors during build, executables are in `build`

### Install

1. Optionally set `CMAKE_INSTALL_PREFIX`
2. Optionally `cmake --build build -- install`

## Additional Doc

* doc/CHiLL.pdf
* Doxygen: `cmake --build build -- doc`

## Folder structure

* `src` - CHiLL's "core" source files
* `include` - CHiLL's include files
* `lib` - Included modules, each as its own `src` and `include`
    * `omega` - omega, LICENSE.omega
    * `codegen` - codegen+, without rose dependence, LICENSE.omega
    * `rosecg` - codegen with rose, LICENSE.omega
    * `parserel` - parse Relation vectors, for adding knowns
* `example` - CHiLL example scripts
* `doc` - manual & doxygen

## Debug

* pass `-DDEBUGCHILL` will enable debug output
