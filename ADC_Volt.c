#include <msp430.h>
#include "stdint.h"

// 定义一个数组，用于存储ADC转换的结果
uint16_t adcbuff[50] = {0};
// 定义变量，用于存储ADC转换结果的最大值和最小值
uint16_t maxval = 0;
uint16_t minval = 0;
// 定义变量，用于存储ADC转换结果的峰峰值（Vpp）
uint16_t vpp = 0;

/*
 * @fn:     void InitSystemClock(void)
 * @brief:  初始化系统时钟
 * @para:   none
 * @return: none
 * @comment:设置系统时钟为1MHz
 */
void InitSystemClock(void) {
    // 设置DCO频率
    DCOCTL = CALDCO_1MHZ;
    // 设置分频器参数
    BCSCTL1 = CALBC1_1MHZ;
    // 选择DCO作为时钟源
    BCSCTL2 &= ~SELS;
    // 设置时钟分频系数
    BCSCTL2 &= ~(DIVS0 | DIVS1);
}

/*
 * @fn:     void InitUART(void)
 * @brief:  初始化UART模块
 * @para:   none
 * @return: none
 * @comment:配置UART模块，设置波特率等参数
 */
void InitUART(void) {
    // 进入UART复位状态
    UCA0CTL1 |= UCSWRST;
    // 配置UART为异步模式
    UCA0CTL0 &= ~UCSYNC;
    // 选择SMCLK作为时钟源
    UCA0CTL1 |= UCSSEL1;
    // 设置波特率
    UCA0BR0 = 0x68;
    UCA0BR1 = 0x00;
    // 设置调制寄存器
    UCA0MCTL = UCBRS_0 + UCBRF_0;
    // 选择P1.1和P1.2作为UART的TXD和RXD
    P1SEL |= BIT1 + BIT2;
    P1SEL2 |= BIT1 + BIT2;
    // 退出UART复位状态
    UCA0CTL1 &= ~UCSWRST;
}

/*
 * @fn:     void UARTSendByte(uint8_t byte)
 * @brief:  通过UART发送单个字节
 * @para:   byte:要发送的字节
 * @return: none
 * @comment:等待UART发送缓冲区为空，然后发送一个字节
 */
void UARTSendByte(uint8_t byte) {
    while (UCA0STAT & UCBUSY);
    UCA0TXBUF = byte;
}

/*
 * @fn:     void UARTSendString(uint8_t *pbuff)
 * @brief:  通过UART发送字符串
 * @para:   pbuff:指向要发送字符串的指针
 * @return: none
 * @comment:发送字符串直到字符串结束符'\0'
 */
void UARTSendString(uint8_t *pbuff) {
    while (*pbuff) {
        UARTSendByte(*pbuff++);
    }
}

/*
 * @fn:     void PrintNumber(int num)
 * @brief:  通过UART发送整数
 * @para:   num:要发送的整数
 * @return: none
 * @comment:将整数转换为字符串并通过UART发送，包括负数处理
 */
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

/*
 * @fn:     void PrintFloat(float num, int decimalPlaces)
 * @brief:  通过UART发送浮点数
 * @para:   num:要发送的浮点数
 *         decimalPlaces:小数点后的位数
 * @return: none
 * @comment:将浮点数转换为字符串并通过UART发送，包括整数部分和小数部分
 */
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

/*
 * @fn:     void InitADC(void)
 * @brief:  初始化ADC模块
 * @para:   none
 * @return: none
 * @comment:配置ADC模块，设置采样时间、参考电压等参数
 */
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
    ADC10SA = (uint16_t)(adcbuff);

    ADC10CTL0 |= ADC10ON;

    ADC10CTL0 |= ENC;
}

/*
 * @fn:     uint16_t GetADCValue(void)
 * @brief:  获取ADC转换值
 * @para:   none
 * @return: ADC转换结果
 * @comment:启动ADC转换并等待完成，返回转换结果
 */
uint16_t GetADCValue(void) {
    ADC10CTL0 |= ADC10SC | ENC;
    while (ADC10CTL1 & ADC10BUSY);
    return ADC10MEM;
}

