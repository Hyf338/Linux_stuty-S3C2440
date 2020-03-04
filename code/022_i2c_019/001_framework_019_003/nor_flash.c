#include "my_printf.h"
#include "string_utils.h"


#define NOR_FLASH_BASE  0  /* jz2440, nor-->cs0, base addr = 0 */


/* æ¯”å¦‚:   55H 98 
 * æœ¬æ„æ˜¯: å¾€(0 + (0x55)<<1)å†™å…¥0x98
 */
void nor_write_word(unsigned int base, unsigned int offset, unsigned int val)
{
	volatile unsigned short *p = (volatile unsigned short *)(base + (offset << 1));
	*p = val;
}

/* offsetæ˜¯åŸºäºNORçš„è§’åº¦çœ‹åˆ° */
void nor_cmd(unsigned int offset, unsigned int cmd)
{
	nor_write_word(NOR_FLASH_BASE, offset, cmd);
}

unsigned int nor_read_word(unsigned int base, unsigned int offset)
{
	volatile unsigned short *p = (volatile unsigned short *)(base + (offset << 1));
	return *p;
}

unsigned int nor_dat(unsigned int offset)
{
	return nor_read_word(NOR_FLASH_BASE, offset);
}


void wait_ready(unsigned int addr)
{
	unsigned int val;
	unsigned int pre;

	pre = nor_dat(addr>>1);
	val = nor_dat(addr>>1);
	while ((val & (1<<6)) != (pre & (1<<6)))
	{
		pre = val;
		val = nor_dat(addr>>1);		
	}
}


/* ½øÈëNOR FLASHµÄCFIÄ£Ê½
 * ¶ÁÈ¡¸÷ÀàĞÅÏ¢
 */
void do_scan_nor_flash(void)
{
	char str[4];
	unsigned int size;
	int regions, i;
	int region_info_base;
	int block_addr, blocks, block_size, j;
	int cnt;

	int vendor, device;
	
	/* ´òÓ¡³§¼ÒID¡¢Éè±¸ID */
	nor_cmd(0x555, 0xaa);    /* ½âËø */
	nor_cmd(0x2aa, 0x55); 
	nor_cmd(0x555, 0x90);    /* read id */
	vendor = nor_dat(0);
	device = nor_dat(1);
	nor_cmd(0, 0xf0);        /* reset */
	
	nor_cmd(0x55, 0x98);  /* ½øÈëcfiÄ£Ê½ */

	str[0] = nor_dat(0x10);
	str[1] = nor_dat(0x11);
	str[2] = nor_dat(0x12);
	str[3] = '\0';
	printf("str = %s\n\r", str);

	/* ´òÓ¡ÈİÁ¿ */
	size = 1<<(nor_dat(0x27));
	printf("vendor id = 0x%x, device id = 0x%x, nor size = 0x%x, %dM\n\r", vendor, device, size, size/(1024*1024));

	/* ´òÓ¡¸÷¸öÉÈÇøµÄÆğÊ¼µØÖ· */
	/* Ãû´Ê½âÊÍ:
	 *    erase block region : ÀïÃæº¬ÓĞ1¸ö»ò¶à¸öblock, ËüÃÇµÄ´óĞ¡Ò»Ñù
	 * Ò»¸önor flashº¬ÓĞ1¸ö»ò¶à¸öregion
	 * Ò»¸öregionº¬ÓĞ1¸ö»ò¶à¸öblock(ÉÈÇø)

	 * Erase block region information:
	 *    Ç°2×Ö½Ú+1    : ±íÊ¾¸ÃregionÓĞ¶àÉÙ¸öblock 
	 *    ºó2×Ö½Ú*256  : ±íÊ¾blockµÄ´óĞ¡
	 */

	regions = nor_dat(0x2c);
	region_info_base = 0x2d;
	block_addr = 0;
	printf("Block/Sector start Address:\n\r");
	cnt = 0;
	for (i = 0; i < regions; i++)
	{
		blocks = 1 + nor_dat(region_info_base) + (nor_dat(region_info_base+1)<<8);
		block_size = 256 * (nor_dat(region_info_base+2) + (nor_dat(region_info_base+3)<<8));
		region_info_base += 4;

//		printf("\n\rregion %d, blocks = %d, block_size = 0x%x, block_addr = 0x%x\n\r", i, blocks, block_size, block_addr);

		for (j = 0; j < blocks; j++)
		{
			/* ´òÓ¡Ã¿¸öblockµÄÆğÊ¼µØÖ· */
			//printf("0x%08x ", block_addr);
			printHex(block_addr);
			putchar(' ');
			cnt++;
			block_addr += block_size;
			if (cnt % 5 == 0)
				printf("\n\r");
		}
	}
	printf("\n\r");
	/* ÍË³öCFIÄ£Ê½ */
	nor_cmd(0, 0xf0);
}

