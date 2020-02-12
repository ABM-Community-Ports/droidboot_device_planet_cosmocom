#
LOCAL_DIR := $(GET_LOCAL_DIR)
TARGET := k50v1_64_bsp
MODULES += app/mt_boot \
           dev/lcm
PMIC_CHIP := mt6353
ifeq ($(findstring PMIC_CHIP, $(strip $(DEFINES))),)
DEFINES += PMIC_CHIP_$(shell echo $(PMIC_CHIP) | tr '[a-z]' '[A-Z]')
endif
MTK_EMMC_SUPPORT = yes
DEFINES += MTK_NEW_COMBO_EMMC_SUPPORT
MTK_KERNEL_POWER_OFF_CHARGING=yes
MTK_PUMP_EXPRESS_PLUS_SUPPORT := yes
MTK_BQ25896_SUPPORT := yes
MTK_LCM_PHYSICAL_ROTATION = 0
CUSTOM_LK_LCM="nt35695_fhd_dsi_cmd_truly_nt50358"
MTK_SECURITY_SW_SUPPORT = yes
MTK_VERIFIED_BOOT_SUPPORT = yes
MTK_SEC_FASTBOOT_UNLOCK_SUPPORT = yes
MTK_SECURITY_ANTI_ROLLBACK = no
BOOT_LOGO := fhd
DEBUG := 0
#DEFINES += WITH_DEBUG_DCC=1
DEFINES += WITH_DEBUG_UART=1
#DEFINES += WITH_DEBUG_FBCON=1
#DEFINES += MACH_FPGA=y
CUSTOM_LK_USB_UNIQUE_SERIAL=no
MTK_PROTOCOL1_RAT_CONFIG = C/Lf/Lt/W/T/G
MTK_GOOGLE_TRUSTY_SUPPORT = no
MTK_AB_OTA_UPDATER = no
MTK_DM_VERITY_OFF = no
SYSTEM_AS_ROOT = yes
