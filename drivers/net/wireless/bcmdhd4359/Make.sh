#!/bin/bash
##################################################
## Easy Make script for Broadcom WiFi Module build
##   Ver 0.2
##################################################


###############
# help message
###############
usage() {
	echo "Make.sh -[option] [arg]"
	echo
	echo "Example)"
	echo "  Default build is Panda platform"
	echo "     $ Make.sh  "
	echo "  -p) Build for Panda platform"
	echo "     $ Make.sh -p panda  "
	echo "  -p) Build for M0 platform"
	echo "     $ Make.sh -p M0 "
	echo "  -p) Build for D2 platform"
	echo "     $ Make.sh -p D2 "
	echo
	echo "  -b) Specific build brand "
	echo "     $ Make.sh -p M0 -b dhd-cdc-sdmmc-android-panda-icsmr1-cfg80211-oob-customer_hw4"
	echo "  -s) Specific SDK location(Kernel source location) "
	echo "     $ Make.sh -p M0 -s /tools/linux/src/linux-3.0.15-m0-0502"
	echo "  -t) Specific Toolchain location "
	echo "     $ Make.sh -p M0 -t /projects/hnd/tools/linux/hndtools-arm-eabi-4.4.3/bin"
	echo ""
	echo "  -a) Build All platform at one time "
	echo "     $ Make.sh -a "
	echo "   -l) Platform List for all build  "
	echo "       $ Make.sh -a -l \"JA JF\""
	echo ""
	echo "  all new platform example)"
	echo "     $ Make.sh -p NewGPhone -t /projects/hnd/tools/linux/hndtools-arm-eabi-4.4.3/bin "
	echo "                 -s /tools/linux/src/linux-4.5.6-NewGPhone-120716"
	echo "                 -b dhd-cdc-sdmmc-android-newgphone-keylimepie-cfg80211-oob-customer_hw4"
	echo ""
	echo "  clean) Clean up"
	echo "     $ Make.sh clean "
	echo
	echo "Predefined Platform list"
	grep $'^\t.*)$' $0 | grep -v '#' | grep -v $'^\t\t'
}


###################
# Script Arguments
###################
while getopts 'ab:deg:hl:s:t:p:fw' opt
do

	case $opt in
		# Build all platform at one time
		a)
			BUILD_ALL=y
			;;
		# Specific Build brand
		b)
			PRIVATE_BUILD_BRAND=$OPTARG
			;;

		# Help message
		h)
			usage
			exit;
			;;
		# build list for all build
		l)
			BUILD_LIST=$OPTARG
			;;
		# SDK kernel source specific location
		s)
			LINUXDIR=$OPTARG
			;;
		# Toolchain specific location
		t)
			PRIVATE_TOOLCHAIN_PATH=$OPTARG
			;;
		# Target Platform
		p)
			PLATFORM=$OPTARG
			;;
		# After Mogrified source (flat directory type)
		f)
			FLAT_TYPE=y
			;;
		g)
			CUSTOM_GCC=$OPTARG
			;;
	esac
done

shift $(($OPTIND - 1))
REMAIN=$1

#######################
# Auto FLAT_TYPE check
#######################
if [ "$FLAT_TYPE" = "" ];then
	# Check mogrified source or not
	if [ -e Makefile ] && [ ! -e src/dhd/linux ];then
		FLAT_TYPE=y
	fi
fi
	


###################
# Default Variable
###################
if [ "$FLAT_TYPE" = "y" ];then
	# for FLAT_TYPE (after mogrified source)
	MAKE_PATH=""
else
	# for directory type (pure svn checkout source)
	MAKE_PATH="-C src/dhd/linux"
fi
if [ "$ANDROID_NDK" = "" ] ; then
	ANDROID_NDK=android_ndk_r6b
fi

#CROSS_COMPILE=arm-eabi-
CROSS_COMPILE=/home/broadcom/kellylee/GP_SDK/arm-eabi-4.8/bin/arm-eabi-
ARCH=arm
CPU_JOBS=`cat /proc/cpuinfo | grep processor | wc -l`
BUILTIN_TARGET_DIR=BUILTIN_OUTPUT

###########
# Clean up
###########
if [ "$1" = "clean" ]; then
	echo "make $MAKE_PATH clean"
	make $MAKE_PATH clean --no-print-directory QUIET_BUILD=1
	exit;
fi


##########################################
# Set Default Variable depend on Platform
##########################################
case $PLATFORM in


