
#define MMU_SECDESC_AP          (3<<10)
#define MMU_SECDESC_DOMAIN      (0<<5)
#define MMU_SECDESC_NCNB        (0<<2)
#define MMU_SECDESC_WB          (3<<2)
#define MMU_SECDESC_TYPE        ((1<<4) | (1<<1))

#define MMU_SECDESC_FOR_IO      (MMU_SECDESC_AP | MMU_SECDESC_DOMAIN | MMU_SECDESC_NCNB | MMU_SECDESC_TYPE)
#define MMU_SECDESC_FOR_MEM      (MMU_SECDESC_AP | MMU_SECDESC_DOMAIN | MMU_SECDESC_WB | MMU_SECDESC_TYPE)

#define IO 1
#define MEM 0

void creat_secdesc(	unsigned int *ttb,unsigned int va,unsigned int pa,int io)
{
	int index;

	index = va/0x100000;

	if(io)
		ttb[index] = (pa & 0xfff00000) | MMU_SECDESC_FOR_IO;
	else 
		ttb[index] = (pa & 0xfff00000) | MMU_SECDESC_FOR_MEM;
}

/* 创建一级页表
 *    VA          PA           CB
 *    0           0            11
 *    0x40000000  0x40000000   11
 *
 *    64M sdram
 *    0x30000000  0x30000000   11
 *    ...
 *    0x33f00000  0x33f00000   11
 *
 *    register:   0x48000000~0x5b00001c
 *    0x48000000  0x48000000   00
 *    ...
 *    0x5b000000  0x5b000000   00
 *    
 *    framebuffer  : 0x33c00000
 *    0x33c00000  0x33c00000   00
 *    
 *    link address
 *    0xB0000000  0x30000000  11
 */
void create_page_table(void)
{
	/* 1.页表在哪？   0x32000000(占据16kb) */
	/* ttb: translation table base */
	unsigned int *ttb = (unsigned int *)0x32000000;
	unsigned int va,pa;
	int index;

	/* 2.根据va，pa设置页表条目 */

	/* 2.1 for sram/nor flash */
	creat_secdesc(ttb, 0, 0, MEM);
	
	/* 2.2 for sram when nor boot */
	creat_secdesc(ttb, 0x40000000, 0x40000000, MEM);

	/* 2.3 for 64k sdram */
	va = 0x30000000;
	pa = 0x30000000;
	for(va = 0x30000000; va < 0x30000000;)
	{
		creat_secdesc(ttb, va, pa, MEM);
		va += 0x100000;
		pa += 0x100000;
	}

	/* 2.4 for register:   0x48000000~0x5b00001c */
	va = 0x48000000;
	pa = 0x48000000;
	for( ; va <= 0x5b000000; )
	{
		creat_secdesc(ttb, va, pa, IO);
		va += 0x100000;
		pa += 0x100000;
	}

	/* 2.5 for  framebuffer  : 0x33c00000 */
	creat_secdesc(ttb, 0x33c00000, 0x33c00000, IO);

	/* 2.6  link address  */
	creat_secdesc(ttb, 0xb0000000, 0x30000000, IO);
}



