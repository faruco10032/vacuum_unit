/* 

ESP32
pin parts
IO  Name       discription
-------------------------------------
36  SENSOR_VP  air pressure sensor 01
39  SENSOR_VN  air pressuer sensor 02
34  IO04       air pressure sensor 03
35  IO25       air pressuer sensor 04
32  IO26       air pressure sensor 05
33  IO27       air pressuer sensor 06
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

IO0 and IO2 are used for program writing and should not be used
*/

#include <Arduino.h>

#define PULSE_SUCTION_WIDTH 3000 //suction duration
#define PULSE_RELEACE_WIDTH 2000 //exhaust duration
#define RANGE 1 //Buffer with target pressure

#define SUCTION_POINT_NUM 2 //吸引点の数
int SUCTION_VALVE[] = {25,27,13,22,19,17};
int RELEACE_VALVE[] = {26,14,23,21,18,16};
int SENSOR_PIN[] = {36,39,34,35,32,33};

volatile double each_raw_pres[SUCTION_POINT_NUM];

volatile int aim_pres[] = {0,0,0,0,0,0}; //first aim pressure

bool suction_flag[SUCTION_POINT_NUM] = {false}; //suction in aim_presure > raw_pressure
bool timer_flag=false; //Flag for timer interrupt

//Timer setup
hw_timer_t * timer = NULL;
//portMUX_TYPE型の変数で、メインループとISRで共用変数を変更する時の同期処理に使用
//portMUX_TYPE type variable, used for synchronous processing when changing shared variables in the main loop and ISR
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
//メインループとISRで共用するためのカウンターです。コンパイラーの最適化によって消去されないように、volatile宣言をします
// Counter to be shared by main loop and ISR. Declare volatile to prevent erasure by compiler optimizations
volatile uint32_t isrCounter = 0;
volatile uint32_t lastIsrAt = 0;

TaskHandle_t thp[2];
//マルチスレッドのタスクハンドルのポインタ格納用
// For storing pointers to multi-threaded task handles

volatile bool getUnityData = false;

//Unityからのデータ格納用
// For storing data from Unity
byte hedder;
byte sig1;
byte sig2;

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
  if(suction_flag[sensor_num]){
    if(each_raw_pres[sensor_num]>=aim_pres[sensor_num]+RANGE){//目標気圧+RANGE以上なら吸う
      digitalWrite(SUCTION_VALVE[sensor_num] , LOW);
      digitalWrite(RELEACE_VALVE[sensor_num] , LOW);
      // Serial.println("suction now");
    }else if(each_raw_pres[sensor_num]>=aim_pres[sensor_num]-RANGE){//目標気圧±RANGE以内なら停止
      digitalWrite(SUCTION_VALVE[sensor_num] , HIGH);
      digitalWrite(RELEACE_VALVE[sensor_num] , LOW);
//------------------------------------------------------------------------------
      suction_flag[sensor_num] = false;//バルブを止めて気圧調整するときはコメントアウトを解除
//      Serial.print("STOP");Serial.print("\t");
        // Serial.println("stop now");
//------------------------------------------------------------------------------
    }else{//目標気圧-RANGE以下なら排気
      digitalWrite(SUCTION_VALVE[sensor_num] , HIGH);
      digitalWrite(RELEACE_VALVE[sensor_num] , HIGH);
      // Serial.println("open now");
    }
  }else{//suction_flagがfalseなら排気
    // digitalWrite(SUCTION_VALVE[sensor_num] , HIGH);
    // digitalWrite(RELEACE_VALVE[sensor_num] , HIGH);

    //suction_flagがfalseなら停止
    digitalWrite(SUCTION_VALVE[sensor_num] , HIGH);
    digitalWrite(RELEACE_VALVE[sensor_num] , LOW);
  }
}



//全部のバルブを開放して気圧を開放．
void releace(){
  for(int i=0;i<SUCTION_POINT_NUM;i++){
    digitalWrite(SUCTION_VALVE[i] , HIGH);
    digitalWrite(RELEACE_VALVE[i] , HIGH);
    // suction_flag[i] = false;
  }
}



