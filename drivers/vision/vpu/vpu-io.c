/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/slab.h>

#include "vpu-io.h"
#if 0
void *io2iocpy(void *dst, void *src, u32 size)
{
	u32 lalign;
	u32 ralign;
	u32 last_wrd;
	u32 curr_wrd;
	u32 *src32;
	u32 *dst32;
	u8  *src8;
	u8  *dst8;

	src8 = (u8 *)(src);
	dst8 = (u8 *)(dst);
	/* first copy byte by byte till the source first alignment
	 * this step is necessary to ensure we do not even try to access
	 * data which is before the source buffer, hence it is not ours.
	 */
	while ((src8 & 3) && size) /* (src mod 4) > 0 and size > 0 */
	{
		IOW8(*dst8, IOR8(*src8));
		dst8++;
		src8++;
		size--;
	}

	/* align destination (possibly disaligning source)*/
	while ((dst8 & 3) && size) /* (dst mod 4) > 0 and size > 0 */
	{
		IOW8(*dst8, IOR8(*src8));
		dst8++;
		src8++;
		size--;
	}

	/* dest is aligned and source is not necessarily aligned */
	lalign = (uint32_t)((PTR2UINT(src8) & 3) << 3);
	ralign = 32 - lalign;

	if (lalign == 0) {
		/* source is also aligned */
		src32 = (uint32_t*)(src8);
		dst32 = (uint32_t*)(dst8);
		while (size >> 2) /* size >= 4 */
		{
			IOW32(*dst32, IOR32(*src32));
			dst32++;
			src32++;
			size -= 4;
		}
		src8 = (uint8_t *)(src32);
		dst8 = (uint8_t *)(dst32);
	}
	else
	{
		/* source is not aligned (destination is aligned)*/
		src32 = (uint32_t*)(src8 - (lalign >> 3));
		dst32 = (uint32_t*)(dst8);
		last_wrd = IOR32(*src32);
		src32++;
		while(size >> 3) /* size >= 8 */
		{
			curr_wrd = IOR32(*src32);
			IOW32(*dst32, (last_wrd << lalign) |
				(curr_wrd >> ralign));
			last_wrd = curr_wrd;
			src32++;
			dst32++;
			size -= 4;
		}
		dst8 = (uint8_t *)(dst32);
		src8 = (uint8_t *)(src32) - 4 + (lalign >> 3);
	}

	/* complete the left overs */
	while (size--)
	{
		IOW8(*dst8, IOR8(*src8));
		dst8++;
		src8++;
	}

	return dst;
}
#endif
void *mem2iocpy(void *dst,void *src, u32 size)
{
	//u32 lalign;
	//u32 ralign;
	//u32 last_wrd;
	//u32 curr_wrd;
	//u32 *src32;
	//u32 *dst32;
	u8  *src8;
	u8  *dst8;

	src8 = (u8 *)(src);
	dst8 = (u8 *)(dst);
	/* first copy byte by byte till the source first alignment
	 * this step is necessary to ensure we do not even try to access
	 * data which is before the source buffer, hence it is not ours.
	 */
#if 1
	/* complete the left overs */
	while (size--)
	{
		IOW8(*dst8, *src8);
		dst8++;
		src8++;
	}
#else
	while((src8 & 3) && size) /* (src mod 4) > 0 and size > 0 */
	{
		IOW8(*dst8, *src8);
		dst8++;
		src8++;
		size--;
	}

	/* align destination (possibly dis aligning source)*/
	while((dst8 & 3) && size) /* (dst mod 4) > 0 and size > 0 */
	{
		IOW8(*dst8, *src8);
		dst8++;
		src8++;
		size--;
	}

	/* destination is aligned and source is not necessarily aligned */
	lalign = (uint32_t)((src8 & 3) << 3);
	ralign = 32 - lalign;

	if (lalign == 0)
	{
		/* source is also aligned */
		src32 = (uint32_t*)(src8);
		dst32 = (uint32_t*)(dst8);
		while (size >> 2) /* size >= 4 */
		{
			IOW32(*dst32, *src32);
			dst32++;
			src32++;
			size -= 4;
		}
		src8 = (uint8_t *)(src32);
		dst8 = (uint8_t *)(dst32);
	}
	else
	{
		/* source is not aligned (destination is aligned)*/
		src32 = (uint32_t*)(src8 - (lalign >> 3));
		dst32 = (uint32_t*)(dst8);
		last_wrd = *src32++;
		while (size >> 3) /* size >= 8 */
		{
			curr_wrd = *src32;
			IOW32(*dst32, (last_wrd << lalign) |
				(curr_wrd >> ralign));
			last_wrd = curr_wrd;
			src32++;
			dst32++;
			size -= 4;
		}
		dst8 = (uint8_t *)(dst32);
		src8 = (uint8_t *)(src32) - 4 + (lalign >> 3);
	}

	/* complete the left overs */
	while (size--)
	{
		IOW8(*dst8, *src8);
		dst8++;
		src8++;
	}
#endif
	return dst;
}
#if 1
void *io2memcpy(void *dst,void *src, u32 size)
{
#if 1
	u8  *src8;
	u8  *dst8;

	src8 = (u8 *)(src);
	dst8 = (u8 *)(dst);

	while (size--)
	{
		*dst8 = IOR8(*src8);
		dst8++;
		src8++;
	}

	return dst;
#else
	uint32_t lalign;
	uint32_t ralign;
	uint32_t last_wrd;
	uint32_t curr_wrd;
	uint32_t *src32;
	uint32_t *dst32;
	uint8_t  *src8;
	uint8_t  *dst8;

	src8 = (uint8_t *)(src);
	dst8 = (uint8_t *)(dst);
	/* first copy byte by byte till the source first alignment
	 * this step is necessary to ensure we do not even try to access
	 * data which is before the source buffer, hence it is not ours.
	 */
	while((PTR2UINT(src8) & 3) && size) /* (src mod 4) > 0 and size > 0 */
	{
		*dst8 = IOR8(*src8);
		dst8++;
		src8++;
		size--;
	}

	/* align destination (possibly dis aligning source)*/
	while((PTR2UINT(dst8) & 3) && size) /* (dst mod 4) > 0 and size > 0 */
	{
		*dst8 = IOR8(*src8);
		dst8++;
		src8++;
		size--;
	}

	/* dest is aligned and source is not necessarily aligned */
	lalign = (uint32_t)((PTR2UINT(src8) & 3) << 3);
	ralign = 32 - lalign;

	if (lalign == 0)
	{
		/* source is also aligned */
		src32 = (uint32_t*)(src8);
		dst32 = (uint32_t*)(dst8);
		while (size >> 2) /* size >= 4 */
		{
			*dst32 = IOR32(*src32);
			dst32++;
			src32++;
			size -= 4;
		}
		src8 = (uint8_t *)(src32);
		dst8 = (uint8_t *)(dst32);
	}
	else
	{
		/* source is not aligned (destination is aligned)*/
		src32 = (uint32_t*)(src8 - (lalign >> 3));
		dst32 = (uint32_t*)(dst8);
		last_wrd = IOR32(*src32);
		src32++;
		while(size >> 3) /* size >= 8 */
		{
			curr_wrd = IOR32(*src32);
			*dst32 = (last_wrd << lalign) | (curr_wrd >> ralign);
			last_wrd = curr_wrd;
			src32++;
			dst32++;
			size -= 4;
		}
		dst8 = (uint8_t *)(dst32);
		src8 = (uint8_t *)(src32) - 4 + (lalign >> 3);
	}

	/* complete the left overs */
	while (size--)
	{
		*dst8 = IOR8(*src8);
		dst8++;
		src8++;
	}

	return dst;
#endif
}
#endif
#if 0
void *iomemset(void *dst, uint8_t val, uint32_t size)
{
	uint32_t val32;
	uint32_t *dst32;
	uint8_t  *dst8;

	dst8 = (uint8_t *)(dst);

	/* generate four 8-bit val's in 32-bit container */
	val32  = (uint32_t) val;
	val32 |= (val32 <<  8);
	val32 |= (val32 << 16);

	/* align destination to 32 */
	while((PTR2UINT(dst8) & 3) && size) /* (dst mod 4) > 0 and size > 0 */
	{
		IOW8(*dst8, val);
		dst8++;
		size--;
	}

	/* 32-bit chunks */
	dst32 = (uint32_t*)(dst8);
	while (size >> 2) /* size >= 4 */
	{
		IOW32(*dst32, val32);
		dst32++;
		size -= 4;
	}

	/* complete the leftovers */
	dst8 = (uint8_t *)(dst32);
	while (size--)
	{
		IOW8(*dst8, val);
		dst8++;
	}

	return dst;
}
#endif