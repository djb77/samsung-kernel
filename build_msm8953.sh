#!/bin/bash
# MSM8953 kernel build script v0.5

BUILD_COMMAND=$1
if [ "$BUILD_COMMAND" == "msm8953lte_chn" ]; then
	BUILD_PRODUCT=msm8953lte_chn
	PRODUCT_NAME=msm8953ltechn
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
elif [ "$BUILD_COMMAND" == "c7lte_chn" ]; then
	BUILD_PRODUCT=c7lte_chn
	PRODUCT_NAME=c7ltechn
	SIGN_MODEL=SM-C7000_CHN_CHC_ROOT0
	USE_FINGERPRINT_TZ="true"
	KASLR_DEFCONFIG=kaslr_defconfig
elif [ "$BUILD_COMMAND" == "gtaxlqlte_spr" ]; then
	BUILD_PRODUCT=gtaxlqlte_usa_spr
	PRODUCT_NAME=gtaxlqlte
	SIGN_MODEL=SM-T587P_NA_SPR_ROOT0
	USE_FINGERPRINT_TZ="false"
elif [ "$BUILD_COMMAND" == "on7xlte_chn" ]; then
	BUILD_PRODUCT=on7xlte_chn
	PRODUCT_NAME=on7xltechn
	SIGN_MODEL=SM-G6100_CHN_CHC_ROOT0
	USE_FINGERPRINT_TZ="true"
	KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "j7poplte_vzw" ]; then
	BUILD_PRODUCT=j7poplte_usa_vzw
	PRODUCT_NAME=j7poplte
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
elif [ "$BUILD_COMMAND" == "j7poplte_spr" ]; then
	BUILD_PRODUCT=j7poplte_usa_spr
	PRODUCT_NAME=j7poplte
	SIGN_MODEL=#SM-J727P_NA_SPR_ROOT0
	USE_FINGERPRINT_TZ="false"

fi

BUILD_WHERE=$(pwd)
BUILD_KERNEL_DIR=$BUILD_WHERE
BUILD_ROOT_DIR=$BUILD_KERNEL_DIR/../..
BUILD_KERNEL_OUT_DIR=$BUILD_ROOT_DIR/android/out/target/product/$PRODUCT_NAME/obj/KERNEL_OBJ
PRODUCT_OUT=$BUILD_ROOT_DIR/android/out/target/product/$PRODUCT_NAME

SECURE_SCRIPT=$BUILD_ROOT_DIR/buildscript/tools/signclient.jar
BUILD_CROSS_COMPILE=$BUILD_ROOT_DIR/android/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
BUILD_JOB_NUMBER=`grep processor /proc/cpuinfo|wc -l`

KERNEL_DEFCONFIG=msm8953_sec_defconfig
DEBUG_DEFCONFIG=msm8953_sec_eng_defconfig
SELINUX_DEFCONFIG=selinux_defconfig
SELINUX_LOG_DEFCONFIG=selinux_log_defconfig
DMVERITY_DEFCONFIG=dmverity_defconfig

while getopts "w:t:" flag; do
	case $flag in
		w)
			BUILD_OPTION_HW_REVISION=$OPTARG
			echo "-w : "$BUILD_OPTION_HW_REVISION""
			;;
		t)
			TARGET_BUILD_VARIANT=$OPTARG
			echo "-t : "$TARGET_BUILD_VARIANT""
			;;
		*)
			echo "wrong 2nd param : "$OPTARG""
			exit -1
			;;
	esac
done

shift $((OPTIND-1))

SECURE_OPTION=
SEANDROID_OPTION=
if [ "$2" == "-B" ]; then
	SECURE_OPTION=$2
elif [ "$2" == "-E" ]; then
	SEANDROID_OPTION=$2
else
	NO_JOB=
fi

if [ "$3" == "-B" ]; then
	SECURE_OPTION=$3
elif [ "$3" == "-E" ]; then
	SEANDROID_OPTION=$3
else
	NO_JOB=
fi