###################
# 43241
# - SPC
###################

	SPC)
		PLATFORM=SPC
		BUILD_BRAND=dhd-Makefile_hw4-android_jbp-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.4.3/bin
		LINUXVER=3.4.5-SPC-JB-MODULE
		;;


###################
# 4330
# - S2V / Heat 3G
###################

	S2V)
		PLATFORM=S2V_JB
		BUILD_BRAND=dhd-Makefile_hw4-android_jbp-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.4.3/bin
		LINUXVER=3.0.31-S2V-JB-MODULE
		;;

	HEAT3G)
		PLATFORM=HEAT3G_JB
		BUILD_BRAND=dhd-Makefile_hw4-android_kk-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.7/bin
		LINUXVER=3.0.31-Heat3G-KK-MODULE
		;;


###############
# 4334
# - Galaxy S3 / Note2
###############

	D2_JB)
		PLATFORM=D2_JB
		BUILD_BRAND=dhd-Makefile_hw4-android_jb-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.6/bin
		LINUXVER=3.0.31-D2-JB-MODULE
		;;

	D2 | D2_KK)
		PLATFORM=D2_KK
		BUILD_BRAND=dhd-Makefile_hw4-android_jb-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.6/bin
		LINUXVER=3.4.0-D2-KK-MODULE
		;;

	M0)
		PLATFORM=M0_JB
		BUILD_BRAND=dhd-Makefile_hw4-android_jbp-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.4.3/bin
		LINUXVER=3.0.31-M0-JB-MODULE
		;;

	M0_KOR)
		PLATFORM=M0_KOR
		BUILD_BRAND=dhd-Makefile_hw4-android_jb-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.4.3/bin
		LINUXVER=3.0.31-M0_KOR-JB-MODULE
		;;

	T0)
		PLATFORM=T0
		BUILD_BRAND=dhd-Makefile_hw4-android_jb-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.4.3/bin
		LINUXVER=3.0.31-T0-JB-MODULE
		;;

	T0_KOR)
		PLATFORM=T0_KOR
		BUILD_BRAND=dhd-Makefile_hw4-android_jb-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.4.3/bin
		LINUXVER=3.0.31-T0_KOR-JB-MODULE
		;;

	M2_3G)
		PLATFORM=M2_3G
		BUILD_BRAND=dhd-Makefile_hw4-android_kk-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.7/bin
		LINUXVER=3.4.39-M2_3G-KK-MODULE
		;;

	M2_LTE)
		PLATFORM=M2_LTE
		BUILD_BRAND=dhd-Makefile_hw4-android_kk-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.7/bin
		LINUXVER=3.4.39-M2_LTE-KK-MODULE
		;;

###############
# 4334w
# - Gear
###############

	SPRAT_BUILTIN)
		BUILTIN=y
		BUILTIN_SDK=/tools/linux/src/3.10.40-SPRAT-LP-BUILTIN
		BUILTIN_MODEL=SPRAT
		CROSS_COMPILE=/projects/hnd/tools/linux/hndtools-arm-eabi-4.8/bin/arm-eabi-
		;;


###############
# 4335
# - Galaxy S4
###############

	JA_KK|JA_3G_KK)
		PLATFORM=JA_3G_KK
		BUILD_BRAND=dhd-Makefile_hw4-android_kk-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.6/bin
		LINUXVER=3.4.5-JA_3G-KK-MODULE
		;;

	JA|JA_3G|JA_3G_LP)
		PLATFORM=JA_3G_LP
		BUILD_BRAND=dhd-Makefile_hw4-android_lp-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.6/bin
		LINUXVER=3.4.5-JA_3G-LP-MODULE
		# PRE_CMD/POST_CMD is WAR for set_power() difference
		PRE_CMD="sed -i 's/int (*set_power)(int val);/int (*set_power)(int val, bool b0rev);/' dhd_linux_platdev.c && "
		PRE_CMD+="sed -i 's/err = plat_data->set_power(on);/err = plat_data->set_power(on, 1);/' dhd_linux_platdev.c"
		POST_CMD="sed -i 's/int (*set_power)(int val, bool b0rev);/int (*set_power)(int val);/' dhd_linux_platdev.c && "
		POST_CMD+="sed -i 's/err = plat_data->set_power(on, 1);/err = plat_data->set_power(on);/' dhd_linux_platdev.c"
		;;

	JF_ATT_JB)
		PLATFORM=JF_ATT_JB
		BUILD_BRAND=dhd-Makefile_hw4-android_jbp-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.7/bin
		LINUXVER=3.4.0-JF_ATT-JB-MODULE
		;;

	JF|JF_ATT)
		PLATFORM=JF_ATT_KK
		BUILD_BRAND=dhd-Makefile_hw4-android_kk-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.7/bin
		LINUXVER=3.4.0-JF_ATT-KK-MODULE
		;;

	JF_SPR)
		PLATFORM=JF-SPR
		BUILD_BRAND=dhd-Makefile_hw4-android_jbp-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.6/bin
		LINUXVER=3.4.0-JF_SPR-JB-MODULE
		;;

	JF_SKT)
		PLATFORM=JF_SKT
		BUILD_BRAND=dhd-Makefile_hw4-android_jbp-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.7/bin
		LINUXVER=3.4.0-JF_SKT-JB-MODULE
		;;


