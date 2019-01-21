#!/bin/bash

BUILD_COMMAND=$1
PRODUCT_OUT=$2
PRODUCT_NAME=$3


if  [ "${SEC_BUILD_CONF_USE_DMVERITY}" == "true" ] ; then
	DMVERITY_DEFCONFIG=dmverity_defconfig
fi

if  [ "${SEC_BUILD_CONF_KERNEL_KASLR}" == "true" ] ; then
	KASLR_DEFCONFIG=kaslr_defconfig
fi

MODEL=${BUILD_COMMAND%%_*}

        
echo PRODUCT_NAME=$PRODUCT_NAME
echo MODEL=$MODEL
echo DMVERITY_DEFCONFIG=$DMVERITY_DEFCONFIG
echo KASLR_DEFCONFIG=$KASLR_DEFCONFIG

BUILD_WHERE=$(pwd)
BUILD_KERNEL_DIR=$BUILD_WHERE
BUILD_ROOT_DIR=$BUILD_KERNEL_DIR/../..
BUILD_KERNEL_OUT_DIR=$PRODUCT_OUT/obj/KERNEL_OBJ

SECURE_SCRIPT=$BUILD_ROOT_DIR/buildscript/tools/signclient.jar
BUILD_CROSS_COMPILE=$BUILD_ROOT_DIR/android/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
BUILD_JOB_NUMBER=`grep processor /proc/cpuinfo|wc -l`

KERNEL_DEFCONFIG=sdm450_sec_defconfig
DEBUG_DEFCONFIG=sdm450_sec_eng_defconfig
SELINUX_DEFCONFIG=selinux_defconfig
SELINUX_LOG_DEFCONFIG=selinux_log_defconfig

MODEL=${BUILD_COMMAND%%_*}
TEMP=${BUILD_COMMAND#*_}
REGION=${TEMP%%_*}
CARRIER=${TEMP##*_}

VARIANT=${CARRIER}
PROJECT_NAME=${VARIANT}
if [ "${REGION}" == "${CARRIER}" ]; then
	VARIANT_DEFCONFIG=sdm450_sec_${MODEL}_${CARRIER}_defconfig
else
	VARIANT_DEFCONFIG=sdm450_sec_${MODEL}_${REGION}_${CARRIER}_defconfig
fi

BUILD_CONF_PATH=$BUILD_ROOT_DIR/buildscript/build_conf/${MODEL}
BUILD_CONF=${BUILD_CONF_PATH}/common_build_conf.${MODEL}

BOARD_KERNEL_PAGESIZE=4096
mkdir -p $BUILD_KERNEL_OUT_DIR

KERNEL_ZIMG=$BUILD_KERNEL_OUT_DIR/arch/arm64/boot/Image.gz-dtb

FUNC_BUILD_KERNEL()
{
	echo ""
	echo "=============================================="
	echo "START : FUNC_BUILD_KERNEL"
	echo "=============================================="
	echo ""
	echo "build project="$PROJECT_NAME""
	echo "build common config="$KERNEL_DEFCONFIG ""
	echo "build variant config="$VARIANT_DEFCONFIG ""
	echo "build secure option="$SECURE_OPTION ""
	echo "build SEANDROID option="$SEANDROID_OPTION ""
	echo "build selinux defconfig="$SELINUX_DEFCONFIG ""
	echo "build selinux log defconfig="$SELINUX_LOG_DEFCONFIG ""
	echo "build dmverity defconfig="$DMVERITY_DEFCONFIG""
	echo "build kaslr defconfig="$KASLR_DEFCONFIG""
	echo "build PRODUCT_OUT="$PRODUCT_OUT ""

        if [ "$BUILD_COMMAND" == "" ]; then
                SECFUNC_PRINT_HELP;
                exit -1;
        fi


    chmod u+w $PRODUCT_OUT/mkbootimg_ver_args.txt     
    echo '--cmdline "console=ttyHSL0,115200,n8 androidboot.console=ttyHSL0 androidboot.hardware=qcom msm_rtb.filter=0x237 ehci-hcd.park=3 lpm_levels.sleep_disabled=1 androidboot.bootdevice=7824900.sdhci earlycon=msm_hsl_uart,0x78af000" --base 0x00000000 --pagesize 4096 --os_version 8.1.0 --os_patch_level 2018-04-01 --ramdisk_offset 0x02000000 --tags_offset 0x01E00000 '   >  $PRODUCT_OUT/mkbootimg_ver_args.txt   
    
    echo ----------------------------------------------
    echo info $PRODUCT_OUT/mkbootimg_ver_args.txt
    echo ----------------------------------------------
    cat  $PRODUCT_OUT/mkbootimg_ver_args.txt
    echo ----------------------------------------------
    
	if [[ "${SEC_BUILD_OPTION_EXTRA_BUILD_CONFIG}" != *"kasan"* ]] && \
		[[ "${SEC_BUILD_OPTION_EXTRA_BUILD_CONFIG}" != *"ubsan"* ]];
	then
		KCFLAGS=-mno-android
	fi

	make -C $BUILD_KERNEL_DIR O=$BUILD_KERNEL_OUT_DIR -j$BUILD_JOB_NUMBER ARCH=arm64 \
			CROSS_COMPILE=$BUILD_CROSS_COMPILE KCFLAGS=$KCFLAGS \
			VARIANT_DEFCONFIG=$VARIANT_DEFCONFIG \
			DEBUG_DEFCONFIG=$DEBUG_DEFCONFIG $KERNEL_DEFCONFIG \
			SELINUX_DEFCONFIG=$SELINUX_DEFCONFIG \
			SELINUX_LOG_DEFCONFIG=$SELINUX_LOG_DEFCONFIG \
			DMVERITY_DEFCONFIG=$DMVERITY_DEFCONFIG \
			KASLR_DEFCONFIG=$KASLR_DEFCONFIG || exit -1

	make -C $BUILD_KERNEL_DIR O=$BUILD_KERNEL_OUT_DIR -j$BUILD_JOB_NUMBER ARCH=arm64 \
			CROSS_COMPILE=$BUILD_CROSS_COMPILE KCFLAGS=$KCFLAGS || exit -1

	make -C $BUILD_KERNEL_DIR O=$BUILD_KERNEL_OUT_DIR -j$BUILD_JOB_NUMBER ARCH=arm64 \
			CROSS_COMPILE=$BUILD_CROSS_COMPILE KCFLAGS=$KCFLAGS dtbs || exit -1            

    rsync -cv $KERNEL_ZIMG $PRODUCT_OUT/kernel
    
    ls -al $PRODUCT_OUT/kernel
    
	echo ""
	echo "================================="
	echo "END   : FUNC_BUILD_KERNEL"
	echo "================================="
	echo ""
}

SECFUNC_PRINT_HELP()
{
	echo -e '\E[33m'
	echo "Help"
	echo "$0 \$1"
	echo "  \$1 : "
	for model in `ls -1 ../../model/build_conf | sed -e "s/.\+\///"`; do
		for build_conf in `find ../../model/build_conf/$model/ -name build_conf.${model}_* | sed -e "s/.\+build_conf\.//"`; do
			echo "      $build_conf"
		done
	done
	echo -e '\E[0m'
}

(
	FUNC_BUILD_KERNEL
)
