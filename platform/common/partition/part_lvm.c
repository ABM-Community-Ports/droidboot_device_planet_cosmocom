#ifdef MTK_GPT_SCHEME_SUPPORT
#include <partition.h>
#else
#include <mt_partition.h>
#endif
#include <ctype.h>
#include <err.h>
#include <pal_log.h>
#include <part_interface.h>
#include <part_lvm.h>
#include <stdlib.h>
#include <storage_api.h>
#include <string.h>

const char *lvm_label_one = "LABELONE";
const char *lvm2_type = "LVM2 001";
const char *lvm_meta_fmtt_magic = " LVM2 x[5A%r0N*>";
const char *lvm_id_start = "id = \"";
const char *lvm_lv_prefix = "LVM-LV§";
const uint lvm_uuid_len = 38;
const int lvm_label_size = 0x200;
const int lvm_mda_header_size = 0x200;
const size_t lvm_disk_sector_size = 0x200;
const int lvm_meta_magic_len = 16;
const uint lvm_fmtt_version = 1;
const uint lvm_wrap_around_extra = 0x1000;
int lvm_root_index = -1;

//malloced items - we only support one single vg on one partition
char *lvm_vg_name = NULL;
char *lvm_vg_uuid = NULL;
struct disk_vg *lvm_disk_vg = NULL;

uint64_t get_value(char **p, const char *string) {
	*p = strstr(*p, string);
	if (!*p)
		return 0;
	*p += strlen(string);
	return strtoul(*p, p, 10);
}

bool check_value(char **ptr, char *string, char *tmpl) {
	int tmpllen = strlen(tmpl);
	if (string != NULL) {
		*ptr = strstr(*ptr, string);
	}
	if (!*ptr)
		return 0;
	if (string != NULL) {
		*ptr += strlen(string);
	}
	if (**ptr != '"')
		return 0;
	return (memcmp(*ptr + 1, tmpl, tmpllen) == 0 && (*ptr)[tmpllen + 1] == '"');
}

bool check_label(char *data, const char *label_value, int size) {
	return (strncmp(data, label_value, size) == 0);
}

int check_flag(char *p, const char *string, char *flag) {
	int len_string = strlen(string), len_flag = strlen(flag);
	while (1) {
		char *q;
		p = strstr(p, string);
		if (!p)
			return 0;
		p += len_string;
		if (memcmp(p, " = [", sizeof(" = [") - 1) != 0)
			continue;
		q = p + sizeof(" = [") - 1;
		while (1) {
			while (isspace(*q))
				q++;
			if (*q != '"')
				return 0;
			q++;
			if (memcmp(q, flag, len_flag) == 0 && q[len_flag] == '"')
				return 1;
			while (*q != '"')
				q++;
			q++;
			if (*q == ']')
				return 0;
			q++;
		}
	}
}

char *read_to_within_and_alloc_with_prefix(char **ptr, const char to, const char *within_end, const char *prefix) {
	char *ret;
	int len;
	int prefix_len=0;
	char *c = *ptr;

	if (prefix != NULL) {
		prefix_len =  strlen(prefix);;
	}

	while (*c != to && c < within_end) {
		c++;
	}

	len = c - *ptr;
	ret = malloc(len + prefix_len + 1);
	if (prefix != NULL) {
		memcpy(ret, prefix, prefix_len);
	}
	memcpy(ret+prefix_len, *ptr, len);
	ret[len+prefix_len] = '\0';

	*ptr += len + 1;

	return ret;
}

char *read_from_into_length_within(char *ptr, const char *from, char *into, const int len, const char *within_end) {
	ptr = strstr(ptr, from);
	if (ptr == NULL) {
		return ptr;
	}
	ptr += strlen(from);
	if (ptr + len > within_end) {
		return NULL;
	}
	memcpy(into, ptr, len);
	into[len] = '\0';
	return ptr;
}

