/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2015. All rights reserved.
*
* BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
* THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
* RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
* AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
* NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
* SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
* SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
* THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
* THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
* CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
* SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
* STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
* CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
* AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
* OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
* MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*/

#ifndef _MT_SCP_EXCEP_H_
#define _MT_SCP_EXCEP_H_

typedef short   __s16;
typedef int __s32;
typedef long long __s64;
typedef unsigned short  __u16;
typedef unsigned int    __u32;
typedef unsigned long long __u64;

struct TaskContextType {
	unsigned int r0;
	unsigned int r1;
	unsigned int r2;
	unsigned int r3;
	unsigned int r4;
	unsigned int r5;
	unsigned int r6;
	unsigned int r7;
	unsigned int r8;
	unsigned int r9;
	unsigned int r10;
	unsigned int r11;
	unsigned int r12;
	unsigned int sp;              /* after pop r0-r3, lr, pc, xpsr                   */
	unsigned int lr;              /* lr before exception                             */
	unsigned int pc;              /* pc before exception                             */
	unsigned int psr;             /* xpsr before exeption                            */
	unsigned int control;         /* nPRIV bit & FPCA bit meaningful, SPSEL bit = 0  */
	unsigned int exc_return;      /* current lr                                      */
	unsigned int msp;             /* msp                                             */
};

#define CRASH_SUMMARY_LENGTH 12
#define CRASH_MEMORY_HEADER_SIZE  (8 * 1024)
#define CRASH_MEMORY_OFFSET  (0x800)
#define CRASH_MEMORY_LENGTH  (512 * 1024 - CRASH_MEMORY_OFFSET)
#define CRASH_DRAM_OFFSET   (0x80000)
#define CRASH_DRAM_LENGTH   (0x100000)

#define CRASH_REG_SIZE  (9 * 32)

#define ELF_NGREGS 18
#define ELF_PRARGSZ (80)    /* Number of chars for args */
#define EM_ARM      40  /* ARM 32 bit */
#define ELF_CORE_EFLAGS 0
#define CORE_STR "CORE"


/* 32-bit ELF base types. */
typedef __u32   Elf32_Addr;
typedef __u16   Elf32_Half;
typedef __u32   Elf32_Off;
typedef __s32   Elf32_Sword;
typedef __u32   Elf32_Word;

/* 64-bit ELF base types. */
typedef __u64   Elf64_Addr;
typedef __u16   Elf64_Half;
typedef __s16   Elf64_SHalf;
typedef __u64   Elf64_Off;
typedef __s32   Elf64_Sword;
typedef __u32   Elf64_Word;
typedef __u64   Elf64_Xword;
typedef __s64   Elf64_Sxword;

/* These constants are for the segment types stored in the image headers */
#define PT_LOAD    1
#define PT_NOTE    4

/* These constants define the different elf file types */
#define ET_NONE   0
#define ET_REL    1
#define ET_EXEC   2
#define ET_DYN    3
#define ET_CORE   4
#define ET_LOPROC 0xff00
#define ET_HIPROC 0xffff

#define ELF_ST_BIND(x)      ((x) >> 4)
#define ELF_ST_TYPE(x)      (((unsigned int) x) & 0xf)
#define ELF32_ST_BIND(x)    ELF_ST_BIND(x)
#define ELF32_ST_TYPE(x)    ELF_ST_TYPE(x)
#define ELF64_ST_BIND(x)    ELF_ST_BIND(x)
#define ELF64_ST_TYPE(x)    ELF_ST_TYPE(x)

#define EI_NIDENT   16

typedef struct elf32_hdr {
	unsigned char e_ident[EI_NIDENT];
	Elf32_Half    e_type;
	Elf32_Half    e_machine;
	Elf32_Word    e_version;
	Elf32_Addr    e_entry;  /* Entry point */
	Elf32_Off e_phoff;
	Elf32_Off e_shoff;
	Elf32_Word    e_flags;
	Elf32_Half    e_ehsize;
	Elf32_Half    e_phentsize;
	Elf32_Half    e_phnum;
	Elf32_Half    e_shentsize;
	Elf32_Half    e_shnum;
	Elf32_Half    e_shstrndx;
} Elf32_Ehdr;

