#ifndef CURRENT_SENSOR_H
#define CURRENT_SENSOR_H

enum current_sensor{
	SENSOR_1 = 0,
	SENSOR_2
};

// Current sensor commands list
#define WAKEUP		0x00
#define SLEEP		0x02
#define SYNC_1		0x04
#define SYNC_2		0x04
#define RESET		0x06
#define NOP		0xFF
#define RDATA		0x12
#define RDATAC		0x14
#define SDATAC		0x16
#define RREG		0x20
#define WREG		0x40
#define SYSOCAL		0x60
#define SYSGCAL		0x61
#define SELFOCAL	0x62

// Current sensor regs list
#define BCS	0x00
#define VBIAS	0x01
#define MUX1	0x02
#define SYS0	0x03
#define OFC0	0x04
#define OFC1	0x05
#define OFC2	0x06
#define FSC0	0x07
#define FSC1	0x08
#define FSC2	0x09
#define ID	0x0A

uint8_t current_sensor_read_ID(enum current_sensor sensor);
void current_sensor_set_80SPS(enum current_sensor sensor);
int32_t current_sensor_get_current(enum current_sensor sensor);

#endif