#include "adc.h"

void adc_test(void)
{
	int val;
	double vol;
	int m;  /* 整数部分 */
	int n;  /* 小数部分 */
	
	adc_init();

	while(1)
	{
		val = adc_read_ain0();
		val =  (double) val /1023 * 3.3 ;  /* 1023-----3.3v */
		m = (int)vol;
		vol = vol - m;  /* 小数部分：0.3 */
		n = vol * 1000;  /* 300 */

		/* 在串口上打印 */
		printf("vol: %d.%03dv\r",m,n);	/* 3.300v */

		/* 在LCD上打印 */
		//fb_print_string();
	}
}

