#!/bin/bash

# SDM450 kernel build script v0.5
export PLATFORM_VERSION=8

BUILD_COMMAND=$1

if [ "$BUILD_COMMAND" == "a6plte_eur_open" ]; then
	BUILD_PRODUCT=a6plte_eur_open
	PRODUCT_NAME=a6plte_eur
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
	KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "a6plte_sea_open" ]; then
	BUILD_PRODUCT=a6plte_sea_open
	PRODUCT_NAME=a6plte_sea
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
	KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "a6plte_ltn_open" ]; then
	BUILD_PRODUCT=a6plte_ltn_open
	PRODUCT_NAME=a6plte_ltn
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
	KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "a6plte_chn_open" ]; then
	BUILD_PRODUCT=a6plte_chn_open
	PRODUCT_NAME=a6plte_chn
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
	KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "a6plte_kor_ktt" ]; then
	BUILD_PRODUCT=a6plte_kor_ktt
	PRODUCT_NAME=a6plte_kor
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
	KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "j8plte_cis_ser" ]; then
	BUILD_PRODUCT=j8plte_cis_ser
	PRODUCT_NAME=j8plte_cis
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
elif [ "$BUILD_COMMAND" == "a6plte_cis_ser" ]; then
	BUILD_PRODUCT=a6plte_cis_ser
	PRODUCT_NAME=a6plte_cis
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
elif [ "$BUILD_COMMAND" == "j8plte_eur_open" ]; then
	BUILD_PRODUCT=j8plte_eur_open
	PRODUCT_NAME=j8plte_eur
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
	KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "j8plte_sea_open" ]; then
	BUILD_PRODUCT=j8plte_sea_open
	PRODUCT_NAME=j8plte_sea
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
	KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "j8plte_chn_open" ]; then
	BUILD_PRODUCT=j8plte_chn_open
	PRODUCT_NAME=j8plte_chn
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
	KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "j8plte_chn_hk" ]; then
	BUILD_PRODUCT=j8plte_chn_hk
	PRODUCT_NAME=j8plte_chn
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
	KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "j8plte_chn_cmcc" ]; then
	BUILD_PRODUCT=j8plte_chn_cmcc
	PRODUCT_NAME=j8plte_chn
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
	KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "j8plte_kor_ktt" ]; then
	BUILD_PRODUCT=j8plte_kor_ktt
	PRODUCT_NAME=j8plte_kor
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
	KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "gta2xllte_eur_open" ]; then
	BUILD_PRODUCT=gta2xllte_eur_open
	PRODUCT_NAME=gta2xllte_eur
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
	KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "gta2xllte_sea_open" ]; then
	BUILD_PRODUCT=gta2xllte_sea_open
	PRODUCT_NAME=gta2xllte_sea
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
	KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "gta2xllte_usa_spr" ]; then
	BUILD_PRODUCT=gta2xllte_usa_spr
	PRODUCT_NAME=gta2xllte_usa
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
	KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "gta2xllte_usa_vzw" ]; then
	BUILD_PRODUCT=gta2xllte_usa_vzw
	PRODUCT_NAME=gta2xllte_usa
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
	KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "gta2xllte_mea_open" ]; then
	BUILD_PRODUCT=gta2xllte_mea_open
	PRODUCT_NAME=gta2xllte_mea
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
	KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "gta2xllte_mea_tur" ]; then
	BUILD_PRODUCT=gta2xllte_mea_tur
	PRODUCT_NAME=gta2xllte_mea
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
	KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "gta2xllte_chn_open" ]; then
	BUILD_PRODUCT=gta2xllte_chn_open
	PRODUCT_NAME=gta2xllte_chn
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
	KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "gta2xllte_chn_hk" ]; then
	BUILD_PRODUCT=gta2xllte_chn_hk
	PRODUCT_NAME=gta2xllte_chn
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
	KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "gta2xllte_cis_ser" ]; then
	BUILD_PRODUCT=gta2xllte_cis_ser
	PRODUCT_NAME=gta2xllte_cis
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
	KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "gta2xllte_can_single" ]; then
	BUILD_PRODUCT=gta2xllte_can_single
	PRODUCT_NAME=gta2xllte_can
	SIGN_MODEL=
	USE_FINGERPRINT_TZ="false"
	KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "gta2xlwifi_eur_open" ]; then
        BUILD_PRODUCT=gta2xlwifi_eur_open
        PRODUCT_NAME=gta2xlwifi_eur
        SIGN_MODEL=
        USE_FINGERPRINT_TZ="false"
        KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "gta2xlwifi_cis_ser" ]; then
        BUILD_PRODUCT=gta2xlwifi_cis_ser
        PRODUCT_NAME=gta2xlwifi_cis
        SIGN_MODEL=
        USE_FINGERPRINT_TZ="false"
        KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "gta2xlwifi_chn_open" ]; then
        BUILD_PRODUCT=gta2xlwifi_chn_open
        PRODUCT_NAME=gta2xlwifi_chn
        SIGN_MODEL=
        USE_FINGERPRINT_TZ="false"
        KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "j6plte_cis_ser" ]; then
        BUILD_PRODUCT=j6plte_cis_ser
        PRODUCT_NAME=j6plte_cis
        SIGN_MODEL=
        USE_FINGERPRINT_TZ="false"
        KASLR_DEFCONFIG=