/*
 * @fn:     void StartADCConvert(void)
 * @brief:  启动ADC转换
 * @para:   none
 * @return: none
 * @comment:启动ADC转换并等待完成
 */
void StartADCConvert(void) {
    // 启动ADC转换
    ADC10CTL0 |= ADC10SC | ENC;
    // 等待转换完成
    while (ADC10CTL1 & ADC10BUSY);
}

/*
 * @fn:     uint16_t Max(uint16_t *numptr, uint16_t num)
 * @brief:  找出数组中的最大值
 * @para:   numptr:数组指针，num:数组长度
 * @return: 数组中的最大值
 * @comment:遍历数组找出最大值
 */
uint16_t Max(uint16_t *numptr, uint16_t num) {
    uint16_t cnt = 0;
    uint16_t max = numptr[0];
    for (cnt = 1; cnt < num; cnt++) {
        // 如果当前值大于已知的最大值，则更新最大值
        if (numptr[cnt] > max) {
            max = numptr[cnt];
        }
    }
    return max;
}

/*
 * @fn:     uint16_t Min(uint16_t *numptr, uint16_t num)
 * @brief:  找出数组中的最小值
 * @para:   numptr:数组指针，num:数组长度
 * @return: 数组中的最小值
 * @comment:遍历数组找出最小值
 */
uint16_t Min(uint16_t *numptr, uint16_t num) {
    uint16_t cnt = 0;
    uint16_t min = numptr[0];
    for (cnt = 1; cnt < num; cnt++) {
        if (numptr[cnt] < min) {
            min = numptr[cnt];
        }
    }
    return min;
}

/*
 * @fn:     uint16_t Average(uint16_t *datptr, uint16_t num)
 * @brief:  计算数组的平均值
 * @para:   datptr:数组指针，num:数组长度
 * @return: 数组的平均值
 * @comment:遍历数组计算平均值
 */
uint16_t Average(uint16_t *datptr, uint16_t num) {
    uint32_t sum = 0;
    uint16_t cnt = 0;
    for (cnt = 0; cnt < num; cnt++) {
        sum += datptr[cnt];
    }
    return (uint16_t)(sum / num);
}

int main(void) {
    uint8_t cnt = 0;
        // 停止看门狗计时器
        WDTCTL = WDTPW | WDTHOLD;
        // 初始化系统时钟
        /*初始化LED2所在IO口P1.6为输出*/
        P1DIR |= BIT6;
        /*初始化LED2所在IO口P1.6为低电平，LED2初始状态为灭*/
        P1OUT &= ~BIT6;
        /*初始化按键所在IO口P1.3为输入*/
        P1DIR &= ~BIT3;
        /*使能P1.3口的上拉电阻*/
        P1REN |= BIT3;
        P1OUT |= BIT3;
        InitSystemClock();
        // 初始化UART
        InitUART();
        // 初始化ADC
        InitADC();
        P1DIR|=BIT6;
        P1OUT&=~BIT6;
        P1DIR&=~BIT3;
        P1REN|=BIT3;
        P1OUT|=BIT3;

        while (1) {
            if(P1IN&BIT3){
                P1OUT&=~BIT6;
            }else{
                P1OUT|=BIT6;
                for (cnt = 0; cnt < 50; cnt++) {
                   StartADCConvert();
                }
                // 计算最大值和最小值
                maxval = Max(adcbuff, 50);
                minval = Min(adcbuff, 50);
                // 计算峰峰值
                vpp = maxval - minval;
                // 通过UART发送最大值
                UARTSendString("Max: ");
                PrintNumber(maxval);
                UARTSendString("\n");

                // 通过UART发送最小值
                UARTSendString("Min: ");
                PrintNumber(minval);
                UARTSendString("\n");

                // 通过UART发送峰值
                UARTSendString("Vpp: ");
                PrintNumber(vpp);
                UARTSendString("\n");

                // 通过UART发送平均值
                UARTSendString("Average: ");
                PrintFloat((float)Average(adcbuff, 50) * 2.5 / 1023, 3);
                UARTSendString("\n");

                // 延时，等待下一次转换
                __delay_cycles(1000000);
            }
        }
        return 0;
}
