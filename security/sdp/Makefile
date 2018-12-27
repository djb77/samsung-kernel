#
# Makefile for DEK and DLP modules
#

ifndef CONFIG_NO_SDP_BASE
obj-y += dek.o dek_aes.o sdp_mm.o pub_crypto_emul.o dek_sysfs.o cache_cleanup.o kek_pack.o fs_handler.o
endif
obj-$(CONFIG_DLP) += sdp_dlp.o
