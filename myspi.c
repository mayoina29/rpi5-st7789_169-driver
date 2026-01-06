#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/spi/spi.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>

struct myspi_dev {
    struct spi_device *spi;
    struct gpio_desc *reset_gpio;
    struct gpio_desc *dc_gpio;
};

static int myspi_prove(struct spi_device *spi){
        struct myspi_dev *dev;

        pr_info("ST7789V2_169 Driver:  Probing device... Start GPIO Setup!\n");

        dev = devm_kzalloc(&spi->dev, sizeof(*dev), GFP_KERNEL);
        if(!dev) return -ENOMEM;

        dev->spi = spi;
        spi_set_drvdata(spi, dev);

        dev->reset_gpio = devm_gpiod_get(&spi->dev, "reset", GPIOD_OUT_HIGH);
        if(IS_ERR(dev->reset_gpio)){
            pr_err("ST7789V2: Failed to get reset gpio\n");
            return PTR_ERR(dev->reset_gpio);
        };
        
        dev->dc_gpio = devm_gpiod_get(&spi->dev, "dc" , GPIOD_OUT_LOW);
        if(IS_ERR(dev->dc_gpio)){
            pr_err("ST7789V2: Failde to get dc gpio\n");
            return PTR_ERR(dev->dc_gpio);
        };

        pr_info("ST7789V2: GPIOs acquired successfully!\n");

        gpiod_set_value(dev->reset_gpio, 1);
        msleep(50);

        gpiod_set_value(dev->reset_gpio, 0);
        msleep(50);

        gpiod_set_value(dev->reset_gpio, 1);
        msleep(150);

        pr_info("ST7789V2: Initialization sequence finished. LCD is ready!\n");

        return 0;
}

static void myspi_remove(struct spi_device *spi){
	pr_info("ST7789V2_169: Removing driver... Goodbye!\n");
}

static const struct of_device_id myspi_dt_ids[] = {
	{ .compatible = "my,st7789v2", },
	{ }
};
MODULE_DEVICE_TABLE(of, myspi_dt_ids);

static struct spi_driver myspi_driver = {
	.driver = {
		.name = "myspi_st7789",
		.owner = THIS_MODULE,
		.of_match_table = myspi_dt_ids,
	},
	.probe = myspi_prove,
	.remove = myspi_remove,
};
module_spi_driver(myspi_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Me");
MODULE_DESCRIPTION("Simple SPI Driver for ST7789V2");	