MODEL=${BUILD_PRODUCT%%_*}
TEMP=${BUILD_PRODUCT#*_}
REGION=${TEMP%%_*}
CARRIER=${TEMP##*_}

VARIANT=k${CARRIER}
PROJECT_NAME=${VARIANT}
if [ "${REGION}" == "${CARRIER}" ]; then
	VARIANT_DEFCONFIG=msm8953_sec_${MODEL}_${CARRIER}_defconfig
else
	VARIANT_DEFCONFIG=msm8953_sec_${MODEL}_${REGION}_${CARRIER}_defconfig
fi

CERTIFICATION=NONCERT

BUILD_CONF_PATH=$BUILD_ROOT_DIR/buildscript/build_conf/${MODEL}
BUILD_CONF=${BUILD_CONF_PATH}/common_build_conf.${MODEL}

case $1 in
		clean)
		echo "Not support... remove kernel out directory by yourself"
		exit 1
		;;

		*)
		BOARD_KERNEL_BASE=0x80000000
		BOARD_KERNEL_PAGESIZE=2048
		BOARD_KERNEL_TAGS_OFFSET=0x01E00000
		BOARD_RAMDISK_OFFSET=0x02000000
		BOARD_KERNEL_CMDLINE="console=ttyHSL0,115200,n8 androidboot.console=ttyHSL0 androidboot.hardware=qcom msm_rtb.filter=0x237 ehci-hcd.park=3 androidboot.bootdevice=7824900.sdhci lpm_levels.sleep_disabled=1"
#		BOARD_KERNEL_CMDLINE="console=null androidboot.hardware=qcom user_debug=31 msm_rtb.filter=0x37 ehci-hcd.park=3 lpm_levels.sleep_disabled=1 boot_cpus=0-5 androidboot.bootdevice=f9824900.sdhci"
#		BOARD_KERNEL_CMDLINE="console=null androidboot.hardware=qcom msm_rtb.filter=0x237 ehci-hcd.park=3 androidboot.bootdevice=7824900.sdhci lpm_levels.sleep_disabled=1 earlyprintk"
		mkdir -p $BUILD_KERNEL_OUT_DIR
		;;

esac

KERNEL_ZIMG=$BUILD_KERNEL_OUT_DIR/arch/arm64/boot/Image
DTC=$BUILD_KERNEL_OUT_DIR/scripts/dtc/dtc

