savedcmd_etx_spi_driver.mod := printf '%s\n'   spi_driver.o ssd1306.o | awk '!x[$$0]++ { print("./"$$0) }' > etx_spi_driver.mod