elif [ "$BUILD_COMMAND" == "c7lte_chn" ]; then
	BUILD_PRODUCT=c7lte_chn
	PRODUCT_NAME=c7ltechn
	SIGN_MODEL=SM-C7000_CHN_CHC_ROOT0
	USE_FINGERPRINT_TZ="true"
	KASLR_DEFCONFIG=kaslr_defconfig
fi

BUILD_WHERE=$(pwd)
BUILD_KERNEL_DIR=$BUILD_WHERE
BUILD_ROOT_DIR=$BUILD_KERNEL_DIR/../..
BUILD_KERNEL_OUT_DIR=$BUILD_ROOT_DIR/android/out/target/product/$PRODUCT_NAME/obj/KERNEL_OBJ
PRODUCT_OUT=$BUILD_ROOT_DIR/android/out/target/product/$PRODUCT_NAME

SECURE_SCRIPT=$BUILD_ROOT_DIR/buildscript/tools/signclient.jar
BUILD_CROSS_COMPILE=$BUILD_ROOT_DIR/android/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
BUILD_JOB_NUMBER=`grep processor /proc/cpuinfo|wc -l`

KERNEL_DEFCONFIG=sdm450_sec_defconfig
DEBUG_DEFCONFIG=sdm450_sec_eng_defconfig
SELINUX_DEFCONFIG=selinux_defconfig
SELINUX_LOG_DEFCONFIG=selinux_log_defconfig
DMVERITY_DEFCONFIG=dmverity_defconfig


if  [ "${SEC_BUILD_CONF_TIMA_ENABLED}" == "true" ] ; then
	TIMA_DEFCONFIG=tima_defconfig
fi

if  [ "${SEC_BUILD_CONF_USE_DMVERITY}" == "true" ] ; then
	DMVERITY_DEFCONFIG=dmverity_defconfig
fi

if  [ "${SEC_BUILD_CONF_KERNEL_KASLR}" == "true" ] ; then
	KASLR_DEFCONFIG=kaslr_defconfig
fi

# default value for various sensors
sec_conf_use_fingerprint_tz=true
sec_conf_use_fingerprint_nontz=false
sec_conf_use_iris_tz=false

USE_FINGERPRINT_TZ="false"

MODEL=${BUILD_COMMAND%%_*}

PRODUCT_NAME=`cat ../../model/build_conf/$MODEL/build_conf.$BUILD_COMMAND \
	| sed -n "/^prebuilt-binary/p"  \
	| sed "s/prebuilt-binary[ \t]=[ \t]//g" \
	| sed "s/android\/vendor\/samsung\/configs\///g" \
    | sed "s/\/.*//g"`

echo PRODUCT_NAME=$PRODUCT_NAME

SIGN_PROJECT=`cat ../../model/build_conf/$MODEL/build_conf.$BUILD_COMMAND \
	| sed -n "/^project/p"  \
	| sed "s/project[ \t]*=[ \t]*//g"  \
	| sed "2,/*/d"`

SIGN_SUFFIX=`cat ../../model/build_conf/$MODEL/build_conf.$BUILD_COMMAND \
	| sed -n "/^suffix/p"  \
	| sed "s/suffix[ \t]*=[ \t]*//g" \
	| sed "2,/*/d"`

SIGN_MODEL=$SIGN_PROJECT$SIGN_SUFFIX

