#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/spi/spi.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>

#define DRV_WIDTH 240 
#define DRV_HEIGHT 280

struct myspi_dev {
    struct spi_device *spi;
    struct gpio_desc *reset_gpio;
    struct gpio_desc *dc_gpio;
    struct fb_info *info;
    void *vram;
    u32 vram_size;
    u32 pseudo_palette[16];
};

static void write_command(struct myspi_dev *dev, u8 cmd){
    gpiod_set_value(dev->dc_gpio, 0);

    spi_write(dev->spi, &cmd, 1);
}

static void write_data(struct myspi_dev *dev, u8 data){
    gpiod_set_value(dev->dc_gpio, 1);

    spi_write(dev->spi, &data, 1);
}

static void st7789_set_window(struct myspi_dev *dev, u16 x, u16 y, u16 w, u16 h){

        u16 x_end = x + w -1;
        u16 y_end = y + h -1;
        
        
        int y_off =20;
        y += y_off;
        y_end += y_off;

        // CASET (0x2A) x 좌표 (열) 설정
        write_command(dev, 0x2A);
        write_data(dev, x >> 8);
        write_data(dev, x & 0xFF);
        write_data(dev, x_end >> 8);
        write_data(dev, x_end & 0xFF);
        
        // RASET (0x2B: y좌표 (행) 설정
        write_command(dev, 0x2B);
        write_data(dev, y >> 8);
        write_data(dev, y & 0xFF);
        write_data(dev, y_end >> 8);
        write_data(dev, y_end & 0xFF);

        // RAMWR (0x2C) 메모리 쓰기 시작 알림 
        write_command(dev, 0x2C);
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

        /*//1.69인치 240x280 오프셋 보정 
        write_command(dev, 0x2A); //CASET 
        write_data(dev, 0); write_data(dev,0);
        write_data(dev, 0); write_data(dev,239);

        write_command(dev, 0x2B); //RASET (offset +20) 
        write_data(dev, 0); write_data(dev,20);
        write_data(dev, 300>>8); write_data(dev,300 & 0xFF);
       */
       st7789_set_window(dev, 0,0, DRV_WIDTH, DRV_HEIGHT);

        // INVON (0x21): 색상 반전 켜기 (IPS 패널 특성상 반전이 필요할 수 있음
        // 화면이 색반전되면 이걸 주석처리하거나 INVOFF(0x20) 을 사용하자 
        write_command(dev, 0x21);

        // NORON (0x13): Normal Display Mode on 
        write_command(dev, 0x13);

        // DISPON (0x29): Display ON 
        write_command(dev, 0x29);

        pr_info("ST7789V2: Initialization done. Display should be On \n");
}

static struct fb_ops myspi_fb_ops = {
        .owner = THIS_MODULE,
        .fb_read = fb_sys_read,
        .fb_write = fb_sys_write,
        .fb_fillrect = sys_fillrect,
        .fb_copyarea = sys_copyarea,
        .fb_imageblit = sys_imageblit,
        .fb_mmap = fb_deferred_io_mmap,
};


/*static void st7789_fill(struct myspi_dev *dev, u16 color){
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

static int myspi_open(struct inode *inode, struct file *file){
        struct myspi_dev *dev = container_of(file->private_data, struct myspi_dev, miscdev);

        file->private_data = dev;

        pr_info("ST7789V2: Device opened\n");
        return 0;
}

static ssize_t myspi_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos){
        struct myspi_dev *dev = file->private_data;
        int ret;

        //버퍼 오버 플로우 방지 
        if(count >= sizeof(dev->kbuf)) count = sizeof(dev->kbuf) -1;

        //유저 공간의 데이터를 커널 공간으로 복사 
        ret = copy_from_user(dev->kbuf, user_buf, count);
        if (ret) return -EFAULT;

        dev->kbuf[count] = '\0';

        if(dev->kbuf[count-1] == '\n') dev->kbuf[count-1] = '\0';

        pr_info("ST7789V2: User wrote: %s\n", dev->kbuf);

        if (strcmp(dev->kbuf, "red") ==0) st7789_fill(dev, 0xF800);
        else if (strcmp(dev->kbuf, "green") == 0) st7789_fill(dev, 0x07E0);
        else if (strcmp(dev->kbuf, "bule") == 0) st7789_fill(dev, 0x001F);
        else if (strcmp(dev->kbuf, "white") == 0) st7789_fill(dev, 0xFFFF);
        else if (strcmp(dev->kbuf, "black") == 0) st7789_fill(dev, 0x0000);
        else if (strcmp(dev->kbuf, "natsuki") == 0) st7789_fill(dev, 0xFA8D);
        else pr_info("ST7789: Unknown command\n");

        return count;
}

static const struct file_operations myspi_fops = {
       .owner = THIS_MODULE,
       .open = myspi_open,
       .write = myspi_write,
};
*/

