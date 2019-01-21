################################################################################

1. How to Build
    - get Toolchain
        From android git server, codesourcery and etc ..
         - arm-eabi-4.8

    - make output folder
        EX)OUTPUT_DIR=out
        $ mkdir out

    - edit Makefile
        edit "CROSS_COMPILE" to right toolchain path(You downloaded).
          EX)  CROSS_COMPILE= $(android platform directory you download)/android/prebuilt/linux-x86/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi-
          Ex)  CROSS_COMPILE=/usr/local/toolchain/arm-eabi-4.8/bin/arm-eabi-          // check the location of toolchain

2.Build command
        ./build_kernel.sh

3. Output files
    - Kernel : arch/arm/boot/Image
	- module : drivers/*/*.lo

4. How to Clean    
        $ make clean

################################################################################