char *read_skip_within(char *ptr, const char *to_skip, const char *within_end) {
	ptr = strstr(ptr, to_skip);
	if (ptr == NULL) {
		return ptr;
	}
	ptr += strlen(to_skip);
	if (ptr > within_end) {
		return NULL;
	}
	return ptr;
}

bool is_white_space(char c) {
	return (c == '\n' || c == '\r' || c == ' ' || c == '\t');
}

char *read_skip_white_space_within(char *ptr, const char *within_end) {
	while (is_white_space(*ptr) && ptr < within_end) {
		ptr++;
	}
	return ptr;
}

ssize_t read_from_partition(int index, uint64_t offset, uint32_t size, char *data) {
    int64_t part_addr = partition_get_offset(index);

	if (part_addr < 0) {
		return ERR_NOT_FOUND;
	}

	u64 read_addr = (u64)part_addr + offset;
	pal_log_info("%s storage_read addr: %llu, size: %d\n", __func__, read_addr, size);

	ssize_t len = storage_read(partition_get_region(index), read_addr, (uint8_t *) data, size);

	if (len != (ssize_t) size) {
		return ERR_NOT_FOUND;
	}
	return len;
}

void free_lv(struct disk_lv *lv) {
	if (lv->segments) {
		if (lv->segments->stripes) {
			free(lv->segments->stripes);
		}
		free(lv->segments);
	}
	free(lv->name);
	free(lv->uuid);
	free(lv);
}

