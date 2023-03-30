#include "fuzzy_regulator.h"
#include "stdlib.h"
//#include "current_sensor.h"

// TO-DO:
//	1. Add supervisor to update quant Timer
//	2. Add recalculation of step time


// Static function definitions
static void intgr_func( __qu32_t stepUs, struct __fuzzy_channel fuzzyChannel)
{
	// Integral calculation is made in two steps
	// At first step integral in step Q_form is calculated
	// If the resulated integral is enough to be converted in general Q_FORM
	//	then it is converted and added to main integral calculation

	__q32_t tmp = 0;

	// First step
	fuzzyChannel.INTGR->valStepQ = FADD(fuzzyChannel.INTGR->valStepQ, 
						FMULG(*fuzzyChannel.SOURCE_IN,
						stepUs, GENERAL_Q_FORM, 
						STEP_S_Q_FORM, STEP_S_Q_FORM));
	
	// Second step
	tmp = FCONV(fuzzyChannel.INTGR->valStepQ, STEP_S_Q_FORM, GENERAL_Q_FORM);
	if (tmp != 0) {
		fuzzyChannel.INTGR->val = FADD(fuzzyChannel.INTGR->val ,tmp);
		fuzzyChannel.INTGR->valStepQ = 0;
	}
}

static struct __INTGR ch1Ingr = {
	.val = 0,
	.valStepQ = 0,
	.intgr = &intgr_func
};
static struct __INTGR ch2Ingr = {
	.val = 0,
	.valStepQ = 0,
	.intgr = &intgr_func
};
static struct __INTGR ch3Ingr = {
	.val = 0,
	.valStepQ = 0,
	.intgr = &intgr_func
};
static struct __INTGR ch4Ingr = {
	.val = 0,
	.valStepQ = 0,
	.intgr = &intgr_func
};
static struct __INTGR ch5Ingr = {
	.val = 0,
	.valStepQ = 0,
	.intgr = &intgr_func
};

static void dfrn_func(__qu32_t invStepUs, struct __fuzzy_channel fuzzyChannel)
{
	__q32_t tmp = 0;
	
	tmp = FSUB(*fuzzyChannel.SOURCE_IN, fuzzyChannel.DFRN->prevX);
	fuzzyChannel.DFRN->val = FMUL(tmp, invStepUs, GENERAL_Q_FORM);
	fuzzyChannel.DFRN->prevX = *fuzzyChannel.SOURCE_IN;
}

static struct __DFRN ch1Dfrn = {
	.val = 0,
	.prevX = 0,
	.dfrn = &dfrn_func
};
static struct __DFRN ch2Dfrn = {
	.val = 0,
	.prevX = 0,
	.dfrn = &dfrn_func
};
static struct __DFRN ch3Dfrn = {
	.val = 0,
	.prevX = 0,
	.dfrn = &dfrn_func
};
static struct __DFRN ch4Dfrn = {
	.val = 0,
	.prevX = 0,
	.dfrn = &dfrn_func
};
static struct __DFRN ch5Dfrn = {
	.val = 0,
	.prevX = 0,
	.dfrn = &dfrn_func
};

/*-------------------------------------------------------*
* Because in fuzzy channel all points are presented      *
* in q form of Q_FORM, then we don't need to convert 	 *
* them to q form and we can work with them in no time.   *
* For guide how to work with fixed point numbers see     *
* Application Note 33 Fixed Point Arithmetic on the ARM  *
*--------------------------------------------------------*/

// Determine input source (internal or external)
static void IN_mux(struct __fuzzy_channel fuzzyChannel) 
{
	switch ((enum __IN_TYPE) *fuzzyChannel.IN_SEL) {
		case (EXT):
			*fuzzyChannel.SOURCE_IN = *fuzzyChannel.EXT_IN;
			break;
		case (ERR):
			*fuzzyChannel.SOURCE_IN = *fuzzyChannel.ERR_IN;
			break;
		default:
			*fuzzyChannel.SOURCE_IN = *fuzzyChannel.EXT_IN;
			break;
	}
}

