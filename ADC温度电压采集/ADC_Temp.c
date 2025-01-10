#include <msp430g2553.h>
#include "stdint.h"

long temp; // �洢ԭʼ�¶ȴ�������ֵ
float IntDegF; // �洢���϶��¶�ֵ
float IntDegC; // �洢���϶��¶�ֵ

/*
 * @fn:     void InitSystemClock(void)
 * @brief:  ��ʼ��ϵͳʱ��
 * @para:   none
 * @return: none
 * @comment:��ʼ��ϵͳʱ��
 */
void InitSystemClock(void)
{
    /*����DCOΪ1MHz*/
    DCOCTL = CALDCO_1MHZ;
    BCSCTL1 = CALBC1_1MHZ;
    /*����SMCLK��ʱ��ԴΪDCO*/
    BCSCTL2 &= ~SELS;
    /*SMCLK�ķ�Ƶϵ����Ϊ1*/
    BCSCTL2 &= ~(DIVS0 | DIVS1);
}
/*
 * @fn:     void InitUART(void)
 * @brief:  ��ʼ�����ڣ��������ò����ʣ�����λ��У��λ��
 * @para:   none
 * @return: none
 * @comment:��ʼ������
 */
void InitUART(void)
{
    /*��λUSCI_Ax*/
    UCA0CTL1 |= UCSWRST;

    /*ѡ��USCI_AxΪUARTģʽ*/
    UCA0CTL0 &= ~UCSYNC;

    /*����UARTʱ��ԴΪSMCLK*/
    UCA0CTL1 |= UCSSEL1;

    /*���ò�����Ϊ9600@1MHz*/
    UCA0BR0 = 0x68;
    UCA0BR1 = 0x00;
    UCA0MCTL = 1 << 1;
    /*ʹ�ܶ˿ڸ���*/
    P1SEL |= BIT1 + BIT2;
    P1SEL2 |= BIT1 + BIT2;
    /*�����λλ��ʹ��UART*/
    UCA0CTL1 &= ~UCSWRST;
}

/*
 * @fn:     void UARTSendByte(uint8_t byte)
 * @brief:  ͨ��UART����һ���ֽ�
 * @para:   byte:Ҫ���͵��ֽ�
 * @return: ��
 * @comment:�ȴ���һ���ֽڷ�����ɣ�Ȼ�����µ��ֽ�
 */
void UARTSendByte(uint8_t byte) {
    // �ȴ���һ���ַ��������
    while (UCA0STAT & UCBUSY);
    UCA0TXBUF = byte; // ���͵�ǰ�ַ�
}

/*
 * @fn:     void UARTSendString(uint8_t *pbuff)
 * @brief:  ͨ��UART�����ַ���
 * @para:   pbuff:Ҫ���͵��ַ���ָ��
 * @return: ��
 * @comment:�����ַ���ֱ�������ַ���������'\0'
 */
void UARTSendString(uint8_t *pbuff) {
    while (*pbuff) {
        UARTSendByte(*pbuff++);
        while (UCA0STAT & UCBUSY);
    }
}

/*
 * @fn:     void PrintNumber(int num)
 * @brief:  ͨ�����ڷ�������
 * @para:   num:Ҫ���͵�����
 * @return: ��
 * @comment:������ת��Ϊ�ַ�����ͨ�����ڷ���
 */
void PrintNumber(int num) {
    char buffer[12];
    int i = 0;

    // ������
    if (num < 0) {
        buffer[i++] = '-';
        num = -num;
    }

    // ������������
    int len = 0;
    while (num > 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
        len++;
    }
    if (len == 0) {  // ����������
        buffer[i++] = '0';
    }

    // ��ת�ַ���
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++;
        end--;
    }
    buffer[i] = '\0'; // ����ַ���������

    UARTSendString((uint8_t *)buffer); // ����ת������ַ���
}

/*
 * @fn:     void PrintFloat(float num, int decimalPlaces)
 * @brief:  ͨ�����ڷ��͸�����
 * @para:   num:Ҫ���͵ĸ�����
 *         decimalPlaces:С������λ��
 * @return: ��
 * @comment:��������ת��Ϊ�ַ�����ͨ�����ڷ��ͣ������������ֺ�С������
 */
void PrintFloat(float num, int decimalPlaces) {
    char str[24]; // ���������С�Է�ֹ���
    int intPart = (int)num; // ��������
    float decPart = num - (float)intPart; // С������
    int len = 0;

    // ������������
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

    // ����С������
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
        // ȷ��С�����ֵ�ĩβ��Ҳ����ӡ����
        while (decimalLen < decimalPlaces) {
            str[len++] = '0';
            decimalLen++;
        }
    }

    str[len] = '\0'; // �ַ���������
    UARTSendString((uint8_t *)str);
}

/*
 * @fn:     void InitADC(void)
 * @brief:  ��ʼ��ADCģ��
 * @para:   ��
 * @return: ��
 * @comment:����ADC10�Ĵ�����ʹ���ڲ��¶ȴ�������Ϊ����
 */
