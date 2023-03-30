#ifndef _FUZZY_REGULATOR_H
#define _FUZZY_REGULATOR_H

#include "stdbool.h"
#include "stdint.h"
#include "fxd_pnt_math.h"
#include "mbregmap.h"

enum __CHANNNEL_NUM {
	FIRST = 0,
	SECOND,
	THIRD,
	FOURTH,
	FIFTH
};

enum __PP_TYPE {
	BUF = 0,
	INTGR,
	DFRN
};

enum __IN_TYPE {
	ERR = 0,
	EXT
};

struct __fuzzy_channel {
	volatile __qu32_t	*NLT_X;
	volatile __q32_t	*NLT_Y;
	volatile __qu32_t	*NLT_DOT_NUM;
	volatile __q32_t	*NLT_IN;
	volatile __q32_t	*NLT_OUT;
	volatile __q32_t	*EXT_IN;
	volatile __q32_t	*ERR_IN;
	volatile __q32_t	*SOURCE_IN;
	volatile uint8_t	*IN_SEL;
	volatile uint8_t	*CH_ON;
	volatile uint8_t	*PP_SEL0;
	volatile uint8_t	*PP_SEL1;
	volatile uint8_t	*INTGR_RST;
	volatile uint8_t	*DFRN_RST;
	struct __INTGR		*INTGR;
	struct __DFRN		*DFRN;
};

struct __INTGR {
	__q32_t	val;
	__q32_t valStepQ;
	void	(*intgr) (__qu32_t h, struct __fuzzy_channel fuzzyCh);
};

struct __DFRN {
	__q32_t	val;
	__q32_t prevX;
	void	(*dfrn) (__qu32_t h, struct __fuzzy_channel fuzzyCh);
};

struct __system_settings {
	__qu32_t	*REG_CYCLE_DUR;
	bool		*REG_ON;
	bool		*STEP_CMPLT;	
};

struct __fuzzy_regulator {
	#define US_TO_S_Q_FORM	1.048576	// 2 ^ STEP_S_Q_FORM / 10^6 
	#define nUS_TO_S_Q_FROM	1024000000

	#define GENERAL_Q_FORM	10
	#define STEP_S_Q_FORM	20

	struct __fuzzy_channel	ch1;
	struct __fuzzy_channel	ch2;
	struct __fuzzy_channel	ch3;
	struct __fuzzy_channel	ch4;
	struct __fuzzy_channel	ch5;

	uint32_t	* REG_CYCLE_DUR;	// us step time
	uint8_t		* REG_ON;
	uint8_t		* STEP_CMPLT;

	// utility fields
	__qu32_t	stepS;			// Q12.20 form is used
	__qu32_t	invStepS;		// Q20.12 form is used
	int		is_next_step;
};

void init_fuzzy_regulator(int32_t *HOLD_REG_BUF, uint8_t *COIL_REG_BUF,
				struct __fuzzy_regulator *fuzzyRegulator);
void fuzzy_reg_transform(struct __fuzzy_regulator fuzzyRegulator);
void fuzzy_regulator_update_step(struct __fuzzy_regulator *fuzzyRegulator);

#endif //_FUZZY_REGULATOR_H
