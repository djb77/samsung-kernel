ifeq ($(CONFIG_VIDEO_EXYNOS_ASTC), y)
	# only build this if both configs are defined
	ifeq ($(CONFIG_SOC_EXYNOS9810), y)
		obj-y += astc-exynos9810-workaround.o
	endif

	obj-y += astc-core.o astc-helper.o
endif

UBSAN_SANITIZE := y
KASAN_SANITIZE := y

# ccflags-y := -Wall -Wextra -Wtype-limits -Wstrict-overflow=5 -Wlogical-op -Wjump-misses-init -Wformat=2 -Werror=implicit-function-declaration -Werror=strict-prototypes -Werror=parentheses -Werror=undef -Wfloat-equal -Wconversion
#
# requires newer gcc
# ccflags-y := -Wnull-dereference -Wshift-overflow=2 -Wduplicated-branches
#
# To filter out warnings from headers, add the following lines at the beginning of the source files
# #pragma GCC diagnostic push
# #pragma GCC diagnostic ignored "-Wunused-parameter"
# #pragma GCC diagnostic ignored "-Wsign-conversion"
# #pragma GCC diagnostic ignored "-Wconversion"
# #pragma GCC diagnostic ignored "-Wsign-compare"
# #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
# #pragma GCC diagnostic ignored "-Wignored-qualifiers"
# <INSERT INCLUDES HERE>
# #pragma GCC diagnostic pop

#CFLAGS_astc-core.o := -Wpadded
#CFLAGS_astc-helper.o := -Wpadded
