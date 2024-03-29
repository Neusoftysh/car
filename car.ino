#include <Audio.h>
#include <SPI.h>
#include <stdio.h>
#include "Adafruit_ILI9341.h"
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF
// For the Adafruit shield, these are the default.
#define TFT_RST 18
#define TFT_DC  25
#define TFT_CS  -1
// the setup function runs once when you press reset or power the board
//A2,A3,A4,A5
#define L2 A2
#define L1 A3
#define R1 A4
#define R2 A5
#define SideL A1
#define SideR A0
int SL;    //左循迹红外传感器状态
int SR;    //右循迹红外传感器状态

int Left_motor_back = 9;       //左电机后退(IN1)
int Left_motor_go = 5;         //左电机前进(IN2)

int Right_motor_go = 6;        // 右电机前进(IN3)
int Right_motor_back = 3;    // 右电机后退(IN4)

int Right_motor_en = 8;      // 右电机前进(EN2)
int Left_motor_en = 7;      // 右电机后退(EN1)

int key = 4;                //定义按键 数字4 接口

float Kp = 10, Ki = 0.02, Kd = 20;
float error = 0, P = 0, I = 0, D = 0, PID_value = 0;
float previous_error = 0, previous_I = 0;
static int initial_motor_speed = 100; //期望速度
int left_motor_speed = 0;
int right_motor_speed = 0;
uint8_t irs = 0;
uint8_t sideirs = 0;
/*
place shows the area where the car in
place == 0 -> in line able place
place == 1 -> in line blank place
place == 2 -> in hill
*/
int place = 0;
int place_count = 0;
int tft_count = 0;
AudioClass *theAudio;
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

class Score
{
public:

  typedef struct {
    int fs;
    int time;
  } Note;

  void init() {
    pos = 0;
  }

  Note get(int sound) {
    return data[sound];
  }

private:

  int pos;

  Note data[9] =
  {
    {262,500},
    {294,500},
    {330,500},
    {349,500},
    {392,500},
    {440,500},
    {494,500},
    {523,1000},
    {0,0}
  };
};

Score theScore;


void setup() {
  Serial.begin(115200);
  theAudio = AudioClass::getInstance();
  theAudio->begin();
  theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP, 0, 0);
  theScore.init();
  pinMode(Left_motor_go, OUTPUT); // PIN 5 (PWM)
  pinMode(Left_motor_back, OUTPUT); // PIN 9 (PWM)
  pinMode(Right_motor_go, OUTPUT);// PIN 6(PWM) 
  pinMode(Right_motor_back, OUTPUT);// PIN 3 (PWM)
  pinMode(key, INPUT);//定义按键接口为输入接口
  digitalWrite(key, HIGH);
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(BLACK);
}


void read_ir_values()
{
  int temp[4];
  temp[0] = analogRead(L2);
  temp[1] = analogRead(L1);
  temp[2] = analogRead(R1);
  temp[3] = analogRead(R2);

  int side[2];
  side[0] = analogRead(SideR);
  side[1] = analogRead(SideL);

  for (int i = 0; i < 4; i++) {
    if (temp[i] < 300) {
      bitSet(irs, (i));
    }
    else {
      bitClear(irs, (i));
    }
  }

  for (int i = 0; i < 2; i++)
  {
    if (side[i] < 200)
    {
      bitSet(sideirs, (i));
    }
    else
    {
      bitClear(sideirs, (i));
    }
  }

  if (place == 0)
  {
    switch (irs) {
    case B0000:
      place_count++;
      if (place_count > 8)
      {
        initial_motor_speed = 70;
        switch (sideirs)
        {
        case B01:
          error = 3;
          break;
        case B10:
          error = -3;
          break;
        default:
          break;
          place_count = 0;
        }
      }
      initial_motor_speed = 100;
      break;

    case B0001: error = 5; place = 0; break;
    case B0011: error = 3; place = 0; break;
    case B0010: error = 1;  place = 0; break;
    case B0110: error = 0;  place = 0; break;
    case B0100: error = -1; place = 0; break;
    case B1100: error = -3; place = 0; break;
    case B1000: error = -5; place = 0; break;
    }
  }





}

void calculate_pid()
{
  P = error;
  I = I + error;
  D = error - previous_error;

  PID_value = (Kp * P) + (Ki * I) + (Kd * D);
  PID_value = constrain(PID_value, -100, 100);

  previous_error = error;
}

