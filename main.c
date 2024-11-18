#include <msp430.h> 
#include "stdint.h"

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

/*
 * @fn:    void UARTSendString(uint8_t *pbuff, uint_8 num)
 * @brief: ��ʼ�����ڷ����ַ���
 * @para:  pbuff:ָ��Ҫ�����ַ�����ָ��
 *         num:Ҫ���͵��ַ�����
 * @return:none
 * @comment: ��ʼ�����ڷ����ַ���
 */
void UARTSendString(uint8_t *pbuff, uint8_t num)
{
    uint8_t cnt = 0;
    for(cnt = 0; cnt < num; cnt++)
    {
        /*�ж��Ƿ����������ڷ���*/
        while(UCA0STAT & UCBUSY);
        UCA0TXBUF = *(pbuff + cnt);
    }
}

/*
 * @fn:    void PrintNumber(uint16_t num)
 * @brief: ��ʼ�����ڷ�������
 * @para:  num������
 * @return:none
 * @comment: ��ʼ�����ڷ�������
 */
void PrintNumber(uint16_t num)
{
    uint8_t cnt = 0;
    uint8_t buff[6] = {0,0,0,0,0,'\n'};

    for(cnt = 0; cnt < 5; cnt++)
    {
        buff[4 - cnt] = (uint8_t)(num % 10 + '0');
        num /= 10;
    }
    UARTSendString(buff,6);
}

/*
 * @fn:    void PrintFloat(float num)
 * @brief: ��ʼ�����ڷ��͸���������
 * @para:  num�������ͱ���
 * @return:none
 * @comment: ��ʼ�����ڷ��͸���������
 */
void PrintFloat(float num)
{
    uint8_t buff[] = {0,'.',0,0,0,'\n'};
    uint16_t temp = (uint16_t)(num * 1000);
    buff[0] = (uint8_t)(temp / 1000) + '0';
    buff[2] = (uint8_t)((temp % 1000) / 100) + '0';
    buff[3] = (uint8_t)((temp / 100) / 10) + '0';
    buff[4] = (uint8_t)(temp % 10) + '0';
    UARTSendString(buff,6);
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

int main(void)
{
    float voltage = 0.0;
    uint16_t adcvalue = 0;
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	InitSystemClock();
	InitUART();
	InitADC();
	
	while(1)
	{
	    adcvalue = GetADCValue();
	    voltage = adcvalue * 2.5 / 1023;

	    UARTSendString("ADC10ת�ӽ��Ϊ��",17);
	    PrintNumber(adcvalue);
	    UARTSendString("��Ӧ��ѹֵΪ��",14);
	    PrintFloat(voltage);
        __delay_cycles(300000);
	}
	return 0;
}