// Determine PP block selection
static void PP_mux(__qu32_t stepUs, __qu32_t invStepUs, 
					struct __fuzzy_channel fuzzyChannel)
{
	enum __PP_TYPE PPType;

	PPType = (enum __PP_TYPE) (*fuzzyChannel.PP_SEL0 | (*fuzzyChannel.PP_SEL1 << 1));
	switch (PPType) {
		case BUF:
			*fuzzyChannel.NLT_IN = *fuzzyChannel.SOURCE_IN;
			break;
		case INTGR:
			fuzzyChannel.INTGR->intgr(stepUs, fuzzyChannel);
			*fuzzyChannel.NLT_IN = fuzzyChannel.INTGR->val;
			break;
		case DFRN:
			fuzzyChannel.DFRN->dfrn(invStepUs, fuzzyChannel);
			*fuzzyChannel.NLT_IN = fuzzyChannel.DFRN->val;
			break;
	}
}

// Make non-linear transfer
static void NL_transform (struct __fuzzy_channel fuzzyChannel)
{
	int i;
	__q32_t		in;
	__qu32_t	lastIndex;
	__q32_t		num, denum = 0;
	__q32_t		k;
	__q32_t		b;
	__q32_t		res; 

	in = *fuzzyChannel.NLT_IN;
	
	// STEP 1. Check if input in saturation zone
	lastIndex = *fuzzyChannel.NLT_DOT_NUM - 1;

	if (in >= *(fuzzyChannel.NLT_X + lastIndex))
		*fuzzyChannel.NLT_OUT = *(fuzzyChannel.NLT_Y + lastIndex);

	// STEP 2. For every point starting from the 0 and to the right
	// find the section of characteristic
	for (i = 0; i < *(fuzzyChannel.NLT_DOT_NUM); i++) {
		if ((in >= *(fuzzyChannel.NLT_X + i)) &&
			(in <= *(fuzzyChannel.NLT_X + i + 1))) {
			
			// Fixed points algorithm
			
			num = FSUB(*(fuzzyChannel.NLT_Y + i + 1), 
					*(fuzzyChannel.NLT_Y + i));
			denum = FSUB(*(fuzzyChannel.NLT_X + i + 1), 
					*(fuzzyChannel.NLT_X + i));
			
			k = FDIV(num, denum, GENERAL_Q_FORM);
			b = FMUL(k, *(fuzzyChannel.NLT_X + i), GENERAL_Q_FORM);
			b = FSUB(*(fuzzyChannel.NLT_Y + i), b);

			res = FMUL(k, in, GENERAL_Q_FORM);
			res = FADD(res, b);

			if (in < 0)
				*fuzzyChannel.NLT_OUT = -res;
			else 
				*fuzzyChannel.NLT_OUT = res;
		}
	}
}

static void fuzzy_ch_transform(__qu32_t stepUs, __qu32_t invStepUs,
				struct __fuzzy_channel fuzzyChannel)
{
	IN_mux(fuzzyChannel);
	PP_mux(stepUs, invStepUs, fuzzyChannel);
	NL_transform(fuzzyChannel);
}

void fuzzy_regulator_update_step(struct __fuzzy_regulator *fuzzyRegulator)
{
	fuzzyRegulator->stepS = (__qu32_t)(*fuzzyRegulator->REG_CYCLE_DUR * US_TO_S_Q_FORM);
	fuzzyRegulator->invStepS = (__qu32_t)(nUS_TO_S_Q_FROM / *fuzzyRegulator->REG_CYCLE_DUR);
}