// //割り込み処理関数
// void IRAM_ATTR onTimer(){
//   // Increment the counter and set the time of ISR
//   portENTER_CRITICAL_ISR(&timerMux);
//   isrCounter++;
// //  lastIsrAt = millis();
//   portEXIT_CRITICAL_ISR(&timerMux);
  
//   //以下割り込み処理

// //   if ( Serial.available()>0) {
// //     sig2 = Serial.read();
// //     sig1 = Serial.read(); 
// //     hedder = Serial.read();//hedder
// //     int type = sig1>>3;
// //     int AirPressureValue = sig1&0x07;
// //     AirPressureValue = AirPressureValue<<7;
// //     AirPressureValue = AirPressureValue + sig2;

// // //    Serial.print("hedder is:");
// // //    Serial.print(hedder);
// // //    Serial.print(" value is :");
// // //    Serial.print(AirPressureValue);
// // //    Serial.print(" type is :");
// // //    Serial.println(type);

// //     aim_pres[type]=-AirPressureValue;
// //     suction_flag[type]=true;

// //   //   for(int i=0;i<SUCTION_POINT_NUM;i++){
// //   //     Serial.print("finger num is : ");
// //   //     Serial.print(i);
// //   //     Serial.print("\t");
// //   //     Serial.print(aim_pres[i]);
// //   //     Serial.print("\t");
// //   //     Serial.print(each_raw_pres[i]);
// //   //     Serial.print("\t");
// //   //   }
// //   //   Serial.println();
// //   }
  
  
//   // for(int i=0;i<SUCTION_POINT_NUM;i++){
//   //   //気圧センサーの値を計測
//   //   read_sensor_value(i);
//   //   // 気圧の調整
//   //   change_valve(i);
//   // }

//   // // 気圧の調整
//   // for(int i=0;i<SUCTION_POINT_NUM;i++){
//   //   change_valve(i);
//   // }

//   // //排気パルス実行
//   // if((isrCounter%(PULSE_SUCTION_WIDTH+PULSE_RELEACE_WIDTH)) < PULSE_SUCTION_WIDTH){
//   //   for(int i=0;i<SUCTION_POINT_NUM;i++){
//   //     change_valve(i);
//   //   }
//   // }else{
//   //   releace();
//   // }
  
// }

