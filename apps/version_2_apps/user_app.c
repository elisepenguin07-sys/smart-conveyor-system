#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

#define SPI_PATH "/dev/spi_driver"
#define I2C_PATH "/dev/i2C_driver"

struct data {
  short accel_x;
  short accel_y;
  short accel_z;
};

int main(void){
  int fd_SPI;
  int fd_i2c;
  int ret;

  printf("===User Space Device Control App ===\n");

  fd_SPI = open(SPI_PATH, O_RDWR);
  if(fd_SPI < 0){
    perror("Failed to open SPI node" SPI_PATH);
    printf("Hint: Did you insmod the SPI driver and run with sudo?\n");
    return EXIT_FAILURE;
  }

  printf("Successfully opened "SPI_PATH"\n");

  fd_i2c = open(I2C_PATH, O_RDWR);
  if(fd_i2c < 0){
    perror("Failed to open i2c node" I2C_PATH);
    printf("Hint: Did you insmod the i2c dricer and run with sudo?\n");
    return EXIT_FAILURE;
  }

  printf("Successfully opened "I2C_PATH"\n");

  char buffer[256];
  struct data my_accel;
  
  while(1){
    
    ret = read(fd_i2c, &my_accel, sizeof(my_accel));
    if(ret < 0){
      perror("I2C Read failed");
    }
    
    float ax = (float)my_accel.accel_x / 16384.0f;
    float ay = (float)my_accel.accel_y / 16384.0f;
    float az = (float)my_accel.accel_z / 16384.0f;

    // 格式化輸出給 OLED
    snprintf(buffer, sizeof(buffer), "MPU6050 Accel:\nX: %+.2f g\nY: %+.2f g\nZ: %+.2f g\n", 
            ax, ay, az);

    ret = write(fd_SPI, buffer, strlen(buffer));
    if(ret<0){
      perror("SPI Write failed");
    }
    printf("Sent to SPI:\n%s", buffer);
    sleep(2);
  }
  close(fd_SPI);
  close(fd_i2c);
  return 0;
}