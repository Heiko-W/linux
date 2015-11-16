/*
 * TAS5518 8-Channel HD Compatible Audio Processor
 *
 * Copyright (C) 2015 Heiko Wilke <heiko.wilke@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define DRV_NAME "TAS5518"


/*
 * TAS5518 I2C Addresses
 */
#define TAS5518_I2C_READ		0x37
#define TAS5518_I2C_WRITE		0x36

/*
 * TAS5518 registers
 */

#define TAS5518_DEV_ID			0x01	 /* Device ID register */
#define TAS5518_ERROR_STATUS		0x02     /* Error status register */
#define TAS5518_SYS_CONTROL_1		0x03     /* System control register 1 */
#define TAS5518_SYS_CONTROL_2		0x04     /* System control register 2 */
#define TAS5518_CHANNEL_CONFIG(X)	(0x05 + (X)) /* Channel Configuration Register 1-8*/
#define TAS5518_HEADPHONE_CONFIG	0x0D	/* Headphone configuration control register */
#define TAS5518_SERIAL_DATA_IF		0x0E	/* Serial data interface register  */
#define TAS5518_SOFT_MUTE		0x0F	/* Soft mute register */
#define TAS5518_ENERGY_MGR		0x10	/* Energy Managers Register */
#define TAS5518_OSC_TRIM		0x12	/* Oscillator trim register */
#define TAS5518_AUTOMUTE_CTRL		0x14	/* Automute control register */
#define TAS5518_AM_PWM_THRESHOLD	0x15	/* Automute PWM threshold and back-end reset period register*/
#define TAS5518_MODULATION_LIMIT(X)     (0x16 + (X)) /* Modulation Limit Reg 1-4 with 2 channels per register */
#define TAS5518_IC_DELAY(X)		(0x1B + (X)) /* IC Delay Channel 1-8 */
#define TAS5518_CHANNEL_SHUTDOWN	0x27	/* Individual Channel Shutdown */
#define TAS5518_INPUT_MUX(X)		(0x30 + (X)) /* Input MUX 1-4 with 2 channels per register byte */
#define TAS5518_PWM_MUX(X)		(0x34 + (X)) /* Input MUX 1-4 with 2 channels per register byte */
#define TAS5518_CHANNEL_VOL(X)		(0xD1 + (X)) /* Channel 1-8 Volume*/
#define TAS5518_MASTER_VOL		0xD9	/* Master volume  */

#define TAS5518_MAX_REGISTER            0xE0

/*
 * TAS5518 Bitmasks and Values
 */
#define TAS5518_DEVICE_ID		0x01		/* Identification code for TAS5518 */
#define TAS5518_DEVICE_ID_MASK          0x3F            /* ID Register Mask*/


struct tas5518_init_command {
        const int size;
        const char *const data;
};

static const struct tas5518_init_command tas5518_init_sequence[] = {
        // System Control Register 1 - PWM High Pass Enabled & Hard Unmute on recovery from clock & PSVC Hi-Z disable
        { .size = 2,  .data = "\x03\x90" },
        // Modulation Index Limit Register - Limit (DCLKS) = 5 & MIN WIDTH (DCLKS) = 10 & MODULATION INDEX = 96.1%
        { .size = 2,  .data = "\x16\x04" },
};