int lvm_detect() {
	int p_index, err, index_to;
	uint index_from;
	uint64_t base_offset = 0x200;
	char buf[lvm_label_size];
	char *meta_ptr, *meta_ptr_end;
	char vg_uuid[lvm_uuid_len + 1];
	char pv_uuid[lvm_uuid_len + 1];
	char *meta_data;
	struct lvm_label_header *lh = (struct lvm_label_header *) buf;
	struct lvm_pv_header *pvh;
	struct lvm_disk_location *disk_location;
	struct lvm_mda_header *mda_header;
	struct lvm_raw_location *raw_location;
	struct disk_pv *pv;

	pal_log_info("%s Checking for LVM partitions\n", __func__);

	for (p_index = 38; p_index < 45; p_index++) {
		err = read_from_partition(p_index, base_offset, sizeof(buf), buf);
		if (err == ERR_NOT_FOUND) {
			pal_log_warn("%s partition not readable\n", __func__);
			break;
		}

		if (check_label(lh->id, lvm_label_one, sizeof(lh->id)) &&
			check_label(lh->type, lvm2_type, sizeof(lh->type))) {
			lvm_root_index = p_index;
			break;
		}
	}

	if (p_index == 45) {
		pal_log_warn("%s no LVM2 partitions found\n", __func__);
		return ERR_NOT_FOUND;
	}

	if (lh->offset_xl >= lvm_label_size - sizeof(struct lvm_pv_header) -
								 2 * sizeof(struct lvm_disk_location)) {
		pal_log_warn("%s LVM PV offset out of range\n", __func__);
		return ERR_NOT_ENOUGH_BUFFER;
	}

	pvh = (struct lvm_pv_header *) (buf + lh->offset_xl);

	for (index_from = 0, index_to = 0; index_from < 32; index_from++) {
		pv_uuid[index_to] = pvh->pv_uuid[index_from];
		index_to++;
		if ((index_from != 1) && (index_from != 29) && (index_from % 4 == 1)) {
			pv_uuid[index_to] = '-';
			index_to++;
		}
	}
	pv_uuid[index_to] = '\0';
	pal_log_info("%s LVM ID: %s\n", __func__, pv_uuid);

	disk_location = pvh->disk_areas_xl;

	disk_location++;
	if (disk_location->offset > 0) {
		pal_log_warn("%s we don't support multiple data areas\n", __func__);
		return ERR_NOT_ALLOWED;
	}
	disk_location++;
	meta_data = malloc(disk_location->size + lvm_wrap_around_extra);
	if (!meta_data) {
		pal_log_warn("%s out of memory\n", __func__);
		return ERR_NO_MEMORY;
	}
	err = read_from_partition(p_index, disk_location->offset, disk_location->size, meta_data);
	if (err == -1) {
		pal_log_warn("%s partition metadata not readable\n", __func__);
		free(meta_data);
		return ERR_NOT_FOUND;
	}

	mda_header = (struct lvm_mda_header *) meta_data;
	if (strncmp(mda_header->magic, lvm_meta_fmtt_magic, lvm_meta_magic_len) || mda_header->version != lvm_fmtt_version) {
		pal_log_warn("%s LVM2 metadata magic or version don't match\n", __func__);
		free(meta_data);
		return ERR_NOT_SUPPORTED;
	}

	raw_location = mda_header->raw_locations;
	if (raw_location->offset > disk_location->size) {
		pal_log_warn("%s metadata offset beyond metadata range\n", __func__);
		free(meta_data);
		return ERR_NOT_ENOUGH_BUFFER;
	}

	if (raw_location->offset + raw_location->size > mda_header->size) {
		uint64_t wrapped_size = raw_location->offset + raw_location->size - disk_location->size;
		if (wrapped_size > lvm_wrap_around_extra) {
			pal_log_warn("%s metadata wrapping too large (%lld) for allowed extra (%d)\n", __func__, wrapped_size, lvm_wrap_around_extra);
			free(meta_data);
			return ERR_NOT_ENOUGH_BUFFER;
		}
		memcpy(meta_data + disk_location->size,
			   meta_data + lvm_mda_header_size,
			   wrapped_size);
	}

	meta_ptr = meta_data + raw_location->offset;
	meta_ptr_end = meta_data + raw_location->offset + raw_location->size;
	lvm_vg_name = read_to_within_and_alloc_with_prefix(&meta_ptr, ' ', meta_ptr_end, NULL);
	pal_log_info("%s vg name:%s\n", __func__, lvm_vg_name);

	meta_ptr = read_from_into_length_within(meta_ptr, lvm_id_start, vg_uuid, lvm_uuid_len, meta_ptr_end);
	pal_log_info("%s vg id:%s\n", __func__, vg_uuid);

	if (lvm_disk_vg == NULL) {
		lvm_disk_vg = malloc(sizeof(struct disk_vg));
		lvm_disk_vg->name = lvm_vg_name;
		lvm_disk_vg->uuid = lvm_vg_uuid = malloc(lvm_uuid_len + 1);
		memcpy(lvm_vg_uuid, vg_uuid, lvm_uuid_len);
		lvm_vg_uuid[lvm_uuid_len + 1] = '\0';
		lvm_disk_vg->uuid_len = lvm_uuid_len;
		lvm_disk_vg->extent_size = get_value(&meta_ptr, "extent_size = ");
		pal_log_info("%s vg extent_size:%lld\n", __func__, lvm_disk_vg->extent_size);
		if (meta_ptr == NULL) {
			pal_log_warn("%s missing extent_size\n", __func__);
			free(meta_data);
			return ERR_NOT_ENOUGH_BUFFER;
		}
		lvm_disk_vg->pvs = NULL;
		lvm_disk_vg->lvs = NULL;
		meta_ptr = read_skip_within(meta_ptr, "physical_volumes {", meta_ptr_end);
		if (meta_ptr == NULL) {
			pal_log_warn("%s missing physical_volumes\n", __func__);
			free(meta_data);
			return ERR_NOT_ENOUGH_BUFFER;
		}
		//read first pv's
		meta_ptr = read_skip_white_space_within(meta_ptr, meta_ptr_end);
		if (meta_ptr == NULL) {
			pal_log_warn("%s parse error: pv whitespace\n", __func__);
			free(meta_data);
			return ERR_NOT_ENOUGH_BUFFER;
		}
		pv = malloc(sizeof(*pv));
		memset(pv, 0, sizeof(*pv));

		pv->name = read_to_within_and_alloc_with_prefix(&meta_ptr, ' ', meta_ptr_end, NULL);
		pal_log_info("%s pv name:%s\n", __func__, pv->name);

		pv->uuid = malloc(lvm_uuid_len + 1);
		pv->uuid_len = lvm_uuid_len;
		meta_ptr = read_from_into_length_within(meta_ptr, lvm_id_start, pv->uuid, lvm_uuid_len, meta_ptr_end);
		pal_log_info("%s pv uuid:%s\n", __func__, pv->uuid);
		if (meta_ptr == NULL) {
			pal_log_warn("%s parse error: pv id\n", __func__);
			free(pv->name);
			free(pv->uuid);
			free(pv);
			free(meta_data);
			return ERR_NOT_ENOUGH_BUFFER;
		}
		pv->start_sector = get_value(&meta_ptr, "pe_start = ");
		pal_log_info("%s pv start_sector:%llu\n", __func__, pv->start_sector);
		if (meta_ptr == NULL) {
			pal_log_warn("%s parse error: pe_start\n", __func__);
			free(pv->name);
			free(pv->uuid);
			free(pv);
			free(meta_data);
			return ERR_NOT_ENOUGH_BUFFER;
		}
		lvm_disk_vg->pvs = pv;
	}
	meta_ptr = read_skip_within(meta_ptr, "logical_volumes {", meta_ptr_end);
	if (meta_ptr == NULL) {
		pal_log_warn("%s missing logical_volumes\n", __func__);
		free(meta_data);
		return ERR_NOT_ENOUGH_BUFFER;
	}
	while (1) {
		//read next lvs
		meta_ptr = read_skip_white_space_within(meta_ptr, meta_ptr_end);
		if (meta_ptr == NULL) {
			pal_log_warn("%s parse error: lv whitespace\n", __func__);
			free(meta_data);
			return ERR_NOT_ENOUGH_BUFFER;
		}
		if (*meta_ptr == '}')//end of LV's list found
			break;

		struct disk_lv *lv = malloc(sizeof(*lv));
		memset(lv, 0, sizeof(*lv));

		lv->name = read_to_within_and_alloc_with_prefix(&meta_ptr, ' ', meta_ptr_end, lvm_lv_prefix);
		pal_log_info("%s lv name:%s\n", __func__, lv->name);

		lv->uuid = malloc(lvm_uuid_len + 1);
		lv->uuid_len = lvm_uuid_len;
		meta_ptr = read_from_into_length_within(meta_ptr, lvm_id_start, lv->uuid, lvm_uuid_len, meta_ptr_end);
		pal_log_info("%s lv uuid:%s\n", __func__, lv->uuid);
		if (meta_ptr == NULL) {
			pal_log_warn("%s parse error: lv id\n", __func__);
			free_lv(lv);
			free(meta_data);
			return ERR_NOT_ENOUGH_BUFFER;
		}

		lv->visible = check_flag(meta_ptr, "status", "VISIBLE");
		// only relevant for mirror type - is_pvmove = check_flag(metaptr, "status", "PVMOVE");

		lv->segment_count = get_value(&meta_ptr, "segment_count = ");
		pal_log_info("%s lv segment_count: %d\n", __func__, lv->segment_count);
		if (meta_ptr == NULL) {
			pal_log_warn("%s parse error: lv segment_count\n", __func__);
			free_lv(lv);
			free(meta_data);
			return ERR_NOT_ENOUGH_BUFFER;
		}

		struct disk_segment *seg = lv->segments = calloc(lv->segment_count, sizeof(*seg));
		for (index_from = 0; index_from < lv->segment_count; index_from++) {
			meta_ptr = read_skip_within(meta_ptr, "segment", meta_ptr_end);
			if (meta_ptr == NULL) {
				pal_log_warn("%s missing lv segment\n", __func__);
				free_lv(lv);
				free(meta_data);
				return ERR_NOT_ENOUGH_BUFFER;
			}

			seg->start_extent = get_value(&meta_ptr, "start_extent = ");
			pal_log_info("%s lv seg start_extent: %llu\n", __func__, seg->start_extent);
			if (meta_ptr == NULL) {
				pal_log_warn("%s missing lv segment start_extent\n", __func__);
				free_lv(lv);
				free(meta_data);
				return ERR_NOT_ENOUGH_BUFFER;
			}

			seg->extent_count = get_value(&meta_ptr, "extent_count = ");
			pal_log_info("%s lv seg extent_count: %lld\n", __func__, seg->extent_count);
			if (meta_ptr == NULL) {
				pal_log_warn("%s missing lv segment extent_count\n", __func__);
				free_lv(lv);
				free(meta_data);
				return ERR_NOT_ENOUGH_BUFFER;
			}

			if (!check_value(&meta_ptr, "type = ", "striped")) {
				pal_log_warn("%s missing or unsupported seg type (we only support striped)\n", __func__);
				free_lv(lv);
				free(meta_data);
				return ERR_NOT_ENOUGH_BUFFER;
			}

			seg->type = DISK_STRIPED;
			seg->stripe_count = get_value(&meta_ptr, "stripe_count = ");
			pal_log_info("%s lv seg stripe_count: %d\n", __func__, seg->stripe_count);
			if (meta_ptr == NULL) {
				pal_log_info("%s missing lv segment stripe_count\n", __func__);
				free_lv(lv);
				free(meta_data);
				return ERR_NOT_ENOUGH_BUFFER;
			}

			if (seg->stripe_count != 1) {
				seg->stripe_size = get_value(&meta_ptr, "stripe_size = ");
				pal_log_info("%s lv seg stripe_size: %d\n", __func__, seg->stripe_size);
				pal_log_info("%s error we only support single striped LVM\n", __func__);
				free_lv(lv);
				free(meta_data);
				return ERR_NOT_ALLOWED;
			}

			struct disk_stripe *stripe = seg->stripes = calloc(seg->stripe_count, sizeof(*stripe));
			meta_ptr = read_skip_within(meta_ptr, "stripes = [", meta_ptr_end);
			if (meta_ptr == NULL) {
				pal_log_info("%s missing lv segment stripes\n", __func__);
				free_lv(lv);
				free(meta_data);
				return ERR_NOT_ENOUGH_BUFFER;
			}

			meta_ptr = read_skip_white_space_within(meta_ptr, meta_ptr_end);
			if (lvm_disk_vg->pvs == NULL || !check_value(&meta_ptr, NULL, lvm_disk_vg->pvs->name)) {
				pal_log_info("%s missing lv segment stripes pv name not valid\n", __func__);
				free_lv(lv);
				free(meta_data);
				return ERR_NOT_ENOUGH_BUFFER;
			}
			stripe->name = lvm_disk_vg->pvs->name;
			stripe->start = get_value(&meta_ptr, ",");
			pal_log_info("%s lv segment stripes pvname: %s, start: %lld\n", __func__, stripe->name, stripe->start);

			meta_ptr = read_skip_within(meta_ptr, "]", meta_ptr_end);// end of stripes

			seg++;
		}
		meta_ptr = read_skip_within(meta_ptr, "}", meta_ptr_end);//end of segments
		meta_ptr = read_skip_within(meta_ptr, "}", meta_ptr_end);//end of lv

		lv->next = lvm_disk_vg->lvs;
		lvm_disk_vg->lvs = lv;
	}
	free(meta_data);

	return NO_ERROR;
}

