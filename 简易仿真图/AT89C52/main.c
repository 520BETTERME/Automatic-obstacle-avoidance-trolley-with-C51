#include <mcs51/at89x52.h>
#include <stdio.h>
typedef unsigned int uint;
typedef unsigned char uchar;

//С���ƶ�
#define FRONT 0xaa
#define BACK  0x55
#define FRONT_LEFT  0xad
#define FRONT_RIGHT  0xda
#define BACK_LEFT 0x5d
#define BACK_RIGHT 0xd5
#define STOP 0xff
/*С�����Ŷ���*/
#define CAR P0    //P0������Գ��ӵ���Ŀ���
#define SWITCH_SELF_CONTROL P1_0    //�ⲿ�ж�1������־�����жϴ�����С���Կ�
//����״ָ̬ʾ�ƣ��͵�ƽ
#define STOP_RED_LED P1_1   //ֹͣʱ��
#define BT_BLUE_LED P1_2   //��������ʱ��
#define SELF_GREEN_LED P1_3     //�Լ����Ƶ�ʱ��
#define FRONT_SENSER P1_4    //ǰ�����ϴ�����
#define BACK_SENSER P1_5	 //�󷽱��ϴ�����
#define LEFT_SENSER P1_6	 //�󷽱��ϴ�����
#define RIGHT_SENSER P1_7    //�ҷ����ϴ�����
#define SEG P2    //�����߶�����ܣ���ʾ��������õľ���
#define STEER_PWM P3_2    //���pwm�����ź����
#define ECHO P3_4    //������ģ������źſڣ�ECHO
#define TRIG P3_5    //������ģ�鴥���źſڣ�TRIG
#define M_PWM P3_6    //����ٶȿ���pwm�ź�
/*��������*/
#define __nop __asm nop __endasm    //�ӳ�һ����������
#define PWM_CYCLE 5    //pwm�źŵ�����
#define CMD_TIME 20    //ִ������ָ���ʱ�䣬ms
#define STEER_S 0    //�����λ
#define STEER_P45 1    //���˳ʱ��ת��45��
#define STEER_P90 2    //���˳ʱ��ת��90��
#define STEER_N45 3    //�����ʱ��ת��45��
#define STEER_N90 4    //�����ʱ��ת��90��
//���������0~F
uchar __code seg[] = {0xc0, 0xf9, 0xa4, 0xb0, 
					  0x99, 0x92, 0x82, 0xf8, 
	                  0x80, 0x90, 0x88, 0x83, 
					  0xc6, 0xa1, 0x86, 0x8e};  
uchar speed = 5;	//С���ٶ�
uchar t0InterruptTimes;    //t0��ʱ����ǰ�������
uchar t2InterruptTimes;    //t0��ʱ����ǰ�������
uchar angle;	//���ת���Ƕ�
uchar timer0For;	//��ʱ��0Ϊ���(0)���ǳ�����(1)������ʱ
__bit isOverstep = 0;	//�����Զ������������Χ

//�ӳٺ�����11.0592MHz n= 1,��Լ�ӳ�1ms
void delay(uint n){
	uint i,j;
	for(i=n;i>0;i--){
		for(j=112;j>0;j--);
	}
}

//�򴮿����,�Զ�����
void putchar(char c) {
  SBUF = c;
  while(!TI);
  TI = 0;
}

//����������
void sensorTrigger() {
	if(!(BACK_SENSER && FRONT_SENSER && LEFT_SENSER&& RIGHT_SENSER)) {
		SWITCH_SELF_CONTROL = 0;
	}
}

//����״ָ̬ʾ�����
//0:ֹͣ 1���Զ� 2:����
void ledStatus(uchar s) {
	switch(s) {
		case(0):
			STOP_RED_LED = 0;	//ָֹͣʾ����
			BT_BLUE_LED = 1;
			SELF_GREEN_LED = 1;
		break;
		case(1):
			STOP_RED_LED = 1;
			BT_BLUE_LED = 1;	  
			SELF_GREEN_LED = 0;    //�Կ�ָʾ����
		break;
		case(2):
			STOP_RED_LED = 1;
			SELF_GREEN_LED = 1;
			BT_BLUE_LED = 0;	//��������ָʾ����
		break;
	}	
}

