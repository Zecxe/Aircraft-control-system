#include "MDR32FxQI_i2c.h"
#include "MDR32FxQI_port.h"
#include "MDR32FxQI_rst_clk.h"
#include "MDR32FxQI_timer.h"
#include "motor_control.h"
#include "clock.h"
#include "fuzzy_regulator.h"

#define TIMER2_CLK	80000	// kHz
#define PWM_FREQ	50	// kHz
#define PWM_PERIOD	1600	// TIMER2_CLK/PWM_FREQ
#define INPUT_VOLTAGE	27	// 27V max voltage
#define VOLT_TO_CNT	59	// (int) 1600/27

static enum motorMode   motor1Mode;
static enum motorMode   motor2Mode;
static enum motorState  motor1State;
static enum motorState  motor2State;

static void __PORT_Init(MDR_PORT_TypeDef* PORTx, const PORT_InitTypeDef* PORT_InitStruct)
{
        uint32_t tmpreg_OE;
        uint32_t tmpreg_FUNC;
        uint32_t tmpreg_ANALOG;
        uint32_t tmpreg_PULL;
        uint32_t tmpreg_PD;
        uint32_t tmpreg_PWR;
        uint32_t tmpreg_GFEN;
        uint32_t portpin, pos, mask_s, mask_l, mask_d;  
        /* Check the parameters */
        assert_param(IS_PORT_ALL_PERIPH(PORTx));
        assert_param(IS_PORT_PIN(PORT_InitStruct->PORT_Pin));
        assert_param(IS_PORT_OE(PORT_InitStruct->PORT_OE));
        assert_param(IS_PORT_PULL_UP(PORT_InitStruct->PORT_PULL_UP));
        assert_param(IS_PORT_PULL_DOWN(PORT_InitStruct->PORT_PULL_DOWN));
        assert_param(IS_PORT_PD_SHM(PORT_InitStruct->PORT_PD_SHM));
        assert_param(IS_PORT_PD(PORT_InitStruct->PORT_PD));
        assert_param(IS_PORT_GFEN(PORT_InitStruct->PORT_GFEN));
        assert_param(IS_PORT_FUNC(PORT_InitStruct->PORT_FUNC));
        assert_param(IS_PORT_SPEED(PORT_InitStruct->PORT_SPEED));
        assert_param(IS_PORT_MODE(PORT_InitStruct->PORT_MODE)); 
        /* Get current PORT register values */
        tmpreg_OE     = PORTx->OE;
        tmpreg_FUNC   = PORTx->FUNC;
        tmpreg_ANALOG = PORTx->ANALOG;
        tmpreg_PULL   = PORTx->PULL;
        tmpreg_PD     = PORTx->PD;
        tmpreg_PWR    = PORTx->PWR;
        tmpreg_GFEN   = PORTx->GFEN;    
        /* Form new values */
        pos = 0;
        mask_s = 0x0001;
        mask_l = 0x00000003;
        mask_d = 0x00010001;
        for (portpin = PORT_InitStruct->PORT_Pin; portpin; portpin >>= 1)
        {
          if (portpin & 0x1)
          {
            tmpreg_OE     = (tmpreg_OE     & ~mask_s) | (PORT_InitStruct->PORT_OE        <<  pos);
            tmpreg_FUNC   = (tmpreg_FUNC   & ~mask_l) | (PORT_InitStruct->PORT_FUNC      << (pos*2));
            tmpreg_ANALOG = (tmpreg_ANALOG & ~mask_s) | (PORT_InitStruct->PORT_MODE      <<  pos);
            tmpreg_PULL   = (tmpreg_PULL   & ~mask_d) | (PORT_InitStruct->PORT_PULL_UP   << (pos + 16))
                                                      | (PORT_InitStruct->PORT_PULL_DOWN <<  pos);
            tmpreg_PD     = (tmpreg_PD     & ~mask_d) | (PORT_InitStruct->PORT_PD_SHM    << (pos + 16))
                                                      | (PORT_InitStruct->PORT_PD        <<  pos);
            tmpreg_PWR    = (tmpreg_PWR    & ~mask_l) | (PORT_InitStruct->PORT_SPEED     << (pos*2));
            tmpreg_GFEN   = (tmpreg_GFEN   & ~mask_s) | (PORT_InitStruct->PORT_GFEN      <<  pos);
          }
          mask_s <<= 1;
          mask_l <<= 2;
          mask_d <<= 1;
          pos++;
        }

        PORTx->OE     = tmpreg_OE;
        PORTx->FUNC   = tmpreg_FUNC;
        PORTx->ANALOG = tmpreg_ANALOG;
        PORTx->PULL   = tmpreg_PULL;
        PORTx->PD     = tmpreg_PD;
        PORTx->PWR    = tmpreg_PWR;
        PORTx->GFEN   = tmpreg_GFEN;
}

