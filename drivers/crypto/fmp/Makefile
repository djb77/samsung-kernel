# Exynos FMP makefile
obj-$(CONFIG_FIPS_FMP) += first_file.o
obj-$(CONFIG_FMP_UFS) += fmp_ufs.o
obj-$(CONFIG_FIPS_FMP) += fmpdev.o fmp_integrity.o fmplib.o
obj-$(CONFIG_FIPS_FMP) += hmac_fmp.o sha256_fmp.o
obj-$(CONFIG_FIPS_FMP) += fmp_ufs_fips.o
ifneq (,$(filter $(CONFIG_FMP_ECRYPT_FS) $(CONFIG_FMP_EXT4CRYPT_FS),y))
obj-y += fmp_derive_iv.o
endif
obj-$(CONFIG_FIPS_FMP) += last_file.o

obj-$(CONFIG_FMP_MMC) += fmp_mmc.o

ccflags-$(CONFIG_FMP_UFS) += -Idrivers/scsi/ufs
ccflags-$(CONFIG_FMP_MMC) += -Idrivers/mmc/host
