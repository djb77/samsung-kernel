# NXP audio amplifier - TFA9896

TFA_VERSION              =tfa9896
#TFADSP_32BITS           =0
#TFADSP_DSP_MSG_APR_PACKET_STRATEGY=0
TFADSP_DSP_BUFFER_POOL   =1
#TFA_ACTIVATED_ASYNCHRONOUSLY=0
TFA_USE_PSTREAM_ONLY     =1
#TFA_USE_DEVICE_SPECIFIC_CONTROL=0
#TFA_PROFILE_ON_DEVICE   =0
TFA_NO_SND_FORMAT_CHECK  =0
TFA_SRC_DIR              =sound/soc/codecs/$(TFA_VERSION)
TFA98XX_FLAGS           := -DUSE_TFA9896

# cc flags
#ccflags-y               := -DDEBUG
#ccflags-y               += -I$(TFA_SRC_DIR)/inc
ccflags-y               := -I$(TFA_SRC_DIR)/inc
ccflags-y               += -Werror
#ifdef TFADSP_32BITS
#ccflags-y               += -DTFADSP_32BITS
#endif
#ifdef TFADSP_DSP_MSG_APR_PACKET_STRATEGY
#ccflags-y               += -DTFADSP_DSP_MSG_APR_PACKET_STRATEGY
#endif
ifdef TFADSP_DSP_BUFFER_POOL
ccflags-y               += -DTFADSP_DSP_BUFFER_POOL
endif
ifdef TFA_ACTIVATED_ASYNCHRONOUSLY
ccflags-y               += -DTFA_ACTIVATED_ASYNCHRONOUSLY
endif
ifdef TFA_USE_PSTREAM_ONLY
ccflags-y               += -DTFA_USE_PSTREAM_ONLY
endif
ifdef TFA_USE_DEVICE_SPECIFIC_CONTROL
ccflags-y               += -DTFA_USE_DEVICE_SPECIFIC_CONTROL
endif
ifdef TFA_PROFILE_ON_DEVICE
ccflags-y               += -DTFA_PROFILE_ON_DEVICE
endif
ifdef TFA_NO_SND_FORMAT_CHECK
ccflags-y               += -DTFA_NO_SND_FORMAT_CHECK
endif

# objs
snd-soc-tfa98xx-objs    := tfa98xx.o
snd-soc-tfa98xx-objs    += tfa_container.o
snd-soc-tfa98xx-objs    += tfa_dsp.o
snd-soc-tfa98xx-objs    += tfa_init.o
snd-soc-tfa98xx-objs    += tfa_debug.o

# CFLAGS
CFLAGS_tfa98xx.o       += $(TFA98XX_FLAGS)
CFLAGS_tfa_container.o += $(TFA98XX_FLAGS)
CFLAGS_tfa_dsp.o       += $(TFA98XX_FLAGS)
CFLAGS_tfa_init.o      += $(TFA98XX_FLAGS)
CFLAGS_tfa_debug.o     += $(TFA98XX_FLAGS)

# built-in driver
obj-y	+= snd-soc-tfa98xx.o
