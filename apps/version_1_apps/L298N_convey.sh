#!/bin/sh

# 1. 設定 GPIO 方向 (IN1=GPIO17, IN2=GPIO27)
pinctrl set 17 op dh    # Drive High
pinctrl set 27 op dl    # Drive Low

# 💡 請確認你的 PWM 腳位 (例如 GPIO 12/18)，並將其切換為 PWM 功能 (a0)
pinctrl set 12 a0   

# 防護：若未 export 則建立節點並停頓一下
if [ ! -d /sys/class/pwm/pwmchip0/pwm0 ]; then
    echo 0 > /sys/class/pwm/pwmchip0/export
    sleep 0.1
fi

# 🟢 將總週期 Period 改為 10ms (10,000,000 ns = 100Hz)，提供大扭力！
echo 0 > /sys/class/pwm/pwmchip0/pwm0/enable
echo 10000000 > /sys/class/pwm/pwmchip0/pwm0/period

MODE=$(echo "$1" | tr '[:lower:]' '[:upper:]')

# 2. 判斷模式控制轉速
if [ "$MODE" = "A" ]; then
    echo 'Set speed to 100% (Full Speed)'
    echo 10000000 > /sys/class/pwm/pwmchip0/pwm0/duty_cycle
    echo 1 > /sys/class/pwm/pwmchip0/pwm0/enable

elif [ "$MODE" = "B" ]; then
    echo 'Set speed to 70% (Medium Speed)'
    echo 7000000 > /sys/class/pwm/pwmchip0/pwm0/duty_cycle
    echo 1 > /sys/class/pwm/pwmchip0/pwm0/enable

elif [ "$MODE" = "C" ]; then
    echo 'Set speed to 40% (Low Speed)'
    echo 4000000 > /sys/class/pwm/pwmchip0/pwm0/duty_cycle
    echo 1 > /sys/class/pwm/pwmchip0/pwm0/enable

elif [ "$MODE" = "D" ]; then
    echo 'Set speed to 0% (Stop)'
    echo 0 > /sys/class/pwm/pwmchip0/pwm0/duty_cycle
    echo 0 > /sys/class/pwm/pwmchip0/pwm0/enable
    # 🟢 拿掉了 unexport，避免第二次切換時節點消失

else
    echo "Usage: $0 <A|B|C|D>"
fi