static int I2C_packet_transfered()
{
        while (I2C_GetFlagStatus(I2C_FLAG_nTRANS) != SET);
        /* Transmit data if ACK was send */
        if (I2C_GetFlagStatus(I2C_FLAG_SLAVE_ACK) == RESET)
                return -1;
        else
                return 0;
}

int motor_control_init()
{
        PORT_InitTypeDef        port;
        TIMER_CntInitTypeDef    sTIM3CntInit;
        TIMER_ChnInitTypeDef    sTIM3ChInit;
	TIMER_ChnOutInitTypeDef sTIM3ChOutInit;
	I2C_InitTypeDef         i2c;

        // ---- WARNING! Only decay mode is implemented ----
        // ----------------------- PWM PINS CONFIGURATION -----------------------
        // set pin PC2 as digital output (coast mode), in general it must be PWM0 (alter func)
        RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTC, ENABLE);
        PORT_StructInit(&port);
        port.PORT_FUNC = PORT_FUNC_PORT;
        port.PORT_MODE = PORT_MODE_DIGITAL;
        port.PORT_PULL_UP = PORT_PULL_UP_OFF;
        port.PORT_PULL_DOWN = PORT_PULL_DOWN_OFF;
        port.PORT_SPEED = PORT_SPEED_FAST;
        port.PORT_Pin = PORT_Pin_2;
        port.PORT_OE = PORT_OE_OUT;
        PORT_Init(MDR_PORTC, &port);

        // set pin PB5 as digital output (coast mode), in general it must be PWM2 (alter func)
        RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTB, ENABLE);
        port.PORT_Pin = PORT_Pin_5;
        __PORT_Init(MDR_PORTB, &port);

        // set pin PD2 as PWM1 (overrid func)
        RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTD, ENABLE);
        port.PORT_Pin = PORT_Pin_2;
        port.PORT_FUNC = PORT_FUNC_OVERRID;
        __PORT_Init(MDR_PORTD, &port);

        // set pin PB7 as PWM3 (overrid func)
        port.PORT_Pin = PORT_Pin_7;
        __PORT_Init(MDR_PORTB, &port);
        // ----------------------- END OF PWM PINS CONFIGURATION -----------------------

        // ----------------------- PWM TIMER CONFIGURATION -----------------------
	TIMER_DeInit(MDR_TIMER3);

	// Enable clocking for TIMER1 perihireal
	RST_CLK_PCLKcmd(RST_CLK_PCLK_TIMER3, ENABLE);

	// Configure TIMER1 settings for period 20us (50 kHz)
	sTIM3CntInit.TIMER_Prescaler = 0;	// resFreq = PCLK / (TIMER_HCLKdivX * (TIMER_Prescaler + 1) * Period)
	sTIM3CntInit.TIMER_IniCounter = 0;
	sTIM3CntInit.TIMER_Period = 1600;
	sTIM3CntInit.TIMER_CounterMode = TIMER_CntMode_ClkFixedDir;
	sTIM3CntInit.TIMER_CounterDirection = TIMER_CntDir_Up;
	sTIM3CntInit.TIMER_EventSource = TIMER_EvSrc_TIM_CLK;
	sTIM3CntInit.TIMER_FilterSampling = TIMER_FDTS_TIMER_CLK_div_1;
	sTIM3CntInit.TIMER_ARR_UpdateMode = TIMER_ARR_Update_Immediately;
	sTIM3CntInit.TIMER_ETR_FilterConf = TIMER_Filter_1FF_at_TIMER_CLK;
	sTIM3CntInit.TIMER_ETR_Prescaler = TIMER_ETR_Prescaler_None;
	sTIM3CntInit.TIMER_ETR_Polarity = TIMER_ETRPolarity_NonInverted;
	sTIM3CntInit.TIMER_BRK_Polarity = TIMER_BRKPolarity_NonInverted;
	TIMER_CntInit(MDR_TIMER3, &sTIM3CntInit);

        // Init channels
	TIMER_ChnStructInit(&sTIM3ChInit);

	sTIM3ChInit.TIMER_CH_Mode		= TIMER_CH_MODE_PWM;
	sTIM3ChInit.TIMER_CH_REF_Format         = TIMER_CH_REF_Format6;
	sTIM3ChInit.TIMER_CH_Number		= TIMER_CHANNEL2;
	TIMER_ChnInit(MDR_TIMER3, &sTIM3ChInit);
        sTIM3ChInit.TIMER_CH_Number		= TIMER_CHANNEL4;
	TIMER_ChnInit(MDR_TIMER3, &sTIM3ChInit);
	
	TIMER_SetChnCompare(MDR_TIMER3, TIMER_CHANNEL2, 0);
	TIMER_SetChnCompare(MDR_TIMER3, TIMER_CHANNEL4, 0);
	
	// Init channels outs
	TIMER_ChnOutStructInit(&sTIM3ChOutInit);
	
	sTIM3ChOutInit.TIMER_CH_DirOut_Polarity         = TIMER_CHOPolarity_NonInverted;
	sTIM3ChOutInit.TIMER_CH_DirOut_Source		= TIMER_CH_OutSrc_REF;
	sTIM3ChOutInit.TIMER_CH_DirOut_Mode		= TIMER_CH_OutMode_Output;
	sTIM3ChOutInit.TIMER_CH_NegOut_Polarity         = TIMER_CHOPolarity_NonInverted;
	sTIM3ChOutInit.TIMER_CH_NegOut_Source		= TIMER_CH_OutSrc_REF;
	sTIM3ChOutInit.TIMER_CH_NegOut_Mode		= TIMER_CH_OutMode_Output;
	sTIM3ChOutInit.TIMER_CH_Number			= TIMER_CHANNEL2;
	TIMER_ChnOutInit(MDR_TIMER3, &sTIM3ChOutInit);
	sTIM3ChOutInit.TIMER_CH_Number			= TIMER_CHANNEL4;
	TIMER_ChnOutInit(MDR_TIMER3, &sTIM3ChOutInit);

	// Set TIM/CNT3 clock from HCLK with prescaler of 8
	TIMER_BRGInit(MDR_TIMER3, TIMER_HCLKdiv1);
        TIMER_Cmd(MDR_TIMER3, ENABLE);
        // ----------------------- END OF PWM TIMER CONFIGURATION -----------------------

        // ----------------------- I2C CONTROL CONFIGURATION -----------------------
        // check if I2C is already configured
        uint32_t i2c_enabled = MDR_I2C->CTR & (1 << I2C_CTR_EN_I2C_Pos);
        if (!i2c_enabled) {
                // Init CLK for PORT and I2C
                RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTC, ENABLE);
                RST_CLK_PCLKcmd(RST_CLK_PCLK_I2C, ENABLE);

                // Configure ports
                PORT_StructInit(&port);
                port.PORT_FUNC = PORT_FUNC_ALTER;
                port.PORT_MODE = PORT_MODE_DIGITAL;
                port.PORT_PULL_UP = PORT_PULL_UP_OFF;
                port.PORT_PULL_DOWN = PORT_PULL_DOWN_OFF;
                port.PORT_SPEED = PORT_SPEED_FAST;
                port.PORT_Pin = PORT_Pin_0 | PORT_Pin_1;
                //port.PORT_OE = PORT_OE_OUT;
                PORT_Init(MDR_PORTC, &port);

                I2C_Cmd(ENABLE);

                I2C_StructInit(&i2c);
                i2c.I2C_ClkDiv = 32;
                i2c.I2C_Speed = I2C_SPEED_UP_TO_400KHz;
                I2C_Init(&i2c);
        }

        // config I2C<->GPIO expander
        // 1. Configure MODE and nSLEEP pins to OUTPUT
        uint8_t ctrlOutVals =   (1 << MOTOR1_MODE_POS) |
                                (1 << MOTOR1_nSLEEP_POS) |
                                (1 << MOTOR2_MODE_POS) |
                                (1 << MOTOR2_nSLEEP_POS);
        
        // 2. Set nSLEEP and MODE pins output state to 0 (inner register)
        uint8_t ctrlDirVals = ~ctrlOutVals;

        // 3. Write settings to expander via I2C
        // 3.1 Write output vals
        I2C_Send7bitAddress(MOTOR_CTRL_ADDR, I2C_Direction_Transmitter);
        if (I2C_packet_transfered())
                return -1;
        I2C_SendByte((uint8_t) MOTOR_CTRL_OUTPUT_REG);
        if (I2C_packet_transfered())
                return -1;
        I2C_SendByte(ctrlOutVals);
        if (I2C_packet_transfered())
                return -1;
	I2C_SendSTOP();

        // 3.2 Write direction vals
        I2C_Send7bitAddress(MOTOR_CTRL_ADDR, I2C_Direction_Transmitter);
        if (I2C_packet_transfered())
                return -1;
        I2C_SendByte((uint8_t) MOTOR_CTRL_CONFIG_REG);
        if (I2C_packet_transfered())
                return -1;
        I2C_SendByte(ctrlDirVals);
        if (I2C_packet_transfered())
                return -1;
	I2C_SendSTOP();

        // Set current motor modes and states
        motor1Mode      = DECAY;
        motor2Mode      = DECAY;
        motor1State     = SLEEP;
        motor2State     = SLEEP;
        // ----------------------- END OF I2C CONTROL CONFIGURATION -----------------------

        return 0;
}