echo MODEL=$MODEL
echo PRODUCT_NAME=$PRODUCT_NAME
echo SIGN_PROJECT=$SIGN_PROJECT
echo SIGN_SUFFIX=$SIGN_SUFFIX
echo SIGN_MODEL=$SIGN_MODEL
echo TIMA_DEFCONFIG=$TIMA_DEFCONFIG
echo DMVERITY_DEFCONFIG=$DMVERITY_DEFCONFIG
echo KASLR_DEFCONFIG=$KASLR_DEFCONFIG

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

VARIANT=${CARRIER}
PROJECT_NAME=${VARIANT}
if [ "${REGION}" == "${CARRIER}" ]; then
	VARIANT_DEFCONFIG=sdm450_sec_${MODEL}_${CARRIER}_defconfig
else
	VARIANT_DEFCONFIG=sdm450_sec_${MODEL}_${REGION}_${CARRIER}_defconfig
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
		BOARD_KERNEL_BASE=0x00000000
		BOARD_KERNEL_PAGESIZE=4096
		BOARD_KERNEL_TAGS_OFFSET=0x01E00000
		BOARD_RAMDISK_OFFSET=0x02000000
		BOARD_KERNEL_CMDLINE="console=ttyHSL0,115200,n8 androidboot.console=ttyHSL0 androidboot.hardware=qcom msm_rtb.filter=0x237 ehci-hcd.park=3 lpm_levels.sleep_disabled=1 androidboot.bootdevice=7824900.sdhci earlycon=msm_hsl_uart,0x78af000"
		mkdir -p $BUILD_KERNEL_OUT_DIR
		;;

esac

