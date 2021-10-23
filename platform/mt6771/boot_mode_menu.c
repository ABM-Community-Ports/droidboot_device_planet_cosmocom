
/*This file implements MTK boot mode.*/

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <debug.h>
#include <err.h>
#include <reg.h>
#include <env.h>
#include <video.h>
#include <platform/mt_typedefs.h>
#include <platform/boot_mode.h>
#include <platform/mt_reg_base.h>
#include <platform/mtk_key.h>
#include <platform/mt_gpt.h>
#include <target/cust_key.h>
#include <platform/mtk_wdt.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <part_interface.h>
#include <part_lvm.h>

extern  int unshield_recovery_detection(void);
extern BOOL recovery_check_command_trigger(void);
extern void boot_mode_menu_select();
extern void mtk_wdt_disable(void);
#define MODULE_NAME "[BOOT_MENU]"
extern bool boot_ftrace;
bool g_boot_menu = false;
#define LAST_BOOT_SELECT "last_boot_select"

#define AW9523_I2C_ID  I2C4
#define AW9523_SLAVE_ADDR_WRITE   0xb6
#define AW9523_SLAVE_ADDR_READ    0xb7

#define P0_NUM_MAX   8
#define P1_NUM_MAX   7

#define P0_KROW_MASK 0xff
#define P1_KCOL_MASK 0x7f

#define KROW_P0_0 0
#define KROW_P0_1 1
#define KROW_P0_2 2
#define KROW_P0_3 3
#define KROW_P0_4 4
#define KROW_P0_5 5
#define KROW_P0_6 6
#define KROW_P0_7 7

#define KROW_P1_0 0
#define KROW_P1_1 1
#define KROW_P1_2 2
#define KROW_P1_3 3
#define KROW_P1_4 4
#define KROW_P1_5 5
#define KROW_P1_6 6
#define KROW_P1_7 7

#define P0_INPUT     0x00 //Read for port input state
#define P1_INPUT     0x01 //Read for port input state
#define P0_OUTPUT    0x02 //R/W port output state
#define P1_OUTPUT    0x03 //R/W port output state
#define P0_CONFIG    0x04 //R/W port direction configuration
#define P1_CONFIG    0x05 //R/W port direction configuration
#define P0_INT       0x06 //R/W port interrupt enable
#define P1_INT       0x07 //R/W port interrupt enable
#define SW_RSTN      0x7F //Soft reset

//KEY_UP,    KROW_P0_5, KROW_P1_4
//KEY_DOWN,  KROW_P0_5, KROW_P1_2
//KEY_ENTER, KROW_P0_6, KROW_P1_5
//KEY_1,     KROW_P0_0, KROW_P1_0
//KEY_2,     KROW_P0_0, KROW_P1_1
//KEY_3,     KROW_P0_0, KROW_P1_2
//KEY_4,     KROW_P0_0, KROW_P1_3
//KEY_5,     KROW_P0_0, KROW_P1_4
//KEY_6,     KROW_P0_0, KROW_P1_5
//KEY_LEFT,  KROW_P0_5, KROW_P1_1
//KEY_RIGHT, KROW_P0_5, KROW_P1_5

kal_uint32 aw9523_write_byte(kal_uint8 addr, kal_uint8 reg_data)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint8 write_data[2];
	kal_uint16 len;
	struct mt_i2c_t aw9523_i2c;

	write_data[0] = addr;
	write_data[1] = reg_data;

	aw9523_i2c.id = AW9523_I2C_ID;
	/* Since i2c will left shift 1 bit, we need to set I2C address to >>1 */
	aw9523_i2c.addr = (AW9523_SLAVE_ADDR_WRITE >> 1);
	aw9523_i2c.mode = ST_MODE;
	aw9523_i2c.speed = 100;
	len = 2;

	ret_code = i2c_write(&aw9523_i2c, write_data, len);

	if (I2C_OK != ret_code)
		dprintf(INFO, "%s: i2c_write: ret_code: %d\n", __func__, ret_code);

	return ret_code;
}