// this function must be called as abs(voltage)
void set_main_motor_out_volt(__qu32_t voltage, enum _motor_dir direction)
{
	uint32_t pwm_cnt = 0;
	
	TIMER_Cmd(MDR_TIMER3, DISABLE);
	//set_main_motor_dir(direction);
	
	if (direction == CLOCKWISE) {
		pwm_cnt = FTOINT(voltage * VOLT_TO_CNT, GENERAL_Q_FORM);
		if (pwm_cnt > PWM_PERIOD)
			pwm_cnt = PWM_PERIOD;
		
		MDR_PORTC->RXTX &= ~PORT_Pin_2;
		//TIMER_SetCounter(MDR_TIMER2, 0); //NOT SURE THAT IT MUST BE DONE
	} else {
		pwm_cnt = FTOINT(voltage * VOLT_TO_CNT, GENERAL_Q_FORM);
		if (pwm_cnt > PWM_PERIOD)
			pwm_cnt = PWM_PERIOD;
		pwm_cnt = PWM_PERIOD - pwm_cnt;
		
		MDR_PORTC->RXTX |= PORT_Pin_2;
	}
	TIMER_SetChnCompare(MDR_TIMER3, TIMER_CHANNEL2,
					(uint16_t)pwm_cnt);
	TIMER_Cmd(MDR_TIMER3, ENABLE);	
}

