/*
 * include/linux/sec_debug_arm64.h
 *
 * COPYRIGHT(C) 2006-2016 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SEC_DEBUG_ARM64_H
#define SEC_DEBUG_ARM64_H

#if defined(CONFIG_ARM64) && defined(CONFIG_SEC_DEBUG)

struct sec_debug_mmu_reg_t {
	uint64_t TTBR0_EL1;
	uint64_t TTBR1_EL1;
	uint64_t TCR_EL1;
	uint64_t MAIR_EL1;
	uint64_t ATCR_EL1;
	uint64_t AMAIR_EL1;

	uint64_t HSTR_EL2;
	uint64_t HACR_EL2;
	uint64_t TTBR0_EL2;
	uint64_t VTTBR_EL2;
	uint64_t TCR_EL2;
	uint64_t VTCR_EL2;
	uint64_t MAIR_EL2;
	uint64_t ATCR_EL2;

	uint64_t TTBR0_EL3;
	uint64_t MAIR_EL3;
	uint64_t ATCR_EL3;
};

/* ARM CORE regs mapping structure */
struct sec_debug_core_t {
	/* COMMON */
	uint64_t x0;
	uint64_t x1;
	uint64_t x2;
	uint64_t x3;
	uint64_t x4;
	uint64_t x5;
	uint64_t x6;
	uint64_t x7;
	uint64_t x8;
	uint64_t x9;
	uint64_t x10;
	uint64_t x11;
	uint64_t x12;
	uint64_t x13;
	uint64_t x14;
	uint64_t x15;
	uint64_t x16;
	uint64_t x17;
	uint64_t x18;
	uint64_t x19;
	uint64_t x20;
	uint64_t x21;
	uint64_t x22;
	uint64_t x23;
	uint64_t x24;
	uint64_t x25;
	uint64_t x26;
	uint64_t x27;
	uint64_t x28;
	uint64_t x29;		/* sp */
	uint64_t x30;		/* lr */

	uint64_t pc;		/* pc */
	uint64_t cpsr;		/* cpsr */

	/* EL0 */
	uint64_t sp_el0;

	/* EL1 */
	uint64_t sp_el1;
	uint64_t elr_el1;
	uint64_t spsr_el1;

	/* EL2 */
	uint64_t sp_el2;
	uint64_t elr_el2;
	uint64_t spsr_el2;

	/* EL3 */
	/* uint64_t sp_el3; */
	/* uint64_t elr_el3; */
	/* uint64_t spsr_el3; */
};

#define READ_SPECIAL_REG(x) ({ \
	uint64_t val; \
	asm volatile ("mrs %0, " # x : "=r"(val)); \
	val; \
})

static inline void sec_debug_save_mmu_reg(struct sec_debug_mmu_reg_t *mmu_reg)
{
	uint64_t pstate, which_el;

	pstate = READ_SPECIAL_REG(CurrentEl);
	which_el = pstate & PSR_MODE_MASK;

	/* pr_emerg("%s: sec_debug EL mode=%d\n", __func__,which_el); */

	mmu_reg->TTBR0_EL1 = READ_SPECIAL_REG(TTBR0_EL1);
	mmu_reg->TTBR1_EL1 = READ_SPECIAL_REG(TTBR1_EL1);
	mmu_reg->TCR_EL1 = READ_SPECIAL_REG(TCR_EL1);
	mmu_reg->MAIR_EL1 = READ_SPECIAL_REG(MAIR_EL1);
	mmu_reg->AMAIR_EL1 = READ_SPECIAL_REG(AMAIR_EL1);

#if 0
	if (which_el >= PSR_MODE_EL2t) {
		mmu_reg->HSTR_EL2 = READ_SPECIAL_REG(HSTR_EL2);
		mmu_reg->HACR_EL2 = READ_SPECIAL_REG(HACR_EL2);
		mmu_reg->TTBR0_EL2 = READ_SPECIAL_REG(TTBR0_EL2);
		mmu_reg->VTTBR_EL2 = READ_SPECIAL_REG(VTTBR_EL2);
		mmu_reg->TCR_EL2 = READ_SPECIAL_REG(TCR_EL2);
		mmu_reg->VTCR_EL2 = READ_SPECIAL_REG(VTCR_EL2);
		mmu_reg->MAIR_EL2 = READ_SPECIAL_REG(MAIR_EL2);
		mmu_reg->AMAIR_EL2 = READ_SPECIAL_REG(AMAIR_EL2);
	}
#endif
}