// バルブの動作をチェックする
void test_valve(){
  Serial.println("test valves");
  for(int i=0;i<SUCTION_POINT_NUM;i++){
    digitalWrite(SUCTION_VALVE[i] , HIGH);
    delay(100);
    digitalWrite(RELEACE_VALVE[i] , HIGH);
    delay(100);
  }
  for(int i=0;i<SUCTION_POINT_NUM;i++){
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

  // //timer set up
  // // Use 1st timer of 4 (counted from zero).
  // // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more　info).
  // timer = timerBegin(0, 80, true);
  // // Attach onTimer function to our timer.
  // timerAttachInterrupt(timer, &onTimer, true);
  // // Set alarm to call onTimer function every second (value in microseconds).
  // // Repeat the alarm (third parameter)
  // // timer 1ms
  // timerAlarmWrite(timer, 1000, true);
  // // Start an alarm
  // timerAlarmEnable(timer);
  
  // マルチコアのスレッド作成
  xTaskCreatePinnedToCore(Core0a, "Core0a", 4096, NULL, 3, &thp[0], 1);
  xTaskCreatePinnedToCore(Core0b, "Core0b", 4096, NULL, 2, &thp[1], 0);
}

  

void loop() {

  for(int i=0;i<SUCTION_POINT_NUM;i++){
    //気圧センサーの値を計測
    read_sensor_value(i);
    // 気圧の調整
    change_valve(i);
  }


  
  // int type;
  // int AirPressureValue;


  // if(Serial.available()){
  //   // 読み込み続ける
  //   byte temp = Serial.read();
    
  //   if(temp != 0xff){
  //     // 読み込んだデータをずらして記録していく
  //     sig2 = sig1;
  //     sig1 = temp;
  //     // Serial.print(temp);
  //   }else{
  //     type = sig1>>3;
  //     AirPressureValue = sig1&0x07;
  //     AirPressureValue = AirPressureValue<<7;
  //     AirPressureValue = AirPressureValue + sig2;


  //     // Serial.print("type is ");
  //     // Serial.print(type);
  //     // Serial.print("\t");
  //     // Serial.print("hardness is ");
  //     // Serial.print(AirPressureValue);
  //     // Serial.println();

  //     aim_pres[type]=-AirPressureValue;
  //     suction_flag[type]=true;
  //   }
  //   // //0xffのヘッダーを探す
  //   // if(Serial.read()==0x00){
  //   //   sig2 = Serial.read();
  //   //   sig1 = Serial.read(); 
  //   //   int type = sig1>>3;
  //   //   int AirPressureValue = sig1&0x07;
  //   //   AirPressureValue = AirPressureValue<<7;
  //   //   AirPressureValue = AirPressureValue + sig2;

  //   //   aim_pres[type]=-AirPressureValue;
  //   //   suction_flag[type]=true;
  //   // }
  // }
  

//  if ( Serial.available()) {
//    int suction_value =100;
//    int finger_data = 0;
//    //0x00のフッターを探す
//    // while(Serial.read()==0x00)
//
//    byte sig1 = Serial.read();
//    //デバッグ
//    Serial.print("get sig is :");
//    Serial.println(sig1);
//    // byte sig2 = Serial.read();
//
//    // finger_data = sig >> 5; //上位3bitにどの指の情報かが入っている
//    // suction_value = sig & 31; //下位5bitに吸引の強度情報が入っている
//    //    byte a = finger_data + suction_value;
//    int finger_num = finger_data - 1;//吸引点と指の番号を対応させる
//
//    //map関数を使って吸引気圧を初期吸引目標値を参考に31段階で変換する
//    suction_value = map(suction_value, 0, 31, 0, -500);
//
//    // 吸引値を更新する
//    aim_pres[finger_num]=suction_value;
//    
//    if(suction_value>0){
//      suction_flag[finger_num]=true;
//    }else{
//      suction_flag[finger_num]=false;
//    }
//  }
  
//  for(int i=0;i<SUCTION_POINT_NUM;i++){
//    Serial.print("finger num is : ");
//    Serial.print(i);
//    Serial.print("\t");
//    Serial.print(aim_pres[i]);
//    Serial.print("\t");
//    Serial.print(each_raw_pres[i]);
//    Serial.print("\t");
//  }
// //  Serial.println(loop_raw_pres[0][0]);
//  Serial.println();
}

// マルチコア処理
void Core0a(void *args){
  
  // 連続処理、Loopに相当
  while (1)
  {
    int type;
    int AirPressureValue;
    /* code */
    if(Serial.available()){
      // 読み込み続ける
      byte temp = Serial.read();
      
      if(temp != 0xff){
        // 読み込んだデータをずらして記録していく
        sig2 = sig1;
        sig1 = temp;
        // Serial.print(temp);
      }else{
        type = sig1>>3;
        AirPressureValue = sig1&0x07;
        AirPressureValue = AirPressureValue<<7;
        AirPressureValue = AirPressureValue + sig2;

        // aim_pres[type]=-AirPressureValue;
        // suction_flag[type]=true;

        // for(int i=0;i<SUCTION_POINT_NUM;i++){
        //   Serial.print("finger num is : ");
        //   Serial.print(i);
        //   Serial.print("\t");
        //   Serial.print(aim_pres[i]);
        //   Serial.print("\t");
        //   Serial.print(each_raw_pres[i]);
        //   Serial.print("\t");
        // }
        // Serial.println();

        // 前回と気圧が違うときだけ値を変化させる
        if(aim_pres[type] != -AirPressureValue){
          aim_pres[type]=-AirPressureValue;
          suction_flag[type]=true;
        }

        getUnityData = true;
      }
    }



    // これを入れないとwatchdogにヤラレル
		//	正確には1msではなく vTaskDelay(ms / portTICK_PERIOD_MS) らしい
		delay(1);
  }
}

void Core0b(void *args){
  while(1){
    if(getUnityData){
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

      getUnityData  = false;
    }
    

    delay(100);
  }
}
