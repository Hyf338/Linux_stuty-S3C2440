/*
 * YAFFS: Yet Another Flash File System. A NAND-flash specific file system.
 *
 * Copyright (C) 2002-2007 Aleph One Ltd.
 *   for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 * Nick Bane modifications flagged NCB
 * Endian handling patches by James Ng.
 * mkyaffs2image hacks by NCB
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * makeyaffs2image.c 
 *
 * Makes a YAFFS2 file system image that can be used to load up a file system.
 * Uses default Linux MTD layout - change if you need something different.
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include "yaffs_ecc.h"
#include "yaffs_guts.h"

#include "yaffs_tagsvalidity.h"
#include "yaffs_packedtags2.h"

typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned int		u_int;
typedef unsigned long		u_long;

typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;
typedef unsigned int		uint32_t;



unsigned yaffs_traceMask=0;

#define MAX_OBJECTS 10000

#define chunkSize 2048
#define spareSize 64

const char * mkyaffsimage_c_version = "$Id: mkyaffs2image.c,v 1.4 2007/02/14 01:09:06 wookey Exp $";


typedef struct
{
	dev_t dev;
	ino_t ino;
	int   obj;
} objItem;


static objItem obj_list[MAX_OBJECTS];
static int n_obj = 0;
static int obj_id = YAFFS_NOBJECT_BUCKETS + 1;

static int nObjects, nDirectories, nPages;

static int outFile;

static int error;

static int convert_endian = 0;

struct nand_oobinfo {
	uint32_t useecc;
	uint32_t eccbytes;
	uint32_t oobfree[8][2];
	uint32_t eccpos[32];
};

/* ECC byte placement */
#define MTD_NANDECC_OFF		0	// Switch off ECC (Not recommended)
#define MTD_NANDECC_PLACE	1	// Use the given placement in the structure (YAFFS1 legacy mode)
#define MTD_NANDECC_AUTOPLACE	2	// Use the default placement scheme
#define MTD_NANDECC_PLACEONLY	3	// Use the given placement in the structure (Do not store ecc result on read)
#define MTD_NANDECC_AUTOPL_USR 	4	// Use the given autoplacement scheme rather than using the default

