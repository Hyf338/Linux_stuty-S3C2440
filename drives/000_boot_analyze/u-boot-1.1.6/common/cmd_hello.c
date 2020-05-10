#include <common.h>
#include <watchdog.h>
#include <command.h>
#include <image.h>
#include <malloc.h>
#include <zlib.h>
#include <bzlib.h>
#include <environment.h>
#include <asm/byteorder.h>


int do_hello(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i;
	printf ("hello,world! , %d\n",argc);
	
	for(i = 0;i < argc ;i++)
	{
		printf("argv[%d]: %s\n",i , argv[i]);
	}
	
	return 0;
}

U_BOOT_CMD(
 	hello,	CFG_MAXARGS,	1,	do_hello,
 	"hello  -  just for test\n",
 	"hello,long help............\n"
);


