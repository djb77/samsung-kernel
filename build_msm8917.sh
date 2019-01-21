#!/bin/bash
# MSM8937 M kernel build script v0.5

BUILD_COMMAND=$1
if [ "$BUILD_COMMAND" == "elitelte_chn_open" ]; then
	BUILD_PRODUCT=elitelte_chn
	PRODUCT_NAME=eliteltechn
	SIGN_MODEL=SM-G1600_CHN_CHC_ROOT0
	
elif [ "$BUILD_COMMAND" == "elitexlte_chn" ]; then
	BUILD_PRODUCT=elitexlte_chn
	PRODUCT_NAME=elitexltechn
	SIGN_MODEL=
	
elif [ "$BUILD_COMMAND" == "j3poplte_usa_spr" ]; then
	BUILD_PRODUCT=j3poplte_usa_spr
	PRODUCT_NAME=j3popltespr
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "j3poplte_usa_usc" ]; then
	BUILD_PRODUCT=j3poplte_usa_usc
	PRODUCT_NAME=j3poplteusc
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "j3poplte_usa_vzw" ]; then
	BUILD_PRODUCT=j3poplte_usa_vzw
	PRODUCT_NAME=j3popltevzw
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "j3poplte_usa_vzwpp" ]; then
	BUILD_PRODUCT=j3poplte_usa_vzwpp
	PRODUCT_NAME=j3popltevzwpp
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "j3poplte_usa_acg" ]; then
	BUILD_PRODUCT=j3poplte_usa_acg
	PRODUCT_NAME=j3poplteacg
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "j3poplte_usa_lra" ]; then
	BUILD_PRODUCT=j3poplte_usa_lra
	PRODUCT_NAME=j3popltelra
	SIGN_MODEL=
	
elif [ "$BUILD_COMMAND" == "j1y17lte_eur_open" ]; then
	BUILD_PRODUCT=j1y17lte_eur_open
	PRODUCT_NAME=j1y17ltexx
	SIGN_MODEL=
	
elif [ "$BUILD_COMMAND" == "j2y18lte_mea_open" ]; then
	BUILD_PRODUCT=j2y18lte_mea_open
	PRODUCT_NAME=j2y18ltejx
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "j5y18lte_swa_open" ]; then
	BUILD_PRODUCT=j5y18lte_swa_open
	PRODUCT_NAME=j5y18ltedd
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "j6primelte_swa_open" ]; then
	BUILD_PRODUCT=j6primelte_swa_open
	PRODUCT_NAME=j6primeltedd
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "j6primelte_eur_open" ]; then
	BUILD_PRODUCT=j6primelte_eur_open
	PRODUCT_NAME=j6primeltexx
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "j6primelte_cis_ser" ]; then
	BUILD_PRODUCT=j6primelte_cis_ser
	PRODUCT_NAME=j6primelteser
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "j4primelte_sea_open" ]; then
	BUILD_PRODUCT=j4primelte_sea_open
	PRODUCT_NAME=j4primeltedx
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "j4primelte_cis_ser" ]; then
	BUILD_PRODUCT=j4primelte_cis_ser
	PRODUCT_NAME=j4primelteser
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "j4primelte_cis_open" ]; then
	BUILD_PRODUCT=j4primelte_cis_open
	PRODUCT_NAME=j4primeltecis
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "j4primelte_sea_xtc" ]; then
	BUILD_PRODUCT=j4primelte_sea_xtc
	PRODUCT_NAME=j4primeltextc
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "j4corelte_mea_open" ]; then
	BUILD_PRODUCT=j4corelte_mea_open
	PRODUCT_NAME=j4coreltejx
	SIGN_MODEL=
	
elif [ "$BUILD_COMMAND" == "j4corelte_ltn_open" ]; then
	BUILD_PRODUCT=j4corelte_ltn_open
	PRODUCT_NAME=j4corelteub
	SIGN_MODEL=
	
