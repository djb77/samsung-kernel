#
# Integrity TEE Driver
#
TEEC_CLIENT_PATH := multibuild/source/gp-api/client
PROTOCOL_PATH := multibuild/source/gp-api/protocol

EXTRA_AFLAGS += -I"$(srctree)"

obj-$(CONFIG_FIVE_TEE_DRIVER) += itee_driver.o

itee_driver-objs += five_tee_driver.o \
                    $(TEEC_CLIENT_PATH)/tee_client_api.o \
                    $(PROTOCOL_PATH)/teec_param_utils.o \
                    $(TEEC_CLIENT_PATH)/teec_common_qsee.o

ccflags-$(CONFIG_FIVE_TEE_DRIVER) += -Isecurity/integrity/five/ \
                                -Idrivers/five/five_tee_driver/$(PROTOCOL_PATH) \
                                -Idrivers/five/five_tee_driver/$(TEEC_CLIENT_PATH) \
                                -Idrivers/five/five_tee_driver/multibuild/include/gp-api \
                                -Idrivers/misc
