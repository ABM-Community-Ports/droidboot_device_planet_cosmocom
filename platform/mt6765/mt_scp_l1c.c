/*
* Copyright (C) 2015 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdint.h>
#include <debug.h>
#include <platform/mt_typedefs.h>
#include <platform/mt_scp.h>

l1c_status_t l1c_set_size(uint32_t l1c_type, l1c_size_t l1c_size)
{
	/* L1C size is invalid */
	if (l1c_size >= L1C_SIZE_MAX) {
		return L1C_STATUS_ERROR_L1C_SIZE;
	}
	/* Set L1C size */
	L1C_SEL(l1c_type)->L1C_CON &= ~L1C_CON_L1CSIZE_MASK;
	L1C_SEL(l1c_type)->L1C_CON |= (l1c_size << L1C_CON_L1CSIZE_OFFSET);

	/* When L1C size is 0KB, make sure the L1C is disabled */
	if (l1c_size == L1C_SIZE_0KB) {
		L1C_SEL(l1c_type)->L1C_CON = 0;
	}

	dprintf(CRITICAL, "L1C_CON, addr:0x%x, value:0x%x\n",
	       (unsigned int)&L1C_SEL(l1c_type)->L1C_CON,
	       L1C_SEL(l1c_type)->L1C_CON);

	return L1C_STATUS_OK;
}

l1c_status_t l1c_region_enable(uint32_t l1c_type, l1c_region_t region)
{
	/* Region is invalid */
	if (region >= L1C_REGION_MAX) {
		return L1C_STATUS_ERROR_REGION;
	}

	/* The region should be configured before region is enabled */
	if (L1C_SEL(l1c_type)->L1C_ENTRY_N[region] & L1C_ENTRY_N_C_MASK) {
		L1C_SEL(l1c_type)->L1C_REGION_EN |= (1 << region);
	} else {
		return L1C_STATUS_ERROR;
	}

	return L1C_STATUS_OK;
}

l1c_status_t l1c_region_config(uint32_t l1c_type, l1c_region_t region,
                               const l1c_region_config_t *region_config)
{
	/* Region is invalid */
	if (region >= L1C_REGION_MAX) {
		return L1C_STATUS_ERROR_REGION;
	}

	/* Parameter check */
	if (region_config == NULL) {
		return L1C_STATUS_INVALID_PARAMETER;
	}

	/* The region address must be 4KB aligned */
	if (region_config->l1c_region_address & (MTK_L1C_REGION_SIZE_UNIT - 1)) {
		//assert(0);
		return L1C_STATUS_ERROR_REGION_ADDRESS;
	}

	/* The region size must be 4KB aligned */
	if (region_config->l1c_region_size & (MTK_L1C_REGION_SIZE_UNIT - 1)) {
		//assert(0);
		return L1C_STATUS_ERROR_REGION_SIZE;
	}

	/* Write the region setting to corresponding register */
	L1C_SEL(l1c_type)->L1C_ENTRY_N[region] = region_config->l1c_region_address;
	L1C_SEL(l1c_type)->L1C_END_ENTRY_N[region] = region_config->l1c_region_address +
	        region_config->l1c_region_size;

	/* Set this bit when region is configured, and this bit will be double checked in l1c_region_enable() function */
	L1C_SEL(l1c_type)->L1C_ENTRY_N[region] |= L1C_ENTRY_N_C_MASK;
	/* Update the global variable */

	return L1C_STATUS_OK;
}

l1c_status_t l1c_enable(uint32_t l1c_type)
{
	/* Enable L1C */
	L1C_SEL(l1c_type)->L1C_CON |= L1C_CON_MCEN_MASK | L1C_CON_CNTEN0_MASK | L1C_CON_CNTEN1_MASK;

	return L1C_STATUS_OK;
}

l1c_status_t l1c_invalidate_all_l1c_lines(uint32_t l1c_type)
{
	/* Invalidate all L1C lines */
	L1C_SEL(l1c_type)->L1C_OP &= ~L1C_OP_OP_MASK;
	L1C_SEL(l1c_type)->L1C_OP |= ((L1C_INVALIDATE_ALL_LINES << L1C_OP_OP_OFFSET) | L1C_OP_EN_MASK);

	return L1C_STATUS_OK;
}

