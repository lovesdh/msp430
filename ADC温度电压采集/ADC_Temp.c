#include <msp430g2553.h>
#include "stdint.h"

long temp; // 存储原始温度传感器读值
float IntDegF; // 存储华氏度温度值
float IntDegC; // 存储摄氏度温度值

/*
 * @fn:     void InitSystemClock(void)
 * @brief:  初始化系统时钟
 * @para:   none
 * @return: none
 * @comment:初始化系统时钟
 */
void InitSystemClock(void)
{
    /*配置DCO为1MHz*/
    DCOCTL = CALDCO_1MHZ;
    BCSCTL1 = CALBC1_1MHZ;
    /*配置SMCLK的时钟源为DCO*/
    BCSCTL2 &= ~SELS;
    /*SMCLK的分频系数置为1*/
    BCSCTL2 &= ~(DIVS0 | DIVS1);
}
/*
 * @fn:     void InitUART(void)
 * @brief:  初始化串口，包括设置波特率，数据位，校验位等
 * @para:   none
 * @return: none
 * @comment:初始化串口
 */
void InitUART(void)
{
    /*复位USCI_Ax*/
    UCA0CTL1 |= UCSWRST;

    /*选择USCI_Ax为UART模式*/
    UCA0CTL0 &= ~UCSYNC;

    /*配置UART时钟源为SMCLK*/
    UCA0CTL1 |= UCSSEL1;

    /*配置波特率为9600@1MHz*/
    UCA0BR0 = 0x68;
    UCA0BR1 = 0x00;
    UCA0MCTL = 1 << 1;
    /*使能端口复用*/
    P1SEL |= BIT1 + BIT2;
    P1SEL2 |= BIT1 + BIT2;
    /*清除复位位，使能UART*/
    UCA0CTL1 &= ~UCSWRST;
}

/*
 * @fn:     void UARTSendByte(uint8_t byte)
 * @brief:  通过UART发送一个字节
 * @para:   byte:要发送的字节
 * @return: 无
 * @comment:等待上一个字节发送完成，然后发送新的字节
 */
void UARTSendByte(uint8_t byte) {
    // 等待上一个字符发送完毕
    while (UCA0STAT & UCBUSY);
    UCA0TXBUF = byte; // 发送当前字符
}

/*
 * @fn:     void UARTSendString(uint8_t *pbuff)
 * @brief:  通过UART发送字符串
 * @para:   pbuff:要发送的字符串指针
 * @return: 无
 * @comment:发送字符串直到遇到字符串结束符'\0'
 */
void UARTSendString(uint8_t *pbuff) {
    while (*pbuff) {
        UARTSendByte(*pbuff++);
        while (UCA0STAT & UCBUSY);
    }
}

/*
 * @fn:     void PrintNumber(int num)
 * @brief:  通过串口发送数字
 * @para:   num:要发送的数字
 * @return: 无
 * @comment:将整数转换为字符串并通过串口发送
 */
void PrintNumber(int num) {
    char buffer[12];
    int i = 0;

    // 处理负数
    if (num < 0) {
        buffer[i++] = '-';
        num = -num;
    }

    // 处理正数和零
    int len = 0;
    while (num > 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
        len++;
    }
    if (len == 0) {  // 处理零的情况
        buffer[i++] = '0';
    }

    // 反转字符串
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++;
        end--;
    }
    buffer[i] = '\0'; // 添加字符串结束符

    UARTSendString((uint8_t *)buffer); // 发送转换后的字符串
}

/*
 * @fn:     void PrintFloat(float num, int decimalPlaces)
 * @brief:  通过串口发送浮点数
 * @para:   num:要发送的浮点数
 *         decimalPlaces:小数点后的位数
 * @return: 无
 * @comment:将浮点数转换为字符串并通过串口发送，包括整数部分和小数部分
 */
void PrintFloat(float num, int decimalPlaces) {
    char str[24]; // 增加数组大小以防止溢出
    int intPart = (int)num; // 整数部分
    float decPart = num - (float)intPart; // 小数部分
    int len = 0;

    // 处理整数部分
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
    int i;
    for (i = intLen - 1; i >= 0; i--) {
        str[len++] = intStr[i];
    }

    // 处理小数部分
    if (decimalPlaces > 0) {
        str[len++] = '.';
        int decimalLen = 0;
        while (decimalPlaces-- > 0) {
            decPart *= 10;
            int digit = (int)decPart;
            str[len++] = digit + '0';
            decPart -= digit;
            decimalLen++;
        }
        // 确保小数部分的末尾零也被打印出来
        while (decimalLen < decimalPlaces) {
            str[len++] = '0';
            decimalLen++;
        }
    }

    str[len] = '\0'; // 字符串结束符
    UARTSendString((uint8_t *)str);
}