typedef struct elf64_hdr {
	unsigned char e_ident[EI_NIDENT]; /* ELF "magic number" */
	Elf64_Half e_type;
	Elf64_Half e_machine;
	Elf64_Word e_version;
	Elf64_Addr e_entry;       /* Entry point virtual address */
	Elf64_Off e_phoff;        /* Program header table file offset */
	Elf64_Off e_shoff;        /* Section header table file offset */
	Elf64_Word e_flags;
	Elf64_Half e_ehsize;
	Elf64_Half e_phentsize;
	Elf64_Half e_phnum;
	Elf64_Half e_shentsize;
	Elf64_Half e_shnum;
	Elf64_Half e_shstrndx;
} Elf64_Ehdr;

/* These constants define the permissions on sections in the program
   header, p_flags. */
#define PF_R        0x4
#define PF_W        0x2
#define PF_X        0x1

typedef struct elf32_phdr {
	Elf32_Word    p_type;
	Elf32_Off p_offset;
	Elf32_Addr    p_vaddr;
	Elf32_Addr    p_paddr;
	Elf32_Word    p_filesz;
	Elf32_Word    p_memsz;
	Elf32_Word    p_flags;
	Elf32_Word    p_align;
} Elf32_Phdr;

typedef struct elf64_phdr {
	Elf64_Word p_type;
	Elf64_Word p_flags;
	Elf64_Off p_offset;       /* Segment file offset */
	Elf64_Addr p_vaddr;       /* Segment virtual address */
	Elf64_Addr p_paddr;       /* Segment physical address */
	Elf64_Xword p_filesz;     /* Segment size in file */
	Elf64_Xword p_memsz;      /* Segment size in memory */
	Elf64_Xword p_align;      /* Segment alignment, file & memory */
} Elf64_Phdr;

/* sh_type */
#define SHT_NULL    0
#define SHT_PROGBITS    1
#define SHT_SYMTAB  2
#define SHT_STRTAB  3
#define SHT_RELA    4
#define SHT_HASH    5
#define SHT_DYNAMIC 6
#define SHT_NOTE    7
#define SHT_NOBITS  8
#define SHT_REL     9
#define SHT_SHLIB   10
#define SHT_DYNSYM  11
#define SHT_NUM     12
#define SHT_LOPROC  0x70000000
#define SHT_HIPROC  0x7fffffff
#define SHT_LOUSER  0x80000000
#define SHT_HIUSER  0xffffffff

/* sh_flags */
#define SHF_WRITE   0x1
#define SHF_ALLOC   0x2
#define SHF_EXECINSTR   0x4
#define SHF_MASKPROC    0xf0000000

/* special section indexes */
#define SHN_UNDEF   0
#define SHN_LORESERVE   0xff00
#define SHN_LOPROC  0xff00
#define SHN_HIPROC  0xff1f
#define SHN_ABS     0xfff1
#define SHN_COMMON  0xfff2
#define SHN_HIRESERVE   0xffff

typedef struct elf32_shdr {
	Elf32_Word    sh_name;
	Elf32_Word    sh_type;
	Elf32_Word    sh_flags;
	Elf32_Addr    sh_addr;
	Elf32_Off sh_offset;
	Elf32_Word    sh_size;
	Elf32_Word    sh_link;
	Elf32_Word    sh_info;
	Elf32_Word    sh_addralign;
	Elf32_Word    sh_entsize;
} Elf32_Shdr;

typedef struct elf64_shdr {
	Elf64_Word sh_name;       /* Section name, index in string tbl */
	Elf64_Word sh_type;       /* Type of section */
	Elf64_Xword sh_flags;     /* Miscellaneous section attributes */
	Elf64_Addr sh_addr;       /* Section virtual addr at execution */
	Elf64_Off sh_offset;      /* Section file offset */
	Elf64_Xword sh_size;      /* Size of section in bytes */
	Elf64_Word sh_link;       /* Index of another section */
	Elf64_Word sh_info;       /* Additional section information */
	Elf64_Xword sh_addralign; /* Section alignment */
	Elf64_Xword sh_entsize;   /* Entry size if section holds table */
} Elf64_Shdr;

#define EI_MAG0     0       /* e_ident[] indexes */
#define EI_MAG1     1
#define EI_MAG2     2
#define EI_MAG3     3
#define EI_CLASS    4
#define EI_DATA     5
#define EI_VERSION  6
#define EI_OSABI    7
#define EI_PAD      8

#define ELFMAG      "\177ELF"
#define SELFMAG     4

#define ELFCLASSNONE    0       /* EI_CLASS */
#define ELFCLASS32  1
#define ELFCLASS64  2
#define ELFCLASSNUM 3

