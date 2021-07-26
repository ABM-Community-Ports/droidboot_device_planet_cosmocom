#pragma once

#include <platform/mt_typedefs.h>
#include <pal_typedefs.h>

ssize_t partition_read_emmc_ufs(const char *part_name, off_t offset, u8* data, size_t size);
ssize_t partition_write_emmc_ufs(const char *part_name, off_t offset, u8* data, size_t size);
int32_t partition_erase_emmc_ufs(const char *part_name);
