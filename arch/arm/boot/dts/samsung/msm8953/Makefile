# For 32-bit system
dtb-$(CONFIG_SEC_MSM895332LTE_CHN) += msm8953-sec-msm8953lte_chn-r00.dtb \
				msm8953-sec-msm8953lte_chn-r01.dtb \
				msm8953-sec-msm8953lte_chn-r02.dtb \
				msm8953-sec-msm8953lte_chn-r03.dtb 

# For 64-bit system
dtb-$(CONFIG_SEC_MSM8953LTE_CHN) += msm8953-sec-msm8953lte_chn-r00.dtb \
				msm8953-sec-msm8953lte_chn-r01.dtb \
				msm8953-sec-msm8953lte_chn-r02.dtb \
				msm8953-sec-msm8953lte_chn-r03.dtb

# For C7LTE_CHN
ifeq ($(CONFIG_SEC_C7LTE_CHN_LDU),y)
dtb-$(CONFIG_SEC_C7LTE_CHN_LDU) += msm8953-sec-c7lte_chn_ldu-r00.dtb \
				msm8953-sec-c7lte_chn_ldu-r01.dtb \
				msm8953-sec-c7lte_chn_ldu-r02.dtb \
				msm8953-sec-c7lte_chn_ldu-r03.dtb \
				msm8953-sec-c7lte_chn_ldu-r04.dtb
else
dtb-$(CONFIG_SEC_C7LTE_CHN) += msm8953-sec-c7lte_chn-r00.dtb \
				msm8953-sec-c7lte_chn-r01.dtb \
				msm8953-sec-c7lte_chn-r02.dtb \
				msm8953-sec-c7lte_chn-r03.dtb \
				msm8953-sec-c7lte_chn-r04.dtb
endif
# For C7LTE_CHN_HK
dtb-$(CONFIG_SEC_C7LTE_CHN_HK) += msm8953-sec-c7lte_chn_hk-r02.dtb \
				msm8953-sec-c7lte_chn_hk-r03.dtb \
				msm8953-sec-c7lte_chn_hk-r04.dtb
				
# For C7LTE_KOR_SKT
dtb-$(CONFIG_SEC_C7LTE_KOR_SKT) += msm8953-sec-c7lte_kor_skt-r00.dtb

# For ON7XLTE_CHN
dtb-$(CONFIG_SEC_ON7XLTE_CHN) += msm8953-sec-on7xlte_chn-r00.dtb \
				msm8953-sec-on7xlte_chn-r01.dtb \
				msm8953-sec-on7xlte_chn-r02.dtb \
				msm8953-sec-on7xlte_chn-r03.dtb \
				msm8953-sec-on7xlte_chn-r04.dtb \
				msm8953-sec-on7xlte_chn-r05.dtb
				
# For GTAXLQLTE_USA_SPR
dtb-$(CONFIG_SEC_GTAXLQLTE_USA_SPR) += msm8953-sec-gtaxlqlte_usa_spr-r00.dtb \
					msm8953-sec-gtaxlqlte_usa_spr-r01.dtb \
					msm8953-sec-gtaxlqlte_usa_spr-r02.dtb
					
# For J7POPLTE_USA_VZW
dtb-$(CONFIG_SEC_J7POPLTE_USA_VZW) += msm8953-sec-j7poplte_usa_vzw-r00.dtb \
					msm8953-sec-j7poplte_usa_vzw-r01.dtb \
					msm8953-sec-j7poplte_usa_vzw-r02.dtb \
					msm8953-sec-j7poplte_usa_vzw-r03.dtb \
					msm8953-sec-j7poplte_usa_vzw-r04.dtb \
					msm8953-sec-j7poplte_usa_vzw-r05.dtb

# For J7POPLTE_USA_SPR
dtb-$(CONFIG_SEC_J7POPLTE_USA_SPR) += msm8953-sec-j7poplte_usa_spr-r00.dtb \
					msm8953-sec-j7poplte_usa_spr-r01.dtb \
					msm8953-sec-j7poplte_usa_spr-r02.dtb \
					msm8953-sec-j7poplte_usa_spr-r03.dtb \
					msm8953-sec-j7poplte_usa_spr-r04.dtb \
					msm8953-sec-j7poplte_usa_spr-r05.dtb

# For J7POPLTE_USA_USC
dtb-$(CONFIG_SEC_J7POPLTE_USA_USC) += msm8953-sec-j7poplte_usa_usc-r00.dtb \
					msm8953-sec-j7poplte_usa_usc-r01.dtb \
					msm8953-sec-j7poplte_usa_usc-r02.dtb \
					msm8953-sec-j7poplte_usa_usc-r03.dtb \
					msm8953-sec-j7poplte_usa_usc-r04.dtb \
					msm8953-sec-j7poplte_usa_usc-r05.dtb

ifeq ($(CONFIG_ARM64),y)
always          := $(dtb-y)
subdir-y        := $(dts-dirs)
else
targets += dtbs
targets += $(addprefix ../, $(dtb-y))

$(obj)/../../%.dtb: $(src)/%.dts FORCE
        $(call if_changed_dep,dtc)

dtbs: $(addprefix $(obj)/../../,$(dtb-y))
endif
clean-files := *.dtb *.reserve.dts