bool lv_is_bootable(const struct disk_lv *lv) {
	int prefix_len = sizeof(lvm_lv_prefix);
	char *lvname= lv->name + prefix_len;
	return lv->name && lv->visible &&
		   (memcmp(lvname, "boot_", strlen("boot_")) == 0 ||
			memcmp(lvname, "BOOT_", strlen("BOOT_")) == 0 ||
			memcmp(lvname, "boot-", strlen("boot-")) == 0 ||
			memcmp(lvname, "BOOT-", strlen("BOOT-")) == 0);
}

int count_of_lvm_bootable_lvs() {
	int count = 0;

	if (lvm_disk_vg && lvm_disk_vg->lvs) {
		struct disk_lv *lv = lvm_disk_vg->lvs;
		do {
			if (lv_is_bootable(lv)) {
				count++;
			}
			lv = lv->next;
		} while (lv);
	}

	return count;
}

char *lvm_boot_name_for_index(int index) {
	int count = 0;

	if (lvm_disk_vg && lvm_disk_vg->lvs) {
		struct disk_lv *lv = lvm_disk_vg->lvs;
		do {
			if (lv_is_bootable(lv)) {
				if (count == index) {
					return lv->name;
				}
				count++;
			}
			lv = lv->next;
		} while (lv);
	}
	return NULL;
}

