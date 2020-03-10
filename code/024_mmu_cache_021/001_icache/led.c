
#include "s3c2440_soc.h"

void delay(volatile int d)
{
	while (d--);
}

/* æ¯10msè¯¥å‡½æ•°è¢«è°ƒç”¨ä¸€æ¬¡ 
 * æ¯500msæ“ä½œä¸€ä¸‹LEDå®ç°è®¡æ•°
 */
void led_timer_irq(void)
{
	/* ç‚¹ç¯è®¡æ•° */
	static int timer_num = 0;
	static int cnt = 0;
	int tmp;

	timer_num++;
	if(timer_num < 50)
		return ;
	timer_num = 0;

	cnt++;

	tmp = ~cnt;
	tmp &= 7;
	GPFDAT &= ~(7<<4);
	GPFDAT |= (tmp<<4);
}

int led_init(void)
{
	/* è®¾ç½®GPFCONè®©GPF4/5/6é…ç½®ä¸ºè¾“å‡ºå¼•è„š */
	GPFCON &= ~((3<<8) | (3<<10) | (3<<12));
	GPFCON |=  ((1<<8) | (1<<10) | (1<<12));

	register_timer("led",led_timer_irq);
}


int led_test(void)
{
	int val = 0;  /* val: 0b000, 0b111 */
	int tmp;

	/* ÉèÖÃGPFCONÈÃGPF4/5/6ÅäÖÃÎªÊä³öÒı½Å */
	GPFCON &= ~((3<<8) | (3<<10) | (3<<12));
	GPFCON |=  ((1<<8) | (1<<10) | (1<<12));

	/* Ñ­»·µãÁÁ */
	while (1)
	{
		tmp = ~val;
		tmp &= 7;
		GPFDAT &= ~(7<<4);
		GPFDAT |= (tmp<<4);
		delay(100000);
		val++;
		if (val == 8)
			val =0;
		
	}

	return 0;
}



