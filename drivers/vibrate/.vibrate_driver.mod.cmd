savedcmd_vibrate_driver.mod := printf '%s\n'   vibrate_driver.o | awk '!x[$$0]++ { print("./"$$0) }' > vibrate_driver.mod