void do_erase_nor_flash(void)
{
	unsigned int addr;
	
	/* »ñµÃµØÖ· */
	printf("Enter the address of sector to erase: ");
	addr = get_uint();

	printf("erasing ...\n\r");
	nor_cmd(0x555, 0xaa);    /* ½âËø */
	nor_cmd(0x2aa, 0x55); 
	nor_cmd(0x555, 0x80);	 /* erase sector */
	
	nor_cmd(0x555, 0xaa);    /* ½âËø */
	nor_cmd(0x2aa, 0x55); 
	nor_cmd(addr>>1, 0x30);	 /* ·¢³öÉÈÇøµØÖ· */
	wait_ready(addr);
}

void do_write_nor_flash(void)
{
	unsigned int addr;
	unsigned char str[100];
	int i, j;
	unsigned int val;
	
	/* è·å¾—åœ°å€Ö· */
	printf("Enter the address of sector to write: ");
	addr = get_uint();

	printf("Enter the string to write: ");
	gets(str);

	printf("writing ...\n\r");

	/* str[0],str[1]==>16bit 
	 * str[2],str[3]==>16bit 
	 */
	i = 0;
	j = 1;
	while (str[i] && str[j])
	{
		val = str[i] + (str[j]<<8);
		
		/* çƒ§å†™ */
		nor_cmd(0x555, 0xaa);	 /* è§£é” */
		nor_cmd(0x2aa, 0x55); 
		nor_cmd(0x555, 0xa0);	 /* program */
		nor_cmd(addr>>1, val);
		/* ç­‰å¾…çƒ§å†™å®Œæˆï¼šè¯»æ•°æ®ï¼ŒQ6æ— å˜åŒ–æ—¶è¡¨ç¤ºç»“æŸ */
		wait_ready(addr);

		i += 2;
		j += 2;
		addr += 2;
	}  

	val = str[i];
	/* çƒ§å†™ */
	nor_cmd(0x555, 0xaa);	 /* è§£é” */
	nor_cmd(0x2aa, 0x55); 
	nor_cmd(0x555, 0xa0);	 /* program */
	nor_cmd(addr>>1, val);
	/* ç­‰å¾…çƒ§å†™å®Œæˆï¼šè¯»æ•°æ®ï¼ŒQ6æ— å˜åŒ– */
	wait_ready(addr);
}
void do_read_nor_flash(void)
{
	unsigned int addr;
	volatile unsigned char *p;
	int i, j;
	unsigned char c;
	unsigned char str[16];
	
	/*è·å¾—åœ°å€Ö· */
	printf("Enter the address to read: ");
	addr = get_uint();

	p = (volatile unsigned char *)addr;

	printf("Data : \n\r");
	/* é•¿åº¦å›ºå®šä¸º64 */
	for (i = 0; i < 4; i++)
	{
		/* æ¯è¡Œæ‰“å°16ä¸ªæ•°æ® */
		for (j = 0; j < 16; j++)
		{
			/* å…ˆæ‰“å°æ•°å€¼ */
			c = *p++;
			str[j] = c;
			printf("%02x ", c);
		}

		printf("   ; ");

		for (j = 0; j < 16; j++)
		{
			/* åæ‰“å°å­—ç¬¦ */
			if (str[j] < 0x20 || str[j] > 0x7e)   /* ä¸å¯è§†å­—ç¬¦ */
				putchar('.');
			else
				putchar(str[j]);
		}
		printf("\n\r");
	}
}

void nor_flash_test(void)
{
	char c;

	while (1)
	{
		/* æ‰“å°èœå•ï¼Œä¾›æˆ‘ä»¬é€‰æ‹©æµ‹è¯•å†…å®¹ */
		printf("[s] Scan nor flash\n\r");
		printf("[e] Erase nor flash\n\r");
		printf("[w] Write nor flash\n\r");
		printf("[r] Read nor flash\n\r");
		printf("[q] quit\n\r");
		printf("Enter selection: ");

		c = getchar();
		printf("%c\n\r", c);

		/* æµ‹è¯•å†…å®¹:
		 * 1. è¯†åˆ«nor flash
		 * 2. æ“¦é™¤nor flashæŸä¸ªæ‰‡åŒº
		 * 3. ç¼–å†™æŸä¸ªåœ°å€
		 * 4. è¯»æŸä¸ªåœ°å€
		 */
		switch (c)		 
		{
			case 'q':
			case 'Q':
				return;
				break;
				
			case 's':
			case 'S':
				do_scan_nor_flash();
				break;

			case 'e':
			case 'E':
				do_erase_nor_flash();
				break;

			case 'w':
			case 'W':
				do_write_nor_flash();
				break;

			case 'r':
			case 'R':
				do_read_nor_flash();
				break;
			default:
				break;
		}
	}
}

