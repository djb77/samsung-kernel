ifeq ($(CONFIG_OF),y)

ifeq ($(CONFIG_SEC_LTE_PROJECT),y)
        ifeq ($(CONFIG_MACH_LTE_EUR_OPEN),y)
                dtb-y += msm8937-sec-lte-eur-r00.dtb
        endif
endif

ifeq ($(CONFIG_SEC_A7MVELTE_PROJECT),y)
        ifeq ($(CONFIG_MACH_A7MVELTE_CHN_OPEN),y)
                dtb-y += msm8937-sec-a7mvelte-chn-r00.dtb
        endif
endif

echo '*** Warning: Samsung $@ Device Tree MSM8937.'
DTB_NAMES := $(subst $\",,$(CONFIG_BUILD_ARM_APPENDED_DTB_IMAGE_NAMES))
ifneq ($(DTB_NAMES),)
DTB_LIST := $(addsuffix .dtb,$(DTB_NAMES))
else
DTB_LIST := $(dtb-y)
endif

targets += dtbs
targets += $(addprefix ../, $(DTB_LIST))
endif

$(obj)/../../%.dtb: $(src)/%.dts FORCE
	$(call if_changed_dep,dtc)

dtbs: $(addprefix $(obj)/../../,$(DTB_LIST))

clean-files := *.dtb *.reverse.dts


