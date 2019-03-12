################################################################################
1. How to Build
        - get Toolchain
                From android git server, codesourcery and etc ..
                - gcc-cfp/gcc-cfp-jopp-only/aarch64-linux-android-4.9/bin/aarch64-linux-android-
        - edit Makefile
                edit "CROSS_COMPILE" to right toolchain path(You downloaded).
                        EX)  CROSS_COMPILE=<android platform directory you download>/android/prebuilts/gcc-cfp/gcc-cfp-jopp-only/aarch64-linux-android-4.9/bin/aarch64-linux-android-
                        EX)  CROSS_COMPILE=/usr/local/toolchain/gcc-cfp/gcc-cfp-jopp-only/aarch64-linux-android-4.9/bin/aarch64-linux-android- // check the location of toolchain
                edit "CLANG" to right path(You downloaded).
                        EX)  CC=<android platform directory you download>/android/prebuilts/clang/host/linux-x86/clang-4639204/bin/clang
                        EX)  CC=/usr/local/toolchain/clang/host/linux-x86/clang-4639204/bin/clang // check the location of toolchain
                edit "CLANG_TRIPLE" to right path(You downloaded).
                        EX)  CLANG_TRIPLE=<android platform directory you download>/android/prebuilts/clang/host/linux-x86/clang-4639204/bin/aarch64-linux-gnu-
                        EX)  CLANG_TRIPLE=/usr/local/toolchain/clang/host/linux-x86/clang-4639204/bin/aarch64-linux-gnu- // check the location of toolchain     
        - to Build
                $ export ANDROID_MAJOR_VERSION=p
                $ make ARCH=arm64 exynos9820-beyond2lte_defconfig
                $ make ARCH=arm64

2. Output files
        - Kernel : arch/arm64/boot/Image
        - module : drivers/*/*.ko

3. How to Clean
        $ make clean
################################################################################
