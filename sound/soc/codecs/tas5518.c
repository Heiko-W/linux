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

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/i2c.h>
#include <linux/of_device.h>
#include <linux/regmap.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#include <linux/kernel.h>

#include "tas5518.h"
 
#define TAS5518_PCM_FORMATS (SNDRV_PCM_FMTBIT_S16_LE  |		\
			     SNDRV_PCM_FMTBIT_S20_3LE |		\
			     SNDRV_PCM_FMTBIT_S24_3LE)

#define TAS5518_PCM_RATES   (SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100  | \
			     SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200  | \
			     SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_176400 | \
			     SNDRV_PCM_RATE_192000)

/* ALSA Controls */
static const DECLARE_TLV_DB_SCALE(tas5518_db_scale, -12725, 25, 1);

static const struct snd_kcontrol_new tas5518_snd_controls[] = {
        SOC_SINGLE_TLV ("Master", TAS5518_MASTER_VOL, 0, 245, 1, tas5518_db_scale),
	SOC_SINGLE_TLV ("Channel 1", TAS5518_CHANNEL_VOL(1), 0, 245, 1, tas5518_db_scale),
	SOC_SINGLE_TLV ("Channel 2", TAS5518_CHANNEL_VOL(2), 0, 245, 1, tas5518_db_scale),
	SOC_SINGLE_TLV ("Channel 3", TAS5518_CHANNEL_VOL(3), 0, 245, 1, tas5518_db_scale),
	SOC_SINGLE_TLV ("Channel 4", TAS5518_CHANNEL_VOL(4), 0, 245, 1, tas5518_db_scale),
	SOC_SINGLE_TLV ("Channel 5", TAS5518_CHANNEL_VOL(5), 0, 245, 1, tas5518_db_scale),
	SOC_SINGLE_TLV ("Channel 6", TAS5518_CHANNEL_VOL(6), 0, 245, 1, tas5518_db_scale),
	SOC_SINGLE_TLV ("Channel 7", TAS5518_CHANNEL_VOL(7), 0, 245, 1, tas5518_db_scale),
	SOC_SINGLE_TLV ("Channel 8", TAS5518_CHANNEL_VOL(8), 0, 245, 1, tas5518_db_scale)
};

struct tas5518_priv {
	struct regmap *regmap;
	struct snd_soc_codec *codec;
};

static struct tas5518_priv *priv_data;

static struct i2c_client *i2c;

/* 
 * Codec DAI and PCM configuration 
 */

static int tas5518_hw_params(struct snd_pcm_substream *substream,
							struct snd_pcm_hw_params *params,
							struct snd_soc_dai *dai)
{
        u8 blen = 0;
        
        struct snd_soc_codec *codec;
        codec = dai->codec;
        priv_data->codec = dai->codec;
	
        switch(params_format(params)) {
                case SNDRV_PCM_FORMAT_S16_LE:
                        blen = 0x03;
                        break;
                case SNDRV_PCM_FORMAT_S24_LE:
                        blen = 0x05;
                        break;
                default:
                        dev_err(dai->dev, "Unsupported word length: %u\n", params_format(params));
                        return -EINVAL;
        }
        
        snd_soc_update_bits(codec, TAS5518_SERIAL_DATA_IF, 0x07, blen);
        
	return 0;
}

// Codec Mute Function
static int tas5518_mute(struct snd_soc_dai *dai, int mute) 
{
	struct snd_soc_codec *codec = dai->codec;
	
	u8 mute_reg = snd_soc_read(codec, TAS5518_SOFT_MUTE);
	
	if (mute)
		snd_soc_write(codec, TAS5518_MASTER_VOL, 0xFF);
	else
		snd_soc_write(codec, TAS5518_MASTER_VOL, mute_reg);
        
        return 0;
}


static struct snd_soc_dai_ops tas5518_dai_ops = {
	.hw_params		= tas5518_hw_params,
	.digital_mute	= tas5518_mute,
};

struct snd_soc_dai_driver tas5518_dai = {
	.name = "tas5518-hifi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 8,
		.rates = TAS5518_PCM_RATES,
		.formats = TAS5518_PCM_FORMATS,
	},
	.ops = &tas5518_dai_ops,
	.symmetric_rates = 1,
};

