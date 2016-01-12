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
#include <linux/i2c.h>
#include <linux/of_device.h>
#include <linux/regmap.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include "tas5518.h"
 
#define TAS5518_PCM_FORMATS (SNDRV_PCM_FMTBIT_S16_LE  |		\
			     SNDRV_PCM_FMTBIT_S20_3LE |		\
			     SNDRV_PCM_FMTBIT_S24_3LE)

#define TAS5518_PCM_RATES   (SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100  | \
			     SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200  | \
			     SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_176400 | \
			     SNDRV_PCM_RATE_192000)
                 
  /*****************/
 /* ALSA Controls */
/*****************/

static const DECLARE_TLV_DB_SCALE(tas5518_db_scale, -12725, 25, 1);

static const struct snd_kcontrol_new tas5518_snd_controls[] = {
        SOC_SINGLE_TLV ("Master Volume",    TAS5518_MASTER_VOL,
                         0, 0x0245, 1, tas5518_db_scale),
	SOC_DOUBLE_R_TLV("Channel 1/2 Volume",
			 TAS5518_CHANNEL_VOL(1), TAS5518_CHANNEL_VOL(2),
			 0, 0x0245, 1, tas5518_db_scale),
        SOC_DOUBLE_R_TLV("Channel 3/4 Volume",
                         TAS5518_CHANNEL_VOL(3), TAS5518_CHANNEL_VOL(4),
                         0, 0x0245, 1, tas5518_db_scale),
        SOC_DOUBLE_R_TLV("Channel 5/6 Volume",
                         TAS5518_CHANNEL_VOL(5), TAS5518_CHANNEL_VOL(6),
                         0, 0x0245, 1, tas5518_db_scale),
        SOC_DOUBLE_R_TLV("Channel 7/8 Volume",
                         TAS5518_CHANNEL_VOL(7), TAS5518_CHANNEL_VOL(8),
                         0, 0x0245, 1, tas5518_db_scale)
};

struct tas5518_priv {
        struct regmap        *regmap;
        unsigned int         mclk, sclk;
        unsigned int         format;
        int                  rate; 
        struct snd_soc_codec *codec;
};

static struct i2c_client *i2c;

  /***********************************/ 
 /* Codec DAI and PCM configuration */
/***********************************/

static int tas5518_hw_params(struct snd_pcm_substream *substream,
                                struct snd_pcm_hw_params *params,
                                struct snd_soc_dai *dai)
{
        int ret;
        u8 blen = 0;
        
        struct snd_soc_codec *codec = dai->codec;
        struct tas5518_priv *priv_data = snd_soc_codec_get_drvdata(codec);
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
        
        ret = regmap_update_bits(priv_data->regmap, TAS5518_SERIAL_DATA_IF, (u8)0x07, blen);
        if (ret <0)
                return ret;
                
        return 0;
}

// Codec Mute Function
static int tas5518_mute(struct snd_soc_dai *dai, int mute) 
{
        struct snd_soc_codec *codec = dai->codec;
        struct tas5518_priv *priv_data = snd_soc_codec_get_drvdata(codec);
	
	if (mute)
                regmap_write(priv_data->regmap, TAS5518_SOFT_MUTE, (u8)0xFF);
	else
                regmap_write(priv_data->regmap, TAS5518_SOFT_MUTE, (u8)0x00);
        return 0;
}

// Codec Init Function
static int tas5518_init(struct device *dev, struct tas5518_priv *priv_data)
{
        int ret;
        
        // Reset Error
        ret = regmap_write (priv_data->regmap, TAS5518_ERROR_STATUS, (u8)0x00);
        if (ret < 0) 
                return ret;
              
        /* Set System Control Register */
        ret = regmap_write(priv_data->regmap, TAS5518_SYS_CONTROL_1, (u8)0x90);
        if (ret < 0) 
                return ret;
                
        /* Set Modulation Limit Register*/
        ret = regmap_write(priv_data->regmap, TAS5518_MODULATION_LIMIT, (u8)0x04);
        if (ret < 0) 
                return ret;
                
        /* Set Master Volume */
        ret = regmap_write(priv_data->regmap, TAS5518_MASTER_VOL, (int)0x90);
        if (ret < 0) 
                return ret;
                
        return 0;
}