static void myspi_fb_deferred_io(struct fb_info *info, struct list_head *pagelist){
        
        struct myspi_dev *dev = info->par;
        u16 *vram16 = (u16 *)dev->vram;

        u8 txbuf[480];
        int x,y;
        
        pr_info("ST7789V2: Drawing screen...\n");
        st7789_set_window(dev, 0,0, DRV_WIDTH, DRV_HEIGHT);

        gpiod_set_value(dev->dc_gpio, 1);

        for(y= 0; y <DRV_HEIGHT; y++){
            for (x=0; x<DRV_WIDTH; x++){
                u16 pixel = vram16[y * DRV_WIDTH + x];
                txbuf[x*2] = pixel >> 8;
                txbuf[x*2+1] = pixel & 0xFF;
            }
            spi_write(dev->spi, txbuf, DRV_WIDTH *2);
        }
}

static struct fb_deferred_io myspi_defio ={
        .delay = HZ / 20,
        .deferred_io = myspi_fb_deferred_io,
};





static int myspi_probe(struct spi_device *spi){
        struct myspi_dev *dev;
        struct fb_info *info;
        int ret;
        u32 vram_size;
        u8 *vram;

        pr_info("ST7789V2_169 :  Probing FB setup...\n");

        //fb_info 구조체 할당 
        info = framebuffer_alloc(sizeof(struct myspi_dev), &spi->dev);
        if(!info) return -ENOMEM;

        dev = info->par;
        dev->spi = spi;
        dev->info  = info;
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

        //가상 vram 할당 
        //크기 가로 x 세로 x 2바이트 16bit 
        vram_size = DRV_WIDTH * DRV_HEIGHT * 2;
        vram = vmalloc(vram_size);
        if(!vram) return -ENOMEM;
        memset(vram, 0 , vram_size);

        dev->vram = vram;
        dev->vram_size = vram_size;

        strcpy(info->fix.id, "ST7789");

        //fb_info 설정 (모니터 스펙작성)
        info->fbops = &myspi_fb_ops;
        info->screen_base = (char __iomem *)vram;//vram 주소 등록
        info->screen_size = vram_size;

        //고정 정보
        info->fix.type = FB_TYPE_PACKED_PIXELS;
        info->fix.visual = FB_VISUAL_TRUECOLOR;
        info->fix.accel = FB_ACCEL_NONE;
        info->fix.line_length = DRV_WIDTH*2;
        info->fix.smem_start = (unsigned long)vram;
        info->fix.smem_len = vram_size;

        // 변동 정보 - 해상도 색상깊이
        info->var.xres = DRV_WIDTH;
        info->var.yres = DRV_HEIGHT;
        info->var.xres_virtual = DRV_WIDTH;
        info->var.yres_virtual = DRV_HEIGHT;
        info->var.bits_per_pixel = 16;

        // RGB565 포맷 정의 (R 5bit G 6bit B 5bit )
        info->var.red.offset = 11; info->var.red.length = 5;
        info->var.green.offset = 5; info->var.green.length = 6;
        info->var.blue.offset = 0; info->var.blue.length = 5;
        info->var.activate = FB_ACTIVATE_NOW;

        info->pseudo_palette = dev->pseudo_palette;
        
        info->fbdefio = &myspi_defio;
        fb_deferred_io_init(info);

        //프레임 버퍼 등록
        ret = register_framebuffer(info);
        if(ret < 0){
            pr_err("ST7789V2: Failed to register framebuffer\n");
            return ret;
        }

        pr_info("ST7789V2: framebuffer /dev/fbX registered!\n");
        return 0;
}

static void myspi_remove(struct spi_device *spi){
    struct myspi_dev *dev = spi_get_drvdata(spi);
    //misc_deregister(&dev->miscdev);

    //등록 해제
    if(dev->info){
        fb_deferred_io_cleanup(dev->info);
        unregister_framebuffer(dev->info);
        framebuffer_release(dev->info);
    }
    if(dev->vram) vfree(dev->vram);
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
	.probe = myspi_probe,
	.remove = myspi_remove,
};
module_spi_driver(myspi_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Me");
MODULE_DESCRIPTION("Simple SPI Driver for ST7789V2");	