#################
# 4339
# - Galaxy Note3
#################

	H_3G)
		PLATFORM=H_3G
		BUILD_BRAND=dhd-Makefile_hw4-android_jbp-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.6/bin
		LINUXVER=3.4.39-H_3G-JB-MODULE
		;;

	H_ATT_KK)
		PLATFORM=H_ATT_KK
		BUILD_BRAND=dhd-Makefile_hw4-android_kk-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.6/bin
		LINUXVER=3.4.0-H_ATT-KK-MODULE
		;;

	H_ATT|H_ATT_LP)
		PLATFORM=H_ATT_LP
		BUILD_BRAND=dhd-Makefile_hw4-android_lp-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.6/bin
		LINUXVER=3.4.0-H_ATT-LP-MODULE
		;;

	H_LTE_JB)
		PLATFORM=H_LTE_JB
		BUILD_BRAND=dhd-Makefile_hw4-android_jbp-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.7/bin
		LINUXVER=3.4.0-H_LTE-JB-MODULE
		;;

	H_LTE|H_LTE_KK)
		PLATFORM=H_LTE_KK
		BUILD_BRAND=dhd-Makefile_hw4-android_kk-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.7/bin
		LINUXVER=3.4.0-H_LTE-KK-MODULE
		;;

#################
# 4343
# - Vivalto
#################

	Vivalto_3G)
		PLATFORM=Vivalto_3G
		BUILD_BRAND=dhd-Makefile_hw4-android_kk-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.7/bin
		LINUXVER=3.10.17-Vivalto_3G-KK-MODULE
		;;

	Vivalto_Higgs|Vivalto_Higgs2G)
		PLATFORM=Vivalto_Higgs2G
		BUILD_BRAND=dhd-Makefile_hw4-android_kk-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.7/bin
		LINUXVER=3.10.17-Vivalto_Higgs2G-KK-MODULE
		;;

	Vivalto_LTE)
		PLATFORM=Vivalto_LTE
		BUILD_BRAND=dhd-Makefile_hw4-android_kk-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.7/bin
		LINUXVER=3.10.17-Vivalto_LTE-KK-MODULE
		;;

#################
# 43438
# - GrandNeoVE
#################

	GRANDNEOVE_3G_BUILTIN)
		PLATFORM=GRANDNEOVE_3G
		BUILTIN=y
		BUILTIN_SDK=/tools/linux/src/linux-3.10.17-GRANDNEOVE_3G-KK-BUILTIN
		BUILTIN_MODEL=TRE
		CROSS_COMPILE=/projects/hnd/tools/linux/hndtools-arm-eabi-4.7/bin/arm-eabi-
		;;

#################
# 43438
# - GrandPrimeVE
#################

	GRANDPRIMEVE_3G_BUILTIN)
		PLATFORM=GRANDPRIMEVE_3G
		BUILTIN=y
		BUILTIN_SDK=/home/broadcom/kellylee/GP_SDK1
		BUILTIN_MODEL=TRE
		CROSS_COMPILE=/home/broadcom/kellylee/GP_SDK/arm-eabi-4.8/bin/arm-eabi-
		;;
#################
# 43455
# - A8
#################

	A8_BUILTIN)
		PLATFORM=A8_SKT
		BUILTIN=y
		BUILTIN_SDK=/home/broadcom/wonil/A8_SDK_20150513
		BUILTIN_MODEL=TRE
		CROSS_COMPILE=/home/broadcom/kellylee/GP_SDK/arm-eabi-4.8/bin/arm-eabi-
		;;
	

