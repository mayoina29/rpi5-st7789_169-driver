#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/spi/spi.h>


static int myspi_prove(struct spi_device *spi){
        pr_info("ST7789V2_169 Driver: Good News! Device Detected on CS %d!\n", (int)spi->chip_select[0]);

        return 0;
}

static void myspi_remove(struct spi_device *spi){
	pr_info("ST7789V2_169 Driver: Goodbye!\n");
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
