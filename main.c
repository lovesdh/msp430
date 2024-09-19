#include <msp430.h> 


/**
 * main.c
 */

void delay_nms(unsigned int n)
  {
    unsigned int j;
    for (j=0;j<n;j++)
    {
      //delay_1ms();
      _delay_cycles(400);
    }
  }

// *************************************************************
// ������
// *************************************************************
void main(void)
{
  unsigned const PWMPeriod = 1500; //����PWMPeriod����
  volatile unsigned int i;        //����
  WDTCTL = WDTPW + WDTHOLD;   // �رտ��Ź�
  BCSCTL1= CALBC1_1MHZ;        //1M�ڲ�ʱ�ӣ�����Ҳ����
  DCOCTL = CALDCO_1MHZ;
  P1DIR |=BIT6;              // ���� P1.6Ϊ���
  P1SEL |=BIT6;              // ���� P1.6ΪTA0.1���
  TACCR0 = PWMPeriod;           // ����PWM ����
  TACCTL1 = OUTMOD_7;           // ����PWM ���ģʽΪ��7 - PWM��λ/��λģʽ��
                              // �������ƽ��TAR��ֵ����CCR1ʱ��λΪ0����TAR��ֵ����CCR0ʱ��λΪ1���ı�CCR1���Ӷ�����PWM����ʵģʽ2Ҳ����
   TACTL =  TASSEL_2 +MC_1;    // ����TIMERA��ʱ��ԴΪSMCLK, ����ģʽΪup,��CCR0���Զ���0��ʼ����
  while(1)
  {
   CCR1=0;//ȷ����ʼ�ǰ���
    //�������̣���������CCR1��ֵ��ʹ��ת��ʱ��䳤���ı�PWM��ռ�ձ�

    for(i=0;i<PWMPeriod;i++)     {
      CCR1=i;
      delay_nms(4-(i/500));  //ռ�ձȱ仯����ʱ�������ӳ�ʱ��ɸı�����Ʊ䰵���ٶ�
                          //�ڰ���ʱ���ӳ�delayʱ�䣬����ǿЧ��
     }
    //�������̣���������CCR1��ֵ��ʹ��ת��ʱ���̣��ı�PWM��ռ�ձ�
    for(i=PWMPeriod;i>0;i-=1)
    {
      CCR1=i;
      delay_nms(4-(i/500));           //ռ�ձȱ仯����ʱ�������ӳ�ʱ��ɸı�����Ʊ䰵���ٶ�
                                    //�ڰ���ʱ���ӳ�delayʱ�䣬����ǿЧ��
    }
     CCR1=0;  //ȷ���ư�
//     delay_nms(600); //��0.6S��ǿЧ��
  }
}