###############
# 4354
# - Galaxy S5 / Note Pro
###############

	K_ATT)
		PLATFORM=K_ATT
		BUILD_BRAND=dhd-Makefile_hw4-android_kk-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.7/bin
		LINUXVER=3.4.0-K_ATT-KK-MODULE
		;;

	K_LTE_KK)
		PLATFORM=K_LTE_KK
		BUILD_BRAND=dhd-Makefile_hw4-android_kk-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.7/bin
		LINUXVER=3.4.0-K_LTE-KK-MODULE
		;;

	K_LTE)
		PLATFORM=K_LTE_LP
		BUILD_BRAND=dhd-Makefile_hw4-android_lp-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.7/bin
		LINUXVER=3.4.0-K_LTE-LP-MODULE
		;;

	V1_3G)
		PLATFORM=V1_3G
		BUILD_BRAND=dhd-Makefile_hw4-android_kk-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.6/bin
		LINUXVER=3.4.39-V1_3G-KK-MODULE
		;;

	V1_LTE)
		PLATFORM=V1_LTE
		BUILD_BRAND=dhd-Makefile_hw4-android_kk-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.7/bin
		LINUXVER=3.4.0-V1_LTE-KK-MODULE
		;;

	V1_WIFI)
		PLATFORM=V1_WIFI
		BUILD_BRAND=dhd-Makefile_hw4-android_kk-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.6/bin
		LINUXVER=3.4.39-V1_WIFI-KK-MODULE
		;;

	V2|V2_LTE)
		PLATFORM=V2_LTE
		BUILD_BRAND=dhd-Makefile_hw4-android_kk-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.7/bin
		LINUXVER=3.4.0-V2-KK-MODULE
		;;

###############
# 4358 A1
# - Galaxy Note4
###############

	TR_SPR)
		PLATFORM=TR_SPR
		BUILD_BRAND=dhd-Makefile_hw4-android_kk-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.7/bin
		LINUXVER=3.10.0-TR_SPR-KK-MODULE
		;;


	TR_LTE_KK)
		PLATFORM=TR_LTE_KK
		BUILD_BRAND=dhd-Makefile_hw4-android_kk-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.7/bin
		LINUXVER=3.10.0-TR_LTE-KK-MODULE
		;;

	TR_LTE|TR_LTE_LP)
		PLATFORM=TR_LTE_LP
		BUILD_BRAND=dhd-Makefile_hw4-android_lp-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.7/bin
		LINUXVER=3.10.40-TR_LTE-LP-MODULE
		;;

	TR_LTE_BUILTIN)
		PLATFORM=TR_LTE_LP_BUILTIN
		BUILTIN=y
		BUILTIN_SDK=/tools/linux/src/linux-3.10.40-TR_LTE-LP-BUILTIN
		BUILTIN_MODEL=TR
		CROSS_COMPILE=/projects/hnd/tools/linux/hndtools-arm-eabi-4.8/bin/arm-eabi-
		;;

	TRE_LTE)
		PLATFORM=TRE_LTE_KK
		BUILD_BRAND=dhd-Makefile_hw4-android_kk-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.6/bin
		LINUXVER=3.10.9-TRE_LTE-KK-MODULE
		;;

	TRE_LTE_BUILTIN)
		PLATFORM=TRE_LTE_LP_BUILTIN
		BUILTIN=y
		BUILTIN_SDK=/tools/linux/src/linux-3.10.9-TRE_LTE-LP-BUILTIN
		BUILTIN_MODEL=TRE
		CROSS_COMPILE=/projects/hnd/tools/linux/hndtools-arm-eabi-4.8/bin/arm-eabi-
		;;


###############
# 4358 A3
# - Galaxy S6
###############

	ZERO_LTE_FLAT_BUILTIN)
		PLATFORM=ZERO_LTE_FLAT_BUILTIN
		BUILTIN=y
		BUILTIN_SDK=/tools/linux/src/linux-3.10.61-ZERO_LTE_FLAT-LP-BUILTIN
		BUILTIN_MODEL=TRE
		CROSS_COMPILE=/projects/hnd/tools/linux/aarch64-linux-android-4.9/bin/aarch64-linux-android-
		;;

	ZERO_SKT_EDGE_BUILTIN)
		PLATFORM=ZERO_SKT_EDGE_BUILTIN
		BUILTIN=y
		BUILTIN_SDK=/tools/linux/src/linux-3.10.61-ZERO_SKT_EDGE-LP-BUILTIN
		BUILTIN_MODEL=TRE
		CROSS_COMPILE=/projects/hnd/tools/linux/aarch64-linux-android-4.9/bin/aarch64-linux-android-
		;;