l1c_status_t l1c_flush_all_l1c_lines(uint32_t l1c_type)
{
	/* Flush all L1C lines */
	L1C_SEL(l1c_type)->L1C_OP &= ~L1C_OP_OP_MASK;
	L1C_SEL(l1c_type)->L1C_OP |= ((L1C_FLUSH_ALL_LINES << L1C_OP_OP_OFFSET) | L1C_OP_EN_MASK);

	return L1C_STATUS_OK;
}

l1c_status_t l1c_init(uint32_t l1c_type)
{
	l1c_region_t region;

	/* If l1c is enable , flush and invalidate all l1c lines */
	if (L1C_SEL(l1c_type)->L1C_CON & L1C_CON_MCEN_MASK) {
		L1C_SEL(l1c_type)->L1C_OP &= ~L1C_OP_OP_MASK;
		L1C_SEL(l1c_type)->L1C_OP |= ((L1C_FLUSH_ALL_LINES << L1C_OP_OP_OFFSET) | L1C_OP_EN_MASK);
		L1C_SEL(l1c_type)->L1C_OP &= ~L1C_OP_OP_MASK;
		L1C_SEL(l1c_type)->L1C_OP |= ((L1C_INVALIDATE_ALL_LINES << L1C_OP_OP_OFFSET) | L1C_OP_EN_MASK);
	}

	/* Set L1C related registers to default value*/
	L1C_SEL(l1c_type)->L1C_CON = 0;
	L1C_SEL(l1c_type)->L1C_REGION_EN = 0;

	/* Set L1C region registers to default value and update the global variable */
	for (region = L1C_REGION_0; region < L1C_REGION_MAX; region ++) {
		/* Set L1C related registers to default value */
		L1C_SEL(l1c_type)->L1C_ENTRY_N[region] = 0;
		L1C_SEL(l1c_type)->L1C_END_ENTRY_N[region] = 0;
	}

	return L1C_STATUS_OK;
}

void scp_l1c_init(unsigned int l1c_icon, unsigned int l1c_dcon)
{
	l1c_region_t region, region_number;
	uint32_t status = 0;

	/* Max region number is 16 */
	l1c_region_config_t region_cfg_tbl[] = {
		{DRAM_REGION_BASE, DRAM_REGION_LENGTH},
	};

	dprintf(CRITICAL, "DRAM l1cable region:0x%x~0x%x\n", DRAM_REGION_BASE, DRAM_REGION_BASE + DRAM_REGION_LENGTH);

	status |= l1c_init(L1C_IL1C);
	status |= l1c_init(L1C_DL1C);

	status |= l1c_set_size(L1C_IL1C, L1C_INDEX(l1c_icon));
	status |= l1c_set_size(L1C_DL1C, L1C_INDEX(l1c_dcon));

	region_number = (l1c_region_t)(sizeof(region_cfg_tbl) / sizeof(region_cfg_tbl[0]));

	for (region = L1C_REGION_0; region < region_number; region++) {
		status |= l1c_region_config(L1C_IL1C, region, &region_cfg_tbl[region]);
		status |= l1c_region_enable(L1C_IL1C, region);
	}

	for (region = L1C_REGION_0; region < region_number; region++) {
		status |= l1c_region_config(L1C_DL1C, region, &region_cfg_tbl[region]);
		status |= l1c_region_enable(L1C_DL1C, region);
	}

	L1C_SEL(L1C_IL1C)->L1C_CON |= (uint32_t)(l1c_icon & L1C_WAY_NUM);

	status |= l1c_enable(L1C_IL1C);
	status |= l1c_enable(L1C_DL1C);

	/* add for l1c sync bug(mips) */
	l1c_invalidate_all_l1c_lines(L1C_IL1C);
	//l1c_flush_all_l1c_lines(L1C_DL1C);
	l1c_invalidate_all_l1c_lines(L1C_DL1C);
	dprintf(CRITICAL, "l1c init status: 0x%x\n", status);
}


