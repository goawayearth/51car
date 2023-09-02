#include "reg52.h"

/****************�޸Ľӿ�************************/
//���ٶ�
#define DUTY 11.5
//�Ƿ�������1�ǿ��ԣ�0�ǲ�����,Ĭ�Ͽ���
unsigned char ableBack=1;
//�ĵ���ʱ��
#define BACK_TIME 4
//�ı���������cm�������Ͼ͸Ĵ�һ��
#define MIN_DISTANCE 20

unsigned char change=0;//����������������⣬�ĳ�1����
/*************************************************/


#define READY_RIGHT 1
#define READY_LEFT 2
unsigned char flag=0;
unsigned char block=0;//0��û���ϰ���1�����ϰ�
unsigned char stay_time=0;
unsigned int foronesecond=0;

//���������ƹܽ�
//����
sbit IN1=P3^4;
sbit IN2=P3^5; 
//�ҵ��
sbit IN3=P1^2;
sbit IN4=P1^3;

//ʹ������
sbit ENTER1=P1^4;
sbit ENTER2=P1^5;

//��������Ӧ���ƹܽ�
sbit Left_In_1=P2^3;	//��������
sbit Left_In_2=P2^4;	//���ڲ����
sbit Right_In_1=P2^2;	//��������
sbit Right_In_2=P2^5;	//���ڲ����

//����������
sbit TRIG = P2^7; //trig
sbit ECHO = P2^6; //echo

unsigned int time_cnt=0;	//��ʱ���жϼ�������
unsigned int freq=100;  //PWM���Ƶ��100HZ
float S;//�洢������������ľ���

void Delay(unsigned int xms)//��ʱ x*1ms
{
	while(xms--)
	{
		unsigned char i, j;
		i = 2;
		j = 199;
		do
		{
			while (--j);
		} while (--i);
	}
}

//��ʱ����10*us��us��
void delay_10us(unsigned int us)
{
	while(us--);
}

//��ʱ��1��ʼ������
//��ʱʱ�䣺0.1ms
void Timer1_Init(void)
{
	TMOD &= 0x0F; //����TMOD�ĸ�4λ,׼�����ö�ʱ��1
	TMOD |= 0x10; //���ö�ʱ��1Ϊ16λ�Զ�����ģʽ
	TH1 = 0xFF; //��ʱ��1��8λ��ֵ
	TL1 = 0x9C; //��ʱ��1��8λ��ֵ,���ö�ʱ����Ϊ0.1ms
	TR1 = 1; //������ʱ��1
	ET1 = 1; //����ʱ��1�ж�
	TF1 = 0; //�����ʱ��1�жϱ�־λ
	EA = 1; //�������ж�
	PT1 = 0; //���ö�ʱ��1�ж����ȼ�Ϊ��
}

//��ʱ��0��ʼ������
void Timer0_Init(void)
{
	TMOD &= 0xF0;//�������λ
	TMOD|=0X01; //�� T0 Ϊ��ʽ 1�� GATE=1��
	TH0=0;
	TL0=0;	
	EA=1; //�������ж�
	ET0=1; //���� T0 �ж�
	TR0=0;
	TRIG = 0;
}

//��������ʼ��
void initUart()
{
	EA = 1;
	ES = 1;
	SCON = 0x40;
	TH2 = 0xFF;
	TL2 = 0xFD;
	RCAP2H = 0xFF;
	RCAP2L = 0xFD;
	T2CON = 0x34;
}

//������ģ������
void StartModule()
 {
 	TRIG = 1;
	delay_10us(2);//20us����ʱ��֤�ߵ�ƽ����ʱ���㹻������ģ��
	TRIG = 0;
 }

 //�����ϰ������
 void Conut(void) //�������
{	
	unsigned int time;
	time=TH0*256+TL0;
	TH0=0;
	TL0=0;
	S=(time*1.7)/100;
}

//����ǰ��
void left_forward()
{
	IN1=1;IN2=0;
}
//���ֺ���
void left_back()
{
	IN1=0;IN2=1;
}
//����ֹͣ
void left_stop()
{
	IN1=0;IN2=0;
}

//����ǰ��
void right_forward()
{
	IN3=0;IN4=1;
}
//���ֺ���
void right_back()
{
	IN3=1;IN4=0;
}
//����ֹͣ
void right_stop()
{
	IN3=0;IN4=0;
}

//����������ҵ���жϺ�����
void main()
{
	
	Timer1_Init();//��ʱ��1��ʼ��
	Timer0_Init();//��ʱ��0��ʼ��
	initUart();//��������ʼ��
	
	while(1)
	{
		StartModule();//������ģ������
		while(!ECHO); //�� ECHO Ϊ��ʱ�ȴ�
		TR0=1; //��������
		while(ECHO); //�� ECHOΪ 1 �������ȴ�
		TR0=0; //�رռ���
		Conut(); //����
		if(S<MIN_DISTANCE)//�ϰ���С��MIN_DISTANCE���ף�С��ֹͣ
		{
			block=1; 
			ENTER1=0;
			ENTER2=0;
			Delay(2000);
		}
		else
		{
			block=0;
		}
	}
}