// this function must be called as abs(voltage)
void set_sub_motor_out_volt(__qu32_t voltage, enum _motor_dir direction)
{
	uint32_t pwm_cnt = 0;
	
	TIMER_Cmd(MDR_TIMER3, DISABLE);
	//set_main_motor_dir(direction);
	
	if (direction == CLOCKWISE) {
		pwm_cnt = FTOINT(voltage * VOLT_TO_CNT, GENERAL_Q_FORM);
		if (pwm_cnt > PWM_PERIOD)
			pwm_cnt = PWM_PERIOD;
		
		MDR_PORTB->RXTX &= ~PORT_Pin_5;
		//TIMER_SetCounter(MDR_TIMER2, 0); //NOT SURE THAT IT MUST BE DONE
	} else {
		pwm_cnt = FTOINT(voltage * VOLT_TO_CNT, GENERAL_Q_FORM);
		if (pwm_cnt > PWM_PERIOD)
			pwm_cnt = PWM_PERIOD;
		pwm_cnt = PWM_PERIOD - pwm_cnt;
		
		MDR_PORTB->RXTX |= PORT_Pin_5;
	}
	TIMER_SetChnCompare(MDR_TIMER3, TIMER_CHANNEL4,
					(uint16_t)pwm_cnt);
	TIMER_Cmd(MDR_TIMER3, ENABLE);	
}

