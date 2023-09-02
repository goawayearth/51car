#include "reg52.h"

/****************修改接口************************/
//改速度
#define DUTY 11.5
//是否允许倒退1是可以，0是不可以,默认可以
unsigned char ableBack=1;
//改倒退时间
#define BACK_TIME 4
//改避障最大距离cm，不避障就改大一点
#define MIN_DISTANCE 20

unsigned char change=0;//如果锐角那里出现问题，改成1试试
/*************************************************/


#define READY_RIGHT 1
#define READY_LEFT 2
unsigned char flag=0;
unsigned char block=0;//0是没有障碍，1是有障碍
unsigned char stay_time=0;
unsigned int foronesecond=0;

//定义电机控制管脚
//左电机
sbit IN1=P3^4;
sbit IN2=P3^5; 
//右电机
sbit IN3=P1^2;
sbit IN4=P1^3;

//使能引脚
sbit ENTER1=P1^4;
sbit ENTER2=P1^5;

//定义红外感应控制管脚
sbit Left_In_1=P2^3;	//左外侧红外
sbit Left_In_2=P2^4;	//左内侧红外
sbit Right_In_1=P2^2;	//右外侧红外
sbit Right_In_2=P2^5;	//右内侧红外

//超声波引脚
sbit TRIG = P2^7; //trig
sbit ECHO = P2^6; //echo

unsigned int time_cnt=0;	//定时器中断计数次数
unsigned int freq=100;  //PWM输出频率100HZ
float S;//存储超声波测出来的距离

void Delay(unsigned int xms)//延时 x*1ms
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

//延时函数10*us（us）
void delay_10us(unsigned int us)
{
	while(us--);
}

//定时器1初始化函数
//定时时间：0.1ms
void Timer1_Init(void)
{
	TMOD &= 0x0F; //清零TMOD的高4位,准备设置定时器1
	TMOD |= 0x10; //设置定时器1为16位自动重载模式
	TH1 = 0xFF; //定时器1高8位初值
	TL1 = 0x9C; //定时器1低8位初值,设置定时周期为0.1ms
	TR1 = 1; //启动定时器1
	ET1 = 1; //允许定时器1中断
	TF1 = 0; //清除定时器1中断标志位
	EA = 1; //开启总中断
	PT1 = 0; //设置定时器1中断优先级为低
}

//定时器0初始化函数
void Timer0_Init(void)
{
	TMOD &= 0xF0;//清除低四位
	TMOD|=0X01; //设 T0 为方式 1， GATE=1；
	TH0=0;
	TL0=0;	
	EA=1; //开启总中断
	ET0=1; //允许 T0 中断
	TR0=0;
	TRIG = 0;
}

//超声波初始化
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

//超声波模块启动
void StartModule()
 {
 	TRIG = 1;
	delay_10us(2);//20us的延时保证高电平持续时间足够来启动模块
	TRIG = 0;
 }

 //计算障碍物距离
 void Conut(void) //计算距离
{	
	unsigned int time;
	time=TH0*256+TL0;
	TH0=0;
	TL0=0;
	S=(time*1.7)/100;
}

//左轮前进
void left_forward()
{
	IN1=1;IN2=0;
}
//左轮后退
void left_back()
{
	IN1=0;IN2=1;
}
//左轮停止
void left_stop()
{
	IN1=0;IN2=0;
}

//右轮前进
void right_forward()
{
	IN3=0;IN4=1;
}
//右轮后退
void right_back()
{
	IN3=1;IN4=0;
}
//右轮停止
void right_stop()
{
	IN3=0;IN4=0;
}

//主函数，主业在中断函数中
void main()
{
	
	Timer1_Init();//定时器1初始化
	Timer0_Init();//定时器0初始化
	initUart();//超声波初始化
	
	while(1)
	{
		StartModule();//超声波模块启动
		while(!ECHO); //当 ECHO 为零时等待
		TR0=1; //开启计数
		while(ECHO); //当 ECHO为 1 计数并等待
		TR0=0; //关闭计数
		Conut(); //计算
		if(S<MIN_DISTANCE)//障碍物小于MIN_DISTANCE厘米，小车停止
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

//定时器1中断函数
void Time1_Isr() interrupt 3
{
	TR1 = 0;	//关闭定时器1
	TH1 = 0XFF;
	TL1 = 0X9C;	//定时0.1ms

	foronesecond++;
	if(foronesecond>=10000)
	{
		foronesecond=0;
		//每1s进来一次
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

	//占空比之内，给电机正常通电
	
	if(time_cnt<=DUTY && block==0)
	{
		ENTER1=1;
		ENTER2=1;
		// √ 都没遇到黑线
		if(Left_In_1==0 && Left_In_2==0 && Right_In_1==0 && Right_In_2==0)
		{
			right_forward();
			left_forward();
		}

		// √ 都遇到黑线,直行
		else if(Left_In_1==1 && Left_In_2==1 && Right_In_1==1 && Right_In_2==1)
		{
			right_forward();
			left_forward();
		}
		// √ 左侧三个都检测到
		else if(Left_In_1==1 && Left_In_2==1 && Right_In_1==0 && Right_In_2==1)
		{
			right_forward();
			if(flag>0 && ableBack==1)left_back();
			else left_stop();
		}
		// √ 右侧三个都检测到
		else if(Left_In_1==0 && Left_In_2==1 && Right_In_1==1 && Right_In_2==1)
		{
			if(flag>0 && ableBack==1)right_back();
			else right_stop();
			left_forward();
		}
		//可能有问题
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
		//可能有问题
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
		
		// √ 左内遇到黑线，左外没遇到
		else if(Left_In_1==0 && Left_In_2==1 && Right_In_1==0 && Right_In_2==0)//略向左偏
		{
			right_forward();//右轮转动
			if(flag>0 && ableBack==1)left_back();
			else left_stop();
		}

		// √ 右内遇到黑线
		else if(Right_In_2==1 && Right_In_1==0 && Left_In_2==0 && Left_In_1==0)//略向右偏
		{
			if(flag>0 && ableBack==1)right_back();
			else right_stop();
			left_forward();//左轮转动
		}
		// √ 中间两个检测到
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

		//左外遇到黑线
		else if(Left_In_1==1 && Right_In_1==0 )//左偏严重
		{
			right_forward();//右轮转动
			left_stop();//左轮倒转
			flag=READY_LEFT;
			stay_time=0;
		}
		//右外遇到黑线
		else if(Right_In_1==1&&Left_In_1==0)//向右偏
		{
			right_stop();//右轮倒动
			left_forward();//左轮转动
			flag=READY_RIGHT;
			stay_time=0;
		}
		//其他没考虑的情况，暂时就直行吧
		else
		{
			right_forward();
			left_forward();
		}

	}
	//占空比之外,工作时间之外不转动
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

	//开启定时器，接收下一次的中断
	TR1 = 1;	//开启定时器1		
}