FUNC_CLEAN_DTB()
{
	if ! [ -d $BUILD_KERNEL_OUT_DIR/arch/arm64/boot/dts ] ; then
		echo "no directory : "$BUILD_KERNEL_OUT_DIR/arch/arm64/boot/dts""
	else
		echo "rm files in : "$BUILD_KERNEL_OUT_DIR/arch/arm64/boot/dts/*.dtb""
		rm $BUILD_KERNEL_OUT_DIR/arch/arm64/boot/dts/*.dtb
	fi
}

INSTALLED_DTIMAGE_TARGET=${PRODUCT_OUT}/dt.img
DTBTOOL=$BUILD_KERNEL_DIR/tools/dtbTool

FUNC_BUILD_DTIMAGE_TARGET()
{
	echo ""
	echo "================================="
	echo "START : FUNC_BUILD_DTIMAGE_TARGET"
	echo "================================="
	echo ""
	echo "DT image target : $INSTALLED_DTIMAGE_TARGET"

	if ! [ -e $DTBTOOL ] ; then
		if ! [ -d $BUILD_ROOT_DIR/android/out/host/linux-x86/bin ] ; then
			mkdir -p $BUILD_ROOT_DIR/android/out/host/linux-x86/bin
		fi
		cp $BUILD_ROOT_DIR/kernel/tools/dtbTool $DTBTOOL
	fi

	cp $BUILD_KERNEL_OUT_DIR/arch/arm64/boot/dts/samsung/msm8953/*.dtb $BUILD_KERNEL_OUT_DIR/arch/arm64/boot/dts/.	

	echo "$DTBTOOL -o $INSTALLED_DTIMAGE_TARGET -s $BOARD_KERNEL_PAGESIZE \
		-p $BUILD_KERNEL_OUT_DIR/scripts/dtc/ $BUILD_KERNEL_OUT_DIR/arch/arm64/boot/dts/"
		$DTBTOOL -o $INSTALLED_DTIMAGE_TARGET -s $BOARD_KERNEL_PAGESIZE \
		-p $BUILD_KERNEL_OUT_DIR/scripts/dtc/ $BUILD_KERNEL_OUT_DIR/arch/arm64/boot/dts/

	chmod a+r $INSTALLED_DTIMAGE_TARGET

	echo ""
	echo "================================="
	echo "END   : FUNC_BUILD_DTIMAGE_TARGET"
	echo "================================="
	echo ""
}

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
	echo "build tima defconfig="$TIMA_DEFCONFIG ""
	echo "build dmverity defconfig="$DMVERITY_DEFCONFIG""
	echo "build kaslr defconfig="$KASLR_DEFCONFIG""

        if [ "$BUILD_PRODUCT" == "" ]; then
                SECFUNC_PRINT_HELP;
                exit -1;
        fi

	FUNC_CLEAN_DTB

	make -C $BUILD_KERNEL_DIR O=$BUILD_KERNEL_OUT_DIR -j$BUILD_JOB_NUMBER ARCH=arm64 \
			CROSS_COMPILE=$BUILD_CROSS_COMPILE \
			VARIANT_DEFCONFIG=$VARIANT_DEFCONFIG \
			DEBUG_DEFCONFIG=$DEBUG_DEFCONFIG $KERNEL_DEFCONFIG \
			SELINUX_DEFCONFIG=$SELINUX_DEFCONFIG \
			SELINUX_LOG_DEFCONFIG=$SELINUX_LOG_DEFCONFIG \
			DMVERITY_DEFCONFIG=$DMVERITY_DEFCONFIG \
			KASLR_DEFCONFIG=$KASLR_DEFCONFIG || exit -1

	make -C $BUILD_KERNEL_DIR O=$BUILD_KERNEL_OUT_DIR -j$BUILD_JOB_NUMBER ARCH=arm64 \
			CROSS_COMPILE=$BUILD_CROSS_COMPILE || exit -1

	FUNC_BUILD_DTIMAGE_TARGET

	echo ""
	echo "================================="
	echo "END   : FUNC_BUILD_KERNEL"
	echo "================================="
	echo ""
}

FUNC_MKBOOTIMG()
{
	echo ""
	echo "==================================="
	echo "START : FUNC_MKBOOTIMG"
	echo "==================================="
	echo ""
	MKBOOTIMGTOOL=$BUILD_ROOT_DIR/android/kernel/tools/mkbootimg

	if ! [ -e $MKBOOTIMGTOOL ] ; then
		if ! [ -d $BUILD_ROOT_DIR/android/out/host/linux-x86/bin ] ; then
			mkdir -p $BUILD_ROOT_DIR/android/out/host/linux-x86/bin
		fi
		cp $BUILD_ROOT_DIR/anroid/kernel/tools/mkbootimg $MKBOOTIMGTOOL
	fi

	echo "Making boot.img ..."
	echo "	$MKBOOTIMGTOOL --kernel $KERNEL_ZIMG \
			--ramdisk $PRODUCT_OUT/ramdisk.img \
			--output $PRODUCT_OUT/boot.img \
			--cmdline "$BOARD_KERNEL_CMDLINE" \
			--base $BOARD_KERNEL_BASE \
			--pagesize $BOARD_KERNEL_PAGESIZE \
			--ramdisk_offset $BOARD_RAMDISK_OFFSET \
			--tags_offset $BOARD_KERNEL_TAGS_OFFSET \
			--dt $INSTALLED_DTIMAGE_TARGET"

	$MKBOOTIMGTOOL --kernel $KERNEL_ZIMG \
			--ramdisk $PRODUCT_OUT/ramdisk.img \
			--output $PRODUCT_OUT/boot.img \
			--cmdline "$BOARD_KERNEL_CMDLINE" \
			--base $BOARD_KERNEL_BASE \
			--pagesize $BOARD_KERNEL_PAGESIZE \
			--ramdisk_offset $BOARD_RAMDISK_OFFSET \
			--tags_offset $BOARD_KERNEL_TAGS_OFFSET \
			--dt $INSTALLED_DTIMAGE_TARGET

	if [ "$SEANDROID_OPTION" == "-E" ] ; then
		FUNC_SEANDROID
	fi

	if [ "$SECURE_OPTION" == "-B" ]; then
		FUNC_SECURE_SIGNING
	fi

	cd $PRODUCT_OUT
	tar cvf boot_${MODEL}_${CARRIER}_${CERTIFICATION}.tar boot.img

	cd $BUILD_ROOT_DIR
	if ! [ -d output ] ; then
		mkdir -p output
	fi

        echo ""
	echo "================================================="
        echo "-->Note, copy to $BUILD_TOP_DIR/../output/ directory"
	echo "================================================="
	cp $PRODUCT_OUT/boot_${MODEL}_${CARRIER}_${CERTIFICATION}.tar $BUILD_ROOT_DIR/output/boot_${MODEL}_${CARRIER}_${CERTIFICATION}.tar || exit -1
        cd ~

	echo ""
	echo "==================================="
	echo "END   : FUNC_MKBOOTIMG"
	echo "==================================="
	echo ""
}

FUNC_SEANDROID()
{
	echo -n "SEANDROIDENFORCE" >> $PRODUCT_OUT/boot.img
}

FUNC_SECURE_SIGNING()
{
	echo "java -jar $SECURE_SCRIPT -model $SIGN_MODEL -runtype ss_openssl_sha -input $PRODUCT_OUT/boot.img -output $PRODUCT_OUT/signed_boot.img"
	openssl dgst -sha256 -binary $PRODUCT_OUT/boot.img > sig_32
	java -jar $SECURE_SCRIPT -runtype ss_openssl_sha -model $SIGN_MODEL -input sig_32 -output sig_256
	cat $PRODUCT_OUT/boot.img sig_256 > $PRODUCT_OUT/signed_boot.img

	mv -f $PRODUCT_OUT/boot.img $PRODUCT_OUT/unsigned_boot.img
	mv -f $PRODUCT_OUT/signed_boot.img $PRODUCT_OUT/boot.img

	CERTIFICATION=CERT
}

SECFUNC_PRINT_HELP()
{
	echo -e '\E[33m'
	echo "Help"
	echo "$0 \$1 \$2 \$3"
	echo "  \$1 : "
	echo "		msm8953lte_chn"
	echo "		c7lte_chn"
	echo "		on7xlte_chn"
	echo "		gtaxlqlte_spr"
	echo "		j7poplte_vzw"
	echo "		j7poplte_spr"
	echo "  \$2 : "
	echo "	-B or Nothing  (-B : Secure Binary)"
	echo "  \$3 : "
	echo "	-E or Nothing  (-E : SEANDROID Binary)"
	echo -e '\E[0m'
}

SET_SEC_VARIABLE()
{
	local __fingerprint_tz_var_name="fingerprint-tz"
	local __use_fingerprint_tz=$USE_FINGERPRINT_TZ

	if [ -f ${BUILD_CONF} ]; then
		local __fingerprint_tz_var=$(grep ${__fingerprint_tz_var_name} ${BUILD_CONF} | tr -d ' ')
		if [[ "${__fingerprint_tz_var}x" != "x" && "${__fingerprint_tz_var:0:1}" != "#" ]]; then
			__use_fingerprint_tz=$(echo "${__fingerprint_tz_var}" | cut -d '#' -f1 | cut -d '=' -f2 | tr -d ' ')
		fi
	fi

	export SEC_BUILD_CONF_USE_FINGERPRINT_TZ=${__use_fingerprint_tz}
}

# MAIN FUNCTION
rm -rf ./build.log
(
	START_TIME=`date +%s`

	SET_SEC_VARIABLE
	FUNC_BUILD_KERNEL
	#FUNC_RAMDISK_EXTRACT_N_COPY
	FUNC_MKBOOTIMG

	END_TIME=`date +%s`

	let "ELAPSED_TIME=$END_TIME-$START_TIME"
	echo "Total compile time is $ELAPSED_TIME seconds"
) 2>&1	 | tee -a ./build.log
