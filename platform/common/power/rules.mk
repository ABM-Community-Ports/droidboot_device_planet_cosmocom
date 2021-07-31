LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += -I$(LOCAL_DIR)
INCLUDES += -I$(LOCAL_DIR)/../include
INCLUDES += -I$(LOCAL_DIR)/../../$(PLATFORM)/include

OBJS += \
	$(LOCAL_DIR)/mtk_battery.o  \
	$(LOCAL_DIR)/mtk_charger.o  \
	$(LOCAL_DIR)/mtk_charger_intf.o \
	$(LOCAL_DIR)/../../$(PLATFORM)/mt_gauge.o

ifeq ($(MTK_CHARGER_NEW_ARCH),yes)
	DEFINES += MTK_CHARGER_NEW_ARCH
endif

ifeq ($(MTK_MT6370_PMU_CHARGER_SUPPORT),yes)
	OBJS += $(LOCAL_DIR)/mt6370_pmu_charger.o
	DEFINES += MTK_MT6370_PMU_CHARGER_SUPPORT
	DEFINES += SWCHR_POWER_PATH
endif

ifeq ($(MTK_MT6370_PMU_BLED_SUPPORT),yes)
	OBJS += $(LOCAL_DIR)/mt6370_pmu_bled.o
	DEFINES += MTK_MT6370_PMU_BLED_SUPPORT
endif

ifeq ($(MTK_MT6360_PMU_CHARGER_SUPPORT),yes)
	OBJS += $(LOCAL_DIR)/mt6360_pmu_charger.o
	DEFINES += MTK_MT6360_PMU_CHARGER_SUPPORT
	DEFINES += SWCHR_POWER_PATH
endif

ifeq ($(MTK_BQ25601_SUPPORT),yes)
	OBJS += $(LOCAL_DIR)/bq25601.o
	DEFINES += MTK_BQ25601_SUPPORT
	DEFINES += SWCHR_POWER_PATH
endif
