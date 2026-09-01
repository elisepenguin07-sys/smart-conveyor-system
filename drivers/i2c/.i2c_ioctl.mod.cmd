savedcmd_i2c_ioctl.mod := printf '%s\n'   i2c_ioctl.o | awk '!x[$$0]++ { print("./"$$0) }' > i2c_ioctl.mod
