@******************************************************************************
@ init.S
@ 功能：OpenOCD先将init.bin下载到Steppingstone(内部RAM)中，初始化SDRAM等
@       然后就可以将bootloader直接下载到SDRAM中运行
@******************************************************************************       

#include "cfg6410.h"
#include "s3c6410.h"

#define  DELAY_COUNT 700000

.text 
.global _start 
_start:
Reset: 
    mrs r0, CPSR
    bic r0, r0, #0x1f
    orr r0, r0, #0xd3
    msr CPSR_fc, r0     @ Supervisor mode, sets the I and F bits
 
cpu_init_crit:
    /*
     * flush v4 I/D caches
     */
    mov r0, #0
    mcr p15, 0, r0, c7, c7, 0 /* flush v3/v4 cache */
    mcr p15, 0, r0, c8, c7, 0 /* flush v4 TLB */
  
    /*
     * disable MMU stuff and caches
     */
    mrc p15, 0, r0, c1, c0, 0
    bic r0, r0, #0x00002300 @ clear bits 13, 9:8 (--V- --RS)
    bic r0, r0, #0x00000087 @ clear bits 7, 2:0 (B--- -CAM)
    orr r0, r0, #0x00000002 @ set bit 2 (A) Align
    orr r0, r0, #0x00001000 @ set bit 12 (I) I-Cache
    mcr p15, 0, r0, c1, c0, 0
  
    /* Peri port setup */
    ldr r0, =0x70000000
    orr r0, r0, #0x13
    mcr p15,0,r0,c15,c2,4       @ 256M(0x70000000-0x7fffffff)

