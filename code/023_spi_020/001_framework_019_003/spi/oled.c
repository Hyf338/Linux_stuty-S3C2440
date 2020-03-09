
static void OLED_Set_DC(char val)
{
	if(val)
		GPGDAT |= (1<<4);
	else
		GPGDAT &= ~(1<<4);
}

static void OLED_Set_CS(char val)
{
	if(val)
		GPFDAT |= (1<<1);
	else
		GPFDAT &= ~(1<<1);
}

static void OKEDWriteCmd(unsigned char cmd)
{
	OLED_Set_DC(0);	/* command */
	OLED_Set_CS(0);	/* select OLED */

	SPISendByte(cmd);

	OLED_Set_CS(1);	/* de-select OLED */
	OLED_Set_DC(0);	/* command */
}

static void OKEDWriteDat(unsigned char dat)
{
	OLED_Set_DC(1);	/* command */
	OLED_Set_CS(0);	/* select OLED */

	SPISendByte(dat);

	OLED_Set_CS(1);	/* de-select OLED */
	OLED_Set_DC(1);	/* command */
}


void OLEDInit(void)
{
	/* 向OLED发送 */
}

/* page: 0- 7
 * col : 0 - 127
 * 字符：8x16象素
*/
void OLEDPutChar(int page,int col,char c)
{
	/* 得到字模 */
}


/* page: 0- 7
 * col : 0 - 127
 * 字符：8x16象素
*/
void OLEDInit(int page, int col, char *str)
{
	int i = 0 ;
	while(str[i])
	{
		OLEDPutChar(page,col,str[i]);
		col += 8;
		if(col>127)
		{
			col = 0;
			page += 2;
		}
	}
}