static struct nand_oobinfo nand_oob_64 = {
	.useecc = MTD_NANDECC_AUTOPLACE,
	.eccbytes = 24,
	.eccpos = {
		40, 41, 42, 43, 44, 45, 46, 47, 
		48, 49, 50, 51, 52, 53, 54, 55, 
		56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = { {2, 38} }
};

static u_char oob_buf[spareSize];

static int obj_compare(const void *a, const void * b)
{
  objItem *oa, *ob;
  
  oa = (objItem *)a;
  ob = (objItem *)b;
  
  if(oa->dev < ob->dev) return -1;
  if(oa->dev > ob->dev) return 1;
  if(oa->ino < ob->ino) return -1;
  if(oa->ino > ob->ino) return 1;
  
  return 0;
}


static void add_obj_to_list(dev_t dev, ino_t ino, int obj)
{
	if(n_obj < MAX_OBJECTS)
	{
		obj_list[n_obj].dev = dev;
		obj_list[n_obj].ino = ino;
		obj_list[n_obj].obj = obj;
		n_obj++;
		qsort(obj_list,n_obj,sizeof(objItem),obj_compare);
		
	}
	else
	{
		// oops! not enough space in the object array
		fprintf(stderr,"Not enough space in object array\n");
		exit(2);
	}
}


static int find_obj_in_list(dev_t dev, ino_t ino)
{
	objItem *i = NULL;
	objItem test;

	test.dev = dev;
	test.ino = ino;
	
	if(n_obj > 0)
	{
		i = bsearch(&test,obj_list,n_obj,sizeof(objItem),obj_compare);
	}

	if(i)
	{
		return i->obj;
	}
	return -1;
}

/* This little function converts a little endian tag to a big endian tag.
 * NOTE: The tag is not usable after this other than calculating the CRC
 * with.
 */
static void little_to_big_endian(yaffs_Tags *tagsPtr)
{
#if 0 // FIXME NCB
    yaffs_TagsUnion * tags = (yaffs_TagsUnion* )tagsPtr; // Work in bytes.
    yaffs_TagsUnion   temp;

    memset(&temp, 0, sizeof(temp));
    // Ick, I hate magic numbers.
    temp.asBytes[0] = ((tags->asBytes[2] & 0x0F) << 4) | ((tags->asBytes[1] & 0xF0) >> 4);
    temp.asBytes[1] = ((tags->asBytes[1] & 0x0F) << 4) | ((tags->asBytes[0] & 0xF0) >> 4);
    temp.asBytes[2] = ((tags->asBytes[0] & 0x0F) << 4) | ((tags->asBytes[2] & 0x30) >> 2) | ((tags->asBytes[3] & 0xC0) >> 6);
    temp.asBytes[3] = ((tags->asBytes[3] & 0x3F) << 2) | ((tags->asBytes[2] & 0xC0) >> 6);
    temp.asBytes[4] = ((tags->asBytes[6] & 0x03) << 6) | ((tags->asBytes[5] & 0xFC) >> 2);
    temp.asBytes[5] = ((tags->asBytes[5] & 0x03) << 6) | ((tags->asBytes[4] & 0xFC) >> 2);
    temp.asBytes[6] = ((tags->asBytes[4] & 0x03) << 6) | (tags->asBytes[7] & 0x3F);
    temp.asBytes[7] = (tags->asBytes[6] & 0xFC) | ((tags->asBytes[7] & 0xC0) >> 6);

    // Now copy it back.
    tags->asBytes[0] = temp.asBytes[0];
    tags->asBytes[1] = temp.asBytes[1];
    tags->asBytes[2] = temp.asBytes[2];
    tags->asBytes[3] = temp.asBytes[3];
    tags->asBytes[4] = temp.asBytes[4];
    tags->asBytes[5] = temp.asBytes[5];
    tags->asBytes[6] = temp.asBytes[6];
    tags->asBytes[7] = temp.asBytes[7];
#endif
}


/*
 * Pre-calculated 256-way 1 byte column parity
 */
static const u_char nand_ecc_precalc_table[] = {
	0x00, 0x55, 0x56, 0x03, 0x59, 0x0c, 0x0f, 0x5a, 0x5a, 0x0f, 0x0c, 0x59, 0x03, 0x56, 0x55, 0x00,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
	0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
	0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
	0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
	0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
	0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
	0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
	0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
	0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
	0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
	0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x00, 0x55, 0x56, 0x03, 0x59, 0x0c, 0x0f, 0x5a, 0x5a, 0x0f, 0x0c, 0x59, 0x03, 0x56, 0x55, 0x00
};


/**
 * nand_trans_result - [GENERIC] create non-inverted ECC
 * @reg2:	line parity reg 2
 * @reg3:	line parity reg 3
 * @ecc_code:	ecc 
 *
 * Creates non-inverted ECC code from line parity
 */
static void nand_trans_result(u_char reg2, u_char reg3,
	u_char *ecc_code)
{
	u_char a, b, i, tmp1, tmp2;
	
	/* Initialize variables */
	a = b = 0x80;
	tmp1 = tmp2 = 0;
	
	/* Calculate first ECC byte */
	for (i = 0; i < 4; i++) {
		if (reg3 & a)		/* LP15,13,11,9 --> ecc_code[0] */
			tmp1 |= b;
		b >>= 1;
		if (reg2 & a)		/* LP14,12,10,8 --> ecc_code[0] */
			tmp1 |= b;
		b >>= 1;
		a >>= 1;
	}
	
	/* Calculate second ECC byte */
	b = 0x80;
	for (i = 0; i < 4; i++) {
		if (reg3 & a)		/* LP7,5,3,1 --> ecc_code[1] */
			tmp2 |= b;
		b >>= 1;
		if (reg2 & a)		/* LP6,4,2,0 --> ecc_code[1] */
			tmp2 |= b;
		b >>= 1;
		a >>= 1;
	}
	
	/* Store two of the ECC bytes */
	ecc_code[0] = tmp1;
	ecc_code[1] = tmp2;
}

/**
 * nand_calculate_ecc - [NAND Interface] Calculate 3 byte ECC code for 256 byte block
 * @mtd:	MTD block structure
 * @dat:	raw data
 * @ecc_code:	buffer for ECC
 */
int nand_calculate_ecc(const u_char *dat, u_char *ecc_code)
{
	u_char idx, reg1, reg2, reg3;
	int j;
	
	/* Initialize variables */
	reg1 = reg2 = reg3 = 0;
	ecc_code[0] = ecc_code[1] = ecc_code[2] = 0;
	
	/* Build up column parity */ 
	for(j = 0; j < 256; j++) {
		
		/* Get CP0 - CP5 from table */
		idx = nand_ecc_precalc_table[dat[j]];
		reg1 ^= (idx & 0x3f);
		
		/* All bit XOR = 1 ? */
		if (idx & 0x40) {
			reg3 ^= (u_char) j;
			reg2 ^= ~((u_char) j);
		}
	}
	
	/* Create non-inverted ECC code from line parity */
	nand_trans_result(reg2, reg3, ecc_code);
	
	/* Calculate final ECC code */
	ecc_code[0] = ~ecc_code[0];
	ecc_code[1] = ~ecc_code[1];
	ecc_code[2] = ((~reg1) << 2) | 0x03;
	return 0;
}


/** 
 * nand_prepare_oobbuf - [GENERIC] Prepare the out of band buffer 
 * @fsbuf:	buffer given by fs driver
 * @oobsel:	out of band selection structre
 * @autoplace:	1 = place given buffer into the oob bytes
 * @numpages:	number of pages to prepare
 *
 * Return:
 * 1. Filesystem buffer available and autoplacement is off,
 *    return filesystem buffer
 * 2. No filesystem buffer or autoplace is off, return internal
 *    buffer
 * 3. Filesystem buffer is given and autoplace selected
 *    put data from fs buffer into internal buffer and
 *    retrun internal buffer
 *
 * Note: The internal buffer is filled with 0xff. This must
 * be done only once, when no autoplacement happens
 * Autoplacement sets the buffer dirty flag, which
 * forces the 0xff fill before using the buffer again.
 *
*/
static void nand_prepare_oobbuf (u_char *oob_buf, u_char *fs_buf, struct nand_oobinfo *oobsel)
{
	int i;

	for (i = 0; oobsel->oobfree[i][1]; i++) {
		int to = oobsel->oobfree[i][0];
		int num = oobsel->oobfree[i][1];
		memcpy (&oob_buf[to], fs_buf, num);
		fs_buf += num;
	}	
}

static const char invparity[256] = {
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
};
//typedef unsigned int		uint32_t;


/**
 * __nand_calculate_ecc - [NAND Interface] Calculate 3-byte ECC for 256/512-byte
 *			 block
 * @buf:	input buffer with raw data
 * @eccsize:	data bytes per ECC step (256 or 512)
 * @code:	output buffer with ECC
 */
void nand_calculate_ecc2(const unsigned char *buf, unsigned char *code)
{
	int i;
	unsigned int eccsize = 256;
	const uint32_t *bp = (uint32_t *)buf;
	/* 256 or 512 bytes/ecc  */
	const uint32_t eccsize_mult = eccsize >> 8;
	uint32_t cur;		/* current value in buffer */
	/* rp0..rp15..rp17 are the various accumulated parities (per byte) */
	uint32_t rp0, rp1, rp2, rp3, rp4, rp5, rp6, rp7;
	uint32_t rp8, rp9, rp10, rp11, rp12, rp13, rp14, rp15, rp16;
	uint32_t rp17;	/* to make compiler happy */
	uint32_t par;		/* the cumulative parity for all data */
	uint32_t tmppar;	/* the cumulative parity for this iteration;
				   for rp12, rp14 and rp16 at the end of the
				   loop */

	par = 0;
	rp4 = 0;
	rp6 = 0;
	rp8 = 0;
	rp10 = 0;
	rp12 = 0;
	rp14 = 0;
	rp16 = 0;

	/*
	 * The loop is unrolled a number of times;
	 * This avoids if statements to decide on which rp value to update
	 * Also we process the data by longwords.
	 * Note: passing unaligned data might give a performance penalty.
	 * It is assumed that the buffers are aligned.
	 * tmppar is the cumulative sum of this iteration.
	 * needed for calculating rp12, rp14, rp16 and par
	 * also used as a performance improvement for rp6, rp8 and rp10
	 */
	for (i = 0; i < eccsize_mult << 2; i++) {
		cur = *bp++;
		tmppar = cur;
		rp4 ^= cur;
		cur = *bp++;
		tmppar ^= cur;
		rp6 ^= tmppar;
		cur = *bp++;
		tmppar ^= cur;
		rp4 ^= cur;
		cur = *bp++;
		tmppar ^= cur;
		rp8 ^= tmppar;

		cur = *bp++;
		tmppar ^= cur;
		rp4 ^= cur;
		rp6 ^= cur;
		cur = *bp++;
		tmppar ^= cur;
		rp6 ^= cur;
		cur = *bp++;
		tmppar ^= cur;
		rp4 ^= cur;
		cur = *bp++;
		tmppar ^= cur;
		rp10 ^= tmppar;

		cur = *bp++;
		tmppar ^= cur;
		rp4 ^= cur;
		rp6 ^= cur;
		rp8 ^= cur;
		cur = *bp++;
		tmppar ^= cur;
		rp6 ^= cur;
		rp8 ^= cur;
		cur = *bp++;
		tmppar ^= cur;
		rp4 ^= cur;
		rp8 ^= cur;
		cur = *bp++;
		tmppar ^= cur;
		rp8 ^= cur;

		cur = *bp++;
		tmppar ^= cur;
		rp4 ^= cur;
		rp6 ^= cur;
		cur = *bp++;
		tmppar ^= cur;
		rp6 ^= cur;
		cur = *bp++;
		tmppar ^= cur;
		rp4 ^= cur;
		cur = *bp++;
		tmppar ^= cur;

		par ^= tmppar;
		if ((i & 0x1) == 0)
			rp12 ^= tmppar;
		if ((i & 0x2) == 0)
			rp14 ^= tmppar;
		if (eccsize_mult == 2 && (i & 0x4) == 0)
			rp16 ^= tmppar;
	}

	/*
	 * handle the fact that we use longword operations
	 * we'll bring rp4..rp14..rp16 back to single byte entities by
	 * shifting and xoring first fold the upper and lower 16 bits,
	 * then the upper and lower 8 bits.
	 */
	rp4 ^= (rp4 >> 16);
	rp4 ^= (rp4 >> 8);
	rp4 &= 0xff;
	rp6 ^= (rp6 >> 16);
	rp6 ^= (rp6 >> 8);
	rp6 &= 0xff;
	rp8 ^= (rp8 >> 16);
	rp8 ^= (rp8 >> 8);
	rp8 &= 0xff;
	rp10 ^= (rp10 >> 16);
	rp10 ^= (rp10 >> 8);
	rp10 &= 0xff;
	rp12 ^= (rp12 >> 16);
	rp12 ^= (rp12 >> 8);
	rp12 &= 0xff;
	rp14 ^= (rp14 >> 16);
	rp14 ^= (rp14 >> 8);
	rp14 &= 0xff;
	if (eccsize_mult == 2) {
		rp16 ^= (rp16 >> 16);
		rp16 ^= (rp16 >> 8);
		rp16 &= 0xff;
	}

	/*
	 * we also need to calculate the row parity for rp0..rp3
	 * This is present in par, because par is now
	 * rp3 rp3 rp2 rp2 in little endian and
	 * rp2 rp2 rp3 rp3 in big endian
	 * as well as
	 * rp1 rp0 rp1 rp0 in little endian and
	 * rp0 rp1 rp0 rp1 in big endian
	 * First calculate rp2 and rp3
	 */
#ifdef __BIG_ENDIAN
	rp2 = (par >> 16);
	rp2 ^= (rp2 >> 8);
	rp2 &= 0xff;
	rp3 = par & 0xffff;
	rp3 ^= (rp3 >> 8);
	rp3 &= 0xff;
#else
	rp3 = (par >> 16);
	rp3 ^= (rp3 >> 8);
	rp3 &= 0xff;
	rp2 = par & 0xffff;
	rp2 ^= (rp2 >> 8);
	rp2 &= 0xff;
#endif

	/* reduce par to 16 bits then calculate rp1 and rp0 */
	par ^= (par >> 16);
#ifdef __BIG_ENDIAN
	rp0 = (par >> 8) & 0xff;
	rp1 = (par & 0xff);
#else
	rp1 = (par >> 8) & 0xff;
	rp0 = (par & 0xff);
#endif

	/* finally reduce par to 8 bits */
	par ^= (par >> 8);
	par &= 0xff;

	/*
	 * and calculate rp5..rp15..rp17
	 * note that par = rp4 ^ rp5 and due to the commutative property
	 * of the ^ operator we can say:
	 * rp5 = (par ^ rp4);
	 * The & 0xff seems superfluous, but benchmarking learned that
	 * leaving it out gives slightly worse results. No idea why, probably
	 * it has to do with the way the pipeline in pentium is organized.
	 */
	rp5 = (par ^ rp4) & 0xff;
	rp7 = (par ^ rp6) & 0xff;
	rp9 = (par ^ rp8) & 0xff;
	rp11 = (par ^ rp10) & 0xff;
	rp13 = (par ^ rp12) & 0xff;
	rp15 = (par ^ rp14) & 0xff;
	if (eccsize_mult == 2)
		rp17 = (par ^ rp16) & 0xff;

	/*
	 * Finally calculate the ECC bits.
	 * Again here it might seem that there are performance optimisations
	 * possible, but benchmarks showed that on the system this is developed
	 * the code below is the fastest
	 */
#ifdef CONFIG_MTD_NAND_ECC_SMC
	code[0] =
	    (invparity[rp7] << 7) |
	    (invparity[rp6] << 6) |
	    (invparity[rp5] << 5) |
	    (invparity[rp4] << 4) |
	    (invparity[rp3] << 3) |
	    (invparity[rp2] << 2) |
	    (invparity[rp1] << 1) |
	    (invparity[rp0]);
	code[1] =
	    (invparity[rp15] << 7) |
	    (invparity[rp14] << 6) |
	    (invparity[rp13] << 5) |
	    (invparity[rp12] << 4) |
	    (invparity[rp11] << 3) |
	    (invparity[rp10] << 2) |
	    (invparity[rp9] << 1)  |
	    (invparity[rp8]);
#else
	code[1] =
	    (invparity[rp7] << 7) |
	    (invparity[rp6] << 6) |
	    (invparity[rp5] << 5) |
	    (invparity[rp4] << 4) |
	    (invparity[rp3] << 3) |
	    (invparity[rp2] << 2) |
	    (invparity[rp1] << 1) |
	    (invparity[rp0]);
	code[0] =
	    (invparity[rp15] << 7) |
	    (invparity[rp14] << 6) |
	    (invparity[rp13] << 5) |
	    (invparity[rp12] << 4) |
	    (invparity[rp11] << 3) |
	    (invparity[rp10] << 2) |
	    (invparity[rp9] << 1)  |
	    (invparity[rp8]);
#endif
	if (eccsize_mult == 1)
		code[2] =
		    (invparity[par & 0xf0] << 7) |
		    (invparity[par & 0x0f] << 6) |
		    (invparity[par & 0xcc] << 5) |
		    (invparity[par & 0x33] << 4) |
		    (invparity[par & 0xaa] << 3) |
		    (invparity[par & 0x55] << 2) |
		    3;
	else
		code[2] =
		    (invparity[par & 0xf0] << 7) |
		    (invparity[par & 0x0f] << 6) |
		    (invparity[par & 0xcc] << 5) |
		    (invparity[par & 0x33] << 4) |
		    (invparity[par & 0xaa] << 3) |
		    (invparity[par & 0x55] << 2) |
		    (invparity[rp17] << 1) |
		    (invparity[rp16] << 0);
}


static int write_chunk(__u8 *data, __u32 objId, __u32 chunkId, __u32 nBytes)
{
	yaffs_ExtendedTags t;
	yaffs_PackedTags2 pt;
	__u8 ecc_code[3];
	int i;

	error = write(outFile,data,chunkSize);
	if(error < 0) return error;

	yaffs_InitialiseTags(&t);
	
	t.chunkId = chunkId;
//	t.serialNumber = 0;
	t.serialNumber = 1;	// **CHECK**
	t.byteCount = nBytes;
	t.objectId = objId;
	
	t.sequenceNumber = YAFFS_LOWEST_SEQUENCE_NUMBER;

// added NCB **CHECK**
	t.chunkUsed = 1;

	if (convert_endian)
	{
    	    little_to_big_endian(&t);
	}

	nPages++;

	yaffs_PackTags2(&pt,&t);
	
//	return write(outFile,&pt,sizeof(yaffs_PackedTags2));

	memset(oob_buf, 0xff, sizeof(oob_buf));
	nand_prepare_oobbuf(oob_buf, (u_char *)&pt, &nand_oob_64);

	for (i = 0; i < chunkSize/256; i++) {
	    nand_calculate_ecc(data+i*256, ecc_code);
		oob_buf[nand_oob_64.eccpos[i*3]] = ecc_code[0];
		oob_buf[nand_oob_64.eccpos[i*3]+1] = ecc_code[1];
		oob_buf[nand_oob_64.eccpos[i*3]+2] = ecc_code[2];
	}
	
	return write(outFile,oob_buf,spareSize);
	
}

#define SWAP32(x)   ((((x) & 0x000000FF) << 24) | \
                     (((x) & 0x0000FF00) << 8 ) | \
                     (((x) & 0x00FF0000) >> 8 ) | \
                     (((x) & 0xFF000000) >> 24))