static struct snd_soc_dai_ops tas5518_dai_ops = {
	.hw_params     = tas5518_hw_params,
	.digital_mute  = tas5518_mute,
};

struct snd_soc_dai_driver tas5518_dai = {
	.name = "tas5518-hifi",
	.playback = {
		.stream_name  = "Playback",
		.channels_min = 2,
		.channels_max = 8,
		.rates        = TAS5518_PCM_RATES,
		.formats      = TAS5518_PCM_FORMATS,
	},
	.ops = &tas5518_dai_ops,
};

  /*********************/
 /* Codec Driver Part */
/*********************/
 
 static int tas5518_probe(struct snd_soc_codec *codec) 
 {
        struct tas5518_priv *priv_data = snd_soc_codec_get_drvdata(codec);
        int ret, i;
        
        i2c = container_of(codec->dev, struct i2c_client, dev);
                
        // Write Init Sequence to Codec
        for (i = 0 ; i < ARRAY_SIZE(tas5518_init_sequence); ++i) {
                ret = i2c_master_send(i2c, tas5518_init_sequence[i].data, tas5518_init_sequence[i].size);
                if(ret < 0) {
                        printk(KERN_INFO "TAS5518 Codec Probe: Init Sequence returns: %d\n", ret);
                }
        }

        // Init Codec
        ret = tas5518_init(codec->dev, priv_data);
        if (ret < 0) 
                return ret;
        
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


  /**************/
 /* I2C Driver */
/**************/

static const struct reg_default tas5518_reg_defaults[] = {
        {0x00, 0x6C},
        {0x01, 0x02},
        {0x02, 0x00},
        {0x03, 0x90},
        {0x04, 0x00},
        {0x05, 0xE0},
        {0x06, 0xE0},
        {0x07, 0xE0},
        {0x08, 0xE0},
        {0x09, 0xE0},
        {0x0A, 0xE0},
        {0x0B, 0xE0},
        {0x0C, 0xE0},
        {0x0D, 0x00},
        {0x0E, 0x05},
        {0x0F, 0x00},
        {0x14, 0x84},
        {0x15, 0x05},
        {0x16, 0x02},
        {0xD1, 0x0245},
        {0xD2, 0x0245},
        {0xD3, 0x0245},
        {0xD4, 0x0245},
        {0xD5, 0x0245},
        {0xD6, 0x0245},
        {0xD7, 0x0245},
        {0xD8, 0x0245},
        {0xD9, 0x0245},
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

static int tas5518_register_size(struct device *dev, unsigned int reg)
{
	switch (reg) {
                case 0x00 ... 0x23:
                        return 1;
                case 0xD0 ... 0xFF:
                        return 4;
	}

	dev_err(dev, "Unsupported register address: %d\n", reg);
	return 0;
}

static bool tas5518_accessible_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
                case 0x10 ... 0x13:
                case 0x17 ... 0x1A:
                case 0x24 ... 0x3F:
                case 0xB2 ... 0xCE:
                case 0xE1 ... 0xFD:
                case 0xFF:
                        return false;
                default:
                        return true;
	}
}

static bool tas5518_writeable_reg(struct device *dev, unsigned int reg)
{
	return tas5518_accessible_reg(dev, reg) && (reg != TAS5518_DEV_ID);
}

static int tas5518_reg_write(void *context, unsigned int reg,
			      unsigned int value)
{
        struct i2c_client *client = context;
	unsigned int i, size;
	uint8_t buf[5];
	int ret;

	size = tas5518_register_size(&client->dev, reg);
	if (size == 0)
		return -EINVAL;

	buf[0] = reg;

	for (i = size; i >= 1; --i) {
		buf[i] = value;
		value >>= 8;
	}

	ret = i2c_master_send(client, buf, size + 1);
	if (ret == size + 1)
		return 0;
	else if (ret < 0)
		return ret;
	else
		return -EIO;
}
                              
