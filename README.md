# ex2_obc_sdr
Software Defined Radio software for Ex-Alta2 on-board computer

## Build and Test on Linux

[Meson](https://mesonbuild.com/index.html) is use to manage build and test functions. The main steps are

```
meson setup build
cd build
ninja
meson test

Likely there will be missing packages, including eigen3, csp, and gtest.

These should be downloaded, built with CMake, and installed. (Usual directory is /usr/local/)

eigen3 has easy to follow instructions

libcsp does not. Download from the original libcsp git, not the ABSat one as this has specific build errors on Linux
Run python3 waf configure --enable-shlib --install-csp --disable-stlib
Run python3 waf build
Run sudo python3 waf install
Finally, copy over the csp folder in include to /usr/local/include/

gtest is similar to eigen3. Clone the repo, build with CMake, do make install.
    
## Build and Test for Hercules
