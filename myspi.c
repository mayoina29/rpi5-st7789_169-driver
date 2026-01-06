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

static void write_command(struct myspi_dev *dev, u8 cmd){
    gpiod_set_value(dev->dc_gpio, 0);

    spi_write(dev->spi, &cmd, 1);
}

static void write_data(struct myspi_dev *dev, u8 data){
    gpiod_set_value(dev->dc_gpio, 1);

    spi_write(dev->spi, &data, 1);
}

static void st7789_init(struct myspi_dev *dev){
        
        pr_info("ST7789V2: Sending initialization commands...\n");

        // SWRESET (0x01): 소프트웨어 리셋
        write_command(dev, 0x01);
        msleep(150);

        // SLPOUT (0x11): Sleep Out 
        write_command(dev, 0x11);
        msleep(255);
    
        //COLMOD (0x3A): 컬러모드 설정 
        write_command(dev, 0x3A);
        write_data(dev, 0x55);

        // MADCTL (0x36): 화면 방향 설정 
        write_command(dev, 0x36);
        write_data(dev, 0x00); // 0x00 = 정방향 (필요시 0x60, 0xC0 등으로 변경)

        // INVON (0x21): 색상 반전 켜기 (IPS 패널 특성상 반전이 필요할 수 있음
        // 화면이 색반전되면 이걸 주석처리하거나 INVOFF(0x20) 을 사용하자 
        write_command(dev, 0x21);

        // NORON (0x13): Normal Display Mode on 
        write_command(dev, 0x13);

        // DISPON (0x29): Display ON 
        write_command(dev, 0x29);

        pr_info("ST7789V2: Initialization done. Display should be On \n");
}

static void st7789_set_window(struct myspi_dev *dev, u16 x_start, u16 y_start, u16 x_end, u16 y_end){

        int y_offset =20;

        y_start += y_offset;
        y_end += y_offset;

        // CASET (0x2A) x 좌표 (열) 설정
        write_command(dev, 0x2A);
        write_data(dev, x_start >> 8);
        write_data(dev, x_start & 0xFF);
        write_data(dev, x_end >> 8);
        write_data(dev, x_end & 0xFF);
        
        // RASET (0x2B: y좌표 (행) 설정
        write_command(dev, 0x2B);
        write_data(dev, y_start >> 8);
        write_data(dev, y_start & 0xFF);
        write_data(dev, y_end >> 8);
        write_data(dev, y_end & 0xFF);

        // RAMWR (0x2C) 메모리 쓰기 시작 알림 
        write_command(dev, 0x2C);
}

static void st7789_fill(struct myspi_dev *dev, u16 color){
    int i;

    int width = 240;
    int height = 280;

    u8 color_hi = color >> 8;
    u8 color_lo = color & 0xFF;

    //전체화면 영역 잡기 
    st7789_set_window(dev, 0,0, width -1, height -1);
    
    // DC 핀을 High(데이터 모드)로 고정 
    gpiod_set_value(dev->dc_gpio, 1);

    //픽셀 하나하나 색칠하기 (나중에 최적화 일단 확인을 위해 반복)
    for (i =0; i < width * height; i++){
        spi_write(dev->spi, &color_hi, 1);
        spi_write(dev->spi, &color_lo, 1);
    }
}


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

        //pr_info("ST7789V2: Initialization sequence finished. LCD is ready!\n");

        st7789_init(dev);
        
        pr_info("ST7789V2: Paninting Screen with #FE5069 (0xFA8D)...\n");

        st7789_fill(dev, 0xFA8D);

        pr_info("ST7789V2: Painting Done\n");

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