kal_uint32 aw9523_read_byte(kal_uint8 addr, kal_uint8 *returnData)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint16 len;
	struct mt_i2c_t aw9523_i2c;
	*returnData = addr;

	aw9523_i2c.id = AW9523_I2C_ID;
	/* Since i2c will left shift 1 bit, we need to set I2C address to >>1 */
	aw9523_i2c.addr = (AW9523_SLAVE_ADDR_READ >> 1);
	aw9523_i2c.mode = ST_MODE;
	aw9523_i2c.speed = 100;
	len = 1;

	ret_code = i2c_write_read(&aw9523_i2c, returnData, len, len);

	if (I2C_OK != ret_code)
		dprintf(INFO, "%s: i2c_read: i2c_write_read: %d\n", __func__, ret_code);

	return ret_code;
}

bool aw9523_key_pressed(unsigned char *keyst, kal_uint8 row, kal_uint8 col)
{
	return !(keyst[col] & (1<<row));
}

void aw9523_hw_reset(void) {
	mt_set_gpio_mode(GPIO175 | 0x80000000, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO175 | 0x80000000, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO175 | 0x80000000, GPIO_OUT_ZERO);
	mdelay(5);
	mt_set_gpio_out(GPIO175 | 0x80000000, GPIO_OUT_ONE);
	mdelay(1);
}

void aw9523_init_keycfg(unsigned char *keyst)
{
	aw9523_write_byte(SW_RSTN, 0x00); // Software Reset

	aw9523_write_byte(P0_CONFIG, 0xFF); // P0: Input Mode
	aw9523_write_byte(P1_CONFIG, 0x00); // P1: Output Mode
	aw9523_write_byte(P1_OUTPUT, 0x00); // P1: 0000 0000

	aw9523_write_byte(P0_INT, 0xFF); // P0: Disable Interrupt
	aw9523_write_byte(P1_INT, 0xFF); // P1: Disable Interrupt

	memset(keyst, P0_KROW_MASK, P1_NUM_MAX); // set all keys not pressed
}

void aw9523_hw_off(void)
{
	aw9523_write_byte(SW_RSTN, 0x00); // Software Reset
	mt_set_gpio_out(GPIO175 | 0x80000000, GPIO_OUT_ZERO);
}