void free_disk_vg() {
	if (lvm_disk_vg) {
		if (lvm_disk_vg->pvs) {
			free(lvm_disk_vg->pvs);
			lvm_disk_vg->pvs = NULL;
		}
		struct disk_lv *lvs = lvm_disk_vg->lvs;
		while (lvs != NULL) {
			lvm_disk_vg->lvs = lvm_disk_vg->lvs->next;
			free_lv(lvs);
			lvs = lvm_disk_vg->lvs;
		}
		free(lvm_disk_vg);
		lvm_disk_vg = NULL;
	}
}

void set_disk_vg(struct disk_vg *disk_vg) {
	lvm_disk_vg = disk_vg;
}

int get_lvm_root_index() {
	return lvm_root_index;
}

struct disk_lv *get_lv_for_lv_name(const char *lv_name) {
	struct disk_lv *lv = lvm_disk_vg->lvs;
	do {
		if (lv->name && memcmp(lv->name, lv_name, strlen(lv_name)) == 0) {
			break;
		}
		lv = lv->next;
	} while (lv);

	return lv;
}

bool lv_exists_with_lv_name(const char *lv_name) {
	return (get_lv_for_lv_name(lv_name) != NULL);
}

ssize_t partition_read_lvm(const char *part_name, off_t offset, u8 *data, size_t size) {
	ssize_t did_read_total = 0;
	uint64_t extent_size = lvm_disk_vg->extent_size * lvm_disk_sector_size;
	uint64_t start_sector = 0;

	pal_log_info("%s lvname: %s, offset: %lld, size: %zu, lvm_root_index: %d\n", __func__, part_name, offset, size, lvm_root_index);
	if (lvm_disk_vg) {
		if (lvm_disk_vg->pvs) {
			start_sector = lvm_disk_vg->pvs->start_sector;
		}
		if (lvm_disk_vg->lvs) {
			struct disk_lv *lv = get_lv_for_lv_name(part_name);

			if (lv) {
				while (size) {
					uint64_t target_extent = offset / extent_size;
					struct disk_segment *segment = lv->segments;
					uint64_t bytes_to_read_from_segment = 0;
					size_t did_read = 0;
					uint i;

					for (i = 0; i < lv->segment_count; i++) {
						if (segment->start_extent <= target_extent &&
							segment->start_extent + segment->extent_count > target_extent) {
							break;
						}
						segment++;
					}
					if (i == lv->segment_count) {
						pal_log_info("%s lv target_extent not within segments, size: %d\n", __func__, size);
						return 0;
					}
					bytes_to_read_from_segment = ((segment->start_extent + segment->extent_count) * extent_size) - offset;
					if (bytes_to_read_from_segment > size) {
						bytes_to_read_from_segment = size;
					}

					did_read = read_from_partition(lvm_root_index,
												   (start_sector * lvm_disk_sector_size) +
													   (segment->stripes->start * extent_size) +
													   offset - (segment->start_extent * extent_size),
                                                   (uint32_t)bytes_to_read_from_segment,
												   (char*) data);
					if (did_read != bytes_to_read_from_segment) {
						pal_log_info("%s lv failed to read desired data from segment, did_read: %d\n", __func__, did_read);
						return did_read;
					}

					size -= did_read;
					offset += did_read;
					data += did_read;
					did_read_total += did_read;
				}
			}
		}
	}
	pal_log_info("%s did_read_total: %ld\n", __func__, did_read_total);
	return did_read_total;
}