#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/device.h>

#define DEVICE_NAME       "i2C_driver"
#define CLASS_NAME        "mpu_class"
#define I2C_BUS_NUM       1          // 樹莓派預設 I2C-1 (GP2/GP3)
#define SLAVE_NAME        "MPU6050"
#define MPU6050_ADDR      0x68       // MPU-6050 I2C 設備位址

#define REG_PWR_MGMT_1    0x6B       // 電源管理暫存器
#define REG_ACCEL_XOUT_H  0x3B       // 加速度 X 軸高位元組 (連續讀取起點)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Smart Conveyor Team");
MODULE_DESCRIPTION("MPU6050 Direct Linux Kernel Driver for Raspberry Pi");
MODULE_VERSION("1.0");

static int major_number;
static struct class *mpu_class = NULL;
static struct device *mpu_device = NULL;
static struct i2c_adapter *i2c_adap = NULL;
static struct i2c_client *i2c_client = NULL;

// 與 User-space 應用程式對應的資料結構
struct mpu6050_data {
    short accel_x;
    short accel_y;
    short accel_z;
};

// 寫入暫存器 (用來喚醒晶片)
static int mpu6050_write_reg(u8 reg, u8 val) {
    u8 buf[2] = {reg, val};
    int ret = i2c_master_send(i2c_client, buf, 2);
    return (ret == 2) ? 0 : -EIO;
}

// 連續讀取 6 Bytes (X, Y, Z 加速度)
static int mpu6050_read_accel(short *accel) {
    u8 reg = REG_ACCEL_XOUT_H;
    u8 data[6];
    int ret;

    // 1. 寫入起始暫存器位址 0x3B
    ret = i2c_master_send(i2c_client, &reg, 1);
    if (ret < 0) {
        pr_err("MPU6050: Failed to write start register (ret=%d)\n", ret);
        return ret;
    }

    // 2. 連續接收 6 Bytes
    ret = i2c_master_recv(i2c_client, data, 6);
    if (ret < 0) {
        pr_err("MPU6050: Failed to read accelerometer data (ret=%d)\n", ret);
        return ret;
    }

    // 3. 高低位元組組裝 (Big-Endian)
    accel[0] = (short)((data[0] << 8) | data[1]); // X
    accel[1] = (short)((data[2] << 8) | data[3]); // Y
    accel[2] = (short)((data[4] << 8) | data[5]); // Z

    return 0;
}

static int dev_open(struct inode *inodep, struct file *filep) {
    return 0;
}

static int dev_release(struct inode *inodep, struct file *filep) {
    return 0;
}

// User-space 呼叫 read() 時觸發
static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
    struct mpu6050_data data;
    short raw[3];

    if (len < sizeof(struct mpu6050_data)) {
        return -EINVAL;
    }

    if (mpu6050_read_accel(raw) < 0) {
        return -EIO;
    }

    data.accel_x = raw[0];
    data.accel_y = raw[1];
    data.accel_z = raw[2];

    if (copy_to_user(buffer, &data, sizeof(data))) {
        return -EFAULT;
    }

    return sizeof(data);
}

static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = dev_open,
    .read    = dev_read,
    .release = dev_release,
};

static int __init mpu6050_driver_init(void) {
    struct i2c_board_info board_info = {
        I2C_BOARD_INFO(SLAVE_NAME, MPU6050_ADDR)
    };

    pr_info("MPU6050: Initializing Driver...\n");

    // 1. 註冊字元裝置
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        pr_err("MPU6050: Failed to register a major number\n");
        return major_number;
    }

    // 2. 建立 class
    mpu_class = class_create(CLASS_NAME);
    if (IS_ERR(mpu_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        pr_err("MPU6050: Failed to register device class\n");
        return PTR_ERR(mpu_class);
    }

    // 3. 自動在 /dev 建立 /dev/i2C_driver 裝置節點
    mpu_device = device_create(mpu_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(mpu_device)) {
        class_destroy(mpu_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        pr_err("MPU6050: Failed to create device\n");
        return PTR_ERR(mpu_device);
    }

    // 4. 取得 I2C-1 Adapter
    i2c_adap = i2c_get_adapter(I2C_BUS_NUM);
    if (!i2c_adap) {
        device_destroy(mpu_class, MKDEV(major_number, 0));
        class_destroy(mpu_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        pr_err("MPU6050: Cannot get I2C adapter %d\n", I2C_BUS_NUM);
        return -ENODEV;
    }

    // 5. 實體化 I2C Client
    i2c_client = i2c_new_client_device(i2c_adap, &board_info);
    if (!i2c_client) {
        i2c_put_adapter(i2c_adap);
        device_destroy(mpu_class, MKDEV(major_number, 0));
        class_destroy(mpu_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        pr_err("MPU6050: Failed to create I2C client device\n");
        return -ENODEV;
    }

    // 6. 喚醒 MPU6050 晶片 (解除 Sleep Mode)
    if (mpu6050_write_reg(REG_PWR_MGMT_1, 0x00) < 0) {
        pr_warn("MPU6050: Wake up failed! Please check wiring.\n");
    } else {
        pr_info("MPU6050: Successfully woke up sensor!\n");
    }

    pr_info("MPU6050: /dev/%s driver created successfully with major %d\n", DEVICE_NAME, major_number);
    return 0;
}

static void __exit mpu6050_driver_exit(void) {
    pr_info("MPU6050: Removing Driver...\n");

    if (i2c_client) {
        i2c_unregister_device(i2c_client);
    }
    if (i2c_adap) {
        i2c_put_adapter(i2c_adap);
    }

    device_destroy(mpu_class, MKDEV(major_number, 0));
    class_unregister(mpu_class);
    class_destroy(mpu_class);
    unregister_chrdev(major_number, DEVICE_NAME);

    pr_info("MPU6050: Driver successfully unloaded\n");
}

module_init(mpu6050_driver_init);
module_exit(mpu6050_driver_exit);