elif [ "$BUILD_COMMAND" == "gta2slte_eur_open" ]; then
	BUILD_PRODUCT=gta2slte_eur_open
	PRODUCT_NAME=gta2sltexx
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "gta2slte_sea_open" ]; then
	BUILD_PRODUCT=gta2slte_sea_open
	PRODUCT_NAME=gta2sltedx
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "gta2slte_cis_ser" ]; then
	BUILD_PRODUCT=gta2slte_cis_ser
	PRODUCT_NAME=gta2slteser
	SIGN_MODEL=
	
elif [ "$BUILD_COMMAND" == "gta2slte_swa_open" ]; then
	BUILD_PRODUCT=gta2slte_swa_open
	PRODUCT_NAME=gta2sltedd
	SIGN_MODEL=
	
elif [ "$BUILD_COMMAND" == "gta2slte_usa_att" ]; then
	BUILD_PRODUCT=gta2slte_usa_att
	PRODUCT_NAME=gta2slteuc
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "gta2slte_chn_open" ]; then
	BUILD_PRODUCT=gta2slte_chn_open
	PRODUCT_NAME=gta2sltezc
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "gta2slte_kor_skt" ]; then
	BUILD_PRODUCT=gta2slte_kor_skt
	PRODUCT_NAME=gta2slteskt
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "gta2slte_kor_ktt" ]; then
	BUILD_PRODUCT=gta2slte_kor_ktt
	PRODUCT_NAME=gta2sltektt
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "gta2slte_kor_lgt" ]; then
	BUILD_PRODUCT=gta2slte_kor_lgt
	PRODUCT_NAME=gta2sltelgt
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "gta2slte_chn_hk" ]; then
	BUILD_PRODUCT=gta2slte_chn_hk
	PRODUCT_NAME=gta2sltezh
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "gta2swifi_sea_open" ]; then
	BUILD_PRODUCT=gta2swifi_sea_open
	PRODUCT_NAME=gta2swifidx
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "gta2swifi_chn_open" ]; then
	BUILD_PRODUCT=gta2swifi_chn_open
	PRODUCT_NAME=gta2swifizc
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "gtesy18lte_usa_vzw" ]; then
	BUILD_PRODUCT=gtesy18lte_usa_vzw
	PRODUCT_NAME=gtesy18ltevzw
	SIGN_MODEL=
	
elif [ "$BUILD_COMMAND" == "gtaslitelte_usa_vzw" ]; then
	BUILD_PRODUCT=gtaslitelte_usa_vzw
	PRODUCT_NAME=gtasliteltevzw
	SIGN_MODEL=
	
elif [ "$BUILD_COMMAND" == "gtaslitelte_usa_vzwkids" ]; then
	BUILD_PRODUCT=gtaslitelte_usa_vzwkids
	PRODUCT_NAME=gtasliteltevzwkids
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "on5xllte_chn_open" ]; then
	BUILD_PRODUCT=on5xllte_chn
	PRODUCT_NAME=on5xlltechn
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "j3y17qlte_chn_cmcc" ]; then
	BUILD_PRODUCT=j3y17qlte_chncmcc
	PRODUCT_NAME=j3y17qltecmcc
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "j3y17qlte_swa_open" ]; then
	BUILD_PRODUCT=j3y17qlte_swa_open
	PRODUCT_NAME=j3y17qltedd
	SIGN_MODEL=

elif [ "$BUILD_COMMAND" == "j2golte_swa_ins" ]; then
	BUILD_PRODUCT=j2golte_swa_ins
	PRODUCT_NAME=j2golteswains
	SIGN_MODEL=

else
#default product
        PRODUCT_NAME=$BUILD_COMMAND;
	SIGN_MODEL=
fi

BUILD_WHERE=$(pwd)
BUILD_KERNEL_DIR=$BUILD_WHERE
BUILD_ROOT_DIR=$BUILD_KERNEL_DIR/../..
BUILD_KERNEL_OUT_DIR=$BUILD_ROOT_DIR/android/out/target/product/$PRODUCT_NAME/obj/KERNEL_OBJ
PRODUCT_OUT=$BUILD_ROOT_DIR/android/out/target/product/$PRODUCT_NAME