#################
# 4359
# - Galaxy Note5
#################

	NOBLE_BUILTIN)
		PLATFORM=NOBLE_BUILTIN
		BUILTIN=y
		BUILTIN_SDK=/tools/linux/src/linux-3.10.61-NOBLE-LP-BUILTIN
		BUILTIN_MODEL=TRE
		CROSS_COMPILE=/projects/hnd/tools/linux/aarch64-linux-android-4.9/bin/aarch64-linux-android-
		;;

	NOBLE_TEST)
		PLATFORM=NOBLE_TEST
		BUILTIN=y
		BUILTIN_SDK=/home/broadcom/work2/noble_0520_taekhun/noble_0520_SDK
		BUILTIN_MODEL=TRE
		CROSS_COMPILE=/home/broadcom/kellylee/GP_SDK/arm-eabi-4.8/bin/arm-eabi-
		;;

	ZEN|ZEN_BUILTIN)
		PLATFORM=ZEN_BUILTIN
		BUILTIN=y
		BUILTIN_SDK=/tools/linux/src/linux-3.10.61-ZEN-LP-BUILTIN
		BUILTIN_MODEL=TRE
		CROSS_COMPILE=/home/broadcom/work2/ZEN_0429_taekhun/android/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
		;;

###################################
# x86 Intel
# - Fedora / Panda / Brix / etc
###################################

	panda|PANDA)
		PLATFORM=panda
		BUILD_BRAND=dhd-Makefile_hw4-android_jb-customer_hw4
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-arm-eabi-4.4.0/bin
		LINUXVER=3.0.8-panda
		;;

	fc15|FC15)
		PLATFORM=fc15
		CROSS_COMPILE=
		ARCH=x86
		BUILD_BRAND=dhd-cdc-sdstd
		LINUXVER=2.6.38.6-26.rc1.fc15.i686.PAE
		;;
	fc18|FC18)
		PLATFORM=fc18
		CROSS_COMPILE=
		ARCH=x86
		BUILD_BRAND=dhd-cdc-sdstd
		LINUXVER=3.6.10-4.fc18.i686.PAE
		;;
	fc19|FC19)
		PLATFORM=fc19
		CROSS_COMPILE=
		ARCH=x86
		BUILD_BRAND=dhd-msgbuf-pciefd
		LINUXVER=3.11.1-200.fc19.x86_64
		CUSTOM_GCC=4.8.1
		;;

	x86pcie)
		CROSS_COMPILE=
		ARCH=x86
		BUILD_BRAND=dhd-msgbuf-pciefd-android-cfg80211
		LINUXVER=3.10.11-aia1+-androidx86
		CUSTOM_GCC=4.8.1
		;;

	x86_64_kk)
		CROSS_COMPILE=x86_64-linux-
		ARCH=x86
		BUILD_BRAND=dhd-msgbuf-pciefd-android-cfg80211
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-x86_64-linux-glibc2.7-4.6/bin
		LINUXVER=3.10.20-gcc264a6-androidx86
		;;

	brix_kk_hw4|BRIX_KK_HW4)
		CROSS_COMPILE=x86_64-linux-
		ARCH=x86
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-x86_64-linux-glibc2.7-4.6/bin
		LINUXVER=3.10.20-gcc264a6-androidx86
		MAKE_ARGS="CONFIG_BCMDHD_PCIE=m CONFIG_BCM4358=m"
		# PRE_CMD/POST_CMD is WAR for compile from lollipop branch(DHD 1.47)
		PRE_CMD="if [ -f Makefile.kk ] && [ -f Makefile ]; then \
				mv Makefile Makefile.real_original && \
				 mv Makefile.kk Makefile;\
			   	fi && "
		PRE_CMD+="sed -i 's/#EXTRA_LDFLAGS += --strip-debug/EXTRA_LDFLAGS += --strip-debug/' Makefile"
		POST_CMD="if [ -f Makefile.real_original ] && [ -f Makefile ]; then \
				mv Makefile Makefile.kk && \
				mv Makefile.real_original Makefile; 
			fi"
		;;