/*
 *	Codec Driver Part
 */
 
 static int tas5518_probe(struct snd_soc_codec *codec) 
 {
	struct tas5518_priv *tas5518;
        
        int i, ret;
        
        i2c = container_of(codec->dev, struct i2c_client, dev);
        
        tas5518 = snd_soc_codec_get_drvdata(codec);
        
        //Check device ID
        ret = snd_soc_read(codec, TAS5518_DEV_ID);
        if ((ret && TAS5518_DEVICE_ID_MASK) != TAS5518_DEVICE_ID) {
                printk(KERN_ERR "Wrong Device ID for TAS5518: %d\n", ret);
        }
        if (ret < 0) return ret;
        
        //Reset Error
        ret = snd_soc_write(codec, TAS5518_ERROR_STATUS, 0x00);
        if (ret < 0) return ret;
        
        // Set Volume to -40 dB
        ret = snd_soc_write(codec, TAS5518_MASTER_VOL, 0xE8);
        if (ret < 0) return ret;
        
        // Write Init Sequence to Codec
        for (i = 0 ; i < ARRAY_SIZE(tas5518_init_sequence); ++i) {
                ret = i2c_master_send(i2c, tas5518_init_sequence[i].data, tas5518_init_sequence[i].size);
                if(ret < 0) {
                        printk(KERN_INFO "TAS5518 Codec Probe: Init Sequence returns: %d\n", ret);
                }
        }
        
	return 0;	 
 }
 
 static int tas5518_remove(struct snd_soc_codec *codec) 
 {
	 struct tas5518_priv *tas5518;
	 tas5518 = snd_soc_codec_get_drvdata(codec);
	 return 0;
 }

static struct snd_soc_codec_driver soc_codec_tas5518 = {
	.probe = tas5518_probe,
	.remove = tas5518_remove,
	.controls = tas5518_snd_controls,
	.num_controls = ARRAY_SIZE(tas5518_snd_controls),
};


/**********************************
 *	I2C Driver
 **********************************/

static const struct reg_default tas5518_reg_defaults[] = {
	{0x0F, 0x00}, // Softmute Register - Unmute all channels_max
	{0xD1, 0x48}, // Channel 1 Volume Register - 0 dB
	{0xD2, 0x48}, // Channel 2 Volume Register - 0 dB
	{0xD3, 0x48}, // Channel 3 Volume Register - 0 dB
	{0xD4, 0x48}, // Channel 4 Volume Register - 0 dB
	{0xD5, 0x48}, // Channel 5 Volume Register - 0 dB
	{0xD6, 0x48}, // Channel 6 Volume Register - 0 dB
	{0xD7, 0x48}, // Channel 7 Volume Register - 0 dB
	{0xD8, 0x48}, // Channel 8 Volume Register - 0 dB
	{0xD9, 0x245}, // Master Volume Register - Master Volume = Mute
};

 
static bool tas5518_reg_volatile(struct device *dev, unsigned int reg) 
{
	 switch (reg) {
		 case TAS5518_DEV_ID:
		 case TAS5518_ERROR_STATUS:
		 	return true;
		 default:
		 	return false;
	 }
}


static const struct of_device_id tas5518_of_match[] = {
	 { .compatible = "ti,tas5518",},
	 {}
};
MODULE_DEVICE_TABLE(of, tas5518_of_match);

 
static struct regmap_config tas5518_regmap_config = {
        .reg_bits = 8,
        .val_bits = 8,
        
        .max_register = TAS5518_MAX_REGISTER,
        .volatile_reg = tas5518_reg_volatile,
        
        .cache_type = REGCACHE_RBTREE,
        .reg_defaults = tas5518_reg_defaults,
        .num_reg_defaults = ARRAY_SIZE(tas5518_reg_defaults),
};


static int tas5518_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
        int ret;
        
        priv_data = devm_kzalloc(&i2c->dev, sizeof *priv_data, GFP_KERNEL);
        if(!priv_data)
                return -ENOMEM;
        
        priv_data->regmap = devm_regmap_init_i2c(i2c, &tas5518_regmap_config);
        if(IS_ERR(priv_data->regmap)) {
                ret = PTR_ERR(priv_data->regmap);
                return ret;
        }
        
        i2c_set_clientdata(i2c, priv_data);
        
        ret = snd_soc_register_codec(&i2c->dev, &soc_codec_tas5518, &tas5518_dai, 1);
        return ret;
}


static int tas5518_i2c_remove(struct i2c_client *i2c)
{
        snd_soc_unregister_codec(&i2c->dev);
        i2c_set_clientdata(i2c, NULL);
        
        kfree(priv_data);
        
        return 0;
}


static const struct i2c_device_id tas5518_i2c_id[] = {
        {"tas5518", 0},
        {}
};
MODULE_DEVICE_TABLE(i2c, tas5518_i2c_id);


static struct i2c_driver tas5518_i2c_driver = {
        .driver = {
                .name = "tas5518",
                .owner = THIS_MODULE,
                .of_match_table = tas5518_of_match,
        },
        .probe = tas5518_i2c_probe,
        .remove = tas5518_i2c_remove,
        .id_table = tas5518_i2c_id
};


static int __init tas5518_modinit(void)
{
        int ret = 0;
        ret = i2c_add_driver(&tas5518_i2c_driver);
        if(ret) {
                printk(KERN_ERR "Failed to register tas5518 I2C driver: %d\n", ret);
        }
        return ret;
}


static int __exit tas5518_exit(void)
{
        i2c_del_driver(&tas5518_i2c_driver);
        return 0;
}


module_init(tas5518_modinit);
module_exit(tas5518_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Heiko Wilke <heiko.wilke@gmail.com>");
MODULE_DESCRIPTION("ALSA ASoC Codec Driver for Texas Instruments TAS5518");
MODULE_ALIAS("platform:" DRV_NAME);