SECURE_SCRIPT=$BUILD_ROOT_DIR/buildscript/tools/signclient.jar
BUILD_CROSS_COMPILE=$BUILD_ROOT_DIR/android/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi-
BUILD_JOB_NUMBER=`grep processor /proc/cpuinfo|wc -l`

# Default Python version is 2.7
mkdir -p bin
ln -sf /usr/bin/python2.7 ./bin/python
export PATH=$(pwd)/bin:$PATH

KERNEL_DEFCONFIG=msm8937_sec_defconfig
DEBUG_DEFCONFIG=msm8937_sec_eng_defconfig
DMVERITY_DEFCONFIG=dmverity_defconfig
SELINUX_DEFCONFIG=selinux_defconfig
SELINUX_LOG_DEFCONFIG=selinux_log_defconfig

if  [ "${SEC_BUILD_CONF_TIMA_ENABLED}" == "true" ] ; then
	TIMA_DEFCONFIG=tima_defconfig
fi

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
	VARIANT_DEFCONFIG=msm8937_sec_${MODEL}_${CARRIER}_defconfig
else
	VARIANT_DEFCONFIG=msm8937_sec_${MODEL}_${REGION}_${CARRIER}_defconfig
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
		BOARD_KERNEL_CMDLINE="console=ttyHSL0,115200,n8 androidboot.console=ttyHSL0 androidboot.hardware=qcom user_debug=31 msm_rtb.filter=0x3F ehci-hcd.park=3 androidboot.bootdevice=7824900.sdhci"
#		BOARD_KERNEL_CMDLINE="console=ttyHSL0,115200,n8 androidboot.hardware=qcom user_debug=31 msm_rtb.filter=0x37 ehci-hcd.park=3"
#		BOARD_KERNEL_CMDLINE="console=ttyHSL0,115200,n8 androidboot.hardware=qcom androidboot.bootdevice=soc.0/7824900.sdhci user_debug=31 msm_rtb.filter=0x3F ehci-hcd.park=3 video=vfb:640x400,bpp=32,memsize=3072000 earlyprintk"
		mkdir -p $BUILD_KERNEL_OUT_DIR
		;;

esac

KERNEL_ZIMG=$BUILD_KERNEL_OUT_DIR/arch/arm/boot/zImage
DTC=$BUILD_KERNEL_OUT_DIR/scripts/dtc/dtc