static inline void sec_debug_save_core_reg(struct sec_debug_core_t *core_reg)
{
	uint64_t pstate,which_el;

	pstate = READ_SPECIAL_REG(CurrentEl);
	which_el = pstate & PSR_MODE_MASK;

	/* pr_emerg("%s: sec_debug EL mode=%d\n", __func__,which_el); */

	asm volatile (
		"str x0, [%0,#0]\n\t"		/* x0 is pushed first to core_reg */
		"mov x0, %0\n\t"		/* x0 will be alias for core_reg */
		"str x1, [x0,#0x8]\n\t"		/* x1 */
		"str x2, [x0,#0x10]\n\t"	/* x2 */
		"str x3, [x0,#0x18]\n\t"	/* x3 */
		"str x4, [x0,#0x20]\n\t"	/* x4 */
		"str x5, [x0,#0x28]\n\t"	/* x5 */
		"str x6, [x0,#0x30]\n\t"	/* x6 */
		"str x7, [x0,#0x38]\n\t"	/* x7 */
		"str x8, [x0,#0x40]\n\t"	/* x8 */
		"str x9, [x0,#0x48]\n\t"	/* x9 */
		"str x10, [x0,#0x50]\n\t"	/* x10 */
		"str x11, [x0,#0x58]\n\t"	/* x11 */
		"str x12, [x0,#0x60]\n\t"	/* x12 */
		"str x13, [x0,#0x68]\n\t"	/* x13 */
		"str x14, [x0,#0x70]\n\t"	/* x14 */
		"str x15, [x0,#0x78]\n\t"	/* x15 */
		"str x16, [x0,#0x80]\n\t"	/* x16 */
		"str x17, [x0,#0x88]\n\t"	/* x17 */
		"str x18, [x0,#0x90]\n\t"	/* x18 */
		"str x19, [x0,#0x98]\n\t"	/* x19 */
		"str x20, [x0,#0xA0]\n\t"	/* x20 */
		"str x21, [x0,#0xA8]\n\t"	/* x21 */
		"str x22, [x0,#0xB0]\n\t"	/* x22 */
		"str x23, [x0,#0xB8]\n\t"	/* x23 */
		"str x24, [x0,#0xC0]\n\t"	/* x24 */
		"str x25, [x0,#0xC8]\n\t"	/* x25 */
		"str x26, [x0,#0xD0]\n\t"	/* x26 */
		"str x27, [x0,#0xD8]\n\t"	/* x27 */
		"str x28, [x0,#0xE0]\n\t"	/* x28 */
		"str x29, [x0,#0xE8]\n\t"	/* x29 */
		"str x30, [x0,#0xF0]\n\t"	/* x30 */
		"adr x1, .\n\t"
		"str x1, [x0,#0xF8]\n\t"	/* pc */

		/* pstate */
		"mrs x15, NZCV\n\t"
		"bic x15, x15, #0xFFFFFFFF0FFFFFFF\n\t"
		"mrs x9, DAIF\n\t"
		"bic x9, x9, #0xFFFFFFFFFFFFFC3F\n\t"
		"orr x15, x15, x9\n\t"
		"mrs x10, CurrentEL\n\t"
		"bic x10, x10, #0xFFFFFFFFFFFFFFF3\n\t"
		"orr x15, x15, x10\n\t"
		"mrs x11, SPSel\n\t"
		"bic x11, x11, #0xFFFFFFFFFFFFFFFE\n\t"
		"orr x15, x15, x11\n\t"
		"str x15, [%0,#0x100]\n\t"

		:				/* output */
		: "r"(core_reg)			/* input */
		: "%x0", "%x1"			/* clobbered registers */
	);

	core_reg->sp_el0 = READ_SPECIAL_REG(sp_el0);

	if(which_el >= PSR_MODE_EL2t){
		core_reg->sp_el0 = READ_SPECIAL_REG(sp_el1);
		core_reg->elr_el1 = READ_SPECIAL_REG(elr_el1);
		core_reg->spsr_el1 = READ_SPECIAL_REG(spsr_el1);
		core_reg->sp_el2 = READ_SPECIAL_REG(sp_el2);
		core_reg->elr_el2 = READ_SPECIAL_REG(elr_el2);
		core_reg->spsr_el2 = READ_SPECIAL_REG(spsr_el2);
	}
}

#endif /* defined(CONFIG_ARM64) && defined(CONFIG_SEC_DEBUG) */

#endif /* SEC_DEBUG_ARM64_H */