lowlevel_init:
    ldr r0, =ELFIN_GPIO_BASE
	ldr r1, =0x55555555
	str r1, [r0, #MEM1DRVCON_OFFSET]
	
    /* Disable Watchdog */
    ldr r0, =0x7e000000   @0x7e004000
    orr r0, r0, #0x4000
    mov r1, #0
    str r1, [r0]

    @ External interrupt pending clear
    ldr r0, =(ELFIN_GPIO_BASE+EINTPEND_OFFSET)  /*EINTPEND*/
    ldr r1, [r0]
    str r1, [r0]
    
    ldr r0, =ELFIN_VIC0_BASE_ADDR   @0x71200000
    ldr r1, =ELFIN_VIC1_BASE_ADDR   @0x71300000
    
    @ Disable all interrupts (VIC0 and VIC1)
    mvn r3, #0x0
    str r3, [r0, #oINTMSK]
    str r3, [r1, #oINTMSK]
    
    @ Set all interrupts as IRQ
    mov r3, #0x0
    str r3, [r0, #oINTMOD]
    str r3, [r1, #oINTMOD]
    
    @ Pending Interrupt Clear
    mov r3, #0x0
    str r3, [r0, #oVECTADDR]
    str r3, [r1, #oVECTADDR]
    
    /* init system clock */
    bl system_clock_init
    
    bl  mem_ctrl_asm_init

leds_flicker:    
	/* LED0~4: GPM0~4 */
	ldr r0, =ELFIN_GPIO_BASE
	ldr r1, =0x1111
	str r1, [r0, #GPMCON_OFFSET]

	ldr r1, =0xf
	str r1, [r0, #GPMDAT_OFFSET]
	
	mov  r5, r0
	mov  r4, #0  @ 0x0
        
led_loop:
    ldr r0, =DELAY_COUNT
    bl  wait

    mov r3, r4
    add r4, r4, #1      @ 0x1
    cmp r4, #16         @ 0x10
    subeq r4, r4, #16   @ 0x10
    str r3, [r5, #GPMDAT_OFFSET]
    b led_loop

wait:
    subs r0, r0, #1
    bne wait
    mov pc, lr
        
/*
 * system_clock_init: Initialize core clock and bus clock.
 * void system_clock_init(void)
 */
system_clock_init:
    ldr r0, =ELFIN_CLOCK_POWER_BASE @0x7e00f000

#ifdef  CONFIG_SYNC_MODE
    ldr r1, [r0, #OTHERS_OFFSET]
    mov r2, #0x40
    orr r1, r1, r2
    str r1, [r0, #OTHERS_OFFSET]

    nop
    nop
    nop
    nop
    nop

    ldr r2, =0x80
    orr r1, r1, r2
    str r1, [r0, #OTHERS_OFFSET]

check_syncack:
    ldr r1, [r0, #OTHERS_OFFSET]
    ldr r2, =0xf00
    and r1, r1, r2
    cmp r1, #0xf00
    bne check_syncack
#else   /* ASYNC Mode */
    nop
    nop
    nop
    nop
    nop

    ldr r1, [r0, #OTHERS_OFFSET]
    bic r1, r1, #0xC0
    orr r1, r1, #0x40
    str r1, [r0, #OTHERS_OFFSET]

wait_for_async:
    ldr r1, [r0, #OTHERS_OFFSET]
    and r1, r1, #0xf00
    cmp r1, #0x0
    bne wait_for_async

    ldr r1, [r0, #OTHERS_OFFSET]
    bic r1, r1, #0x40
    str r1, [r0, #OTHERS_OFFSET]
#endif

    mov r1, #0xff00
    orr r1, r1, #0xff
    str r1, [r0, #APLL_LOCK_OFFSET]
    str r1, [r0, #MPLL_LOCK_OFFSET]
    str r1, [r0, #EPLL_LOCK_OFFSET]
/* CLKUART(=66.5Mhz) = CLKUART_input(532/2=266Mhz) / (UART_RATIO(3)+1) */
/* CLKUART(=50Mhz) = CLKUART_input(400/2=200Mhz) / (UART_RATIO(3)+1) */
/* Now, When you use UART CLK SRC by EXT_UCLK1, We support 532MHz & 400MHz value */

#if defined(CONFIG_CLKSRC_CLKUART)
    ldr     r1, [r0, #CLK_DIV2_OFFSET]
    bic r1, r1, #0x70000
    orr r1, r1, #0x30000
    str r1, [r0, #CLK_DIV2_OFFSET]
#endif


    ldr     r1, [r0, #CLK_DIV0_OFFSET]  /*Set Clock Divider*/
    bic r1, r1, #0x30000
    bic r1, r1, #0xff00
    bic r1, r1, #0xff
    ldr r2, =CLK_DIV_VAL
    orr r1, r1, r2
    str r1, [r0, #CLK_DIV0_OFFSET]

    ldr r1, =APLL_VAL
    str r1, [r0, #APLL_CON_OFFSET]
    ldr r1, =MPLL_VAL
    str r1, [r0, #MPLL_CON_OFFSET]

    ldr r1, =0x80200203         /* FOUT of EPLL is 96MHz */
    str r1, [r0, #EPLL_CON0_OFFSET]
    ldr r1, =0x0
    str r1, [r0, #EPLL_CON1_OFFSET]

    ldr r1, [r0, #CLK_SRC_OFFSET]   /* APLL, MPLL, EPLL select to Fout */

#if defined(CONFIG_CLKSRC_CLKUART)
    ldr r2, =0x2007
#else
    ldr r2, =0x7
#endif
    orr r1, r1, r2

    str r1, [r0, #CLK_SRC_OFFSET]

    /* wait at least 200us to stablize all clock */
    mov r1, #0x10000
1:  subs    r1, r1, #1
    bne 1b
#if 0
    mrc p15, 0, r0, c1, c0, 0
    orr r0, r0, #0xc0000000 /* clock setting in MMU */
    mcr p15, 0, r0, c1, c0, 0
#endif

#ifdef CONFIG_SYNC_MODE             /* Synchronization for VIC port */
    ldr r1, [r0, #OTHERS_OFFSET]
    orr r1, r1, #0x20
    str r1, [r0, #OTHERS_OFFSET]
#else
    ldr r1, [r0, #OTHERS_OFFSET]
    bic r1, r1, #0x20
    str r1, [r0, #OTHERS_OFFSET]
#endif
    mov pc, lr

mem_ctrl_asm_init:
    ldr r0, =ELFIN_MEM_SYS_CFG          @Memory sussystem address 0x7e00f120
    mov r1, #0xd                @ Xm0CSn2 = NFCON CS0, Xm0CSn3 = NFCON CS1
    str r1, [r0]

    ldr r0, =ELFIN_DMC1_BASE            @DMC1 base address 0x7e001000

    ldr r1, =0x04
    str r1, [r0, #INDEX_DMC_MEMC_CMD]

    ldr r1, =DMC_DDR_REFRESH_PRD
    str r1, [r0, #INDEX_DMC_REFRESH_PRD]

    ldr r1, =DMC_DDR_CAS_LATENCY
    str r1, [r0, #INDEX_DMC_CAS_LATENCY]

    ldr r1, =DMC_DDR_t_DQSS
    str r1, [r0, #INDEX_DMC_T_DQSS]

    ldr r1, =DMC_DDR_t_MRD
    str r1, [r0, #INDEX_DMC_T_MRD]

    ldr r1, =DMC_DDR_t_RAS
    str r1, [r0, #INDEX_DMC_T_RAS]

    ldr r1, =DMC_DDR_t_RC
    str r1, [r0, #INDEX_DMC_T_RC]

    ldr r1, =DMC_DDR_t_RCD
    ldr r2, =DMC_DDR_schedule_RCD
    orr r1, r1, r2
    str r1, [r0, #INDEX_DMC_T_RCD]

    ldr r1, =DMC_DDR_t_RFC
    ldr r2, =DMC_DDR_schedule_RFC
    orr r1, r1, r2
    str r1, [r0, #INDEX_DMC_T_RFC]

    ldr r1, =DMC_DDR_t_RP
    ldr r2, =DMC_DDR_schedule_RP
    orr r1, r1, r2
    str r1, [r0, #INDEX_DMC_T_RP]

    ldr r1, =DMC_DDR_t_RRD
    str r1, [r0, #INDEX_DMC_T_RRD]

    ldr r1, =DMC_DDR_t_WR
    str r1, [r0, #INDEX_DMC_T_WR]

    ldr r1, =DMC_DDR_t_WTR
    str r1, [r0, #INDEX_DMC_T_WTR]

    ldr r1, =DMC_DDR_t_XP
    str r1, [r0, #INDEX_DMC_T_XP]

    ldr r1, =DMC_DDR_t_XSR
    str r1, [r0, #INDEX_DMC_T_XSR]

    ldr r1, =DMC_DDR_t_ESR
    str r1, [r0, #INDEX_DMC_T_ESR]

    ldr r1, =DMC1_MEM_CFG
    str r1, [r0, #INDEX_DMC_MEMORY_CFG]

    ldr r1, =DMC1_MEM_CFG2
    str r1, [r0, #INDEX_DMC_MEMORY_CFG2]

    ldr r1, =DMC1_CHIP0_CFG
    str r1, [r0, #INDEX_DMC_CHIP_0_CFG]

    ldr r1, =DMC_DDR_32_CFG
    str r1, [r0, #INDEX_DMC_USER_CONFIG]

    @DMC0 DDR Chip 0 configuration direct command reg
    ldr r1, =DMC_NOP0
    str r1, [r0, #INDEX_DMC_DIRECT_CMD]

    @Precharge All
    ldr r1, =DMC_PA0
    str r1, [r0, #INDEX_DMC_DIRECT_CMD]

    @Auto Refresh   2 time
    ldr r1, =DMC_AR0
    str r1, [r0, #INDEX_DMC_DIRECT_CMD]
    str r1, [r0, #INDEX_DMC_DIRECT_CMD]

    @MRS
    ldr r1, =DMC_mDDR_EMR0
    str r1, [r0, #INDEX_DMC_DIRECT_CMD]

    @Mode Reg
    ldr r1, =DMC_mDDR_MR0
    str r1, [r0, #INDEX_DMC_DIRECT_CMD]

#ifdef CONFIG_SMDK6410_X5A
    ldr r1, =DMC1_CHIP1_CFG
    str r1, [r0, #INDEX_DMC_CHIP_1_CFG]

    @DMC0 DDR Chip 0 configuration direct command reg
    ldr r1, =DMC_NOP1
    str r1, [r0, #INDEX_DMC_DIRECT_CMD]

    @Precharge All
    ldr r1, =DMC_PA1
    str r1, [r0, #INDEX_DMC_DIRECT_CMD]

    @Auto Refresh   2 time
    ldr r1, =DMC_AR1
    str r1, [r0, #INDEX_DMC_DIRECT_CMD]
    str r1, [r0, #INDEX_DMC_DIRECT_CMD]

    @MRS
    ldr r1, =DMC_mDDR_EMR1
    str r1, [r0, #INDEX_DMC_DIRECT_CMD]

    @Mode Reg
    ldr r1, =DMC_mDDR_MR1
    str r1, [r0, #INDEX_DMC_DIRECT_CMD]
#endif

    @Enable DMC1
    mov r1, #0x0
    str r1, [r0, #INDEX_DMC_MEMC_CMD]

check_dmc1_ready:
    ldr r1, [r0, #INDEX_DMC_MEMC_STATUS]
    mov r2, #0x3
    and r1, r1, r2
    cmp r1, #0x1
    bne check_dmc1_ready
    nop
    mov pc, lr