#define SWAP16(x)   ((((x) & 0x00FF) << 8) | \
                     (((x) & 0xFF00) >> 8))
        
// This one is easier, since the types are more standard. No funky shifts here.
static void object_header_little_to_big_endian(yaffs_ObjectHeader* oh)
{
    oh->type = SWAP32(oh->type); // GCC makes enums 32 bits.
    oh->parentObjectId = SWAP32(oh->parentObjectId); // int
    oh->sum__NoLongerUsed = SWAP16(oh->sum__NoLongerUsed); // __u16 - Not used, but done for completeness.
    // name = skip. Char array. Not swapped.
    oh->yst_mode = SWAP32(oh->yst_mode);
#ifdef CONFIG_YAFFS_WINCE // WinCE doesn't implement this, but we need to just in case. 
    // In fact, WinCE would be *THE* place where this would be an issue!
    oh->notForWinCE[0] = SWAP32(oh->notForWinCE[0]);
    oh->notForWinCE[1] = SWAP32(oh->notForWinCE[1]);
    oh->notForWinCE[2] = SWAP32(oh->notForWinCE[2]);
    oh->notForWinCE[3] = SWAP32(oh->notForWinCE[3]);
    oh->notForWinCE[4] = SWAP32(oh->notForWinCE[4]);
#else
    // Regular POSIX.
    oh->yst_uid = SWAP32(oh->yst_uid);
    oh->yst_gid = SWAP32(oh->yst_gid);
    oh->yst_atime = SWAP32(oh->yst_atime);
    oh->yst_mtime = SWAP32(oh->yst_mtime);
    oh->yst_ctime = SWAP32(oh->yst_ctime);
#endif

    oh->fileSize = SWAP32(oh->fileSize); // Aiee. An int... signed, at that!
    oh->equivalentObjectId = SWAP32(oh->equivalentObjectId);
    // alias  - char array.
    oh->yst_rdev = SWAP32(oh->yst_rdev);

#ifdef CONFIG_YAFFS_WINCE
    oh->win_ctime[0] = SWAP32(oh->win_ctime[0]);
    oh->win_ctime[1] = SWAP32(oh->win_ctime[1]);
    oh->win_atime[0] = SWAP32(oh->win_atime[0]);
    oh->win_atime[1] = SWAP32(oh->win_atime[1]);
    oh->win_mtime[0] = SWAP32(oh->win_mtime[0]);
    oh->win_mtime[1] = SWAP32(oh->win_mtime[1]);
    oh->roomToGrow[0] = SWAP32(oh->roomToGrow[0]);
    oh->roomToGrow[1] = SWAP32(oh->roomToGrow[1]);
    oh->roomToGrow[2] = SWAP32(oh->roomToGrow[2]);
    oh->roomToGrow[3] = SWAP32(oh->roomToGrow[3]);
    oh->roomToGrow[4] = SWAP32(oh->roomToGrow[4]);
    oh->roomToGrow[5] = SWAP32(oh->roomToGrow[5]);
#else
    oh->roomToGrow[0] = SWAP32(oh->roomToGrow[0]);
    oh->roomToGrow[1] = SWAP32(oh->roomToGrow[1]);
    oh->roomToGrow[2] = SWAP32(oh->roomToGrow[2]);
    oh->roomToGrow[3] = SWAP32(oh->roomToGrow[3]);
    oh->roomToGrow[4] = SWAP32(oh->roomToGrow[4]);
    oh->roomToGrow[5] = SWAP32(oh->roomToGrow[5]);
    oh->roomToGrow[6] = SWAP32(oh->roomToGrow[6]);
    oh->roomToGrow[7] = SWAP32(oh->roomToGrow[7]);
    oh->roomToGrow[8] = SWAP32(oh->roomToGrow[8]);
    oh->roomToGrow[9] = SWAP32(oh->roomToGrow[9]);
    oh->roomToGrow[10] = SWAP32(oh->roomToGrow[10]);
    oh->roomToGrow[11] = SWAP32(oh->roomToGrow[11]);
#endif
}