#define ELFDATANONE 0       /* e_ident[EI_DATA] */
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define EV_NONE     0       /* e_version, EI_VERSION */
#define EV_CURRENT  1
#define EV_NUM      2

#define ELFOSABI_NONE   0

#ifndef ELF_OSABI
#define ELF_OSABI ELFOSABI_NONE
#endif

#define NT_PRSTATUS 1
#define NT_PRFPREG  2
#define NT_PRPSINFO 3
#define NT_TASKSTRUCT   4
#define NT_AUXV     6


/* Note header in a PT_NOTE section */
typedef struct elf32_note {
	Elf32_Word    n_namesz;   /* Name size */
	Elf32_Word    n_descsz;   /* Content size */
	Elf32_Word    n_type;     /* Content type */
} Elf32_Nhdr;

/* Note header in a PT_NOTE section */
typedef struct elf64_note {
	Elf64_Word n_namesz;  /* Name size */
	Elf64_Word n_descsz;  /* Content size */
	Elf64_Word n_type;    /* Content type */
} Elf64_Nhdr;

/* scp reg dump*/
struct scp_reg_dump_list {
	uint32_t scp_reg_magic;
	uint32_t ap_resource;
	uint32_t bus_resource;
	uint32_t slp_protect;
	uint32_t cpu_sleep_status;
	uint32_t clk_sw_sel;
	uint32_t clk_enable;
	uint32_t clk_high_core;
	uint32_t debug_wdt_sp;
	uint32_t debug_wdt_lr;
	uint32_t debug_wdt_psp;
	uint32_t debug_wdt_pc;
	uint32_t debug_addr_s2r;
	uint32_t debug_addr_dma;
	uint32_t debug_addr_spi0;
	uint32_t debug_addr_spi1;
	uint32_t debug_addr_spi2;
	uint32_t debug_bus_status;
	uint32_t debug_infra_mon;
	uint32_t infra_addr_latch;
	uint32_t ddebug_latch;
	uint32_t pdebug_latch;
	uint32_t pc_value;
	uint32_t scp_reg_magic_end;
};

struct scp_dump_header_list {
	uint32_t scp_head_magic;
	struct scp_region_info_st scp_region_info;
	uint32_t scp_head_magic_end;
};

struct MemoryDump {
	struct elf32_hdr elf;
	struct elf32_phdr nhdr;
	struct elf32_phdr phdr;
	char notes[CRASH_MEMORY_HEADER_SIZE-sizeof(struct elf32_hdr)
		-sizeof(struct elf32_phdr)-sizeof(struct elf32_phdr)
		-sizeof(struct scp_dump_header_list)];
	/* ram dump total header size(elf+nhdr+phdr+header)
	 * must be fixed at CRASH_MEMORY_HEADER_SIZE
	 */
	struct scp_dump_header_list scp_dump_header;
	/*scp sram*/
	char memory[CRASH_MEMORY_LENGTH];
	/*scp reg*/
	struct scp_reg_dump_list scp_reg_dump;
};

enum scp_core_id {
	SCP_A_ID = 0,
	SCP_CORE_TOTAL = 1,
};


#define ET_CORE   4
#define AED_LOG_PRINT_SIZE  SZ_16K
#define SCP_LOCK_OFS    0xE0
#define SCP_TCM_LOCK_BIT    (1 << 20)

enum scp_excep_id {
	EXCEP_LOAD_FIRMWARE = 0,
	EXCEP_RESET,
	EXCEP_BOOTUP,
	EXCEP_RUNTIME,
	SCP_NR_EXCEP,
};

void scp_aed(enum scp_excep_id type, enum scp_core_id id);
void scp_aed_reset(enum scp_excep_id type, enum scp_core_id id);
void scp_aed_reset_inplace(enum scp_excep_id type, enum scp_core_id id);
void scp_get_log(enum scp_core_id id);
char *scp_get_last_log(enum scp_core_id id);
void scp_A_dump_regs(void);
uint32_t scp_dump_pc(void);
uint32_t scp_dump_lr(void);
void aed_scp_exception_api(const int *log, int log_size, const int *phy,
                           int phy_size, const char *detail, const int db_opt);
void scp_excep_cleanup(void);
int scp_crash_dump(void *crash_buffer);

enum { r0, r1, r2, r3, r12, lr, pc, psr};

#endif