void init_fuzzy_regulator(int32_t *HOLD_REG_BUF, uint8_t *COIL_REG_BUF,
				struct __fuzzy_regulator *fuzzyRegulator)
{
	// Init all fuzzy channels
	// First channel (Proportional)
	fuzzyRegulator->ch1.NLT_X	=	HOLD_REG_BUF + NLT1_X_OFFSET;
	fuzzyRegulator->ch1.NLT_Y	=	HOLD_REG_BUF + NLT1_Y_OFFSET;
	fuzzyRegulator->ch1.NLT_DOT_NUM	=	HOLD_REG_BUF + NLT1_DOT_NUM_OFFSET;
	fuzzyRegulator->ch1.NLT_IN	=	HOLD_REG_BUF + NLT1_IN_OFFSET;
	fuzzyRegulator->ch1.NLT_OUT	=	HOLD_REG_BUF + NLT1_OUT_OFFSET;
	fuzzyRegulator->ch1.EXT_IN	=	HOLD_REG_BUF + EXT_IN1_OFFSET;
	fuzzyRegulator->ch1.ERR_IN	=	HOLD_REG_BUF + ERR_IN1_OFFSET;
	fuzzyRegulator->ch1.SOURCE_IN	=	HOLD_REG_BUF + SOURCE_IN1_OFFSET;
	fuzzyRegulator->ch1.IN_SEL	=	COIL_REG_BUF + IN1_SEL_OFFSET;
	fuzzyRegulator->ch1.CH_ON	=	COIL_REG_BUF + CH1_ON_OFFSET;
	fuzzyRegulator->ch1.PP_SEL0	=	COIL_REG_BUF + PP1_SEL0_OFFSET;
	fuzzyRegulator->ch1.PP_SEL1	=	COIL_REG_BUF + PP1_SEL1_OFFSET;
	fuzzyRegulator->ch1.INTGR_RST	=	COIL_REG_BUF + INTGR1_RST_OFFSET;
	fuzzyRegulator->ch1.DFRN_RST	=	COIL_REG_BUF + DFRN1_RST_OFFSET;
	fuzzyRegulator->ch1.INTGR	=	&ch1Ingr;
	fuzzyRegulator->ch1.DFRN	=	&ch1Dfrn;

	// Second channel (Integrational)
	fuzzyRegulator->ch2.NLT_X	=	HOLD_REG_BUF + NLT2_X_OFFSET;
	fuzzyRegulator->ch2.NLT_Y	=	HOLD_REG_BUF + NLT2_Y_OFFSET;
	fuzzyRegulator->ch2.NLT_DOT_NUM	=	HOLD_REG_BUF + NLT2_DOT_NUM_OFFSET;
	fuzzyRegulator->ch2.NLT_IN	=	HOLD_REG_BUF + NLT2_IN_OFFSET;
	fuzzyRegulator->ch2.NLT_OUT	=	HOLD_REG_BUF + NLT2_OUT_OFFSET;
	fuzzyRegulator->ch2.EXT_IN	=	HOLD_REG_BUF + EXT_IN2_OFFSET;
	fuzzyRegulator->ch2.ERR_IN	=	HOLD_REG_BUF + ERR_IN2_OFFSET;
	fuzzyRegulator->ch2.SOURCE_IN	=	HOLD_REG_BUF + SOURCE_IN2_OFFSET;
	fuzzyRegulator->ch2.IN_SEL	=	COIL_REG_BUF + IN2_SEL_OFFSET;
	fuzzyRegulator->ch2.CH_ON	=	COIL_REG_BUF + CH2_ON_OFFSET;
	fuzzyRegulator->ch2.PP_SEL0	=	COIL_REG_BUF + PP2_SEL0_OFFSET;
	fuzzyRegulator->ch2.PP_SEL1	=	COIL_REG_BUF + PP2_SEL1_OFFSET;
	fuzzyRegulator->ch2.INTGR_RST	=	COIL_REG_BUF + INTGR2_RST_OFFSET;
	fuzzyRegulator->ch2.DFRN_RST	=	COIL_REG_BUF + DFRN2_RST_OFFSET;
	fuzzyRegulator->ch2.INTGR	=	&ch2Ingr;
	fuzzyRegulator->ch2.DFRN	=	&ch2Dfrn;

