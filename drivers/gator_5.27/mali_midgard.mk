# Defines for Mali-Midgard driver
EXTRA_CFLAGS += -DMALI_USE_UMP=1 \
                -DMALI_LICENSE_IS_GPL=1 \
                -DMALI_BASE_TRACK_MEMLEAK=0 \
                -DMALI_DEBUG=0 \
                -DMALI_ERROR_INJECT_ON=0 \
                -DMALI_CUSTOMER_RELEASE=1 \
                -DMALI_UNIT_TEST=0 \
                -DMALI_BACKEND_KERNEL=1 \
                -DMALI_NO_MALI=0

DDK_DIR ?= .
  CONFIG_GATOR_MALI_MIDGARD_PATH = drivers/gpu/arm/tHEx/r7p0
  EXTRA_CFLAGS += -DMALI_SIMPLE_API=1 \
                  -DMALI_DIR_MIDGARD=1 \
                  -I$(DDK_DIR)/drivers/gpu/arm/tHEx/r7p0
    KBASE_DIR = $(DDK_DIR)/drivers/gpu/arm/tHEx/r7p0
    OSK_DIR = $(DDK_DIR)/drivers/gpu/arm/tHEx/r7p0/osk


  UMP_DIR = $(DDK_DIR)/include/linux

  # Include directories in the DDK
  EXTRA_CFLAGS += -I$(KBASE_DIR)/ \
                  -I$(KBASE_DIR)/.. \
                  -I$(KBASE_DIR)/backend/gpu \
                  -I$(OSK_DIR)/.. \
                  -I$(UMP_DIR)/.. \
                  -I$(DDK_DIR)/include \
                  -I$(KBASE_DIR)/osk/src/linux/include \
                  -I$(KBASE_DIR)/platform_dummy \
                  -I$(KBASE_DIR)/src \
                  -Idrivers/staging/android \