###################################
# Util build
# - dhd / wl utils
###################################

	wlarm_android)
		CROSS_COMPILE=arm-linux-androideabi-
		ARCH=arm_android
		TARGETENV=android
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/arm-linux-androideabi-4.6/bin
		UTIL_BUILD=wl
		;;

	wlarm64_android)
		CROSS_COMPILE=aarch64-linux-android-
		ARCH=arm64_android
		TARGETENV=android
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/aarch64-linux-android-4.9/bin
		UTIL_BUILD=wl
		;;
	
	dhdarm_android)
		CROSS_COMPILE=arm-linux-androideabi-
		ARCH=arm_android
		TARGETENV=android
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/arm-linux-androideabi-4.6/bin
		UTIL_BUILD=dhd
		;;

	dhdarm64_android)
		CROSS_COMPILE=aarch64-linux-android-
		ARCH=arm64_android
		TARGETENV=android
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/aarch64-linux-android-4.9/bin
		UTIL_BUILD=dhd
		;;
	

	dhdarm)
		CROSS_COMPILE=arm-none-linux-gnueabi-
		ARCH=arm
		TARGETENV=arm
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-armeabi-linux-2009q3/bin
		UTIL_BUILD=dhd
		;;

	wlarm)
		CROSS_COMPILE=arm-none-linux-gnueabi-
		ARCH=arm
		TARGETENV=arm
		TOOLCHAIN_PATH=/projects/hnd/tools/linux/hndtools-armeabi-linux-2009q3/bin
		UTIL_BUILD=wl
		;;


	wl64)
		CROSS_COMPILE=
		ARCH=x86
		ARCH_SFX=64
		TARGETENV=linux
		CUSTOM_GCC=4.8.1
		UTIL_BUILD=wl
		;;

	dhd64)
		CROSS_COMPILE=
		ARCH=x86
		DHD_EXE=dhd64
		TARGETENV=linux
		CUSTOM_GCC=4.8.1
		UTIL_BUILD=dhd
		;;


	"")
		# Skip for 
		;;
	
	*)
		echo "========================";
		echo " Not supported platform for $PLATFORM";
		echo " Please check again"
		echo "========================";
		exit 2;
esac

##############################
# build_brand by user command
##############################
if [ ! "$PRIVATE_BUILD_BRAND" = "" ]; then
	BUILD_BRAND=$PRIVATE_BUILD_BRAND
fi

# user toolchain
if [ ! "$PRIVATE_TOOLCHAIN_PATH" = "" ]; then
	TOOLCHAIN_PATH=$PRIVATE_TOOLCHAIN_PATH
fi

if [ ! -d $TOOLCHAIN_PATH ]; then
	echo "toolchain path($TOOLCHAIN_PATH) is not exist!! please check it";
	exit;
fi

# Builtin SDK path
if [ "$BUILTIN" == "y" ] && [ "$LINUXDIR" != "" ];then
	BUILTIN_SDK=$LINUXDIR
fi
#######################
# Setup Make arguments
#######################
MAKE_ARGS+=" CROSS_COMPILE=$CROSS_COMPILE"
MAKE_ARGS+=" ARCH=$ARCH"
MAKE_ARGS+=" PATH=$PATH:$TOOLCHAIN_PATH"
MAKE_ARGS+=" OEM_ANDROID=1"

# GCC specific version
if [ ! "$CUSTOM_GCC" = "" ]; then
	MAKE_ARGS+=" GCCVER=$CUSTOM_GCC "
fi