void motor_control()
{
  //计算每个电机的速度
  left_motor_speed = initial_motor_speed - PID_value;
  right_motor_speed = initial_motor_speed + PID_value;

  constrain(left_motor_speed, -255, 255); //速度限定在(-255，255)
  constrain(right_motor_speed, -255, 255);
  LeftRoll(left_motor_speed);
  RightRoll(right_motor_speed);
}

void LeftRoll(int speed)
{
  if (speed > 0)
  {
    digitalWrite(Left_motor_back, LOW);  // 左电机前进
    digitalWrite(Left_motor_go, HIGH);
    analogWrite(Left_motor_go, speed);//PWM比例0~255调速，左右轮差异略增减
  }
  else
  {
    digitalWrite(Left_motor_go, LOW);
    digitalWrite(Left_motor_back, HIGH);
    analogWrite(Left_motor_back, -speed);
  }
}

void RightRoll(int speed)
{
  if (speed > 0)
  {
    digitalWrite(Right_motor_go, HIGH);  // 右电机前进
    digitalWrite(Right_motor_back, LOW);
    analogWrite(Right_motor_go, speed);//PWM比例0~255调速，左右轮差异略增减
  }
  else
  {
    digitalWrite(Right_motor_go, LOW);
    digitalWrite(Right_motor_back, HIGH);
    analogWrite(Right_motor_back, -speed);
  }
}

void brake()
{
  digitalWrite(Right_motor_go, LOW);
  digitalWrite(Right_motor_back, LOW);
  digitalWrite(Left_motor_go, LOW);
  digitalWrite(Left_motor_back, LOW);
  //delay(time * 100);//执行时间，可以调整  
}

void keysacn()//按键扫描
{
  int val;




  val = digitalRead(key);//读取数字3 口电平值赋给val
  while (digitalRead(key))//当按键没被按下时，一直循环
  {
    val = digitalRead(key);//此句可省略，可让循环跑空
  }
  while (!digitalRead(key))//当按键被按下时
  {
    delay(10);  //延时10ms
    val = digitalRead(key);//读取数字3 口电平值赋给val
  }
}

void print_debug()
{
  // 打印串口调试信息
  Serial.print("IRS:");
  String irs2bin = String(irs, 2);
  int len = irs2bin.length();
  if (len < 5) {
    for (int i = 0; i < 5 - len; i++) {
      irs2bin += "0" + irs2bin;
    }
  }
  Serial.print(irs2bin);
  Serial.print("   ML:");
  Serial.print(left_motor_speed, OCT);
  Serial.print(" MR:");
  Serial.print(right_motor_speed, OCT);
  Serial.print(" er:");
  Serial.print(error, OCT);
  //  Serial.print(" P:");
  //  Serial.print(Kp, OCT);
  Serial.print(" PID:");
  Serial.print(PID_value, OCT);
  Serial.println();
}

void NowBeep(int sound)
{
  Score::Note theNote = theScore.get(sound);
  theAudio->setBeep(1, -40, theNote.fs);
  usleep(theNote.time * 1000);
  theAudio->setBeep(0, 0, 0);
}

void display_debug()
{
  tft.setTextSize(2);
  tft.setTextColor(GREEN, BLACK);
  tft.setCursor(0, 0);
  tft.print(analogRead(A2)); tft.print(",");
  tft.print(analogRead(A3)); tft.print(",");
  tft.print(analogRead(A4)); tft.print(",");
  tft.print(analogRead(A5)); tft.print(",");
  tft.setCursor(0, 20);
  switch (irs)
  {
  case B0000: tft.print("  -  -  -  -       "); break;
  case B0001: tft.print(" @  -  -  -       "); break;
  case B0011: tft.print(" @ @ -  -       "); break;
  case B0010: tft.print("  -  @  -   -     "); break;
  case B0110: tft.print("  -  @  @  -    "); break;
  case B0100: tft.print("  -   -   @   -    "); break;
  case B1100: tft.print("  -   -   @  @  "); break;
  case B1000: tft.print("  -   -    -  @   "); break;
  }
}


void loop() {

  //Serial.print(analogRead(A0)); Serial.print(",");
  //Serial.print(analogRead(A1)); Serial.print(",");
  Serial.print(analogRead(A2)); Serial.print(",");
  Serial.print(analogRead(A3)); Serial.print(",");
  Serial.print(analogRead(A4)); Serial.print(",");
  Serial.print(analogRead(A5)); Serial.print(",");
  //Serial.print(irs, BIN); Serial.println("");
  //display_debug();
  //NowBeep(1);
  read_ir_values();
  calculate_pid();
  motor_control();
  //print_debug();
}