	// Third channel (Differntor)
	fuzzyRegulator->ch3.NLT_X	=	HOLD_REG_BUF + NLT3_X_OFFSET;
	fuzzyRegulator->ch3.NLT_Y	=	HOLD_REG_BUF + NLT3_Y_OFFSET;
	fuzzyRegulator->ch3.NLT_DOT_NUM	=	HOLD_REG_BUF + NLT3_DOT_NUM_OFFSET;
	fuzzyRegulator->ch3.NLT_IN	=	HOLD_REG_BUF + NLT3_IN_OFFSET;
	fuzzyRegulator->ch3.NLT_OUT	=	HOLD_REG_BUF + NLT3_OUT_OFFSET;
	fuzzyRegulator->ch3.EXT_IN	=	HOLD_REG_BUF + EXT_IN3_OFFSET;
	fuzzyRegulator->ch3.ERR_IN	=	HOLD_REG_BUF + ERR_IN3_OFFSET;
	fuzzyRegulator->ch3.SOURCE_IN	=	HOLD_REG_BUF + SOURCE_IN3_OFFSET;
	fuzzyRegulator->ch3.IN_SEL	=	COIL_REG_BUF + IN3_SEL_OFFSET;
	fuzzyRegulator->ch3.CH_ON	=	COIL_REG_BUF + CH3_ON_OFFSET;
	fuzzyRegulator->ch3.PP_SEL0	=	COIL_REG_BUF + PP3_SEL0_OFFSET;
	fuzzyRegulator->ch3.PP_SEL1	=	COIL_REG_BUF + PP3_SEL1_OFFSET;
	fuzzyRegulator->ch3.INTGR_RST	=	COIL_REG_BUF + INTGR3_RST_OFFSET;
	fuzzyRegulator->ch3.DFRN_RST	=	COIL_REG_BUF + DFRN3_RST_OFFSET;
	fuzzyRegulator->ch3.INTGR	=	&ch3Ingr;
	fuzzyRegulator->ch3.DFRN	=	&ch3Dfrn;

	// Fourth channel (External compensation)
	fuzzyRegulator->ch4.NLT_X	=	HOLD_REG_BUF + NLT4_X_OFFSET;
	fuzzyRegulator->ch4.NLT_Y	=	HOLD_REG_BUF + NLT4_Y_OFFSET;
	fuzzyRegulator->ch4.NLT_DOT_NUM	=	HOLD_REG_BUF + NLT4_DOT_NUM_OFFSET;
	fuzzyRegulator->ch4.NLT_IN	=	HOLD_REG_BUF + NLT4_IN_OFFSET;
	fuzzyRegulator->ch4.NLT_OUT	=	HOLD_REG_BUF + NLT4_OUT_OFFSET;
	fuzzyRegulator->ch4.EXT_IN	=	HOLD_REG_BUF + EXT_IN4_OFFSET;
	fuzzyRegulator->ch4.ERR_IN	=	HOLD_REG_BUF + ERR_IN4_OFFSET;
	fuzzyRegulator->ch4.SOURCE_IN	=	HOLD_REG_BUF + SOURCE_IN4_OFFSET;
	fuzzyRegulator->ch4.IN_SEL	=	COIL_REG_BUF + IN4_SEL_OFFSET;
	fuzzyRegulator->ch4.CH_ON	=	COIL_REG_BUF + CH4_ON_OFFSET;
	fuzzyRegulator->ch4.PP_SEL0	=	COIL_REG_BUF + PP4_SEL0_OFFSET;
	fuzzyRegulator->ch4.PP_SEL1	=	COIL_REG_BUF + PP4_SEL1_OFFSET;
	fuzzyRegulator->ch4.INTGR_RST	=	COIL_REG_BUF + INTGR4_RST_OFFSET;
	fuzzyRegulator->ch4.DFRN_RST	=	COIL_REG_BUF + DFRN4_RST_OFFSET;
	fuzzyRegulator->ch4.INTGR	=	&ch4Ingr;
	fuzzyRegulator->ch4.DFRN	=	&ch4Dfrn;

