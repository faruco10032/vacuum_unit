/* 
2020/10/19
プロセシングとの通信用のコード
ESP32→Processing　気圧値の生データ
Processing→ESP32　目標気圧値

ESP32
pin parts
32  valve01:suction valve
33  valve02:rerease valve
36  air pressure sensor

3 way valve (SC415GF)
LOW       HIGH
------    ----x-
  └-x-      └---

2 way valve (SC0526GF)
LOW       HIGH
---x--    ------

参考サイト
https://qiita.com/hideakitai/items/347985528656be03b620#3-0---255-%E3%82%88%E3%82%8A%E5%A4%A7%E3%81%8D%E3%81%84%E6%95%B4%E6%95%B0%E3%82%84%E5%B0%8F%E6%95%B0%E3%82%92%E9%80%81%E3%82%8A%E3%81%9F%E3%81%84
*/

#include <Arduino.h>

#define PULSE_SUCTION_WIDTH 1000 //吸引の間隔
#define PULSE_RELEACE_WIDTH 500 //排気の間隔
#define RANGE 1 //目標気圧との誤差許容範囲

#define SENSOR_PIN 36 //気圧センサのピン番号
#define VALVE_NUM 2 //バルブの数
#define SUCTION_POINT_NUM 1 //吸引点の数
int suction_valve_pin[SUCTION_POINT_NUM] = {32};
int release_valve_pin[SUCTION_POINT_NUM] = {33};

float val; //
float raw_pres; //raw air pressure value
int aim_pres = -200; //初期目標気圧
int new_aim_press = -200; //Processingから送られてくる目標気圧

int state = 0; //状態変数，0:初期状態，1:吸引，2:排気，3:停止

bool suction_flag = false; //吸引の調整を行うかどうか判断するフラグfalseで常に解放
bool timer_flag=false; //タイマー割り込みを行うフラグ

bool monitar_flag = false;//シリアルモニタに送信するかどうか判断するフラグmキーで切り替え

//Timer関連セットアップ
hw_timer_t * timer = NULL;
//portMUX_TYPE型の変数で、メインループとISRで共用変数を変更する時の同期処理に使用
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
//メインループとISRで共用するためのカウンターです。コンパイラーの最適化によって消去されないように、volatile宣言をします
volatile uint32_t isrCounter = 0;
volatile uint32_t lastIsrAt = 0;

void suction(){
  digitalWrite(suction_valve_pin[0] , LOW);
  digitalWrite(release_valve_pin[0] , LOW);
}

void stop(){
  digitalWrite(suction_valve_pin[0] , HIGH);
  digitalWrite(release_valve_pin[0] , LOW);
}

void release(){
  digitalWrite(suction_valve_pin[0] , HIGH);
  digitalWrite(release_valve_pin[0] , HIGH);
}

//
void change_valve(){
  if(suction_flag){
    if(raw_pres>=aim_pres+RANGE){//目標気圧+RANGE以上なら吸う
      //直前まで吸ってて系を密閉したいときは停止でループさせる．
      if(state==3){
        stop();
        state=3;
      }else{
        suction();
        state=1;
      }

      // //吸うときはコメントアウトを解除
      // suction();
      // state = 1;

    }else if(raw_pres>=aim_pres-RANGE){//目標気圧±RANGE以内なら停止
      stop();
      state = 3;
    }else{//目標気圧-RANGE以下
      //目標気圧-RANGE以下かつ吸引してた場合は停止してループする
      if(state==1 || state==3){
        stop();
        state=3;
      }else{
        release();
        state=2;
      }
    }
  }else{
    release();
  }
}

//割り込み処理関数
void IRAM_ATTR onTimer(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  isrCounter++;
//  lastIsrAt = millis();
  portEXIT_CRITICAL_ISR(&timerMux);
  
  //以下割り込み処理
  val = analogRead(SENSOR_PIN);
  raw_pres = val/4095*3.3/2.7*(-1000); //単位 mbar (1mbar = 1hPa)

  //排気パルス実行
  if((isrCounter%(PULSE_SUCTION_WIDTH+PULSE_RELEACE_WIDTH)) < PULSE_SUCTION_WIDTH){
    suction_flag = true;
  }else{
    suction_flag = false;
    state = 0;
  }

  // 吸引気圧の調整
  change_valve();
}



void setup() {
  //change pin mode
  Serial.begin(115200);
  for(int i=0;i<SUCTION_POINT_NUM;i++){
    pinMode(suction_valve_pin[i] , OUTPUT);
    pinMode(release_valve_pin[i] , OUTPUT);
  }

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


void loop() {

  if ( Serial.available()>0) {

    byte dataH = (byte)Serial.read();
    byte dataL = (byte)Serial.read();

    int data = (int) (
          (((int)dataH << 6) & 0x000003FF)
        | (((int)dataL << 0) & 0x0000003F)
    );
    while(Serial.read()!=0xff);

    //目標気圧が変化した場合初期状態に戻す
    if(-data!=aim_pres)state = 0;
    //目標気圧の更新
    aim_pres = -data;
  }

    int data = -raw_pres;
    if(raw_pres>0)data = 0;

    //計測した気圧の生データを送る
    //データを2byteに分割して先頭2bitは00にする
    uint8_t dataH = (uint8_t)((data & 0x000003FF) >>  6);
    uint8_t dataL = (uint8_t)((data & 0x0000003F) >>  0);

    Serial.write(dataH);
    Serial.write(dataL);
    Serial.write(0xff);//フッターを送る
  
}