/*
 * =================================================================
 *
 *
 *	Description:  samsung display common file
 *
 *	Author: samsung display driver team
 *	Company:  Samsung Electronics
 *
 * ================================================================
 */

#include "ss_ddi_spi_common.h"

static struct spi_device *ss_spi;
static DEFINE_MUTEX(ss_spi_lock);

int ss_spi_read(struct spi_device *spi, u8 rxaddr, u8 *buf, int size)
{
	int i;
    u16 cmd_addr;
	u16 rx_buf[256];

    struct spi_message msg;
    int ret;

    struct spi_transfer xfer_tx = {
		.len        = 2,
		.tx_buf     = (u8 *)&cmd_addr,
    };
    struct spi_transfer xfer_rx = {
		.len        = 0,
		.rx_buf        = rx_buf,
    };

	mutex_lock(&ss_spi_lock);

    pr_debug("[mdss spi] %s ++ \n", __func__);

    if (!spi) {
		pr_err("[mdss spi] %s : no spi device..\n", __func__);
		return 0;
    }

	xfer_rx.len = size * 2;
    cmd_addr = (0x0 << 8) | rxaddr;

    spi_message_init(&msg);
    spi_message_add_tail(&xfer_tx, &msg);
    spi_message_add_tail(&xfer_rx, &msg);

    ret = spi_sync(spi, &msg);
	if (ret) {
		pr_err("[mdss spi] %s : spi_sync fail..\n", __func__);
		return -EINVAL;
	}

	pr_debug("[mdss spi] rx(0x%x) : ", rxaddr);
	for (i = 0; i < size; i++) {
		buf[i] = rx_buf[i] & 0xFF;
		pr_debug("%02x ", buf[i]);
	}
	pr_debug("\n");

    pr_debug("[mdss spi] %s -- \n", __func__);

	mutex_unlock(&ss_spi_lock);

    return size;
}

static int ss_spi_probe(struct spi_device *client)
{
	int ret = 0;
	struct device_node *np;
	struct samsung_display_driver_data *vdd = samsung_get_vdd();

	pr_info("[mdss] %s : ++ \n", __func__);

	if (client == NULL) {
		pr_err("[mdss] %s : ss_spi_probe spi is null\n", __func__);
		return -1;
	}

	np = client->master->dev.of_node;
	if (!np) {
		LCD_ERR("of_node is null..\n");
		return -ENODEV;
	}

	client->max_speed_hz = 8000000;
	client->bits_per_word = 9;
	client->mode =  SPI_MODE_0;

	ret = spi_setup(client);

	if (ret < 0) {
		pr_err("[mdss] %s : spi_setup error (%d)\n", __func__, ret);
		return ret;
	}

	ss_spi = client;
	vdd->spi_dev = ss_spi;

	// parse spi gpios..
	vdd->copr.copr_spi_gpio.clk = of_get_named_gpio(np, "qcom,gpio-clk", 0);
	if (!gpio_is_valid(vdd->copr.copr_spi_gpio.clk))
		LCD_ERR("failed to get spi gpio clk\n");
	vdd->copr.copr_spi_gpio.miso = of_get_named_gpio(np, "qcom,gpio-miso", 0);
	if (!gpio_is_valid(vdd->copr.copr_spi_gpio.miso))
		LCD_ERR("failed to get miso gpio miso\n");
	vdd->copr.copr_spi_gpio.mosi = of_get_named_gpio(np, "qcom,gpio-mosi", 0);
	if (!gpio_is_valid(vdd->copr.copr_spi_gpio.mosi))
		LCD_ERR("failed to get miso gpio mosi\n");
	vdd->copr.copr_spi_gpio.cs = of_get_named_gpio(np, "qcom,gpio-cs1", 0);
	if (!gpio_is_valid(vdd->copr.copr_spi_gpio.cs))
		LCD_ERR("failed to get miso gpio cs1\n");

	LCD_ERR("clk(%d) miso(%d) mosi(%d) cs1(%d)\n",
		vdd->copr.copr_spi_gpio.clk,
		vdd->copr.copr_spi_gpio.miso,
		vdd->copr.copr_spi_gpio.mosi,
		vdd->copr.copr_spi_gpio.cs);

	pr_info("[mdss] %s : -- \n", __func__);
	return ret;
}

static int ss_spi_remove(struct spi_device *spi)
{
	pr_err("[mdss] %s : remove \n", __func__);
	return 0;
}

static const struct of_device_id ddi_spi_match_table[] = {
	{   .compatible = "ss,ddi-spi",
	},
	{}
};

static struct spi_driver ddi_spi_driver = {
	.driver = {
		.name		= DRIVER_NAME,
		.owner		= THIS_MODULE,
		.of_match_table = ddi_spi_match_table,
	},
	.probe		= ss_spi_probe,
	.remove		= ss_spi_remove,
};

static int __init ss_spi_init(void)
{
	int ret;

	pr_info("[mdss] %s : ++ \n", __func__);

	ret = spi_register_driver(&ddi_spi_driver);
	if (ret)
		pr_err("[mdss] %s : ddi spi register fail : %d\n", __func__, ret);

	pr_info("[mdss] %s : -- \n", __func__);
	return ret;
}
module_init(ss_spi_init);

static void __exit ss_spi_exit(void)
{
	spi_unregister_driver(&ddi_spi_driver);
}
module_exit(ss_spi_exit);

MODULE_DESCRIPTION("spi interface driver for ddi");
MODULE_LICENSE("GPL");