static int write_object_header(int objId, yaffs_ObjectType t, struct stat *s, int parent, const char *name, int equivalentObj, const char * alias)
{
	__u8 bytes[chunkSize];
	
	
	yaffs_ObjectHeader *oh = (yaffs_ObjectHeader *)bytes;
	
	memset(bytes,0xff,sizeof(bytes));
	
	oh->type = t;

	oh->parentObjectId = parent;
	
	strncpy(oh->name,name,YAFFS_MAX_NAME_LENGTH);
	
	
	if(t != YAFFS_OBJECT_TYPE_HARDLINK)
	{
		oh->yst_mode = s->st_mode;
		oh->yst_uid = s->st_uid;
// NCB 12/9/02		oh->yst_gid = s->yst_uid;
		oh->yst_gid = s->st_gid;
		oh->yst_atime = s->st_atime;
		oh->yst_mtime = s->st_mtime;
		oh->yst_ctime = s->st_ctime;
		oh->yst_rdev  = s->st_rdev;
	}
	
	if(t == YAFFS_OBJECT_TYPE_FILE)
	{
		oh->fileSize = s->st_size;
	}
	
	if(t == YAFFS_OBJECT_TYPE_HARDLINK)
	{
		oh->equivalentObjectId = equivalentObj;
	}
	
	if(t == YAFFS_OBJECT_TYPE_SYMLINK)
	{
		strncpy(oh->alias,alias,YAFFS_MAX_ALIAS_LENGTH);
	}

	if (convert_endian)
	{
    		object_header_little_to_big_endian(oh);
	}
	
	return write_chunk(bytes,objId,0,0xffff);
	
}