/*
 * @fn:     void InitADC(void)
 * @brief:  初始化ADC模块
 * @para:   无
 * @return: 无
 * @comment:配置ADC10寄存器以使用内部温度传感器作为输入
 */
void InitADC(void) {
    ADC10CTL1 = INCH_10 + ADC10DIV_3; // Temp Sensor ADC10CLK/4
    ADC10CTL0 = SREF_1 + ADC10SHT_3 + REFON + ADC10ON + ADC10IE;
    __enable_interrupt();
}

/*
 * @fn:     uint16_t GetADCValue(void)
 * @brief:  获取ADC值
 * @para:   无
 * @return: ADC值
 * @comment:启动ADC转换并等待完成，返回ADC转换结果
 */
uint16_t GetADCValue(void) {
    ADC10CTL0 |= ADC10SC | ENC;
    while (ADC10CTL1 & ADC10BUSY);
    return ADC10MEM;
}
uint8_t flag = 0;

//// 延时函数
void DelaySeconds(unsigned int seconds) {
    TA1CTL |= TASSEL_2 + MC_1 + ID_3; // 设置时钟源为SMCLK，工作模式为Up Mode，分频系数为8
    TA1CCR0 = 62500 - 1; // 设置定时器周期为1秒（1MHz/8 = 125kHz，1秒 = 125000周期）
    while (seconds--) {
        TA1CTL |= TAIE; // 开启定时器中断
        __bis_SR_register(GIE); // 打开全局中断
        TA1CTL &= ~TAIFG; // 清除定时器中断标志
        while (!TAIFG); // 等待定时器中断发生
        TA1CTL &= ~TAIFG; // 清除定时器中断标志
    }
    TA1CTL &= ~MC_1; // 关闭定时器
}
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;
    InitSystemClock();
    InitUART();
    InitADC();
    /*设置P1.6为输出*/
    P1DIR |= BIT6;
    // 添加延时函数，延时3秒
    DelaySeconds(3);
    TA1CTL = TASSEL_2 + MC_1 + ID_0; // 设置时钟源为SMCLK，工作模式为Up Mode，不分频
   TA1CCR0 = 50000 - 1; // 设置定时器周期为50ms（1MHz/1 = 1MHz，50ms = 50000周期）
   TA1CTL |= TAIE; // 开启定时器中断
   __bis_SR_register(GIE); // 打开全局中断



    while(1)
    {
        if(flag == 1)
        {
            flag = 0;
            P1OUT ^= BIT6;
            temp = GetADCValue(); // 获取ADC值
            // 将ADC值转换为摄氏度和华氏度
            IntDegC = ((temp - 746) / (0.000355 * 678) + 286) / 10.0; // 转换为摄氏度
            IntDegF = IntDegC * 9.0 / 5.0 + 32.0; // 转换为华氏度

            UARTSendString("摄氏度: ");
            PrintFloat(IntDegC,3); // 发送摄氏度温度
            UARTSendString(" C\n");
            UARTSendString("华氏度: ");
            PrintFloat(IntDegF,3); // 发送华氏度温度
            UARTSendString(" F\n");
        }
    }
    return 0;
}
#pragma vector = TIMER1_A1_VECTOR
// 指定这个函数为定时器1的中断向量处理函数。

__interrupt void Time_Tick(void)
{
    // 定义一个中断服务函数Time_Tick，当定时器1的中断发生时，将调用此函数。

    static uint8_t cnt = 0;
    // 定义一个静态局部变量cnt，用于计数中断发生的次数。由于是static类型，其值在函数调用之间保持不变。

    switch(TA1IV)
    {
    // 使用switch语句根据定时器1的中断向量号（TA1IV）来确定中断的原因。
    // TA1IV是定时器1中断向量号的寄存器，它包含了产生中断的原因。

    case 0x02:
        // 如果TA1IV的值是0x02，这是定时器1的捕获/比较模块1的中断，但这里没有实现任何操作。
        break;

    case 0x04:
        // 如果TA1IV的值是0x04，这是定时器1的捕获/比较模块2的中断，这里同样没有实现任何操作。
        break;

    case 0x0A:
        // 如果TA1IV的值是0x0A，这是定时器1的定时器溢出中断（Timer Overflow）。
        cnt++;
        // 每次定时器溢出时，cnt变量的值增加1。

        if(cnt == 20)
        {
            // 每20次定时器溢出（即1秒，因为每次溢出代表50ms），执行以下操作。
            cnt = 0; // 清零计数器，准备下一轮计数。
            flag = 1; // 设置flag标志位，表示1秒时间到了，在主循环中将根据这个标志位更新时间和翻转LED状态。
        }
        break;

    default:
        // 如果TA1IV的值不是上述任何一个情况，不执行任何操作。
        break;
    }
}

#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void) {
    __bic_SR_register_on_exit(CPUOFF);
}
