#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/of.h>
#include "ssd1306.h"

#define DEVICE_NAME "spi_driver"
#define CLASS_NAME "spi_class"

static int major_number;
static struct class *spi_class = NULL;
static struct device *spi_device = NULL;

static char message[256] = {};
static short size_of_message;

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops = {
  .open = dev_open,
  .read = dev_read,
  .write = dev_write,
  .release = dev_release,
};

static struct spi_device *etx_spi_device = NULL;

int etx_spi_write(uint8_t data){
  int ret = 0;
  uint8_t rx = 0x00;

  if(etx_spi_device){
    struct spi_transfer tr = {
      .tx_buf = &data,
      .rx_buf = &rx,
      .len = 1,
    };
    ret = spi_sync_transfer(etx_spi_device, &tr, 1);
    if(ret<0){
      printk(KERN_ALERT "spi_sync_transfer fail");
    }
  }
  return(ret);
}

static int etx_spi_probe(struct spi_device *spi){
  printk(KERN_INFO "SPI_Driver: OLED SPI Device Probed!\n");

  etx_spi_device = spi;
  etx_spi_device->bits_per_word = 8;
  if(spi_setup(etx_spi_device)){
    pr_err("Failed to setup SPI slave.\n");
    return -EFAULT;
  }

  int ret = ETX_SSD1306_GpiodInit(spi);
    if (ret < 0) {
        pr_err("ERROR: Failed to init gpiod from DT\n");
        return ret;
    }

  ETX_SSD1306_DisplayInit();
  ETX_SSD1306_InvertDisplay(false);

  ETX_SSD1306_StartScrollHorizontal(true,0,2);

  ETX_SSD1306_SetCursor(0,0);
  ETX_SSD1306_String("Welcone\nTo\nEmbeTronicX\n");

  ETX_SSD1306_SetCursor(4,35);
  ETX_SSD1306_String("SPI Linex\n");

  ETX_SSD1306_SetCursor(5,23);
  ETX_SSD1306_String("Device Driver\n");

  ETX_SSD1306_SetCursor(6,37);
  ETX_SSD1306_String("Tutorial\n");

  msleep(9000);

  ETX_SSD1306_DeactivateScroll();
  ETX_SSD1306_ClearDisplay();

  return 0;
}

static void etx_spi_remove(struct spi_device *spi){
  ETX_SSD1306_GpiodDeInit();

    // 對應 init 裡面的註冊清理
    device_destroy(spi_class, MKDEV(major_number, 0));
    class_destroy(spi_class);
    unregister_chrdev(major_number, DEVICE_NAME);
  printk(KERN_INFO "SPI_Driver: OLED SPI Device Remove!\n");
}

static const struct of_device_id etx_spi_of_match[] = {
  {.compatible = "peng,spi-dummy-driver"},
  {}
};

static const struct spi_device_id etx_spi_id[] = {
  { "spi-dummy-driver", 0 },
  { }
};
MODULE_DEVICE_TABLE(spi, etx_spi_id);

static struct spi_driver etx_spi_driver = {
  .driver = {
    .name = "etx-spi-sdd1306-driver",
    .owner = THIS_MODULE,
    .of_match_table = of_match_ptr(etx_spi_of_match),
  },
  .probe = etx_spi_probe,
  .remove = etx_spi_remove,
  .id_table = etx_spi_id,
};

static int __init spi_driver_init(void){
  printk(KERN_INFO "SPI_Driver: Initializing the SPI Driver\n");

  int ret;
  
  major_number = register_chrdev(0, DEVICE_NAME, &fops);
  if(major_number < 0 ){
    printk(KERN_ALERT "SPI_Driver failed to register  a major number\n");
    return major_number;
  }

  spi_class = class_create(CLASS_NAME);
  if(IS_ERR(spi_class)){
    unregister_chrdev(major_number, DEVICE_NAME);
    return PTR_ERR(spi_class);
  }

  spi_device = device_create(spi_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
  if(IS_ERR(spi_device)){
    class_destroy(spi_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    return PTR_ERR(spi_device);
  }

  ret = spi_register_driver(&etx_spi_driver);
  if(ret <0){
    printk(KERN_ALERT "Failed to register SPI driver\n");
    return ret;
  }

  printk(KERN_INFO "SPI_Driver: device class and SPI driver registered correctly\n");
  return 0;
}

static void __exit spi_driver_exit(void){

  spi_unregister_driver(&etx_spi_driver);
  printk(KERN_INFO "SPI_Driver: Goodbye from the Kernel!\n");
}

static int dev_open(struct inode *inodep, struct file *filep){
  printk(KERN_INFO "SPI_Driver: Device has been opened\n");
  return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
  int error_count = 0;
  if(*offset > 0) return 0;

  size_of_message = sprintf(message, "Dummy Data from SPI Driver!");
  error_count = copy_to_user(buffer, message, size_of_message);

  if(error_count == 0){
    *offset += size_of_message;
    return size_of_message;
  }
  else{
    printk(KERN_INFO "SPI_Driver: Failed to send data to user\n");
    return -EFAULT;
  }
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
  char kernel_buffer[256];
  size_t max_len = (len < sizeof(message)-1 ? len:sizeof(message) -1);
  if(copy_from_user(kernel_buffer, buffer, max_len)){
    return -EFAULT;
  }
  kernel_buffer[max_len] = '\0';

  printk(KERN_INFO "SPI_Driver: Received %zu size bytes from the user: %s\n", max_len, kernel_buffer);
  

  ETX_SSD1306_ClearDisplay();
  ETX_SSD1306_SetCursor(0,0);
  ETX_SSD1306_String(kernel_buffer);

  return len;
}

static int dev_release(struct inode *inodep, struct file *filep){
  printk(KERN_INFO "SPI_Driver: Device successfully closed\n");
  return 0;
}

module_init(spi_driver_init);
module_exit(spi_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Peng");
MODULE_DESCRIPTION("A simple spi dummy driver for software logic testing");