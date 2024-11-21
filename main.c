#include <msp430.h> 
#include "stdint.h"

uint16_t adcvolt[101] = {0};
uint16_t maxn = 0;
uint16_t minn = 0;
uint16_t vpp = 0;
/*
 * @fn:    void InitSystemClock(void)
 * @brief: ��ʼ��ϵͳʱ��
 * @para:  none
 * @return:none
 * @comment: ��ʼ��ϵͳʱ��
 */
void InitSystemClock(void)
{
    /*����DCOΪ1MHZʱ��*/
    DCOCTL = CALDCO_1MHZ;
    BCSCTL1 = CALBC1_1MHZ;

    /*����SMCLK��ʱ��ԴΪDCO*/
    BCSCTL2 &= ~SELS;
    /*SMCLK�ķ�Ƶϵ����Ϊ1*/
    BCSCTL2 &= ~(DIVS0 | DIVS1);
}

/*
 * @fn:    void InitUART(void)
 * @brief: ��ʼ�����ڣ��������ò����ʣ�����λ��У��λ��
 * @para:  none
 * @return:none
 * @comment: ��ʼ������
 */
void InitUART(void)
{
    /*��λUSCI_Ax*/
    UCA0CTL1 |= UCSWRST;

    /*����Ϊ�첽ģʽ*/
    UCA0CTL0 &= ~UCSYNC;

    /*����UARTʱ��ԴΪSMCLK*/
    UCA0CTL1 |= UCSSEL1;

    /*���ò�����Ϊ9600*/
    UCA0BR0 = 0x68;
    UCA0BR1 = 0x00;
    UCA0MCTL = 1 << 1;

    /*���ö˿�,ʹ�ܶ˿ڸ���*/
    P1SEL   |= BIT1 + BIT2;
    P1SEL2  |= BIT1 + BIT2;

    /*�����λλ��ʹ��UART*/
    UCA0CTL1 &= ~UCSWRST;

    /*�����ж�����*/
    IE2 |= UCA0RXIE;
    /*��ս����жϱ�־*/
    IFG2 &= ~UCA0RXIFG;
}

void UARTSendByte(uint8_t byte) {
    while (UCA0STAT & UCBUSY);
    UCA0TXBUF = byte;
}

/*
 * @fn:    void UARTSendString(uint8_t *pbuff, uint_8 num)
 * @brief: ��ʼ�����ڷ����ַ���
 * @para:  pbuff:ָ��Ҫ�����ַ�����ָ��
 *         num:Ҫ���͵��ַ�����
 * @return:none
 * @comment: ��ʼ�����ڷ����ַ���
 */
void UARTSendString(uint8_t *pbuff)
{
    while (*pbuff) {
       UARTSendByte(*pbuff++);
    }
}

/*
 * @fn:    void PrintNumber(uint16_t num)
 * @brief: ��ʼ�����ڷ�������
 * @para:  num������
 * @return:none
 * @comment: ��ʼ�����ڷ�������
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
 * @brief: ��ʼ�����ڷ��͸���������
 * @para:  num�������ͱ���
 * @return:none
 * @comment: ��ʼ�����ڷ��͸���������
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
 * @brief: ADC��ʼ��
 * @para:  none
 * @return:none
 * @comment: ADC��ʼ��
 */
void InitADC(void)
{
    /*����ADCʱ��MCLK*/
    ADC10CTL1 |= ADC10SSEL_2;
    /*ADC 2��Ƶ*/
    ADC10CTL1 |= ADC10DIV_0;
    /*����ADC��׼Դ*/
    ADC10CTL0 |= SREF_1;
    /*����ADC��������ʱ��64CLK*/
    ADC10CTL0 |= ADC10SHT_3;
    /*����ADC������200k*/
    ADC10CTL0 &= ~ADC10SR;
    /*ADC��׼ѡ��2.5V*/
    ADC10CTL0 |= REF2_5V;
    /*������׼*/
    ADC10CTL0 |= REFON;
    /*ѡ��ADC����ͨ��A0*/
    ADC10CTL1 |= INCH_0;
    /*����A0ģ������*/
    ADC10AE0 |= 0x0001;
    /*����ADC*/
    ADC10CTL0 |= ADC10ON;
}

void StartADCConvert(void) {
    // ����ADCת��
    ADC10CTL0 |= ADC10SC | ENC;
    // �ȴ�ת�����
    while (ADC10CTL1 & ADC10BUSY);
}

/*
 * @fn:    uint16_t GetADCValue(void)
 * @brief: ����һ��ADCת��������ADCת�����
 * @para:  none
 * @return:ADCת�����
 * @comment: ADCת�����Ϊ10bit����uint16_t���ͷ��أ���10λ��Ч����
 */
uint16_t GetADCValue(void)
{
    /*��ʼת��*/
    ADC10CTL0 |= ADC10SC|ENC;
    /*�ȴ�ת�����*/
    while(ADC10CTL1 & ADC10BUSY);
    /*���ؽ��*/
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

        // ͨ��UART������Сֵ
        UARTSendString("Min: ");
        PrintNumber(minn);
        UARTSendString("\n");

        //ͨ��UART���ͷ�ֵ
        UARTSendString("VPP: ");
        PrintNumber(vpp);
        UARTSendString("\n");

        // ͨ��UART����ƽ��ֵ
        UARTSendString("Average: ");
        PrintFloat((float)Average(adcvolt, 100)*2.5/1023, 3);
        UARTSendString("\n");

        UARTSendString("Finished!");
        __delay_cycles(300000);
    }
    return 0;
}