void hall_init() {
	mt_set_gpio_mode(GPIO8 | 0x80000000, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO8 | 0x80000000, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(GPIO8 | 0x80000000, GPIO_PULL_DISABLE);
}

bool hall_state_open() {
	return mt_get_gpio_in(GPIO8 | 0x80000000);
}

static bool skip_boot_option(const char* boot) {
	return (strncmp("EMPTY",boot,strlen("EMPTY")) == 0);
}

static int print_boot_option(int index, int count, const char* boot, int select, int last_boot_select)
{
	if (!skip_boot_option(boot)) {
		video_printf("[%d] %s boot", count, boot);
		if (last_boot_select == index)
			video_printf(" *");
		if (select == index)
			video_printf(" <-----                        \n");
		else
			video_printf("                               \n");
		count++;
	}
	return count;
}

void boot_mode_menu_select()
{
	int select = 0;
	int last_boot_select = 0;
	int kbd_select = 0;
	int count = 1;
	int refreshOptions = 1;
	const char *title_msg = "Select Boot Mode\nUse arrow keys or numbers\nPress Enter to select\n\n";
	int timeout = 8000;
	int autoBootInterrupted = 0;
	int boot_selected = 0;
	int lvm_boot_lvs_count = 0;
	bool update_boot_selection_default = false;
	static unsigned char keyst[P1_NUM_MAX];

	char *boot2;
	char *boot3;
	char *boot4;

	char lbs_buf[2];

	if (lvm_detect() == NO_ERROR) {
		lvm_boot_lvs_count = count_of_lvm_bootable_lvs();
	}

	last_boot_select = (get_env(LAST_BOOT_SELECT) == NULL) ? 0 : atoi(get_env(LAST_BOOT_SELECT));
	if (last_boot_select >= 0 && last_boot_select < (5+lvm_boot_lvs_count)) {
		select = last_boot_select;
	}

	aw9523_hw_reset();
	aw9523_init_keycfg(keyst);
	hall_init();

	partition_get_name(partition_get_bootable(PartBoot2Num), &boot2);
	partition_get_name(partition_get_bootable(PartBoot3Num), &boot3);
	partition_get_name(partition_get_bootable(PartBoot4num), &boot4);

#ifdef MACH_FPGA_NO_DISPLAY
	g_boot_mode = RECOVERY_BOOT;
	return ;
#endif
	video_clean_screen();
	int vol_up_pressed = 0;
	int vol_down_pressed = 0;
	while (boot_selected == 0) {

		if (hall_state_open()) {
			for (int i = 0; i < P1_NUM_MAX; i++) {
				if (P1_KCOL_MASK & (1 << i)) {
					kal_uint8 val = 0;
					aw9523_read_byte(P1_CONFIG, &val);
					aw9523_write_byte(P1_CONFIG, (P1_KCOL_MASK | val) & (~(1 << i))); //set p1_x port output mode
					aw9523_read_byte(P1_OUTPUT, &val);
					aw9523_write_byte(P1_OUTPUT, (P1_KCOL_MASK | val) & (~(1 << i)));

					aw9523_read_byte(P0_INPUT, &val); // read p0 port status

					keyst[i] = (val & P0_KROW_MASK);
				}
			}
		}

		if (mtk_detect_key(MT65XX_MENU_SELECT_KEY) || aw9523_key_pressed(keyst, KROW_P0_5, KROW_P1_4)) {
			if (vol_down_pressed == 0) {
				select--;
				if ((select == 4) && skip_boot_option(boot4))
					select--;
				if ((select == 3) && skip_boot_option(boot3))
					select--;
				if ((select == 2) && skip_boot_option(boot2))
					select--;
				if (select < 0)
					select = 5+lvm_boot_lvs_count;
				refreshOptions = 1;
				autoBootInterrupted = 1;
				vol_down_pressed = 1;
			}
		}
		else
			vol_down_pressed = 0;
		if (!mt_get_gpio_in(GPIO11 | 0x80000000) || aw9523_key_pressed(keyst, KROW_P0_5, KROW_P1_2)) {
			if (vol_up_pressed == 0) {
				select++;
				if ((select == 2) && skip_boot_option(boot2))
					select++;
				if ((select == 3) && skip_boot_option(boot3))
					select++;
				if ((select == 4) && skip_boot_option(boot4))
					select++;
				if (select == 6+lvm_boot_lvs_count)
					select = 0;
				refreshOptions = 1;
				autoBootInterrupted = 1;
				vol_up_pressed = 1;
			}
		}
		else
			vol_up_pressed = 0;

		if (mtk_detect_key(8) || aw9523_key_pressed(keyst, KROW_P0_6, KROW_P1_5))
			boot_selected = 1;

		kbd_select = -1;
		if (aw9523_key_pressed(keyst, KROW_P0_0, KROW_P1_0)) { //1
			kbd_select = 0;
			boot_selected = 1;
		}
		if (aw9523_key_pressed(keyst, KROW_P0_0, KROW_P1_1)) { //2
			kbd_select = 1;
			boot_selected = 1;
		}
		if (aw9523_key_pressed(keyst, KROW_P0_0, KROW_P1_2)) { //3
			kbd_select = 2;
			boot_selected = 1;
		}
		if (aw9523_key_pressed(keyst, KROW_P0_0, KROW_P1_3)) { //4
			kbd_select = 3;
			boot_selected = 1;
		}
		if (aw9523_key_pressed(keyst, KROW_P0_0, KROW_P1_4)) {
			kbd_select = 4;
			boot_selected = 1;
		}
		if (aw9523_key_pressed(keyst, KROW_P0_0, KROW_P1_5)) {
			kbd_select = 5;
			boot_selected = 1;
		}
		if (aw9523_key_pressed(keyst, KROW_P0_0, KROW_P1_6)) {
			kbd_select = 6;
			boot_selected = 1;
		}
		if (aw9523_key_pressed(keyst, KROW_P0_6, KROW_P1_0)) {
			kbd_select = 7;
			boot_selected = 1;
		}
		if (aw9523_key_pressed(keyst, KROW_P0_6, KROW_P1_1)) {
			kbd_select = 8;
			boot_selected = 1;
		}
		if (aw9523_key_pressed(keyst, KROW_P0_5, KROW_P1_1)) {
			refreshOptions = 1;
			update_boot_selection_default = false;
		}
		if (aw9523_key_pressed(keyst, KROW_P0_5, KROW_P1_5)) {
			refreshOptions = 1;
			update_boot_selection_default = true;
		}

		if (kbd_select >= 0)
		{
			select = kbd_select;
			if (select >= 2 && skip_boot_option(boot2))
				select++;
			if (select >= 3 && skip_boot_option(boot3))
				select++;
			if (select >= 4 && skip_boot_option(boot4))
				select++;
		}

		if (refreshOptions && !boot_selected) {
			refreshOptions = 0;
			g_boot_menu = true;
			video_set_cursor(video_get_rows() / 2 - 30, 0);
			video_printf(title_msg);

			count = 1;
			count = print_boot_option(0, count, "NORMAL", select, last_boot_select);
			count = print_boot_option(1, count, "RECOVERY", select, last_boot_select);
			count = print_boot_option(2, count, boot2, select, last_boot_select);
			count = print_boot_option(3, count, boot3, select, last_boot_select);
			count = print_boot_option(4, count, boot4, select, last_boot_select);
			count = print_boot_option(5, count, "FASTBOOT", select, last_boot_select);
			int prefix_len = strlen(lvm_lv_prefix);
			for (int lv_index=0; lv_index<lvm_boot_lvs_count; lv_index++) {
				const char *lvname = lvm_boot_name_for_index(lv_index) + prefix_len;
				count = print_boot_option(6+lv_index, count, lvname, select, last_boot_select);
			}

			if (!autoBootInterrupted)
				video_printf("\nAutomatic boot in %d seconds...\n", (timeout / 1000));
			else {
				if (!update_boot_selection_default)
					video_printf("\n* Default  < [Retain] |  Set  >\n");
				else
					video_printf("\n* Default  <  Retain  | [Set] >\n");
			}
		}

		mdelay(50);
		timeout -= 50;
		if (timeout % 1000 == 0 && !autoBootInterrupted)
			refreshOptions = 1;

		if (timeout < 0 && !autoBootInterrupted) {
			boot_selected = 1;
		}
	}

	aw9523_hw_off();

	if (update_boot_selection_default
		&& select != last_boot_select
		&& (select == 0 || select == 3 || select == 4)) {
		sprintf(lbs_buf, "%d", select);
		set_env(LAST_BOOT_SELECT, lbs_buf);
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
	} else if (select > 5) {
		g_boot_mode = NORMAL_BOOT;
		advancedBootMode = NORMAL_BOOT_LVM_BASE + (select - 6);
	}
	video_clean_screen();
	video_set_cursor(video_get_rows() / 2, 0);

	mtk_wdt_restart();
	return;
}

BOOL boot_menu_key_trigger(void)
{
#if 1
	//wait
	ulong begin = get_timer(0);
	dprintf(INFO, "\n%s Check  boot menu\n", MODULE_NAME);
	dprintf(INFO, "%s Wait 50ms for special keys\n", MODULE_NAME);

	//let some case of recovery mode pass.
	if (unshield_recovery_detection()) {
		return TRUE;
	}

	while (get_timer(begin) < 50) {
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
	return recovery_check_command_trigger() ? TRUE : FALSE;
}