int motor_control_write_mode(enum motorMode mode, enum Motor motor)
{
        uint8_t ctrlByte;
        if (motor == MOTOR1)
                ctrlByte =      (mode << MOTOR1_MODE_POS) | 
                                (motor2Mode << MOTOR2_MODE_POS);
        else
                ctrlByte =      (mode << MOTOR2_MODE_POS) | 
                                (motor1Mode << MOTOR1_MODE_POS);
        ctrlByte |=     (motor1State << MOTOR1_nSLEEP_POS) |
                        (motor2State << MOTOR2_nSLEEP_POS);

        I2C_Send7bitAddress(MOTOR_CTRL_ADDR, I2C_Direction_Transmitter);
        if (I2C_packet_transfered())
                return -1;
        I2C_SendByte((uint8_t) MOTOR_CTRL_CONFIG_REG);
        if (I2C_packet_transfered())
                return -1;
        I2C_SendByte(ctrlByte);
        if (I2C_packet_transfered())
                return -1;
	I2C_SendSTOP();

        // Change current motor mode
        if (motor == MOTOR1)
                motor1Mode = mode;
        else
                motor2Mode = mode;

        return 0;
}

int motor_control_write_state(enum motorState state, enum Motor motor)
{
        uint8_t ctrlByte;
        if (motor == MOTOR1)
                ctrlByte =      (state << MOTOR1_nSLEEP_POS) | 
                                (motor2State << MOTOR2_nSLEEP_POS);
        else
                ctrlByte =      (state << MOTOR2_nSLEEP_POS) | 
                                (motor1State << MOTOR1_nSLEEP_POS);
        ctrlByte |=     (motor1Mode << MOTOR1_MODE_POS) |
                        (motor2Mode << MOTOR2_MODE_POS);

        I2C_Send7bitAddress(MOTOR_CTRL_ADDR, I2C_Direction_Transmitter);
        if (I2C_packet_transfered())
                return -1;
        I2C_SendByte((uint8_t) MOTOR_CTRL_CONFIG_REG);
        if (I2C_packet_transfered())
                return -1;
        I2C_SendByte(ctrlByte);
        if (I2C_packet_transfered())
                return -1;
	I2C_SendSTOP();

        // Change current motor mode
        if (motor == MOTOR1)
                motor1State = state;
        else
                motor2State = state;

        return 0;
}