	// Fifth channel (External stabilisation)
	fuzzyRegulator->ch5.NLT_X	=	HOLD_REG_BUF + NLT5_X_OFFSET;
	fuzzyRegulator->ch5.NLT_Y	=	HOLD_REG_BUF + NLT5_Y_OFFSET;
	fuzzyRegulator->ch5.NLT_DOT_NUM	=	HOLD_REG_BUF + NLT5_DOT_NUM_OFFSET;
	fuzzyRegulator->ch5.NLT_IN	=	HOLD_REG_BUF + NLT5_IN_OFFSET;
	fuzzyRegulator->ch5.NLT_OUT	=	HOLD_REG_BUF + NLT5_OUT_OFFSET;
	fuzzyRegulator->ch5.EXT_IN	=	HOLD_REG_BUF + EXT_IN5_OFFSET;
	fuzzyRegulator->ch5.ERR_IN	=	HOLD_REG_BUF + ERR_IN5_OFFSET;
	fuzzyRegulator->ch5.SOURCE_IN	=	HOLD_REG_BUF + SOURCE_IN5_OFFSET;
	fuzzyRegulator->ch5.IN_SEL	=	COIL_REG_BUF + IN5_SEL_OFFSET;
	fuzzyRegulator->ch5.CH_ON	=	COIL_REG_BUF + CH5_ON_OFFSET;
	fuzzyRegulator->ch5.PP_SEL0	=	COIL_REG_BUF + PP5_SEL0_OFFSET;
	fuzzyRegulator->ch5.PP_SEL1	=	COIL_REG_BUF + PP5_SEL1_OFFSET;
	fuzzyRegulator->ch5.INTGR_RST	=	COIL_REG_BUF + INTGR5_RST_OFFSET;
	fuzzyRegulator->ch5.DFRN_RST	=	COIL_REG_BUF + DFRN5_RST_OFFSET;
	fuzzyRegulator->ch5.INTGR	=	&ch5Ingr;
	fuzzyRegulator->ch5.DFRN	=	&ch5Dfrn;

	// Init system settings
	fuzzyRegulator->REG_CYCLE_DUR	=	HOLD_REG_BUF + REG_CYCLE_DUR_OFFSET;
	fuzzyRegulator->REG_ON		=	COIL_REG_BUF + REG_ON_OFFSET;
	fuzzyRegulator->STEP_CMPLT	=	COIL_REG_BUF + STEP_CMPLT_OFFSET;
	fuzzyRegulator->is_next_step	=	0;
}

void fuzzy_reg_transform(struct __fuzzy_regulator fuzzy_regulator)
{
	if (*fuzzy_regulator.ch1.CH_ON)
		fuzzy_ch_transform(fuzzy_regulator.stepS, fuzzy_regulator.invStepS,
					fuzzy_regulator.ch1);
	if (*fuzzy_regulator.ch2.CH_ON)
		fuzzy_ch_transform(fuzzy_regulator.stepS, fuzzy_regulator.invStepS,
					fuzzy_regulator.ch2);
	if (*fuzzy_regulator.ch3.CH_ON)
		fuzzy_ch_transform(fuzzy_regulator.stepS, fuzzy_regulator.invStepS,
					fuzzy_regulator.ch3);
	if (*fuzzy_regulator.ch4.CH_ON)
		fuzzy_ch_transform(fuzzy_regulator.stepS, fuzzy_regulator.invStepS,
					fuzzy_regulator.ch4);
	if (*fuzzy_regulator.ch5.CH_ON)
		fuzzy_ch_transform(fuzzy_regulator.stepS, fuzzy_regulator.invStepS,
					fuzzy_regulator.ch5);

	*fuzzy_regulator.STEP_CMPLT = true;

	// !!!! UPDATE STEP TIMER AFTER IT IN ANOTHER FUNC
}