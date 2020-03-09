
#include "../s3c2440_soc.h"

static void SPI_GOIO_Init(void)
{
	/* GPF1 OLED_CSn  output */
	GPFCON &= ~(3<<(1*2));
	GPFCON |= (1<<(1*2));

	/* GPG2 FLASH_CSn output
	 * GPG4 OLED_DC   output
 	 * GPG5 SPIMISO   input
 	 * GPG6 SPIMOSI   output
 	 * GPG7 SPICLK    output
	 */
	 GPGCON &= ~((3<<(2*2)) | (3<<(4*2)) | (3<<(5*2)) |(3<<(6*2)) | (3<<(7*2)));
	 GPGCON |= ((1<<(2*2)) | (1<<(4*2)) |(1<<(6*2)) | (1<<(7*2)));
}


void SPIInit(void)
{
	/* 初始化引脚 */
	SPI_GOIO_Init();
}


