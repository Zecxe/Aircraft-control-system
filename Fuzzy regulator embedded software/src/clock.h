#ifndef __CLOCK_H
#define __CLOCK_H

void config_ext_clk(void);
void step_timer_init(void);
void step_timer_update_step(uint16_t step_us);
void step_timer_restart(void);
void step_timer_stop(void);

#endif