static int process_directory(int parent, const char *path)
{

	DIR *dir;
	struct dirent *entry;

	nDirectories++;
	
	dir = opendir(path);
	
	if(dir)
	{
		while((entry = readdir(dir)) != NULL)
		{
		
			/* Ignore . and .. */
			if(strcmp(entry->d_name,".") &&
			   strcmp(entry->d_name,".."))
 			{
 				char full_name[500];
				struct stat stats;
				int equivalentObj;
				int newObj;
				
				sprintf(full_name,"%s/%s",path,entry->d_name);
				
				lstat(full_name,&stats);
				
				if(S_ISLNK(stats.st_mode) ||
				    S_ISREG(stats.st_mode) ||
				    S_ISDIR(stats.st_mode) ||
				    S_ISFIFO(stats.st_mode) ||
				    S_ISBLK(stats.st_mode) ||
				    S_ISCHR(stats.st_mode) ||
				    S_ISSOCK(stats.st_mode))
				{
				
					newObj = obj_id++;
					nObjects++;
					
					printf("Object %d, %s is a ",newObj,full_name);
					
					/* We're going to create an object for it */
					if((equivalentObj = find_obj_in_list(stats.st_dev, stats.st_ino)) > 0)
					{
					 	/* we need to make a hard link */
					 	printf("hard link to object %d\n",equivalentObj);
						error =  write_object_header(newObj, YAFFS_OBJECT_TYPE_HARDLINK, &stats, parent, entry->d_name, equivalentObj, NULL);
					}
					else 
					{
						
						add_obj_to_list(stats.st_dev,stats.st_ino,newObj);
						
						if(S_ISLNK(stats.st_mode))
						{
					
							char symname[500];
						
							memset(symname,0, sizeof(symname));
					
							readlink(full_name,symname,sizeof(symname) -1);
						
							printf("symlink to \"%s\"\n",symname);
							error =  write_object_header(newObj, YAFFS_OBJECT_TYPE_SYMLINK, &stats, parent, entry->d_name, -1, symname);

						}
						else if(S_ISREG(stats.st_mode))
						{
							printf("file, ");
							error =  write_object_header(newObj, YAFFS_OBJECT_TYPE_FILE, &stats, parent, entry->d_name, -1, NULL);

							if(error >= 0)
							{
								int h;
								__u8 bytes[chunkSize];
								int nBytes;
								int chunk = 0;
								
								h = open(full_name,O_RDONLY);
								if(h >= 0)
								{
									memset(bytes,0xff,sizeof(bytes));
									while((nBytes = read(h,bytes,sizeof(bytes))) > 0)
									{
										chunk++;
										write_chunk(bytes,newObj,chunk,nBytes);
										memset(bytes,0xff,sizeof(bytes));
									}
									if(nBytes < 0) 
									   error = nBytes;
									   
									printf("%d data chunks written\n",chunk);
								}
								else
								{
									perror("Error opening file");
								}
								close(h);
								
							}							
														
						}
						else if(S_ISSOCK(stats.st_mode))
						{
							printf("socket\n");
							error =  write_object_header(newObj, YAFFS_OBJECT_TYPE_SPECIAL, &stats, parent, entry->d_name, -1, NULL);
						}
						else if(S_ISFIFO(stats.st_mode))
						{
							printf("fifo\n");
							error =  write_object_header(newObj, YAFFS_OBJECT_TYPE_SPECIAL, &stats, parent, entry->d_name, -1, NULL);
						}
						else if(S_ISCHR(stats.st_mode))
						{
							printf("character device\n");
							error =  write_object_header(newObj, YAFFS_OBJECT_TYPE_SPECIAL, &stats, parent, entry->d_name, -1, NULL);
						}
						else if(S_ISBLK(stats.st_mode))
						{
							printf("block device\n");
							error =  write_object_header(newObj, YAFFS_OBJECT_TYPE_SPECIAL, &stats, parent, entry->d_name, -1, NULL);
						}
						else if(S_ISDIR(stats.st_mode))
						{
							printf("directory\n");
							error =  write_object_header(newObj, YAFFS_OBJECT_TYPE_DIRECTORY, &stats, parent, entry->d_name, -1, NULL);
// NCB modified 10/9/2001				process_directory(1,full_name);
							process_directory(newObj,full_name);
						}
					}
				}
				else
				{
					printf(" we don't handle this type\n");
				}
			}
		}
	}
	
	return 0;

}


