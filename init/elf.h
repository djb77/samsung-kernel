/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#ifndef __ELF_H__
#define __ELF_H__

#define __TARGET_64__

#define _SHN_UNDEF	0

#ifdef __TARGET_64__
#define _DT_NULL      0
#define _DT_NEEDED    1
#define _DT_PLTRELSZ  2
#define _DT_PLTGOT    3
#define _DT_STRTAB	  5
#define _DT_SYMTAB	  6
#define _DT_RELA      7
#define _DT_RELASZ    8
#define _DT_RELAENT   9
#define _DT_STRSZ     10
#define _DT_SYMENT    11
#define _DT_INIT      12
#define _DT_FINI      13
#define _DT_SONAME    14
#define _DT_REL       17
#define _DT_PLTREL    20
#define _DT_JMPREL    23

#define _R_AARCH64_NONE            0
#define _R_AARCH64_ABS64         257
#define _R_AARCH64_RELATIVE     1027
#define _R_AARCH64_IRELATIVE	1032
#define _R_AARCH64_GLOB_DAT     1025
#define _R_AARCH64_JUMP_SLOT    1026
#define _R_AARCH64_RELATIVE     1027

#else //__TARGET_32__
#define _DT_INIT       12
#define _DT_FINI       13
#define _DT_SONAME      1
#define _DT_STRTAB      5
#define _DT_SYMTAB      6
#define _DT_RELA        7
#define _DT_RELASZ      8
#define _DT_RELAENT     9
#define _DT_STRSZ      10
#define _DT_SYMENT     11
#define _DT_REL        17
#define _DT_RELSZ      18
#define _DT_RELENT     19

#define _DT_JMPREL     23
#define _DT_PLTRELSZ    2
#define _DT_PLTREL     20

#define _R_ARM_NONE       0
#define _R_ARM_ABS32      2
#define _R_ARM_GLOB_DAT  21
#define _R_ARM_JUMP_SLOT 22
#define _R_ARM_RELATIVE  23
#endif

#define _PT_LOAD          1

#define _EI_NIDENT (16)

struct _Elf_Ehdr {
	unsigned char	e_ident[_EI_NIDENT];
	unsigned short	e_type;
	unsigned short	e_machine;
	unsigned int	e_version;
	unsigned long long	e_entry;
	unsigned long long	e_phoff;
	unsigned long long	e_shoff;
	unsigned int	e_flags;
	unsigned short	e_ehsize;
	unsigned short	e_phentsize;
	unsigned short	e_phnum;
	unsigned short	e_shentsize;
	unsigned short	e_shnum;
	unsigned short	e_shstrndx;
};

#ifdef __TARGET_64__

struct _Elf_Shdr {
	unsigned int	sh_name;
	unsigned int	sh_type;
	unsigned long long	sh_flags;
	unsigned long long	sh_addr;
	unsigned long long	sh_offset;
	unsigned long long	sh_size;
	unsigned int	sh_link;
	unsigned int	sh_info;
	unsigned long long	sh_addralign;
	unsigned long long	sh_entsize;
};

#else //__TARGET_32__
struct _Elf_Shdr {
	unsigned int	sh_name;
	unsigned int	sh_type;
	unsigned int	sh_flags;
	unsigned long long	sh_addr;
	unsigned long long	sh_offset;
	unsigned int	sh_size;
	unsigned int	sh_link;
	unsigned int	sh_info;
	unsigned int	sh_addralign;
	unsigned int	sh_entsize;
};

#endif //__TARGET_64__ | __TARGET_32__

#ifdef __TARGET_64__
struct _Elf_Sym {
	unsigned int	st_name;
	unsigned char	st_info;
	unsigned char st_other;
	unsigned short	st_shndx;
	unsigned long long	st_value;
	unsigned long long	st_size;
};

#else //__TARGET_32__
struct _Elf_Sym {
	unsigned int	st_name;
	unsigned long long	st_value;
	unsigned int	st_size;
	unsigned char	st_info;
	unsigned char	st_other;
	unsigned short	st_shndx;
};

#endif //__TARGET_64__ | __TARGET_32__

#define _ELF_ST_BIND(val)		(((unsigned char) (val)) >> 4)
#define _ELF_ST_TYPE(val)		((val) & 0xf)
#define _ELF_ST_INFO(bind, type)	(((bind) << 4) + ((type) & 0xf))

#ifdef __TARGET_64__
struct _Elf_Rela {
	unsigned long long	r_offset;		/* Address */
	unsigned long long	r_info;			/* Relocation type and symbol index */
	long long	r_addend;		/* Addend */
};

#define _ELF_R_SYM(i)			((i) >> 32)
#define _ELF_R_TYPE(i)			((i) & 0xffffffff)

#else //__TARGET_32__
struct _Elf_Rel {
	unsigned long long	r_offset;		/* Address */
	unsigned int	r_info;			/* Relocation type and symbol index */
};

#define _ELF_R_SYM(val)		((val) >> 8)
#define _ELF_R_TYPE(val)	((val) & 0xff)

#endif //__TARGET_64__ | __TARGET_32__

#ifdef __TARGET_64__
struct _Elf_Phdr {
	unsigned int	p_type;			/* Segment type */
	unsigned int	p_flags;		/* Segment flags */
	unsigned long long	p_offset;		/* Segment file offset */
	unsigned long long	p_vaddr;		/* Segment virtual address */
	unsigned long long	p_paddr;		/* Segment physical address */
	unsigned long long	p_filesz;		/* Segment size in file */
	unsigned long long	p_memsz;		/* Segment size in memory */
	unsigned long long	p_align;		/* Segment alignment */
};

#else //__TARGET_32__
struct _Elf_Phdr {
	unsigned int	p_type;			/* Segment type */
	unsigned long long	p_offset;		/* Segment file offset */
	unsigned long long	p_vaddr;		/* Segment virtual address */
	unsigned long long	p_paddr;		/* Segment physical address */
	unsigned int	p_filesz;		/* Segment size in file */
	unsigned int	p_memsz;		/* Segment size in memory */
	unsigned int	p_flags;		/* Segment flags */
	unsigned int	p_align;		/* Segment alignment */
};

#endif //__TARGET_64__ | __TARGET_32__

#ifdef __TARGET_64__
struct _Elf_Dyn {
	long long	d_tag;			/* Dynamic entry type */
	union {
		unsigned long long d_val;		/* Integer value */
		unsigned long long d_ptr;			/* Address value */
	} d_un;
};

#else //__TARGET_32__
struct _Elf_Dyn {
	int	d_tag;			/* Dynamic entry type */
	union {
		unsigned int d_val;			/* Integer value */
		unsigned long long d_ptr;			/* Address value */
	} d_un;
};

#endif //__TARGET_64__ | __TARGET_32__

#endif //__ELF_H__
