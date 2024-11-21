#include <msp430.h>
#include "stdint.h"

// ����һ�����飬���ڴ洢ADCת���Ľ��
uint16_t adcbuff[50] = {0};
// ������������ڴ洢ADCת����������ֵ����Сֵ
uint16_t maxval = 0;
uint16_t minval = 0;
// ������������ڴ洢ADCת������ķ��ֵ��Vpp��
uint16_t vpp = 0;

/*
 * @fn:     void InitSystemClock(void)
 * @brief:  ��ʼ��ϵͳʱ��
 * @para:   none
 * @return: none
 * @comment:����ϵͳʱ��Ϊ1MHz
 */
void InitSystemClock(void) {
    // ����DCOƵ��
    DCOCTL = CALDCO_1MHZ;
    // ���÷�Ƶ������
    BCSCTL1 = CALBC1_1MHZ;
    // ѡ��DCO��Ϊʱ��Դ
    BCSCTL2 &= ~SELS;
    // ����ʱ�ӷ�Ƶϵ��
    BCSCTL2 &= ~(DIVS0 | DIVS1);
}

/*
 * @fn:     void InitUART(void)
 * @brief:  ��ʼ��UARTģ��
 * @para:   none
 * @return: none
 * @comment:����UARTģ�飬���ò����ʵȲ���
 */
void InitUART(void) {
    // ����UART��λ״̬
    UCA0CTL1 |= UCSWRST;
    // ����UARTΪ�첽ģʽ
    UCA0CTL0 &= ~UCSYNC;
    // ѡ��SMCLK��Ϊʱ��Դ
    UCA0CTL1 |= UCSSEL1;
    // ���ò�����
    UCA0BR0 = 0x68;
    UCA0BR1 = 0x00;
    // ���õ��ƼĴ���
    UCA0MCTL = UCBRS_0 + UCBRF_0;
    // ѡ��P1.1��P1.2��ΪUART��TXD��RXD
    P1SEL |= BIT1 + BIT2;
    P1SEL2 |= BIT1 + BIT2;
    // �˳�UART��λ״̬
    UCA0CTL1 &= ~UCSWRST;
}

/*
 * @fn:     void UARTSendByte(uint8_t byte)
 * @brief:  ͨ��UART���͵����ֽ�
 * @para:   byte:Ҫ���͵��ֽ�
 * @return: none
 * @comment:�ȴ�UART���ͻ�����Ϊ�գ�Ȼ����һ���ֽ�
 */
void UARTSendByte(uint8_t byte) {
    while (UCA0STAT & UCBUSY);
    UCA0TXBUF = byte;
}

/*
 * @fn:     void UARTSendString(uint8_t *pbuff)
 * @brief:  ͨ��UART�����ַ���
 * @para:   pbuff:ָ��Ҫ�����ַ�����ָ��
 * @return: none
 * @comment:�����ַ���ֱ���ַ���������'\0'
 */
void UARTSendString(uint8_t *pbuff) {
    while (*pbuff) {
        UARTSendByte(*pbuff++);
    }
}

/*
 * @fn:     void PrintNumber(int num)
 * @brief:  ͨ��UART��������
 * @para:   num:Ҫ���͵�����
 * @return: none
 * @comment:������ת��Ϊ�ַ�����ͨ��UART���ͣ�������������
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
 * @brief:  ͨ��UART���͸�����
 * @para:   num:Ҫ���͵ĸ�����
 *         decimalPlaces:С������λ��
 * @return: none
 * @comment:��������ת��Ϊ�ַ�����ͨ��UART���ͣ������������ֺ�С������
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
 * @brief:  ��ʼ��ADCģ��
 * @para:   none
 * @return: none
 * @comment:����ADCģ�飬���ò���ʱ�䡢�ο���ѹ�Ȳ���
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
 * @brief:  ��ȡADCת��ֵ
 * @para:   none
 * @return: ADCת�����
 * @comment:����ADCת�����ȴ���ɣ�����ת�����
 */
uint16_t GetADCValue(void) {
    ADC10CTL0 |= ADC10SC | ENC;
    while (ADC10CTL1 & ADC10BUSY);
    return ADC10MEM;
}

/*
 * @fn:     void StartADCConvert(void)
 * @brief:  ����ADCת��
 * @para:   none
 * @return: none
 * @comment:����ADCת�����ȴ����
 */
void StartADCConvert(void) {
    // ����ADCת��
    ADC10CTL0 |= ADC10SC | ENC;
    // �ȴ�ת�����
    while (ADC10CTL1 & ADC10BUSY);
}

/*
 * @fn:     uint16_t Max(uint16_t *numptr, uint16_t num)
 * @brief:  �ҳ������е����ֵ
 * @para:   numptr:����ָ�룬num:���鳤��
 * @return: �����е����ֵ
 * @comment:���������ҳ����ֵ
 */
uint16_t Max(uint16_t *numptr, uint16_t num) {
    uint16_t cnt = 0;
    uint16_t max = numptr[0];
    for (cnt = 1; cnt < num; cnt++) {
        // �����ǰֵ������֪�����ֵ����������ֵ
        if (numptr[cnt] > max) {
            max = numptr[cnt];
        }
    }
    return max;
}

/*
 * @fn:     uint16_t Min(uint16_t *numptr, uint16_t num)
 * @brief:  �ҳ������е���Сֵ
 * @para:   numptr:����ָ�룬num:���鳤��
 * @return: �����е���Сֵ
 * @comment:���������ҳ���Сֵ
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
 * @brief:  ���������ƽ��ֵ
 * @para:   datptr:����ָ�룬num:���鳤��
 * @return: �����ƽ��ֵ
 * @comment:�����������ƽ��ֵ
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
        // ֹͣ���Ź���ʱ��
        WDTCTL = WDTPW | WDTHOLD;
        // ��ʼ��ϵͳʱ��
        /*��ʼ��LED2����IO��P1.6Ϊ���*/
        P1DIR |= BIT6;
        /*��ʼ��LED2����IO��P1.6Ϊ�͵�ƽ��LED2��ʼ״̬Ϊ��*/
        P1OUT &= ~BIT6;
        /*��ʼ����������IO��P1.3Ϊ����*/
        P1DIR &= ~BIT3;
        /*ʹ��P1.3�ڵ���������*/
        P1REN |= BIT3;
        P1OUT |= BIT3;
        InitSystemClock();
        // ��ʼ��UART
        InitUART();
        // ��ʼ��ADC
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
                // �������ֵ����Сֵ
                maxval = Max(adcbuff, 50);
                minval = Min(adcbuff, 50);
                // ������ֵ
                vpp = maxval - minval;
                // ͨ��UART�������ֵ
                UARTSendString("Max: ");
                PrintNumber(maxval);
                UARTSendString("\n");

                // ͨ��UART������Сֵ
                UARTSendString("Min: ");
                PrintNumber(minval);
                UARTSendString("\n");

                // ͨ��UART���ͷ�ֵ
                UARTSendString("Vpp: ");
                PrintNumber(vpp);
                UARTSendString("\n");

                // ͨ��UART����ƽ��ֵ
                UARTSendString("Average: ");
                PrintFloat((float)Average(adcbuff, 50) * 2.5 / 1023, 3);
                UARTSendString("\n");

                // ��ʱ���ȴ���һ��ת��
                __delay_cycles(1000000);
            }
        }
        return 0;
}
