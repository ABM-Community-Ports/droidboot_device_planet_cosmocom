#pragma once

#include <platform/mt_typedefs.h>
#include <pal_typedefs.h>

struct lvm_label_header {
	char id[8];			/* LABELONE */
	uint64_t sector_xl; /* Sector number of this label */
	uint32_t crc_xl;	/* From next field to end of sector */
	uint32_t offset_xl;	/* Offset from start of struct to contents */
	char type[8];		/* LVM2 001 */
};

struct lvm_disk_location {
	uint64_t offset;	/* Offset in bytes to start sector */
	uint64_t size;		/* Bytes */
};

struct lvm_pv_header {
	int8_t pv_uuid[32];

	/* This size can be overridden if PV belongs to a VG */
	uint64_t device_size_xl; /* Bytes */

	/* NULL-terminated list of data areas followed by */
	/* NULL-terminated list of metadata area headers */
	struct lvm_disk_location disk_areas_xl[0]; /* Two lists */
};

struct disk_vg {
	char *uuid;
	size_t uuid_len;

	char *name;
	uint64_t extent_size;
	struct disk_pv *pvs;
	struct disk_lv *lvs;
	struct disk_vg *next;
};

struct disk_pv {
	char *uuid;
	size_t uuid_len;

	char *name;
	uint64_t part_start;
	uint64_t part_size;
	uint64_t start_sector; /* Sector number where the data area starts. */
	struct disk_pv *next;
};

struct disk_lv {
	char *uuid;
	size_t uuid_len;

	char *name;
	unsigned int segment_count;
	int visible;

	/* Pointer to segment_count segments. */
	struct disk_segment *segments;
	struct disk_vg *vg;
	struct disk_lv *next;
};

struct disk_segment {
	uint64_t start_extent;
	uint64_t extent_count;
	enum {
		DISK_STRIPED = 0,
		DISK_MIRROR = 1,
		DISK_RAID4 = 4,
		DISK_RAID5 = 5,
		DISK_RAID6 = 6,
		DISK_RAID10 = 10,
	} type;

	unsigned int stripe_count;
	struct disk_stripe *stripes;

	unsigned int stripe_size;
};

struct disk_stripe {
	uint64_t start;
	char *name;
};

struct lvm_raw_location {
	uint64_t offset; /* Offset in bytes to start sector */
	uint64_t size;	 /* Bytes */
	uint32_t checksum;
	uint32_t filler;
};

struct lvm_mda_header {
	uint32_t checksum_xl; /* Checksum of rest of mda header */
	char magic[16];	  /* To aid scans for metadata */
	uint32_t version;
	uint64_t start; /* Absolute start byte of mda header */
	uint64_t size;	/* Size of metadata area */

	struct lvm_raw_location raw_locations[0]; /* NULL-terminated list */
};

extern const char *lvm_lv_prefix;

int lvm_detect();
int get_lvm_root_index();
int count_of_lvm_bootable_lvs();
char *lvm_boot_name_for_index(int index);

bool lv_exists_with_lv_name(const char *lv_name);
ssize_t partition_read_lvm(const char *part_name, off_t offset, u8 *data, size_t size);