FUNC_CLEAN_DTB()
{
	if ! [ -d $BUILD_KERNEL_OUT_DIR/arch/arm/boot/dts ] ; then
		echo "no directory : "$BUILD_KERNEL_OUT_DIR/arch/arm/boot/dts""
	else
		echo "rm files in : "$BUILD_KERNEL_OUT_DIR/arch/arm/boot/dts/*.dtb""
		rm $BUILD_KERNEL_OUT_DIR/arch/arm/boot/dts/*.dtb
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

	cp $BUILD_KERNEL_OUT_DIR/arch/arm64/boot/dts/samsung/*.dtb $BUILD_KERNEL_OUT_DIR/arch/arm/boot/dts/.	

	echo "$DTBTOOL -o $INSTALLED_DTIMAGE_TARGET -s $BOARD_KERNEL_PAGESIZE \
		-p $BUILD_KERNEL_OUT_DIR/scripts/dtc/ $BUILD_KERNEL_OUT_DIR/arch/arm/boot/dts/"
		$DTBTOOL -o $INSTALLED_DTIMAGE_TARGET -s $BOARD_KERNEL_PAGESIZE \
		-p $BUILD_KERNEL_OUT_DIR/scripts/dtc/ $BUILD_KERNEL_OUT_DIR/arch/arm/boot/dts/

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

        if [ "$BUILD_PRODUCT" == "" ]; then
                SECFUNC_PRINT_HELP;
                exit -1;
        fi

	FUNC_CLEAN_DTB

	make -C $BUILD_KERNEL_DIR O=$BUILD_KERNEL_OUT_DIR -j$BUILD_JOB_NUMBER ARCH=arm \
			CROSS_COMPILE=$BUILD_CROSS_COMPILE \
			$KERNEL_DEFCONFIG VARIANT_DEFCONFIG=$VARIANT_DEFCONFIG \
			DEBUG_DEFCONFIG=$DEBUG_DEFCONFIG \
			SELINUX_DEFCONFIG=$SELINUX_DEFCONFIG \
			SELINUX_LOG_DEFCONFIG=$SELINUX_LOG_DEFCONFIG || exit -1

	make -C $BUILD_KERNEL_DIR O=$BUILD_KERNEL_OUT_DIR -j$BUILD_JOB_NUMBER ARCH=arm \
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
		cp $BUILD_ROOT_DIR/android/kernel/tools/mkbootimg $MKBOOTIMGTOOL
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
	echo "	for elitelte chn use elitelte_chn_open"
	echo "	for j3poplte usa spr use j3poplte_usa_spr"
	echo "	for j3poplte usa usc use j3poplte_usa_usc"
	echo "	for j3poplte usa vzw use j3poplte_usa_vzw"
	echo "	for j3poplte usa vzwpp use j3poplte_usa_vzwpp"
	echo "	for j3poplte usa acg use j3poplte_usa_acg"
	echo "	for j3poplte usa lra use j3poplte_usa_lra"
	echo "	for j1y17lte eur open use j1y17lte_eur_open"
	echo "	for j2y18lte mea open use j2y18lte_mea_open"
	echo "	for j5y18lte swa open use j5y18lte_swa_open"
	echo "	for j6primelte swa open use j6primelte_swa_open"
	echo "	for j6primelte eur open use j6primelte_eur_open"
	echo "	for j6primelte cis ser use j6primelte_cis_ser"
	echo "	for j4primelte sea open use j4primelte_sea_open"
	echo "	for j4primelte cis ser use j4primelte_cis_ser"
	echo "	for j4primelte cis open use j4primelte_cis_open"
	echo "	for j4primelte sea xtc use j4primelte_sea_xtc"
	echo "	for j4corelte mea open use j4corelte_mea_open"
	echo "	for j3y17qlte swa open use j3y17qlte_swa_open"
	echo "	for gta2slte eur open use gta2slte_eur_open"
	echo "	for gta2slte sea open use gta2slte_sea_open"
	echo "	for gta2slte cis ser use gta2slte_cis_ser"
	echo "	for gta2slte swa open use gta2slte_swa_open"
	echo "	for gta2slte usa att use gta2slte_usa_att"
	echo "	for gta2slte chn open use gta2slte_chn_open"
	echo "	for gta2slte kor skt use gta2slte_kor_skt"
	echo "	for gta2slte kor ktt use gta2slte_kor_ktt"
	echo "	for gta2slte kor lgt use gta2slte_kor_lgt"
	echo "	for gta2slte chn hk use gta2slte_chn_hk"
	echo "	for gta2swifi sea open use gta2swifi_sea_open"
	echo "	for gta2swifi chn open use gta2swifi_chn_open"
	echo "	for gtesy18lte eur open use gtesy18lte_usa_vzw"
	echo "	for gtaslitelte usa vzw use gtaslitelte_usa_vzw"
	echo "	for gtaslitelte usa vzwkids use gtaslitelte_usa_vzwkids"
	echo "  \$2 : "
	echo "	-B or Nothing  (-B : Secure Binary)"
	echo "  \$3 : "
	echo "	-E or Nothing  (-E : SEANDROID Binary)"
	echo -e '\E[0m'
}

SET_SEC_VARIABLE()
{
	local __fingerprint_tz_var_name="fingerprint-tz"
	local __use_fingerprint_tz="false"

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
