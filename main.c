#include <msp430.h> 
#include "stdint.h"

uint16_t adcvolt[101] = {0};
uint16_t maxn = 0;
uint16_t minn = 0;
uint16_t vpp = 0;
/*
 * @fn:    void InitSystemClock(void)
 * @brief: 初始化系统时钟
 * @para:  none
 * @return:none
 * @comment: 初始化系统时钟
 */
void InitSystemClock(void)
{
    /*配置DCO为1MHZ时钟*/
    DCOCTL = CALDCO_1MHZ;
    BCSCTL1 = CALBC1_1MHZ;

    /*配置SMCLK的时钟源为DCO*/
    BCSCTL2 &= ~SELS;
    /*SMCLK的分频系数置为1*/
    BCSCTL2 &= ~(DIVS0 | DIVS1);
}

/*
 * @fn:    void InitUART(void)
 * @brief: 初始化串口，包括设置波特率，数据位，校验位等
 * @para:  none
 * @return:none
 * @comment: 初始化串口
 */
void InitUART(void)
{
    /*复位USCI_Ax*/
    UCA0CTL1 |= UCSWRST;

    /*设置为异步模式*/
    UCA0CTL0 &= ~UCSYNC;

    /*配置UART时钟源为SMCLK*/
    UCA0CTL1 |= UCSSEL1;

    /*配置波特率为9600*/
    UCA0BR0 = 0x68;
    UCA0BR1 = 0x00;
    UCA0MCTL = 1 << 1;

    /*配置端口,使能端口复用*/
    P1SEL   |= BIT1 + BIT2;
    P1SEL2  |= BIT1 + BIT2;

    /*清除复位位，使能UART*/
    UCA0CTL1 &= ~UCSWRST;

    /*接收中断启用*/
    IE2 |= UCA0RXIE;
    /*清空接收中断标志*/
    IFG2 &= ~UCA0RXIFG;
}

void UARTSendByte(uint8_t byte) {
    while (UCA0STAT & UCBUSY);
    UCA0TXBUF = byte;
}

/*
 * @fn:    void UARTSendString(uint8_t *pbuff, uint_8 num)
 * @brief: 初始化串口发送字符串
 * @para:  pbuff:指向要发送字符串的指针
 *         num:要发送的字符个数
 * @return:none
 * @comment: 初始化串口发送字符串
 */
void UARTSendString(uint8_t *pbuff)
{
    while (*pbuff) {
       UARTSendByte(*pbuff++);
    }
}

/*
 * @fn:    void PrintNumber(uint16_t num)
 * @brief: 初始化串口发送数字
 * @para:  num：变量
 * @return:none
 * @comment: 初始化串口发送数字
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
 * @fn:    void PrintFloat(float num)
 * @brief: 初始化串口发送浮点型数字
 * @para:  num：浮点型变量
 * @return:none
 * @comment: 初始化串口发送浮点型数字
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
 * @fn:    void InitADC(void)
 * @brief: ADC初始化
 * @para:  none
 * @return:none
 * @comment: ADC初始化
 */
void InitADC(void)
{
    /*设置ADC时钟MCLK*/
    ADC10CTL1 |= ADC10SSEL_2;
    /*ADC 2分频*/
    ADC10CTL1 |= ADC10DIV_0;
    /*设置ADC基准源*/
    ADC10CTL0 |= SREF_1;
    /*设置ADC采样保持时间64CLK*/
    ADC10CTL0 |= ADC10SHT_3;
    /*设置ADC采样率200k*/
    ADC10CTL0 &= ~ADC10SR;
    /*ADC基准选择2.5V*/
    ADC10CTL0 |= REF2_5V;
    /*开启基准*/
    ADC10CTL0 |= REFON;
    /*选择ADC输入通道A0*/
    ADC10CTL1 |= INCH_0;
    /*允许A0模拟输入*/
    ADC10AE0 |= 0x0001;
    /*开启ADC*/
    ADC10CTL0 |= ADC10ON;
}

void StartADCConvert(void) {
    // 启动ADC转换
    ADC10CTL0 |= ADC10SC | ENC;
    // 等待转换完成
    while (ADC10CTL1 & ADC10BUSY);
}

/*
 * @fn:    uint16_t GetADCValue(void)
 * @brief: 进行一次ADC转换并返回ADC转换结果
 * @para:  none
 * @return:ADC转换结果
 * @comment: ADC转换结果为10bit，以uint16_t类型返回，低10位有效数据
 */
uint16_t GetADCValue(void)
{
    /*开始转换*/
    ADC10CTL0 |= ADC10SC|ENC;
    /*等待转换完成*/
    while(ADC10CTL1 & ADC10BUSY);
    /*返回结果*/
    return ADC10MEM;
}

uint16_t Max(uint16_t *numptr,uint16_t num)
{
    uint16_t cnt = 0;
    uint16_t max = 0;
    for(cnt = 0;cnt < num;cnt ++)
    {
        if(numptr[cnt] > max){
            max = numptr[cnt];
        }
    }
    return max;
}

uint16_t Min(uint16_t *numptr,uint16_t num)
{
    uint16_t cnt = 0;
    uint16_t min = 0;
    min = numptr[0];
    for(cnt = 0;cnt < num;cnt ++)
    {
        if(numptr[cnt] < min){
            min = numptr[cnt];
        }
    }
    return min;
}

uint16_t Average(uint16_t *datptr,uint16_t num)
{
    uint32_t sum = 0;
    uint8_t cnt = 0;
    for(cnt = 0;cnt < num;cnt ++)
    {
        sum += *(datptr + cnt);
    }
    return (uint16_t)(sum /num);
}

int main(void)
{
    uint8_t cnt = 0;
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    InitSystemClock();
    InitUART();
    InitADC();

    while(1)
    {
        for (cnt = 0; cnt < 100; cnt++) {
           StartADCConvert();
        }

        maxn=Max(adcvolt,100);
        minn=Min(adcvolt,100);
        vpp=maxn-minn;
        UARTSendString("Max: ");
        PrintNumber(maxn);
        UARTSendString("\n");

        // 通过UART发送最小值
        UARTSendString("Min: ");
        PrintNumber(minn);
        UARTSendString("\n");

        //通过UART发送峰值
        UARTSendString("VPP: ");
        PrintNumber(vpp);
        UARTSendString("\n");

        // 通过UART发送平均值
        UARTSendString("Average: ");
        PrintFloat((float)Average(adcvolt, 100)*2.5/1023, 3);
        UARTSendString("\n");

        UARTSendString("Finished!");
        __delay_cycles(300000);
    }
    return 0;
}