KERNEL_ZIMG=$BUILD_KERNEL_OUT_DIR/arch/arm64/boot/Image.gz-dtb
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
		cp $BUILD_KERNEL_DIR/tools/dtbTool $DTBTOOL
	fi

	cp $BUILD_KERNEL_OUT_DIR/arch/arm64/boot/dts/samsung/sdm450/*.dtb $BUILD_KERNEL_OUT_DIR/arch/arm64/boot/dts/.

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
	echo "build MODEL="$MODEL""
	echo "build CARRIER="$CARRIER""
	echo "build CERTIFICATION="$CERTIFICATION""
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
        if [ "$BUILD_COMMAND" == "" ]; then
                SECFUNC_PRINT_HELP;
                exit -1;
        fi

	FUNC_CLEAN_DTB

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

    	rsync -cv $KERNEL_ZIMG $PRODUCT_OUT/kernel

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
	if [ -e $PRODUCT_OUT/boot.img ] ; then
		tar cvf boot_${MODEL}_${CARRIER}_${CERTIFICATION}.tar boot.img
	fi

	if [ -e $PRODUCT_OUT/recovery.img ] ; then
		tar cvf recovery_${MODEL}_${CARRIER}_${CERTIFICATION}.tar recovery.img
	fi

	cd $BUILD_ROOT_DIR
	if ! [ -d output ] ; then
		mkdir -p output
	fi

        echo ""
	echo "================================================="
        echo "-->Note, copy to $BUILD_TOP_DIR/../output/ directory"
	echo "================================================="
	if [ -e $PRODUCT_OUT/boot.img ] ; then
		cp $PRODUCT_OUT/boot_${MODEL}_${CARRIER}_${CERTIFICATION}.tar $BUILD_ROOT_DIR/output/boot_${MODEL}_${CARRIER}_${CERTIFICATION}.tar || exit -1
 	fi
	if [ -e $PRODUCT_OUT/recovery.img ] ; then
		cp $PRODUCT_OUT/recovery_${MODEL}_${CARRIER}_${CERTIFICATION}.tar $BUILD_ROOT_DIR/output/recovery_${MODEL}_${CARRIER}_${CERTIFICATION}.tar || exit -1
 	fi

	echo ""
	echo "==================================="
	echo "END   : FUNC_MKBOOTIMG"
	echo "==================================="
	echo ""
}

FUNC_SEANDROID()
{
	if [ -e $PRODUCT_OUT/boot.img ] ; then
		echo -n "SEANDROIDENFORCE" >> $PRODUCT_OUT/boot.img
	fi

	if [ -e $PRODUCT_OUT/recovery.img ] ; then
		echo -n "SEANDROIDENFORCE" >> $PRODUCT_OUT/recovery.img
	fi
}

FUNC_SECURE_SIGNING()
{
	if [ -e $PRODUCT_OUT/boot.img ] ; then
		echo "java -jar $SECURE_SCRIPT -model $SIGN_MODEL -runtype ss_openssl_sha -input $PRODUCT_OUT/boot.img -output $PRODUCT_OUT/signed_boot.img"
		openssl dgst -sha256 -binary $PRODUCT_OUT/boot.img > sig_32
		java -jar $SECURE_SCRIPT -runtype ss_openssl_sha -model $SIGN_MODEL -input sig_32 -output sig_256
		cat $PRODUCT_OUT/boot.img sig_256 > $PRODUCT_OUT/signed_boot.img

		mv -f $PRODUCT_OUT/boot.img $PRODUCT_OUT/unsigned_boot.img
		mv -f $PRODUCT_OUT/signed_boot.img $PRODUCT_OUT/boot.img
	fi

	if [ -e $PRODUCT_OUT/recovery.img ] ; then
		echo "java -jar $SECURE_SCRIPT -model $SIGN_MODEL -runtype ss_openssl_sha -input $PRODUCT_OUT/recovery.img -output $PRODUCT_OUT/signed_recovery.img"
		openssl dgst -sha256 -binary $PRODUCT_OUT/boot.img > sig_32
		java -jar $SECURE_SCRIPT -runtype ss_openssl_sha -model $SIGN_MODEL -input sig_32 -output sig_256
		cat $PRODUCT_OUT/recovery.img sig_256 > $PRODUCT_OUT/signed_recovery.img

		mv -f $PRODUCT_OUT/recovery.img $PRODUCT_OUT/unsigned_recovery.img
		mv -f $PRODUCT_OUT/signed_recovery.img $PRODUCT_OUT/recovery.img
	fi

	CERTIFICATION=CERT
}

SECFUNC_PRINT_HELP()
{
	echo -e '\E[33m'
	echo "Help"
	echo "$0 \$1 \$2 \$3"
	echo "  \$1 : "
	echo "      a6plte_eur_open"
	echo "      a6plte_sea_open"
	echo "      a6plte_ltn_open"
	echo "      a6plte_chn_open"
	echo "      a6plte_kor_ktt"
	echo "      j8plte_cis_ser"
	echo "      j8plte_eur_open"
	echo "      j8plte_sea_open"
	echo "      j8plte_chn_open"
	echo "      j8plte_chn_cmcc"
	echo "      gta2xllte_eur_open"
	echo "      gta2xllte_sea_open"
	echo "      gta2xllte_usa_spr"
	echo "      gta2xllte_usa_vzw"
	echo "      gta2xllte_can_single"
        echo "      gta2xllte_chn_open"
	echo "      gta2xllte_chn_hk"
	echo "      gta2xllte_cis_ser"
	echo "      gta2xlwifi_chn_open"
	echo "      gta2xlwifi_cis_ser"
	echo "  \$2 : "
	echo "      -B or Nothing  (-B : Secure Binary)"
	echo "  \$3 : "
	echo "      -E or Nothing  (-E : SEANDROID Binary)"
	echo -e '\E[0m'
}

function set_export_var() {
	local __var_name=${1}
	local __value=${2}
	local __export_var=${3}

	[ "${__value}x" == "x" ] && \
		__value=false

	echo "set sxport var for ${__export_var}"
	if [[ "${BUILD_CONF}x" != "x" && -f ${BUILD_CONF} ]]; then
		local __get_var=$(grep ${__var_name} ${BUILD_CONF} | tr -d ' ')
		if [[ "${__get_var}x" != "x" && "${__get_var:0:1}" != "#" ]]; then
			__value=$(echo "${__get_var}" | cut -d '#' -f1 | cut -d '=' -f2 | tr -d ' ')
		fi
	fi

	export ${__export_var}=${__value}
	echo "${__export_var}=${__value}"
}
SET_SEC_VARIABLE()
{
	set_export_var fingerprint-tz \
			${sec_conf_use_fingerprint_tz} \
			SEC_BUILD_CONF_USE_FINGERPRINT_TZ
	set_export_var fingerprint-nontz \
			${sec_conf_use_fingerprint_nontz} \
			SEC_BUILD_CONF_USE_FINGERPRINT_NONTZ
	set_export_var iris-tz \
			${sec_conf_use_iris_tz} \
			SEC_BUILD_CONF_USE_IRIS_TZ
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
