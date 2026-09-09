#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>

#define DEVICE_NAME "i2C_driver"
#define CLASS_NAME  "i2C_class"
#define SLAVE_DEVICE_NAME "MPU6050_device"
#define PICO_SLAVE_ADDR 0x42

static int major_number;
static struct class* i2C_class = NULL;
static struct device* i2C_driver = NULL;
struct i2c_adapter *etx_i2c_adapter = NULL;
struct i2c_client  *etx_i2c_client  = NULL;

static char message[256] = {0};


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

struct mpu6050_data {
    short accel_x;
    short accel_y;
    short accel_z;
};


static int etx_mpu_probe(struct i2c_client *client)
{
    pr_info("PICO Driver Probed successfully!\n");
    return 0;
}

static void etx_mpu_remove(struct i2c_client *client)
{   
    pr_info("PICO Driver Removed!\n");
}

// 定義 Device ID Table
static const struct i2c_device_id mpu_id_table[] = {
    { SLAVE_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, mpu_id_table);

// 定義 i2c_driver 結構
static struct i2c_driver etx_mpu_driver = {
    .driver = {
        .name   = SLAVE_DEVICE_NAME,
        .owner  = THIS_MODULE,
    },
    .probe      = etx_mpu_probe,
    .remove     = etx_mpu_remove,
    .id_table   = mpu_id_table,
};

// 驅動初始化
static int __init i2c_init(void) {
    printk(KERN_INFO "I2CDriver: Initializing the Generic Driver\n");

    // 1. 動態註冊 Major Number
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ALERT "I2CDriver failed to register a major number\n");
        return major_number;
    }

    // 2. 註冊 Device Class
    i2C_class = class_create(CLASS_NAME);
    if (IS_ERR(i2C_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(i2C_class);
    }

    // 3. 自動在 /dev 下建立裝置檔案 (/dev/i2c_driver)
    i2C_driver = device_create(i2C_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(i2C_driver)) {
        class_destroy(i2C_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(i2C_driver);        
    }
    printk(KERN_INFO "I2CDriver: device class created correctly\n");

    // 4. 取得 I2C Adapter (/sys/bus/i2c/devices/i2c-1)
    etx_i2c_adapter = i2c_get_adapter(1);
    if (etx_i2c_adapter == NULL) {
        pr_err("Cannot get I2C Adapter 1！\n");
        device_destroy(i2C_class, MKDEV(major_number, 0));
        class_destroy(i2C_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return -ENODEV;
    }
    pr_info("Got I2C Adapter 1！\n");

    // 5. 建立 Board Info
    struct i2c_board_info mpu_board_info = {
        I2C_BOARD_INFO(SLAVE_DEVICE_NAME, PICO_SLAVE_ADDR)
    };

    // 6. 建立 I2C Client 裝置
    etx_i2c_client = i2c_new_client_device(etx_i2c_adapter, &mpu_board_info);
    if (IS_ERR(etx_i2c_client)) {
        pr_err("Failed to create I2C Client device!\n");
        i2c_put_adapter(etx_i2c_adapter);
        device_destroy(i2C_class, MKDEV(major_number, 0));
        class_destroy(i2C_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(etx_i2c_client);
    }

    // 釋放 adapter 引用
    i2c_put_adapter(etx_i2c_adapter);

    // 7. 向 Linux I2C 子系統註冊 i2c_driver

    i2c_add_driver(&etx_mpu_driver);

    return 0;
}

// 驅動卸載
static void __exit i2c_exit(void) {
    // 1. 從 I2C 子系統刪除 Driver
    i2c_del_driver(&etx_mpu_driver);

    // 2. 註銷 I2C Client 裝置
    if (etx_i2c_client) {
        i2c_unregister_device(etx_i2c_client);
    }

    // 3. 清理字元驅動資源
    device_destroy(i2C_class, MKDEV(major_number, 0));
    class_destroy(i2C_class);
    class_unregister(i2C_class);
    unregister_chrdev(major_number, DEVICE_NAME);

    printk(KERN_INFO "I2CDriver: Goodbye from the Kernel!\n");
}

static int dev_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "I2CDriver: Device has been opened\n");
    return 0;
}

static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
    struct mpu6050_data mpu_data;
    u8 raw_data[6];

    if (len < sizeof(mpu_data)) {
        return -EINVAL;
    }
    
    // 讀取 Pico (0x42)
    int ret = i2c_master_recv(etx_i2c_client, raw_data, 6);
    if (ret < 0) {
        pr_err("I2CDriver: Failed to read data from PICO! (ret = %d)\n", ret);
        return -EIO;
    }

    mpu_data.accel_x = (short)((raw_data[0] << 8) | raw_data[1]);
    mpu_data.accel_y = (short)((raw_data[2] << 8) | raw_data[3]);
    mpu_data.accel_z = (short)((raw_data[4] << 8) | raw_data[5]);

    if (copy_to_user(buffer, &mpu_data, sizeof(mpu_data))) {
        return -EFAULT;
    }

    return sizeof(mpu_data); 
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
    if (copy_from_user(message, buffer, len)) {
        return -EFAULT;
    }
    return len;
}

static int dev_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "I2CDriver: Device successfully closed\n");
    return 0;
}

module_init(i2c_init);
module_exit(i2c_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Peng");
MODULE_DESCRIPTION("A simple i2c dummy driver for software logic testing");
