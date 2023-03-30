#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include "fxd_pnt_math.h"

#define PWM0_PIN_POS		2	// port C
#define PWM1_PIN_POS		2	// port D (must be PWM in decay mode)
#define PWM2_PIN_POS		5	// port B
#define PWM3_PIN_POS		7	// port B (must be PWM in decay mode)

#define PWM0_TIMER3_CH		1	
#define PWM1_TIMER3_CH		2
#define PWM2_TIMER3_CH		3
#define PWM3_TIMER3_CH		4

#define MOTOR_CTRL_ADDR         0x40	// i2C<->GPIO Expander
#define MOTOR_CTRL_INPUT_REG	0
#define MOTOR_CTRL_OUTPUT_REG	1
#define MOTOR_CTRL_POLARITY_REG	2
#define MOTOR_CTRL_CONFIG_REG	3

#define MOTOR2_MODE_POS		0
#define MOTOR2_nSLEEP_POS	1
#define MOTOR2_nFAULT_POS	2
#define MOTOR1_MODE_POS		3
#define MOTOR1_nSLEEP_POS	4
#define MOTOR1_nFAULT_POS	5

enum _en_state {
	OFF,
	ON
};

enum _motor_dir {
	CLOCKWISE,
	COUNTER_CLOCKWISE
};

enum Motor {
	MOTOR2 = 0,
	MOTOR1
};

enum motorMode {
	DECAY = 0,
	COAST
};

enum motorState {
	SLEEP = 0,
	RUN
};

int motor_control_init(void);
void set_main_motor_out_volt(__qu32_t voltage, enum _motor_dir direction);
void set_sub_motor_out_volt(__qu32_t voltage, enum _motor_dir direction);
int motor_control_write_mode(enum motorMode mode, enum Motor motor);
int motor_control_write_state(enum motorState state, enum Motor motor);


#endif