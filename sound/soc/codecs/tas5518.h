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

  /*********************/
 /* TAS5518 registers */
/*********************/

#define TAS5518_CLK_CTRL             0x00        /* Clock Control register*/
#define TAS5518_DEV_ID               0x01        /* Device ID register */
#define TAS5518_ERROR_STATUS         0x02        /* Error status register */
#define TAS5518_SYS_CONTROL_1        0x03        /* System control register 1 */
#define TAS5518_SYS_CONTROL_2        0x04        /* System control register 2 */
#define TAS5518_CHANNEL_CONFIG(X)   (0x05 + (X)) /* Channel Configuration Register 1-8*/
#define TAS5518_HEADPHONE_CONFIG     0x0D	 /* Headphone configuration control register */
#define TAS5518_SERIAL_DATA_IF       0x0E	 /* Serial data interface register  */
#define TAS5518_SOFT_MUTE            0x0F	 /* Soft mute register */
#define TAS5518_ENERGY_MGR           0x10	 /* Energy Managers Register */
#define TAS5518_OSC_TRIM             0x12	 /* Oscillator trim register */
#define TAS5518_AUTOMUTE_CTRL        0x14	 /* Automute control register */
#define TAS5518_AM_PWM_THRESHOLD     0x15	 /* Automute PWM threshold and back-end reset period register*/
#define TAS5518_MODULATION_LIMIT     0x16        /* Modulation Limit Reg 1-4 with 2 channels per register */
#define TAS5518_IC_DELAY(X)         (0x1B + (X)) /* IC Delay Channel 1-8 */
#define TAS5518_CHANNEL_SHUTDOWN     0x27	     /* Individual Channel Shutdown */
#define TAS5518_INPUT_MUX(X)        (0x30 + (X)) /* Input MUX 1-4 with 2 channels per register byte */
#define TAS5518_PWM_MUX(X)          (0x34 + (X)) /* Input MUX 1-4 with 2 channels per register byte */
#define TAS5518_CHANNEL_VOL(X)      (0xD0 + (X)) /* Channel 1-8 Volume*/
#define TAS5518_MASTER_VOL           0xD9	     /* Master volume  */

#define TAS5518_MAX_REGISTER         0xE0


  /*******************************/
 /* TAS5518 Bitmasks and Values */
/*******************************/

#define TAS5518_DEVICE_ID       0x02    /* Identification code for TAS5518 */
#define TAS5518_DEVICE_ID_MASK  0x3F    /* ID Register Mask*/

#define TAS5518_DATA_RATE(val)	(val << 5)
#define TAS5518_DATA_RATE_MASK  0x0E    /* Data Rate Mask Clock Control Register*/

#define TAS5518_CLK_IDX_MCLK	0
#define TAS5518_CLK_IDX_SCLK	1

struct tas5518_init_command {
        const int size;
        const char *const data;
};

static const struct tas5518_init_command tas5518_init_sequence[] = {
        // System Control Register 1 - PWM High Pass Enabled & Hard Unmute on recovery from clock & PSVC Hi-Z disable
        { .size = 2,  .data = "\x03\x90" },
        // Modulation Index Limit Register - Limit (DCLKS) = 5 & MIN WIDTH (DCLKS) = 10 & MODULATION INDEX = 96.1%
        { .size = 2,  .data = "\x16\x04" },
        
        /*
        // Route Input Channel 1 to Output Channel 7
        { .size = 33,  .data = "\x47\x00\x80\x00\x00\
                                    \x00\x00\x00\x00\
                                    \x00\x00\x00\x00\
                                    \x00\x00\x00\x00\
                                    \x00\x00\x00\x00\
                                    \x00\x00\x00\x00\
                                    \x00\x00\x00\x00\
                                    \x00\x00\x00\x00" },

        // Channel 1 to Channel 8 Mixer
        { .size = 5,  .data = "\x49\x00\x80\x00\x00" },       
        // Channel 2 to Channel 8 Mixer
        { .size = 5,  .data = "\x4A\x00\x80\x00\x00" },   
        // Disable Channel 8 Input
        { .size = 5,  .data = "\x50\x00\x00\x00\x00" }, 
        // Set Channel 7 Output to Channel 8
        { .size = 13,  .data = "\xB0\x70\x80\x00\x00\
                                    \x00\x00\x00\x00\
                                    \x00\x00\x00\x00" }, 
        
        // Biquad Filter 50 Hz Lowpass on Channel 8
        { .size = 5,  .data = "\x84\x00\x00\x00\x69" },
        // Biquad Filter 100 Hz Lowpass
        { .size = 5,  .data = "\x85\x00\x00\x00\xD3" }, 
        // Biquad Filter 100 Hz Lowpass
        { .size = 5,  .data = "\x86\x00\x00\x00\x69" }, 
        // Biquad Filter 100 Hz Lowpass
        { .size = 5,  .data = "\x87\x00\xFE\xB5\xE1" }, 
        // Biquad Filter 100 Hz Lowpass
        { .size = 5,  .data = "\x88\xFF\x81\x48\x78" }, 
        */
};