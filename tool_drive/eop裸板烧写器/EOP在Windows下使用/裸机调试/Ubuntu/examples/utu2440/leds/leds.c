
#define	GPFCON		(*(volatile unsigned long *)0x56000050)
#define	GPFDAT		(*(volatile unsigned long *)0x56000054)

#define	GPF4_out	(1<<(4*2))
#define	GPF5_out	(1<<(5*2))
#define	GPF6_out	(1<<(6*2))
#define	GPF7_out	(1<<(7*2))

void  wait(volatile unsigned long dly)
{
	for(; dly > 0; dly--);
}

int main(void)
{
	unsigned long i = 0;

	GPFCON = GPF4_out|GPF5_out|GPF6_out|GPF7_out;		// ��LED1-4��Ӧ��GPF4/5/6/7�ĸ�������Ϊ���

	while(1){
		wait(30000);
		GPFDAT = (~(i<<4));	 	// ����i��ֵ������LED1-4
		if(++i == 16)
			i = 0;
	}

	return 0;
}
