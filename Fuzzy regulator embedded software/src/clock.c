#include "MDR32FxQI_rst_clk.h"
#include "MDR32FxQI_timer.h"
#include "MDR32F9Q2I.h"
#include "MDR32FxQI_port.h"
#include "stdint.h"
#include "fuzzy_regulator.h"

extern struct __fuzzy_regulator fuzzyRegulator;

static void initDebugLED(void)
{
	PORT_InitTypeDef port;

	RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTA, ENABLE);
	PORT_StructInit(&port);
	
	port.PORT_Pin = PORT_Pin_2;
	port.PORT_FUNC = PORT_FUNC_PORT;
	port.PORT_MODE = PORT_MODE_DIGITAL;
	port.PORT_OE = PORT_OE_OUT;
	port.PORT_PULL_UP = PORT_PULL_UP_OFF;
	port.PORT_PULL_DOWN = PORT_PULL_DOWN_OFF;
	port.PORT_SPEED = PORT_SPEED_FAST;
	PORT_Init(MDR_PORTA, &port);

	PORT_ResetBits(MDR_PORTA, PORT_Pin_2);

	return;
}
    
void config_ext_clk() {

	RST_CLK_CPU_PLLconfig(RST_CLK_CPU_PLLsrcHSIdiv1, RST_CLK_CPU_PLLmul10);
	RST_CLK_CPU_PLLcmd(ENABLE);
	while(RST_CLK_CPU_PLLstatus() != SUCCESS);
	RST_CLK_CPU_PLLuse(ENABLE);

	RST_CLK_CPUclkSelection(RST_CLK_CPUclkCPU_C3);
}

void step_timer_init()
{
	TIMER_CntInitTypeDef sTIM1CntInit;
	TIMER_DeInit(MDR_TIMER1);

	// Enable clocking for TIMER1 perihireal
	RST_CLK_PCLKcmd(RST_CLK_PCLK_TIMER1, ENABLE);

	// Configure TIMER1 settings for period 1us
	sTIM1CntInit.TIMER_Prescaler = 9;	// resFreq = PCLK / (TIMER_HCLKdivX * (TIMER_Prescaler + 1) * Period)
	sTIM1CntInit.TIMER_IniCounter = 0;
	sTIM1CntInit.TIMER_Period = 1;
	sTIM1CntInit.TIMER_CounterMode = TIMER_CntMode_ClkFixedDir;
	sTIM1CntInit.TIMER_CounterDirection = TIMER_CntDir_Up;
	sTIM1CntInit.TIMER_EventSource = TIMER_EvSrc_TIM_CLK;
	sTIM1CntInit.TIMER_FilterSampling = TIMER_FDTS_TIMER_CLK_div_1;
	sTIM1CntInit.TIMER_ARR_UpdateMode = TIMER_ARR_Update_Immediately;
	sTIM1CntInit.TIMER_ETR_FilterConf = TIMER_Filter_1FF_at_TIMER_CLK;
	sTIM1CntInit.TIMER_ETR_Prescaler = TIMER_ETR_Prescaler_None;
	sTIM1CntInit.TIMER_ETR_Polarity = TIMER_ETRPolarity_NonInverted;
	sTIM1CntInit.TIMER_BRK_Polarity = TIMER_BRKPolarity_NonInverted;
	TIMER_CntInit(MDR_TIMER1, &sTIM1CntInit);

	// Enable TIM2 interrupt when CNT == ARR
	TIMER_ITConfig(MDR_TIMER1, TIMER_STATUS_CNT_ARR, ENABLE);

	// Globally init interrupt vector
	NVIC_EnableIRQ(Timer1_IRQn);

	// Set TIM/CNT1 clock from HCLK with prescaler of 8
	TIMER_BRGInit(MDR_TIMER1, TIMER_HCLKdiv8);

	// Enable TII/CNT2
	//TIMER_Cmd(MDR_TIMER2, ENABLE);
}

void step_timer_update_step(uint16_t step_us)
{
	TIMER_SetCntAutoreload(MDR_TIMER1, step_us);
}

void step_timer_restart()
{
	/* Enable the timer with the timeout passed to xMBPortTimersInit( ) */
	TIMER_Cmd(MDR_TIMER1, DISABLE);
	TIMER_SetCounter(MDR_TIMER1, 0);
	TIMER_Cmd(MDR_TIMER1, ENABLE);
}

void step_timer_stop()
{
	TIMER_Cmd(MDR_TIMER1, DISABLE);
	TIMER_SetCounter(MDR_TIMER1, 0);
}

void TIMER1_IRQHandler()
{
	// Clear CNT, STATUS 
	// Then execute your code
	// don't stop timer until regulator is ON
	
	MDR_TIMER1->STATUS &= ~TIMER_IE_CNT_ARR_EVENT_IE;
	step_timer_stop();
	step_timer_update_step(*fuzzyRegulator.REG_CYCLE_DUR);
	step_timer_restart();
	fuzzyRegulator.is_next_step = 1;
}

int sys_timer_init(uint32_t quant_us)
{
	// Check if external clock source is used
	uint8_t clkSource = (MDR_RST_CLK->CPU_CLOCK >> RST_CLK_CPU_CLOCK_HCLK_SEL_Pos) & (uint32_t) 0x03;
	if (clkSource != 1)
		return -1;

	// Assumed that 80 MHz is used as HCLK
	SysTick->CTRL = (1 << SysTick_CTRL_CLKSOURCE_Pos) |
			(1 << SysTick_CTRL_TICKINT_Pos);
	SysTick->LOAD = (*fuzzyRegulator.REG_CYCLE_DUR * 80) & (0x00FFFFFF);
	NVIC_EnableIRQ(SysTick_IRQn);

	return 0;
}

void sys_timer_start()
{
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

void sys_timer_stop()
{
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Handler()
{
	sys_timer_stop();
	SysTick->LOAD = (*fuzzyRegulator.REG_CYCLE_DUR * 80) & (0x00FFFFFF);
	sys_timer_start();
	fuzzyRegulator.is_next_step = 1;
}