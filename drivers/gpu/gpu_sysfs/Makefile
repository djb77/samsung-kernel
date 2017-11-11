obj-m := gpu_sysfs.o


# Common files.
gpu_sysfs-y :=	gpu_sysfs_main.o \
                gpu_sysfs_util.o


# Platform specific files.

# ==================================
# Start ----- Mali T8xx device -----
ifeq ($(CONFIG_MALI_T8XX),y)

# --- For EXYNOS8890(MaliT880) Device. 
ifeq ($(CONFIG_SOC_EXYNOS8890),y)
gpu_sysfs-y +=	gpu_sysfs_target_exynos8890.o 
# Verify: target specific file is defined.
GPU_SYSFS_TARGET_FILE_DEFINED=1
endif

endif
# End ----- Mali T8xx device -------
# ==================================


# ==================================
# Start ----- Mali T7xx device -----
ifeq ($(CONFIG_MALI_T7XX),y)

# --- For EXYNOS7420(MaliT760) Device. 
ifeq ($(CONFIG_SOC_EXYNOS7420),y)
gpu_sysfs-y +=	gpu_sysfs_target_exynos7420.o 
# Verify: target specific file is defined.
GPU_SYSFS_TARGET_FILE_DEFINED=1
endif

# --- For EXYNOS5433(MaliT760) Device. 
ifeq ($(CONFIG_SOC_EXYNOS5433),y)
gpu_sysfs-y +=	gpu_sysfs_target_exynos5433.o 
# Verify: target specific file is defined.
GPU_SYSFS_TARGET_FILE_DEFINED=1
endif

endif
# End ----- Mali T7xx device -------
# ==================================


# ==================================
# Start ---- ARM Midgard-WK04 device
ifeq ($(CONFIG_MALI_MIDGARD_WK04),y)

# --- For EXYNOS5430(MaliT628) Device. 
ifeq ($(CONFIG_SOC_EXYNOS5430),y)
gpu_sysfs-y +=	gpu_sysfs_target_exynos5430.o 
# Verify: target specific file is defined.
GPU_SYSFS_TARGET_FILE_DEFINED=1
endif

endif
# End ------ ARM Midgard-WK04 device
# ==================================


# ==================================
# Start ---- ARM Midgard device ----
ifeq ($(CONFIG_MALI_MIDGARD),y)

# --- For EXYNOS5422(MaliT628) Device. 
ifeq ($(CONFIG_SOC_EXYNOS5422),y)
gpu_sysfs-y +=	gpu_sysfs_target_exynos5422.o 
# Verify: target specific file is defined.
GPU_SYSFS_TARGET_FILE_DEFINED=1
endif

endif
# End ---- ARM Midgard device ------
# ==================================


# ==================================
# Generate error if the TARGET 
# specific file is not defined OR 
# picked using the PLATFORM configs.
# ---- Temporary Patch for removing WARNING LOG by steeze.kim(Graphics) ----
#ifndef GPU_SYSFS_TARGET_FILE_DEFINED
#$(warning **** Make sure target file is defined. \
#               Check the TARGET and GPU configs in Makefile and Kernel Config. \
#               Module won't work without target file. ****)
#endif
# ---- Temporary Patch for removing WARNING LOG by steeze.kim(Graphics) ----


clean:
	rm -f *.o .*.cmd modules.order Module.symvers gpu_sysfs.ko gpu_sysfs.mod.c
	rm -rf .tmp_versions