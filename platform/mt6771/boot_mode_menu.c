
/*This file implements MTK boot mode.*/

#include <sys/types.h>
#include <debug.h>
#include <err.h>
#include <reg.h>
#include <video.h>
#include <platform/mt_typedefs.h>
#include <platform/boot_mode.h>
#include <platform/mt_reg_base.h>
#include <platform/mtk_key.h>
#include <platform/mt_gpt.h>
#include <target/cust_key.h>
#include <platform/mtk_wdt.h>
#include <platform/mt_gpio.h>

extern  int unshield_recovery_detection(void);
extern BOOL recovery_check_command_trigger(void);
extern void boot_mode_menu_select();
extern void mtk_wdt_disable(void);
#define MODULE_NAME "[BOOT_MENU]"
extern void cmdline_append(const char* append_string);
extern bool boot_ftrace;
bool g_boot_menu = false;

void boot_mode_menu_select()
{
	int select = 0;  // 0=recovery mode, 1=fastboot.  2=normal boot 3=normal boot + ftrace.4=kmemleak on
	int refreshOptions = 1;
	const char* title_msg = "Select Boot Mode\nPress VOL+/VOL- to move up/down\nPress ESC to select\n\n";
	int timeout = 8000;
	int autoBootInterrupted = 0;
	int boot_selected = 0;

  char *boot2;
	char *boot3;
	char *boot4;

	partition_get_name(PART_BOOT2_NUM, &boot2);
 	partition_get_name(PART_BOOT3_NUM, &boot3);
	partition_get_name(PART_BOOT4_NUM, &boot4);

#ifdef MACH_FPGA_NO_DISPLAY
	g_boot_mode = RECOVERY_BOOT;
	return ;
#endif
	video_clean_screen();
	int vol_up_pressed = 0;
	int vol_down_pressed = 0;
	while (boot_selected == 0) {

		if (mtk_detect_key(MT65XX_MENU_SELECT_KEY)) {
			if (vol_down_pressed == 0) {
			    select--;
			    if (select < 0)
					     select = 5;
			    refreshOptions = 1;
			    autoBootInterrupted = 1;
					vol_down_pressed = 1;
			}
		}
		else
			vol_down_pressed = 0;
		if (!mt_get_gpio_in(GPIO11 | 0x80000000)) {
			if (vol_up_pressed == 0) {
			     select++;
			     if (select == 6)
			 		    select = 0;
			     refreshOptions = 1;
			     autoBootInterrupted = 1;
					 vol_up_pressed = 1;
			}
		}
		else
			vol_up_pressed = 0;

		if (mtk_detect_key(8))
				boot_selected = 1;

		if (refreshOptions) {
		    refreshOptions = 0;
		    g_boot_menu = true;
				video_set_cursor(video_get_rows()/2-30, 0);
				video_printf(title_msg);

		    switch (select) {
		    case 0:
			video_printf("NORMAL boot <-----                       \n");
			video_printf("RECOVERY boot                            \n");
			video_printf("%s boot                                  \n", boot2);
			video_printf("%s boot                                  \n", boot3);
			video_printf("%s boot                                  \n", boot4);
			video_printf("FASTBOOT boot                            \n");
			if (!autoBootInterrupted)
    			    video_printf("Automatic boot in %d seconds...\n", (timeout / 1000));
			else
    			    video_printf("                                    \n");
			break;
		    case 1:
			video_printf("NORMAL boot                              \n");
			video_printf("RECOVERY boot <-----                     \n");
			video_printf("%s boot                                  \n", boot2);
			video_printf("%s boot                                  \n", boot3);
			video_printf("%s boot                                  \n", boot4);
			video_printf("FASTBOOT boot                            \n");
			if (!autoBootInterrupted)
    			    video_printf("Automatic boot in %d seconds...\n", (timeout / 1000));
			else
    			    video_printf("                                    \n");
			break;
		    case 2:
			video_printf("NORMAL boot                              \n");
			video_printf("RECOVERY boot                            \n");
			video_printf("%s boot <-----                           \n", boot2);
			video_printf("%s boot                                  \n", boot3);
			video_printf("%s boot                                  \n", boot4);
			video_printf("FASTBOOT boot                            \n");
			if (!autoBootInterrupted)
    			    video_printf("Automatic boot in %d seconds...\n", (timeout / 1000));
			else
    			    video_printf("                                    \n");
			break;
		    case 3:
			video_printf("NORMAL boot                              \n");
			video_printf("RECOVERY boot                            \n");
			video_printf("%s boot                                  \n", boot2);
			video_printf("%s boot <-----                           \n", boot3);
			video_printf("%s boot                                  \n", boot4);
			video_printf("FASTBOOT boot                            \n");
			if (!autoBootInterrupted)
    			    video_printf("Automatic boot in %d seconds...\n", (timeout / 1000));
			else
    			    video_printf("                                    \n");
			break;
		    case 4:
			video_printf("NORMAL boot                              \n");
			video_printf("RECOVERY boot                            \n");
			video_printf("%s boot                                  \n", boot2);
			video_printf("%s boot                                  \n", boot3);
			video_printf("%s boot <-----                           \n", boot4);
			video_printf("FASTBOOT boot                            \n");
			if (!autoBootInterrupted)
    			    video_printf("Automatic boot in %d seconds...\n", (timeout / 1000));
			else
    			    video_printf("                                    \n");
			break;
		    case 5:
			video_printf("NORMAL boot                              \n");
			video_printf("RECOVERY boot                            \n");
			video_printf("%s boot                                  \n", boot2);
			video_printf("%s boot                                  \n", boot3);
			video_printf("%s boot                                  \n", boot4);
			video_printf("FASTBOOT boot <-----                     \n");
			if (!autoBootInterrupted)
    			    video_printf("Automatic boot in %d seconds...\n", (timeout / 1000));
			else
    			    video_printf("                                    \n");
			break;
		    default:
			break;
		    }
		}

		mdelay(50);
		timeout -= 50;
		if (timeout % 1000 == 0 && !autoBootInterrupted)
			  refreshOptions = 1;

		if (timeout < 0 && !autoBootInterrupted) {
		    select = 0;
				boot_selected = 1;
		}
	}
	if (select == 0) {
		g_boot_mode = NORMAL_BOOT;
	} else if (select == 1) {
		g_boot_mode = RECOVERY_BOOT;
	} else if (select == 2) {
		g_boot_mode = RECOVERY_BOOT2;
	} else if (select == 3) {
		g_boot_mode = NORMAL_BOOT;
		advancedBootMode = NORMAL_BOOT3;
	} else if (select == 4) {
		g_boot_mode = NORMAL_BOOT;
		advancedBootMode = NORMAL_BOOT4;
	} else if (select == 5) {
  	g_boot_mode = FASTBOOT;
  } else {
		//pass
	}
        video_clean_screen();
	video_set_cursor(video_get_rows()/2, 0);
	return;
}

BOOL boot_menu_key_trigger(void)
{
#if 1
	//wait
	ulong begin = get_timer(0);
	dprintf(INFO, "\n%s Check  boot menu\n",MODULE_NAME);
	dprintf(INFO, "%s Wait 50ms for special keys\n",MODULE_NAME);

	//let some case of recovery mode pass.
	if (unshield_recovery_detection()) {
		return TRUE;
	}

	while (get_timer(begin)<50) {
		if (mtk_detect_key(MT65XX_BOOT_MENU_KEY)) {
			mtk_wdt_disable();
			boot_mode_menu_select();
			mtk_wdt_init();
			return TRUE;
		}
	}
#endif
	return FALSE;
}

BOOL boot_menu_detection(void)
{
	return boot_menu_key_trigger();
}


int unshield_recovery_detection(void)
{
	//because recovery_check_command_trigger's BOOL is different from the BOOL in this file.
	//so use code like this type.
	return recovery_check_command_trigger()? TRUE:FALSE;
}