static int tas5518_reg_read(void *context, unsigned int reg,
			     unsigned int *value)
{
        struct i2c_client *client = context;
	uint8_t send_buf, recv_buf[4];
	struct i2c_msg msgs[2];
	unsigned int size;
	unsigned int i;
	int ret;

	size = tas5518_register_size(&client->dev, reg);
	if (size == 0)
		return -EINVAL;

	send_buf = reg;

	msgs[0].addr = client->addr;
	msgs[0].len = sizeof(send_buf);
	msgs[0].buf = &send_buf;
	msgs[0].flags = 0;

	msgs[1].addr = client->addr;
	msgs[1].len = size;
	msgs[1].buf = recv_buf;
	msgs[1].flags = I2C_M_RD;

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0)
		return ret;
	else if (ret != ARRAY_SIZE(msgs))
		return -EIO;

	*value = 0;

	for (i = 0; i < size; i++) {
		*value <<= 8;
		*value |= recv_buf[i];
	}

	return 0;
}

static const struct of_device_id tas5518_of_match[] = {
	 { .compatible = "ti,tas5518",},
	 {}
};
MODULE_DEVICE_TABLE(of, tas5518_of_match);

 
static struct regmap_config tas5518_regmap_config = {
        .reg_bits               = 8,
        .val_bits               = 32,
        .max_register           = TAS5518_MAX_REGISTER,
        .reg_defaults           = tas5518_reg_defaults,
        .num_reg_defaults       = ARRAY_SIZE(tas5518_reg_defaults),
        .cache_type             = REGCACHE_RBTREE,
        .volatile_reg           = tas5518_reg_volatile,
        .writeable_reg          = tas5518_writeable_reg,
        .readable_reg           = tas5518_accessible_reg,
        .reg_read               = tas5518_reg_read,
        .reg_write              = tas5518_reg_write,
};


static int tas5518_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	struct tas5518_priv *priv_data;
        struct device *dev = &i2c->dev;
        int ret;
        unsigned int  i;

	priv_data = devm_kzalloc(dev, sizeof(*priv_data), GFP_KERNEL);
	if (!priv_data)
		return -ENOMEM;

	priv_data->regmap = devm_regmap_init(dev, NULL, i2c, &tas5518_regmap_config);
	if (IS_ERR(priv_data->regmap)) {
		ret = PTR_ERR(priv_data->regmap);
		dev_err(&i2c->dev, "Failed to create regmap: %d\n", ret);
		return ret;
	}

	i2c_set_clientdata(i2c, priv_data);
                
        /* Check TAS5518 ID */
        ret = regmap_read(priv_data->regmap, TAS5518_DEV_ID, &i);
        if (ret < 0) return ret;
        
        if ((i & TAS5518_DEVICE_ID_MASK) != TAS5518_DEVICE_ID) {
                printk(KERN_ERR "Wrong Device ID for TAS5518: %d\n", i);
                return -ENODEV;
        }
        
	ret = snd_soc_register_codec(&i2c->dev, &soc_codec_tas5518,
					     &tas5518_dai, 1);
	return ret;
}


static int tas5518_i2c_remove(struct i2c_client *i2c)
{
	snd_soc_unregister_codec(&i2c->dev);
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


static void __exit tas5518_exit(void)
{
        i2c_del_driver(&tas5518_i2c_driver);
}


module_init(tas5518_modinit);
module_exit(tas5518_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Heiko Wilke <heiko.wilke@gmail.com>");
MODULE_DESCRIPTION("ALSA ASoC Codec Driver for Texas Instruments TAS5518");
MODULE_ALIAS("platform:" DRV_NAME);
