/* 
2020/05/19
Takayuki Kameoka

ESP32
pin parts
IO  Name       discription
-------------------------------------
36  SENSOR_VP  air pressure sensor 01
39  SENSOR_VN  air pressuer sensor 02
34  IO34       air pressure sensor 03
35  IO35       air pressuer sensor 04
32  IO32       air pressure sensor 05
33  IO33       air pressuer sensor 06
25  IO25       suction valve 01
26  IO26       rerease valve 01
27  IO27       suction valve 02
14  IO14       rerease valve 02
13  IO13       suction valve 03
23  IO23       rerease valve 03
22  IO22       suction valve 04
21  IO21       rerease valve 04
19  IO19       suction valve 05
18  IO18       rerease valve 05
17  IO17       suction valve 06
16  IO16       rerease valve 06
04  IO04       air pump valve

IO0，IO2はプログラム書き込み時に使われるので使用しないほうが良い?

3 way valve (SC415GF)
LOW       HIGH
------    ----x-
  └-x-      └---

2 way valve (SC0526GF)
LOW       HIGH
---x--    ------

*/

#include <Arduino.h>

#define PULSE_SUCTION_WIDTH 3000 //吸引の間隔
#define PULSE_RELEACE_WIDTH 2000 //排気の間隔
#define RANGE 20 //目標気圧との誤差許容範囲

#define SUCTION_POINT_NUM 2 //吸引点の数
int SUCTION_VALVE[] = {25,27,13,22,19,17};
int RELEACE_VALVE[] = {26,14,23,21,18,16};
int SENSOR_PIN[] = {36,39,34,35,32,33};
#define PUMP_VALVE_PIN 4 //吸引ポンプに繋がる3方向電磁弁の制御ピン

//#define LOOP 10 // 生データを時間平滑化するためのループ回数
//int loop_time; //ループ回数
//double loop_raw_pres[SUCTION_POINT_NUM][LOOP]; //時間平滑化のためのデータ保存場所，センサー値（センサー番号）（ループ番号）
//double average_pres[SUCTION_POINT_NUM]; //平滑化したあとの各センサーの値
double each_raw_pres[SUCTION_POINT_NUM]; //各センサーの値

int aim_pres[] = {-0,-500,-300,-300,-300,-300}; //初期目標気圧

bool suction_flag[SUCTION_POINT_NUM] = {false}; //目標気圧より気圧が高いときに吸引を行う
bool pump_valve_flag = false; //吸引ポンプのバルブの開閉を管理するフラグ
bool timer_flag=false; //タイマー割り込みを行うフラグ

//Timer関連セットアップ
hw_timer_t * timer = NULL;
//portMUX_TYPE型の変数で、メインループとISRで共用変数を変更する時の同期処理に使用
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
//メインループとISRで共用するためのカウンターです。コンパイラーの最適化によって消去されないように、volatile宣言をします
volatile uint32_t isrCounter = 0;
volatile uint32_t lastIsrAt = 0;



//気圧センサの値を読み込み，単純時間平均をとる関数
void read_sensor_value(int sensor_num){
  double raw_pres = 0;
  raw_pres = analogRead(SENSOR_PIN[sensor_num]);
  raw_pres = raw_pres/4095*3.3/2.7*(-1000); //単位 mbar (1mbar = 1hPa)に変換

  each_raw_pres[sensor_num]=raw_pres;

//  //計測値をループ回数分保存する
//  loop_time=(loop_time+1)%LOOP;//ループ番号を更新
//  loop_raw_pres[sensor_num][loop_time] = raw_pres;
//
//  //計測値の単純移動平均を計算
//  average_pres[sensor_num] = 0;//平均値の初期化
//  for(int i=0;i<LOOP;i++){
//    Serial.println();  
//    average_pres[sensor_num] += loop_raw_pres[sensor_num][i];
//  }
//  average_pres[sensor_num] = average_pres[sensor_num]/LOOP;

}