void InitADC(void) {
    ADC10CTL1 = INCH_10 + ADC10DIV_3; // Temp Sensor ADC10CLK/4
    ADC10CTL0 = SREF_1 + ADC10SHT_3 + REFON + ADC10ON + ADC10IE;
    __enable_interrupt();
}

/*
 * @fn:     uint16_t GetADCValue(void)
 * @brief:  ��ȡADCֵ
 * @para:   ��
 * @return: ADCֵ
 * @comment:����ADCת�����ȴ���ɣ�����ADCת�����
 */
uint16_t GetADCValue(void) {
    ADC10CTL0 |= ADC10SC | ENC;
    while (ADC10CTL1 & ADC10BUSY);
    return ADC10MEM;
}
uint8_t flag = 0;

//// ��ʱ����
void DelaySeconds(unsigned int seconds) {
    TA1CTL |= TASSEL_2 + MC_1 + ID_3; // ����ʱ��ԴΪSMCLK������ģʽΪUp Mode����Ƶϵ��Ϊ8
    TA1CCR0 = 62500 - 1; // ���ö�ʱ������Ϊ1�루1MHz/8 = 125kHz��1�� = 125000���ڣ�
    while (seconds--) {
        TA1CTL |= TAIE; // ������ʱ���ж�
        __bis_SR_register(GIE); // ��ȫ���ж�
        TA1CTL &= ~TAIFG; // �����ʱ���жϱ�־
        while (!TAIFG); // �ȴ���ʱ���жϷ���
        TA1CTL &= ~TAIFG; // �����ʱ���жϱ�־
    }
    TA1CTL &= ~MC_1; // �رն�ʱ��
}
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;
    InitSystemClock();
    InitUART();
    InitADC();
    /*����P1.6Ϊ���*/
    P1DIR |= BIT6;
    // �����ʱ��������ʱ3��
    DelaySeconds(3);
    TA1CTL = TASSEL_2 + MC_1 + ID_0; // ����ʱ��ԴΪSMCLK������ģʽΪUp Mode������Ƶ
   TA1CCR0 = 50000 - 1; // ���ö�ʱ������Ϊ50ms��1MHz/1 = 1MHz��50ms = 50000���ڣ�
   TA1CTL |= TAIE; // ������ʱ���ж�
   __bis_SR_register(GIE); // ��ȫ���ж�



    while(1)
    {
        if(flag == 1)
        {
            flag = 0;
            P1OUT ^= BIT6;
            temp = GetADCValue(); // ��ȡADCֵ
            // ��ADCֵת��Ϊ���϶Ⱥͻ��϶�
            IntDegC = ((temp - 746) / (0.000355 * 678) + 286) / 10.0; // ת��Ϊ���϶�
            IntDegF = IntDegC * 9.0 / 5.0 + 32.0; // ת��Ϊ���϶�

            UARTSendString("���϶�: ");
            PrintFloat(IntDegC,3); // �������϶��¶�
            UARTSendString(" C\n");
            UARTSendString("���϶�: ");
            PrintFloat(IntDegF,3); // ���ͻ��϶��¶�
            UARTSendString(" F\n");
        }
    }
    return 0;
}
#pragma vector = TIMER1_A1_VECTOR
// ָ���������Ϊ��ʱ��1���ж�������������

__interrupt void Time_Tick(void)
{
    // ����һ���жϷ�����Time_Tick������ʱ��1���жϷ���ʱ�������ô˺�����

    static uint8_t cnt = 0;
    // ����һ����̬�ֲ�����cnt�����ڼ����жϷ����Ĵ�����������static���ͣ���ֵ�ں�������֮�䱣�ֲ��䡣

    switch(TA1IV)
    {
    // ʹ��switch�����ݶ�ʱ��1���ж������ţ�TA1IV����ȷ���жϵ�ԭ��
    // TA1IV�Ƕ�ʱ��1�ж������ŵļĴ������������˲����жϵ�ԭ��

    case 0x02:
        // ���TA1IV��ֵ��0x02�����Ƕ�ʱ��1�Ĳ���/�Ƚ�ģ��1���жϣ�������û��ʵ���κβ�����
        break;

    case 0x04:
        // ���TA1IV��ֵ��0x04�����Ƕ�ʱ��1�Ĳ���/�Ƚ�ģ��2���жϣ�����ͬ��û��ʵ���κβ�����
        break;

    case 0x0A:
        // ���TA1IV��ֵ��0x0A�����Ƕ�ʱ��1�Ķ�ʱ������жϣ�Timer Overflow����
        cnt++;
        // ÿ�ζ�ʱ�����ʱ��cnt������ֵ����1��

        if(cnt == 20)
        {
            // ÿ20�ζ�ʱ���������1�룬��Ϊÿ���������50ms����ִ�����²�����
            cnt = 0; // �����������׼����һ�ּ�����
            flag = 1; // ����flag��־λ����ʾ1��ʱ�䵽�ˣ�����ѭ���н����������־λ����ʱ��ͷ�תLED״̬��
        }
        break;

    default:
        // ���TA1IV��ֵ���������κ�һ���������ִ���κβ�����
        break;
    }
}

#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void) {
    __bic_SR_register_on_exit(CPUOFF);
}