//���ת�� angle:ת���ĽǶ�
void setTurnAngle(uchar a) {

	switch(a) {
		//��λ
		case(STEER_S):angle = 3; break;
		//˳45��
		case(STEER_P45):angle = 4; break;
		//˳90��
		case(STEER_P90):angle = 5; break;
		//��45��
		case(STEER_N45):angle = 2; break;
		//��90��
		case(STEER_N90):angle = 1; break;
	}
	timer0For = 0;
	initTimer0();	
}

void steerTurn() {

	t0InterruptTimes++;
	if (t0InterruptTimes > PWM_CYCLE) {
		t0InterruptTimes = 0;
	}
	// printf("%d", t0InterruptTimes);
	if (t0InterruptTimes < angle) {
		 STEER_PWM = 1;
	}else {
		STEER_PWM = 0;
	}
	startSR04();
}

//����������ģ�鹤��
void trigger()
{
  TRIG = 1;
  // �ߵ�ƽ����10us����
  __nop; __nop; __nop; __nop; __nop;
  __nop; __nop; __nop; __nop; __nop;
  __nop; __nop; __nop; __nop; __nop;
  __nop; __nop; __nop; __nop; __nop;
  TRIG = 0;
}

//���������
uchar calculate() {

	uchar time, distance;
	// ��ȡ��ʱ����ֵ
	time = TH0 * 256 + TL0;
	// ���ö�ʱ����ֵ
	TH0 = 0;
	TL0 = 0;
	time *= 1.085;
	// ���� = 340m/s = 0.34m/ms = 0.00034m/us = 0.034cm/us
	// ���� = �ߵ�ƽʱ�� * ���� / 2
	distance = (time * 0.017);
	if(isOverstep) {
		isOverstep = 0;
		SEG = 0xff;
		printf("overstep\n");
	}else {
		SEG = seg[distance];
		printf("distance = %dcm\n", distance);
	}
}

//����������ģ��
void startSR04() {

	timer0For = 1;
	initTimer0();
	trigger();
	while(!ECHO);
	TR0 = 1;
	while(ECHO);
	TR0 = 0;
	calculate();
}

//Զ���󷽵��ϰ���
void awayLEFTObs() {
	// CAR = BACK;
	// delay(400);
	CAR = FRONT_RIGHT;
	delay(700);
}

//Զ���ҷ����ϰ���
void awayRightObs() {
	// CAR = BACK;
	// delay(400);
	CAR = FRONT_LEFT;
	delay(700);

}

//Զ��ǰ����ϰ���
void awayFrontObs() {
	CAR = BACK;
	delay(200);
}

//Զ�������ϰ���
void awayBackObs() {
	CAR = FRONT;
	delay(400);
}

//�Լ�����
void selfControl() __interrupt 2 __using 0 {
	
	ledStatus(1);
	printf("self control\n");
	
	//ǰ�����ϰ����û���ϰ���
	if (FRONT_SENSER == 0 && BACK_SENSER == 1) {
		//�������ϰ����������û���ϰ���
		if ((LEFT_SENSER== 0 && RIGHT_SENSER == 0) || (LEFT_SENSER&& RIGHT_SENSER) == 1) {
			CAR = STOP;
		//�����ϰ���ҷ�û���ϰ���
		}else if (LEFT_SENSER== 0 && RIGHT_SENSER == 1) {
			CAR = STOP;
			setTurnAngle(STEER_P45);
			setTurnAngle(STEER_P90);
		//�ҷ����ϰ����û���ϰ���
		}else {
			CAR = STOP;
			setTurnAngle(STEER_N45);
			setTurnAngle(STEER_N90);
		}
	//�����ϰ��ǰ��û���ϰ���
	}else if (BACK_SENSER == 0 && FRONT_SENSER == 1){
		//����û���ϰ���
		if ((LEFT_SENSER && RIGHT_SENSER) == 1) {
			CAR = FRONT;
		//���Ҷ����ϰ���
		}else if ((LEFT_SENSER || RIGHT_SENSER) == 0) {
			CAR = BACK;
			delay(400);
			//todo:������������߳�ȥ
		//�����ϰ���ҷ�û���ϰ���
		}else if (LEFT_SENSER== 0 && RIGHT_SENSER == 1) {
			CAR = STOP;
			setTurnAngle(STEER_P45);
			setTurnAngle(STEER_P90);
		//�ҷ����ϰ����û���ϰ���
		}else {
			CAR = STOP;
			setTurnAngle(STEER_N45);
			setTurnAngle(STEER_N90);
		}
	//ֻ�����ϰ���
	}else if (LEFT_SENSER== 0 && (RIGHT_SENSER && BACK_SENSER && FRONT_SENSER) == 1 ){
		CAR = STOP;
		setTurnAngle(STEER_P45);
		setTurnAngle(STEER_P90);
	//ֻ���ҷ����ϰ���
	}else if (RIGHT_SENSER == 0 && (LEFT_SENSER && FRONT_SENSER && BACK_SENSER) == 1) {
		CAR = STOP;
		setTurnAngle(STEER_N45);
		setTurnAngle(STEER_N90);
	//�����ϰ���
	}else {
		CAR = STOP;
	}
	SWITCH_SELF_CONTROL = 1;
}

