#
# Integrity TEE Driver
#
TEEC_CLIENT_PATH := multibuild/source/gp-api/client
PROTOCOL_PATH := multibuild/source/gp-api/protocol

obj-$(CONFIG_FIVE_TEE_DRIVER) += itee_driver.o
itee_driver-objs += five_tee_driver.o \
                    $(TEEC_CLIENT_PATH)/teec_common_tbase.o \
                    $(TEEC_CLIENT_PATH)/tee_client_api.o \
                    $(PROTOCOL_PATH)/teec_param_utils.o
ccflags-$(CONFIG_FIVE_TEE_DRIVER) += -Isecurity/integrity/five \
                                -Idrivers/misc/five_tee_driver/$(PROTOCOL_PATH) \
                                -Idrivers/misc/five_tee_driver/$(TEEC_CLIENT_PATH) \
                                -Idrivers/misc/five_tee_driver/multibuild/include/gp-api

ifeq ($(CONFIG_TRUSTONIC_TEE), y)
  ifeq ($(CONFIG_SOC_EXYNOS9810), y)
    PLATFORM := exynos9810
    ccflags-$(CONFIG_FIVE_TEE_DRIVER) += -Idrivers/gud/gud-$(PLATFORM)/MobiCoreDriver
    ccflags-$(CONFIG_FIVE_TEE_DRIVER) += -Idrivers/gud/gud-$(PLATFORM)/MobiCoreDriver/public
  else
    $(error "Unknown SoC")
  endif
endif
