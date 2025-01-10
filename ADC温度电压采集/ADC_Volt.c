#include <msp430.h>
#include "stdint.h"

uint16_t adc_storage[50] = {0}; // 存储ADC转换结果
uint16_t maxn = 0;    // 最大值
uint16_t minn = 0;    // 最小值


void InitSystemClock(void) {
    DCOCTL = CALDCO_1MHZ;   // 设置DCO频率
    BCSCTL1 = CALBC1_1MHZ;  // 设置分频器参数
    BCSCTL2 &= ~SELS;    // 选择DCO作为时钟源
    BCSCTL2 &= ~(DIVS0 | DIVS1);    // 设置时钟分频系数
}

void InitUART(void) {
    UCA0CTL1 |= UCSWRST;    // 进入UART复位状态
    UCA0CTL0 &= ~UCSYNC;    // 配置UART为异步模式
    UCA0CTL1 |= UCSSEL1;    // 选择SMCLK作为时钟源
    UCA0BR0 = 0x68;
    UCA0BR1 = 0x00;    // 设置波特率
    UCA0MCTL = UCBRS_0 + UCBRF_0;    // 设置调制寄存器
    P1SEL |= BIT1 + BIT2;
    P1SEL2 |= BIT1 + BIT2;  // 选择P1.1和P1.2作为UART的TXD和RXD
    UCA0CTL1 &= ~UCSWRST;    // 退出UART复位状态
}

void UARTSendByte(uint8_t byte) {
    while (UCA0STAT & UCBUSY);
    UCA0TXBUF = byte;
}

void UARTSendString(uint8_t *buff) {
    while (*buff) {
        UARTSendByte(*buff++);
    }
}

void PrintNumber(int num) {
    char buffer[12];
    int i = 0;

    if (num < 0) {
        buffer[i++] = '-';
        num = -num;
    }

    int len = 0;
    while (num > 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
        len++;
    }
    if (len == 0) {
        buffer[i++] = '0';
    }

    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++;
        end--;
    }
    buffer[i] = '\0';

    UARTSendString((uint8_t *)buffer);
}

void PrintFloat(float num, int decimalPlaces) {
    char str[12];
    int intPart = (int)num;
    float decPart = num - (float)intPart;
    int decMultiplier = 10;
    int i;

    int len = 0;
    if (intPart < 0) {
        str[len++] = '-';
        intPart = -intPart;
    }
    char intStr[12];
    int intLen = 0;
    while (intPart > 0) {
        intStr[intLen++] = (intPart % 10) + '0';
        intPart /= 10;
    }
    if (intLen == 0) {
        intStr[intLen++] = '0';
    }
    for (i = intLen - 1; i >= 0; i--) {
        str[len++] = intStr[i];
    }

    if (decimalPlaces > 0) {
        str[len++] = '.';
        while (decimalPlaces-- > 0 && decPart > 0) {
            decPart *= decMultiplier;
            int digit = (int)decPart;
            str[len++] = digit + '0';
            decPart -= digit;
        }
    }

    str[len] = '\0';
    UARTSendString(str);
}

void InitADC(void) {
    ADC10CTL1 |= ADC10SSEL_2;
    ADC10CTL0 |= SREF_1;
    ADC10CTL0 |= ADC10SHT_2;
    ADC10CTL0 |= ADC10SR;
    ADC10CTL0 |= REF2_5V;
    ADC10CTL0 |= REFON;
    ADC10CTL1 |= INCH_4;
    ADC10AE0 |= 1 << 4;

    ADC10DTC0 |= ADC10CT;
    ADC10DTC1 = 50;
    ADC10SA = (uint16_t)(adc_storage);

    ADC10CTL0 |= ADC10ON;

    ADC10CTL0 |= ENC;
}

void StartADCConvert(void) {
    ADC10CTL0 |= ADC10SC | ENC;    // 启动ADC转换
    while (ADC10CTL1 & ADC10BUSY);    // 等待转换完成
}

uint16_t Max(uint16_t *nums, uint16_t num) {
    uint16_t cnt = 0;
    uint16_t max = nums[0];
    for (cnt = 1; cnt < num; cnt++) {
        if (nums[cnt] > max) {
            max = nums[cnt];
        }
    }
    return max;
}

uint16_t Min(uint16_t *nums, uint16_t num) {
    uint16_t cnt = 0;
    uint16_t min = nums[0];
    for (cnt = 1; cnt < num; cnt++) {
        if (nums[cnt] < min) {
            min = nums[cnt];
        }
    }
    return min;
}

uint16_t Average(uint16_t *nums, uint16_t num) {
    uint32_t sum = 0;
    uint16_t cnt = 0;
    for (cnt = 0; cnt < num; cnt++) {
        sum += nums[cnt];
    }
    return (uint16_t)(sum / num);
}

// 定时器中断初始化
void InitTimer(void) {
    TA0CCR0 = 50000;               // 设置定时器周期（大约1秒钟）
    TA0CCTL0 = CCIE;               // 启用定时器中断
    TA0CTL = TASSEL_2 + MC_1;      // 使用SMCLK，设置为增计数模式
}

// 定时器中断服务程序
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A(void) {
    // 在定时器中断时唤醒MCU，触发ADC转换
    StartADCConvert();
    __bic_SR_register_on_exit(LPM0_bits);  // 退出低功耗模式
}

int main(void) {
    uint8_t cnt = 0;

    // 停止看门狗计时器
    WDTCTL = WDTPW | WDTHOLD;   
    InitSystemClock();    // 初始化系统时钟
    InitUART();           // 初始化UART
    InitADC();            // 初始化ADC
    InitTimer();          // 初始化定时器

    __bis_SR_register(GIE);  // 开启全局中断

    while (1) {
        // 每次进入低功耗模式前，等待定时器中断触发
        __bis_SR_register(LPM0_bits + GIE); // 进入LPM0模式并允许中断

        // 当ADC采样完成后，处理数据
        maxn = Max(adc_storage, 50);
        minn = Min(adc_storage, 50);

        UARTSendString("Max: ");
        PrintNumber(maxn);
        UARTSendString("\n");

        UARTSendString("Min: ");
        PrintNumber(minn);
        UARTSendString("\n");

        UARTSendString("Average: ");
        PrintFloat((float)Average(adc_storage, 50) * 2.5 / 1023, 3);
        UARTSendString("\n");

        __delay_cycles(1000000);  // 延时，防止过快重复打印
    }

    return 0;
}