//��������С��
void btControl(uchar cmd) {
	
	ledStatus(2);
	switch(cmd) {
		case('f'): CAR = FRONT; break;
		case('b'): CAR = BACK; break;
		case('l'): CAR = FRONT_LEFT; break;
		case('r'): CAR = FRONT_RIGHT; break;
		case('s'): CAR = STOP; break;
		case('a'): 
			if (speed < 5) {
				speed++;
			}; 
		break;
		case('d'): 
			if (speed > 0) {
				speed--;
			}
		break;
		default:CAR = STOP; break;
	}
	initTimer2();
}

//��ʼ���ж�
void initInterrupt() {

	EA = 1;			//�������ж�
	ES = 1;			//�����п��ж�
	ET0 = 1;		//����ʱ��0�ж�
	ET2 = 1;		//����ʱ��2�ж�
	EX1 = 1;		//�����ⲿ�ж�1�ж�
	IT1 = 0;		//�͵�ƽ����
}

//��ʼ����ʱ��0
void initTimer0() {

	TMOD = 0x01;	//������ʽ1
	if (timer0For == 0) {
		
		TR0 = 1;	//������ʱ��0
	}else {
		TH0 = 0xFE;
		TL0 = 0x33;
	}
}

void reloadTimer0() {

	if (timer0For == 0) {
		TH0 = 0xFE;
		TL0 = 0x33;	
	}else {
		TH0 = 0xFE;
		TL0 = 0x33;
	}
}

//��ʼ������
void initSerial() {
	
	SCON = 0x50;	//���пڹ���ģʽ1
	PCON = 0x00;
	RI = 0;			//�����жϱ�־����

	TMOD = 0x21;	//��ʱ��T1��ʽ2 T0������ʽ1
	TL1 = 0xfd;
	TH1 = 0xfd;
	TR1 = 1;		//��ʱ����ʼ����
}

//��ʼ����ʱ��2,�����Ҫ1ms
void initTimer2() {
	T2MOD = 0x00;	//��ʱ��T2���ϼ���
	C_T2 = 0;		//ѡ��T2Ϊ��ʱ����ʽ
	CP_RL2 = 0;		//T2�Զ�װ��
	TH0 = 0x0fc;
    TL0 = 0x66;
}

void timer0() __interrupt 1 __using 0 {

	if (timer0For == 0) {
		reloadTimer0();
		steerTurn();
	} else {
		isOverstep = 1;
	}

}


//���п��ж�
void serial() __interrupt 4 __using 1 {
	
	RI = 0;
	//putchar(SBUF);	//���ܵ������ٷ��͸����ƶ�
	btControl(SBUF);
}

//��ʱ��2�ж�
void timer2() __interrupt 5 __using 2 {

	uchar a;
	TF2 = 0;	//�����0
	t2InterruptTimes++;
	a = t2InterruptTimes % PWM_CYCLE;
	if (t2InterruptTimes == CMD_TIME) {
		t2InterruptTimes = 0;
		CAR = STOP;
		TR2 = 0;	//���20�Σ�˵��ִ�����������͵�ָ��20ms�ˣ�ֹͣ������2������ִֹͣ��ָ��ȴ����������µ�ָ��
	}
	if (a <= speed) {
		M_PWM = 1;
	}else {
		M_PWM = 0;
	}	
}

void main() {

	initInterrupt();
	initTimer0();
	initSerial();
	initTimer2();
	while(1) {
		sensorTrigger();
		if (SWITCH_SELF_CONTROL) {
			ledStatus(0);
		}
	}
}