########################
# Get Latest SDK info
########################
LINUX_DIR_PREFIX="/tools/linux/src/linux-"
function isnum
{
	local var=$1
	for ((idx=0;idx<${#var};idx++))
	do
		case ${var:$idx:1} in
			[0-9]*) continue;;
			*) return 0;;
		esac
	done
	return 1
}
function comp
{
	local old=$1
	local new=$2
	for ((idx=0;idx<${#old};idx++))
	do
		if [ "${old:$idx:1}" -lt "${new:$idx:1}" ] ; then
#			echo $old ${old:$idx:1}
			return 0
		fi
	done
	echo $old $new
	return 1
}
if [ "$USE_LATEST" != "" ] && [ "$LINUXDIR" == "" ]
then
list=`ls /tools/linux/src | grep $LINUXVER`
cdate=
for sdk in $list
do
#	echo $sdk
	tdate=${sdk##*-}
	isnum $tdate
	if [ "$?" == 0 ] ; then
		continue
	fi
	if [ "$cdate" == "" ] ; then
		cdate=$tdate;
		continue
	fi
	comp $cdate $tdate
	if [ "$?" == 0 ] ; then
		cdate=$tdate;
		continue
	fi
done
LINUXVER=${LINUXVER}-$cdate
echo "Autodetected latest SDK for $PLATFORM is $LINUXVER"
fi


if [ "$FLAT_TYPE" = "y" ];then
	# for FLAT_TYPE (after mogrified source)
	if [ ! $LINUXDIR = "" ];then
		MAKE_ARGS+=" KDIR=$LINUXDIR"
	else
		if [ ! $USE_LATEST == "" ]; then
			MAKE_ARGS+=" KDIR=/tools/linux/src/linux-$LINUXVER"
		else
			echo $LATEST_DATE
			MAKE_ARGS+=" KDIR=/tools/linux/src/linux-$LINUXVER"
		fi
	fi
	BUILD_BRAND=""
else
	# for directory type (pure svn checkout source)
	if [ ! $LINUXDIR = "" ];then
		MAKE_ARGS+=" LINUXDIR=$LINUXDIR"
	else
		MAKE_ARGS+=" LINUXVER=$LINUXVER"
	fi
fi

MAKE_ARGS+=" -j$CPU_JOBS"

#####################
# Build All platform
#####################
if [ "$BUILD_ALL" = "y" ];then
	echo "=========================================="
	echo "===== Build All platform at one time ====="
	echo "=========================================="
	mkdir -p build_all
	if [ "$BUILD_LIST" = "" ];then
		BUILD_LIST="JA JF M0 D2 T0"
	fi

	for platform in $BUILD_LIST
	do
		MAKE_SH_CMD="${0#./}"
		RUN_CLEAN=""
		RUN_CLEAN+="$MAKE_SH_CMD "
		if [ "$FLAT_TYPE" = "y" ];then
			RUN_CLEAN+=" -f"
		fi
		RUN_CLEAN+=" clean "
		RUN_MAKE=""
		RUN_MAKE+=" $MAKE_SH_CMD"
		if [ "$FLAT_TYPE" = "y" ];then
			RUN_MAKE+=" -f"
		fi
		RUN_MAKE+=" -p $platform "

		$RUN_CLEAN
		$RUN_MAKE
		if [ $? != "0" ];then
			exit 2
		fi

		# Builtin exception
		if [[ $platform == *BUILTIN ]]; then
			mkdir -p build_all/${platform}

			# SPRAT model exception
			if [[ $platform == SPRAT* ]]; then
			    mv BUILTIN_OUTPUT/sprat-KERNEL.tar build_all/${platform}
			else
			    mv BUILTIN_OUTPUT/boot.img.tar.md5 build_all/${platform}
			fi
		else
			mv dhd.ko build_all/dhd.ko.${platform}
		fi

	done
	exit $?;
fi

####################
# Build Information
####################
echo "====================================================="
if [ "$UTIL_BUILD" != "" ] ; then
	echo "			WL/DHD utility build"
	echo "PLATFORM       : $PLATFORM"
	echo "UTIL_BUILD     : $UTIL_BUILD"
	echo "ARCH           : $ARCH"
	echo "CROSS_COMPILE  : $CROSS_COMPILE"
	echo "TOOLCHAIN_PATH : $TOOLCHAIN_PATH"
else
	echo "			dhd.ko build"
	echo "PLATFORM       : $PLATFORM"
	echo "BUILD_BRAND    : $BUILD_BRAND"
	if [ ! $LINUXDIR = "" ];then
		echo "LINUXDIR       : $LINUXDIR"
	else
		echo "LINUXVER       : $LINUXVER"
	fi
	echo
	echo "ARCH           : $ARCH"
	echo "CROSS_COMPILE  : $CROSS_COMPILE"
	echo "TOOLCHAIN_PATH : $TOOLCHAIN_PATH"
	echo "MAKE_PATH      : $MAKE_PATH"
	echo "PATH           : $PATH:$TOOLCHAIN_PATH"
fi
echo "====================================================="
echo
#echo "====================================================="
#echo "$ make $MAKE_PATH $MAKE_ARGS $BUILD_BRAND"
#echo "====================================================="
#echo ""


###########################
# epivers.h check & create
##########################
if [ "$FLAT_TYPE" == "" ] && [ "$BUILTIN" == "" ] ;then
	if [ ! -e src/include/epivers.h ];then
		echo "-----------------------------------"
		echo "epivers.h create for version header"
		cd src/include && make && cd ../../
		echo "-----------------------------------"
	fi
fi

####################################
# check WIFI_BUILD_ONLY in Makefile. if not exist then add three line at top of Makefile
####################################
if [ "$FLAT_TYPE" = "y" ];then
	grep "WIFI_BUILD_ONLY" Makefile > /dev/null
	if [ $? -ne 0 ];then
		mv Makefile Makefile.original
		echo "# Three line added by Make.sh, original Makefile backup to Makefile.original
ifeq (\$(WIFI_BUILD_ONLY),y)
	include \$(KDIR)/.config
endif
" > Makefile
		cat Makefile.original >> Makefile
		sed -i 's/#EXTRA_LDFLAGS += --strip-debug/EXTRA_LDFLAGS += --strip-debug/' Makefile
	fi
fi


####################
# Call Sub Makefile
####################
#echo $MAKE_ARGS
if [ "$UTIL_BUILD" != "" ] ; then

	echo "== Make clean before build =="
	echo make -C src/$UTIL_BUILD/exe clean TARGETARCH=$ARCH
	make -C src/$UTIL_BUILD/exe clean TARGETARCH=$ARCH

	echo "== Build $UTIL_BUILD util =="
	# GCC specific version
	if [ "$ARCH" = "x86" ]; then
		if [ ! "$CUSTOM_GCC" = "" ]; then
			echo make -C src/$UTIL_BUILD/exe -j$CPU_JOBS TARGETOS=unix TARGETARCH=${ARCH} TARGETENV=${TARGETENV} GCCVER=${CUSTOM_GCC} ARCH_SFX=${ARCH_SFX} DHD_EXE=${DHD_EXE}
			make -C src/$UTIL_BUILD/exe -j$CPU_JOBS TARGETOS=unix TARGETARCH=${ARCH} TARGETENV=${TARGETENV} GCCVER=${CUSTOM_GCC} ARCH_SFX=${ARCH_SFX} DHD_EXE=${DHD_EXE}
		else
			echo make -C src/$UTIL_BUILD/exe -j$CPU_JOBS TARGETOS=unix TARGETARCH=${ARCH} TARGETENV=${TARGETENV} ARCH_SFX=${ARCH_SFX} DHD_EXE=${DHD_EXE}
			make -C src/$UTIL_BUILD/exe -j$CPU_JOBS TARGETOS=unix TARGETARCH=${ARCH} TARGETENV=${TARGETENV} ARCH_SFX=${ARCH_SFX} DHD_EXE=${DHD_EXE}
		fi
	else
		echo make -C src/$UTIL_BUILD/exe -j$CPU_JOBS TARGETOS=unix TARGETARCH=${ARCH} TARGETENV=${TARGETENV} TARGET_PREFIX=$TOOLCHAIN_PATH/$CROSS_COMPILE 
		make -C src/$UTIL_BUILD/exe -j$CPU_JOBS TARGETOS=unix TARGETARCH=${ARCH} TARGETENV=${TARGETENV} TARGET_PREFIX=$TOOLCHAIN_PATH/$CROSS_COMPILE 
	fi


elif [ "$BUILTIN" == "y" ] ;then

#	echo $ Make.py --sdk_path=$BUILTIN_SDK --CROSS_COMPILE=$CROSS_COMPILE --model=$BUILTIN_MODEL --target_dir=$BUILTIN_TARGET_DIR
#	Make.py --sdk_path=$BUILTIN_SDK --CROSS_COMPILE=$CROSS_COMPILE --model=$BUILTIN_MODEL --target_dir=$BUILTIN_TARGET_DIR
	echo $ /home/broadcom/40_youngho/Make.py --sdk_path=$BUILTIN_SDK --CROSS_COMPILE=$CROSS_COMPILE --model=$BUILTIN_MODEL --target_dir=$BUILTIN_TARGET_DIR
	/home/broadcom/40_youngho/Make.py --sdk_path=$BUILTIN_SDK --CROSS_COMPILE=$CROSS_COMPILE --model=$BUILTIN_MODEL --target_dir=$BUILTIN_TARGET_DIR

else
	echo "$ make $MAKE_PATH $MAKE_ARGS $BUILD_BRAND --no-print-directory QUIET_BUILD=1 WIFI_BUILD_ONLY=y"
	eval $PRE_CMD
	make $MAKE_PATH $MAKE_ARGS $BUILD_BRAND --no-print-directory QUIET_BUILD=1 WIFI_BUILD_ONLY=y

	# return exit code of make result to upper script
	if [ "$?" != 0 ]; then 
		MAKE_RESULTS=2
	else
		MAKE_RESULTS=0
	fi
	eval $POST_CMD
	exit $MAKE_RESULTS
fi
