#include <string.h>
#include <part_emmc_ufs.h>
#include <part_lvm.h>
#include <pal_log.h>

ssize_t partition_read(const char *part_name, off_t offset, u8 *data, size_t size) {
	pal_log_info("%s part_name: %s, offset: %lld, size: %zu\n", __func__, part_name, offset, size);

	if (memcmp(part_name, lvm_lv_prefix, sizeof(lvm_lv_prefix)) == 0) {
		return partition_read_lvm(part_name, offset, data, size);
	} else {
		return partition_read_emmc_ufs(part_name, offset, data, size);
	}
}

ssize_t partition_write(const char *part_name, off_t offset, u8 *data, size_t size) {
	return partition_write_emmc_ufs(part_name, offset, data, size);
}

int32_t partition_erase(const char *part_name) {
	return partition_erase_emmc_ufs(part_name);
}