int main(int argc, char *argv[])
{
	struct stat stats;
	
	printf("mkyaffs2image: image building tool for YAFFS2 built "__DATE__"\n");
	
	if(argc < 3)
	{
		printf("usage: mkyaffs2image dir image_file [convert]\n");
		printf("           dir        the directory tree to be converted\n");
		printf("           image_file the output file to hold the image\n");
        printf("           'convert'  produce a big-endian image from a little-endian machine\n");
		exit(1);
	}

    if ((argc == 4) && (!strncmp(argv[3], "convert", strlen("convert"))))
    {
        convert_endian = 1;
    }
    
	if(stat(argv[1],&stats) < 0)
	{
		printf("Could not stat %s\n",argv[1]);
		exit(1);
	}
	
	if(!S_ISDIR(stats.st_mode))
	{
		printf(" %s is not a directory\n",argv[1]);
		exit(1);
	}
	
	outFile = open(argv[2],O_CREAT | O_TRUNC | O_WRONLY, S_IREAD | S_IWRITE);
	
	
	if(outFile < 0)
	{
		printf("Could not open output file %s\n",argv[2]);
		exit(1);
	}
	
	printf("Processing directory %s into image file %s\n",argv[1],argv[2]);
	error =  write_object_header(1, YAFFS_OBJECT_TYPE_DIRECTORY, &stats, 1,"", -1, NULL);
	if(error)
	error = process_directory(YAFFS_OBJECTID_ROOT,argv[1]);
	
	close(outFile);
	
	if(error < 0)
	{
		perror("operation incomplete");
		exit(1);
	}
	else
	{
		printf("Operation complete.\n"
		       "%d objects in %d directories\n"
		       "%d NAND pages\n",nObjects, nDirectories, nPages);
	}
	
	close(outFile);
	
	exit(0);
}	

