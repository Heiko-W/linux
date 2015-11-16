/*
 * ASoC Driver for openHiFi DA-2 
 *
 * Copyright (C) 2015 Heiko Wilke <heiko.wilke@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
 
#include <linux/module.h>
#include <linux/platform_device.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/jack.h>
 
static int snd_rpi_openhifi_da_2_init(struct snd_soc_pcm_runtime *rtd)
{
        // Add additional Init Code here
        return 0;
}


static int snd_rpi_openhifi_da_2_hw_params(struct snd_pcm_substream *substream,
                                               struct snd_pcm_hw_params *params)
{
        struct snd_soc_pcm_runtime *rtd = substream->private_data;
        struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
        
        return snd_soc_dai_set_bclk_ratio(cpu_dai, 64);
}


static struct snd_soc_ops snd_rpi_openhifi_da_2_ops = {
        .hw_params = snd_rpi_openhifi_da_2_hw_params,
};


static struct snd_soc_dai_link snd_rpi_openhifi_da_2_dai[] = {
        {
                .name           = "openHiFi DA-2",
                .stream_name    = "openHiFi DA-2 Stereo",
                .cpu_dai_name   = "bcm2708-i2s.0",
                .codec_dai_name = "tas5518",
                .platform_name  = "bcm2708-i2s.0",
                .codec_name     = "tas5518.1-001b",
                .dai_fmt        = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
                .ops            = &snd_rpi_openhifi_da_2_ops,
                .init           = snd_rpi_openhifi_da_2_init,
        },
};


static struct snd_soc_card snd_rpi_openhifi_da_2 = {
        .name           = "snd_rpi_openhifi_da_2",
        .dai_link       = snd_rpi_openhifi_da_2_dai,
        .num_links      =ARRAY_SIZE(snd_rpi_openhifi_da_2_dai),
};

static const struct of_device_id snd_rpi_openhifi_da_2_of_match[] = {
        {.compatible = "openhifi,openhifi-da-2",},
        {},
};
MODULE_DEVICE_TABLE(of, snd_rpi_openhifi_da_2_of_match);


static int snd_rpi_openhifi_da_2_probe(struct platform_device *pdev)
{
        int ret = 0;
        
        snd_rpi_openhifi_da_2.dev = &pdev->dev;
        
        if(pdev->dev.of_node) {
                struct device_node *i2s_node;
                struct snd_soc_dai_link *dai = &snd_rpi_openhifi_da_2_dai[0];
                i2s_node = of_parse_phandle(pdev->dev.of_node, "i2s-controller", 0);
                
                if(i2s_node) {
                        dai->cpu_dai_name = NULL;
                        dai->cpu_of_node = i2s_node;
                        dai->platform_name = NULL;
                        dai->platform_of_node = i2s_node;
                }
        }
        
        ret = snd_soc_register_card(&snd_rpi_openhifi_da_2);
        
        if(ret != 0) {
                dev_err(&pdev->dev, "snd_soc_register_card() failed: %d\n", ret);
        }
        
        return ret;
}


static int snd_rpi_openhifi_da_2_remove(struct platform_device *pdev)
{
        return snd_soc_unregister_card(&snd_rpi_openhifi_da_2);
}


static struct platform_driver snd_rpi_openhifi_da_2_driver = {
        .driver = {
                .name           = "openhifi-da-2",
                .owner          = THIS_MODULE,
                .of_match_table = snd_rpi_openhifi_da_2_of_match,
        },
        .probe          = snd_rpi_openhifi_da_2_probe,
        .remove         = snd_rpi_openhifi_da_2_remove,
};

module_platform_driver(snd_rpi_openhifi_da_2_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Heiko Wilke <heiko.wilke@gmail.com>");
MODULE_DESCRIPTION("ASoC Codec Driver for openHiFi DA-2");