#!/bin/sh

# 1. 強制切換 GPIO 13 到 PWM 功能 (a0)
pinctrl set 13 a0

# 2. 設定 GPIO 方向 (IN3=GPIO23, IN4=GPIO24)
pinctrl set 23 op dh
pinctrl set 24 op dl

# 防護：若 pwm1 節點不存在則自動 export
if [ ! -d /sys/class/pwm/pwmchip0/pwm1 ]; then
    echo 1 > /sys/class/pwm/pwmchip0/export
fi

# 🟢 設定總週期 Period 為 20ms (20,000,000 ns)
echo 10000000 > /sys/class/pwm/pwmchip0/pwm1/period

# 將傳入的參數轉大寫
ZONE=$(echo "$1" | tr '[:lower:]' '[:upper:]')

# 3. 判斷模式控制轉速 (PWM1)
if [ "$ZONE" = "D" ]; then
    echo 'Set to ZONE D (Duty Cycle: 18%)'
   
    echo 1800000 > /sys/class/pwm/pwmchip0/pwm1/duty_cycle
    echo 1 > /sys/class/pwm/pwmchip0/pwm1/enable

elif [ "$ZONE" = "C" ]; then
    echo 'Set to ZONE C (Duty Cycle: 8%)'
    # 20,000,000 * 8% = 1,600,000 ns
    echo 600000 > /sys/class/pwm/pwmchip0/pwm1/duty_cycle
    echo 1 > /sys/class/pwm/pwmchip0/pwm1/enable

elif [ "$ZONE" = "B" ]; then
    echo 'Set to ZONE B (Duty Cycle: 4%)'
    # 20,000,000 * 4% = 800,000 ns
    echo 400000 > /sys/class/pwm/pwmchip0/pwm1/duty_cycle
    echo 1 > /sys/class/pwm/pwmchip0/pwm1/enable

elif [ "$ZONE" = "A" ]; then
    echo 'Set to ZONE A (Stop)'
    echo 0 > /sys/class/pwm/pwmchip0/pwm1/duty_cycle
    echo 0 > /sys/class/pwm/pwmchip0/pwm1/enable
    # 🟢 拿掉了 unexport，避免節點被刪除

else
    echo "Usage: $0 <A|B|C|D>"
fi