//��ʱ��1�жϺ���
void Time1_Isr() interrupt 3
{
	TR1 = 0;	//�رն�ʱ��1
	TH1 = 0XFF;
	TL1 = 0X9C;	//��ʱ0.1ms

	foronesecond++;
	if(foronesecond>=10000)
	{
		foronesecond=0;
		//ÿ1s����һ��
		if(flag>=1)
		{
			stay_time++;
			if(stay_time>=BACK_TIME)
			{
				flag=0;
				stay_time=0;  
			}
		}
		
	}


	time_cnt++;	
	if(time_cnt>=freq)		
		time_cnt=0;

	//ռ�ձ�֮�ڣ����������ͨ��
	
	if(time_cnt<=DUTY && block==0)
	{
		ENTER1=1;
		ENTER2=1;
		// �� ��û��������
		if(Left_In_1==0 && Left_In_2==0 && Right_In_1==0 && Right_In_2==0)
		{
			right_forward();
			left_forward();
		}

		// �� ����������,ֱ��
		else if(Left_In_1==1 && Left_In_2==1 && Right_In_1==1 && Right_In_2==1)
		{
			right_forward();
			left_forward();
		}
		// �� �����������⵽
		else if(Left_In_1==1 && Left_In_2==1 && Right_In_1==0 && Right_In_2==1)
		{
			right_forward();
			if(flag>0 && ableBack==1)left_back();
			else left_stop();
		}
		// �� �Ҳ���������⵽
		else if(Left_In_1==0 && Left_In_2==1 && Right_In_1==1 && Right_In_2==1)
		{
			if(flag>0 && ableBack==1)right_back();
			else right_stop();
			left_forward();
		}
		//����������
		else if(Left_In_1==1 && Left_In_2==0 && Right_In_1==0 && Right_In_2==1)
		{
			if(change==1)
			{
				right_forward();
				if(flag>0 && ableBack==1)left_back();
				else left_stop();
			}
			else 
			{
				right_stop();
				left_forward();
			}
			flag=READY_LEFT;
			stay_time=0;
			
		}
		//����������
		else if(Left_In_1==0 && Left_In_2==1 && Right_In_1==1 && Right_In_2==0)
		{
			if(change==1)
			{
				if(flag>0 && ableBack==1)right_back();
				else right_stop();
				left_forward();
			}
			else
			{
				left_stop();
				right_forward();
			}
			flag=READY_RIGHT;
			stay_time=0;
		}
		
		// �� �����������ߣ�����û����
		else if(Left_In_1==0 && Left_In_2==1 && Right_In_1==0 && Right_In_2==0)//������ƫ
		{
			right_forward();//����ת��
			if(flag>0 && ableBack==1)left_back();
			else left_stop();
		}

		// �� ������������
		else if(Right_In_2==1 && Right_In_1==0 && Left_In_2==0 && Left_In_1==0)//������ƫ
		{
			if(flag>0 && ableBack==1)right_back();
			else right_stop();
			left_forward();//����ת��
		}
		// �� �м�������⵽
		else if(Left_In_1==0 && Left_In_2==1 && Right_In_1==0 && Right_In_2==1)
		{
			if(flag==READY_LEFT)
			{
				right_forward();
				left_back();
			}
			else if(flag==READY_RIGHT)
			{
				left_forward();
				right_back();
			}
			else
			{
				right_forward();
				left_forward();
			}
		}

		//������������
		else if(Left_In_1==1 && Right_In_1==0 )//��ƫ����
		{
			right_forward();//����ת��
			left_stop();//���ֵ�ת
			flag=READY_LEFT;
			stay_time=0;
		}
		//������������
		else if(Right_In_1==1&&Left_In_1==0)//����ƫ
		{
			right_stop();//���ֵ���
			left_forward();//����ת��
			flag=READY_RIGHT;
			stay_time=0;
		}
		//����û���ǵ��������ʱ��ֱ�а�
		else
		{
			right_forward();
			left_forward();
		}

	}
	//ռ�ձ�֮��,����ʱ��֮�ⲻת��
	else if(time_cnt>DUTY && block==0)
	{
		ENTER1=0;
		ENTER2=0; 
		right_stop(); 
		left_stop();
	}
	else
	{
		right_stop();
		left_stop();
	}

	//������ʱ����������һ�ε��ж�
	TR1 = 1;	//������ʱ��1		
}