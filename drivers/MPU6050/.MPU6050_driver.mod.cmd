savedcmd_MPU6050_driver.mod := printf '%s\n'   MPU6050_driver.o | awk '!x[$$0]++ { print("./"$$0) }' > MPU6050_driver.mod