//目標気圧値までバルブの開閉を行う関数
void change_valve(int sensor_num){
  if(!suction_flag[sensor_num]){
    if(each_raw_pres[sensor_num]>=aim_pres[sensor_num]+RANGE){//目標気圧+RANGE以上なら吸う
      pump_valve_flag = true;
      digitalWrite(SUCTION_VALVE[sensor_num] , HIGH);
      digitalWrite(RELEACE_VALVE[sensor_num] , LOW);
    }else if(each_raw_pres[sensor_num]>=aim_pres[sensor_num]-RANGE){//目標気圧±RANGE以内なら停止
      digitalWrite(SUCTION_VALVE[sensor_num] , LOW);
      digitalWrite(RELEACE_VALVE[sensor_num] , LOW);
//------------------------------------------------------------------------------
      // suction_flag[sensor_num] = true;//バルブを止めて気圧調整するときはコメントアウトを解除
//      Serial.print("STOP");Serial.print("\t");
//------------------------------------------------------------------------------
    }else{//目標気圧-RANGE以下なら排気
      digitalWrite(SUCTION_VALVE[sensor_num] , LOW);
      digitalWrite(RELEACE_VALVE[sensor_num] , HIGH);
    }
  }else{
    digitalWrite(SUCTION_VALVE[sensor_num] , LOW);
    digitalWrite(RELEACE_VALVE[sensor_num] , HIGH);
  }
}



//全部のバルブを開放して気圧を開放．
void releace(){
  for(int i;i<SUCTION_POINT_NUM;i++){
    digitalWrite(SUCTION_VALVE[i] , LOW);
    digitalWrite(RELEACE_VALVE[i] , HIGH);
    suction_flag[i] = false;
  }
  digitalWrite(PUMP_VALVE_PIN, HIGH);
}



//割り込み処理関数
void IRAM_ATTR onTimer(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  isrCounter++;
//  lastIsrAt = millis();
  portEXIT_CRITICAL_ISR(&timerMux);
  
  //以下割り込み処理

  for(int i=0;i<SUCTION_POINT_NUM;i++){
    //気圧センサーの値を計測
    read_sensor_value(i);
  }

  //排気パルスの判断
  if((isrCounter%(PULSE_SUCTION_WIDTH+PULSE_RELEACE_WIDTH)) < PULSE_SUCTION_WIDTH){
    for(int i=0;i<SUCTION_POINT_NUM;i++){
      // 目標気圧と現在気圧を比較してバルブの開け締め
      change_valve(i);
    }
    //吸引ポンプのバルブの開閉
    if(pump_valve_flag){
      digitalWrite(PUMP_VALVE_PIN, LOW);
      pump_valve_flag = false;
    }else{
      digitalWrite(PUMP_VALVE_PIN, HIGH);
    }
  }else{
    //排気パルスタイミングで排気
    releace();
  }
}

// バルブの動作をチェックする
void test_valve(){
  Serial.println("test valves");
  for(int i;i<SUCTION_POINT_NUM;i++){
    digitalWrite(SUCTION_VALVE[i] , HIGH);
    delay(100);
    digitalWrite(RELEACE_VALVE[i] , HIGH);
    delay(100);
  }
  for(int i;i<SUCTION_POINT_NUM;i++){
    digitalWrite(SUCTION_VALVE[i] , LOW);
    digitalWrite(RELEACE_VALVE[i] , LOW);
  }
}

void setup() {
  //change pin mode
  Serial.begin(115200);
  Serial.println("start setup");
  for(int i=0;i<SUCTION_POINT_NUM;i++){
    pinMode(SUCTION_VALVE[i] , OUTPUT);
    pinMode(RELEACE_VALVE[i] , OUTPUT);
  }

  pinMode(PUMP_VALVE_PIN, OUTPUT);

  //timer set up
  // Use 1st timer of 4 (counted from zero).
  // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more　info).
  timer = timerBegin(0, 80, true);
  // Attach onTimer function to our timer.
  timerAttachInterrupt(timer, &onTimer, true);
  // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter)
  // timer 1ms
  timerAlarmWrite(timer, 1000, true);
  // Start an alarm
  timerAlarmEnable(timer);
  
}

//コマンド入力
char input[30];// コマンド分活一時格納用
int len; //コマンドの長さ
int command[5]; //','で区切った値が入る
String str; //読み込んだ文字列を格納

void loop() {
  if ( Serial.available()) {
    str = Serial.readStringUntil('\r');//改行コマンドLFのみ\n，CRのみ\r
    len = str.length();// コマンドの長さ
    str.toCharArray(input, ++len);// strの中身をchar型配列inputに格納

    str = String(strtok(input, ","));//inputの中身を','で分ける
    command[0] = atoi(strtok(NULL, ","));//  第一引数をここにいれる．第二引数～を作る場合は同様に書く．

    if(str == "0")aim_pres[0]=command[0];
    if(str == "1")aim_pres[1]=command[0];
    
  }


   for(int i=0;i<SUCTION_POINT_NUM;i++){
     Serial.print("finger num is : ");
     Serial.print(i);
     Serial.print("\t");
     Serial.print(aim_pres[i]);
     Serial.print("\t");
     Serial.print(each_raw_pres[i]);
     Serial.print("\t");
   }
   Serial.println();
}